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

#define PHASOR_BUILD				10057
#define PHASOR_BUILD_STRING			"01.00.10.057"
#define PHASOR_BUILD_STRINGW		L"01.00.10.057"
#ifdef PHASOR_PC
#define PHASOR_BUILD_TYPE			L"PC"
#elif PHASOR_CE
#define PHASOR_BUILD_TYPE			L"CE"
#endif
