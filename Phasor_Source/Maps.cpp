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
 * exactly what map is loaded. k   
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
		std::string current_map; // name of base map

		// Returns the (base) name of the loaded map
		const char* GetCurrentMap()
		{
			return current_map.c_str();
		}

		// Called when a map is being loaded
		bool LoadingAddressPatched = false;
		bool OnMapLoad(LPBYTE mapData)
		{
			bool bMapUnchanged = true;

			int curmap = *(DWORD*)ADDR_MAPCYCLEINDEX;

			char* map = (char*)*(DWORD*)mapData;
			char* gametype = (char*)*(DWORD*)(mapData + 4);
			LPBYTE m_scripts = (LPBYTE)*(DWORD*)(mapData + 8);		

			// Notify the map vote system
			vote::cmap newMap;
			if (vote::OnEnd(newMap))
			{
				try
				{
					// Make sure everything is valid with the vote
					BYTE buff[GAMET_BUFFER_SIZE] = {0};
					if (!GetGametypeData((char*)newMap.gametype.c_str(), buff, sizeof(buff)))
					{
						std::string error = m_sprintf_s("The voted gametype %s is not valid. Vote ignored.", newMap.gametype.c_str());
						throw std::exception(error.c_str());
					}
					else if (!ValidateMap((char*)newMap.map.c_str()))
					{
						std::string error = m_sprintf_s("The voted map %s is not valid. Vote ignored.", newMap.gametype.c_str());
						throw std::exception(error.c_str());
					}
					
					// Free the memory
					GlobalFree(map);				

					// Reallocate the memory
					map = (char*)GlobalAlloc(GMEM_FIXED, newMap.map.size() + 1);
					strcpy(map, newMap.map.c_str());

					// Update map
					*(DWORD*)(mapData) = PtrToUlong(map);				

					// Update gametype
					char* m_gametype = (char*)GlobalAlloc(GMEM_FIXED, newMap.gametype.size() + 1);
					strcpy(m_gametype, newMap.gametype.c_str());
					GlobalFree(gametype); // free old memory
					gametype = m_gametype;

					// Update gametype data
					*(DWORD*)(mapData + 4) = PtrToUlong(gametype);
					memcpy(mapData + 12, buff, sizeof(buff));

					// Cleanup any scripts associated with the current map
					if (m_scripts)
					{
						DWORD dwCount = *(DWORD*)m_scripts;

						for (size_t x = 0; x < dwCount; x++)
						{
							char* script = (char*)*(DWORD*)(m_scripts + 4 + (x * 4));
							GlobalFree(script);
						}

						GlobalFree(m_scripts);								
					}

					// Allocate memory for the scripts
					DWORD dwSize = 4 + (newMap.scripts.size() * 4);
					LPBYTE scriptData = (LPBYTE)GlobalAlloc(GMEM_FIXED, dwSize);

					// Populate the script data
					*(DWORD*)scriptData = newMap.scripts.size();

					for (size_t x = 0; x < newMap.scripts.size(); x++)
					{
						// Allocate memory for this specific script
						char* m_script = (char*)GlobalAlloc(GMEM_FIXED, newMap.scripts[x].size() + 1);
						strcpy_s(m_script, newMap.scripts[x].size() + 1, newMap.scripts[x].c_str());

						// Save this script
						*(DWORD*)(scriptData + 4 + (x * 4)) = (DWORD)m_script;
					}

					// Save the scripts
					*(DWORD*)(mapData + 8) = (DWORD)scriptData;									
				}
				catch (std::exception & e)
				{
					halo::hprintf("%s", e.what());
					logging::LogData(LOGGING_PHASOR, "%s", e.what());
				}
			}

			#ifdef PHASOR_PC
				UpdateLoadingMap(map);
			#endif

			current_map = map;

			return bMapUnchanged;
		}

		// This function returns the gametype data used for maps
		bool GetGametypeData(std::string mbGametype, LPBYTE szOutput, DWORD dwSize)
		{
			if (mbGametype.empty())
				return false;

			std::wstring szGametype = ToWideChar(mbGametype);

			LPBYTE lpListing = 0;

			try
			{
				// Get the path to the hdmu file
				std::string szDir = GetWorkingDirectory() + "\\saved\\hdmu.map";
				HANDLE hFile = CreateFile(szDir.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				if (hFile == INVALID_HANDLE_VALUE)
				{
					std::string err = "GetGametypeData:: Couldn't open the hdmu file: " + szDir;
					err += " - Reason: " + GetLastErrorAsText();
					throw std::exception(err.c_str());
				}

				DWORD dwFileSize = GetFileSize(hFile, NULL);
				lpListing = new BYTE[dwFileSize + 1];

				if (!lpListing)
				{
					CloseHandle(hFile);

					std::string err = "GetGametypeData:: Couldn't allocate memory for hdmu file";
					throw std::exception(err.c_str());
				}

				DWORD dwRead = 0;
				if (!ReadFile(hFile, lpListing, dwFileSize, &dwRead, NULL))
				{
					CloseHandle(hFile);

					std::string err = "GetGametypeData:: Couldn't read data from the hdmu file: " + szDir;
					err += " - Reason: " + GetLastErrorAsText();
					throw std::exception(err.c_str());
				}

				std::string szGametypeDir;
				std::transform(szGametype.begin(), szGametype.end(), szGametype.begin(), tolower);

				// Find the location of the gametype file
				DWORD n = 0;
				while (n < dwFileSize)
				{
					char* szFilePath = (char*)(lpListing + n);
					wchar_t* szGametypeName = (wchar_t*)(lpListing + n + 0x100);

					std::wstring checkName = szGametypeName;
					std::transform(checkName.begin(), checkName.end(), checkName.begin(), tolower);

					if (checkName == szGametype)
					{
						szGametypeDir = szFilePath;
						break;
					}

					n+= 0x206;
				}

				delete[] lpListing;
				lpListing = 0;

				CloseHandle(hFile);

				if (szGametypeDir.empty())
				{
					throw std::exception("The specified gametype is invalid.");
				}

				// If a buffer is provided read the gametype data into it
				if (szOutput)
				{
					HANDLE hFile = CreateFile(szGametypeDir.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

					if (hFile == INVALID_HANDLE_VALUE)
					{
						std::string err = "GetGametypeData:: Couldn't open gametype file: " + szGametypeDir + "\n";
						err += " - Reason: " + GetLastErrorAsText();
						throw std::exception(err.c_str());				
					}

					if (!ReadFile(hFile, szOutput, dwSize-4, &dwRead, NULL))
					{
						CloseHandle(hFile);

						std::string err = "GetGametypeData:: Couldn't read from gametype file: " + szGametypeDir;
						err += " - Reason: " + GetLastErrorAsText();
						throw std::exception(err.c_str());						
					}

					CloseHandle(hFile);
				}

				return true;
			}
			catch (std::exception & e)
			{
				if (lpListing)
					delete[] lpListing;
				
				logging::LogData(LOGGING_PHASOR, "%s", e.what());

				return false;
			}
		}

		// ----------------------------------------------------------------
		// SCRIPT LOADING CODE, THIS INCLUDES HOOKED SERVER COMMANDS (sv_map etc)
		// AND FULL MAPCYCLE CONTROL
		// ----------------------------------------------------------------
		namespace scriptmanager
		{
			struct mapcycleEntry
			{
				std::string map;
				std::string gametype;
				std::vector<std::string> scripts;
			};

			std::vector<mapcycleEntry*> mapcycleList;

			// This is used to track whether or not we're in a mapcycle
			bool mapcycle = false;

			void StartGame(const char* map);
			void ClearCurrentMapData();
			bool LoadCurrentMap(const char* map, const char* gametype, std::vector<std::string> scripts);

			// Loads scripts for the current game
			void Load()
			{
				Lua::CloseAllScripts();

				// get index for current game
				DWORD index = *(DWORD*)ADDR_MAPCYCLEINDEX;

				if (index >= 0 && index < *(DWORD*)ADDR_CURRENTMAPDATACOUNT)
				{
					LPBYTE this_map = (LPBYTE)(*(DWORD*)ADDR_CURRENTMAPDATA + (index * CONST_MENTRY_SIZE));
					LPBYTE scripts = (LPBYTE)*(DWORD*)(this_map + 8);

					if (scripts)
					{
						DWORD dwCount = *(DWORD*)scripts;

						// Loop through and load the scripts
						for (size_t x = 0; x < dwCount; x++)
							Lua::LoadScript((char*)(*(DWORD*)(scripts + 4 + (x * 4))));
					}
				}
			}

			// Hooked functions
			bool sv_mapcycle_begin(halo::h_player* exec, std::vector<std::string>& tokens)
			{
				// Check if there is any cycle data
				if (!mapcycleList.size())
				{
					halo::hprintf("There aren't any maps in the mapcycle.");

					return true;
				}

				// Clear the current playlist
				ClearCurrentMapData();

				// Loop through the cycle and add its data
				std::vector<mapcycleEntry*>::iterator itr = mapcycleList.begin();

				while (itr != mapcycleList.end())
				{
					LoadCurrentMap((*itr)->map.c_str(), (*itr)->gametype.c_str(), (*itr)->scripts);
					itr++;
				}

				// Set the map index to -1
				*(DWORD*)ADDR_MAPCYCLEINDEX	= -1;

				// Start the game
				StartGame(mapcycleList[0]->map.c_str());

				mapcycle = true;

				return true;
			}

			bool sv_mapcycle_add(halo::h_player* exec, std::vector<std::string>& tokens)
			{
				if (tokens.size() >= 3)
				{
					std::vector<std::string> scripts;

					if (!maps::ValidateMap((char*)tokens[1].c_str()))
					{
						halo::hprintf("%s isn't a valid map.", tokens[1].c_str());

						return true;
					}

					if (!maps::GetGametypeData((char*)tokens[2].c_str(), 0, 0))
					{
						halo::hprintf("%s isn't a valid gametype.", tokens[2].c_str());

						return true;
					}

					// store scripts if there are any
					if (tokens.size() > 3)
					{
						for (size_t x = 3; x < tokens.size(); x++)
						{
							// Check the script exists
							if (Lua::CheckScriptVailidity(tokens[x]))
								scripts.push_back(tokens[x]);
							else
							{
								logging::LogData(LOGGING_PHASOR, "sv_mapcycle_add: %s isn't a valid script and can therefore not be used.", tokens[x].c_str());
								halo::hprintf("sv_mapcycle_add: %s isn't a valid script and can therefore not be used.", tokens[x].c_str());
							}
						}
					}

					// allocate memory for mapcycle entry
					mapcycleEntry* entry = new mapcycleEntry;
					entry->map = tokens[1];
					entry->gametype = tokens[2];
					entry->scripts = scripts;

					// Add to list
					mapcycleList.push_back(entry);

					// If we're in the mapcycle add the data to the current playlist
					if (mapcycle)
						LoadCurrentMap(tokens[1].c_str(), tokens[2].c_str(), scripts);

					halo::hprintf("%s (game: %s) has been added to the mapcycle.", tokens[1].c_str(), tokens[2].c_str());

				}
				else
					halo::hprintf("Syntax: sv_mapcycle_add <map> <gametype> opt: <script1> <script2> ...");

				return true;
			}

			bool sv_mapcycle_del(halo::h_player* exec, std::vector<std::string>& tokens)
			{
				if (tokens.size() == 2)
				{
					DWORD index = atoi(tokens[1].c_str());

					// Make sure the index is valid
					if (index >= 0 && index < mapcycleList.size())
					{
						// Cleanup
						delete mapcycleList[index];

						// Remove from vector
						mapcycleList.erase(mapcycleList.begin() + index);

						// If we're currently running the mapcycle remove from current maps
						if (mapcycle)
						{
							// Clear the current playlist
							ClearCurrentMapData();

							// Loop through the cycle and add its data
							std::vector<mapcycleEntry*>::iterator itr = mapcycleList.begin();

							while (itr != mapcycleList.end())
							{
								LoadCurrentMap((*itr)->map.c_str(), (*itr)->gametype.c_str(), (*itr)->scripts);
								itr++;
							}

							DWORD cur = *(DWORD*)ADDR_MAPCYCLEINDEX;

							// Make sure the index is still within bounds
							if (cur	>= mapcycleList.size())
								*(DWORD*)ADDR_MAPCYCLEINDEX = -1; // restart the cycle
							else if (index <= cur && cur > -1)
								*(DWORD*)ADDR_MAPCYCLEINDEX = cur - 1;
						}

						// Display cycle as it is now
						sv_mapcycle(exec, tokens);
					}
					else
						halo::hprintf("You entered an invalid index, see sv_mapcycle.");
				}
				else
					halo::hprintf("Syntax: sv_mapcycle_del <index>");

				return true;
			}

			bool sv_mapcycle(halo::h_player* exec, std::vector<std::string>& tokens)
			{
				halo::hprintf("   Map                  Variant         Script(s)");

				std::vector<mapcycleEntry*>::iterator itr = mapcycleList.begin();

				int x = 0;
				while (itr != mapcycleList.end())
				{
					std::string szEntry =  m_sprintf_s("%i  %s", x, (*itr)->map.c_str());
					
					int tabCount = 3 - (szEntry.size() / 8);

					for (int i = 0; i < tabCount; i++)
						szEntry += "\t";

					szEntry += (*itr)->gametype;
					int len = (*itr)->gametype.size();
					tabCount = 2 - (len / 8);

					for (int i = 0; i < tabCount; i++)
						szEntry += "\t";

					// Loop through and add scripts (if any)
					for (size_t i = 0; i < (*itr)->scripts.size(); i++)
					{
						if (i != 0)
							szEntry += ",";
						szEntry += (*itr)->scripts[i];
					}

					if (!(*itr)->scripts.size())
						szEntry += "<no scripts>";

					std::string output = ExpandTabsToSpace(szEntry.c_str());
					halo::hprintf("%s", output.c_str());

					itr++; x++;
				}

				return true;
			}

			bool sv_map(halo::h_player* exec, std::vector<std::string>& tokens)
			{
				if (tokens.size() >= 3)
				{
					std::vector<std::string> scripts;

					if (tokens.size() > 3)
					{
						for (size_t x = 3; x < tokens.size(); x++)
						{
							// Check the script exists
							if (Lua::CheckScriptVailidity(tokens[x]))
								scripts.push_back(tokens[x]);
							else
							{
								logging::LogData(LOGGING_PHASOR, "sv_map: %s isn't a valid script and therefore can't be used.", tokens[x].c_str());
								halo::hprintf("sv_map: %s isn't a valid script and therefore can't be used.", tokens[x].c_str());
							}
						}
					}

					// Clear all currently loaded maps
					ClearCurrentMapData();

					// Load this map
					if (LoadCurrentMap(tokens[1].c_str(), tokens[2].c_str(), scripts))
					{
						if (vote::IsEnabled())
						{
							halo::hprintf("Map voting has been disabled.");
							vote::SetMapVote(false);
						}

						*(DWORD*)ADDR_MAPCYCLEINDEX	= -1;

						// Start the game
						StartGame(tokens[1].c_str());

						// Stop map voting from calling sv_mapcycle_begin if its set to
						//mapvote::DisableMapcycle();

						mapcycle = false;

						halo::hprintf("%s is being loaded with game type %s", tokens[1].c_str(), tokens[2].c_str());
					}			
				}
				else
					halo::hprintf("Syntax: sv_map <map> <gametype> opt: <script1> <script2> ...");

				return true;
			}

			bool sv_end_game(halo::h_player* exec, std::vector<std::string>& tokens)
			{
				mapcycle = false;
				return false;
			}

			// Custom functions
			bool sv_reloadscripts(halo::h_player* exec, std::vector<std::string>& tokens)
			{
				Lua::ReloadScripts();
				halo::hprintf("All currently loaded scripts have been reloaded.");

				return true;
			}

			void ClearCurrentMapData()
			{
				// Get the current map data
				LPBYTE m_mapinfo = (LPBYTE)*(DWORD*)ADDR_CURRENTMAPDATA;

				if (m_mapinfo)
				{
					DWORD dwCount = *(DWORD*)ADDR_CURRENTMAPDATACOUNT;

					// Loop through all maps
					for (size_t i = 0; i < dwCount; i++)
					{
						char* cur_map = (char*)*(DWORD*)(m_mapinfo + (i * CONST_MENTRY_SIZE));
						char* cur_gametype = (char*)*(DWORD*)(m_mapinfo + (i * CONST_MENTRY_SIZE) + 4);
						LPBYTE cur_scripts = (LPBYTE)*(DWORD*)(m_mapinfo + (i * CONST_MENTRY_SIZE) + 8);

						// Free each pointer
						if (cur_map)
							GlobalFree(cur_map);

						if (cur_gametype)
							GlobalFree(cur_gametype);

						if (cur_scripts)
						{
							DWORD scriptCount = *(DWORD*)cur_scripts;

							for (size_t x = 0; x < scriptCount; x++)
							{
								char* script = (char*)*(DWORD*)(cur_scripts + 4 + (x * 4));
								GlobalFree(script);
							}

							GlobalFree(cur_scripts);
						}
					}

					GlobalFree(m_mapinfo);

					// Reset counters
					*(DWORD*)ADDR_CURRENTMAPDATACOUNT = 0;
					*(DWORD*)ADDR_CURRENTMAPDATA = 0;
				}
			}

			// Loads a map into the currently loaded cycle
			bool LoadCurrentMap(const char* map, const char* gametype, std::vector<std::string> scripts)
			{
				// Check if the map is valid
				if (!maps::ValidateMap((char*)map))
				{
					halo::hprintf("%s is an invalid map.", map);

					return false; 
				}

				// Get the gametype data
				BYTE buff[GAMET_BUFFER_SIZE] = {0};
				if (!GetGametypeData((char*)gametype, buff, sizeof(buff)))
				{ 
					halo::hprintf("%s is an invalid gametype.", gametype);

					return false;
				}

				// Allocate memory for the map
				DWORD len = strlen(map) + 1;
				char* m_map = (char*)GlobalAlloc(GMEM_FIXED, len);
				strcpy_s(m_map, len, map);

				// Allocate memory for the gametype
				len = strlen(gametype) + 1;
				char* m_gametype = (char*)GlobalAlloc(GMEM_FIXED, len);
				strcpy_s(m_gametype, len, gametype);

				// Allocate memory for the scripts
				DWORD dwCount = 4 + (scripts.size() * 4);
				LPBYTE scriptData = (LPBYTE)GlobalAlloc(GMEM_FIXED, dwCount);
				*(DWORD*)scriptData = scripts.size();

				// Populate the script data
				for (size_t x = 0; x < scripts.size(); x++)
				{
					char* m_script = (char*)GlobalAlloc(GMEM_FIXED, scripts[x].size() + 1);
					strcpy_s(m_script, scripts[x].size() + 1, scripts[x].c_str());
					*(DWORD*)(scriptData + 4 + (x * 4)) = (DWORD)m_script;
				}

				// Get address of current map (base)
				LPBYTE m_mapinfo = (LPBYTE)*(DWORD*)ADDR_CURRENTMAPDATA;
				LPBYTE new_data = 0;

				// We need to expand the allocation
				if (m_mapinfo)
				{
					DWORD dwCount = *(DWORD*)ADDR_CURRENTMAPDATACOUNT;
					LPBYTE new_mapinfo = (LPBYTE)GlobalAlloc(GMEM_FIXED, CONST_MENTRY_SIZE * (dwCount + 1));

					// Copy old data over
					memcpy(new_mapinfo, m_mapinfo, dwCount * CONST_MENTRY_SIZE);

					// Calculate pointer for new data
					new_data = new_mapinfo + (dwCount * CONST_MENTRY_SIZE);

					GlobalFree(m_mapinfo);
					*(DWORD*)ADDR_CURRENTMAPDATACOUNT = dwCount + 1;
					*(DWORD*)ADDR_CURRENTMAPDATA = (DWORD)new_mapinfo;
				}
				else // no allocation
				{
					new_data = (LPBYTE)GlobalAlloc(GMEM_FIXED, CONST_MENTRY_SIZE);
					*(DWORD*)ADDR_CURRENTMAPDATA = (DWORD)new_data;
					*(DWORD*)ADDR_CURRENTMAPDATACOUNT = 1;
				}

				// Write the info into new_data
				*(DWORD*)new_data = (DWORD)m_map;
				*(DWORD*)(new_data + 4) = (DWORD)m_gametype;
				*(DWORD*)(new_data + 8) = (DWORD)scriptData;
				memcpy(new_data + 12, buff, sizeof(buff));

				return true;
			}

			// This function is effectively sv_map_next
			void StartGame(const char* map)
			{
				if (*(DWORD*)ADDR_GAMEREADY != 2)
				{
					// start the server not just game
					DWORD dwInitGame1 = FUNC_PREPAREGAME_ONE;
					DWORD dwInitGame2 = FUNC_PREPAREGAME_TWO;
					__asm
					{
						pushad
						MOV EDI,map
						CALL dword ptr ds:[dwInitGame1]

						push 0
						push esi // we need a register for a bit
						mov esi, dword ptr DS:[ADDR_PREPAREGAME_FLAG]
						mov byte PTR ds:[esi], 1
						pop esi
						call dword ptr ds:[dwInitGame2]
						add esp, 4
						popad
					}
				}
				else
				{
					// Halo 1.09 addresses
					// 00517845  |.  BF 90446900   MOV EDI,haloded.00694490                                  ;  UNICODE "ctf1"
					//0051784A  |.  F3:A5         REP MOVS DWORD PTR ES:[EDI],DWORD PTR DS:[ESI]
					//0051784C  |.  6A 00         PUSH 0
					//	0051784E  |.  C605 28456900>MOV BYTE PTR DS:[694528],1
					//	00517855  |.  E8 961B0000   CALL haloded.005193F0                                     ;  start server
					DWORD dwStartGame = FUNC_EXECUTEGAME;

					__asm
					{
						pushad
						call dword ptr ds:[dwStartGame]
						popad
					}
				}
			}
		};
		
		// ----------------------------------------------------------------
		// MAP VOTING CODE
		// ----------------------------------------------------------------
		namespace vote
		{
			bool voting = false; // used for current vote
			bool do_vote = false; // if map vote is enabled
			std::vector<cmap> mapList, displayList;

			// Returns a boolean representing whether or not voting is enabled
			bool IsEnabled()
			{
				return do_vote;
			}

			// Called when a map is to be added to vote options
			void AddVotableMap(const char* map, const char* gametype, const char* description, std::vector<std::string> scripts)
			{
				try
				{
					// Make sure the map is valid
					if (maps::ValidateMap((char*)map))
					{
						// Make sure the gametype is valid
						if (GetGametypeData((char*)gametype, 0, 0))
						{
							// Add map to the list
							cmap vote;
							vote.map = map;
							vote.gametype = gametype;
							vote.description = description;
							vote.scripts = scripts;
							mapList.push_back(vote);

							halo::hprintf("%s has been added to the votable maps.", description);
						}
						else
						{
							std::string status = m_sprintf_s("%s is an invalid gametype.", gametype);
							throw std::exception(status.c_str());
						}
					}
					else
					{
						std::string status = m_sprintf_s("%s is an invalid map.", map);
						throw std::exception(status.c_str());
					}
				}
				catch (std::exception & e)
				{
					halo::hprintf("%s", e.what());
					std::string error = std::string("sv_mapvote_add: ") + e.what();
					logging::LogData(LOGGING_PHASOR, "%s", error.c_str());
				}
			}

			// Removes a map from the vote options
			bool RemoveVotableMap(DWORD index)
			{
				bool bRet = true;

				// Make sure the index is valid
				if (index < mapList.size() && index >= 0)
				{
					// Remove the map from the list
					mapList.erase(mapList.begin() + index);

					if (!mapList.size())
					{
						halo::hprintf(L"There are longer any votable maps. Map voting disabled.");
						SetMapVote(false);
					}
				}
				else
					bRet = false;

				return bRet;
			}

			// Called when a map vote starts
			void OnBegin()
			{
				if (do_vote && mapList.size())
				{
					// Tell the server we're ready for votes
					server::GlobalMessage(L"*SERVER*: Please vote for the next map by typing 'vote <option>':");

					displayList.clear();
					
					// clear any existing votes
					std::vector<halo::h_player*> players = game::GetPlayerList();
					for (std::vector<halo::h_player*>::iterator itr = players.begin(); 
						itr != players.end(); itr++)
					{
						(*itr)->mapVote = -1;
					}

					size_t mapListSize = mapList.size();

					// Check if we need to randomly select the maps
					if (mapListSize > 5)
					{
						std::vector<cmap> tmpList = mapList;

						size_t tmpListSize = tmpList.size();

						for (int i = 0; i < 5; i++)
						{
							int rindex = rand() %  (tmpListSize);

							// Add to the display list
							displayList.push_back(tmpList[rindex]);

							// Remove from the selection vector
							tmpList.erase(tmpList.begin() + rindex);
							tmpListSize--;
						}
					}
					else
						displayList = mapList;

					for (size_t x = 0; x < displayList.size(); x++)
					{
						// Build the vote message
						std::string voteMsg = m_sprintf_s("[%i] %s", x + 1, displayList[x].description.c_str());

						// Convert to wide char
						std::wstring as_wide = ToWideChar(voteMsg);

						// Send the message to the players
						server::GlobalMessage((wchar_t*)as_wide.c_str());
					}

					voting = true;
				}

			}

			// Called when the map vote is over
			bool OnEnd(cmap & decision)
			{
				if (!voting || !displayList.size())
					return false;

				voting = false;

				int results[] = {0,0,0,0,0};

				std::vector<halo::h_player*> players = GetPlayerList();

				// count up all the votes
				for (std::vector<halo::h_player*>::iterator itr = players.begin();
					itr != players.end(); itr++)
				{
					// If they voted increment the total vote count
					if ((*itr)->mapVote != -1)
						results[(*itr)->mapVote]++;
				}

				// find the most voted option(s)
				std::vector<int> maxVoted;
				int maxVotes = 0;
				for (size_t i = 0; i < displayList.size(); i++)
				{
					// Check if the votes equal the max
					if (results[i] == maxVotes)
						maxVoted.push_back(i);
					else if (results[i] > maxVotes)
					{
						maxVoted.clear();
						maxVoted.push_back(i);
						maxVotes = results[i];
					}
				}

				int index_ = rand() % maxVoted.size();
				int nextMapIndex = maxVoted[index_];
				decision = displayList[nextMapIndex];

				return true;
			}

			// Called when there's server chat
			bool OnServerChat(halo::h_player* player, wchar_t* msg)
			{
				bool bRet = true;

				if (voting)
				{
					std::vector<std::wstring> tokens = TokenizeWideString(msg, L" ");

					if (tokens.size() == 1 || tokens.size() == 2)
					{
						size_t vote = 0;

						// Allow a few typos
						if (tokens[0] == L"vote" || tokens[0] == L"vtoe" || tokens[0] == L"voet")
						{
							if (tokens.size() == 2)
							{
								// Get the vote
								vote = _wtoi(tokens[1].c_str());
							}
						}
						else if (tokens.size() == 1)
						{
							if (tokens[0].size() == 1)
								vote = _wtoi(tokens[0].c_str());
						}

						// Validate the vote
						if (vote >= 1 && vote <= 5)
						{
							if (vote <= displayList.size())
							{
								vote--;

								if (player->mapVote == -1)
									player->ServerMessage(L"Thank you. Your vote has been received.");
								else
									player->ServerMessage(L"Thank you. Your vote has been updated.");

								player->mapVote = vote;
							}

							bRet = false;
						}
					}
				}

				return bRet;
			}

			// Toggles map voting
			bool SetMapVote(bool new_status)
			{
				bool success = true;

				if (!mapList.size())
					success = false;
				do_vote = new_status;

				return success;
			}

			bool sv_mapvote(halo::h_player* exec, std::vector<std::string>& tokens)
			{
				if (tokens.size() == 2)
				{
					if (tokens[1] == "true" || tokens[1] == "1")
					{
						if (SetMapVote(true))
							halo::hprintf(L"Map voting has been enabled.");							
						else
							halo::hprintf(L"There are no entries in the map vote list.");
					}
					else
					{
						SetMapVote(false);
						halo::hprintf("Map voting has been disabled.");
					}
				}
				else
					halo::hprintf("Correct usage: sv_mapvote <status>");

				return true;
			}

			bool sv_mapvote_begin(halo::h_player* exec, std::vector<std::string>& tokens)
			{
				if (tokens.size() == 1)
				{
					if (!do_vote) // only continue if voting is enabled
						halo::hprintf("Map voting isn't enabled.");
					else if (!mapList.size())
						halo::hprintf("The map vote list is empty.");
					else
					{
						// Clear all currently loaded maps
						scriptmanager::ClearCurrentMapData();

						// Load this map
						if (scriptmanager::LoadCurrentMap(mapList[0].map.c_str(), mapList[0].gametype.c_str(), mapList[0].scripts))
						{
							*(DWORD*)ADDR_MAPCYCLEINDEX	= -1;

							// Start the game
							scriptmanager::StartGame(mapList[0].map.c_str());

							scriptmanager::mapcycle = false;
						}	
					}
				}
				else
					halo::hprintf("Correct usage: sv_mapcycle_begin");

				return true;
			}

			bool sv_mapvote_add(halo::h_player* exec, std::vector<std::string>& tokens)
			{
				if (tokens.size() >= 4)
				{
					std::vector<std::string> scriptlist;

					// Build the script list
					if (tokens.size() > 4)
					{
						for (size_t x = 4; x < tokens.size(); x++)
						{
							if (Lua::CheckScriptVailidity(tokens[x]))
							{
								scriptlist.push_back(tokens[x]);
							}
							else
							{
								halo::hprintf("%s isn't a valid script.", tokens[x].c_str());

								logging::LogData(LOGGING_PHASOR, "sv_mapvote_add: %s isn't a valid script.", tokens[x].c_str());
							}
						}
					}

					AddVotableMap(tokens[1].c_str(), tokens[2].c_str(), tokens[3].c_str(), scriptlist);
				}
				else
					halo::hprintf("Correct usage: sv_mapvote_add <map> <gametype> <description> opt:<scripts>");

				return true;
			}

			bool sv_mapvote_del(halo::h_player* exec, std::vector<std::string>& tokens)
			{
				if (tokens.size() == 2)
				{
					int index = atoi(tokens[1].c_str());

					if (RemoveVotableMap(index))
						halo::hprintf("The map has been removed from the voting options.");
					else
						halo::hprintf("The map couldn't be removed because the index is invalid.");
				}
				else
					halo::hprintf("Correct usage: sv_mapvote_del <index>");

				return true;
			}

			bool sv_mapvote_list(halo::h_player* exec, std::vector<std::string>& tokens)
			{
				halo::hprintf("   Map                  Gametype        Description");

				int count = 0;
				for (std::vector<cmap>::iterator itr = mapList.begin();
					itr != mapList.end(); itr++, count++)
				{
					std::string disp = itr->map;
					int len = itr->map.size() + 3; // 3 spaces at start

					// Fix size based on length of index
					if (count >= 10 && count < 100)
						len++;
					else if (count > 999)
						len += 2;

					// get the number of tabs to append
					int tabCount = 3 - len / 8;

					for (int i = 0; i < tabCount; i++)
						disp += "\t";

					// append gametype
					disp += itr->gametype;

					// get tab count required
					len = itr->gametype.size();
					tabCount = 2 - len / 8;

					// append tabs
					for (int i = 0; i < tabCount; i++)
						disp += "\t";

					// Build final message
					disp = m_sprintf_s("%i  %s%s", count, disp.c_str(), itr->description.c_str());
					disp = ExpandTabsToSpace(disp.c_str());

					// Display message
					halo::hprintf("%s", disp.c_str());
				}

				return true;
			}
		}
	};
};