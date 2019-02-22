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
#include <string>
#include <vector>

namespace halo
{
	class h_player;
}

namespace admin
{
	struct adminInfo
	{
		std::string hash;
		std::string name;
		int level;
	};

	struct accessLevel
	{
		int level;
		bool global;
		std::vector<std::string> cmds;

		accessLevel()
		{
			global = false;
			level = -1;
		}
	};

	// Loads and processes the admin/access files
	void InitializeAdminSystem();

	// Checks if a player is allowed to use an rcon command
	bool CanUseCommand(int acessLevel, std::string command);

	// Checks if an access level is valid
	bool ValidateAccessLevel(int level);

	// Checks if a player is an admin
	bool IsAdmin(std::string hash, adminInfo & data);

	// Loads/reloads the access list
	void LoadAccesslist();

	// Saves the current admin list to file, replacing its contents.
	void SaveAdmins();

	// Returns the number of admins there are
	int GetAdminCount();

	bool sv_admin_add(halo::h_player* exec, std::vector<std::string>& tokens);
	bool sv_admin_del(halo::h_player* exec, std::vector<std::string>& tokens);
	bool sv_admin_list(halo::h_player* exec, std::vector<std::string>& tokens);
	bool sv_admin_cur(halo::h_player* exec, std::vector<std::string>& tokens);
	bool sv_reloadaccess(halo::h_player* exec, std::vector<std::string>& tokens);
	bool sv_commands(halo::h_player* exec, std::vector<std::string>& tokens);
};