/* Phasor - Halo PC Server Extension
   Copyright (C) 2010-2011  Oxide (http://haloapps.wordpress.com)
  
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#include "Game.h"
#include "Halo.h"
#include "Lua.h"
#include "Server.h"
#include "Common.h"
#include "Teams.h"
#include "objects.h"
#include "Alias.h"
#include "Messages.h"
#include "Addresses.h"
#include <map>
#include <algorithm>

namespace game
{
	using namespace halo;
	std::map<int, halo::h_player*> playerMap;

	// Returns the data associated with the specified player
	halo::h_player* GetPlayer(int m_playerid)
	{
		if (playerMap.find(m_playerid) == playerMap.end())
			return 0;
		else
			return playerMap[m_playerid];
	}

	// Returns the data associated with the specified player
	halo::h_player* GetPlayer_pi(int p_playerid)
	{
		halo::h_player* player = 0;

		for (std::map<int, halo::h_player*>::iterator itr = playerMap.begin();
			itr != playerMap.end(); itr++)
		{
			if (itr->second->mem->playerNum == p_playerid)
			{
				player = itr->second;
				break;
			}
		}

		return player;
	}

	// gets the player who owns the object
	h_player* GetPlayer_obj(PObject* m_object)
	{
		halo::h_player* player = 0;

		if (!m_object)
			return 0;

		for (std::map<int, halo::h_player*>::iterator itr = playerMap.begin();
			itr != playerMap.end(); itr++)
		{
			if (itr->second->m_object == m_object)
			{
				player = itr->second;
				break;
			}
		}

		return player;
	}

	// Returns a list of all current players
	std::vector<h_player*> GetPlayerList()
	{
		std::vector<h_player*> output;

		for (std::map<int, halo::h_player*>::iterator itr = playerMap.begin();
			itr != playerMap.end(); itr++)
			output.push_back(itr->second);

		return output;
	}

	// Gets the gametype's respawn ticks
	DWORD GetRespawnTicks()
	{
		return *(DWORD*)(ADDR_GAMETYPE + OFFSET_RESPAWNTICKS);
	}

	// Called when a game stage ends
	void __stdcall OnEnd(DWORD mode)
	{
		switch (mode)
		{
		case 1: // just ended (in game scorecard is shown)
			{
				afksystem::GameEnd();
				messages::automated::GameEnd();

				logging::LogData(LOGGING_GAME, L"\tGAME\tEND\t\tThe game has ended.");
				halo::hprintf(L"The game is ending...");

			} break;

		case 2: // post game carnage report
			{

			} break; 

		case 3: // players can leave
			{
				maps::vote::OnBegin();

			} break;
		}

		Lua::funcCall("OnGameEnd", "l", mode);
	}

	// Called when a new game starts
	void __stdcall OnNew(char* map)
	{
		// Cleanup the player map
		for (std::map<int, halo::h_player*>::iterator itr = playerMap.begin();
			itr != playerMap.end(); itr++)
			delete itr->second;

		playerMap.clear();

		#ifdef PHASOR_PC
			// Fix the map name
			maps::FixMapName();
			map = (char*)maps::GetCurrentMap();
		#endif
		
		// Notify any functions
		afksystem::GameStart();
		objects::OnGameStart();
		messages::automated::GameStart();

		hprintf("A new game has started on map %s", map);
		logging::LogData(LOGGING_GAME, "\tGAME\tSTART\t\tA new game has started on map %s", map);

		// Load the scripts
		game::maps::scriptmanager::Load();
		Lua::funcCall("OnNewGame", "s", map);
	}

	// Called when a player joins (after verification).
	void __stdcall OnPlayerWelcome(DWORD playerId)
	{
		// Create a new player
		halo::h_player* player = new halo::h_player(playerId);

		// Make sure the player's memory was found correctly
		if (player->Validate())
		{
			// Make sure this player doesn't already exist in the map (it SHOULDN'T).
			if (playerMap.find(player->memoryId) != playerMap.end())
			{
				logging::LogData(LOGGING_PHASOR, L"JOIN: PLAYER ALREADY EXISTS IN MEMORY MAP! OVERWRITING CURRENT ENTRY: %i", player->memoryId);
				delete playerMap[player->memoryId];
			}
			// Add to the map and notify other functions of the event
			playerMap[player->memoryId] = player;
			alias::OnPlayerJoin(player);
			messages::welcome::OnPlayerJoin(player);

			halo::hprintf(L"%s (player %i) has joined the game.", player->mem->playerName, player->mem->playerNum + 1);
			logging::LogData(LOGGING_GAME, L"\tPLAYER\tJOIN\t\t%s (%s)", player->mem->playerName, player->whash.c_str());

			Lua::funcCall("OnPlayerJoin", "ll", player->memoryId, player->mem->team);
		}
		else
		{
			logging::LogData(LOGGING_PHASOR, L"JOIN: INVALID PLAYER INDEX GIVEN! CANNOT PROCESS PLAYER: %i", player->memoryId);
			delete player;
		}
	}

	// Called when a player quits
	void __stdcall OnPlayerQuit(DWORD playerId)
	{
		// Remove the player from the map
		std::map<int, halo::h_player*>::iterator itr = playerMap.find(playerId);

		if (itr != playerMap.end())
		{
			halo::h_player* player = itr->second;

			// log the event/notify scripts
			halo::hprintf(L"%s (player %i) has left the game.", player->mem->playerName, player->mem->playerNum + 1);
			logging::LogData(LOGGING_GAME, L"\tPLAYER\tLEAVE\t\t%s (%s)", player->mem->playerName, player->whash.c_str());
			
			Lua::funcCall("OnPlayerLeave", "ll", player->memoryId, player->mem->team);

			// cleanup
			delete player;
			playerMap.erase(itr);
		}
		else
		{
			logging::LogData(LOGGING_PHASOR, L"LEAVE: COULDN'T FIND LEAVING PLAYER: %i", playerId);
		}
	}

	// Called when a player's team is being assigned
	DWORD __stdcall OnTeamSelection(DWORD cur_team)
	{
		unsigned long new_team = Lua::rfuncCall(cur_team, "OnTeamDecision", "l", cur_team);
		return new_team;
	}

	// Called when a player tries to change team
	bool __stdcall OnTeamChange(DWORD playerId, DWORD team)
	{
		int canChange = true;

		halo::h_player* player = GetPlayer(playerId);

		if (player)
		{
			// see if phasor is blocking team changes
			if (teams::CanChange())
			{
				// notify scripts
				canChange = Lua::rfuncCall(1, "OnTeamChange", "illl", true, playerId, player->mem->team, team);
			}
			else
			{
				canChange = false;

				// Message the player
				player->ServerMessage(L"The host has disabled team changing.");
			}
	
			// log the event
			logging::LogData(LOGGING_GAME, L"\tPLAYER\tCHANGE\t\t%s\t\t%s\t%s", player->mem->playerName, 
				(player->mem->team == 1) ? L"Red" : L"Blue", (canChange == 1) ? "ALLOWED" : "BLOCKED");	
		}
		
		return (canChange == 1) ? true:false;
	}

	// Called when a player is about to spawn (object already created)
	void __stdcall OnPlayerSpawn(DWORD playerId, DWORD m_objectId)
	{
		halo::h_player* player = GetPlayer(playerId);

		if (player)
		{
			player->m_object = objects::GetObjectAddress(3, m_objectId);
			halo::hprintf("Spawn at: %08X", player->m_object);

			Lua::funcCall("OnPlayerSpawn", "ll", playerId, m_objectId);		
		}
	}

	// Called after the server has been notified of a player spawn
	void __stdcall OnPlayerSpawnEnd(DWORD playerId, DWORD m_objectId)
	{
		halo::h_player* player = GetPlayer(playerId);

		if (player)
		{
			player->m_object = objects::GetObjectAddress(3, m_objectId);
			Lua::funcCall("OnPlayerSpawnEnd", "ll", playerId, m_objectId);
		}
	}

	// Called when a weapon is created
	void __stdcall OnObjectCreation(DWORD m_objectId)
	{
		PObject* m_object = objects::GetObjectAddress(3, m_objectId);
		if (m_object)
		{		
			DWORD owner = m_object->base.ownerPlayer;
			// it is the player mask, only want id
			if (owner != -1)
				owner &= 0xffff;

			// Lookup the tag
			objects::hTagHeader* header = objects::LookupTag(m_object->base.mapId);

			if (header)
				Lua::funcCall("OnObjectCreation", "lis", m_objectId, owner, header->tagName);
		}
	}

	// Called when a weapon is assigned to an object
	DWORD __stdcall OnWeaponAssignment(DWORD playerId, DWORD owningObjectId, objects::objInfo* curWeapon, DWORD order)
	{
		// not all weapons are assigned to players (ie vehicles)
		if (!GetPlayer(playerId))
			playerId = -1;

		objects::hTagHeader* newWeapon = (objects::hTagHeader*)Lua::rfuncCall(0, "OnWeaponAssignment", "llls", playerId, owningObjectId, order, curWeapon->tagName);
		
		DWORD weaponId = 0;
		if (newWeapon)
			weaponId = newWeapon->id;

		return weaponId;
	}

	// Called when a player can interact with an object
	bool __stdcall OnObjectInteraction(DWORD playerId, DWORD m_ObjId)
	{
		int allow = 1;

		// find the object and lookup its map id
		PObject* obj = objects::GetObjectAddress(3, m_ObjId);

		if (obj)
		{
			objects::hTagHeader* header = objects::LookupTag(obj->base.mapId);

			if (header)
			{
				// copy the tag into a string buffer, header->tagType is like
				// paew, we want as weap/0
				char text_type[5] = {0};
				DWORD en = objects::endian_swap(header->tagType);
				memcpy(text_type, &en, 4);

				allow = Lua::rfuncCall(1, "OnObjectInteraction", "llss", playerId, m_ObjId, text_type, header->tagName);
			}			
		}

		return (allow == 1) ? true:false;
	}

	// Called when an objects position is updated
	void __stdcall OnClientUpdate(PObject* m_object)
	{
		halo::h_player* player = GetPlayer_obj(m_object);

		if (player)
		{
			player->CheckCameraMovement();

		/*	if (player->m_object)
			{
				player->m_object->biped.actionFlags.secondaryWeaponFire = 0;
				player->m_object->biped.actionFlags.secondaryWeaponFire1 = 0;
				player->m_object->biped.actionFlags.crouching = 0;
				player->m_object->biped.actionFlags.actionPress = 0;
				player->m_object->biped.actionFlags.actionHold = 0;
				player->m_object->biped.actionFlags.reload = 0;
				halo::hprintf("%02X", m_object->biped.isAirbourne);
			}*/

			Lua::funcCall("OnClientUpdate", "ll", player->memoryId, player->mem->m_playerObjectid);
		}
	}

	// used to replace a damage tag after processing
	BYTE m_dmg_info[0x2A1] = {0};
	LPBYTE dmgInfo_addr = 0;

	// Called when an object's damage is being looked up
	void __stdcall OnDamageLookup(DWORD receivingObj, DWORD causingObj, LPBYTE tagEntry)
	{
		// Get the address of the tag's metadata
		objects::hTagHeader* header = (objects::hTagHeader*)tagEntry;

		// Check if we need to overwrite the tag data
		if (dmgInfo_addr)
		{
			// Replace the data with the original
			memcpy(dmgInfo_addr, m_dmg_info, 0x2A0);
		}

		// Save info for rewriting the tag
		dmgInfo_addr = header->metaData;
		memcpy(m_dmg_info, header->metaData, 0x2A0);

		Lua::funcCall("OnDamageLookup", "llls", receivingObj, causingObj, header->metaData, header->tagName);
	}

	// used to control whether or not the player's number is prepended to chat
	bool bChatNumbers = true;

	// Called when someone chats in the server
	void __stdcall OnChat(chatData* chat)
	{
		halo::h_player* sender = GetPlayer(chat->player);

		if (!sender)
			return;
			
		// they're obviously not afk if they're chatting
		sender->ResetAfkTimer();

		// Let scripts process it
		if (!Lua::rfuncCall(1, "OnServerChat", "lls", chat->player, chat->type, ToChar(chat->msg).c_str()))
			return;

		if (!maps::vote::OnServerChat(sender, chat->msg))
			return;

		// process the chat
		if (bChatNumbers)
			server::SendChat(chat->type, sender, L"(%i) %s", sender->mem->playerNum + 1, chat->msg);
		else
			server::SendChat(chat->type, sender, L"%s", chat->msg);
	}

	// Called when a player attempts to enter a vehicle
	bool __stdcall OnVehicleEntry(DWORD playerId)
	{
		halo::h_player* player = game::GetPlayer(playerId);
		int allow = true;

		if (player)
		{
			// the player is interacting with the vehicle they're entering, 
			// so get its id and then find its memory
			DWORD dwVehicleId = player->mem->m_interactionObject;
			PObject* obj = objects::GetObjectAddress(3, dwVehicleId);
			char* vehicle_tag = "";
			DWORD vehicle_seat = player->mem->interactionSpecifier;

			if (obj)
			{
				// get the vehicle's mapId and then find its tag, so we 
				// can determine which type of vehicle it is entering
				objects::hTagHeader* header = objects::LookupTag(obj->base.mapId);
				
				if (header)
					vehicle_tag = header->tagName;				
			}

			allow = Lua::rfuncCall(true, "OnVehicleEntry", "lllsl", !player->force_entered, playerId, dwVehicleId, vehicle_tag, vehicle_seat);
		
			if (player->force_entered)
				allow = true;
		}

		// return vealue specifies whether or not they're allowed to enter
		return (allow == 1)? true:false;
	}

	// Called when a player is being ejected from a vehicle
	bool __stdcall OnVehicleEject(PObject* m_playerObject, bool forceEjected)
	{
		int allow = true;

		halo::h_player* player = game::GetPlayer_obj(m_playerObject);

		if (player)
			allow = Lua::rfuncCall(true, "OnVehicleEject", "lb", player->memoryId, forceEjected);
		
		return (allow == 1)? true:false;
	}

	// Called when a player dies
	void __stdcall OnDeath(DWORD killerId, DWORD victimId, DWORD mode)
	{
		halo::h_player* victim = GetPlayer(victimId);
		halo::h_player* killer = GetPlayer(killerId);
		
		if (victim)
		{
			// log the death based on type
			switch (mode)
			{
			case 1: // fall dmg or server kill
				{
					if (victim->sv_killed)
					{
						// Change the mode to indicate a server kill (not fall damage)
						mode = 0;
					}

					logging::LogData(LOGGING_GAME, L"\tGAME\tDEATH (%i)\t%s died.", mode, victim->mem->playerName);
				} break;

			case 2: // killed by guardians
				{
					logging::LogData(LOGGING_GAME, L"\tGAME\tDEATH (%i)\t%s was killed by the guardians.", mode, victim->mem->playerName);
				} break;

			case 3: // killed by a vehicle
				{
					logging::LogData(LOGGING_GAME, L"\tGAME\tDEATH (%i)\t%s was killed by a vehicle.", mode, victim->mem->playerName);
				} break;

			case 4: // killed by another player
				{
					if (killer)
						logging::LogData(LOGGING_GAME, L"\tGAME\tDEATH (%i)\t%s was killed by %s.", mode, victim->mem->playerName, killer->mem->playerName);
				} break;

			case 5: // betrayed
				{
					if (killer)
						logging::LogData(LOGGING_GAME, L"\tGAME\tDEATH (%i)\t%s was betrayed by %s.", mode, victim->mem->playerName, killer->mem->playerName);

				} break;

			case 6: // suicide
				{
					logging::LogData(LOGGING_GAME, L"\tGAME\tDEATH (%i)\t%s committed suicide.", mode, victim->mem->playerName);

				} break;
			}

			// Notify the scripts
			Lua::funcCall("OnPlayerKill", "lll", killerId, victimId, mode);	

			// their object will be disassociated soon, do our one now
			victim->m_object = 0;
		}
	}

	// Called when a player gets a double kill, spree etc
	void __stdcall OnKillMultiplier(DWORD playerId, DWORD multiplier)
	{
		// multiplier values
		/*switch(multiplier)
	{
	case 0x10: // double kill with score
	case 7:  // double kill
		{
			hprintf("Double kill");

		}break;

	case 0x0f: // with score
	case 9: // triple kill
		{
			hprintf("Triple kill");

		} break;

	case 0x0a: // with score
	case 0x0e: // killtacular
		{
			hprintf("Killtacular");

		} break;

	case 0x12: // with score
	case 0x0b: // killing spree
		{
			hprintf("Killing spree");

		} break;

	case 0x11: // with score
	case 0x0c: // running riot
		{
			hprintf("Running riot");

		} break;
	}*/

		// Notify the lua script
		Lua::funcCall("OnKillMultiplier", "ll", playerId, multiplier);
	}

	// Called when a weapon is reloaded
	bool __stdcall OnWeaponReload(DWORD m_WeaponId)
	{
		DWORD dwPlayerId = -1;
		std::vector<halo::h_player*> players = GetPlayerList();

		// This function only gets the player's weapon id, so look through
		// players and find the one who is currently using this weapon
		for (std::vector<halo::h_player*>::iterator itr = players.begin();
			itr < players.end(); itr++)
		{
			if ((*itr)->m_object)
			{
				if ((*itr)->m_object->base.player_curWeapon)
				{
					dwPlayerId = (*itr)->memoryId;
					break;
				}
			}
		}
		
		int allow = 1;
		allow = Lua::rfuncCall(1, "OnWeaponReload", "ll", dwPlayerId, m_WeaponId);		

		return (allow == 1)? true:false;
	}

	// --------------------------------------------------------------------
	// SERVER COMMANDS
	bool sv_chatids(halo::h_player* execPlayer, std::vector<std::string>& tokens)
	{
		if (tokens.size() == 2)
		{
			std::transform(tokens[1].begin(), tokens[1].end(), tokens[1].begin(), tolower);
			if (tokens[1] == "true" || tokens[1] == "1")
			{
				bChatNumbers = true;
				halo::hprintf(L"Chat numbers will be displayed in chat");				
			}
			else if (tokens[1] == "false" || tokens[1] == "0")
			{
				bChatNumbers = false;
				halo::hprintf(L"Chat numbers will no longer appear in chat");
			}
			else
				halo::hprintf(L"Correct usage: sv_chatids <status> options: true, false");
		}
		else
			halo::hprintf(L"Correct usage: sv_chatids <status> options: true, false");

		return true;
	}
}