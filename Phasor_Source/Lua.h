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

extern "C"
{
#include "../lua5.14/lua.h"
#include "../lua5.14/lualib.h"
#include "../lua5.14/lauxlib.h"
}

#include "Version.h"

namespace Lua
{
	// Returns true if a script function is currently being executed
	bool IsScriptExecuting();

	// Checks if a script exists
	bool CheckScriptVailidity(std::string szFile);

	// Loads the specified lua script
	bool LoadScript(std::string szFile);

	// Closes the specified lua script
	void CloseScript(std::string szFile);

	// Closes all scripts
	void CloseAllScripts();

	void ReloadScripts();

	// Used for calling script functions
	void funcCall(const char* function, char* desc, ...);
	int rfuncCall(int defaultValue, const char* function, char* desc, ...);

	// Functions Phasor lets script have acess to
	// See documentation for descriptions
	// --------------------------------------------------------------------
	int l_updateammo(lua_State* luaVM);
	// Game commands
	int l_resolveplayer(lua_State* luaVM);
	int l_rresolveplayer(lua_State* luaVM);
	int l_getobject(lua_State* luaVM);
	int l_getplayer(lua_State* luaVM);
	int l_getplayerobjectid(lua_State* luaVM);
	int l_objecttoplayer(lua_State* luaVM);
	int l_getobjectcoords(lua_State* luaVM);
	// Game action commands
	int l_changeteam(lua_State* luaVM);
	int l_kill(lua_State* luaVM);
	int l_applycamo(lua_State* luaVM);
	int l_setspeed(lua_State* luaVM);
	int l_moveobjcoords(lua_State* luaVM);
	int l_moveobjname(lua_State* luaVM);
	// Game data commands
	int l_getteam(lua_State* luaVM);
	int l_getname(lua_State* luaVM);
	int l_gethash(lua_State* luaVM);
	int l_isadmin(lua_State* luaVM);
	int l_getteamsize(lua_State* luaVM);
	// Server output commands
	int l_hprintf(lua_State* luaVM);
	int l_svcmd(lua_State* luaVM);
	int l_say(lua_State* luaVM);
	int l_privatesay(lua_State* luaVM);
	// Tag commands
	int l_lookuptag(lua_State* luaVM);
	// Memory commands
	int l_writebit(lua_State* luaVM);
	int l_writebyte(lua_State* luaVM);
	int l_writeword(lua_State* luaVM);
	int l_writedword(lua_State* luaVM);
	int l_writefloat(lua_State* luaVM);
	int l_readbit(lua_State* luaVM);
	int l_readbyte(lua_State* luaVM);
	int l_readword(lua_State* luaVM);
	int l_readdword(lua_State* luaVM);
	int l_readfloat(lua_State* luaVM);
	// Object management
	int l_createobject(lua_State* luaVM);
	int l_destroyobject(lua_State* luaVM);
	int l_assignweapon(lua_State* luaVM);
	int l_entervehicle(lua_State* luaVM);
	int l_exitvehicle(lua_State* luaVM);
	// Generic commands
	int l_registertimer(lua_State* luaVM);
	int l_removetimer(lua_State* luaVM);
	int l_getrandomnumber(lua_State* luaVM);
	int l_gettokencount(lua_State* luaVM);
	int l_gettoken(lua_State* luaVM);
	int l_getcmdtokencount(lua_State* luaVM);
	int l_getcmdtoken(lua_State* luaVM);
	int l_getprofilepath(lua_State* luaVM);
}

