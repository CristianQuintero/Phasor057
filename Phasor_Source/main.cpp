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
#define _WIN32_WINNT _WIN32_WINNT_WINXP

#include "Crash.h"
#include "Common.h"
#include "Hooks.h"
#include "Halo.h"
#include "Lua.h"
#include "Game.h"
#include "Timers.h"
#include "PhasorServer.h"
#include "main.h"
#include "Admin.h"
#include "objects.h"

HMODULE s_hDll = 0;

// Entry point for the dll
BOOL WINAPI DllMain(HMODULE hModule, DWORD ulReason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);
	if(ulReason == DLL_PROCESS_ATTACH)
	{
		s_hDll = hModule;

		// Do not notify this DLL of thread based events
		DisableThreadLibraryCalls(hModule);
	}

	return TRUE;
}

// Called when the dll is loaded
extern "C" __declspec(dllexport) void OnLoad()
{
	halo::hprintf(L"44656469636174656420746f206d756d2e2049206d69737320796f752e");
	
	// Get the working directory
	if (!SetupDirectories())
		return; // Phasor cannot continue

	std::string dir = GetWorkingDirectory() + "\\data";

	// force create the data directory
	CreateDirectory(dir.c_str(), NULL);

	// Initialize the logging
	logging::Initialize(LOGGING_PHASOR, "Phasor_Reports");

	logging::LogData(LOGGING_PHASOR, L"---------------------------------");
	logging::LogData(LOGGING_PHASOR, L"Initializing Phasor...");

	logging::LogData(LOGGING_PHASOR, L"\t- Searching for addresses...");
	if (!addrLocations::LocateAddresses())
	{
		halo::hprintf(L"The necessary byte signatures couldn't be located.");
		halo::hprintf(L"The server will continue to load, but Phasor will be inactive.");

		logging::LogData(LOGGING_PHASOR, L"The necessary byte signatures couldn't be located.");
		logging::LogData(LOGGING_PHASOR, L"The server will continue to load, but Phasor will be inactive.");
	
		// Cleanup the logging
		logging::StopLogging(LOGGING_PHASOR);

		return;
	}

	logging::LogData(LOGGING_PHASOR, L"\t- Initializing game logging...");
	logging::Initialize(LOGGING_GAME, "Phasor_GameLog");

	try
	{
		logging::LogData(LOGGING_PHASOR, L"\t- Installing exception hooks..");

		// Install the exception hooks
		CrashHandler::InstallCatchers();

		// seed any further random number generation
		srand(GetTickCount());

		#ifdef PHASOR_PC
			logging::LogData(LOGGING_PHASOR, L"\t- Rerouting map table..");
			game::maps::BuildMapList();
		#endif

		logging::LogData(LOGGING_PHASOR, L"\t- Installing game hooks...");

		// Install all codecaves/write patches
		halo::InstallHooks();

		logging::LogData(LOGGING_PHASOR, L"\t- Initializing admin system...");
		admin::InitializeAdminSystem();

		logging::LogData(LOGGING_PHASOR, L"\t- Creating auxillary thread...");

		// setup the multithreaded interface
		threaded::Setup();
		objects::tp::LoadData();

		logging::LogData(LOGGING_PHASOR, L"Phasor has loaded successfully!");
		logging::LogData(LOGGING_PHASOR, L"---------------------------------");

	}
	catch (std::exception & e)
	{
		logging::LogData(LOGGING_PHASOR, L"Phasor failed to initialize, the server's stability cannot be guaranteed.");	
		logging::LogData(LOGGING_PHASOR, "Error details: %s", e.what());
	}
}

// Called when the server is closed
void OnClose()
{
	RemoveAllManagedBytes();
	Sleep(100);

	halo::hprintf("closing from thread %08X", GetCurrentThreadId());
	halo::hprintf("1");
	ThreadTimer::Cleanup();
	halo::hprintf("2");
	admin::SaveAdmins();
	halo::hprintf("3");
	Lua::CloseAllScripts();
	halo::hprintf("4");
	logging::Cleanup();
	halo::hprintf("done");

}