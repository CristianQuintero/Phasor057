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

#include "Addresses.h"
#include <stack>

// Returns the execution stack
std::stack<std::string> GetExecutionStack();

namespace halo
{
	// Installs all codecaves and applies all patches
	void InstallHooks();
}