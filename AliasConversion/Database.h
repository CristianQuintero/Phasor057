// Modified version of the code found at:
// http://www.dreamincode.net/forums/topic/122300-sqlite-in-c/
// Changes: query -> utf-16 and bug fix
// 
#pragma once

#include <string>
#include <vector>
#include "../sqlite/sqlite3.h"

class Database
{
public:
	Database(const char* filename);
	~Database();

	bool open(const char* filename);
	std::vector<std::vector<std::wstring>> query(const wchar_t* query);
	void close();

private:
	sqlite3 *database;
};

