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

#include "Timers.h"
#include "PhasorServer.h"
#include "Alias.h"
#include "Common.h"
#include <list>

namespace threaded
{
	DWORD dwAuxThreadId = 0, dwMainThreadId;

	// Callbacks for thread safe functions (transfer execution from aux thread
	// to the main one)
	bool thread_safe_log(LPARAM data);
	bool thread_safe_print(LPARAM data);

	// returns the thread id of the auxillary thread
	DWORD GetAuxillaryThreadId()
	{
		return dwAuxThreadId; // don't need locking for this as it's only written once
	}

	DWORD GetMainThreadId()
	{
		return dwMainThreadId;
	}

	// Sets up the thread and initialzes all code that uses it
	void Setup()
	{
		dwMainThreadId = GetCurrentThreadId();

		// Create the thread and wait until it's ready
		HANDLE hEvent = CreateEvent(0, TRUE, FALSE, NULL);
		CreateThread(0, 0, Auxillary_Thread, (LPVOID)hEvent, 0, 0);
		WaitForSingleObject(hEvent, INFINITE);
		CloseHandle(hEvent);	
	}

	// This thread is used for slow (or blocking) operations, ie;
	// alias sqlite database requests and phasor server connection
	DWORD WINAPI Auxillary_Thread(LPVOID lParam)
	{
		dwAuxThreadId = GetCurrentThreadId();

		HANDLE hEvent = (HANDLE)lParam;

		// call initialization events
		ThreadTimer::Initialize();
		PhasorServer::Setup();
		alias::Setup();

		// register the thread safe functions
		ThreadTimer::RegisterMsg(dwMainThreadId, THREAD_SAFE_PRINT, thread_safe_print);
		ThreadTimer::RegisterMsg(dwMainThreadId, THREAD_SAFE_LOG, thread_safe_log);

		// force the msg queue to be created
		MSG msg = {0};
		PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

		// let phasor continue
		SetEvent(hEvent);

		// loop and process all thread events
		while (1)
		{
			ThreadTimer::ProcessQueue();
			Sleep(300);
		}

		// cleanup
		ThreadTimer::Cleanup(); // needs to be first so other funcs no they wont get calls
		PhasorServer::Cleanup();	
		alias::Cleanup();

		return 0;
	}

	// Methods for invoking thread-safe functions, called from auxillary thread
	// and processed in main thread
	// used for thread safe console printing
	void safe_hprintf(const char* _Format, ...)
	{
		// Format the data into a string
		va_list ArgList;
		va_start(ArgList, _Format);
		std::string szText = FormatVarArgs(_Format, ArgList);
		va_end(ArgList);

		std::wstring as_wide = ToWideChar(szText);
		safe_hprintf(L"%s", as_wide.c_str());		
	}

	void safe_hprintf(const wchar_t* _Format, ...)
	{
		va_list ArgList;
		va_start(ArgList, _Format);
		std::wstring* msg = new std::wstring;
		*msg = WFormatVarArgs(_Format, ArgList);
		va_end(ArgList);

		// notify main thread
		ThreadTimer::AddToQueue(dwMainThreadId, 0, THREAD_SAFE_PRINT, (LPARAM)msg);	
	}

	struct ts_logging
	{
		DWORD type;
		std::wstring msg;
	};

	// thread safe logging
	void safe_logging(DWORD type, const char* _Format, ...)
	{
		// Convert data to wchar_t and then invoke wide version of func
		va_list ArgList;
		va_start(ArgList, _Format);
		std::string szText = FormatVarArgs(_Format, ArgList);
		va_end(ArgList);

		std::wstring as_wide = ToWideChar(szText);
		safe_logging(type, L"%s", as_wide.c_str());	
	}

	void safe_logging(DWORD type, const wchar_t* _Format, ...)
	{
		// store the data
		ts_logging* data = new ts_logging;
		data->type = type;

		// format string
		va_list ArgList;
		va_start(ArgList, _Format);
		data->msg = WFormatVarArgs(_Format, ArgList);
		va_end(ArgList);

		ThreadTimer::AddToQueue(dwMainThreadId, 0, THREAD_SAFE_LOG, (LPARAM)data);	
	}

	// used for printing from server thread
	bool thread_safe_print(LPARAM data)
	{
		std::wstring* msg = (std::wstring*)data;
		halo::hprintf(L"%s", msg->c_str());

		delete msg;

		return false;
	}

	bool thread_safe_log(LPARAM data)
	{
		ts_logging* msg = (ts_logging*)data;
		logging::LogData(msg->type, L"%s", msg->msg.c_str());

		delete msg;

		return false;
	}
}

namespace ThreadTimer
{
	struct timerInfo // used for timers
	{
		DWORD dwEnd;
		DWORD delay;
		DWORD msgId;
		DWORD dwThreadId;
		LPARAM userData;
	};

	struct msgReg // used for registering messages
	{
		DWORD dwThreadId;
		DWORD dwMsg;
		bool (*callback)(LPARAM);
	};

	CRITICAL_SECTION cs;
	std::vector<timerInfo*> eventList;
	std::vector<msgReg*> msgRegister;
	bool initialized = false;

	// Wrapper around GetTickCount for cycling
	bool CheckTimer(DWORD dwEnd, DWORD dwTime)
	{
		bool bExpired = false;

		// Check if the end point has cycled back to 0
		if (dwEnd <= dwTime)
		{
			if (GetTickCount() >= dwEnd)
				bExpired = true;
		}
		else if (dwEnd <= GetTickCount())
			bExpired = true;

		return bExpired;
	}

	void Initialize()
	{
		InitializeCriticalSection(&cs);
		initialized = true;
	}

	void Cleanup()
	{
		if (!initialized)
			return;

		initialized = false; // this will stop all other functions from executing

		EnterCriticalSection(&cs);

		// Free the structures that were allocated
		for (std::vector<msgReg*>::iterator itr = msgRegister.begin(); 
			itr != msgRegister.end(); itr++)
		{
			delete (*itr);
		}

		for (std::vector<timerInfo*>::iterator itr = eventList.begin(); 
			itr != eventList.end(); itr++)
		{
			delete (*itr);
		}

		msgRegister.clear();
		eventList.clear();

		LeaveCriticalSection(&cs);

		DeleteCriticalSection(&cs);
	}

	// Associates a message id with a function
	void RegisterMsg(DWORD dwThreadId, DWORD msgId, bool (*callback)(LPARAM) )
	{
		if (!initialized)
			return;		

		msgReg* reg = new msgReg;
		reg->callback = callback;
		reg->dwMsg = msgId;
		reg->dwThreadId = dwThreadId;

		// add to the register
		EnterCriticalSection(&cs);
		msgRegister.push_back(reg);
		LeaveCriticalSection(&cs);
	}

	// Adds a timer to the queue
	void AddToQueue(DWORD dwThreadId, DWORD delay, DWORD msgId, LPARAM userData)
	{
		if (!initialized)
			return;

		timerInfo* new_event = new timerInfo;
		new_event->dwEnd = GetTickCount() + delay;
		new_event->delay = delay;
		new_event->msgId = msgId;
		new_event->dwThreadId = dwThreadId;
		new_event->userData = userData;

		// Lock the resource
		EnterCriticalSection(&cs);

		eventList.push_back(new_event);

		LeaveCriticalSection(&cs);
	}

	void DispatchQueue()
	{
		if (!initialized)
			return;

		DWORD dwThreadId = GetCurrentThreadId();

		// process all available messages
		MSG Msg = {0};
		while(PeekMessage(&Msg, 0, 0, 0, PM_REMOVE) > 0)
		{		
			// lookup the event to see if it's been registered
			timerInfo* timer = (timerInfo*)Msg.wParam;
			LPARAM userData = (LPARAM)Msg.lParam;
			bool bRepeat = false;

			if (!timer) // timer should always be set
				continue;

			bool processed = false;

			// lock the register vector
			EnterCriticalSection(&cs);

			for (std::vector<msgReg*>::iterator itr = msgRegister.begin(); 
				itr != msgRegister.end(); itr++)
			{
				// make sure only the messages from this queue a processed, the
				// check probably isn't necessary but it makes the logic more
				// implicit
				if (dwThreadId == (*itr)->dwThreadId && (*itr)->dwMsg == Msg.message)
				{				
					// don't want the callback to ever halt other threads
					LeaveCriticalSection(&cs);

					// process timer
					bRepeat = (*itr)->callback(userData);
					processed = true;
					break;
				}
			}

			// unlock it
			if (!processed)
				LeaveCriticalSection(&cs);
			else  // only continue if it's a phasor registered event
			{
				if (bRepeat)
					AddToQueue(dwThreadId, timer->delay, timer->msgId, timer->userData);

				// cleanup
				delete timer;
			}
		}
	}

	// Process current timers
	void ProcessQueue()
	{
		if (!initialized)
			return;

		EnterCriticalSection(&cs);

		std::vector<timerInfo*>::iterator itr = eventList.begin();

		// loop through all events and see if any are ready to be processed,
		// if so send the the calling thread's message queue
		while (itr != eventList.end())
		{
			if (CheckTimer((*itr)->dwEnd, (*itr)->delay))
			{
				// send event to thread
				PostThreadMessage((*itr)->dwThreadId, (*itr)->msgId, (WPARAM)(*itr), (*itr)->userData);

				// the callback needs to free memory of timer object
				itr = eventList.erase(itr);
			}
			else
				itr++;
		}

		LeaveCriticalSection(&cs);

		// process events for this thread
		DispatchQueue();
	}
}

// Basic timer that acts in a single thread, timers can be removed.
namespace Timer
{
	class timer
	{
	private:
		DWORD count, dwEnd, dwDelay; // number of times processed
		LPARAM user_data;
		bool (*completion)(LPARAM);
	public:
		timer(DWORD delay, LPARAM userData, bool (*callback)(LPARAM));

		// checks if the timer has expired
		bool Check();

		// processes a completed timer
		bool Invoke();

		// restarts the timer
		void Reset();
	};

	std::list<timer*> timerList;

	timer::timer(DWORD delay, LPARAM userData, bool (*callback)(LPARAM))
	{
		// save the data
		dwDelay = delay;
		user_data = userData;
		completion = callback;
		dwEnd = GetTickCount() + dwDelay;
	}

	bool timer::Check()
	{
		return ThreadTimer::CheckTimer(dwEnd, dwDelay);
	}

	// processes a completed timer
	bool timer::Invoke()
	{
		return completion(user_data);
	}

	// restarts the timer
	void timer::Reset()
	{
		dwEnd = GetTickCount() + dwDelay;
	}

	// --------------------------------------------------------------------
	// 
	// Creates a new timer
	DWORD CreateTimer(DWORD delay, LPARAM userData, bool (*callback)(LPARAM))
	{
		timer* t = new timer(delay, userData, callback);
		timerList.push_back(t);

		return (DWORD)t;
	}

	// Processes timers and calls completion routines if ready
	void ProcessTimers()
	{
		std::list<timer*>::iterator itr = timerList.begin();
		while (itr != timerList.end())
		{
			bool bInc = true;
			if ((*itr)->Check()) // see if it's ready
			{
				if ((*itr)->Invoke())
				{
					// reset the timer
					(*itr)->Reset();
				}
				else
				{
					delete *itr;
					itr = timerList.erase(itr);
					bInc = false;
				}
			}

			// increment iterator to next timer
			if (bInc)
			{
				itr++;
			}
		}
	}

	// Removes a timer
	void RemoveTimer(DWORD id)
	{
		timer* t = (timer*)id;

		// loop and find the timer
		std::list<timer*>::iterator itr = timerList.begin();
		while (itr != timerList.end())
		{
			if (*itr == t) 
			{
				delete t;
				itr = timerList.erase(itr);
				break;
			}
			else
				itr++;
		}
	}

	void RemoveTimers()
	{
		// loop through and free all timers
		std::list<timer*>::iterator itr = timerList.begin();
		while (itr != timerList.end())
		{
			delete *itr;
			itr = timerList.erase(itr);
		}
	}

}