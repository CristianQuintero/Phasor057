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

#include "Logging.h"
#include "Halo.h"
#include "Common.h"
#include <algorithm>

namespace logging
{
	cLogging* gameLogging = 0, *phasorLogging = 0;

	// Opens a file and prepares it for use
	void cLogging::SetupFile(const char* szFile)
	{
		// Get the phasor directory
		std::string szCurDir = GetWorkingDirectory();

		szFilePath = szCurDir + "\\logs";

		BOOL bCreate = CreateDirectory(szFilePath.c_str(), NULL);

		// Make sure there is a logs directory
		if (bCreate == TRUE || GetLastError() == ERROR_ALREADY_EXISTS)
			szCurDir += "\\logs"; // Only use logs directory if we can create it

		// Build directory for old path
		oldDirectory = szCurDir + "\\old";

		bCreate = CreateDirectory(oldDirectory.c_str(), NULL);

		if (!bCreate && GetLastError() != ERROR_ALREADY_EXISTS)
			oldDirectory = szCurDir;

		szFilePath = szCurDir + "\\" + szFile + ".log";
		szFileName = szFile;

		// Get the current size of the file (if it exists)
		HANDLE hFile = CreateFile(szFilePath.c_str(), GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			dwCurSize = GetFileSize(hFile, NULL);

			CloseHandle(hFile);
		}
		else
			dwCurSize = 0;
	}

	// Constructor for the class
	cLogging::cLogging(DWORD size, const char* szFile)
	{
		SetupFile(szFile);

		cacheSize = size;
		dwMaxSize = 0;
	}

	cLogging::~cLogging()
	{
		SaveFile();
	}

	// Sets the cache size
	void cLogging::SetCacheSize(DWORD size)
	{
		cacheSize = size;

		if (cache.size() >= cacheSize)
			SaveFile();
	}

	// Sets the log max size
	void cLogging::SetMaxSize(DWORD size)
	{
		dwMaxSize = size;
	}

	// Sets the file name for the class
	void cLogging::SetFileName(const char* szFile)
	{
		// Save the current file
		SaveFile();

		SetupFile(szFile);
	}

	// Forces the file to be saved and cache cleared
	void cLogging::SaveFile()
	{
		if (cache.size())
		{
			// Open the file
			FILE* pFile = 0;
			fopen_s(&pFile, szFilePath.c_str(), "a+");

			// Make sure the file was opened correctly
			if (!pFile)
				return;

			for (size_t x = 0; x < cache.size(); x++)
			{
				fwprintf(pFile, cache[x].c_str());
			}

			// Close the file
			fclose(pFile);

			// Clear the cache
			cache.clear();
		}
	}

	// Adds data to the file
	void cLogging::AppendData(const wchar_t* finalMsg)
	{
		cache.push_back(finalMsg);

		// Increment the size tracker
		dwCurSize += 1 + wcslen(finalMsg);

		// Check if we need to make a new file
		if (dwCurSize >= dwMaxSize && dwMaxSize != 0)
		{
			// Get time information
			SYSTEMTIME st = {0};		
			GetLocalTime(&st);

			// Build new name for the current log
			std::string newName = m_sprintf_s("%s\\%s_%02i-%02i-%i_%02i-%02i-%02i.log", oldDirectory.c_str(), szFileName.c_str(), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

			// Append final message to the cache
			cache.push_back(L"-- End of log as the size limit has been reached --");

			// Save the remaining data
			SaveFile();

			// Move the file
			if (!MoveFile(szFilePath.c_str(), newName.c_str()))
			{
				// Append error to cache
				cache.push_back(L"-- Log file continuing as file cannot be moved --");

				halo::hprintf("The log file: %s cannot be moved as such the size will be exceeded.", szFilePath.c_str());

				// Disable size tracking
				dwMaxSize = 0;
			}
			else
				dwCurSize = 0;
		}
		else if (cache.size() >= cacheSize)
			SaveFile();
	}

	// ------------------------------------------------------------------------
	// 

	// Creates all the memory classes etc
	void Initialize(DWORD type, const char* szPath)
	{
		switch (type)
		{
		case LOGGING_GAME:
			{
				if (!gameLogging)
				{
					gameLogging = new cLogging(20, szPath);
				}
			} break;

		case LOGGING_PHASOR:
			{
				if (!phasorLogging)
					phasorLogging = new cLogging(0, szPath);

			} break;
		}
	}

	// Cleans up all the logs etc
	void Cleanup()
	{
		StopLogging(LOGGING_GAME);
		StopLogging(LOGGING_PHASOR);
	}

	// Returns the pointer to the logging class
	cLogging* GetLoggingPtr(DWORD type)
	{
		cLogging* ptr = 0;

		switch (type)
		{
		case LOGGING_GAME:
			{
				ptr = gameLogging;
			} break;

		case LOGGING_PHASOR:
			{
				ptr = phasorLogging;

			} break;
		}

		return ptr;
	}

	// Deletes a logging class
	void StopLogging(DWORD type)
	{
		switch (type)
		{
		case LOGGING_GAME:
			{
				delete gameLogging;
				gameLogging = 0;

			} break;

		case LOGGING_PHASOR:
			{
				delete phasorLogging;
				phasorLogging = 0;
			}
		}
	}

	// Sets the cache size
	void SetCacheSize(DWORD type, DWORD size)
	{
		cLogging* ptr = GetLoggingPtr(type);

		if (ptr)
			ptr->SetCacheSize(size);
	}

	// Sets the file name for the class
	void SetName(DWORD type, const char* szFile)
	{
		cLogging* ptr = GetLoggingPtr(type);

		if (ptr)
			ptr->SetFileName(szFile);
		else
			Initialize(type, szFile);
	}

	// Forces the file to be saved and cache cleared
	bool Save(DWORD type)
	{
		bool bSaved = true;

		cLogging* ptr = GetLoggingPtr(type);

		if (ptr)
			ptr->SaveFile();
		else
			bSaved = false;

		return bSaved;
	}

	// Sets the logs max size
	void SetMaxSize(DWORD type, DWORD dwSize)
	{
		cLogging* ptr = GetLoggingPtr(type);

		if (ptr)
			ptr->SetMaxSize(dwSize);
	}

	// Adds data to the file
	void LogData(DWORD type, const char* _Format, ...)
	{
		if (!GetLoggingPtr(type))
			return;

		// Format the input
		va_list ArgList;
		va_start(ArgList, _Format);
		std::string szText = FormatVarArgs(_Format, ArgList);
		va_end(ArgList);

		// Convert to std::wstring
		std::wstring wideMsg = ToWideChar(szText);
		LogData(type, L"%s", wideMsg.c_str());		
	}

	// Adds data to the file
	void LogData(DWORD type, const wchar_t* _Format, ...)
	{
		cLogging* ptr = GetLoggingPtr(type);

		if (ptr)
		{
			va_list ArgList;
			va_start(ArgList, _Format);

			// Get the formatted data
			std::wstring formatted = WFormatVarArgs(_Format, ArgList);

			va_end(ArgList);

			// Get time
			SYSTEMTIME st = {0};		
			GetLocalTime(&st);

			// format the message
			std::wstring msg = m_swprintf_s(L"%04i/%02i/%02i %02i:%02i:%02i %s\n", st.wYear, st.wMonth, st.wDay, st.wHour,st.wMinute,st.wSecond, formatted.c_str());
			ptr->AppendData(msg.c_str());

		}
	}

	// --------------------------------------------------------------------
	// SERVER COMMANDS FOR LOGGING
	bool sv_disablelog(halo::h_player* execPlayer, std::vector<std::string> & tokens)
	{
		if (tokens.size() == 1)
		{
			logging::StopLogging(LOGGING_GAME);
			halo::hprintf("Game events will no longer be logged.");
		}
		else
			halo::hprintf("Correct usage: sv_disablelog");

		return true;
	}

	bool sv_logname(halo::h_player* execPlayer, std::vector<std::string> & tokens)
	{
		if (tokens.size() == 3)
		{
			std::transform(tokens[1].begin(), tokens[1].end(), tokens[1].begin(), tolower);

			// Find the log we want save/change name
			if (tokens[1] == "game")
			{
				logging::SetName(LOGGING_GAME, tokens[2].c_str());
				halo::hprintf("Game events will be logged into %s", tokens[2].c_str());
			}
			else if (tokens[1] == "phasor")
			{
				logging::SetName(LOGGING_PHASOR, tokens[2].c_str());
				halo::hprintf("Phasor events will be logged into %s", tokens[2].c_str());
			}
			else
				halo::hprintf("Invalid log type. Available types: game, phasor");
		}
		else
			halo::hprintf("Syntax: sv_logname <type> <name>");

		return true;
	}

	bool sv_savelog(halo::h_player* execPlayer, std::vector<std::string> & tokens)
	{
		if (tokens.size() == 1)
		{
			logging::Save(LOGGING_GAME);
			halo::hprintf("All logs have been saved.");
		}

		return true;
	}

	bool sv_loglimit(halo::h_player* execPlayer, std::vector<std::string> & tokens)
	{
		if (tokens.size() == 3)
		{
			std::transform(tokens[1].begin(), tokens[1].end(), tokens[1].begin(), tolower);

			DWORD dwType = 0;
			if (tokens[1] == "game")
				dwType = LOGGING_GAME;
			else if (tokens[1] == "phasor")
				dwType = LOGGING_PHASOR;

			if (dwType)
			{
				// Get the max size (in bytes)
				DWORD dwSize = atoi(tokens[2].c_str()) * 1024;

				if (dwSize > 0)
				{
					// Notify the logging class
					logging::SetMaxSize(dwType, dwSize);

					halo::hprintf("The %s log will be moved once its size exceeds %i kB.", tokens[1].c_str(), dwSize / 1024);
				}
				else
					halo::hprintf("You need to enter a size that's greater than 0.");
			}
			else
				halo::hprintf("You entered an invalid log type. There are only game or phasor logs.");
		}
		else
			halo::hprintf("Syntax: sv_loglimit <type> <size in kB>");

		return true;
	}
}