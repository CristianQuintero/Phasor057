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

#include "Teams.h"
#include "Server.h"

namespace game
{
	namespace teams
	{
		bool bCanChange = true; // controls whether or not they can change team

		// Returns a boolean specfiying whether or not a player can change team
		bool CanChange()
		{
			return bCanChange;
		}

		// Server commands
		// ----------------------------------------------------------------
		bool sv_teams_lock(halo::h_player* exec, std::vector<std::string> & tokens)
		{
			if (bCanChange)
			{
				bCanChange = false;

				halo::hprintf("The teams have been locked.");
			}
			else
				halo::hprintf("The teams are already locked.");

			return true;
		}

		bool sv_teams_unlock(halo::h_player* exec, std::vector<std::string> & tokens)
		{
			if (!bCanChange)
			{
				bCanChange = true;

				halo::hprintf("The teams have been unlocked.");
			}
			else
				halo::hprintf("The teams aren't locked.");

			return true;
		}

		bool sv_teams_balance(halo::h_player* exec, std::vector<std::string> & tokens)
		{
			if (tokens.size() == 1)
			{
				// Get the team counts
				std::vector<halo::h_player*> redTeam, blueTeam, bigTeam;
				std::vector<halo::h_player*> players = game::GetPlayerList();

				for (size_t x = 0; x < players.size(); x++)
				{
					if (players[x]->mem->team == 0)
						redTeam.push_back(players[x]);
					else if (players[x]->mem->team == 1)
						blueTeam.push_back(players[x]);
					else
					{
						halo::hprintf("sv_teams_balance is only for team games.");
						return true;
					}
				}
				
				// Check if they need balancing
				int dwDifference = 0;
				DWORD dwRedCount = redTeam.size(), dwBlueCount = blueTeam.size();

				if (dwBlueCount > dwRedCount)
				{
					dwDifference = dwBlueCount - dwRedCount;
					bigTeam = blueTeam;
				}
				else if (dwRedCount > dwBlueCount)
				{
					dwDifference = dwRedCount - dwBlueCount;
					bigTeam = redTeam;
				}

				if (dwDifference > 1)
				{
					// Check if there is work to do
					while (dwDifference > 1)
					{		
						int index = rand() % bigTeam.size();
						halo::h_player* p = bigTeam[index];

						// remove from vector
						bigTeam.erase(bigTeam.begin() + index);

						// change team and force death
						p->ChangeTeam(true);

						// Adjust the difference
						dwDifference -= 2;
					}

					// Notify the server
					server::GlobalMessage(L"The teams have been balanced.");
					halo::hprintf("The teams are now balanced.");
				}
				else
					halo::hprintf("The teams are already balanced.");
			}
			else
				halo::hprintf("Syntax: sv_teams_balance");

			return true;
		}

		bool sv_changeteam(halo::h_player* exec, std::vector<std::string> & tokens)
		{
			if (tokens.size() == 2)
			{
				halo::h_player* player = game::GetPlayer_pi(atoi(tokens[1].c_str())-1);

				if (player)
				{
					player->ChangeTeam(true);
					halo::hprintf("The player's team has been changed to %s", (player->mem->team == 0) ? "Red" : "Blue");
				}
				else
					halo::hprintf(L"You entered an invalid player index.");				
			}
			else
				halo::hprintf("Correct usage: sv_changeteam <player>");

			return true;
		}
	}
}