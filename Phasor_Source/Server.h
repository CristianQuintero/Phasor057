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
#include "Halo.h"
#include "Game.h"

namespace server
{
	using namespace halo;

	struct PhasorCommands
	{
		(bool)(*fn)(halo::h_player*, std::vector<std::string>&);
		const char* key;
	};

	PhasorCommands* GetConsoleCommands();

	// The following functions are used to action events
	// --------------------------------------------------------------------
	// 
	void DispatchChat(game::chatData d, int len, halo::h_player* to_player=0);

	// Sends a chat message to the server from 'player'
	void SendChat(DWORD type, h_player* player, wchar_t* _Format, ...);

	// Sends a global server message
	void GlobalMessage(wchar_t* _Format, ...);

	// Returns the current server ticks
	DWORD GetServerTicks();

	// The following functions are used for event notifications
	// --------------------------------------------------------------------
	// 
	// Called for console events (exit etc)
	void __stdcall ConsoleHandler(DWORD fdwCtrlType);

	// Called periodically by Halo to check for console input, I use as timer
	void __stdcall OnConsoleProcessing();

	// Called when a console command is to be executed
	bool __stdcall ConsoleCommand(char* command);

	// Called when a new map is loaded
	bool __stdcall OnMapLoad(LPBYTE mapData);

	// Miscellaneous helper functions
	// --------------------------------------------------------------------

	// This function builds a packet
	DWORD BuildPacket(LPBYTE output, DWORD arg1, DWORD packettype, DWORD arg3, LPBYTE dataPtr, DWORD arg4, DWORD arg5, DWORD arg6);

	// This function adds a packet to the queue
	void AddPacketToGlobalQueue(LPBYTE packet, DWORD packetCode, DWORD arg1, DWORD arg2, DWORD arg3, DWORD arg4, DWORD arg5);

	// This function adds a packet to a specific player's queue
	void AddPacketToPlayerQueue(DWORD player, LPBYTE packet, DWORD packetCode, DWORD arg1, DWORD arg2, DWORD arg3, DWORD arg4, DWORD arg5);
}