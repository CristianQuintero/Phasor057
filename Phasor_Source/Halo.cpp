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

#include "Halo.h"
#include "Addresses.h"
#include "Common.h"
#include "Game.h"
#include "Server.h"
#include "Admin.h"
#include "Lua.h"
#include "objects.h"

// Ban functions
typedef BYTE*(*AddToBanList)(const char* player, const char* hash);
typedef void (*SaveBanlistToFile)();

// Hash functions
typedef char* (__cdecl *_HaloGetHash)(DWORD type, DWORD offset);

namespace halo
{
	BYTE packetBuffer[65536] = {0};

	// h_player class code
	// --------------------------------------------------------------------
	h_player::h_player(int m_index)
	{
		afksys::AssignPlayer(this); // for afk system

		memoryId = m_index;
		mapVote = -1;
		is_admin = false;
		sv_killed = false;
		force_entered = false;

		// Find the player's memory
		mem = GetPlayerMemory(m_index);

		// get player hash
		int offset = GetHashOffset(this);

		if (offset != -1)
		{
			_HaloGetHash Halo_GetHash = (_HaloGetHash)(FUNC_HALOGETHASH);
			char* str_hash = Halo_GetHash(0x319, offset);

			if (str_hash)
			{
				hash = str_hash;
				whash = ToWideChar(hash);
				is_admin = admin::IsAdmin(hash, admin_info);
			}
		}
	}

	h_player::~h_player()
	{

	}

	// Validates the initialization of the class
	bool h_player::Validate()
	{
		bool bSuccess = true;

		if (!mem)
			bSuccess = false;

		return bSuccess;
	}

	// Sends console text to the specified player
	bool h_player::SendConsoleText(const char* text)
	{
		bool bSent = true;
		if (strlen(text) < 0x50)
		{
			#pragma pack(push, 1)
			struct c_struct
			{
				char* msg_ptr;
				DWORD unk; // always 0
				char msg[0x50];

				c_struct::c_struct(const char* text)
				{
					memset(msg, 0, 0x50);
					strcpy_s(msg, 0x50, text);
					unk = 0;
					msg_ptr = msg;
				}
			};
			#pragma pack(pop)

			c_struct d(text);
			LPBYTE buffer = new BYTE[8192]; // use new buffer as hprintf isnt expected to use packetBuffer
			DWORD size = server::BuildPacket(buffer, 0, 0x37, 0, (LPBYTE)&d, 0,1,0);
			server::AddPacketToPlayerQueue(mem->playerNum, buffer, size, 1,1,0,1,3);
			delete[] buffer;
		}
		else
			bSent = false; // too long

		return bSent;
	}


	// Kills this player
	void h_player::Kill()
	{
		sv_killed = true; // used later for detecting what killed the player

		// kill them
		DWORD playerMask = (mem->playerJoinCount << 0x10) | memoryId;
		DWORD playerObj = mem->m_playerObjectid;

		if (playerObj != -1)
		{
			__asm
			{
				pushad

				PUSH 0
				PUSH -1
				PUSH -1
				PUSH -1
				MOV EAX,playerObj
				call DWORD PTR ds:[FUNC_ONPLAYERDEATH]
				add esp, 0x10
				mov eax, playerObj
				call DWORD PTR ds:[FUNC_ACTIONDEATH_1]
				mov eax, playerMask
				call DWORD PTR ds:[FUNC_ACTIONDEATH_2]
				push playerMask
				call DWORD PTR ds:[FUNC_ACTIONDEATH_3]
				add esp, 4

				popad
			}
		}

		sv_killed = false;
	}

	// Kicks the player
	void h_player::Kick()
	{
		ExecuteServerCommand("sv_kick %i", mem->playerNum+1);
	}

	// Changes the player's team
	void h_player::ChangeTeam(bool forceDeath)
	{
		// Toggle the team
		bool oldTeam = (bool)mem->team;
		mem->team = !mem->team;		mem->team_Again = mem->team;
		// find the player in the other list (not really sure why this list
		// matters) and change their team
		LPBYTE other_list = (LPBYTE)(*(DWORD*)UlongToPtr(ADDR_PLAYERINFOBASE)) + OFFSET_PLAYERTABLE;		for (int i = 0; i < 16; i++)
		{
			// Check if player exists
			if (*(DWORD*)(other_list + 0x1C + (i * 0x20)) != -1)
			{
				if (other_list[0x1C + (i * 0x20)] == memoryId)
				{
					// Change the team
					other_list[0x1E + (i * 0x20)] = (BYTE)mem->team;
					break;
				}
			}
		}

		// Check if we're to kill the player
		if (forceDeath)
			Kill();

		// build the packet that notifies the server of the team change
		BYTE d[4] = {(BYTE)memoryId, (BYTE)mem->team, 0x18, 0};
		// Gotta pass a pointer to the data
		DWORD d_ptr = (DWORD)&d;		DWORD retval = server::BuildPacket(packetBuffer, 0, 0x1A, 0, (LPBYTE)&d_ptr, 0, 1, 0);
		server::AddPacketToGlobalQueue(packetBuffer, retval, 1, 1, 0, 1, 3);

		// notify scripts
		Lua::rfuncCall(1, "OnTeamChange", "illl", false, memoryId, oldTeam, mem->team);

	}

	// Gives the player active camoflauge
	void h_player::ApplyCamo(float duration)
	{
		DWORD playerMask = (mem->playerJoinCount << 0x10) | memoryId;

		// this duration is treated as infinite
		DWORD count = 0x8000;

		if (duration != 0)
			count = (DWORD)(duration * 30); // 30 ticks per second

		// Make the player invisible
		__asm
		{
			mov eax, memoryId
			mov ebx, playerMask
			push count
			push 0
			call dword ptr ds:[FUNC_DOINVIS]
			add esp, 8
		}
	}

	// Sends the player a private server message
	void h_player::ServerMessage(const wchar_t* message)
	{
		// Build the chat packet
		game::chatData d;
		d.type = CHAT_NONE;
		d.player = -1;
		d.msg = (wchar_t*)message;

		server::DispatchChat(d, wcslen(message), this);
	}

	// -------------------------------------------------------------------
	// Generic Halo related functions used by Phasor
	// 
	// This function returns a player's memory space
	hPlayerStructure* GetPlayerMemory(int index)
	{
		hPlayerStructure* mem = 0;

		if (index >= 0 && index < 16)
		{
			LPBYTE lpPlayerList = (LPBYTE)*(DWORD*)ULongToPtr(ADDR_PLAYERBASE);

			if (lpPlayerList)
				mem = (hPlayerStructure*)(lpPlayerList + 0x38 + (0x200 * index));
		}
		
		return mem;
	}

	// Bans a hash indefinitely
	void BanHash(bool inc, char* name, char* hash)
	{
		AddToBanList AddPlayerToBanList = (AddToBanList)FUNC_BANPLAYER;
		SaveBanlistToFile SaveBanList = (SaveBanlistToFile)FUNC_SAVEBANLIST;
		BYTE* entry = AddPlayerToBanList(name, hash);

		if (!entry)
			return;

		// Increment the counter for the ban
		*(entry + 0x2E) += inc;

		// Indefinite ban
		*(entry + 0x30) = 1; // Specifies that the ban is indefinite
		*(entry + 0x34) = 0; // irrelevant for this ban, set to 0
		SaveBanList();
	}

	// This is a helper function for GetHash
	int GetHashOffset(halo::h_player* player)
	{
		LPBYTE origin = (LPBYTE)ADDR_PLAYERINFOBASE;
		LPBYTE lpBase = (LPBYTE)UlongToPtr(OFFSET_HASHBASE + *(DWORD*)origin);

		int i = 0;
		for (; i < 16; i++)
		{
			WORD check = *(WORD*)lpBase;

			if (check == player->mem->playerNum)
				break;

			lpBase += OFFSET_HASHLOOKUPLEN;
		}

		LPBYTE ptr = (LPBYTE)(*(DWORD*)origin + ((i * 3) << 5) + OFFSET_HASHRESOLVE);

		int offset = -1;

		if (ptr && *(DWORD*)ptr)
			offset = *(DWORD*)(ptr + 0x5C);

		return offset;
	}

	// Executes a server command
	void ExecuteServerCommand(const char* _Format, ...)
	{
		va_list ArgList;
		va_start(ArgList, _Format);
		std::string szText = FormatVarArgs(_Format, ArgList);
		va_end(ArgList);

		const char* cmd = szText.c_str();

		// execute the command as the server
		DWORD rconPlayer = *(DWORD*)UlongToPtr(ADDR_RCONPLAYER);
		*(DWORD*)UlongToPtr(ADDR_RCONPLAYER) = -1;

		__asm
		{
			pushad

			PUSH 0x2000
			mov edi, cmd
			call dword ptr ds: [FUNC_EXECUTESVCMD]
			add esp, 4		

			popad
		}

		// restore actual executing player
		*(DWORD*)UlongToPtr(ADDR_RCONPLAYER) = rconPlayer;
	}

	// Prints formatted text to the console
	void hprintf(const char* _Format, ...)
	{
		// Format the data into a string
		va_list ArgList;
		va_start(ArgList, _Format);
		std::string szText = FormatVarArgs(_Format, ArgList);
		va_end(ArgList);

		// Convert to wide char
		hprintf(L"%s", ToWideChar(szText).c_str());
	}

	// Prints formatted text to the console
	void hprintf(const wchar_t* _Format, ...)
	{
		va_list ArgList;
		va_start(ArgList, _Format);
		std::wstring szText = WFormatVarArgs(_Format, ArgList);
		va_end(ArgList);

		// Send console text
		DWORD exec = *(DWORD*)UlongToPtr(ADDR_RCONPLAYER);
		halo::h_player* player = game::GetPlayer_pi(exec);

		if (player)
			player->SendConsoleText(ToChar(szText).c_str());

		bool ready = *(bool*)UlongToPtr(ADDR_CONSOLEREADY);

		if (ready)
		{
			// Prepare for writing the string
			HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
			DWORD written = 0;

			CONSOLE_SCREEN_BUFFER_INFO info;
			SHORT oldX = 0; // used to set cursor back to old position

			// Get current console position info
			GetConsoleScreenBufferInfo(hConsole, &info);
			oldX = info.dwCursorPosition.X;

			// Set cursor to start of the last row (where we want to start writing)
			info.dwCursorPosition.X = 0;
			SetConsoleCursorPosition(hConsole, info.dwCursorPosition);

			FillConsoleOutputCharacterA(hConsole, ' ', 95, info.dwCursorPosition, &written);
			FillConsoleOutputAttribute(hConsole, 7, 95, info.dwCursorPosition, &written);

			// Write the text
			WriteConsoleW(hConsole, szText.c_str(), szText.size(), &written, NULL);
			WriteConsoleW(hConsole, L"\n", 1, &written, NULL);

			// Get the current text in the console
			LPBYTE ptr = (LPBYTE)ADDR_CONSOLEINFO;

			// Build current command input
			std::string formatted = "halo( ";

			if (*ptr != 0)
				formatted += (char*)UlongToPtr(*(DWORD*)ptr + OFFSET_CONSOLETEXT); // current text

			// Rewrite the data to console
			GetConsoleScreenBufferInfo(hConsole, &info);

			FillConsoleOutputCharacterA(hConsole, ' ', 95, info.dwCursorPosition, &written);
			WriteConsoleOutputCharacterA(hConsole, formatted.c_str(), formatted.size(), info.dwCursorPosition, &written);

			// Set the cursor to its old position
			GetConsoleScreenBufferInfo(hConsole, &info);
			info.dwCursorPosition.X = oldX;

			SetConsoleCursorPosition(hConsole, info.dwCursorPosition);
		}
		else
			wprintf(L"%s\n", szText.c_str());
	}

	void OutputInColumns(int col, std::vector<std::string> data)
	{
		// Lookup and call the function
		for (size_t i = 0; i < data.size(); i++)
		{
			std::string line;
			int proc = 0;

			for (size_t z = 0; z < col; z++)
			{
				if (i+z < data.size())
				{
					int len = strlen(data[i+z].c_str());
					int tabCount = col - (len / 8);

					line += data[i+z];

					for (int x = 0; x < tabCount; x++)
						line += "\t";		

					proc++;
				}
				else
				{
					break;
				}
			}

			// Convert tabs to spaces as halo (game) doesn't like tabs in console
			std::string expanded = ExpandTabsToSpace((char*)line.c_str());
			hprintf("%s", expanded.c_str());

			i+= proc-1;
		}
	}

}