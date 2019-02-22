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

#ifdef PHASOR_PC
	#define CONST_MENTRY_SIZE				0x0A4 // size of each entry in map cycle
	#define GAMET_BUFFER_SIZE				0x098	
#elif PHASOR_CE
	#define CONST_MENTRY_SIZE				0x0E4
	#define GAMET_BUFFER_SIZE				0x0D8	
#endif

// Offsets
#define OFFSET_CONSOLETEXT					0x0B4
#define OFFSET_RESPAWNTICKS					0x068
#ifdef PHASOR_PC
	#define OFFSET_MAXPLAYERS				0x1A5
	#define OFFSET_PLAYERTABLE				0x1AA
	#define OFFSET_HASHBASE					0x3C4
	#define OFFSET_HASHRESOLVE				0x3B8
	#define OFFSET_HASHLOOKUPLEN			0x060
#elif PHASOR_CE
	#define OFFSET_MAXPLAYERS				0x1E5
	#define OFFSET_PLAYERTABLE				0x1EA
	#define OFFSET_HASHBASE			0x404
	#define OFFSET_HASHRESOLVE			0x3F8
	#define OFFSET_HASHLOOKUPLEN			0x0EC
#endif

// This file is used to store all memory addresses Phasor uses
// ------------------------------------------------------------------------
// 
// Memory addresses
//
extern unsigned long ADDR_CONSOLEINFO;
extern unsigned long ADDR_RCONPLAYER;
extern unsigned long ADDR_TAGTABLE;
extern unsigned long ADDR_PLAYERINFOBASE;
extern unsigned long ADDR_OBJECTBASE;
extern unsigned long ADDR_PLAYERBASE;
extern unsigned long ADDR_MAPCYCLEINDEX;
extern unsigned long ADDR_CURRENTMAPDATA;
extern unsigned long ADDR_CURRENTMAPDATACOUNT;
extern unsigned long ADDR_NEWGAMEMAP;
extern unsigned long ADDR_CURMAPCOUNT;
extern unsigned long ADDR_MAXMAPCOUNT;
extern unsigned long ADDR_SOCKETREADY;
extern unsigned long ADDR_GAMEREADY;
extern unsigned long ADDR_PREPAREGAME_FLAG; //updated (should work)
extern unsigned long ADDR_CMDCACHE;
extern unsigned long ADDR_CMDCACHE_INDEX;
extern unsigned long ADDR_CMDCACHE_CUR;
extern unsigned long ADDR_GAMETYPE;
extern unsigned long ADDR_PORT;
extern unsigned long ADDR_SERVERNAME;
extern unsigned long ADDR_CONSOLEREADY;

// ------------------------------------------------------------------------
//
// Function addresses
//
extern unsigned long FUNC_HALOGETHASH;
extern unsigned long FUNC_EXECUTESVCMD; // fine.. i think
extern unsigned long FUNC_ONPLAYERDEATH;
extern unsigned long FUNC_ACTIONDEATH_1;
extern unsigned long FUNC_ACTIONDEATH_2;
extern unsigned long FUNC_ACTIONDEATH_3;
extern unsigned long FUNC_DOINVIS;
extern unsigned long FUNC_PLAYERJOINING; // fine.. i think
extern unsigned long FUNC_TEAMSELECT; // fine.. i think
extern unsigned long FUNC_GETMAPPATH;
extern unsigned long FUNC_VALIDATEGAMETYPE;
extern unsigned long FUNC_BUILDPACKET;
extern unsigned long FUNC_ADDPACKETTOQUEUE;
extern unsigned long FUNC_ADDPACKETTOPQUEUE;
extern unsigned long FUNC_AFTERSPAWNROUTINE; // fine.. i think
extern unsigned long FUNC_EXECUTEGAME;
extern unsigned long FUNC_PREPAREGAME_ONE;
extern unsigned long FUNC_PREPAREGAME_TWO;
extern unsigned long FUNC_BANPLAYER;
extern unsigned long FUNC_SAVEBANLIST;
extern unsigned long FUNC_UPDATEAMMO;

// ------------------------------------------------------------------------
//
// Codecave addresses
//
extern unsigned long CC_PHASORLOAD;
extern unsigned long CC_CONSOLEPROC;
extern unsigned long CC_CONSOLEHANDLER;
extern unsigned long CC_SERVERCMD;
extern unsigned long CC_GAMEEND;
extern unsigned long CC_PLAYERWELCOME;
extern unsigned long CC_CHAT;
extern unsigned long CC_MAPLOADING;
extern unsigned long CC_TEAMSELECTION;
extern unsigned long CC_NEWGAME;
extern unsigned long CC_PLAYERQUIT;
extern unsigned long CC_TEAMCHANGE;
extern unsigned long CC_DEATH;
extern unsigned long CC_KILLMULTIPLIER;
extern unsigned long CC_OBJECTINTERACTION;
extern unsigned long CC_PLAYERSPAWN;
extern unsigned long CC_PLAYERSPAWNEND;
extern unsigned long CC_VEHICLEENTRY;
extern unsigned long CC_WEAPONRELOAD;
extern unsigned long CC_DAMAGELOOKUP;
extern unsigned long CC_WEAPONASSIGN;
//extern unsigned long CC_WEAPONCREATION;
//extern unsigned long CC_WEAPONCREATION;
extern unsigned long CC_OBJECTCREATION; // all objects??
extern unsigned long CC_MAPCYCLEADD;
extern unsigned long CC_CLIENTUPDATE;
extern unsigned long CC_EXCEPTION_HANDLER;

// ------------------------------------------------------------------------
//
// Patch addresses
//
extern unsigned long PATCH_ALLOCATEMAPNAME;
extern unsigned long PATCH_MAPTABLEALLOCATION;
extern unsigned long PATCH_MAPTABLE;
extern unsigned long PATCH_MAPLOADING;
extern unsigned long PATCH_NOMAPPROCESS;
extern unsigned long PATCH_TEAMSLECTION;

// TO FIND SIGNATURES FOR
extern unsigned long FUNC_CREATEOBJECTQUERY;
extern unsigned long FUNC_CREATEOBJECT;
extern unsigned long CC_OBJECTRESPAWN;
extern unsigned long ADDR_SERVERINFO;
extern unsigned long CC_EQUIPMENTDESTROY;
extern unsigned long FUNC_DESTROYOBJECT;
extern unsigned long FUNC_PLAYERASSIGNWEAPON;
extern unsigned long FUNC_NOTIFY_WEAPONPICKUP;
extern unsigned long FUNC_ENTERVEHICLE;
extern unsigned long FUNC_EJECTVEHICLE;
extern unsigned long CC_VEHICLEFORCEEJECT;
extern unsigned long CC_VEHICLEUSEREJECT;

namespace addrLocations
{
	// Called to find all the above addresses
	bool LocateAddresses();
}