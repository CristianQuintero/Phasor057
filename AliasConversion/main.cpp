// This file converts the alias database used in versions prior to 01.00.10.057
// to an sqlite database compatible with newer versions.
// 
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <map>
#include "resource.h"
#include "Database.h"

// Legacy code
// 
// This should be called when the server starts, it loads the current data
void LoadData(std::string file, std::map<std::string, std::vector<std::wstring>> & AliasMap);
// This function reads data from the specified file
void ProcessPhasorBase(HANDLE hFile, std::map<std::string, std::vector<std::wstring>> & AliasMap);
// ------------------------------------------------------------------------

// Dialog callback
INT_PTR CALLBACK Main_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Dynamic memory allocation version of below function
std::wstring m_swprintf_s(const wchar_t* _Format, ...);

HWND hWnd = 0;
WNDPROC Orginal_EditControlProc = 0; 

//int WINAPI WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd)
int main()
{
	// Create our dialog
	hWnd = CreateDialog(GetModuleHandle(0), MAKEINTRESOURCE(IDD_DIALOG1), NULL, Main_DlgProc);

	// Message loop for the dialog
	MSG Msg = {0};
	while(GetMessage(&Msg, 0, 0, 0) > 0)
	{
		// Not a dialog message
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return 0;
}

INT_PTR CALLBACK SubclassedEdit(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) 
	{
	case WM_DROPFILES:
		{
			HDROP fDrop = (HDROP)wParam;      

			char data[4096] = {0};
			int length = DragQueryFile(fDrop, 0, data, sizeof(data));

			if (length)
				SetWindowText(hwnd, data);
			
			DragFinish(fDrop);

		} break;

	} 

	return CallWindowProc(Orginal_EditControlProc, hwnd, uMsg, wParam, lParam);
}

// Dialog callback
INT_PTR CALLBACK Main_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Handle the message
	switch(uMsg) 
	{
		// Dialog initialize
	case WM_INITDIALOG:
		{	
			// subclass the edit control so we can detect drag 'n drops
			Orginal_EditControlProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hWnd, IDC_LOCATION), GWL_WNDPROC);
			SetWindowLongPtr(GetDlgItem(hWnd, IDC_LOCATION), GWL_WNDPROC, (LONG)SubclassedEdit);

			SetWindowText(GetDlgItem(hWnd, IDC_LOCATION), "Not set");

		}break;

		// Commands
	case WM_COMMAND:
		{
			int button = LOWORD(wParam);
			switch (button)
			{
				// Close the process
			case IDCANCEL:
				{
					PostQuitMessage(0);

				} break;

			case IDOK:
				{
					char szFile[2048] = {0};
					GetWindowTextA(GetDlgItem(hWnd, IDC_LOCATION), szFile, sizeof(szFile)-1);

					std::map<std::string, std::vector<std::wstring>> AliasMap;

					// Load the old file
					LoadData(szFile, AliasMap);

					std::string path = szFile;

					// get the output directory
					if (path.find(".") != std::string::npos)
					{
						// find last slash and end the string there
						size_t pos = path.rfind("\\");

						if (pos != std::string::npos)
						{
							path = path.substr(0, pos);
						}
						else
							path = "";
					}

					if (AliasMap.size()) // process the data
					{
						std::string output = path + "\\alias.sqlite";

						// create the database and add our table
						Database* db = new Database(output.c_str());
						db->query(L"BEGIN TRANSACTION;");  // save data at end (WAY FASTER)
						db->query(L"CREATE TABLE IF NOT EXISTS alias (hash TEXT, name TEXT);");

						// loop through all old alias entries
						std::map<std::string, std::vector<std::wstring>>::iterator itr = AliasMap.begin();

						DWORD cur = 0, count = AliasMap.size();
						// Loop through the entries
						while (itr != AliasMap.end())
						{
							cur++;
							printf("Processing hash %i/%i\n", cur,count);
	
							// build the query
							std::wstring whash;
							whash.assign(itr->first.begin(), itr->first.end());

							// get all names associated with this hash
							std::wstring query = m_swprintf_s(L"SELECT name FROM alias WHERE hash=\"%s\";", whash.c_str());
							std::vector<std::vector<std::wstring>> result = db->query(query.c_str());

							// loop through each name from old file
							for (size_t x = 0;x < itr->second.size(); x++)
							{							
								// check if the name is already in new file
								bool bExists = false;
								for(std::vector<std::vector<std::wstring>>::iterator it = result.begin();
									it < result.end(); it++)
								{
									std::vector<std::wstring> row = *it;

									// should only be one entry in the row
									if (row.size() == 1 && row[0] == itr->second[x])
									{
										bExists = true;
										break;
									}
								}

								// if this name isn't already tracked, add it to the database
								if (!bExists)
								{
									query = m_swprintf_s(L"INSERT INTO 'alias' VALUES(\"%s\", \"%s\");", whash.c_str(), itr->second[x].c_str());
									db->query(query.c_str());
								}
							}
							itr++;
						}

						db->query(L"COMMIT;");

						std::vector<std::vector<std::wstring>> result = db->query(L"select * from alias where name LIKE '[Z%';");

						printf("found %i\n", result.size());
						for(std::vector<std::vector<std::wstring>>::iterator it = result.begin();
							it < result.end(); it++)
						{
							std::vector<std::wstring> row = *it;

							// should only be one entry in the row
							if (row.size() == 2)
							{
								wprintf(L"%s\n", row[1].c_str());
							}
						}

						db->close();
						delete db;

						MessageBox(hWnd, "The database has been converted, you can now use it with newer versions of Phasor.", "Done!", MB_ICONINFORMATION);
					}

				} break;

			}
		} break;

		//-------------------------------------------------------------------------

	default:
		{
			return FALSE;
		}
	}

	// Message handled
	return TRUE;
}

// This is the legacy code, we need it to read the file
// ------------------------------------------------------------------------
#pragma pack(push, 1)
#pragma warning(push)
#pragma warning( disable : 4200 ) // disable warning about data[], it's safe as we only use this struct for casting
struct h_PhasorBaseEntry
{
	DWORD length;
	char key[32];
	BYTE data[];
};
#pragma warning( pop )
#pragma pack(pop)

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

// This function reads data from the specified file
void ProcessPhasorBase(HANDLE hFile, std::map<std::string, std::vector<std::wstring>> & AliasMap)
{
	// Get size of the file
	DWORD dwSize = GetFileSize(hFile, NULL);

	if (dwSize)
	{
		// Try to allocate enough memory for the file
		LPBYTE szBuffer = new BYTE[dwSize+1];

		DWORD dwRead = 0;

		// Read the file
		if (!ReadFile(hFile, szBuffer, dwSize, &dwRead, NULL))
		{
			std::string err = "Couldn't read the alias database, reason: " + GetLastErrorAsText();

			MessageBox(hWnd, err.c_str(), "ProcessPhasorBase", MB_ICONERROR);
			
			return;
		}

		// Process the file
		DWORD dwEntryCount = *(DWORD*)szBuffer;

		LPBYTE ptr = szBuffer+4;

		// Loop through all the data
		for (size_t x = 0; x < dwEntryCount; x++)
		{
			h_PhasorBaseEntry* entry = (h_PhasorBaseEntry*)ptr;

			char localKey[33] = {0};
			memcpy(localKey, entry->key, 32);

			std::vector<std::wstring> names;
			std::map<std::string, std::vector<std::wstring>>::iterator itr = AliasMap.find(localKey);

			if (itr != AliasMap.end())
			{
				// Save its data and then erase the entry
				names = itr->second;

				AliasMap.erase(itr);
			}

			// Loop through all strings within this entry
			size_t i = 0;
			while (i < entry->length)
			{
				wchar_t* name = (wchar_t*)(entry->data + i);
				std::wstring asString = name;

				bool bAdd = true;

				// Before adding to the vector we need to make sure it doesn't already exist
				for (size_t z = 0; z < names.size(); z++)
				{
					if (asString == names[z])
					{
						bAdd = false;
						break;
					}
				}

				if (bAdd)
				{
					// Add the name to the local vector
					names.push_back(name);
				}

				// Increment the counter to the next entry
				i += 2 + wcslen(name)*2;
			}

			// Add this entry to the map
			AliasMap.insert(std::pair<std::string, std::vector<std::wstring>>(localKey, names));

			ptr += entry->length + 36;
		}

		delete[] szBuffer;
	}
	else if (dwSize == INVALID_FILE_SIZE)
	{
		std::string err = "Couldn't determine the size of the alias database, reason: " + GetLastErrorAsText();

		MessageBox(hWnd, err.c_str(), "ProcessPhasorBase", MB_ICONERROR);

		return;
	}
}

// This should be called when the server starts, it loads the current data
void LoadData(std::string file, std::map<std::string, std::vector<std::wstring>> & AliasMap)
{
	// Load the file
	HANDLE hFile = CreateFile(file.c_str(), GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_ALWAYS, NULL, NULL);

	// Check for an error
	if (!hFile)
	{
		// Log the error
		std::string err = "Couldn't open " + file + " reason: " + GetLastErrorAsText();

		MessageBox(hWnd, err.c_str(), "LoadData", MB_ICONERROR);

		return;
	}

	// Process the file
	ProcessPhasorBase(hFile, AliasMap);	

	// Close the file
	CloseHandle(hFile);
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