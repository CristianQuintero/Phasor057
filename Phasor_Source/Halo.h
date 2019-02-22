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

#include "Admin.h"
#include "afksys.h"
#include <windows.h>
#include <vector>
#include <string>

#define CHAT_GLOBAL		0
#define CHAT_TEAM		1
#define CHAT_VEHICLE	2
#define CHAT_NONE		3

struct PObject;

namespace halo
{
	// Partial structure of the player structure
	struct hPlayerStructure
	{
		WORD playerJoinCount; // 0x0000
		WORD localClient; // 0x0002 always FF FF on a dedi in Halo is 00 00 if its your player
		wchar_t playerName[12]; //0x0004
		DWORD unk; // 0x001C only seen FF FF FF FF
		DWORD team; // 0x0020
		DWORD m_interactionObject; // 0x0024 ie Press E to enter THIS vehicle
		WORD interactionType; // 0x0028 8 for vehicle, 7 for weapon
		WORD interactionSpecifier; // 0x002A which seat of car etc
		DWORD respawnTimer; // 0x002c
		DWORD unk2; // 0x0030 only seen empty
		DWORD m_playerObjectid; // 0x0034
		DWORD m_oldObjectid; // 0x0038
		DWORD unkCounter; // 0x003C sometimes changes as you move, fuck idk
		DWORD empty; // 0x0040 always FF FF FF FF, never accessed
		DWORD bulletUnk; // 0x0044 changes when the player shoots
		wchar_t playerNameAgain[12]; // 0x0048
		WORD playerNum_NotUsed; // 0x0060 seems to be the player number.. never seen it accessed tho
		WORD empty1; // 0x0062 byte allignment
		BYTE playerNum; // 0x0064 player number used for rcon etc (ofc this is 0 based tho)
		BYTE unk_PlayerNumberHigh; // 0x0065 just a guess
		BYTE team_Again; // 0x0066
		BYTE unk3; // 0x0067  idk, probably something to do with player numbers
		DWORD unk4; // 0x0068 only seen 0, it wont always be 0 tho
		float speed; // 0x006C
		DWORD unk5[11]; // 0x0070 16 bytes of FF? then 4 0, some data, rest 0... meh idk about rest either
		WORD kills; // 0x009C
		WORD idk; // 0x009E byte allignment?
		DWORD unk6; // 0x00A0 maybe to do with scoring, not sure
		WORD assists; // 0x00A4
		WORD unk7; // 0x00A6 idk byte allignment?
		DWORD unk8; // 0x00A8
		WORD betrayals; // 0x00AC
		WORD deaths; // 0x00AE
		WORD suicides; // 0x00B0
		// cbf with the rest
		BYTE rest[0x14E]; // 0x00B2
	};
	
	// This class is used to action events, it is not used when events occur
	// For event processing see Game
	class h_player: public afksys
	{
	public:
		// Data members
		int memoryId, mapVote;
		admin::adminInfo admin_info;
		bool is_admin, sv_killed, force_entered; // sv_killed is set to true when the server kills them
		std::wstring whash;
		std::string hash;

		h_player(int m_index);
		~h_player();
	
		// Validates the initialization of the class
		bool Validate();

		// Pointer to this player's memory
		hPlayerStructure* mem;
		PObject* m_object;

		// Sends console text to the specified player
		bool SendConsoleText(const char* text);

		// Kills this player
		void Kill();

		// Kicks the player
		void Kick();

		// Changes the player's team
		void ChangeTeam(bool forceDeath);

		// Gives the player active camoflauge
		void ApplyCamo(float duration);

		// Sends the player a chat message appearing to be from 'from'
		void ServerMessage(const wchar_t* message);
	};
	
	// -------------------------------------------------------------------
	// Generic Halo related functions used by Phasor
	// 
	// Returns a player's memory space
	hPlayerStructure* GetPlayerMemory(int index);

	// Bans a hash indefinitely
	void BanHash(bool inc, char* name, char* hash);

	// This is a helper function for GetHash
	int GetHashOffset(h_player* player);

	// Executes a server command
	void ExecuteServerCommand(const char* _Format, ...);

	// Prints formatted text to the console
	void hprintf(const wchar_t* _Format, ...);
	void hprintf(const char* _Format, ...);
	void OutputInColumns(int col, std::vector<std::string> data);
}