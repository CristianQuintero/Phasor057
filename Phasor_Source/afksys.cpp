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

#include "afksys.h"
#include "Halo.h"
#include "Timers.h"
#include "Logging.h"
#include "Server.h"
#include "Common.h"
#include "objects.h"

namespace halo
{
	namespace afksystem
	{
		size_t max_duration = 0; // minutes to wait before kicking
		bool bGameEnded = false;

		// Notifys the afk kicking system that the game has ended
		void GameEnd()
		{
			bGameEnded = true;
		}

		void GameStart()
		{
			bGameEnded = false;
		}

		bool sv_kickafk(h_player* exec, std::vector<std::string> & tokens)
		{
			if (tokens.size() == 2)
			{
				max_duration = atoi(tokens[1].c_str());

				if (max_duration)
					hprintf("Inactive players will be kicked after %i minute(s).", max_duration);
				else
					hprintf("Inactive players will no longer be kicked.");
			}
			else
				hprintf("Correct usage: sv_kickafk <duration> (to disable set duration to 0).");

			return true;
		}
	}

	// AFK KICKING PLAYER CODE
	// --------------------------------------------------------------------
	afksys::afksys()
	{
		timer_id = Timer::CreateTimer(60000, (LPARAM)this, process_timer);

		if (!timer_id)
		{
			const wchar_t* err = L"Couldn't create a timer for kicking inactive players, as such the feature may not work correctly.";
			halo::hprintf(L"%s", err);
			logging::LogData(LOGGING_PHASOR, L"%s", err);
		}

		// initial coords for camera
		camera_x = 0;
		camera_y = 0;
		move_count = 0;  // no camera movements
		afk_duration = 0; // minutes afk
	}

	afksys::~afksys()
	{
		if (timer_id)
			Timer::RemoveTimer(timer_id);
	}

	// assigns the player this timer is for
	void afksys::AssignPlayer(h_player* p)
	{
		afksys_player = p;
	}

	// Checks if they have moved their camera
	void afksys::CheckCameraMovement()
	{
		// make sure the object is available
		if (afksys_player->m_object)
		{
			DWORD new_camera_x = afksys_player->m_object->biped.cameraX;
			DWORD new_camera_y = afksys_player->m_object->biped.cameraY;

			// check if the camera has moved
			if (new_camera_x != camera_x || new_camera_y != camera_y)
				move_count++;

			camera_x = new_camera_x;
			camera_y = new_camera_y;
		}
	}

	// resets the afk timer
	void afksys::ResetAfkTimer()
	{
		afk_duration = 0;
		move_count = 101;
	}

	// This is called every minute inorder to determine if a player is inactive
	bool afksys::process_timer(LPARAM instance)
	{
		if (!afksystem::max_duration) // only process if the feature is enabled
			return true;

		afksys* afk = (afksys*)instance;

		// Check the movement counter
		if (!afksystem::bGameEnded && !afk->afksys_player->is_admin && afk->move_count < 101)
		{
			// they are deemed to be afk
			afk->afk_duration++;

			if (afk->afk_duration >= afksystem::max_duration)
			{
				afk->afksys_player->Kick();

				// Notify the server
				std::wstring msg = m_swprintf_s(L"%s has been kicked due to inactivity.", afk->afksys_player->mem->playerName);
				server::GlobalMessage((wchar_t*)msg.c_str());
				hprintf(L"%s", msg.c_str());
			}
			else // notify the player
			{
				std::wstring warning = m_swprintf_s(L"You don't appear to be playing. You will be kicked in %i minute(s), if you remain inactive.", afksystem::max_duration - afk->afk_duration);
				afk->afksys_player->ServerMessage((wchar_t*)warning.c_str());
			}
		}	
		else
			afk->afk_duration = 0;

		afk->move_count = 0;

		return true; // keep the timer
	}
}