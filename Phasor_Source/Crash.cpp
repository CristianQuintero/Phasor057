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

#include "Crash.h"
#include "Common.h"
#include "Halo.h"
#include "main.h"
#include "Addresses.h"
#include "Timers.h"
#include "Hooks.h"
#include "Lua.h"
#include <tlhelp32.h>
#include <Psapi.h>

#pragma comment(lib, "Psapi.lib")

namespace CrashHandler
{
	// Handler for all unhandled exceptions
	LONG WINAPI OnUnhandledException(PEXCEPTION_POINTERS pExceptionInfo);
	bool bPassOn = true;

	DWORD halo_seh_ret = 0;
	__declspec(naked) void HaloExceptionHandler()
	{
		__asm
		{
			pop halo_seh_ret

			pushad
			push 0
			call OnUnhandledException
			popad

			PUSH EBP
			MOV EBP,ESP
			SUB ESP,0x8

			push halo_seh_ret
			ret
		}
	}

	// This installs the exception catches (globally and through hooking Halo's).
	void InstallCatchers()
	{
		SetUnhandledExceptionFilter(OnUnhandledException);

		// Hook Halo's exception handler
		CreateManagedCodeCave(CC_EXCEPTION_HANDLER, 6, HaloExceptionHandler);
	}

	// Notifies that exception hooks should be rerouted to a hard-exit
	// this is to avoid recursion if an exception occurs in main.cpp:OnClose()
	void OnClose()
	{
		bPassOn = false;
	}

	LONG WINAPI OnUnhandledException(PEXCEPTION_POINTERS pExceptionInfo)
	{
		if (!bPassOn) // exception occured while closing, force kill the server
			exit(1);

		DWORD dwThreadId = GetCurrentThreadId();

		RemoveAllManagedBytes();
		Sleep(100); // wait for all codecaves to be cleaned up
		
		FILE* pFile = fopen("Phasor_CrashLog.txt", "a+");

		if (pFile)
		{
			SYSTEMTIME st = {0};		
			GetLocalTime(&st);

			fprintf(pFile, "------------------ [%04i/%02i/%02i %02i:%02i:%02i] ------------------\n", st.wYear, st.wMonth, st.wDay, st.wHour,st.wMinute,st.wSecond);
			fprintf(pFile, "Server crashed. Exception record: %i (thread: %08X)\nAuxillary thread: %08X\nMain thread: %08X\n", pExceptionInfo, dwThreadId, threaded::GetAuxillaryThreadId(), threaded::GetMainThreadId());

			// print whether or not the crash is caused by a script
			if (threaded::GetMainThreadId() == dwThreadId)
				fprintf(pFile, "Crashed in script: %s\n", Lua::IsScriptExecuting()?"true":"false");
			
			if (pExceptionInfo)
			{
				fprintf(pFile, "Exception information:\n");
				DWORD location = pExceptionInfo->ContextRecord->Eip; // where exception happened
				
				// Find the module the exception occured in
				HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
				
				if(hModuleSnap != INVALID_HANDLE_VALUE)
				{
					MODULEENTRY32 me32;
					me32.dwSize = sizeof(MODULEENTRY32);

					if(Module32First(hModuleSnap, &me32)) // don't care about errors at this point
					{
						do
						{
							if ((DWORD)me32.modBaseAddr <= location && 
								(DWORD)me32.modBaseAddr+me32.modBaseSize >= location)
							{
								fprintf(pFile, "Module: %s\nRVA: %08X", me32.szModule, location-(DWORD)me32.modBaseAddr);
								break;
							}
						} while(Module32Next(hModuleSnap, &me32));												
					}

					CloseHandle(hModuleSnap);
				}
				
				fprintf(pFile, "Absolute location: %08x\n", pExceptionInfo->ContextRecord->Eip);
			}

			std::stack<std::string> execution_stack = GetExecutionStack();
			if (!execution_stack.empty())
				fprintf(pFile, "Last codecave: %s\n", execution_stack.top().c_str());

			// read last 10 lines of console output
			fprintf(pFile, "Console text:\n");
			CONSOLE_SCREEN_BUFFER_INFO end = {0};
			HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
			GetConsoleScreenBufferInfo(hConsole, &end);

			int startRow = end.dwCursorPosition.Y-10;
			if (startRow < 0)
				startRow = 0;

			// Loop through each line
			for (int i = startRow; i < end.dwCursorPosition.Y; i++)
			{
				// Read from the start of the row
				COORD dwRow;
				dwRow.Y = i;
				dwRow.X = 0;

				// Buffer to receive the text
				char consoleLine[81] = {0};

				// Read the data and save to file
				DWORD dwRead = 0;
				ReadConsoleOutputCharacter(hConsole, consoleLine, 80, dwRow, &dwRead);
				fprintf(pFile, "\t%s\n", consoleLine);
			}
			
			fprintf(pFile, "\nMemory Usage:\n");

			PROCESS_MEMORY_COUNTERS memc;
			if (GetProcessMemoryInfo(GetCurrentProcess(), &memc, sizeof(memc)))
			{
				float workingSet = (float)memc.WorkingSetSize / 1024.0f;
				float maxWorkingSet = (float)memc.PeakWorkingSetSize / 1024.0f;
				fprintf(pFile, "Working Set: %.2f kB\nPeak Working Set: %.2f kB", workingSet, maxWorkingSet);
			}
			fprintf(pFile, "\n\n");
			fclose(pFile);
		}
		// cleanup everything
		OnClose();

		exit(1);

		return EXCEPTION_EXECUTE_HANDLER;
	}
}