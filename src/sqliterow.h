#ifndef SQLITEROW_H_
#define SQLITEROW_H_

#include <vector>
#include <unordered_map>

#include "sqlitevariant.h"

struct Database;
class SQLiteTable;

//Contains the values of the row
class SQLiteRow
{
public:
	void AddRef();
	void Release();
	static SQLiteRow* Factory(SQLiteTable*);
private:
	std::vector<SQLiteVariant*> m_values;
	std::unordered_map<std::string, SQLiteVariant*> m_valuemap;
	SQLiteTable* m_table;

	int m_refcount;
public:
	SQLiteRow(SQLiteTable* table);
	~SQLiteRow();

	void ClearValues();

	//We can't use template member functions here because we need to provide
	//function pointers to the AngelScript engine to create bindings
	void SetColumnValue(const std::string& colname, const SQLiteVariant* value);
	void SetColumnValue(const std::string& colname, const int v);
	void SetColumnValue(const std::string& colname, const long long v);
	void SetColumnValue(const std::string& colname, const unsigned int v);
	void SetColumnValue(const std::string& colname, const unsigned long long v);
	void SetColumnValue(const std::string& colname, const float v);
	void SetColumnValue(const std::string& colname, const double v);
	void SetColumnValue(const std::string& colname, const std::string& v);
	void SetColumnValue(const std::string& colname, const char* data, const size_t datalen);

	SQLiteVariant* GetColumnValue(const std::string& colname);

	bool GetColumnValue(const std::string& colname, int& out);
	bool GetColumnValue(const std::string& colname, unsigned int& out);
	bool GetColumnValue(const std::string& colname, long long& out);
	bool GetColumnValue(const std::string& colname, unsigned long long& out);
	bool GetColumnValue(const std::string& colname, float& out);
	bool GetColumnValue(const std::string& colname, double& out);
	bool GetColumnValue(const std::string& colname, std::string& out);
	bool GetColumnValue(const std::string& colname, std::vector<char>& out);

	int Load();
	int Store(SQLiteRow* parent_row = 0);
private:
	void InitFromTable(SQLiteTable* table);
};

int RegisterDBRow(struct Database* db);

#endif
