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
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Logging.h"

// String functions
std::string ToChar(std::wstring str);
std::wstring ToWideChar(std::string str);
void ConvertToLower(char* str);
std::string ExpandTabsToSpace(const char* str);

// Formats a va_list and return a std::string representation
std::string FormatVarArgs(const char* _Format, va_list ArgList);
std::wstring WFormatVarArgs(const wchar_t* _Format, va_list ArgList);

// Dynamic memory allocation versions of below functions
std::wstring m_swprintf_s(const wchar_t* _Format, ...);
std::string m_sprintf_s(const char* _Format, ...);

// Error functions
std::string GetLastErrorAsText();

// File functions
bool LoadFileIntoVector(const char* fileName, std::vector<std::string> &fileData);

// Working directory functions
// ------------------------------------------------------------------------
std::string GetWorkingDirectory();
bool SetupDirectories();

// Tokenization functions
// ------------------------------------------------------------------------
// http://www.gamedev.net/community/forums/topic.asp?topic_id=381544#TokenizeString
std::vector<std::string> TokenizeString(const std::string& str, const std::string& delim);
std::vector<std::wstring> TokenizeWideString(const std::wstring& str, const std::wstring& delim);
std::vector<std::string> TokenizeCommand(const char* str);

// Memory access functions
// ------------------------------------------------------------------------
BOOL WriteBytes(DWORD destAddress, LPVOID patch, DWORD numBytes);
BOOL ReadBytes(DWORD sourceAddress, LPVOID buffer, DWORD numBytes);
void CreateManagedCodeCave(DWORD destAddress, BYTE patchSize, VOID (*function)(VOID));
void RemoveManagedBytes(DWORD destAddr);
void RemoveAllManagedBytes();
std::vector<DWORD> FindSignature(LPBYTE sigBuffer, LPBYTE sigWildCard, DWORD sigSize, LPBYTE pBuffer, DWORD size);