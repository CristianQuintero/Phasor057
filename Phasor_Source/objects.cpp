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

#include "objects.h"
#include "Addresses.h"
#include "Halo.h"
#include "Server.h"
#include "Maps.h"
#include "Common.h"
#include <map>
#include <hash_map>

namespace objects
{
	// To save computation time tags that have already been looked up
	// associated with a map. 
	std::map<DWORD, hTagHeader*> tagCache_id;
	stdext::hash_map<const char*, hTagHeader*> tagCache_str;

	// ----------------------------------------------------------------
	// Functions
	// 
	void ClearTagCache()
	{
		tagCache_id.clear();
		tagCache_str.clear();
	}

	// Swaps the endian of a 32bit value
	DWORD endian_swap(unsigned int x)
	{
		return (x>>24) | 
			((x<<8) & 0x00FF0000) |
			((x>>8) & 0x0000FF00) |
			(x<<24);
	}

	// This function gets an object's address
	PObject* GetObjectAddress(int mode, DWORD objectId)
	{
		// Make sure request data is valid
		if (objectId == -1)
			return 0;

		WORD hi = (objectId >> 16) & 0xFFFF;
		WORD lo = (objectId & 0xFFFF);

		LPBYTE lpObjectList = (LPBYTE)*(DWORD*)UlongToPtr(ADDR_OBJECTBASE);

		if (lo < (*(WORD*)(lpObjectList + 0x20)))
		{
			LPBYTE objPtr = (LPBYTE)ULongToPtr(*(DWORD*)(lpObjectList + 0x34) + (*(WORD*)(lpObjectList + 0x22) * lo));

			if (!*(WORD*)objPtr)
				return 0;

			LPBYTE finalPtr = 0;
			if (!hi)
				finalPtr = objPtr;
			else if (hi == *(WORD*)objPtr)
				finalPtr = objPtr;

			if (!finalPtr)
				return 0;

			if (mode < (1 << finalPtr[2]))
				return (PObject*)*(DWORD*)UlongToPtr(finalPtr + 8);
		}

		return 0;
	}

	// ----------------------------------------------------------------
	// GENERIC STUFF
	// Returns the object that controls the given object's coordinates
	// It currently only works with players in vehicles
	PObject* GetPositionalObject(PObject* m_obj)
	{
		// If in a vehicle it determines the location
		if (m_obj && m_obj->base.vehicleId != 0xFFFFFFFF)
		{
			PObject* m_obj1 = objects::GetObjectAddress(3, m_obj->base.vehicleId);
			if (m_obj1)
				m_obj = m_obj1;
		}
		return m_obj;
	}

	// --------------------------------------------------------------------
	// Teleporting stuff
	namespace tp
	{
		struct locdata
		{
			std::string name;
			std::string map;
			vect3d location;

			locdata(std::string l_name, std::string l_map, float x, float y, float z)
			{
				name = l_name;
				map = l_map;
				location.x = x;
				location.y = y;
				location.z = z;
			}
		};

		// Used to store the location data
		std::vector<locdata*> LocationList;

		// Saves the location list to file
		void SaveData();

		// Saves a location (x,y,z) with a reference name
		bool AddLocation(std::string name, float x, float y, float z)
		{
			bool bSuccess = true;
			std::string map = game::maps::GetCurrentMap();

			if (map.size())
			{
				// Check if the name is already in use within this map
				std::vector<locdata*>::iterator itr = LocationList.begin();

				while (itr != LocationList.end())
				{
					if ((*itr)->map == map && (*itr)->name == name)
						bSuccess = false;

					itr++;
				}

				if (bSuccess)
				{
					locdata* loc = new locdata(name, map, x, y, z);
					LocationList.push_back(loc);
					SaveData();
				}
			}

			return bSuccess;
		}

		// Removes a saved location
		bool RemoveLocation(int index)
		{
			bool bRet = true;

			std::string map = game::maps::GetCurrentMap();

			// The provided index is relative to the current map
			// so search through the vector for map entries and remove the 'index'th one.
			int n = 0;
			for (size_t x = 0; x < LocationList.size(); x++)
			{
				if (LocationList[x]->map == map)
				{
					if (n == index)
					{
						LocationList.erase(LocationList.begin() + x);
						bRet = true;
						SaveData();
						break;
					}
					else
						n++;
				}
			}

			return bRet;
		}

		// Moves an object to the specified position
		bool MoveObject(DWORD m_objId, float x, float y, float z)
		{
			bool bSuccess = false;
			PObject* m_object = GetPositionalObject(GetObjectAddress(0, m_objId));

			if (m_object)
			{
				m_object->base.location.x = x;
				m_object->base.location.y = y;
				m_object->base.location.z = z;
				m_object->base.ignoreGravity = 0; // stops objects staying in air
				bSuccess = true;
			}
			return bSuccess;
		}

		// Moves an object to the specified position
		bool MoveObject(DWORD m_objId, std::string location)
		{
			bool bSuccess = false;
			std::string map = game::maps::GetCurrentMap();
			PObject* m_object = GetObjectAddress(0, m_objId);

			// Lookup the location name
			std::vector<locdata*>::iterator itr = LocationList.begin();
			while (itr != LocationList.end())
			{
				if ((*itr)->name == location && (*itr)->map == map)
				{
					bSuccess = MoveObject(m_objId, (*itr)->location.x, (*itr)->location.y,
						(*itr)->location.z);
					break;					
				}
				itr++;
			}		
			
			return bSuccess;
		}

		// Called to load location data
		void LoadData()
		{
			try
			{
				std::string dir = GetWorkingDirectory() + "\\data\\locations.txt";

				std::vector<std::string> fileData;

				// Load location list
				if (LoadFileIntoVector(dir.c_str(), fileData))
				{
					for (size_t i = 0; i < fileData.size(); i++)
					{
						std::vector<std::string> tokens = TokenizeString(fileData[i], ",");

						// Make sure it's correctly formatted
						if (tokens.size() != 5)
						{
							logging::LogData(LOGGING_PHASOR, L"locations.txt: Line %i is incorrectly formatted.", i+1);

							continue;
						}

						// Get the coordinates
						float x = (float)atof(tokens[2].c_str());
						float y = (float)atof(tokens[3].c_str());
						float z = (float)atof(tokens[4].c_str());

						locdata* loc = new locdata(tokens[1], tokens[0], x, y, z);
						LocationList.push_back(loc);
					}
				}
			}
			catch (std::exception & e)
			{
				logging::LogData(LOGGING_PHASOR, "locations.txt: LoadData failed because: %s", e.what());
			}
		}

		// Called to unload location data
		void Cleanup()
		{
			std::vector<locdata*>::iterator itr = LocationList.begin();

			while (itr != LocationList.end())
			{
				delete (*itr);
				itr = LocationList.erase(itr);
			}
		}

		// Saves the location list to file
		void SaveData()
		{
			std::string dir = GetWorkingDirectory() + "\\data\\locations.txt";

			// Open the location file
			FILE* pFile = fopen(dir.c_str(), "w");

			if (!pFile)
			{
				std::string err = "Cannot write to location data at: " + dir;

				throw std::exception(err.c_str());
			}

			// Check if the name is already in use within this map
			std::vector<locdata*>::iterator itr = LocationList.begin();

			while (itr != LocationList.end())
			{
				fprintf(pFile, "%s,%s,%f,%f,%f\r\n", (*itr)->map.c_str(), (*itr)->name.c_str(), (*itr)->location.x, (*itr)->location.y, (*itr)->location.z);
				itr++;
			}

			fclose(pFile);
		}

		// ------------------------------------------------------------------------
		// User commands
		bool sv_teleport(halo::h_player* execPlayer, std::vector<std::string>& tokens)
		{
			// sv_teleport <player> <location name> (OR) <x> <y> <z>
			if (tokens.size() == 3 || tokens.size() == 5)
			{
				// Find the player to teleport
				int playerId = atoi(tokens[1].c_str()) - 1;
				halo::h_player* m_player = game::GetPlayer_pi(playerId);

				if (m_player)
				{
					bool b = false;
					if (tokens.size() == 3)
						b = MoveObject(m_player->mem->m_playerObjectid, tokens[2]);		
					else
					{
						float x = (float)atof(tokens[2].c_str());
						float y = (float)atof(tokens[3].c_str());
						float z = (float)atof(tokens[4].c_str());

						b = MoveObject(m_player->mem->m_playerObjectid, x, y, z);
					}

					if (b)
						halo::hprintf("The player has been successfully teleported.");
					else
						halo::hprintf("The player couldn't be teleported, make sure the location is valid.");
				}
				else
					halo::hprintf("The specified player doesn't exist.");
			}
			else
				halo::hprintf("Syntax: sv_teleport <player> either: <name> or: <x> <y> <z>");

			return true;
		}

		bool sv_teleport_add(halo::h_player* execPlayer, std::vector<std::string>& tokens)
		{
			// sv_teleport_add <name> opt: <x> <y> <z>
			if (tokens.size() == 2 || tokens.size() == 5)
			{
				vect3d location = {0};
				// Check which method is being used
				if (tokens.size() == 2) // use current coordinates
				{
					// Find the position of the player executing the command
					if (!execPlayer && execPlayer->m_object)
					{
						halo::hprintf("Your location couldn't be determined, try specifying the coordinates.");

						return true;
					}

					location = execPlayer->m_object->base.location;
				}
				else
				{
					location.x = (float)atof(tokens[2].c_str());
					location.y = (float)atof(tokens[3].c_str());
					location.z = (float)atof(tokens[4].c_str());
				}

				// Try to add the name
				if (AddLocation(tokens[1], location.x, location.y, location.z))
					halo::hprintf("%s now corresponds to: %.2f %.2f %.2f for this map.", tokens[1].c_str(),
					 location.x, location.y, location.z);
			}
			else
				halo::hprintf("Syntax: sv_teleport_add <name> opt: <x> <y> <z>");

			return true;
		}

		bool sv_teleport_del(halo::h_player* execPlayer, std::vector<std::string>& tokens)
		{
			if (tokens.size() == 2)
			{
				int i = atoi(tokens[1].c_str());

				if (RemoveLocation(i))
					halo::hprintf("The location has been removed.");
				else
					halo::hprintf("The specified index is invalid. Use sv_teleport_list for a list of locations.");
			}
			else
				halo::hprintf("Syntax: sv_teleport_del <index>");	

			return true;
		}

		bool sv_teleport_list(halo::h_player* execPlayer, std::vector<std::string>& tokens)
		{
			halo::hprintf("index x,y,z name");
			std::string map = game::maps::GetCurrentMap();

			if (!map.size())
			{
				halo::hprintf("The current map couldn't be determined.");

				return true;
			}

			int n = 0;
			for (size_t x = 0; x < LocationList.size(); x++)
			{
				if (LocationList[x]->map == map)
				{
					halo::hprintf("%i  %.2f,%.2f,%.2f    %s", n, LocationList[x]->location.x, 
						LocationList[x]->location.y, LocationList[x]->location.z, LocationList[x]->name.c_str());
					n++;
				}
			}

			return true;
		}
	}

	// --------------------------------------------------------------------
	// Returns the tag data for a specific map id. 
	hTagHeader* LookupTag(DWORD dwMapId)
	{
		hTagHeader* result = 0;

		std::map<DWORD, hTagHeader*>::iterator cached = tagCache_id.find(dwMapId);

		if (cached == tagCache_id.end()) // hasn't been looked up yet
		{
			// do a linear search through the table
			hTagIndexTableHeader* header = (hTagIndexTableHeader*)UlongToPtr(*(DWORD*)(LPBYTE)ADDR_TAGTABLE);
			hTagHeader* walkPtr = (hTagHeader*)header->next_ptr;

			for (size_t i = 0; i < header->entityCount; i++)
			{
				// Check if the tag type matches
				if (dwMapId == walkPtr->id)
				{
					// add to map
					tagCache_id[dwMapId] = walkPtr;
					result = walkPtr;
					break;
				}

				walkPtr++; // next tag
			}
		}
		else
			result = cached->second;

		return result;		
	}

	// Returns the tag data for a specific tag type/name 
	hTagHeader* LookupTag(const char* tagType, const char* tag)
	{
		hTagHeader* result = 0;

		// see if it's been cached
		std::string key = tagType + std::string("\\") + tag;
		stdext::hash_map<const char*, hTagHeader*>::iterator cached = tagCache_str.find(key.c_str());

		if (cached == tagCache_str.end()) // hasn't been looked up yet
		{
			DWORD type = endian_swap(*(DWORD*)tagType);

			hTagIndexTableHeader* header = (hTagIndexTableHeader*)UlongToPtr(*(DWORD*)(LPBYTE)ADDR_TAGTABLE);
			hTagHeader* walkPtr = (hTagHeader*)header->next_ptr;
			hTagHeader* tagptr = 0;

			for (size_t i = 0; i < header->entityCount; i++)
			{
				// Check if the tag matches
				if (type == walkPtr->tagType && !strcmp(walkPtr->tagName, tag))
				{
					tagCache_str.insert(std::pair<const char*, hTagHeader*>(key.c_str(),walkPtr));
					result = walkPtr;
					break;
				}

				walkPtr++;
			}
		}
		else
			result = cached->second;

		return result;
	}

	// --------------------------------------------------------------------
	// OBJECT SPAWNING
	//
	struct ph_objInfo
	{
		DWORD m_objId, respawnTicks, creationTicks;
		PObject* m_object;
		bool bRecycle; // if false object gets destroyed instead of respawned
		vect3d spawnPoint, velocity, rotation, axialPos;

		ph_objInfo(DWORD obj, DWORD ticks, bool recycle, float x, float y, float z)
		{
			m_objId = obj;
			m_object = GetObjectAddress(3, m_objId);
			respawnTicks = ticks;
			creationTicks = server::GetServerTicks();
			bRecycle = recycle;
			spawnPoint.x = x;
			spawnPoint.y = y;
			spawnPoint.z = z;
			velocity = m_object->base.velocity;
			rotation = m_object->base.rotation;
			axialPos = m_object->base.axial;
		}
	};

	// List of all object's phasor needs to manage
	std::map<DWORD, ph_objInfo*> ph_objList;

	// All objects are removed when the map is reset
	bool sv_map_reset(halo::h_player* execPlayer, std::vector<std::string>& tokens)
	{
		std::map<DWORD, ph_objInfo*>::iterator itr = ph_objList.begin();
		for (; itr != ph_objList.end(); itr++)
			DestroyObject(itr->second->m_objId);

		ClearObjectSpawns();

		return false; // let server process it too
	}

	void ClearObjectSpawns()
	{
		// Free all memory allocated to the managed objects
		std::map<DWORD, ph_objInfo*>::iterator itr = ph_objList.begin();
		for (; itr != ph_objList.end(); itr++)
			delete itr->second;
		
		ph_objList.clear();
	}

	// Destroys an object
	void DestroyObject(DWORD m_objId)
	{
		__asm
		{
			pushad
			mov eax, m_objId
			call dword ptr ds:[FUNC_DESTROYOBJECT]
			popad
		}
	}

	// Called when an object is being checked to see if it should respawn
	int __stdcall ObjectRespawnCheck(DWORD m_objId, LPBYTE m_obj)
	{
		int bRespawn = false; // false is don't respawn

		std::map<DWORD, ph_objInfo*>::iterator itr = ph_objList.find(m_objId);
		DWORD dwIdle = *(DWORD*)(m_obj + 0x5AC); // ticks when object became idle

		if (dwIdle != 0xFFFFFFFF) // ffffffff means currently active
		{
			if (itr != ph_objList.end())
			{
				ph_objInfo* ph_obj = itr->second;
				if (ph_obj->respawnTicks)
				{
					DWORD expiration = dwIdle + ph_obj->respawnTicks;
					if (expiration < server::GetServerTicks())
					{
						if (ph_obj->bRecycle)
						{
							LPBYTE axial = (LPBYTE)&ph_obj->axialPos, rotation = (LPBYTE)&ph_obj->rotation;
							LPBYTE pos1 = (LPBYTE)&ph_obj->spawnPoint;
							DWORD id = ph_obj->m_objId, func = 0x0052C310, func1 = 0x0052C2B0;

							__asm
							{
								pushad
								push id
								call dword ptr ds:[func1] // set flags to let object fall, reset velocities etc
								add esp, 4
								push axial
								push rotation
								push id
								mov edi, pos1
								call dword ptr ds:[func] // move the object (proper way)
								add esp, 0x0c
								popad
							}

							// set last interacted to be now
							*(DWORD*)(m_obj + 0x5AC) = server::GetServerTicks();

						}
						else // delete object
						{
							DestroyObject(ph_obj->m_objId);
							bRespawn = 2; // object deleted
						}
					}
				}
			}
			else // not managed, use default processing
			{
				DWORD dwRespawn = game::GetRespawnTicks();

				if (dwRespawn > 0)
				{
					DWORD expiration = dwIdle + dwRespawn;
					if (expiration < server::GetServerTicks())
						bRespawn = true;
				}
			}
		}

		return bRespawn;
	}

	// This is called when weapons/equipment are going to be destroyed.
	// Return value: True -> Destroy (well, tells server it is due to be destroyed
	// may not be destroyed based on obj attributes though), False -> Keep
	bool __stdcall EquipmentDestroyCheck(int checkTicks, DWORD m_objId, LPBYTE m_obj)
	{
		bool bDestroy = false;

		std::map<DWORD, ph_objInfo*>::iterator itr = ph_objList.find(m_objId);

		if (itr != ph_objList.end()) // check if we're managing this object
		{
			ph_objInfo* ph_obj = itr->second;

			// respawn ticks are treated as expiration ticks
			if (ph_obj->respawnTicks > 0)
			{
				if ((int)(ph_obj->respawnTicks + ph_obj->creationTicks) < checkTicks)
					bDestroy = true;
			}
			else if (ph_obj->respawnTicks == -1) // use default value
			{
				int objTicks = *(int*)(m_obj + 0x204);

				if (checkTicks > objTicks)
					bDestroy = true;
			}
		}
		else // we're not managing it, apply default processing
		{
			int objTicks = *(int*)(m_obj + 0x204);

			if (checkTicks > objTicks)
				bDestroy = true;
		}

		return bDestroy;
	}

	// parentId - Object id for this object's parent
	// respawnTime - Number of seconds inactive before it should respawn (0 for no respawn, -1 for gametype value)
	// bRecycle - Specifies whether or not the object should be respawn once respawnTimer has passed (if false it is destroyed)
	// location - Location of the object's respawn point
	// Return value: Created object's id, 0 on failure.
	// Note: When spawning items with an item collection tag (weapons, nades
	// powerups) bRecycle is ignored and respawnTime is considered to be
	// destruction time (time until object is destroyed)
	DWORD SpawnObject(const char* tagType, const char* tagName, DWORD parentId, int respawnTime, bool bRecycle, vect3d* location)
	{
		if (!tagType || !tagName)
			return 0;

		hTagHeader* tag = LookupTag(tagType, tagName);

		if (!tag)
			return 0;

		// Build the request data for <halo>.CreateObject
		DWORD mapId = tag->id;
		BYTE query[0x100] = {0}; // i think max ever used is 0x90
		__asm
		{
			pushad
			push parentId
			push mapId
			lea eax, dword ptr ds:[query]
			call DWORD PTR ds:[FUNC_CREATEOBJECTQUERY]
			add esp, 8
			popad
		}

		// Set the spawn coordinates (if supplied)
		if (location)
		{
			vect3d* query_loc = (vect3d*)(query + 0x18);
			*query_loc = *location;
		}

		// Create the object
		DWORD newobj_id = 0;
		__asm
		{
			pushad
			push 0
			lea eax, dword ptr ds:[query]
			push eax
			call DWORD PTR ds:[FUNC_CREATEOBJECT]
			add esp, 8
			mov newobj_id, eax
			popad
		}

		if (newobj_id == 0xFFFFFFFF) // error
			return 0;

		// resolve the respawn timer
		if (respawnTime == -1) // use gametype's value
		{
			// weapons/equipment don't respawn, they're destroyed and
			// so the default value is handled in the processing func
			std::string tagtype = tagType;
			if (tagtype == "eqip" || tagtype == "weap")
				respawnTime = -1;
			else
				respawnTime = game::GetRespawnTicks();
		}
		else
			respawnTime *= 30;

		ph_objInfo* obj = new ph_objInfo(newobj_id, respawnTime, bRecycle,
			location->x, location->y, location->z);
		if (!obj)
			return 0;

		ph_objList[obj->m_objId] = obj;
		return newobj_id;
	}

	// Forces a player to equip (and change to) the specified weapon.
	bool AssignPlayerWeapon(halo::h_player* player, DWORD m_weaponId)
	{
		bool bSuccess = false;

		// player must have an object and not be in a vehicle
		if (player->m_object && player->m_object->base.vehicleId == -1)
		{
			// make sure they passed a weapon
			PObject* m_object = objects::GetObjectAddress(3, m_weaponId);

			if (m_object)
			{
				hTagHeader* tag = LookupTag(m_object->base.mapId);

				if (tag->tagType == 0x77656170) // weap
				{
					DWORD mask = (player->mem->playerJoinCount << 0x10)|player->memoryId;
					DWORD playerObj = player->mem->m_playerObjectid;

					__asm
					{
						pushad
						push 1
						mov eax, m_weaponId
						mov ecx, playerObj
						call dword ptr ds:[FUNC_PLAYERASSIGNWEAPON] // assign the weapon
						add esp, 4
						mov bSuccess, al
						cmp al, 1
						jnz ASSIGNMENT_FAILED
						mov ecx, mask
						mov edi, m_weaponId
						push -1
						push -1
						push 7
						push 1
						call DWORD PTR ds:[FUNC_NOTIFY_WEAPONPICKUP] // notify clients of the assignment
						add esp, 0x10
ASSIGNMENT_FAILED:
						popad
					}
				}
			}
		}

		return bSuccess;
	}

	// Forces a player into a vehicle
	// Seat numbers: 0 (driver) 1 (passenger) 2 (gunner)
	void EnterVehicle(halo::h_player* player, DWORD m_vehicleId, DWORD seat)
	{
		if (player)
		{
			// set interaction info
			player->mem->m_interactionObject = m_vehicleId;
			player->mem->interactionType = 8; // vehicle interaction
			player->mem->interactionSpecifier = (WORD)seat;
			player->force_entered = true; // so scripts cant stop them entering
			DWORD mask = (player->mem->playerJoinCount << 0x10)|player->memoryId;

			// enter the vehicle (if seat is free)
			__asm
			{
				pushad
				push mask
				call dword ptr ds:[FUNC_ENTERVEHICLE]
				add esp, 4
				popad
			}

			player->force_entered = false;
		}
	}

	// Forces a player to exit a vehicle
	void ExitVehicle(halo::h_player* player)
	{
		if (player && player->m_object && player->m_object->base.vehicleId != -1) // exists and in vehicle
		{
			DWORD playerObj = player->mem->m_playerObjectid;
			__asm
			{
				pushad
				mov eax, playerObj
				call dword ptr ds:[FUNC_EJECTVEHICLE]
				popad
			}
		}
	}

	// --------------------------------------------------------------------
	// Called when a new game starts
	void OnGameStart()
	{
		ClearTagCache();
		ClearObjectSpawns();
	}

}
