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
#include "MapLoader.h"
#include <Shlobj.h>

#ifdef PHASOR_PC
	std::string gameRes = "\\My Games\\Halo";
#elif PHASOR_CE
	std::string gameRes = "\\My Games\\Halo CE";
#endif

// String functions
std::string ToChar(std::wstring str)
{
	std::string output;
	output.assign(str.begin(), str.end());
	return output;
}

std::wstring ToWideChar(std::string str)
{
	std::wstring output;
	output.assign(str.begin(), str.end());
	return output;
}

void ConvertToLower(char* str)
{
	for (int x = 0; str[x] != 0; x++)
	{
		if (str[x] >= 'A' && str[x] <= 'Z')
			str[x] += 32; // see asci table A - a
	}
}

// Error functions
// Formats the return value of GetLastError into a string
std::string GetLastErrorAsText()
{
	// FormatMessage will allocate this buffer with LocalAlloc
	LPVOID lpMsgBuf = 0;
	DWORD dw = GetLastError(); 

	DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS;
	FormatMessage(dwFlags, NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),	(LPTSTR)&lpMsgBuf, 0, NULL);

	std::string asString;

	if (lpMsgBuf)
	{
		asString = (char*)lpMsgBuf;

		LocalFree(lpMsgBuf);
	}

	return asString;
}

// File functions
bool LoadFileIntoVector(const char* fileName, std::vector<std::string> &fileData)
{
	bool success = false;

	FILE * pFile = fopen(fileName, "a+");
	rewind(pFile);

	if (pFile)
	{
		char line[2048] = {0};

		while (fgets(line, sizeof(line), pFile) != NULL)
			fileData.push_back(line);

		fclose(pFile);
		success = true;
	}

	return success;
}

// Formats a va_list and return a std::string representation
std::string FormatVarArgs(const char* _Format, va_list ArgList)
{
	int count = 1 + _vscprintf(_Format, ArgList);

	char* szText = new char[count];

	if (!szText)
		return "FormatVarArgs - Memory allocation error.";

	memset(szText, 0, sizeof(char)*count);

	_vsnprintf_s(szText, count, _TRUNCATE, _Format, ArgList);
	std::string as_str = szText;

	// Cleanup
	delete[] szText;

	return as_str;
}

// Formats a va_list and return a std::wstring representation
std::wstring WFormatVarArgs(const wchar_t* _Format, va_list ArgList)
{
	// See how much room we'll need
	int count = 1 + _vscwprintf(_Format, ArgList);

	wchar_t* wideMsg = new wchar_t[count];

	if (!wideMsg)
		return L"WFormatVarArgs - Memory allocation error.";
	
	memset(wideMsg, 0, sizeof(wchar_t)*count);

	_vsnwprintf_s(wideMsg, count, _TRUNCATE, _Format, ArgList);
	std::wstring as_str = wideMsg;

	// Cleanup
	delete[] wideMsg;

	return as_str;
}

// Dynamic memory allocation version of below function
std::wstring m_swprintf_s(const wchar_t* _Format, ...)
{
	va_list ArgList;
	va_start(ArgList, _Format);

	// See how much room we'll need
	int count = 1 + _vscwprintf(_Format, ArgList);

	wchar_t* wideMsg = new wchar_t[count];

	if (!wideMsg)
	{
		va_end(ArgList);
		return L"m_swprintf_s - Memory allocation error.";
	}

	memset(wideMsg, 0, sizeof(wchar_t)*count);
	_vsnwprintf_s(wideMsg, count, _TRUNCATE, _Format, ArgList);

	va_end(ArgList);

	std::wstring output = wideMsg;
	delete[] wideMsg;

	return output;
}

// Dynamic memory allocation version of below function
std::string m_sprintf_s(const char* _Format, ...)
{
	va_list ArgList;
	va_start(ArgList, _Format);

	// See how much room we'll need
	int count = 1 + _vscprintf(_Format, ArgList);

	char* msg = new char[count];

	if (!msg)
	{
		va_end(ArgList);
		return "m_sprintf_s - Memory allocation error.";
	}

	memset(msg, 0, sizeof(char)*count);
	_vsnprintf_s(msg, count, _TRUNCATE, _Format, ArgList);
	va_end(ArgList);

	std::string output = msg;
	delete[] msg;

	return output;
}

std::string szWorkingDir;

// Gets the working directory
std::string GetWorkingDirectory()
{
	return szWorkingDir;
}

// Sets the working directory
bool SetupDirectories()
{
	char* cmdline = GetCommandLineA();

	// Tokenize the command line
	if (cmdline)
	{
		std::vector<std::string> tokens = TokenizeCommand(cmdline);

		// Loop through tokens
		for (size_t x = 0; x < tokens.size(); x++)
		{
			if (tokens[x] == "-path")
			{
				// Make sure there is another token
				if (x + 1 < tokens.size())
				{
					// Make sure the directory is valid
					DWORD dwAttributes = GetFileAttributes(tokens[x+1].c_str());

					if (dwAttributes != INVALID_FILE_ATTRIBUTES)
						szWorkingDir = tokens[x+1];
					x++;
				}
			}
			else if (tokens[x] == "-mappath")
			{
				// Make sure there is another token
				if (x + 1 < tokens.size())
				{
					// Make sure the directory is valid
					DWORD dwAttributes = GetFileAttributes(tokens[x+1].c_str());

					if (dwAttributes != INVALID_FILE_ATTRIBUTES)
						game::maps::SetMapPath(tokens[x+1]);

					x++;
				}				
			}
		}
	}

	// Check if the directory was found
	if (szWorkingDir.empty())
	{
		char szOut[MAX_PATH] = {0};

		if (SHGetFolderPath(0, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, szOut) != S_OK)
		{
			FILE* pFile = 0;
			fopen_s(&pFile, "earlyerror.txt", "a+");

			if (pFile)
			{
				fprintf(pFile, "The documents path couldn't be determined. Phasor cannot continue.\n");

				fclose(pFile);
			}

			return false;
		}

		szWorkingDir = szOut;
#ifdef PHASOR_PC
		szWorkingDir += "\\My Games\\Halo";
#elif PHASOR_CE
		szWorkingDir += "\\My Games\\Halo CE";
#endif
	}

	return true;
}

// http://www.gamedev.net/community/forums/topic.asp?topic_id=381544#TokenizeString
std::vector<std::string> TokenizeString(const std::string& str, const std::string& delim)
{
	using namespace std;
	vector<string> tokens;
	size_t p0 = 0, p1 = string::npos;
	while(p0 != string::npos)
	{
		p1 = str.find_first_of(delim, p0);
		if(p1 != p0)
		{
			string token = str.substr(p0, p1 - p0);
			tokens.push_back(token);
		}
		p0 = str.find_first_not_of(delim, p1);
	}
	return tokens;
}

std::vector<std::wstring> TokenizeWideString(const std::wstring& str, const std::wstring& delim)
{
	using namespace std;
	vector<wstring> tokens;
	size_t p0 = 0, p1 = string::npos;
	while(p0 != string::npos)
	{
		p1 = str.find_first_of(delim, p0);
		if(p1 != p0)
		{
			wstring token = str.substr(p0, p1 - p0);
			tokens.push_back(token);
		}
		p0 = str.find_first_not_of(delim, p1);
	}
	return tokens;
}

// Tokenize a string at spaces/quotation blocks
std::vector<std::string> TokenizeCommand(const char* str)
{
	std::vector<std::string> tokens;

	int len = strlen(str);

	if (len)
	{
		bool inQuote = false;
		std::string curToken;

		for (int i = 0; i < len; i++)
		{
			if (str[i] == '"')
			{
				// If there's currently data save it
				if (curToken.size() > 0)
				{
					tokens.push_back(curToken);

					curToken.clear();
				}

				// toggle inQuote
				inQuote = !inQuote;

				continue;
			}

			// If in quote append data regardless of what it is
			if (inQuote)
				curToken += str[i];
			else
			{
				if (str[i] == ' ')
				{
					if (curToken.size() > 0)
					{
						tokens.push_back(curToken);

						curToken.clear();
					}
				}
				else
					curToken += str[i];
			}
		}

		if (curToken.size() > 0)
			tokens.push_back(curToken);
	}

	return tokens;
}

std::string ExpandTabsToSpace(const char* str)
{
	int len = strlen(str);
	std::string output;

	for (int i = 0; i < len; i++)
	{
		// Check if it's a tab
		if (str[i] == '\t')
		{
			// Expand the tab
			int spaceCount = 8 - (output.size() % 8);

			for (int x = 0; x < spaceCount; x++)
				output += ' ';
		}
		else
			output += str[i];
	}

	return output;
}

// Writes bytes in the current process (Made by Drew_Benton).
BOOL WriteBytes(DWORD destAddress, LPVOID patch, DWORD numBytes)
{
	// Store old protection of the memory page
	DWORD oldProtect = 0;

	// Store the source address
	DWORD srcAddress = PtrToUlong(patch);

	// Result of the function
	BOOL result = TRUE;

	// Make sure page is writable
	result = result && VirtualProtect(UlongToPtr(destAddress), numBytes, PAGE_EXECUTE_READWRITE, &oldProtect);

	// Copy over the patch
	memcpy(UlongToPtr(destAddress), patch, numBytes);

	// Restore old page protection
	result = result && VirtualProtect(UlongToPtr(destAddress), numBytes, oldProtect, &oldProtect);

	// Make sure changes are made
	result = result && FlushInstructionCache(GetCurrentProcess(), UlongToPtr(destAddress), numBytes); 

	// Return the result
	return result;
}

// Reads bytes in the current process (Made by Drew_Benton).
BOOL ReadBytes(DWORD sourceAddress, LPVOID buffer, DWORD numBytes)
{
	// Store old protection of the memory page
	DWORD oldProtect = 0;

	// Store the source address
	DWORD dstAddress = PtrToUlong(buffer);

	// Result of the function
	BOOL result = TRUE;

	// Make sure page is writable
	result = result && VirtualProtect(UlongToPtr(sourceAddress), numBytes, PAGE_EXECUTE_READWRITE, &oldProtect);

	// Copy over the patch
	memcpy(buffer, UlongToPtr(sourceAddress), numBytes);

	// Restore old page protection
	result = result && VirtualProtect(UlongToPtr(sourceAddress), numBytes, oldProtect, &oldProtect);

	// Return the result
	return result;
}

// Creates a codecave
BOOL CreateCodeCave(DWORD destAddress, BYTE patchSize, VOID (*function)(VOID))
{
	// Offset to make the codecave at
	DWORD offset = 0;

	// Bytes to write
	BYTE patch[5] = {0};

	// Number of extra nops we need
	BYTE nopCount = 0;

	// NOP buffer
	static BYTE nop[0xFF] = {0};

	// Is the buffer filled?
	static BOOL filled = FALSE;

	// Need at least 5 bytes to be patched
	if(patchSize < 5)
		return FALSE;

	// Calculate the code cave
	offset = (PtrToUlong(function) - destAddress) - 5;

	// Construct the patch to the function call
	patch[0] = 0xE8;
	memcpy(patch + 1, &offset, sizeof(DWORD));
	WriteBytes(destAddress, patch, 5);

	// We are done if we do not have NOPs
	nopCount = patchSize - 5;
	if(nopCount == 0)
		return TRUE;

	// Fill in the buffer
	if(filled == FALSE)
	{
		memset(nop, 0x90, 0xFF);
		filled = TRUE;
	}

	// Make the patch now
	WriteBytes(destAddress + 5, nop, nopCount);

	// Success
	return TRUE;
}

std::vector<DWORD> FindSignature(LPBYTE sigBuffer, LPBYTE sigWildCard, DWORD sigSize, LPBYTE pBuffer, DWORD size)
{
	std::vector<DWORD> results;
	for(DWORD index = 0; index < size; ++index)
	{
		bool found = true;
		for(DWORD sindex = 0; sindex < sigSize; ++sindex)
		{
			// Make sure we don't overrun the buffer!
			if(sindex + index >= size)
			{
				found = false;
				break;
			}

			if(sigWildCard != 0)
			{
				if(pBuffer[index + sindex] != sigBuffer[sindex] && sigWildCard[sindex] == 0)
				{
					found = false;
					break;
				}
			}
			else
			{
				if(pBuffer[index + sindex] != sigBuffer[sindex])
				{
					found = false;
					break;
				}
			}
		}
		if(found)
		{
			results.push_back(index);
		}
	}
	return results;
}

// Managed wrapper around WriteBytes/CreateManagedCodeCave
// ------------------------------------------------------------------------
struct patchData
{
	DWORD address;
	DWORD size;
	std::vector<BYTE> orig;

	patchData(DWORD addr, DWORD len)
	{
		address = addr;
		size = len;
	}
};

std::vector<patchData*> PatchList, tempPatchList;

// This function adds a patch to the managed list
// This isn't used as i want patches to remain for stability
void WriteManagedBytes(DWORD destAddr, LPVOID patch, DWORD size)
{
	patchData* m_patch = new patchData(destAddr, size);

	// Read the original bytes
	for (size_t i = 0; i < size; i++)
	{
		BYTE orig = 0;
		ReadBytes(destAddr + i, &orig, 1);
		m_patch->orig.push_back(orig);
	}

	// Apply the patch
	if (WriteBytes(destAddr, patch, size))
		PatchList.push_back(m_patch);

}

// This function adds a codecave to the managed list
void CreateManagedCodeCave(DWORD destAddress, BYTE patchSize, VOID (*function)(VOID))
{
	patchData* patch = new patchData(destAddress, patchSize);

	// Read the original bytes
	for (size_t i = 0; i < patchSize; i++)
	{
		BYTE orig = 0;
		ReadBytes(destAddress + i, &orig, 1);
		patch->orig.push_back(orig);
	}

	// Apply the codecave
	if (CreateCodeCave(destAddress, patchSize, function))
		PatchList.push_back(patch);
}

// This function removes a patch from the managed list and writes the original bytes back
void RemoveManagedBytes(DWORD destAddr)
{
	std::vector<patchData*>::iterator itr = PatchList.begin();

	while (itr != PatchList.end())
	{
		if ((*itr)->address == destAddr)
		{
			// Write original bytes
			for (size_t i = 0; i < (*itr)->size; i++)
			{
				BYTE orig = (*itr)->orig.at(i);
				WriteBytes(destAddr + i, &orig, 1);
			}

			// Cleanup
			(*itr)->orig.clear();

			delete (*itr);

			itr = PatchList.erase(itr);

			break;
		}

		itr++;
	}
}

void RemoveAllTempHooks()
{
	std::vector<patchData*>::iterator itr = PatchList.begin();

	while (itr != PatchList.end())
	{
		// Write original bytes
		for (size_t i = 0; i < (*itr)->size; i++)
		{
			BYTE orig = (*itr)->orig.at(i);
			WriteBytes((*itr)->address + i, &orig, 1);
		}

		// Cleanup
		(*itr)->orig.clear();
		delete (*itr);

		itr = tempPatchList.erase(itr);
	}
}

DWORD ccremove_temp = 0;
__declspec(naked) void CodecaveRemoval_Temp()
{
	__asm
	{
		pop ccremove_temp

		pushad
		pushfd
		call RemoveAllTempHooks
		popfd
		popad

		sub ccremove_temp, 5
		push ccremove_temp
		ret
	}
}

// Removes all managed codecaves and inserts a temporary codecave
// The temporary codecave is called when the main ones return, all temporary
// codecaves are then removed and execution is returned to where the temporary
// codecave was called from.
// All managed codecaves should be called from the same thread, which in this case
// they basically are (except for the console hook), but that codecave is only 5
// bytes and so won't use temporary ones.
void RemoveAllManagedBytes()
{
	std::vector<patchData*>::iterator itr = PatchList.begin();

	while (itr != PatchList.end())
	{
		DWORD destAddr = (*itr)->address;

		// Write original bytes
		for (size_t i = 0; i < (*itr)->size; i++)
		{
			BYTE orig = (*itr)->orig.at(i);
			WriteBytes(destAddr + i, &orig, 1);
		}

		if ((*itr)->size != 5) // install temp codecave
		{
			// 5 bytes require for temp cc, install directly after the installed on
			patchData* patch = new patchData((*itr)->address + 5, 5);

			// Read the original bytes
			for (size_t i = 0; i < 5; i++)
			{
				BYTE orig = 0;
				ReadBytes(destAddr + i, &orig, 1);
				patch->orig.push_back(orig);
			}

			// Apply the codecave
			if (CreateCodeCave(destAddr, 5, CodecaveRemoval_Temp))
				tempPatchList.push_back(patch);

		}
		// Cleanup
		(*itr)->orig.clear();

		delete (*itr);

		itr = PatchList.erase(itr);
	}

	PatchList.clear();
}