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
#include <vector>

namespace halo
{
	class h_player;
}

// Used for connecting to my server
// server just tracks a few basic statistics so i can see how many people, and who
// use phasor
namespace PhasorServer
{
	// Console command for setting host info
	bool sv_host(halo::h_player* exec, std::vector<std::string>& tokens);

	// The auxillary thread calls this function as it initializes
	void Setup();

	// The auxillary thread calls this function as it terminates
	void Cleanup();

	// Used for all methods concerning the stats server
	DWORD WINAPI PhasorServer_Thread(LPVOID lParam);

	// All these functions are invoked by the main server thread
	// This is called every 5 minutes to update info sent to my server
	bool get_server_info(LPARAM unused);
	bool thread_safe_ban(LPARAM data);
}

