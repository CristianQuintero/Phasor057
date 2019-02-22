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

#include "Lua.h"
#include "Common.h"
#include "Halo.h"
#include "Game.h"
#include "objects.h"
#include "Server.h"
#include "Timers.h"
#include "Addresses.h"
#include <stack>

#pragma comment(lib, "../Release/lua5.14.lib")

// List of versions this one is backwards compatible with
DWORD numVers[] = {10057};

namespace Lua
{
	struct lua_info
	{
		lua_State* luaVM;
		std::string absolutePath;
		std::string fileName;
		std::vector<std::string> errFuncs;
		//std::vector<timerInfo*> timers;
	};

	struct luaArgs 
	{
		char* s;
		float f;
		unsigned long l;
		int i;
		bool b;
		char active; // which one is being used
	};

	struct luaExports
	{
		(int)(*fn)(lua_State* luaVM);
		const char* key;
	};

	luaExports luaExportTable[] =
	{
		{&l_updateammo, "updateammo"},
		{&l_hprintf, "hprintf"},
		{&l_writebit, "writebit"},
		{&l_readbit, "readbit"},
		{&l_resolveplayer, "resolveplayer"},
		{&l_rresolveplayer, "rresolveplayer"},
		{&l_getobject, "getobject"},
		{&l_getplayer, "getplayer"},
		{&l_getplayerobjectid, "getplayerobjectid"},
		{&l_objecttoplayer, "objecttoplayer"},
		{&l_getobjectcoords, "getobjectcoords"},
		{&l_changeteam, "changeteam"},
		{&l_kill, "kill"},
		{&l_applycamo, "applycamo"},
		{&l_getteam, "getteam"},
		{&l_writebit, "writebit"},
		{&l_writebyte, "writebyte"},
		{&l_writeword, "writeword"},
		{&l_writeword, "writeint"},
		{&l_writedword, "writedword"},
		{&l_writefloat, "writefloat"},
		{&l_readbit, "readbit"},
		{&l_readbyte, "readbyte"},
		{&l_readword, "readword"},
		{&l_readfloat, "readint"},
		{&l_readdword, "readdword"},
		{&l_readfloat, "readfloat"},
		{&l_lookuptag, "lookuptag"},
		{&l_setspeed, "setspeed"},
		{&l_say, "say"},
		{&l_privatesay, "privatesay"},
		{&l_registertimer, "registertimer"},
		{&l_removetimer, "removetimer"},
		{&l_getrandomnumber, "getrandomnumber"},
		{&l_getname, "getname"},
		{&l_gethash, "gethash"},
		{&l_isadmin, "isadmin"},
		{&l_getteamsize, "getteamsize"},
		{&l_svcmd, "svcmd"},
		{&l_moveobjcoords, "movobjcoords"},
		{&l_moveobjname, "movobjname"},
		{&l_moveobjcoords, "moveobjcoords"}, // typo from previous builds
		{&l_moveobjname, "moveobjname"}, // typo from previous builds
		{&l_createobject, "createobject"},
		{&l_destroyobject, "destroyobject"},
		{&l_assignweapon, "assignweapon"},
		{&l_entervehicle, "entervehicle"},
		{&l_exitvehicle, "exitvehicle"},
		{&l_gettokencount, "gettokencount"},
		{&l_gettoken, "gettoken"},
		{&l_getcmdtokencount, "getcmdtokencount"},
		{&l_getcmdtoken, "getcmdtoken"},
		{&l_getprofilepath, "getprofilepath"},
		{NULL, NULL}
	};

	// Calls a Lua script function, only used within this file
	bool call(lua_info* lua, const char* function, char* desc, ...);
	int rcall(lua_info* lua, const char* function, char* desc,  ...);

	// frees all timers associated with a script
	void CleanupTimers(lua_State* lua_VM);

	std::vector<lua_info*> scriptList;
	std::stack<std::string> callStack; // keeps track of function calls

	// Returns true if a script function is currently being executed
	bool IsScriptExecuting()
	{
		return callStack.size() ? true:false;
	}

	std::string GetStackMsg(lua_State* luaVM)
	{
		std::string msg;
		int top = lua_gettop(luaVM);

		if (top != 0)
			msg = (char*)lua_tostring(luaVM, top);
		
		return msg;
	}

	// Checks if a script exists
	bool CheckScriptVailidity(std::string szFile)
	{
		bool exists = false;
		std::string absolutePath = GetWorkingDirectory() + "\\scripts\\" + szFile + ".lua";

		std::ifstream myfile(absolutePath.c_str());
		if (myfile.is_open())
		{
			exists = true;
			myfile.close();
		}

		return exists;
	}

	// Loads the specified lua script
	bool LoadScript(std::string szFile)
	{
		bool bSuccess = true;

		try
		{
			for (size_t x = 0; x < scriptList.size(); x++)
			{
				if (scriptList[x]->fileName == szFile)
				{
					const char* error = "LUA_EXCEPTION: Cannot load the script because it's already loaded.";
					halo::hprintf("%s", error);
					throw std::exception(error);
				}
			}

			// Resolve the directory
			std::string absolutePath = GetWorkingDirectory() + "\\scripts\\" + szFile + ".lua";
			lua_info* L_info = new lua_info;

			if (!L_info)
			{
				const char* error = "LUA_EXCEPTION: Cannot allocate memory for lua_state wrapper.";
				halo::hprintf("%s", error);
				throw std::exception(error);
			}

			L_info->absolutePath = absolutePath;
			L_info->fileName = szFile;

			// initialize Lua 
			L_info->luaVM = lua_open();

			if (!L_info->luaVM)
			{
				const char* error = "LUA_EXCEPTION: Cannot open the Lua interface.";
				halo::hprintf("%s", error);
				throw std::exception(error);
			}

			// load Lua base libraries 
			luaL_openlibs(L_info->luaVM);

			// Register the functions we want the lua script to have access to
			for (int i = 0; luaExportTable[i].fn; i++)
				lua_register(L_info->luaVM, luaExportTable[i].key, *(luaExportTable[i].fn));

			// load the script
			if (luaL_dofile(L_info->luaVM, absolutePath.c_str()))
			{
				std::string user_error = "LUA_EXCEPTION: The script (" + absolutePath + ") couldn't be loaded. Check the Phasor log for details.";
				//halo::hprintf("%s", user_error.c_str());

				// Get details about the error
				std::string error = GetStackMsg(L_info->luaVM);

				if (!error.size())
					error = "No details were available.";

				// Close the script
				lua_close(L_info->luaVM);

				delete L_info;

				std::string err = "LUA_EXCEPTION: The script (" + absolutePath + ") couldn't be loaded. Details:\n" + error;
				halo::hprintf("%s", err.c_str());
				throw std::exception(err.c_str());
			}

			// Check if the script supports this Phasor version
			int requiredVer = rcall(L_info, "GetRequiredVersion", 0);

			bool compatible = false;
			// Check if the script supports this Phasor build
			for (int i = 0; i < sizeof(numVers)/sizeof(DWORD); i++)
			{
				if (numVers[i] == requiredVer)
				{
					compatible = true;
					break;
				}
			}

			if (!compatible || requiredVer == -1)
			{
				// Close the script
				lua_close(L_info->luaVM);			

				std::string error;

				if (!compatible)
					error = m_sprintf_s("LUA_ERROR: The script (%s) doesn't support this version of Phasor (%s). It requires %i or another compatible build.", L_info->absolutePath.c_str(), PHASOR_BUILD_STRING, requiredVer);
				else
					error = m_sprintf_s("LUA_ERROR: The script (%s) was made for an earlier version of Phasor and cannot be loaded.", L_info->absolutePath.c_str());

				delete L_info;

				halo::hprintf("%s", error.c_str());
				throw std::exception(error.c_str());
			}

			if (!call(L_info, "OnScriptLoad", "l", GetCurrentProcessId()))
			{
				// Close the script
				lua_close(L_info->luaVM);

				delete L_info;

				std::string err = "LUA_EXCEPTION: The OnScriptLoad function for " + absolutePath + " failed to execute.";
				halo::hprintf("%s", err.c_str());
				throw std::exception(err.c_str());
			}

			// Add to the list
			scriptList.push_back(L_info);
		}
		catch (std::exception &e)
		{
			logging::LogData(LOGGING_PHASOR, "%s", e.what());

			bSuccess = false;
		}
		return bSuccess;
	}

	// Checks if a script is able to execute a function
	bool CheckFunctionValidity(const char* function, lua_info* vm)
	{
		// Make sure this script is allowed to use this func (hasn't errored it)
		for (size_t x = 0; x < vm->errFuncs.size(); x++)
		{
			if (vm->errFuncs[x] == function)
				return false;
		}

		return true;
	}

	// This is the base calling function for Lua states, all others
	// are wrappers around it
	int base_call(lua_info* lua, const char* function, char* desc, int nresults, va_list vl)
	{
		//halo::hprintf("Lua call %s from %08X", function, GetCurrentThreadId());
		int count = strlen(desc);

		// Check if the function is allowed to be called
		for (size_t x = 0; x < lua->errFuncs.size(); x++)
		{
			if (lua->errFuncs[x] == function)
				return -1; // Error
		}

		// Get the function
		lua_getglobal(lua->luaVM, function);

		for (int i = 0; i < count; i++)
		{
			switch (desc[i])
			{
			case 'n':
			case 'l':
				{
					DWORD val = va_arg(vl, DWORD);
					lua_pushnumber(lua->luaVM, val);
				} break;

			case 'f':
				{
					float val = va_arg(vl, float);
					lua_pushnumber(lua->luaVM, val);
				} break;

			case 'i':
				{
					int val = va_arg(vl, int);
					lua_pushinteger(lua->luaVM, val);

				} break;

			case 's':
				{
					char* val = va_arg(vl, char*);
					lua_pushstring(lua->luaVM, val);
				} break;

			case 'b':
				{
					bool val = va_arg(vl, bool);
					lua_pushboolean(lua->luaVM, val);
				} break;
			}
		}

		callStack.push(function);
		int r = lua_pcall(lua->luaVM, count, nresults, 0);
		callStack.pop();

		return r;
	}

	int base_call(lua_info* lua, const char* function, const char* desc, int nresults, std::vector<luaArgs> args)
	{
		int count = strlen(desc);

		if (count != args.size())
			return 1; // error

		// Check if the function is allowed to be called
		for (size_t x = 0; x < lua->errFuncs.size(); x++)
		{
			if (lua->errFuncs[x] == function)
				return -1; // Error
		}

		// Get the function
		lua_getglobal(lua->luaVM, function);

		for (int i = 0; i < count; i++)
		{
			switch (desc[i])
			{
			case 'n':
			case 'l':
				{
					lua_pushnumber(lua->luaVM, args[i].l);
				} break;

			case 'f':
				{
					lua_pushnumber(lua->luaVM, args[i].f);
				} break;

			case 'i':
				{
					lua_pushinteger(lua->luaVM, args[i].i);

				} break;

			case 's':
				{
					lua_pushstring(lua->luaVM, args[i].s);
				} break;

			case 'b':
				{
					lua_pushboolean(lua->luaVM, args[i].b);
				} break;
			}
		}

		callStack.push(function);
		int r = lua_pcall(lua->luaVM, count, nresults, 0);
		callStack.pop();

		return r;
	}

	// Calls a Lua script function
	bool call(lua_info* lua, const char* function, char* desc, ...)
	{
		// so it can be called with 0 for desc
		if (!desc)
			desc = "";

		bool bSuccess = true;

		// Get additional arguments
		va_list vl;
		va_start(vl, desc);

		// Call the function
		if (base_call(lua, function, desc, 0, vl))
			bSuccess = false; // Error

		va_end(vl);

		return bSuccess;
	}

	// Calls a lua script function
	int rcall(lua_info* lua, const char* function, char* desc,  ...)
	{
		// so it can be called with 0 for desc
		if (!desc)
			desc = "";

		int retValue = -1;

		// Get additional arguments
		va_list vl;
		va_start(vl, desc);

		// Call the function
		if (!base_call(lua, function, desc, 1, vl))
		{
			retValue = (int)lua_tointeger(lua->luaVM, -1);
			lua_pop(lua->luaVM, 1);
		}
		
		va_end(vl);

		return retValue;
	}

	int rcall(lua_info* lua, const char* function, const char* desc, std::vector<luaArgs> args)
	{
		// so it can be called with 0 for desc
		if (!desc)
			desc = "";

		int retValue = -1;

		// Call the function
		if (!base_call(lua, function, desc, 1, args))
		{
			retValue = (int)lua_tointeger(lua->luaVM, -1);
			lua_pop(lua->luaVM, 1);
		}

		return retValue;
	}


	// Used for calling script functions
	void funcCall(const char* function, char* desc, ...)
	{
		// so it can be called with 0 for desc
		if (!desc)
			desc = "";	

		// Loop through all loaded scripts
		std::vector<lua_info*>::iterator itr = scriptList.begin();
		while (itr != scriptList.end())
		{		
			// Get additional arguments
			va_list vl;
			va_start(vl, desc);

			// Call the function
			int scriptret = base_call(*itr, function, desc, 0, vl);

			if (scriptret != 0 && scriptret != -1)
			{
				// The function call failed, add it to blacklist
				(*itr)->errFuncs.push_back(function);

				halo::hprintf("Executing %s (script: %s) failed, further calls to this function will be ignored.", function, (*itr)->absolutePath.c_str());
			}
			
			itr++;

			va_end(vl);
		}
	}

	// Calls script functions and expects a return value
	int rfuncCall(int defaultValue, const char* function, char* desc, ...)
	{
		// so it can be called with 0 for desc
		if (!desc)
			desc = "";

		int val = defaultValue;

		// Loop through all loaded scripts
		std::vector<lua_info*>::iterator itr = scriptList.begin();
		while (itr != scriptList.end())
		{	
			// Get additional arguments
			va_list vl;
			va_start(vl, desc);

			// Call the function
			int scriptret = base_call(*itr, function, desc, 1, vl);

			if (scriptret == 0)
			{
				int type = lua_type((*itr)->luaVM, -1);
				int retValue = 0;

				if (type == LUA_TBOOLEAN)
					retValue = lua_toboolean((*itr)->luaVM, -1);
				else
					retValue = (int)lua_tointeger((*itr)->luaVM, -1);
			
				lua_pop((*itr)->luaVM, 1);

				// Only process the first acting return value
				if (val == defaultValue)
					val = retValue;
			}
			else if (scriptret != -1)
			{
				// The function call failed, add it to blacklist
				(*itr)->errFuncs.push_back(function);

				halo::hprintf("Executing %s (script: %s) failed, further calls to this function will be ignored.", function, (*itr)->absolutePath.c_str());
			}

			itr++;

			va_end(vl);
		}

		return val;
	}

	// Closes the specified lua script
	std::vector<lua_info*>::iterator CloseScript(std::vector<lua_info*>::iterator itr)
	{
		// Notify the script that it's being unloaded
		call((*itr), "OnScriptUnload", 0);		

		// Cleanup
		CleanupTimers((*itr)->luaVM);
		lua_close((*itr)->luaVM);
		delete (*itr);
		return scriptList.erase(itr);
	}

	// Closes the specified lua script
	void CloseScript(std::string szFile)
	{
		// Search the list for the script
		std::vector<lua_info*>::iterator itr = scriptList.begin();

		while (itr != scriptList.end())
		{
			if ((*itr)->fileName == szFile)
			{
				CloseScript(itr);
				break;
			}
			else
				itr++;
		}
	}

	// Closes all scripts
	void CloseAllScripts()
	{
		std::vector<lua_info*>::iterator itr = scriptList.begin();

		// Close all current scripts
		while (itr != scriptList.end())
		{
			itr = CloseScript(itr);	
		}

		scriptList.clear();
	}

	void ReloadScripts()
	{
		std::vector<std::string> filelist;

		for (std::vector<lua_info*>::iterator itr = scriptList.begin();
			itr != scriptList.end(); itr++)
			filelist.push_back((*itr)->fileName);

		CloseAllScripts();

		for (size_t x = 0; x < filelist.size(); x++)
		{
			LoadScript(filelist[x]);
		}
	}

	
	// Functions Phasor lets script have access to
	// See documentation for descriptions
	// --------------------------------------------------------------------
	bool GetValueOfType(lua_State* luaVM, luaArgs & arg, int index, char type)
	{
		// if a nil is passed and isn't being used as a string there is 
		// an error (without this check it will be converted to 0 and
		// produce unexpected results)
		int argtype = lua_type(luaVM, index);
		if (argtype == LUA_TNIL && type != 's')
			return false;

		switch (type)
		{
		case 's':
			{
				arg.s = (char*)lua_tostring(luaVM, index);
			} break;
		case 'l':
		case 'f':
		case 'i':
			{
				lua_Number num = lua_tonumber(luaVM, index);
				arg.l = (DWORD)num;
				arg.f = (float)num;
				arg.i = (int)num;
				arg.b = lua_toboolean(luaVM, index);
			}break;

		case 'b':
			{
				arg.b = (bool)lua_toboolean(luaVM, index);
				arg.i = arg.b;
			} break;
		}
		return true;
	}
	// Helper function for getting arguments passed by scripts
	// Makes sure arguments scripts pass are the correct format, if too
	// few arguments are passed then default values are assumed for the remaining
	bool GetLuaArguments(lua_State* luaVM, const char* format, std::vector<luaArgs> & output, bool bOverload=false)
	{
		int nexpect = strlen(format);

		if (!nexpect) // no arguments
			return true;

		int nargs = lua_gettop(luaVM);

		if (nargs < nexpect) // should always be at least what is expected
			return false;

		bool bSuccess = true;

		char* ptr = (char*)format;
		for (int index = nargs*-1; index < 0; index++, ptr++, nexpect--) // start popping arguments, starting at first
		{
			if (nexpect > 0)
			{
				luaArgs arg;
				bSuccess = GetValueOfType(luaVM, arg, index, *ptr);
				if (!bSuccess)
					break;
				arg.active = *ptr;
				output.push_back(arg);
			}
			else if (bOverload) // overload parameters
			{
				int type = lua_type(luaVM, index);
				char typeChar = 0;
				switch (type)
				{
				case LUA_TNUMBER:
					{
						typeChar = 'l'; // doent matter if its l,f or i
					} break;
				case  LUA_TBOOLEAN:
					{
						typeChar = 'b';
					} break;
				case LUA_TSTRING:
					{
						typeChar = 's';
					} break;
				}
				if (typeChar)
				{
					luaArgs arg;
					bSuccess = GetValueOfType(luaVM, arg, index, typeChar);

					if (!bSuccess)
						break;

					arg.active = typeChar;
					output.push_back(arg);
				}
			}
		}

		if (!bSuccess)
			output.clear();

		return bSuccess;
	}

	// SCRIPT FUNCTIONS
	// --------------------------------------------------------------------
	int l_updateammo(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "l", args))
		{
			DWORD weaponId = args[0].l;

			if (weaponId)
			{
				__asm
				{
					pushad
					mov ebx, 0
					mov ecx, weaponId
					call dword ptr ds:[FUNC_UPDATEAMMO]
					popad
				}
			}		
		}

		lua_pushnumber(luaVM, 0);
		return 0; // no return value
	}

	// Game commands
	int l_resolveplayer(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "l", args))
		{
			DWORD playerId = args[0].l;
			halo::h_player* player = game::GetPlayer(playerId);

			if (player)
			{
				lua_pushnumber(luaVM, player->mem->playerNum + 1);
				bSuccess = true;
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1; // 1 indicates there's a return value
	}

	int l_rresolveplayer(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "l", args))
		{
			DWORD playerNum = args[0].l - 1;
			halo::h_player* player = game::GetPlayer_pi(playerNum);

			if (player)
			{
				lua_pushnumber(luaVM, player->memoryId);
				bSuccess = true;
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1; // 1 indicates there's a return value
	}

	int l_getobject(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "l", args))
		{
			DWORD objId = args[0].l;
			PObject* obj = objects::GetObjectAddress(0, objId);
			if (obj)
			{
				lua_pushnumber(luaVM, (DWORD)obj);
				bSuccess = true;
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1;
	}

	int l_getplayer(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "l", args))
		{
			DWORD playerId = args[0].l;
			halo::h_player* player = game::GetPlayer(playerId);

			if (player)
			{
				lua_pushnumber(luaVM, (DWORD)player->mem);
				bSuccess = true;
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1;
	}

	int l_getplayerobjectid(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "l", args))
		{
			DWORD playerId = args[0].l;
			halo::h_player* player = game::GetPlayer(playerId);

			if (player)
			{
				lua_pushnumber(luaVM, (DWORD)player->mem->m_playerObjectid);
				bSuccess = true;
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1;
	}

	int l_objecttoplayer(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "l", args))
		{
			DWORD objId = args[0].l;
			PObject* m_obj = objects::GetObjectAddress(3, objId);
			halo::h_player* player = game::GetPlayer_obj(m_obj);

			if (player)
			{
				bSuccess = true;
				lua_pushnumber(luaVM, player->memoryId);
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1; // 1 indicates there's a return value
	}

	int l_getobjectcoords(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "l", args))
		{
			DWORD objId = args[0].l;
			PObject* m_obj = objects::GetObjectAddress(3, objId);

			if (m_obj)
			{
				m_obj = objects::GetPositionalObject(m_obj);

				lua_pushnumber(luaVM, m_obj->base.location.x);
				lua_pushnumber(luaVM, m_obj->base.location.y);
				lua_pushnumber(luaVM, m_obj->base.location.z);
				bSuccess = true;
			}
		}
		if (!bSuccess)
		{
			lua_pushnil(luaVM);
			lua_pushnil(luaVM);
			lua_pushnil(luaVM);
		}
		return 3; // 3 return values
	}
	// Game action commands
	int l_changeteam(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "lb", args))
		{
			DWORD playerId = args[0].l;
			bool forcedeath = args[1].b;			
			halo::h_player* player = game::GetPlayer(playerId);

			if (player)
				player->ChangeTeam(forcedeath);	
		}

		lua_pushnumber(luaVM, 0);
		return 0; // no return value
	}

	int l_kill(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "l", args))
		{
			DWORD playerId = args[0].l;
			halo::h_player* player = game::GetPlayer(playerId);
			if (player)
				player->Kill();
		}

		lua_pushnumber(luaVM, 0);
		return 0; // 1 indicates there's a return value
	}

	int l_applycamo(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "lf", args))
		{
			DWORD playerId = args[0].l;
			float duration = args[1].f;
			
			halo::h_player* player = game::GetPlayer(playerId);

			if (player)
				player->ApplyCamo(duration);
		}

		lua_pushnumber(luaVM, 0);
		return 0; // 1 indicates there's a return value
	}
	
	int l_setspeed(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "lf", args))
		{
			DWORD playerId = args[0].l;
			float speed = args[1].f;
			
			halo::h_player* player = game::GetPlayer(playerId);

			if (player)
				player->mem->speed = speed;
		}

		lua_pushnumber(luaVM, 0);
		return 0; // 1 indicates there's a return value
	}

	// Moves an object to specified coords (or position name)
	int l_moveobjcoords(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "lfff", args))
		{
			DWORD m_ObjectId = args[0].l;
			float x = args[1].f;
			float y = args[2].f;
			float z = args[3].f;
			bSuccess = objects::tp::MoveObject(m_ObjectId, x, y, z);
		}

		lua_pushboolean(luaVM, bSuccess);
		return 1;
	}

	int l_moveobjname(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "ls", args))
		{
			DWORD m_ObjectId = args[0].l;
			const char* location = args[1].s;
			bSuccess = objects::tp::MoveObject(m_ObjectId, location);
		}

		lua_pushboolean(luaVM, bSuccess);
		return 1;
	}

	// Object management
	int l_createobject(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "sslibfff", args))
		{
			const char* tagType = args[0].s;
			const char* tagName = args[1].s;
			DWORD parentId = args[2].l;
			int respawnTime = args[3].i;
			bool bRecycle = args[4].b;
			float x = args[5].f;
			float y = args[6].f;
			float z = args[7].f;

			//halo::hprintf("Creating %s - %s", tagType, tagName);
			vect3d location = {0};
			location.x = x;
			location.y = y;
			location.z = z;

			DWORD objId = objects::SpawnObject(tagType, tagName, parentId, 
				respawnTime, bRecycle, &location);

			if (objId != 0)
			{
				lua_pushnumber(luaVM, objId);
				bSuccess = true;
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1; // 1 indicates there's a return value
	}

	int l_destroyobject(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "l", args))
		{
			DWORD objId = args[0].l;
			objects::DestroyObject(objId);
		}

		lua_pushnumber(luaVM, 0);
		return 0; // 1 indicates there's a return value
	}

	int l_assignweapon(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "ll", args))
		{
			DWORD m_playerId = args[0].l;
			DWORD objId = args[1].l;			

			halo::h_player* player = game::GetPlayer(m_playerId);
			if (player)
				bSuccess = objects::AssignPlayerWeapon(player, objId);
		}
				
		lua_pushboolean(luaVM, bSuccess);
		return 1;
	}

	int l_entervehicle(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "lll", args))
		{
			DWORD m_playerId = args[0].l;
			DWORD vehicleId = args[1].l;
			DWORD seat = args[2].l;

			halo::h_player* player = game::GetPlayer(m_playerId);
			objects::EnterVehicle(player, vehicleId, seat);
		}

		lua_pushnumber(luaVM, 0);
		return 0; // 1 indicates there's a return value
	}

	int l_exitvehicle(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "l", args))
		{
			DWORD m_playerId = args[0].l;
			halo::h_player* player = game::GetPlayer(m_playerId);
			objects::ExitVehicle(player);
		}		

		lua_pushnumber(luaVM, 0);
		return 0; // 1 indicates there's a return value
	}

	// Game data commands
	int l_getteam(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "l", args))
		{
			DWORD playerId = args[0].l;
			halo::h_player* player = game::GetPlayer(playerId);

			if (player)
			{
				lua_pushnumber(luaVM, player->mem->team);
				bSuccess = true;
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1;
	}

	int l_getname(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "l", args))
		{
			DWORD playerId = args[0].l;
			halo::h_player* player = game::GetPlayer(playerId);

			if (player)
			{
				std::string name = ToChar(player->mem->playerName);
				lua_pushstring(luaVM, name.c_str());
				bSuccess = true;
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1; // 1 indicates there's a return value
	}

	int l_gethash(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "l", args))
		{
			DWORD playerId = args[0].l;
			halo::h_player* player = game::GetPlayer(playerId);

			if (player && player->hash.size())
			{
				lua_pushstring(luaVM, player->hash.c_str());
				bSuccess = true;
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);		
		
		return 1; // 1 indicates there's a return value
	}

	int l_isadmin(lua_State* luaVM)
	{
		bool bAdmin = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "l", args))
		{
			DWORD playerId = args[0].l;
			halo::h_player* player = game::GetPlayer(playerId);

			if (player && player->is_admin)
				bAdmin = true;
		}
		
		lua_pushboolean(luaVM, bAdmin);
		return 1;
	}

	int l_getteamsize(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "l", args))
		{
			DWORD team = args[0].l, count = 0;

			std::vector<halo::h_player*> players = game::GetPlayerList();
			for (std::vector<halo::h_player*>::iterator itr = players.begin(); 
				itr != players.end(); itr++)
			{
				if ((*itr)->mem->team == team)
					count++;
			}

			lua_pushnumber(luaVM, count);
		}
		else
			lua_pushnil(luaVM);	

		return 1;
	}

	// Server output commands
	int l_hprintf(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "s", args))
		{
			const char* msg = args[0].s;
			halo::hprintf("%s", msg);
		}

		lua_pushnumber(luaVM, 0);
		return 0;
	}

	int l_svcmd(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "s", args))
		{
			const char* cmd = args[0].s;
			halo::ExecuteServerCommand(cmd);
		}
		lua_pushnumber(luaVM, 0);
		return 0; // 1 indicates there's a return value
	}

	int l_say(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "s", args))
		{
			const char* cmd = args[0].s;
			server::GlobalMessage(L"%s", ToWideChar(cmd).c_str());
		}
		lua_pushnumber(luaVM, 0);
		return 0; // 1 indicates there's a return value
	}

	int l_privatesay(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "ls", args))
		{
			DWORD playerId = args[0].l;
			const char* msg = args[1].s;
			halo::h_player* player = game::GetPlayer(playerId);

			if (player)
				player->ServerMessage(ToWideChar(msg).c_str());
		}
		
		lua_pushnumber(luaVM, 0);
		return 0; // 1 indicates there's a return value
	}

	// Tag commands
	int l_lookuptag(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "ss", args))
		{
			char* tagType = args[0].s;
			char* tag = args[1].s;
			objects::hTagHeader* tagHeader = objects::LookupTag(tagType, tag);
			if (tagHeader)
			{
				lua_pushnumber(luaVM, (DWORD)tagHeader);
				bSuccess = true;
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1;
	}

	// Memory commands
	int l_writebit(lua_State* luaVM)
	{	
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "llll", args))
		{
			LPBYTE data_addr = (LPBYTE)args[0].l;
			DWORD data_offset = args[1].l;
			DWORD bit_offset = 7 - (args[2].l % 8);
			DWORD data = args[3].l;

			if (data_addr && bit_offset >= 0 && data_offset >= 0)
			{
				if (data)
					data_addr[data_offset] |= (1 << bit_offset); // set bit to 1	
				else
					data_addr[data_offset] &= ~(1 << bit_offset); // set bit to 0
			}			
		}

		lua_pushnumber(luaVM, 0);
		return 0;
	}

	int l_writebyte(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "lll", args))
		{
			LPBYTE data_addr = (LPBYTE)args[0].l;
			DWORD offset = args[1].l;
			DWORD data = args[2].l;

			if (data_addr && offset >= 0)
				data_addr[offset] = (BYTE)data;
		}

		lua_pushnumber(luaVM, 0);
		return 0;
	}

	int l_writeword(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "lll", args))
		{
			LPBYTE data_addr = (LPBYTE)args[0].l;
			DWORD offset = args[1].l;
			DWORD data = args[2].l;

			if (data_addr && offset >= 0)
				*(WORD*)(data_addr + offset) = (WORD)data;
		}

		lua_pushnumber(luaVM, 0);
		return 0;
	}

	int l_writedword(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "lll", args))
		{
			LPBYTE data_addr = (LPBYTE)args[0].l;
			DWORD offset = args[1].l;
			DWORD data = args[2].l;

			if (data_addr && offset >= 0)
				*(DWORD*)(data_addr + offset) = data;
		}

		lua_pushnumber(luaVM, 0);
		return 0;
	}

	int l_writefloat(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "llf", args))
		{
			LPBYTE data_addr = (LPBYTE)args[0].l;
			DWORD offset = args[1].l;
			float data = args[2].f;

			if (data_addr && offset >= 0)
				*(float*)(data_addr + offset) = data;
		}

		lua_pushnumber(luaVM, 0);
		return 0;
	}

	int l_readbit(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "lll", args))
		{
			LPBYTE data_addr = (LPBYTE)args[0].l;
			DWORD data_offset = args[1].l;
			DWORD bit_offset = 7 - (args[2].l % 8);

			if (data_addr && data_offset >= 0 && bit_offset >= 0)
			{
				BYTE bit = (data_addr[data_offset] & (1 << bit_offset)) >> bit_offset;
				lua_pushnumber(luaVM, bit);
				bSuccess = true;
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1;
	}

	int l_readbyte(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "ll", args))
		{
			LPBYTE data_addr = (LPBYTE)args[0].l;
			DWORD offset = args[1].l;

			if (data_addr && offset >= 0)
			{
				lua_pushnumber(luaVM, data_addr[offset]);
				bSuccess =  true;
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1;
	}

	int l_readword(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "ll", args))
		{
			LPBYTE data_addr = (LPBYTE)args[0].l;
			DWORD offset = args[1].l;

			if (data_addr && offset >= 0)
			{
				lua_pushnumber(luaVM, *(WORD*)(data_addr + offset));
				bSuccess =  true;
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1;
	}

	int l_readdword(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "ll", args))
		{
			LPBYTE data_addr = (LPBYTE)args[0].l;
			DWORD offset = args[1].l;

			if (data_addr && offset >= 0)
			{
				lua_pushnumber(luaVM, *(DWORD*)(data_addr + offset));
				bSuccess =  true;
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1;
	}

	int l_readfloat(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "ll", args))
		{
			LPBYTE data_addr = (LPBYTE)args[0].l;
			DWORD offset = args[1].l;

			if (data_addr && offset >= 0)
			{
				lua_pushnumber(luaVM, *(float*)(data_addr + offset));
				bSuccess =  true;
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1;
	}

	// General commands
	int l_getrandomnumber(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "ll", args))
		{
			DWORD min = args[0].l;
			DWORD max = args[1].l;
			DWORD result = 0;

			if (max == min)
				result = max;
			else
				result = min + rand() % (max - min);

			lua_pushnumber(luaVM, result);
		}
		else
			lua_pushnil(luaVM);

		return 1;
	}

	int l_gettokencount(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "ss", args))
		{
			const char* source = args[0].s;
			const char* delim = args[1].s;
			if (delim && source)
			{
				std::vector<std::string> tokens = TokenizeString(source, delim);
				lua_pushnumber(luaVM, tokens.size());
				bSuccess = true;
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1;
	}

	int l_gettoken(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "ssl", args))
		{
			const char* source = args[0].s;
			const char* delim = args[1].s;
			size_t token = (size_t)args[2].l;

			if (delim != 0 && source != 0)
			{
				std::vector<std::string> tokens = TokenizeString(source, delim);

				// Make sure the token they're after is valid
				if (tokens.size() > token)
				{
					lua_pushstring(luaVM, tokens[token].c_str());
					bSuccess = true;
				}
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1;
	}

	int l_getcmdtokencount(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "s", args))
		{
			const char* source = args[0].s;
			if (source != 0)
			{
				std::vector<std::string> tokens = TokenizeCommand(source);
				lua_pushnumber(luaVM, tokens.size());
				bSuccess = true;
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1;
	}

	int l_getcmdtoken(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "sl", args))
		{
			const char* source = args[0].s;
			size_t token = args[1].l;

			if (source != 0)
			{
				std::vector<std::string> tokens = TokenizeCommand(source);

				// Make sure the token they're after is valid
				if (tokens.size() > token)
				{
					lua_pushstring(luaVM, tokens[token].c_str());
					bSuccess = true;
				}				
			}
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1;
	}

	int l_getprofilepath(lua_State* luaVM)
	{
		std::string workingDir = GetWorkingDirectory();
		lua_pushstring(luaVM, workingDir.c_str());
		return 1;
	}

	// Script timer code
	// 
	// callback for script timers
	bool script_process_timer(LPARAM instance);

	struct stimer_info
	{
		std::string callback;
		std::vector<luaArgs> userdata;
		DWORD timer_id;
		DWORD count;
		lua_State* luaVM;

		stimer_info::stimer_info()
		{
			userdata.clear();
		}
	};

	// We need to track all script timers so we can remove them on unloading
	std::vector<stimer_info*> scriptTimers;

	lua_info* FindLuaObject(lua_State* luaVM)
	{
		lua_info* obj = 0;

		// find the lua_info object as that's what rcall() requires
		std::vector<lua_info*>::iterator itr = scriptList.begin();

		while (itr != scriptList.end())
		{
			if ((*itr)->luaVM == luaVM)
			{
				obj = *itr;
				break;
			}
			itr++;
		}
		return obj;
	}
	
	// Generic commands
	int l_registertimer(lua_State* luaVM)
	{
		bool bSuccess = false;
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "ls", args, true)) // overload for arguments to pass
		{
			DWORD delay = args[0].l;
			char* callback = args[1].s;
			
			// create the timer, we need to track id too incase scripts unload
			stimer_info* t = new stimer_info;
			t->callback = callback;
			t->luaVM = luaVM;
			t->count = 1;

			// build userdata list
			for (size_t x = 2; x < args.size(); x++)
				t->userdata.push_back(args[x]);

			DWORD timer_id = Timer::CreateTimer(delay, (LPARAM)t, script_process_timer);
			t->timer_id = timer_id;
			scriptTimers.push_back(t);

			// old versions of Phasor called a timer at its creation
			/*lua_info* lua_obj = FindLuaObject(luaVM);
			if (lua_obj)
			{
				rcall(lua_obj, t->callback.c_str(), "lll", t->timer_id, t->userdata, t->count);
				t->count++;
			}*/
			lua_pushnumber(luaVM, timer_id);
			bSuccess = true;
		}
		if (!bSuccess)
			lua_pushnil(luaVM);

		return 1; // 1 indicates there's a return value
	}

	int l_removetimer(lua_State* luaVM)
	{
		std::vector<luaArgs> args;
		if (GetLuaArguments(luaVM, "l", args)) // overload for arguments to pass
		{
			DWORD id = args[0].l;

			// remove from our list and free memory
			std::vector<stimer_info*>::iterator itr = scriptTimers.begin();
			while(itr != scriptTimers.end())
			{
				if ((*itr)->timer_id == id)
				{
					Timer::RemoveTimer(id);
					delete (*itr);					
					itr = scriptTimers.erase(itr);					
					break;
				}
				else
					itr++;
			}	
		}
		
		lua_pushnumber(luaVM, 0);
		return 0; // 1 indicates there's a return value
	}

	// callback for script timers
	bool script_process_timer(LPARAM data)
	{
		stimer_info* t = (stimer_info*)data;

		lua_info* lua_obj = FindLuaObject(t->luaVM);

		int keep = 0;

		if (lua_obj)
		{
			// i hate how i have to do this but cbf rewriting how i call 
			// functions
			std::vector<luaArgs> args; luaArgs arg;
			arg.l = t->timer_id;
			args.push_back(arg); // arg1
			arg.l = t->count;
			args.push_back(arg); // arg2
				
			// the callbacks are expected to have same number of userdatas as was created with
			// so we need to build a format specifier 
			std::string fmt = "ll"; // id, count
			for (size_t x = 0; x < t->userdata.size(); x++)
			{
				args.push_back(t->userdata[x]); // add the arg
				fmt += t->userdata[x].active;				
			}
			keep = rcall(lua_obj, t->callback.c_str(), fmt.c_str(), args);
			t->count++;
		}

		return (keep == 1) ? true:false;
	}

	// frees all timers associated with a script
	void CleanupTimers(lua_State* lua_VM)
	{
		// remove from our list and free memory
		std::vector<stimer_info*>::iterator itr = scriptTimers.begin();
		while (itr != scriptTimers.end())
		{
			if ((*itr)->luaVM == lua_VM)
			{
				Timer::RemoveTimer((*itr)->timer_id);
				delete (*itr);
				itr = scriptTimers.erase(itr);
			}
			else
				itr++;
		}
	}
}