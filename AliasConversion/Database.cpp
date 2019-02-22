// Modified version of the code found at:
// http://www.dreamincode.net/forums/topic/122300-sqlite-in-c/
// Changes: query -> utf-16 and bug fix
// 
#include "Database.h"
#include <iostream>
#include <string>
#include <windows.h>

#pragma comment(lib, "../Release/sqlite.lib")

using namespace std;

Database::Database(const char* filename)
{
	database = NULL;
	open(filename);
}

Database::~Database()
{
}

bool Database::open(const char* filename)
{
	if(sqlite3_open(filename, &database) == SQLITE_OK)
		return true;

	return false;   
}

vector<vector<wstring>> Database::query(const wchar_t* query)
{
	sqlite3_stmt *statement;
	vector<vector<wstring> > results;

	if(sqlite3_prepare16_v2(database, (const void*)query, -1, &statement, 0) == SQLITE_OK)
	{
		int cols = sqlite3_column_count(statement);
		int result = 0;
		while(true)
		{
			result = sqlite3_step(statement);

			if(result == SQLITE_ROW)
			{
				vector<wstring> values;
				for(int col = 0; col < cols; col++)
				{
					wstring s;
					wchar_t* ptr = (wchar_t*)sqlite3_column_text16(statement, col);
					if (ptr)
						s = ptr;

					values.push_back(s);
				}
				results.push_back(values);
			}
			else
			{
				break;   
			}
		}

		sqlite3_finalize(statement);
	}

	wstring error = (wchar_t*)sqlite3_errmsg16(database);
	if(error != L"not an error") 
	{
		std::wstring disp = std::wstring(query) + L" " + error;
		MessageBoxW(NULL, disp.c_str(), __FUNCTIONW__, MB_ICONERROR);
	}

	return results;  
}

void Database::close()
{
	sqlite3_close(database);   
}
