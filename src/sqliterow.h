#ifndef SQLITEROW_H_
#define SQLITEROW_H_

#include <vector>
#include <unordered_map>

#include "uuid.h"
#include "sqlitevariant.h"

#ifndef TESTING_
struct Database;
class asIScriptEngine;
#endif

class SQLiteTable;
struct sqlite3;
//Contains the values of the row
class SQLiteRow
{
public:
#ifndef TESTING_
	void AddRef();
	void Release();
#endif

	static SQLiteRow* Factory(SQLiteTable *parenttable);
private:
	std::unordered_map<std::string, SQLiteVariant*> m_valuemap;
	SQLiteTable *m_table;

	std::vector<SQLiteRow*> m_childrows;
	int m_refcount;
public:
	SQLiteRow(SQLiteTable *table);
	~SQLiteRow();

	void ClearValues();

	//We can't use template member functions here because we need to provide
	//function pointers to the AngelScript engine to create bindings
	void SetColumnValue(const std::string &colname, const SQLiteVariant *value);
	void SetColumnValue(const std::string &colname, const int v);
	void SetColumnValue(const std::string &colname, const long long v);
	void SetColumnValue(const std::string &colname, const unsigned int v);
	void SetColumnValue(const std::string &colname, const unsigned long long v);
	void SetColumnValue(const std::string &colname, const float v);
	void SetColumnValue(const std::string &colname, const double v);
	void SetColumnValue(const std::string &colname, const std::string &v);
	void SetColumnValue(const std::string &colname, const char *data,
			const size_t datalen);
	void SetColumnValue(const std::string &colname, const UUID &uuid);

	SQLiteVariant* GetColumnValue(const std::string &colname);

	bool GetColumnValue(const std::string &colname, int &out, int defval = 0);
	bool GetColumnValue(const std::string &colname, unsigned int &out,
			unsigned int defval = 0);
	bool GetColumnValue(const std::string &colname, long long &out,
			long long defval = 0);
	bool GetColumnValue(const std::string &colname, unsigned long long &out,
			unsigned long long defval = 0);
	bool GetColumnValue(const std::string &colname, float &out, float defval = 0.f);
	bool GetColumnValue(const std::string &colname, double &out, double defval = 0.0);
	bool GetColumnValue(const std::string &colname, std::string &out, std::string defval = "");
	bool GetColumnValue(const std::string &colname, std::vector<char> &out);
	bool GetColumnValue(const std::string &colname, UUID &uuidout);

	//These load/store functions just wrap SQLiteTable's
	bool LoadFromDB();
	bool StoreIntoDB();
	bool StoreChildRowIntoDB(SQLiteRow *parent_row = 0);

	void StoreChildRow(SQLiteRow *childrow);

	bool StoreAllChildRowsIntoDB()
	{
		for (SQLiteRow *childrow : m_childrows)
		{
			if (!childrow->StoreChildRowIntoDB(this))
			{
				return false;
			}
		}
		return true;
	}

private:
	void InitFromTable(SQLiteTable *table);
};

#ifndef TESTING_
int RegisterDBRow(sqlite3 *sqldb, asIScriptEngine *sengine);
#endif

#endif
