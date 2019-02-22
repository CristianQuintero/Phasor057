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

#include "Alias.h"
#include "Database.h"
#include "Timers.h"
#include "Common.h"
#include "Game.h"

namespace alias
{
	DWORD dwAuxThreadId = 0, dwMainThreadId = 0;
	Database *db = 0;
	bool do_track = true; // we track alias data by default

	struct alias_ts
	{
		std::wstring whash;
		std::wstring name;
	};

	// --------------------------------------------------------------------
	// Called from the main server thread
	
	// Called when a player joins, adds their alias data
	void OnPlayerJoin(halo::h_player* player)
	{
		if (!do_track)
			return;

		alias_ts* data = new alias_ts;

		// continue only if successfully allocated
		if (!data)
			return;

		data->whash = player->whash;
		data->name = player->mem->playerName;

		// send to processing thread
		ThreadTimer::AddToQueue(dwAuxThreadId, 0, ALIAS_ADD_DATA, (LPARAM)data);						
	}

	// Toggles tracking of alias data
	bool sv_alias(halo::h_player* exec, std::vector<std::string>& tokens)
	{
		if (tokens.size() == 2)
		{
			if (tokens[1] == "true" || tokens[1] == "1")
			{
				if (!do_track)
				{
					do_track = true;
					halo::hprintf(L"Player tracking has been enabled (this is the default).");	
				}
				else
					halo::hprintf(L"Player tracking is already enabled.");
			}
			else
			{
				do_track = false;
				halo::hprintf("Player tracking has been disabled. Add to init file to make default.");
			}
		}
		else
			halo::hprintf("Correct usage: sv_alias <status>");

		return true;
	}

	// searches for all entries matching specified hash
	bool sv_alias_hash(halo::h_player* exec, std::vector<std::string>& tokens)
	{
		if (tokens.size() == 2)
		{
			std::wstring hash;

			// If they passed a player number, get the player's hash
			if (tokens[1].size() <= 2)
			{
				int p_playerId = atoi(tokens[1].c_str()) - 1; // player ids are different from memory ids
				halo::h_player* player = game::GetPlayer_pi(p_playerId);

				if (player)
					hash = player->whash;
				else
					halo::hprintf(L"sv_alias: The specified player doesn't exist.");
			}
			else
				hash = ToWideChar(tokens[1]);

			if (hash.size() == 32) // process the message
			{
				halo::hprintf(L"Fetching results..");

				std::wstring* search = new std::wstring;
				*search = hash;
				ThreadTimer::AddToQueue(dwAuxThreadId, 0, ALIAS_SEARCH_HASH, (LPARAM)search);	
			}
			else if (hash.size())
				halo::hprintf(L"The hash should be 32 characters long.");
		}			
		else
			halo::hprintf(L"Correct usage: sv_alias <player or hash>");

		return true;
	}

	// searches for all names matching input (partial searches supported)
	bool sv_alias_search(halo::h_player* exec, std::vector<std::string>& tokens)
	{
		if (tokens.size() == 2)
		{
			halo::hprintf("Fetching results..");

			std::wstring* search = new std::wstring;
			*search = ToWideChar(tokens[1]);
			ThreadTimer::AddToQueue(dwAuxThreadId, 0, ALIAS_SEARCH_DATA, (LPARAM)search);

		}
		else
			halo::hprintf("Correct usage: sv_alias_search <partial name to find>");

		return true;
	}

	// --------------------------------------------------------------------
	// Called from the auxillary thread
	// 
	// Adds a hash name entry to the alias database
	bool alias_add_data(LPARAM data)
	{
		alias_ts* player_info = (alias_ts*)data;

		if (player_info)
		{
			// before adding to the database we need to check if it already
			// exists
			
			// build the query
			std::wstring query = m_swprintf_s(L"SELECT name FROM alias WHERE hash=\"%s\";", player_info->whash.c_str());
			std::vector<std::vector<std::wstring>> result = db->query(query.c_str());

			// check if the name is already being tracked
			bool bExists = false;
			for(std::vector<std::vector<std::wstring>>::iterator it = result.begin();
				it < result.end(); it++)
			{
				std::vector<std::wstring> row = *it;

				// should only be one entry in the row
				if (row.size() == 1 && row[0] == player_info->name)
				{
					bExists = true;
					break;
				}
			}
			
			// if this name isn't already tracked, add it to the database
			if (!bExists)
			{
				query = m_swprintf_s(L"INSERT INTO alias(hash,name) VALUES('%s', \"%s\");", player_info->whash.c_str(), player_info->name.c_str());
				db->query(query.c_str());
			}

			// cleanup
			delete player_info;
			
		}

		return false; // don't keep timer
	}

	// Searches the alias database for a match string (or partial string)
	bool alias_search_data(LPARAM data)
	{
		if (data)
		{
			std::wstring* search = (std::wstring*)data;

			std::wstring query = m_swprintf_s(L"SELECT * FROM alias WHERE name LIKE \"%s\";", search->c_str());
			std::vector<std::vector<std::wstring>> result = db->query(query.c_str());

			threaded::safe_hprintf(L"%i players matching \"%s\"", result.size(), search->c_str());

			for(std::vector<std::vector<std::wstring>>::iterator it = result.begin();
				it < result.end(); it++)
			{
				std::vector<std::wstring> row = *it;

				// should only be two entries in the row
				if (row.size() == 2)
					threaded::safe_hprintf(L"%s - %s", row[1].c_str(), row[0].c_str());
			}

			delete search;
		}
		
		return false; // don't keep
	}

	// Searches the alias database for all names matching a hash
	bool alias_search_hash(LPARAM data)
	{
		if (data)
		{
			std::wstring* search = (std::wstring*)data;

			std::wstring query = m_swprintf_s(L"SELECT name FROM alias WHERE hash=\"%s\";", search->c_str());
			std::vector<std::vector<std::wstring>> result = db->query(query.c_str());

			threaded::safe_hprintf(L"%i players matching hash \"%s\"", result.size(), search->c_str());

			for(std::vector<std::vector<std::wstring>>::iterator it = result.begin();
				it < result.end(); it++)
			{
				std::vector<std::wstring> row = *it;

				// should only be two entries in the row
				if (row.size() == 1)
					threaded::safe_hprintf(L"%s", row[0].c_str());
			}

			delete search;
		}

		return false;
	}

	// --------------------------------------------------------------------

	// Called by the auxillary thread when its initializing
	void Setup()
	{		
		std::string dir = GetWorkingDirectory() + "\\data\\alias.sqlite";
		db = new Database(dir.c_str());
		db->query(L"CREATE TABLE IF NOT EXISTS alias (hash TEXT, name TEXT);");

		dwAuxThreadId = threaded::GetAuxillaryThreadId();
		dwMainThreadId = threaded::GetMainThreadId();

		// register timers
		ThreadTimer::RegisterMsg(dwAuxThreadId, ALIAS_ADD_DATA, alias_add_data);
		ThreadTimer::RegisterMsg(dwAuxThreadId, ALIAS_SEARCH_DATA, alias_search_data);
		ThreadTimer::RegisterMsg(dwAuxThreadId, ALIAS_SEARCH_HASH, alias_search_hash);
	}

	// Called by the auxillary thread when its terminating
	void Cleanup()
	{
		db->close();
		delete db;
	}
}