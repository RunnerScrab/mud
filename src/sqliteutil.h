#ifndef SQLITEUTIL_H_
#define SQLITEUTIL_H_

#include <set>
#include <string>

struct sqlite3;
class SQLiteVariant;

void SQLEscapeString(std::string& str);
void SQLStripString(std::string& str);
int DoesSQLiteTableExist(sqlite3* pDB, const char* tablename);
int GetTableColumns(sqlite3* pDB, const char* tablename, std::set<std::string>& columnset);
int ExecSQLiteStatement(sqlite3* pDB, const char* createtablequery);
int AddColumnToSQLiteTable(sqlite3* pDB, const char* tablename, const char* colname, const char* coltype);

#endif
