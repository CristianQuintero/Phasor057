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

#define _CRT_SECURE_NO_WARNINGS

#include "Maps.h"
#include "Addresses.h"
#include "Common.h"
#include "Logging.h"
#include "Lua.h"
#include "Server.h"
#include <map>
#include <algorithm>

/* The reasoning behind this whole system is actually quite complex. The basic
 * idea is as follows:
 * 1.
 * Reroute the table Halo uses for storing map info to memory controlled my Phasor
 * This allows me to add new maps and have the server accept them, ie ones
 * without original names. To do this the counts need to be tracked and modified
 * too (curCount, maxCount).
 * 
 * 2.
 * All map commands are hooked, specifically sv_mapcycle_add and sv_map.
 * Phasor implements all of the logic and expands the map info struct so that
 * it has room to accomodate scripts. When a map is being loaded the scripts
 * for this entry are saved so they can be loaded later (game::OnNew).
 * Phasor needs to manage the mapcycle list. When a game is being started the
 * data needs to be transferred into the memory that the server uses. 
 * 
 * 3.
 * When a map is being loaded checks need to be made. If the map being loaded
 * is to be changed (for example, map voting) the entire map data needs to be
 * rebuilt. 
 * If the map being loaded is one that isn't default (ie bloodgulch1) then
 * a bit of swapping needs to be done regarding the name of the map to load.
 * If the data is changed the codecave is notified so that the data Halo
 * processes is reloaded. 
 * The map name is later overwritten (once loaded) so it can be determined
 * exactly what map is loaded. 
 * 
 * I coded all this mid 2010, when I originally made Phasor. It
 * took awhile to get it working and I don't want to go through and rewrite
 * and re-reverse it. Unfortunately I didn't comment it very well when I made
 * it so I decided to add the information above to help you make sense of it.
*/
namespace game
{
	namespace maps
	{
		// ----------------------------------------------------------------
		// Structues used for map processing
		// ----------------------------------------------------------------
		#pragma pack(push, 1)

		// Structure of map header
		struct hMapHeader
		{
			DWORD integrity; // should be 'head' if not corrupt
			DWORD type; // 7 for halo PC, ce is 0x261
			DWORD filesize;
			DWORD highSize;
			DWORD offset1; // offset to data that is read, not sure what this data is yet.
			BYTE unk1[12]; // Unknown atm
			char name[32]; // map name
			char version[32]; // version the map was built at
			DWORD mapType; // 0 campaign, 1 multiplayer, 2 data (ui.map)
			BYTE unk2[0x798]; // first few bytes have data, rest are 00
			DWORD integrity2; // should be 'foot' if not corrupt
		};

		struct mapTableInfo
		{
			char* map;
			DWORD index;
			DWORD empty;
		};

		#pragma pack(pop)

		// ----------------------------------------------------------------
		// MAP CODE, THIS INCLUDES NON-DEFAULT MAP LOADING AND VARIOUS MAP
		// VALIDATION/INFORMATION FUNCTIONS
		// ----------------------------------------------------------------	

		LPDWORD curCount = 0, maxCount = 0;

		// Map of file names (bloodgulch2 = key) and their corresponding original maps
		std::map<std::string, std::string> fileMap;
		std::string map_being_loaded, mapDirectory;

		// Halo expects a char buffer, not std::string
		char map_being_loaded_buffer[64] = {0};

		// Used for rerouting the map table
		mapTableInfo* mapTable = 0;

		// Counts used in map table
		int modCount = 0;
		
		// Stores the address of the cyclic map name, requires fixing in FixGetMap
		// 'Fixing' is just replacing the name with the actual loaded name (ie bloodgulch1)
		char* m_fixMap = 0;

		// Returns the address of the loading buffer Halo should use
		char* GetLoadingMapBuffer()
		{
			return map_being_loaded_buffer;
		}

		// This function returns the address of our map table
		DWORD GetMapTable()
		{
			return (DWORD)mapTable;
		}

		// Called to fix the loaded map name (call when game begins)
		void FixMapName()
		{
			// Fix the map name
			strcpy_s(m_fixMap, sizeof(map_being_loaded_buffer), map_being_loaded_buffer);
		}

		// Updates the data in 'map' to the maps base data
		void UpdateLoadingMap(char* map)
		{
			// Save the map name ptr for fixing
			m_fixMap = map;

			// Make the map name lower case
			ConvertToLower(map);

			// Copy the actual map into the map we should load
			map_being_loaded = map;
			strcpy_s(map_being_loaded_buffer, sizeof(map_being_loaded_buffer), map);

			// Change the passed parameter to the unmodded name to avoid loading issues
			char* baseMap = (char*)fileMap[map].c_str();
			strcpy_s(map, sizeof(map_being_loaded_buffer), baseMap);
		}

		// This function checks if a map exists
		bool ValidateMap(char* map)
		{
			return fileMap.find(map) != fileMap.end();
		}

		// Sets the directory where maps are located from
		void SetMapPath(std::string path)
		{
			mapDirectory = path;
		}

		// This function a maps header and returns the base map (ie bloodgulch)
		std::string GetMapName(const char* szPath)
		{
			std::string output;

			// Open the map
			HANDLE hFile = CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

			bool bError = true;
			if (hFile != INVALID_HANDLE_VALUE)
			{
				// Read the header
				BYTE header[2048] = {0};

				DWORD read = 0;
				if (ReadFile(hFile, (char*)header, sizeof(header), &read, NULL))
				{
					hMapHeader* MapHeader = (hMapHeader*)header;

					// Make sure the map is a valid, halo pc, multiplayer map
					if (MapHeader->integrity == 'head' && MapHeader->type  == 7\
						&& MapHeader->mapType == 1)
					{
						// Copy the map name over
						output = MapHeader->name;
					}
					bError = false;
				}

				CloseHandle(hFile);
			}
			
			// Was the an error accessing and reading the file?
			if (bError)
			{
				std::string err = GetLastErrorAsText();

				// Log the error
				logging::LogData(LOGGING_PHASOR, "Cannot validate map %s, reason: %s", szPath, err.c_str());

				halo::hprintf("ERROR: cannot open map %s", szPath);
			}

			return output;
		}

		// This function generates the map list
		void BuildMapList()
		{
			// Allocate the memory (Halo expects it to be done w/ GlobalAlloc)
			int table_count = 200;
			mapTable = (mapTableInfo*)GlobalAlloc(GMEM_ZEROINIT, sizeof(mapTableInfo)*table_count);

			// Resolves the directory of the halo maps folder and appends the input to it
			typedef void (__cdecl *hResolveMapPath)(char* map, char* output);
			hResolveMapPath GetMapPath = (hResolveMapPath)(FUNC_GETMAPPATH);

			curCount = (LPDWORD)UlongToPtr(ADDR_CURMAPCOUNT);
			maxCount = (LPDWORD)UlongToPtr(ADDR_MAXMAPCOUNT);

			char szMapDir[1024] = {0};

			if (mapDirectory.size()) // if manually set
				sprintf_s(szMapDir, sizeof(szMapDir), "%s\\.map", mapDirectory.c_str());
			else
				GetMapPath("", szMapDir);	

			int len = strlen(szMapDir);

			if (len >= 4)
			{
				// Fix the string for the FindFirstFileA query
				szMapDir[len - 4] = '*';

				logging::LogData(LOGGING_PHASOR, "Building map list from: %s", szMapDir);

				WIN32_FIND_DATAA data;
				HANDLE hFind = FindFirstFileA(szMapDir, &data);

				int count = 0;

				// Loop through all files
				do
				{
					count++;

					// Originally only 200 slots are allocated
					if (count > 200)
					{
						table_count += 100;

						mapTable = (mapTableInfo*)GlobalReAlloc(mapTable,\
							sizeof(mapTableInfo)*table_count, GMEM_ZEROINIT);
					}

					// Make sure it's a file
					if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
						int maplen = strlen(data.cFileName);

						if (maplen < 0x20) // halo imposed limit
						{
							// Build the path to the map
							szMapDir[len - 4] = 0;
							strcat_s(szMapDir, sizeof(szMapDir), data.cFileName);

							std::string base_map = GetMapName(szMapDir);
							if (base_map.size())
							{
								// Make the map name lower case
								ConvertToLower(data.cFileName);

								// Terminate the string before .map
								maplen -= 4;
								data.cFileName[maplen] = 0;
								char* actual_map = data.cFileName;

								// Store the actual map and orig map names
								fileMap[data.cFileName] = base_map;

								// Default map, let halo add it.
								if (base_map == actual_map)
									continue;

								// Add the data into the map table
								char * map_alloc = (char*)GlobalAlloc(GMEM_FIXED, maplen + 1);
								strcpy_s(map_alloc, maplen+1, actual_map);
								mapTable[*curCount].index = *curCount;
								mapTable[*curCount].map = map_alloc;
								mapTable[*curCount].empty = 1;

								// Increment counters
								*curCount += 1; 
								*maxCount += 1;
								modCount++;
							}
						}
						else
						{
							// Log the error
							logging::LogData(LOGGING_PHASOR, "The map \"%s\" is too long and cannot be loaded.", data.cFileName);

							halo::hprintf("ERROR : The map name %s is too long.", data.cFileName);
						}
					}
				}
				while (FindNextFileA(hFind, &data) != 0);

				FindClose(hFind);
			}
		}
	};
};