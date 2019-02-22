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
#include <sstream>
#include <string>
#include <vector>

#define LOGGING_GAME			1
#define LOGGING_PHASOR			2

namespace halo
{
	class h_player;
};

namespace logging
{
	class cLogging
	{
		std::vector<std::wstring> cache;
		std::string szFilePath, szFileName, oldDirectory;
		DWORD cacheSize;
		DWORD dwMaxSize, dwCurSize;

		// Opens a file and prepares it for use
		void SetupFile(const char* szFile);

	public:

		// Constructor for the class
		cLogging(DWORD size, const char* szFile);
		~cLogging();

		// Sets the cache size
		void SetCacheSize(DWORD size);

		// Sets the log max size
		void SetMaxSize(DWORD size);

		// Sets the file name for the class
		void SetFileName(const char* szFile);

		// Forces the file to be saved and cache cleared
		void SaveFile();

		// Adds data to the file
		void AppendData(const wchar_t* _Format);
	};

	// Creates the logging class
	void Initialize(DWORD type, const char* szPath);

	// Cleans up all the logs etc
	void Cleanup();

	// Deletes a logging class
	void StopLogging(DWORD type);

	// Sets the cache size
	void SetCacheSize(DWORD type, DWORD size);

	// Sets the file name for the class
	void SetName(DWORD type, const char* szFile);

	// Forces the file to be saved and cache cleared
	bool Save(DWORD type);

	// Sets the logs max size
	void SetMaxSize(DWORD type, DWORD dwSize);

	// Adds data to the file
	void LogData(DWORD type, const char* _Format, ...);
	void LogData(DWORD type, const wchar_t* _Format, ...);

	// --------------------------------------------------------------------
	// SERVER COMMANDS FOR LOGGING
	bool sv_disablelog(halo::h_player* execPlayer, std::vector<std::string> & tokens);
	bool sv_logname(halo::h_player* execPlayer, std::vector<std::string> & tokens);
	bool sv_savelog(halo::h_player* execPlayer, std::vector<std::string> & tokens);
	bool sv_loglimit(halo::h_player* execPlayer, std::vector<std::string> & tokens);
	
}