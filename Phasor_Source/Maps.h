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

#include "Game.h"
#include "Halo.h"
#include "MapLoader.h"
#include <string>
#include <vector>

namespace game
{
	namespace maps
	{
		// This function returns the gametype data used for maps
		bool GetGametypeData(std::string mbGametype, LPBYTE szOutput, DWORD dwSize);

		// Returns the (base) name of the loaded map
		const char* GetCurrentMap();

		// Called when a map is being loaded
		bool OnMapLoad(LPBYTE mapData);

		// Called when a map is successfully loaded
		std::string OnGameStart();

		namespace vote
		{
			// Structure used for storing votable maps
			struct cmap
			{
				std::string map;
				std::string gametype;
				std::string description;	
				std::vector<std::string> scripts;
			};

			// Returns a boolean representing whether or not voting is enabled
			bool IsEnabled();

			// Called when a map vote starts
			void OnBegin();

			// Called when the map vote is over
			bool OnEnd(cmap & decision);

			// Called when there's server chat
			bool OnServerChat(halo::h_player* player, wchar_t* msg);

			// Called when a map is to be added to vote options
			void AddVotableMap(const char* map, const char* gametype, const char* description, std::vector<std::string> scripts);
		
			// Removes a map from the vote options
			bool RemoveVotableMap(DWORD index);

			// Toggles map voting
			bool SetMapVote(bool new_status);

			bool sv_mapvote(halo::h_player* exec, std::vector<std::string>& tokens);
			bool sv_mapvote_begin(halo::h_player* exec, std::vector<std::string>& tokens);
			bool sv_mapvote_add(halo::h_player* exec, std::vector<std::string>& tokens);
			bool sv_mapvote_del(halo::h_player* exec, std::vector<std::string>& tokens);
			bool sv_mapvote_list(halo::h_player* exec, std::vector<std::string>& tokens);
		}

		namespace scriptmanager
		{
			// Loads scripts for the current game
			void Load();

			// Hooked functions
			bool sv_mapcycle_begin(halo::h_player* exec, std::vector<std::string>& tokens);			
			bool sv_mapcycle_add(halo::h_player* exec, std::vector<std::string>& tokens);
			bool sv_mapcycle_del(halo::h_player* exec, std::vector<std::string>& tokens);
			bool sv_mapcycle(halo::h_player* exec, std::vector<std::string>& tokens);
			bool sv_map(halo::h_player* exec, std::vector<std::string>& tokens);
			bool sv_end_game(halo::h_player* exec, std::vector<std::string>& tokens);

			// Custom functions
			bool sv_reloadscripts(halo::h_player* exec, std::vector<std::string>& tokens);	
		};
	};
};