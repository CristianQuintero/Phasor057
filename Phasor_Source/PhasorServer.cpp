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

#include "../libcurl/curl/curl.h"
#include "Logging.h"
#include "PhasorServer.h"
#include "md5wrapper.h"
#include "Common.h"
#include "Maps.h"
#include "Game.h"
#include "Version.h"
#include "Addresses.h"
#include "Halo.h"
#include "Timers.h"

#pragma comment(lib, "../Release/libcurl.lib")

class urlEncoding
{
private:
	// Stores the encoded url
	std::string encoding;
	bool openPair;

	// Converts data to uri escaped for use in urls
	std::string urlEscape(const wchar_t* szPtr);

	// Helper function for urlEscape
	std::string wstring_to_utf8_hex(const std::wstring &input);

public:
	urlEncoding(std::string domain);

	// Adds a data pair to the query
	void addDataPair(wchar_t* key, const wchar_t* data);

	// Returns the encoded url
	std::string GetEncoding();
};

namespace PhasorServer
{
	// Sends data to the server
	bool send_server_update(LPARAM unused);

	struct server_info
	{
		wchar_t name[256];
		wchar_t map[33];
		wchar_t gametype[64];
		wchar_t port[12];
		wchar_t maxp[12];
		wchar_t curp[12];
		wchar_t host[128];
		
	};
	std::wstring name, map, gametype, port, curp, maxp, hash, host, raw_host;

	// thread ids
	DWORD conThreadId = 0, mainThreadId = 0;
	CURL* curl = 0;
	bool sent = false; // used to send data when esrver loads
	
	// --------------------------------------------------------------------
	// The below fuctions are all called from the PHASOR SERVER THREAD
	// They are either timer callbacks or invoke events
	// 
	// Saves the server info, this is a callback
	bool serverdata_update(LPARAM data)
	{
		if (!data)
			return false;

		server_info* info = (server_info*)data;

		map = info->map;
		gametype = info->gametype;
		port = info->port;
		curp = info->curp;
		maxp = info->maxp;
		name = info->name;
		host = info->host;

		if (!sent) // send data now
		{
			send_server_update(0);
			sent = true;
		}

		delete info;

		return false; // don't keep the timer
	}

	// Invokes a thread safe ban
	void safe_ban(const char* hash)
	{
		int len = strlen(hash) + 1;
		char* data = new char[len];

		if (!data)
			return;

		strcpy_s(data, len, hash);

		ThreadTimer::AddToQueue(mainThreadId, 0, THREAD_SAFE_BAN, (LPARAM)data);		
	}
	
	// --------------------------------------------------------------------
	// The below fuctions are all called from the MAIN SERVER THREAD, as timer
	// callbacks.
	 
	// This is called every 10 seconds to update info sent to my server
	bool get_server_info(LPARAM unused)
	{
		server_info* info = new server_info;

		// this is in the main server thread so can use resources freely
		swprintf_s(info->name, sizeof(info->name)/2, L"%s", (wchar_t*)UlongToPtr(ADDR_SERVERNAME));
		swprintf_s(info->map, sizeof(info->map)/2, L"%s", ToWideChar(game::maps::GetCurrentMap()).c_str());
		swprintf_s(info->gametype, sizeof(info->gametype)/2, L"%s", (wchar_t*)UlongToPtr(ADDR_GAMETYPE));
		swprintf_s(info->port, sizeof(info->port)/2, L"%i", *(WORD*)UlongToPtr(ADDR_PORT));
		
		DWORD resolvedPlayerBase = *(DWORD*)ADDR_PLAYERINFOBASE;
		WORD maxp = 0;
		if (resolvedPlayerBase)
			maxp = *(BYTE*)UlongToPtr(resolvedPlayerBase + OFFSET_MAXPLAYERS);

		swprintf_s(info->maxp, sizeof(info->maxp)/2, L"%i", maxp);
		swprintf_s(info->curp, sizeof(info->curp)/2, L"%i", game::GetPlayerList().size());
		swprintf_s(info->host, sizeof(info->name)/2, L"%s", raw_host.c_str());

		// send to the other thread
		ThreadTimer::AddToQueue(conThreadId, 0, SAVE_SERVER_INFO, (LPARAM)info);

		return true; // keep the timer
	}

	bool thread_safe_ban(LPARAM data)
	{
		char* hash = (char*)data;
		char* player = "phasor_ban";

		halo::BanHash(false, player, hash);

		delete[] hash;

		return false;
	}

		
	// Console command for setting host info
	bool sv_host(halo::h_player* exec, std::vector<std::string>& tokens)
	{
		if (tokens.size() == 2)
		{
			if (tokens[1].size() < 127)
				raw_host = ToWideChar(tokens[1]);
			else
				halo::hprintf("You can only enter 127 characters about yourself.");
		}
		else
			halo::hprintf("Correct usage: sv_host <details about yourself>");

		return true;
	}
	// --------------------------------------------------------------------
	// 
	// Called when data is received from the server
	size_t write_function(char *ptr, size_t size, size_t nmemb, void *s)
	{
		size_t realSize = size*nmemb;

		// The data isn't NULL terminated, do it ourselves
		char* as_string = new char[realSize + 1];
		as_string[realSize] = 0;
		memcpy(as_string, ptr, realSize);

		std::vector<std::string> tokens = TokenizeString(ptr, "\n");

		// response will be in the format
		// ACTION\ndata\nACTION1\ndata1
		// ie: VER\nnew version released blah..\nBAN\nhash1,hash2
		for (size_t x = 0; x < tokens.size(); x += 2)
		{
			// 2 parameters are processed at a time
			if (x >= tokens.size()-1)
				break;

			if (tokens[x] == "BAN")
			{
				std::vector<std::string> bans = TokenizeString(tokens[x+1], ",");

				for (size_t i = 0; i < bans.size(); i++)
					safe_ban(bans[i].c_str());
			}
			else if (tokens[x] == "VER")
			{
				threaded::safe_hprintf("%s", tokens[x+1].c_str());
				threaded::safe_logging(LOGGING_PHASOR, "%s", tokens[x+1].c_str());
			}
			else if (tokens[x] == "ERR")
			{
				//threaded::safe_hprintf("Connection refused: %s", tokens[x+1].c_str());
				threaded::safe_logging(LOGGING_PHASOR, "Connection refused: %s", tokens[x+1].c_str());
			}
		}

		delete[] as_string;

		return size*nmemb;
	}
	
	// Sends data to the server
	bool send_server_update(LPARAM unused)
	{
		// Build the url query
		urlEncoding url("http://phasorstats.appspot.com/usage");
		//urlEncoding url("localhost:8080/usage");
		url.addDataPair(L"ver", PHASOR_BUILD_STRINGW);
		url.addDataPair(L"name", name.c_str());
		url.addDataPair(L"port", port.c_str());
		url.addDataPair(L"curp", curp.c_str());
		url.addDataPair(L"maxp", maxp.c_str());
		url.addDataPair(L"hash", hash.c_str());
		url.addDataPair(L"host", host.c_str());
		url.addDataPair(L"map", map.c_str());
		url.addDataPair(L"gametype", gametype.c_str());
		url.addDataPair(L"type", PHASOR_BUILD_TYPE);

		std::string s_url = url.GetEncoding();

		// Set the url to connect to
		curl_easy_setopt(curl, CURLOPT_URL, s_url.c_str());

		// Function called when data is received from server
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);

		// Send the GET
		curl_easy_perform(curl);

		return true;
	}

	// --------------------------------------------------------------------
	// The auxillary thread calls this function as it initializes
	void Setup()
	{
		// initialize curl
		curl = curl_easy_init();

		if (!curl)
			logging::LogData(LOGGING_PHASOR, L"\t\t - Couldn't initialize cURL. Phasor will not call home.");

		// Build the hardware hash
		HW_PROFILE_INFO hwpi; 
		GetCurrentHwProfile(&hwpi);

		md5wrapper md5;
		std::string hwid1 = md5.getHashFromString(hwpi.szHwProfileGuid);
		hash = ToWideChar(hwid1);

		// Get the thread ids
		conThreadId = threaded::GetAuxillaryThreadId();
		mainThreadId = threaded::GetMainThreadId();

		logging::LogData(LOGGING_PHASOR, L"\t- Registering timer events...");

		// register events for the aux thread
		ThreadTimer::RegisterMsg(conThreadId, SAVE_SERVER_INFO, serverdata_update);
		ThreadTimer::RegisterMsg(conThreadId, SEND_SERVER_INFO, send_server_update);

		// create timer for sending data to my server
		ThreadTimer::AddToQueue(conThreadId, 300*1000, SEND_SERVER_INFO, NULL);

		// ------------------------------------------------
		// register events for main thread

		ThreadTimer::RegisterMsg(mainThreadId, GET_SERVER_INFO, PhasorServer::get_server_info);
		ThreadTimer::RegisterMsg(mainThreadId, THREAD_SAFE_BAN, PhasorServer::thread_safe_ban);

		// create timer for saving server info
		ThreadTimer::AddToQueue(mainThreadId, 10*1000, GET_SERVER_INFO, NULL);
	}

	// The auxillary thread calls this function as it terminates
	void Cleanup()
	{
		if (curl)
			curl_easy_cleanup(curl);
	}	
}

// ------------------------------------------------------------------------
// URL ENCODING
// 
urlEncoding::urlEncoding(std::string domain)
{
	encoding = domain + "?";
	openPair = false;
}

// This function is used to convert non-ascii (>127) to their uri representations
std::string urlEncoding::wstring_to_utf8_hex(const std::wstring &input)
{
	// http://stackoverflow.com/questions/3300025/how-do-i-html-url-encode-a-stdwstring-containing-unicode-characters
	std::string output;
	int cbNeeded = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, NULL, 0, NULL, NULL);
	if (cbNeeded > 0) 
	{
		char *utf8 = new char[cbNeeded];
		if (WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, utf8, cbNeeded, NULL, NULL) != 0)
		{
			for (char *p = utf8; *p; *p++)
			{
				char onehex[5];
				sprintf_s(onehex, sizeof(onehex), "%%%02.2X", (unsigned char)*p);
				output.append(onehex);
			}
		}

		delete[] utf8;
	}
	return output;
}

// Converts data to uri escaped for use in urls
std::string urlEncoding::urlEscape(const wchar_t* szPtr)
{
	std::string escaped;

	for (size_t x = 0; x < wcslen(szPtr); x++)
	{
		wchar_t c = szPtr[x];
		// check if we need to escape it, to make life easy anything that isnt below is escaped
		if (!(c >= L'A' && c <= L'Z') && !(c >= L'a' && c <= L'z') && !(c >= L'0' && c <= L'9') && c != L'.')
		{
			escaped += wstring_to_utf8_hex(std::wstring(&c));			
		}
		else
			escaped += (char)c;
	}

	return escaped;
}

// Returns the encoded url
std::string urlEncoding::GetEncoding()
{
	return encoding;
}

// Adds a data pair to the query
void urlEncoding::addDataPair(wchar_t* key, const wchar_t* data)
{
	if (openPair)
		encoding += "&";

	encoding += urlEscape(key) + "=" + urlEscape(data);
	openPair = true;
}
