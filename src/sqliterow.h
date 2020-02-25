#ifndef SQLITEROW_H_
#define SQLITEROW_H_

#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>
#include <set>
#include <map>

#include "sqlitecolumn.h"
#include "sqlite/sqlite3.h"

//SQLiteRow really represents a SQL table schema which can be used to insert
//rows into it

class SQLiteRow
{
	//These store combinations of store operations and collate them into a single row action
	std::vector<SQLiteColumn*> m_operations;
	std::map<std::string, SQLiteColumn*> m_columns;
	sqlite3* m_pDB;
	SQLiteColumn* m_primary_keycol;
	std::string m_tablename;
public:

	SQLiteRow(sqlite3* pDB, const char* tablename);
	~SQLiteRow();

	void ClearValues();
	void AddColumn(SQLiteColumn* op); //Add a column to this SQLite row

	//Can't use a templated function here for these member functions because
	//we need specific function pointers for the AngelScript engine to bind
	//to

	bool SetPropertyValue(const std::string& propname, const int v);
	bool SetPropertyValue(const std::string& propname, const long long v);
	bool SetPropertyValue(const std::string& propname, const unsigned int v);
	bool SetPropertyValue(const std::string& propname, const unsigned long long v);
	bool SetPropertyValue(const std::string& propname, const float v);
	bool SetPropertyValue(const std::string& propname, const double v);
	bool SetPropertyValue(const std::string& propname, const std::string& v);
	bool SetPropertyValue(const std::string& propname, const char* data, const size_t datalen);

	bool GetPropertyValue(const std::string& propname, int& out);
	bool GetPropertyValue(const std::string& propname, unsigned int& out);
	bool GetPropertyValue(const std::string& propname, long long& out);
	bool GetPropertyValue(const std::string& propname, unsigned long long& out);
	bool GetPropertyValue(const std::string& propname, float& out);
	bool GetPropertyValue(const std::string& propname, double& out);

	bool GetPropertyValue(const std::string& propname, std::string& out);
	bool GetPropertyValue(const std::string& propname, std::vector<char>& out);

	//Load a row from the database into the values here. Make sure to set the primary key's value first.
	int LoadRow();

	//Insert or update a row into or in the database from the values set here
	int StoreRow();
private:
	int InsertSQLiteRow();
	std::string ProduceUpdateList(); //The a list of SQLite assignments to all the columns during an upsert (update/insert)
	std::string ProducePlaceholderList(); //The names of all the columns prepended by $, without type declarations
	std::string ProduceInsertValuesNameList(); //The names of all the columns in a list, without type declarations
	std::string ProducePropertyNameList(); //The names of all the columns along with type declarations, for CREATE operation
};


#endif
