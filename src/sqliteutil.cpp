#include "sqliteutil.h"
#include "sqlite/sqlite3.h"
#include "sqlitevariant.h"

#include <cstring>

void SQLEscapeString(std::string& str)
{
	std::string::size_type pos = 0;
	for(; pos < str.length() &&
		    (pos = str.find("'", pos)) != std::string::npos;)
	{
		str.replace(pos, 1, "''");
		pos += 2;
	}
}

void SQLStripString(std::string& str)
{
	std::string::size_type pos = 0;
	for(; pos < str.length() &&
		    (pos = str.find("'", pos)) != std::string::npos;)
	{
		str.replace(pos, 1, "");
		++pos;
	}
}

int DoesSQLiteTableExist(sqlite3* pDB, const char* tablename)
{
	sqlite3_stmt* query = 0;
	static const char* checktablequery = "SELECT name FROM sqlite_master WHERE type='table' AND name=$tablename;";
	if(SQLITE_OK != sqlite3_prepare_v2(pDB, checktablequery, -1, &query, 0))
	{
		return -1;
	}
	if(SQLITE_OK != sqlite3_bind_text(query, 1, tablename, strlen(tablename), 0))
	{
		sqlite3_finalize(query);
		return -1;
	}
	int result = sqlite3_step(query) == SQLITE_ROW;

	sqlite3_finalize(query);

	return result;
}

int GetTableColumns(sqlite3* pDB, const char* tablename, std::set<std::string>& columnset)
{
	//Do not call this before verifying the table exists. (pragmas cannot use bound parameters).
	std::string tablenamestr(tablename);
	SQLStripString(tablenamestr);

	std::string columnquerystr = "pragma table_info(" + tablenamestr + ");";
	sqlite3_stmt* columnquery = 0;

	if(SQLITE_OK != sqlite3_prepare_v2(pDB, columnquerystr.c_str(), -1, &columnquery, 0))
	{
		printf("Failed to prepare sql statement\n");
		return -1;
	}

	while(sqlite3_step(columnquery) != SQLITE_DONE)
	{
		std::string row((char*) sqlite3_column_text(columnquery, 1));
		columnset.insert(row);
	}

	sqlite3_finalize(columnquery);
	return SQLITE_OK;
}

int ExecSQLiteStatement(sqlite3* pDB, const char* createtablequery)
{
	sqlite3_stmt* query = 0;
	int result = 0;
	result = sqlite3_prepare_v2(pDB, createtablequery, -1, &query, 0);
	if(SQLITE_OK != result)
	{
		printf("Failed preparing create table sql statement.\n");
		return result;
	}
	result = sqlite3_step(query);
	sqlite3_finalize(query);

	return result;
}

int AddColumnToSQLiteTable(sqlite3* pDB, const char* tablename, const char* colname, const char* coltype)
{
	printf("Trying to add column %s with type %s to table %s\n", colname, coltype, tablename);
	sqlite3_stmt* query = 0;
	int result = 0;

	//SQLite does not support binding parameters to alter table, either.
	//The values here are not user supplied, but are taken from the game scripts
	std::string atqstr = "alter table " + std::string(tablename) + " add column " + std::string(colname) + " " + std::string(coltype);
	SQLStripString(atqstr); //Strip all apostrophes from this string

	result = sqlite3_prepare_v2(pDB, atqstr.c_str(), -1, &query, 0);
	if(SQLITE_OK != result)
	{
		printf("Add Column Prepare failed: %d\n", result);
		return result;
	}

	result = sqlite3_step(query);
	sqlite3_finalize(query);
	return result;
}
