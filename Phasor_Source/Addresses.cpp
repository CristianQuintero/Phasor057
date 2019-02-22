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

#include "Common.h"
#include "Logging.h"
#include "Halo.h"

// This file is used to store all memory addresses Phasor uses
// ------------------------------------------------------------------------
// 
// Memory addresses
//
extern unsigned long ADDR_CONSOLEINFO = 0x0063BC1C;
extern unsigned long ADDR_RCONPLAYER = ADDR_CONSOLEINFO + 0x10;
extern unsigned long ADDR_TAGTABLE = 0x00460678;
extern unsigned long ADDR_PLAYERINFOBASE = 0x0069B91C;
extern unsigned long ADDR_OBJECTBASE = 0x00744C18;
extern unsigned long ADDR_PLAYERBASE = 0x0075ECE4;
extern unsigned long ADDR_MAPCYCLEINDEX = 0x00614B58;
extern unsigned long ADDR_CURRENTMAPDATA = 0x00614B4C;
extern unsigned long ADDR_CURRENTMAPDATACOUNT = 0x00614B50;
extern unsigned long ADDR_NEWGAMEMAP = 0x006713D8;
extern unsigned long ADDR_CURMAPCOUNT = 0x00692480;
extern unsigned long ADDR_MAXMAPCOUNT = 0x00692484;
extern unsigned long ADDR_SOCKETREADY = 0x0069B91C;
extern unsigned long ADDR_GAMEREADY = 0x00698DC8;
extern unsigned long ADDR_PREPAREGAME_FLAG = 0x00694528;
extern unsigned long ADDR_CMDCACHE = 0x0063FED4;
extern unsigned long ADDR_CMDCACHE_INDEX = 0x006406CE;
extern unsigned long ADDR_CMDCACHE_CUR = 0x006406D0;
extern unsigned long ADDR_GAMETYPE = 0x00671340;
extern unsigned long ADDR_PORT = 0x00625230;
extern unsigned long ADDR_SERVERNAME = 0x006265B0;
extern unsigned long ADDR_CONSOLEREADY = 0x0063BC28;

// ------------------------------------------------------------------------
//
// Function addresses
//
extern unsigned long FUNC_HALOGETHASH = 0x0059BBB0;
extern unsigned long FUNC_EXECUTESVCMD = 0x004EB7E0;
extern unsigned long FUNC_ONPLAYERDEATH = 0x00490050;
extern unsigned long FUNC_ACTIONDEATH_1 = 0x00524410;
extern unsigned long FUNC_ACTIONDEATH_2 = 0x0057D510;
extern unsigned long FUNC_ACTIONDEATH_3 = 0x00495A10;
extern unsigned long FUNC_DOINVIS = 0x0049AAA0;
extern unsigned long FUNC_PLAYERJOINING = 0x00517290;
extern unsigned long FUNC_TEAMSELECT = 0x00490940;
extern unsigned long FUNC_GETMAPPATH = 0x0045FC20;
extern unsigned long FUNC_VALIDATEGAMETYPE = 0x00481830;
extern unsigned long FUNC_BUILDPACKET = 0x00522E30;
extern unsigned long FUNC_ADDPACKETTOQUEUE = 0x00516610;
extern unsigned long FUNC_ADDPACKETTOPQUEUE = 0x00516460;
extern unsigned long FUNC_AFTERSPAWNROUTINE = 0x00498920;
extern unsigned long FUNC_EXECUTEGAME = 0x0047F0E0;
extern unsigned long FUNC_PREPAREGAME_ONE = 0x004ED240;
extern unsigned long FUNC_PREPAREGAME_TWO = 0x005193F0;
extern unsigned long FUNC_BANPLAYER = 0x00518890;
extern unsigned long FUNC_SAVEBANLIST = 0x00518270;
extern unsigned long FUNC_UPDATEAMMO = 0x004E83E0;

// ------------------------------------------------------------------------
//
// Codecave addresses
//
extern unsigned long CC_PHASORLOAD = 0x00595A12;
extern unsigned long CC_CONSOLEPROC = 0x004EB325;
extern unsigned long CC_CONSOLEHANDLER = 0x004B9FF0;
extern unsigned long CC_SERVERCMD = FUNC_EXECUTESVCMD;
extern unsigned long CC_GAMEEND = 0x00486B80;
extern unsigned long CC_PLAYERWELCOME = 0x0051692B;
extern unsigned long CC_CHAT = 0x004CEBC7;
extern unsigned long CC_MAPLOADING = 0x00483376;
extern unsigned long CC_TEAMSELECTION = 0x00513BB4;
extern unsigned long CC_NEWGAME = 0x0047B3B0;
extern unsigned long CC_PLAYERQUIT = 0x00494780;
extern unsigned long CC_TEAMCHANGE = 0x00490AD2;
extern unsigned long CC_DEATH = 0x00480168;
extern unsigned long CC_KILLMULTIPLIER = 0x004800D9;
extern unsigned long CC_OBJECTINTERACTION = 0x0049977B;
extern unsigned long CC_PLAYERSPAWN = 0x0049909A;
extern unsigned long CC_PLAYERSPAWNEND = 0x004990DF;
extern unsigned long CC_VEHICLEENTRY = 0x0049A395;
extern unsigned long CC_WEAPONRELOAD = 0x004E8303;
extern unsigned long CC_DAMAGELOOKUP = 0x00525081;
extern unsigned long CC_WEAPONASSIGN = 0x005827AC;
//extern unsigned long CC_WEAPONCREATION = 0x004E5F3F;
//extern unsigned long CC_WEAPONCREATION = 0x0052CA29;
extern unsigned long CC_OBJECTCREATION = 0x0052CA1B; // all objects??
extern unsigned long CC_MAPCYCLEADD = 0x005191CE;
extern unsigned long CC_CLIENTUPDATE = 0x00578D6D;
extern unsigned long CC_EXCEPTION_HANDLER = 0x005B036C;

// ------------------------------------------------------------------------
//
// Patch addresses
//
extern unsigned long PATCH_ALLOCATEMAPNAME = 0x005191C4;
extern unsigned long PATCH_MAPTABLEALLOCATION = 0x004B82D1;
extern unsigned long PATCH_MAPTABLE = 0x0069247C;
extern unsigned long PATCH_MAPLOADING = 0x0047EF22;
extern unsigned long PATCH_NOMAPPROCESS = 0x00483280;
extern unsigned long PATCH_TEAMSLECTION = 0x00513BAE;

// TO FIND SIGNATURES FOR
extern unsigned long FUNC_CREATEOBJECTQUERY = 0x0052C4F0;
extern unsigned long FUNC_CREATEOBJECT = 0x0052C600;
extern unsigned long CC_OBJECTRESPAWN = 0x00586D81;
extern unsigned long ADDR_SERVERINFO = 0x00671420;
extern unsigned long CC_EQUIPMENTDESTROY = 0x0047E61C;
extern unsigned long FUNC_DESTROYOBJECT = 0x0052CD20;
extern unsigned long FUNC_PLAYERASSIGNWEAPON = 0x00582C60;
extern unsigned long FUNC_NOTIFY_WEAPONPICKUP = 0x00499EF0;
extern unsigned long FUNC_ENTERVEHICLE = 0x0049A2A0;
extern unsigned long FUNC_EJECTVEHICLE = 0x00580B00;
extern unsigned long CC_VEHICLEFORCEEJECT = 0x0056E6CD;
extern unsigned long CC_VEHICLEUSEREJECT = 0x0056E107;

namespace addrLocations
{
	// ------------------------------------------------------------------------
	// 
	// Locates an address given a signature/offset
	// Note: If the address is not found, an exception is returned.
	DWORD FindAddress(const char* desc, LPBYTE data, DWORD size, LPBYTE sig, DWORD sig_len, DWORD occ, int offset)
	{
		DWORD dwAddress = 0;

		// find the address
		std::vector<DWORD> results = FindSignature(sig, NULL, sig_len, data, size);

		if (results.size() && results.size()-1 >= occ)
			dwAddress = (DWORD)data + results[occ] + offset;
		else
		{
			std::string err = m_sprintf_s("FindAddress - Signature not found for local function %s", desc);
			throw std::exception(err.c_str());
		}

		//halo::hprintf("extern unsigned long %s = 0x%08X;", desc, dwAddress);

		return dwAddress;
	}

	// Locates an address given a signature/offset
	// Note: If the address is not found, an exception is returned.
	DWORD FindAddressWC(const char* desc, LPBYTE data, DWORD size, LPBYTE sig, LPBYTE wildcard, DWORD sig_len, DWORD occ, int offset)
	{
		DWORD dwAddress = 0;

		// find the address
		std::vector<DWORD> results = FindSignature(sig, wildcard, sig_len, data, size);

		if (results.size() && results.size()-1 >= occ)
			dwAddress = (DWORD)data + results[occ] + offset;
		else
		{
			std::string err = m_sprintf_s("FindAddress - Signature not found for local function %s", desc);
			throw std::exception(err.c_str());
		}

		//halo::hprintf("extern unsigned long %s = 0x%08X;", desc, dwAddress);
		return dwAddress;
	}

	// Locates an address given a signature/offset and reads 4 bytes of data
	// Note: If the address is not found, an exception is returned.
	DWORD FindPtrAddress(const char* desc, LPBYTE data, DWORD size, LPBYTE sig, DWORD sig_len, DWORD occ, int offset)
	{
		DWORD dwAddress = 0;

		// find the address
		std::vector<DWORD> results = FindSignature(sig, NULL, sig_len, data, size);

		if (results.size() && results.size()-1 >= occ)
		{
			dwAddress = (DWORD)data + results[occ] + offset;
			ReadBytes(dwAddress, &dwAddress, 4);
		}
		else
		{
			std::string err = m_sprintf_s("FindAddress - Signature not found for local function %s", desc);
			throw std::exception(err.c_str());
		}

		//halo::hprintf("extern unsigned long %s = 0x%08X;", desc, dwAddress);
		return dwAddress;
	}

	// Called to find all the above addresses
	bool LocateAddresses()
	{
		try
		{
			// find the server's code section
			LPBYTE module = (LPBYTE)GetModuleHandle(0);
			DWORD OffsetToPE = *(DWORD*)(module + 0x3C);
			DWORD codeSize = *(DWORD*)(module + OffsetToPE + 0x1C);
			DWORD BaseOfCode = *(DWORD*)(module + OffsetToPE + 0x2C);
			LPBYTE codeSection = (LPBYTE)(module + BaseOfCode);

			// attempt to locate all addresses, if there are any errors an exception
			// will be thrown
			// ----------------------------------------------------------------
			BYTE sig1[] = {0xB1, 0x01, 0x83, 0xC4, 0x08, 0x8B, 0xF0};
			ADDR_CONSOLEINFO = FindPtrAddress("ADDR_CONSOLEINFO", codeSection, codeSize, sig1, sizeof(sig1), 0, 0x22);
			ADDR_RCONPLAYER = ADDR_CONSOLEINFO + 0x10;

			BYTE sig2[] = {0x51, 0x68, 0x00, 0x00, 0x00, 0x40};
			ADDR_TAGTABLE = FindPtrAddress("ADDR_TAGTABLE", codeSection, codeSize, sig2, sizeof(sig2), 0, -76);

			BYTE sig3[] = {0x05, 0x14, 0x0B, 0x00, 0x00};
			ADDR_PLAYERINFOBASE = FindPtrAddress("ADDR_PLAYERINFOBASE", codeSection, codeSize, sig3, sizeof(sig3), 0, 8);

			BYTE sig4[] = {0xDD, 0xD8, 0xF6, 0xC4, 0x41};
			ADDR_OBJECTBASE = FindPtrAddress("ADDR_OBJECTBASE", codeSection, codeSize, sig4, sizeof(sig4), 0, 0x20);

			BYTE sig5[] = {0x35, 0x72, 0x65, 0x74, 0x69};
			ADDR_PLAYERBASE = FindPtrAddress("ADDR_PLAYERBASE", codeSection, codeSize, sig5, sizeof(sig5), 0, -8);

			BYTE sig6[] = {0x57, 0x33, 0xFF, 0x3B, 0xC7};
			ADDR_MAPCYCLEINDEX = FindPtrAddress("ADDR_MAPCYCLEINDEX", codeSection, codeSize, sig6, sizeof(sig6), 0, 0x60);
			ADDR_CURRENTMAPDATA = FindPtrAddress("ADDR_CURRENTMAPDATA", codeSection, codeSize, sig6, sizeof(sig6), 0, 0x1C);
			ADDR_CURRENTMAPDATACOUNT = FindPtrAddress("ADDR_CURRENTMAPDATACOUNT", codeSection, codeSize, sig6, sizeof(sig6), 0, 9);

			//BYTE sig7[] = {0x8B, 0x0C, 0xF0, 0x8D, 0x3C, 0xF0};
			//ADDR_MAPCYCLECOUNT = FindPtrAddress("ADDR_MAPCYCLECOUNT", codeSection, codeSize, sig7, sizeof(sig7), 0, -24);
			//ADDR_MAPCYCLEDATA = FindPtrAddress("ADDR_MAPCYCLECOUNT", codeSection, codeSize, sig7, sizeof(sig7), 0, -4);

			BYTE sig8[] = {0x90, 0x8B, 0xC3, 0x25, 0xFF, 0xFF, 0x00, 0x00};
			ADDR_NEWGAMEMAP = FindPtrAddress("ADDR_NEWGAMEMAP", codeSection, codeSize, sig8, sizeof(sig8), 0, 0x25);

			BYTE sig9[] = {0x83, 0xC8, 0xFF, 0x5E, 0x81, 0xC4, 0x04, 0x01, 0x00, 0x00};
			ADDR_CURMAPCOUNT = FindPtrAddress("ADDR_CURMAPCOUNT", codeSection, codeSize, sig9, sizeof(sig9), 0, -88);
			ADDR_MAXMAPCOUNT = ADDR_CURMAPCOUNT + 4;

			BYTE sig10[] = {0xB8, 0xD3, 0x4D, 0x62, 0x10};
			ADDR_SOCKETREADY = FindPtrAddress("ADDR_SOCKETREADY", codeSection, codeSize, sig10, sizeof(sig10), 0, 0xA1);

			BYTE sig11[] = {0x8A, 0x83, 0x8A, 0x02, 0x00, 0x00, 0x84, 0xC0};
			ADDR_GAMEREADY = FindPtrAddress("ADDR_GAMEREADY", codeSection, codeSize, sig11, sizeof(sig11), 0, 0x4C);

			BYTE sig12[] = {0x56, 0x57, 0xB3, 0x01};
			ADDR_PREPAREGAME_FLAG = FindPtrAddress("ADDR_PREPAREGAME_FLAG", codeSection, codeSize, sig12, sizeof(sig12), 0, 0x1F);

			BYTE sig13[] = {0x8A, 0x07, 0x81, 0xEC, 0x00, 0x05, 0x00, 0x00};
			ADDR_CMDCACHE = FindPtrAddress("ADDR_CMDCACHE", codeSection, codeSize, sig13, sizeof(sig13), 0, 0x78);
			ADDR_CMDCACHE_INDEX = FindPtrAddress("ADDR_CMDCACHE_INDEX", codeSection, codeSize, sig13, sizeof(sig13), 0, 0x55);
			#ifdef PHASOR_PC
			ADDR_CMDCACHE_CUR = FindPtrAddress("ADDR_CMDCACHE_CUR", codeSection, codeSize, sig13, sizeof(sig13), 0, 0xB1);
			#elif PHASOR_CE
			ADDR_CMDCACHE_CUR = FindPtrAddress("ADDR_CMDCACHE_CUR", codeSection, codeSize, sig13, sizeof(sig13), 0, 0xB2);
			#endif
			
			FUNC_EXECUTESVCMD = FindAddress("FUNC_EXECUTESVCMD", codeSection, codeSize, sig13, sizeof(sig13), 0, 0);
			CC_SERVERCMD = FUNC_EXECUTESVCMD;

			BYTE sig14[] = {0xC3, 0x68, 0x00, 0x01, 0x00, 0x00};
			ADDR_GAMETYPE = FindPtrAddress("ADDR_GAMETYPE", codeSection, codeSize, sig14, sizeof(sig14), 1, 0x0C);

			BYTE sig15[] = {0xC1, 0xE6, 0x10, 0x25, 0x00, 0xFF, 0x00, 0x00};
			ADDR_PORT = FindPtrAddress("ADDR_PORT", codeSection, codeSize, sig15, sizeof(sig15), 0, 0x26);

			BYTE sig16[] = {0x6A, 0x3F, 0x8D, 0x44, 0x24, 0x10, 0x50};
			ADDR_SERVERNAME = FindPtrAddress("ADDR_SERVERNAME", codeSection, codeSize, sig16, sizeof(sig16), 0, 8);

			BYTE sig17[] = {0x33, 0xC9, 0x83, 0xF8, 0xFF, 0x0F, 0x95, 0xC1};
			ADDR_CONSOLEREADY = FindPtrAddress("ADDR_CONSOLEREADY", codeSection, codeSize, sig17, sizeof(sig17), 0, 0xC9);

			BYTE sig18[] = {0x8B, 0x4C, 0x24, 0x0C, 0x8B, 0x10, 0x39, 0x0A};
			FUNC_HALOGETHASH = FindAddress("FUNC_HALOGETHASH", codeSection, codeSize, sig18, sizeof(sig18), 0, -60);

			BYTE sig19[] = {0x66, 0xFF, 0x80, 0xAE, 0x00, 0x00, 0x00};
#ifdef PHASOR_PC
			FUNC_ONPLAYERDEATH = FindAddress("FUNC_ONPLAYERDEATH", codeSection, codeSize, sig19, sizeof(sig19), 0, -87);
#elif PHASOR_CE
			FUNC_ONPLAYERDEATH = FindAddress("FUNC_ONPLAYERDEATH", codeSection, codeSize, sig19, sizeof(sig19), 0, -90);
#endif
			BYTE sig20[] = {0xC7, 0x81, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F};
			FUNC_ACTIONDEATH_1 = FindAddress("FUNC_ACTIONDEATH_1", codeSection, codeSize, sig20, sizeof(sig20), 0, 0x13);

			BYTE sig21[] = {0xC7, 0x42, 0x04, 0x03, 0x00, 0x00, 0x00, 0x5F};
			FUNC_ACTIONDEATH_2 = FindAddress("FUNC_ACTIONDEATH_2", codeSection, codeSize, sig21, sizeof(sig21), 0, 0x0F);

			BYTE sig22[] = {0x8B, 0x41, 0x34, 0x83, 0xEC, 0x10, 0x53, 0x55, 0x56};
			FUNC_ACTIONDEATH_3 = FindAddress("FUNC_ACTIONDEATH_3", codeSection, codeSize, sig22, sizeof(sig22), 0, -6);
			
			BYTE sig23[] = {0x83, 0xFB, 0xFF, 0x55, 0x8B, 0x6C, 0x24, 0x08};
			FUNC_DOINVIS = FindAddress("FUNC_DOINVIS", codeSection, codeSize, sig23, sizeof(sig23), 0, 0);

			BYTE sig24[] = {0x66, 0x8B, 0x46, 0x04, 0x83, 0xEC, 0x0C, 0x66, 0x85, 0xC0, 0x53};
			FUNC_PLAYERJOINING = FindAddress("FUNC_PLAYERJOINING", codeSection, codeSize, sig24, sizeof(sig24), 0, 0);

			BYTE sig25[] = {0x3B, 0xD0, 0x0F, 0x9F, 0xC0};
			FUNC_TEAMSELECT = FindAddress("FUNC_TEAMSELECT", codeSection, codeSize, sig25, sizeof(sig25), 0, -84);

			BYTE sig26[] = {0xC6, 0x00, 0x01, 0x5E};
			FUNC_GETMAPPATH = FindAddress("FUNC_GETMAPPATH", codeSection, codeSize, sig26, sizeof(sig26), 0, 0x0E);

			BYTE sig27[] =    {0x81, 0xEC, 0x60, 0x02, 0x00, 0x00, 0x53, 0x55, 0x8B, 0xAC, 0x24, 0x6C, 0x02, 0x00, 0x00, 0x56, 0x57, 0x8B, 0xD9};
			BYTE sig27_wc[] = {0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00};
			FUNC_VALIDATEGAMETYPE = FindAddressWC("FUNC_VALIDATEGAMETYPE", codeSection, codeSize, sig27, sig27_wc, sizeof(sig27), 0, 0);
			
			BYTE sig28[] = {0x81, 0xEC, 0xA4, 0x00, 0x00, 0x00, 0x53, 0x55};
			#ifdef PHASOR_PC
				FUNC_BUILDPACKET = FindAddress("FUNC_BUILDPACKET", codeSection, codeSize, sig28, sizeof(sig28), 0, 0);
			#elif PHASOR_CE
				FUNC_BUILDPACKET = FindAddress("FUNC_BUILDPACKET", codeSection, codeSize, sig28, sizeof(sig28), 1, 0);
			#endif

			BYTE sig29[] = {0x66, 0x8B, 0x46, 0x0E, 0x8A, 0xD0, 0xD0, 0xEA, 0xF6, 0xC2, 0x01};
			FUNC_ADDPACKETTOQUEUE = FindAddress("FUNC_ADDPACKETTOQUEUE", codeSection, codeSize, sig29, sizeof(sig29), 0, -56);

			BYTE sig30[] = {0x51, 0x53, 0x57, 0x8B, 0xF8, 0x32, 0xC0, 0x33, 0xC9};
			FUNC_ADDPACKETTOPQUEUE = FindAddress("FUNC_ADDPACKETTOPQUEUE", codeSection, codeSize, sig30, sizeof(sig30), 0, 0);

			BYTE sig31[] = {0x83, 0xEC, 0x2C, 0x53, 0x55};
			FUNC_AFTERSPAWNROUTINE = FindAddress("FUNC_AFTERSPAWNROUTINE", codeSection, codeSize, sig31, sizeof(sig31), 0, -9);

			// for this function we need to locate a call to FUNC_EXECUTEGAME and find the address from there
			BYTE sig32[] = {0x68, 0xCD, 0xCC, 0x4C, 0x3E};
#ifdef PHASOR_PC
			DWORD call_addr = FindAddress("FUNC_EXECUTEGAME", codeSection, codeSize, sig32, sizeof(sig32), 0, 0x9B);
#elif PHASOR_CE
			DWORD call_addr = FindAddress("FUNC_EXECUTEGAME", codeSection, codeSize, sig32, sizeof(sig32), 0, 0x99);
#endif
			DWORD call_offset = *(DWORD*)(call_addr + 1);
			FUNC_EXECUTEGAME = call_addr + 5 + call_offset;
			//halo::hprintf("extern unsigned long FUNC_EXECUTEGAME = 0x%08X", FUNC_EXECUTEGAME);
			
			BYTE sig33[] = {0x56, 0x68, 0xFF, 0x00, 0x00, 0x00, 0x57};
			FUNC_PREPAREGAME_ONE = FindAddress("FUNC_PREPAREGAME_ONE", codeSection, codeSize, sig33, sizeof(sig33), 0, 0);

			BYTE sig34[] = {0x83, 0xC4, 0x14, 0x8B, 0x44, 0x24, 04};
			FUNC_PREPAREGAME_TWO = FindAddress("FUNC_PREPAREGAME_TWO", codeSection, codeSize, sig34, sizeof(sig34), 0, -52);

			BYTE sig35[] = {0x53, 0x55, 0x8B, 0x6C, 0x24, 0x10, 0x57, 0x55};
			FUNC_BANPLAYER = FindAddress("FUNC_BANPLAYER", codeSection, codeSize, sig35, sizeof(sig35), 0, 0);

			BYTE sig36[] = {0x83, 0xEC, 0x6C, 0x53, 0x57};
			FUNC_SAVEBANLIST = FindAddress("FUNC_SAVEBANLIST", codeSection, codeSize, sig36, sizeof(sig36), 0, 0);

			BYTE sig37[] = {0x6A, 0x2D, 0x6A, 0x00, 0xBA, 0xF8, 0x7F, 0x00, 0x00};
			FUNC_UPDATEAMMO = FindAddress("FUNC_UPDATEAMMO", codeSection, codeSize, sig37, sizeof(sig37), 0, -117);

			
			BYTE sig38[] = {0x8D, 0xA4, 0x24, 0x00, 0x00, 0x00, 0x00, 0x8D, 0x64, 0x24, 0x00, 0x0F, 0xBF, 0xC5};
			CC_CONSOLEPROC = FindAddress("CC_CONSOLEPROC", codeSection, codeSize, sig38, sizeof(sig38), 0, -96);

			BYTE sig39[] =    {0xB8, 0x01, 0x00, 0x00, 0x00, 0xC6, 0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
			BYTE sig39_wc[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00};
			CC_CONSOLEHANDLER = FindAddressWC("CC_CONSOLEHANDLER", codeSection, codeSize, sig39, sig39_wc, sizeof(sig39), 0, 0);
			
			BYTE sig40[] = {0x83, 0xEC, 0x0C, 0x6A, 0x00, 0x6A, 0x01, 0x6A, 0x00};
			CC_GAMEEND = FindAddress("CC_GAMEEND", codeSection, codeSize, sig40, sizeof(sig40), 0, 0);

			BYTE sig41[] = {0x0F, 0xB6, 0x4C, 0x24, 0x10, 0x49, 0x83, 0xF9, 0x24};
			#ifdef PHASOR_PC
				CC_PLAYERWELCOME = FindAddress("CC_PLAYERWELCOME", codeSection, codeSize, sig41, sizeof(sig41), 0, 0xCF);
			#elif PHASOR_CE
				CC_PLAYERWELCOME = FindAddress("CC_PLAYERWELCOME", codeSection, codeSize, sig41, sizeof(sig41), 0, 0xC1);	
			#endif

			BYTE sig42[] = {0x57, 0x33, 0xFF, 0x3B, 0xD7};
			CC_CHAT = FindAddress("CC_CHAT", codeSection, codeSize, sig42, sizeof(sig42), 0, 0x2D);

			BYTE sig43[] = {0x8B, 0x0C, 0x06, 0x6A, 0x3F, 0x51};
			CC_MAPLOADING = FindAddress("CC_MAPLOADING", codeSection, codeSize, sig43, sizeof(sig43), 0, 0);

			BYTE sig44[] = {0x80, 0x7B, 0x1E, 0xFF, 0xC7, 0x44, 0x24, 0x08, 0x25, 0x00, 0x00, 0x00, 0xC7, 0x44, 0x24, 0x0C, 0x7C, 0x00, 0x00, 0x00};
			CC_TEAMSELECTION = FindAddress("CC_TEAMSELECTION", codeSection, codeSize, sig44, sizeof(sig44), 0, 0x1A);
			
			BYTE sig45[] = {0x32, 0xC9, 0x83, 0xF8, 0x13};
			CC_NEWGAME = FindAddress("CC_NEWGAME", codeSection, codeSize, sig45, sizeof(sig45), 0, -26);

			BYTE sig46[] = {0x5F, 0xC1, 0xE1, 0x04, 0x48, 0x5E};
			CC_PLAYERQUIT = FindAddress("CC_PLAYERQUIT", codeSection, codeSize, sig46, sizeof(sig46), 0, -130);

			BYTE sig47[] = {0x8A, 0x04, 0x24, 0x3C, 0x10, 0x53, 0x0F, 0xB6, 0x5C, 0x24, 0x05};
			#ifdef PHASOR_PC
				CC_TEAMCHANGE = FindAddress("CC_TEAMCHANGE", codeSection, codeSize, sig47, sizeof(sig47), 0, 0x6E);
			#elif PHASOR_CE
				CC_TEAMCHANGE = FindAddress("CC_TEAMCHANGE", codeSection, codeSize, sig47, sizeof(sig47), 0, 0x6F);
			#endif

			BYTE sig48[] = {0x55, 0x8B, 0x6C, 0x24, 0x20, 0x89, 0x44, 0x24, 0x04};
			CC_DEATH = FindAddress("CC_DEATH", codeSection, codeSize, sig48, sizeof(sig48), 0, 0);

			BYTE sig49[] = {0x83, 0xEC, 0x10, 0x55, 0x8B, 0x6C, 0x24, 0x1C};
			CC_KILLMULTIPLIER = FindAddress("CC_KILLMULTIPLIER", codeSection, codeSize, sig49, sizeof(sig49), 0, 0x19);

			BYTE sig50[] = {0x53, 0x8B, 0x5C, 0x24, 0x20, 0x55, 0x8B, 0x6C, 0x24, 0x20, 0x81, 0xE5, 0xFF, 0xFF, 0x00, 0x00, 0xC1, 0xE5, 0x09};
			CC_OBJECTINTERACTION = FindAddress("CC_OBJECTINTERACTION", codeSection, codeSize, sig50, sizeof(sig50), 0, 0);
			
			BYTE sig51[] = {0x8B, 0x54, 0x24, 0x14, 0x6A, 0xFF};
			CC_PLAYERSPAWN = FindAddress("CC_PLAYERSPAWN", codeSection, codeSize, sig51, sizeof(sig51), 0, -52);
			CC_PLAYERSPAWNEND = FindAddress("CC_PLAYERSPAWNEND", codeSection, codeSize, sig51, sizeof(sig51), 0, 0x11);

			BYTE sig52[] = {0x33, 0xD2, 0x8A, 0x56, 0x64, 0x33, 0xC0};
			CC_VEHICLEENTRY = FindAddress("CC_VEHICLEENTRY", codeSection, codeSize, sig52, sizeof(sig52), 0, 0x4D);

			BYTE sig53[] = {0x83, 0xCD, 0xFF, 0x80, 0xF9, 0x01};
			CC_WEAPONRELOAD = FindAddress("CC_WEAPONRELOAD", codeSection, codeSize, sig53, sizeof(sig53), 0, -4);

			BYTE sig54[] = {0x81, 0xC7, 0xC4, 0x01, 0x00, 0x00, 0x81, 0xC1, 0x5F, 0xF3, 0x6E, 0x3C};
			CC_DAMAGELOOKUP = FindAddress("CC_DAMAGELOOKUP", codeSection, codeSize, sig54, sizeof(sig54), 0, 0);

			BYTE sig55[] = {0x8D, 0x04, 0xC0, 0x8D, 0x04, 0x81, 0x8B, 0x40, 0x0C};
			CC_WEAPONASSIGN = FindAddress("CC_WEAPONASSIGN", codeSection, codeSize, sig55, sizeof(sig55), 0, 6);

			BYTE sig56[] = {0x8B, 0x48, 0x04, 0x85, 0xC9, 0x75, 0x3A};
			CC_OBJECTCREATION = FindAddress("CC_OBJECTCREATION", codeSection, codeSize, sig56, sizeof(sig56), 0, 0);

			BYTE sig57[] = {0x8B, 0xF0, 0x8B, 0xCF, 0x2B, 0xF7};
			CC_MAPCYCLEADD = FindAddress("CC_MAPCYCLEADD", codeSection, codeSize, sig57, sizeof(sig57), 0, 0);
			PATCH_ALLOCATEMAPNAME = FindAddress("PATCH_ALLOCATEMAPNAME", codeSection, codeSize, sig57, sizeof(sig57), 0, -10);

			BYTE sig58[] = {0x8A, 0x12, 0x88, 0x90, 0xA6, 0x02, 0x00, 0x00};
			CC_CLIENTUPDATE = FindAddress("CC_POSITIONUPDATE", codeSection, codeSize, sig58, sizeof(sig58), 0, 2);

			BYTE sig59[] = {0x30, 0x55, 0x8B, 0xEC};
			CC_EXCEPTION_HANDLER = FindAddress("CC_EXCEPTION_HANDLER", codeSection, codeSize, sig59, sizeof(sig59), 0, 1);
			
			#ifdef PHASOR_PC
				BYTE sig60[] = {0x81, 0xEC, 0x00, 0x08, 0x00, 0x00, 0x53, 0x55};
				PATCH_MAPTABLEALLOCATION = FindAddress("PATCH_MAPTABLEALLOCATION", codeSection, codeSize, sig60, sizeof(sig60), 0, 0x1B);

				BYTE sig61[] = {0x8D, 0x04, 0x40, 0x8A, 0x54, 0x81, 0x08, 0x84, 0xD2};
				PATCH_MAPTABLE = FindPtrAddress("PATCH_MAPTABLE", codeSection, codeSize, sig61, sizeof(sig61), 0, -4);

				BYTE sig62[] = {0x83, 0xE0, 0xBF};
				PATCH_MAPLOADING = FindAddress("PATCH_MAPLOADING", codeSection, codeSize, sig62, sizeof(sig62), 0, 0x3F);	
			#endif
			
			BYTE sig63[] = {0x53, 0x56, 0x57, 0xB3, 0x01};
			PATCH_NOMAPPROCESS = FindAddress("PATCH_NOMAPPROCESS", codeSection, codeSize, sig63, sizeof(sig63), 0, -12);

			BYTE sig64[] = {0xC7, 0x44, 0x24, 0x0C, 0x7C, 0x00, 0x00, 0x00};
			PATCH_TEAMSLECTION = FindAddress("PATCH_TEAMSLECTION", codeSection, codeSize, sig64, sizeof(sig64), 0, 8);

			//system("PAUSE");
		}
		catch (std::exception & e)
		{
			halo::hprintf("%s", e.what());
			logging::LogData(LOGGING_PHASOR, "%s", e.what());

			return false;
		}

		// success
		return true;
	}
}
