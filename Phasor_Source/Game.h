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

#pragma once

#include <windows.h>
#include "Maps.h"
#include "Halo.h"

struct PObject;
namespace objects
{
	struct objInfo; // see objects.h
}

namespace game
{
	// Ensure it's aligned exactly as specified
	#pragma pack(push, 1)
	struct chatData
	{
		DWORD type;
		DWORD player;
		wchar_t* msg;
	};

	#pragma pack(pop)

	// Returns the data associated with the specified player
	halo::h_player* GetPlayer(int m_playerid);
	halo::h_player* GetPlayer_pi(int p_playerid);
	halo::h_player* GetPlayer_obj(PObject* m_objectAddr);

	// Returns a list of all current players
	std::vector<halo::h_player*> GetPlayerList();

	// Gets the gametype's respawn ticks
	DWORD GetRespawnTicks();

	// --------------------------------------------------------------------
	// EVENTS
	// 
	// Called when a game stage ends
	void __stdcall OnEnd(DWORD mode);

	// Called when a new game starts
	void __stdcall OnNew(char* map);

	// Called when a player joins (after verification).
	void __stdcall OnPlayerWelcome(DWORD playerId);

	// Called when a player quits
	void __stdcall OnPlayerQuit(DWORD playerId);

	// Called when a player's team is being assigned
	DWORD __stdcall OnTeamSelection(DWORD cur_team);

	// Called when a player tries to change team
	bool __stdcall OnTeamChange(DWORD playerId, DWORD team);

	// Called when a player is about to spawn (object already created)
	void __stdcall OnPlayerSpawn(DWORD playerId, DWORD m_objectId);

	// Called after the server has been notified of a player spawn
	void __stdcall OnPlayerSpawnEnd(DWORD playerId, DWORD m_objectId);

	// Called when a weapon is created
	void __stdcall OnObjectCreation(DWORD m_weaponId);

	// Called when a weapon is assigned to an object
	DWORD __stdcall OnWeaponAssignment(DWORD playerId, DWORD owningObjectId, objects::objInfo* curWeapon, DWORD order);

	// Called when a player can interact with an object
	bool __stdcall OnObjectInteraction(DWORD playerId, DWORD m_ObjId);

	// Called when a player's position is updated
	void __stdcall OnClientUpdate(PObject* m_object);

	// Called when an object's damage is being looked up
	void __stdcall OnDamageLookup(DWORD receivingObj, DWORD causingObj, LPBYTE tagEntry);

	// Called when someone chats in the server
	void __stdcall OnChat(chatData* chat);

	// Called when a player attempts to enter a vehicle
	bool __stdcall OnVehicleEntry(DWORD playerId);

	// Called when a player is being ejected from a vehicle
	bool __stdcall OnVehicleEject(PObject* m_playerObject, bool forceEjected);

	// Called when a player dies
	void __stdcall OnDeath(DWORD killerId, DWORD victimId, DWORD mode);

	// Called when a player gets a double kill, spree etc
	void __stdcall OnKillMultiplier(DWORD playerId, DWORD multiplier);

	// Called when a weapon is reloaded
	bool __stdcall OnWeaponReload(DWORD m_WeaponId);

	// --------------------------------------------------------------------
	// SERVER COMMANDS
	bool sv_chatids(halo::h_player* execPlayer, std::vector<std::string>& tokens);

}