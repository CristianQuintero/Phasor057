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

	class afksys
	{	
		static bool process_timer(LPARAM instance);

		// data members
		DWORD timer_id, camera_x, camera_y, move_count, afk_duration;
		h_player* afksys_player;

	public:
		
		afksys();
		~afksys();

		// assigns the player this timer is for
		void AssignPlayer(h_player* p);

		// Checks if they have moved their camera
		void CheckCameraMovement();

		// resets the afk timer
		void ResetAfkTimer();
	};

	namespace afksystem
	{
		// Notifys the afk kicking system that the game has ended
		void GameEnd();
		void GameStart();

		// Server commands
		// --------------------------------------------------------------------
		bool sv_kickafk(h_player* exec, std::vector<std::string> & tokens);
	}
}