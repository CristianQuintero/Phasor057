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

#include "Admin.h"
#include "Common.h"
#include "Halo.h"
#include "Game.h"
#include "Server.h"

namespace admin
{
	std::string szAdminPath, szAccessPath;
	std::vector<adminInfo> adminList;
	std::vector<accessLevel> accessList;

	// Loads/reloads the access list
	void LoadAccesslist();

	// --------------------------------------------------------------------
	// File/Loading functions.. 
	// 
	// Loads and processes the admin/access files
	void InitializeAdminSystem()
	{
		szAdminPath = GetWorkingDirectory() + "\\admin.txt";
		szAccessPath = GetWorkingDirectory() + "\\access.ini";

		std::vector<std::string> fileData;
		LoadFileIntoVector(szAdminPath.c_str(), fileData);

		int i = 0;
		for (std::vector<std::string>::iterator itr = fileData.begin();
			itr != fileData.end(); itr++, i++)
		{
			std::vector<std::string> tokens = TokenizeString(*itr, ",");

			if (tokens.size() == 3)
			{
				adminInfo a;
				a.name = tokens[0];
				a.hash = tokens[1];
				a.level = atoi(tokens[2].c_str());

				halo::hprintf("Adding admin %s", a.name.c_str());
				adminList.push_back(a);
			}
			else
			{
				// Log the error
				logging::LogData(LOGGING_PHASOR, "Line %i in %s is incorrectly formatted.", i + 1, szAdminPath.c_str());
			}
		}

		LoadAccesslist();
	}

	// Loads/reloads the access list
	void LoadAccesslist()
	{
		// Clear the current access list
		accessList.clear();

		// Load the access data
		char outBuffer[4096] = {0};
		DWORD count = GetPrivateProfileSectionNames(outBuffer, sizeof(outBuffer), szAccessPath.c_str());

		if (count > 0)
		{
			DWORD processedCount = 0;		

			// Loop through all entries and read their data.
			while (processedCount < count - 1)
			{
				char dataBuffer[4096] = {0};

				accessLevel acc;
				acc.level = atoi(outBuffer + processedCount);

				DWORD dataCount = GetPrivateProfileString(outBuffer + processedCount, "data", "", dataBuffer, sizeof(dataBuffer), szAccessPath.c_str());

				std::vector<std::string> tokens = TokenizeString(dataBuffer, ",");
				for (size_t x = 0; x < tokens.size(); x++) // add commands to access level
				{
					if (tokens[x] == "-1")
						acc.global = true;

					acc.cmds.push_back(tokens[x]);
				}

				accessList.push_back(acc);

				processedCount += strlen(outBuffer + processedCount) + 1;			
			}
		}
	}

	// Saves the current admin list to file, replacing its contents.
	void SaveAdmins()
	{
		FILE * pFile = fopen(szAdminPath.c_str(), "w");

		if (pFile)
		{
			std::vector<adminInfo>::iterator itr = adminList.begin();

			while (itr != adminList.end())
			{
				fprintf(pFile, "%s,%s,%i\r\n", itr->name.c_str(), itr->hash.c_str(), itr->level);
				itr++;
			}

			fclose(pFile);
		}
		else
		{
			std::string err = GetLastErrorAsText();

			// Log the error
			logging::LogData(LOGGING_PHASOR, "Cannot save the admin file to %s, reason: %s", szAdminPath.c_str(), err.c_str());
		}
	}

	// Returns the number of admins there are
	int GetAdminCount()
	{
		return (int)adminList.size();

	}

	// --------------------------------------------------------------------
	// Validation functions
	// 
	// Checks if a player is an admin
	bool IsAdmin(std::string hash, adminInfo & data)
	{
		bool is_admin = false;

		for (size_t x = 0; x < adminList.size(); x++)
		{
			if (adminList[x].hash == hash)
			{
				is_admin = true;
				data = adminList[x];
				break;
			}
		}

		return is_admin;
	}

	std::vector<accessLevel>::iterator GetAccessLevel(int level)
	{
		std::vector<accessLevel>::iterator itr = accessList.begin();
		while (itr != accessList.end())
		{
			if (itr->level == level)
				break;

			itr++;
		}

		return itr;
	}
	// Checks if an access level is allowed to use an rcon command
	bool CanUseCommand(int level, std::string command)
	{
		std::vector<accessLevel>::iterator itr = GetAccessLevel(level);

		bool bCanUse = false;

		if (itr != accessList.end())
		{
			if (itr->global) // global means they can use any command
				bCanUse = true;
			else
			{
				for (size_t x = 0; x < itr->cmds.size(); x++)
				{
					if (itr->cmds[x] == command)
					{
						bCanUse = true;
						break;
					}
				}
			}
		}

		return bCanUse;
	}

	// Checks if an access level is valid
	bool ValidateAccessLevel(int level)
	{
		return (GetAccessLevel(level) != accessList.end());
	}

	// --------------------------------------------------------------------
	// Implementation functions (sv_admin_add etc)
	// 
	bool sv_admin_add(halo::h_player* exec, std::vector<std::string>& tokens)
	{
		if (tokens.size() == 4)
		{
			bool isValid = true;

			// check if they're using a player's number
			if (tokens[1].size() <= 2)
			{
				int p_playerId = atoi(tokens[1].c_str()) - 1; // player ids are different from memory ids
				halo::h_player* player = game::GetPlayer_pi(p_playerId);

				if (!player || !player->hash.size())
				{
					halo::hprintf("sv_admin_add: The specified player's hash couldn't be found, make sure they exist.");

					isValid = false;
				}
				else
					tokens[1] = player->hash;
			}
			else
			{
				// validate hash length
				if (tokens[1].size() != 32)
				{
					halo::hprintf("sv_admin_add: The hash code needs to be 32 characters, you entered %i.", tokens[1].size());

					isValid = false;
				}
			}

			// Make sure the hash is valid
			if (isValid)
			{
				int access = atoi(tokens[3].c_str());

				std::string name = tokens[2], hash = tokens[1];

				try
				{
					// Make sure the data isn't in use
					for (size_t x = 0; x < adminList.size(); x++)
					{
						// Check if name is in use
						if (adminList[x].name == name)
						{
							std::string err = m_sprintf_s("sv_admin_add: The name %s is already is use.", name.c_str());
							throw std::exception(err.c_str());
						}

						if (adminList[x].hash == hash)
						{
							std::string err = m_sprintf_s("sv_admin_add: The hash %s is already is use.", hash.c_str());
							throw std::exception(err.c_str());
						}
					}

					if (!ValidateAccessLevel(access))
					{
						std::string err = m_sprintf_s("sv_admin_add: The access level %i doesn't exist.", access);
						throw std::exception(err.c_str());
					}

					// Add to the vector
					adminInfo a;
					a.hash = hash;
					a.level = access;
					a.name = name;
					adminList.push_back(a);

					halo::hprintf("%s can now use rcon at level %i.", name.c_str(), access);

					// Check if the player is currently in the server
					std::vector<halo::h_player*> players = game::GetPlayerList();

					for (std::vector<halo::h_player*>::iterator itr = players.begin(); 
						itr != players.end(); itr++)
					{
						if ((*itr)->hash == hash)
						{
							(*itr)->is_admin = true;
							(*itr)->admin_info = a;
							break;
						}
					}

					SaveAdmins();

				}
				catch (std::exception & e)
				{
					halo::hprintf("%s", e.what());
				}
			}								
		}
		else
			halo::hprintf("Correct usage: sv_admin_add <player number or hash> <auth name> <level>");

		return true;
	}

	bool sv_admin_del(halo::h_player* exec, std::vector<std::string>& tokens)
	{
		if (tokens.size() == 2)
		{
			int index = atoi(tokens[1].c_str()) - 1;

			if (index < adminList.size() && index >= 0)
			{
				// remove the admin
				std::vector<adminInfo>::iterator itr = adminList.begin() + index;
				halo::hprintf("%s is no longer an admin", itr->name.c_str());
				adminList.erase(itr);
				SaveAdmins();
			}
			else
				halo::hprintf("Invalid index, the admin couldn't be removed.");
		}
		else
			halo::hprintf("Correct usage: sv_admin_del <index>");

		return true;
	}

	bool sv_admin_list(halo::h_player* exec, std::vector<std::string>& tokens)
	{
		halo::hprintf("Current admins are:");
		halo::hprintf("Pos:    Auth Name:                      Access Level:");

		int count = 1;
		for (std::vector<adminInfo>::iterator itr = adminList.begin();
			itr != adminList.end(); itr++, count++)
		{
			std::string disp = itr->name;
			int len = disp.size();
			int tabCount = 5 - len / 8;

			if (len % 8) 
				tabCount--;

			for (int i = 0; i < tabCount; i++)
				disp += "\t";

			std::string dispMsg = m_sprintf_s("[%i]\t%s%i", count, disp.c_str(), itr->level);
			dispMsg = ExpandTabsToSpace(dispMsg.c_str());
			halo::hprintf("%s", dispMsg.c_str());
		}

		return true;
	}

	bool sv_admin_cur(halo::h_player* exec, std::vector<std::string>& tokens)
	{
		halo::hprintf("Current admins in the server are:");

		std::vector<halo::h_player*> players = game::GetPlayerList();

		for (std::vector<halo::h_player*>::iterator itr = players.begin(); 
			itr != players.end(); itr++)
		{
			if ((*itr)->is_admin)
				halo::hprintf(L"Player %i: %s (authed as %s)", (*itr)->mem->playerNum + 1, (*itr)->mem->playerName, ToWideChar((*itr)->admin_info.name.c_str()).c_str());
		}

		return true;
	}

	bool sv_reloadaccess(halo::h_player* exec, std::vector<std::string>& tokens)
	{
		// Reload the file
		admin::LoadAccesslist();
		halo::hprintf("The access list has been reloaded.");

		return true;
	}

	bool sv_commands(halo::h_player* exec, std::vector<std::string>& tokens)
	{
		std::vector<std::string> output;

		bool all_cmds = false;

		if (exec && exec->is_admin)
		{
			std::vector<accessLevel>::iterator itr = GetAccessLevel(exec->admin_info.level);

			if (itr != accessList.end())
			{
				if (itr->global)
					all_cmds = true;
				else
					output = itr->cmds;
			}
		}
		else // server console can use all commands
			all_cmds = true;

		if (all_cmds)
		{
			halo::hprintf("You're allowed to use all commands, they're listed below:");

			server::PhasorCommands* cmds = server::GetConsoleCommands();

			for (int i = 0; cmds[i].key; i++) // build list of commands from table
				output.push_back(cmds[i].key);
		}		
		else
			halo::hprintf("The commands you're allowed to use are below:");
		
		halo::OutputInColumns(3, output);

		return true;
	}
};