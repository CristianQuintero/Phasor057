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

// event definitions
#define SAVE_SERVER_INFO				WM_USER + 1
#define GET_SERVER_INFO					WM_USER + 2
#define SEND_SERVER_INFO				WM_USER + 3
#define THREAD_SAFE_PRINT				WM_USER + 4
#define THREAD_SAFE_LOG					WM_USER + 5
#define THREAD_SAFE_BAN					WM_USER + 6
#define ALIAS_ADD_DATA					WM_USER + 7
#define ALIAS_SEARCH_DATA				WM_USER + 8
#define ALIAS_SEARCH_HASH				WM_USER + 9

namespace threaded
{
	// This thread is used for slow (or blocking) operations, ie;
	// alias sqlite database requests and phasor server connection
	DWORD WINAPI Auxillary_Thread(LPVOID lParam);

	// returns the thread id of the auxillary thread
	DWORD GetAuxillaryThreadId();
	DWORD GetMainThreadId();

	// Sets up the thread and initialzes all code that uses it
	void Setup();

	// These functions invoke the named routine but from the main server thread
	void safe_logging(DWORD type, const char* _Format, ...);
	void safe_hprintf(const char* _Format, ...);
	void safe_logging(DWORD type, const wchar_t* _Format, ...);
	void safe_hprintf(const wchar_t* _Format, ...);
}

// Timer that interacts between different threads, useful for thread safety.
// Timers can't be removed until completion.
namespace ThreadTimer
{
	void Initialize();
	void Cleanup();

	// Associates a message id with a function
	// The thread is should be that of the thread that will process the callback
	void RegisterMsg(DWORD dwThreadId, DWORD msgId, bool (*callback)(LPARAM));

	// Adds a timer to the queue
	void AddToQueue(DWORD dwThreadId, DWORD delay, DWORD msgId, LPARAM userData);

	void ProcessQueue();
	void DispatchQueue();
}

// Basic timer that acts in a single thread, timers can be removed.
namespace Timer
{
	// Creates a new timer
	DWORD CreateTimer(DWORD delay, LPARAM userData, bool (*callback)(LPARAM));

	// Processes timers and calls completion routines if ready
	void ProcessTimers();

	// Removes a timer
	void RemoveTimer(DWORD id);
	void RemoveTimers();
}