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

#include "Server.h"
#include "Addresses.h"
#include "Game.h"
#include "Maps.h"
#include "Halo.h"
#include "Common.h"
#include "Timers.h"
#include "main.h"
#include "Admin.h"
#include "PhasorServer.h"
#include "Teams.h"
#include "Alias.h"
#include "Lua.h"
#include "Messages.h"
#include "objects.h"
#include "main.h"

namespace server
{
	using namespace game;

	// ---------------------------------------------------------------------
	// GENERAL SERVER COMMANDS
	// I'll find a better place to put these commands in next release,
	// i'm rushing now...
	bool sv_quit(halo::h_player* execPlayer, std::vector<std::string>& tokens)
	{
		OnClose();
		return false;
	}

	bool sv_getobject(halo::h_player* execPlayer, std::vector<std::string>& tokens)
	{
		if (tokens.size() == 2)
		{
			int playerNum = atoi(tokens[1].c_str()) - 1;
			halo::h_player* player = game::GetPlayer_pi(playerNum);

			if (player)
				halo::hprintf("The player's object is at: %08X", player->m_object);
			else
			{
				DWORD obj = strtoul(tokens[1].c_str(), 0, 16);
				PObject* m_obj = objects::GetObjectAddress(0, obj);
				halo::hprintf("The object is at: %08X", m_obj);
			}
		}
		else
			halo::hprintf("Syntax: sv_getobject <player or obj id>");

		return true;
	}

	bool sv_setspeed(halo::h_player* execPlayer, std::vector<std::string>& tokens)
	{
		if (tokens.size() == 3)
		{
			int playerNum = atoi(tokens[1].c_str()) - 1;
			halo::h_player* player = game::GetPlayer_pi(playerNum);

			if (player)
			{
				float newSpeed = (float)atof(tokens[2].c_str());
				player->mem->speed = newSpeed;

				halo::hprintf("The players speed has been changed.");
			}
			else
				halo::hprintf("The player doesn't exist.");
		}
		else
			halo::hprintf("Syntax: sv_setspeed <player> <speed>");

		return true;

	}

	bool sv_invis(halo::h_player* execPlayer, std::vector<std::string>& tokens)
	{
		if (tokens.size() == 2 || tokens.size() == 3)
		{
			int playerNum = atoi(tokens[1].c_str()) - 1;
			halo::h_player* player = game::GetPlayer_pi(playerNum);

			if (player)
			{
				float duration = 0;

				if (tokens.size() == 3)
				{
					duration = (float)atof(tokens[2].c_str());

					if (duration > 1092.00)
						halo::hprintf("That duration is so large that it is treated as infinite.");
				}

				player->ApplyCamo(duration);
				halo::hprintf("The player is now invisible.");
			}
			else
				halo::hprintf("The player doesn't exist");
		}
		else
			halo::hprintf("Correct usage: sv_invis <player> opt:<duration>");

		return true;
	}

	bool sv_say(halo::h_player* execPlayer, std::vector<std::string>& tokens)
	{
		if (tokens.size() == 2)
			GlobalMessage(L"%s", ToWideChar(tokens[1]).c_str());
		else
			halo::hprintf(L"Correct usage: sv_say <message>");

		return true;
	}

	bool sv_spawn(halo::h_player* execPlayer, std::vector<std::string>& tokens)
	{
		if (tokens.size() == 3 && execPlayer)
		{
			vect3d loc = {0};
			if (execPlayer && execPlayer->m_object)
			{
				loc = execPlayer->m_object->base.location;
				loc.x += 2;
				loc.z += 3;
			}

			DWORD obj = objects::SpawnObject(tokens[1].c_str(), tokens[2].c_str(), 0, -1, 
				false, &loc);

			if (obj)
			{
				PObject* m_obj = objects::GetObjectAddress(3, obj);
				halo::hprintf(L"Spawned object (%08X)", m_obj);
				//halo::hprintf(L"Assignment success: %s", objmng::AssignPlayerWeapon(execPlayer, obj)?L"true":L"false");
			}
			else
				hprintf("Couldn't spawn object");

			//0052C4F0 >/$  8B15 30376200 MOV EDX,DWORD PTR DS:[623730]            ;  arg1 = parent id, arg2 = tag id for object, eax = input/output (0x90 bytes in length)
			//	0052C600 >/$  81EC 1C020000 SUB ESP,21C                              ;  arg1 = mode (integer, only seen 0/3), arg2 = output from BuildReq
		}
		else
			halo::hprintf("Correct usage: sv_spawn <tag type> <tag name>");

		return true;
	}

	bool sv_gethash(halo::h_player* execPlayer, std::vector<std::string>& tokens)
	{
		if (tokens.size() == 2)
		{
			halo::h_player* player = game::GetPlayer_pi(atoi(tokens[1].c_str())-1);

			if (player)
				halo::hprintf(L"%s - %s", player->mem->playerName, player->whash.c_str());
			else
				halo::hprintf(L"You entered an invalid player index.");
		}
		else
			halo::hprintf(L"Correct usage: sv_gethash <player>");

		return true;
	}

	bool sv_kill(halo::h_player* execPlayer, std::vector<std::string>& tokens)
	{
		if (tokens.size() == 2)
		{
			halo::h_player* player = game::GetPlayer_pi(atoi(tokens[1].c_str())-1);

			if (player)
			{
				player->Kill();
				halo::hprintf(L"The player has been killed.");
			}
			else
				halo::hprintf(L"You entered an invalid player index.");
		}
		else
			halo::hprintf(L"Correct usage: sv_kill <player>");

		return true;
	}

	PhasorCommands function_lookup_table[] =
	{
		// hooked functions
		{&sv_quit, "quit"},
		{&maps::scriptmanager::sv_end_game, "sv_end_game"},
		{&maps::scriptmanager::sv_mapcycle_begin, "sv_mapcycle_begin"},
		{&maps::scriptmanager::sv_mapcycle_add, "sv_mapcycle_add"},
		{&maps::scriptmanager::sv_mapcycle_del, "sv_mapcycle_del"},
		{&maps::scriptmanager::sv_mapcycle, "sv_mapcycle"},
		{&maps::scriptmanager::sv_map, "sv_map"},
		{&maps::scriptmanager::sv_reloadscripts, "sv_reloadscripts"},

		// map vote
		{&maps::vote::sv_mapvote, "sv_mapvote"},
		{&maps::vote::sv_mapvote_begin, "sv_mapvote_begin"},
		{&maps::vote::sv_mapvote_add, "sv_mapvote_add"},
		{&maps::vote::sv_mapvote_del, "sv_mapvote_del"},
		{&maps::vote::sv_mapvote_list, "sv_mapvote_list"},

		// admin commands
		{&admin::sv_admin_add, "sv_admin_add"},
		{&admin::sv_admin_del, "sv_admin_del"},
		{&admin::sv_admin_list, "sv_admin_list"},
		{&admin::sv_admin_cur, "sv_admin_cur"},
		{&admin::sv_reloadaccess, "sv_reloadaccess"},
		{&admin::sv_commands, "sv_commands"},

		// team commands
		{&teams::sv_teams_balance, "sv_teams_balance"},
		{&teams::sv_teams_lock, "sv_teams_lock"},
		{&teams::sv_teams_unlock, "sv_teams_unlock"},
		{&teams::sv_changeteam, "sv_changeteam"},
		
		// alias commands
		{&alias::sv_alias_search, "sv_alias_search"},
		{&alias::sv_alias_hash, "sv_alias_hash"},
		{&alias::sv_alias, "sv_alias"},

		// message functions
		{&messages::welcome::sv_welcome_add, "sv_welcome_add"},
		{&messages::welcome::sv_welcome_del, "sv_welcome_del"},
		{&messages::welcome::sv_welcome_list, "sv_welcome_list"},
		{&messages::automated::sv_msg_add, "sv_msg_add"},
		{&messages::automated::sv_msg_del, "sv_msg_del"},
		{&messages::automated::sv_msg_list, "sv_msg_list"},
		{&messages::automated::sv_msg_start, "sv_msg_start"},
		{&messages::automated::sv_msg_end, "sv_msg_end"},

		// object commands
		{&objects::tp::sv_teleport, "sv_teleport"},
		{&objects::tp::sv_teleport_add, "sv_teleport_add"},
		{&objects::tp::sv_teleport_del, "sv_teleport_del"},
		{&objects::tp::sv_teleport_list, "sv_teleport_list"},
		{&objects::sv_map_reset, "sv_map_reset"},

		// general commands
		{&halo::afksystem::sv_kickafk, "sv_kickafk"},
		{&PhasorServer::sv_host, "sv_host"},
		//{&sv_spawn, "sv_spawn"},	
		{&sv_say, "sv_say"},
		{&sv_gethash, "sv_gethash"},
		{&sv_kill, "sv_kill"},
		{game::sv_chatids, "sv_chatids"},
		
		// logging commands
		{&logging::sv_disablelog, "sv_disablelog"},
		{&logging::sv_logname, "sv_logname"},
		{&logging::sv_savelog, "sv_savelog"},	
		{&logging::sv_loglimit, "sv_loglimit"},

		// undocumented/dev commands
		{&sv_getobject, "sv_getobject"},
		{&sv_invis, "sv_invis"},
		{&sv_setspeed, "sv_setspeed"},

		{NULL, NULL}
	};

	PhasorCommands* GetConsoleCommands()
	{
		return (PhasorCommands*)&function_lookup_table;
	}
	// The following functions are used to action events
	// --------------------------------------------------------------------
	
	void DispatchChat(game::chatData d, int len, halo::h_player* to_player)
	{
		std::wstring actual_msg = d.msg;

		// Prepend *SERVER*
		if (d.type == CHAT_NONE)
			actual_msg = L"** SERVER ** " + actual_msg;

		d.msg = (wchar_t*)actual_msg.c_str();

		halo::h_player* player = game::GetPlayer(d.player);
		std::vector<halo::h_player*> players = game::GetPlayerList();

		// Gotta pass a pointer to the dataPass sturct
		DWORD d_ptr = (DWORD)&d;

		BYTE* packetBuffer = new BYTE[4096 + 2*len];

		if (!packetBuffer)
			return;

		// Build the chat packet
		DWORD retval =  BuildPacket(packetBuffer, 0, 0x0F, 0, (LPBYTE)&d_ptr, 0, 1, 0);

		if (!to_player) // apply normal processing if a dest player isn't specified
		{
			// Different chat types need to be handled differently
			if (d.type == 0 || d.type == 3) // send to everyone (all chat,server messages)
			{
				// Send the packet to the server
				AddPacketToGlobalQueue(packetBuffer, retval, 1, 1, 0, 1, 3);
			}
			else if (d.type == 1) // Team chat
			{
				for (std::vector<halo::h_player*>::iterator itr = players.begin(); 
					itr != players.end(); itr++)
				{
					if ((*itr)->mem->team == player->mem->team)
						AddPacketToPlayerQueue((*itr)->mem->playerNum, packetBuffer, retval, 1, 1, 0, 1, 3);
				}
			}
			else if (d.type == 2) // Vehicle chat
			{
				// Check if the sender is in a vehicle
				if (player->m_object && player->m_object->base.vehicleId != -1)
				{
					// Iterate through all players and send to those in vehicles
					for (std::vector<halo::h_player*>::iterator itr = players.begin(); 
						itr != players.end(); itr++)
					{
						if ((*itr)->m_object && (*itr)->m_object->base.vehicleId != -1) // check the vehicle object
							AddPacketToPlayerQueue((*itr)->mem->playerNum, packetBuffer, retval, 1, 1, 0, 1, 3);
					}					
				}
			}
		}
		else
			// Send the packet to the player
			AddPacketToPlayerQueue(to_player->mem->playerNum, packetBuffer, retval, 1, 1, 0, 1, 3);	

		delete[] packetBuffer;
	}

	// Sends a chat message to the server from 'player'
	void SendChat(DWORD type, h_player* player, wchar_t* _Format, ...)
	{
		va_list ArgList;
		va_start(ArgList, _Format);
		std::wstring msg = WFormatVarArgs(_Format, ArgList);
		va_end(ArgList);

		// Build the chat packet
		game::chatData d;
		d.type = type;
		d.player = player->memoryId; // Player we're sending it from
		d.msg = (wchar_t*)msg.c_str();

		DispatchChat(d, msg.size());
	}

	// Sends a global server message
	void GlobalMessage(wchar_t* _Format, ...)
	{
		va_list ArgList;
		va_start(ArgList, _Format);
		std::wstring msg = WFormatVarArgs(_Format, ArgList);
		va_end(ArgList);

		// Build the chat packet
		game::chatData d;
		d.type = CHAT_NONE;
		d.player = -1;
		d.msg = (wchar_t*)msg.c_str();

		DispatchChat(d, msg.size());
	}

	// Returns the current server ticks
	DWORD GetServerTicks()
	{
		DWORD server_base = *(DWORD*)ADDR_SERVERINFO;
		DWORD ticks = *(DWORD*)(server_base + 0x0C);
		return ticks;
	}

	// The following functions are used for event notifications
	// --------------------------------------------------------------------
	// 
	// Called for console events (exit etc)
	void __stdcall ConsoleHandler(DWORD fdwCtrlType)
	{
		switch(fdwCtrlType) 
		{ 
		case CTRL_CLOSE_EVENT: 
		case CTRL_SHUTDOWN_EVENT: 
		case CTRL_LOGOFF_EVENT: 
			{
				// cleanup everything
				OnClose();

			} break;
		}
	}

	// Called periodically by Halo to check for console input, I use as timer
	void __stdcall OnConsoleProcessing()
	{
		// Process this thread's timers
		ThreadTimer::ProcessQueue();
		Timer::ProcessTimers();		
	}

	// Called when a console command is to be executed
	// true: Event has been handled, don't pass to server
	// false: Not handled, pass to server.
	bool __stdcall ConsoleCommand(char* command)
	{
		std::vector<std::string> tokens = TokenizeCommand(command);

		if (!tokens.size()) // no data
			return true;

		bool processed = false;

		// get the player who's executing the command
		DWORD exec = *(DWORD*)UlongToPtr(ADDR_RCONPLAYER);
		halo::h_player* player = game::GetPlayer_pi(exec);

		bool bAllow = true;

		if (admin::GetAdminCount() > 0 && player) // player == NULL when executed through server console
		{
			bAllow = false;
			std::wstring wcommand = ToWideChar(command);

			if (!player->is_admin) // check if they're an admin
			{
				const wchar_t* err = L"An unauthorized player is attempting to use RCON:";
				std::wstring fmt = m_swprintf_s(L"Name: %s Hash: %s", player->mem->playerName, player->whash.c_str());

				// output the error
				halo::hprintf(L"%s", err);	halo::hprintf(L"%s", fmt.c_str());
				logging::LogData(LOGGING_GAME, L"%s", err); 
				logging::LogData(LOGGING_GAME, L"%s", fmt.c_str());
			}
			else if (!admin::CanUseCommand(player->admin_info.level, tokens[0]))
			{
				const char* err = "An authorized player is attempting to use an unauthorized command:";
				std::string fmt = m_sprintf_s("Name: %s Hash: %s", player->admin_info.name.c_str(), command);

				halo::hprintf("%s", err);	halo::hprintf("%s", fmt.c_str());	
				logging::LogData(LOGGING_GAME, "%s", err);
				logging::LogData(LOGGING_GAME, "%s", fmt.c_str());
			}
			else // allowed
			{
				std::wstring fmt = m_swprintf_s(L"Executing ' %s ' from %s (authed as %s).",
				wcommand.c_str(), player->mem->playerName, ToWideChar(player->admin_info.name).c_str());

				hprintf(L"%s", fmt.c_str());
				logging::LogData(LOGGING_GAME, L"%s", fmt.c_str());
				bAllow = true;
			}
		}

		if (bAllow)
		{
			DWORD dwPlayerId = -1;
			if (player)
				dwPlayerId = player->memoryId;

			// Notify scripts
			int allow = Lua::rfuncCall(1, "OnServerCommand", "ls", dwPlayerId, command);

			// allow the command if scripts say so
			if (allow)
			{
				// Lookup and call our function
				for (int i = 0; function_lookup_table[i].fn; i++)
				{
					if (tokens[0] == function_lookup_table[i].key)
					{
						processed = (*(function_lookup_table[i].fn))(player, tokens);
						break;
					}
				}

				if (processed && !player)
				{
					// index for command
					WORD* lpCount = (WORD*)UlongToPtr(ADDR_CMDCACHE_INDEX);
					WORD count = (1 + *(WORD*)lpCount) % 8;
					*(WORD*)lpCount = count;

					// Write command to cache
					char* cmd_memory = (char*)UlongToPtr(ADDR_CMDCACHE + (count * 0xff));
					strcpy_s(cmd_memory, 0xff, command);

					// set the current index to what we just added
					*(WORD*)UlongToPtr(ADDR_CMDCACHE_CUR) = 0xFFFF;
				}
			}
			else
				processed = true;
		}
		else
			processed = true;

		return processed;
	}

	// Called when a new map is loaded
	bool __stdcall OnMapLoad(LPBYTE mapData)
	{
		return game::maps::OnMapLoad(mapData);
	}

	// Miscellaneous helper functions
	// --------------------------------------------------------------------

	// This function builds a packet
	DWORD BuildPacket(LPBYTE output, DWORD arg1, DWORD packettype, DWORD arg3, LPBYTE dataPtr, DWORD arg4, DWORD arg5, DWORD arg6)
	{
		DWORD dwBuildPacket = FUNC_BUILDPACKET;
		DWORD retval = 0;

		__asm
		{
			pushad

			PUSH arg6
			PUSH arg5
			PUSH arg4
			PUSH dataPtr
			PUSH arg3
			PUSH packettype
			PUSH arg1
			MOV EDX,0x7FF8
			MOV EAX,output
			call dword ptr ds:[dwBuildPacket]
			mov retval, eax
			add esp, 0x1C

			popad
		}

		return retval;
	}

	// This function adds a packet to the queue
	void AddPacketToGlobalQueue(LPBYTE packet, DWORD packetCode, DWORD arg1, DWORD arg2, DWORD arg3, DWORD arg4, DWORD arg5)
	{
		DWORD dwAddToQueue = FUNC_ADDPACKETTOQUEUE;

		__asm
		{
			pushad

			MOV ECX,DWORD PTR DS:[ADDR_SOCKETREADY]
			mov ECX, [ECX]
			cmp ecx, 0
			je NOT_READY_TO_SEND

			PUSH arg5
			PUSH arg4
			PUSH arg3
			PUSH arg2
			PUSH packet
			PUSH arg1
			MOV EAX,packetCode
			call dword ptr ds:[dwAddToQueue]
			add esp, 0x18

NOT_READY_TO_SEND:

			popad
		}
	}

	// This function adds a packet to a specific player's queue
	void AddPacketToPlayerQueue(DWORD player, LPBYTE packet, DWORD packetCode, DWORD arg1, DWORD arg2, DWORD arg3, DWORD arg4, DWORD arg5)
	{
		DWORD dwAddToPlayerQueue = FUNC_ADDPACKETTOPQUEUE;

		__asm
		{
			pushad

			MOV ESI,DWORD PTR DS:[ADDR_SOCKETREADY]
			mov esi, [esi]

			cmp esi, 0
			je NOT_READY_TO_SEND

			PUSH arg5
			PUSH arg4
			PUSH arg3
			PUSH arg2
			PUSH packetCode
			PUSH packet
			PUSH arg1
			mov eax, player
			call dword ptr ds:[dwAddToPlayerQueue]
			add esp, 0x1C

NOT_READY_TO_SEND:

			popad
		}
	}
}