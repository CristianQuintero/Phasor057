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

#include "Game.h"
#include "Halo.h"
#include <string>
#include <vector>

namespace game
{
	namespace maps
	{
#ifdef PHASOR_PC
		// Returns the address of the loading buffer Halo should use
		char* GetLoadingMapBuffer();

		// Generates the map list
		void BuildMapList();

		// Called when a map is being loaded
		bool OnMapLoad(LPBYTE mapData);

		// Called to fix the loaded map name (call when game begins)
		void FixMapName();

		// Updates the data in 'map' to the maps base data
		void UpdateLoadingMap(char* map);

		// This function returns the address of our map table
		DWORD GetMapTable();
#endif
		// This function checks if a map exists
		bool ValidateMap(char* map);

		// Sets the directory where maps are located from
		void SetMapPath(std::string path);
	}
}