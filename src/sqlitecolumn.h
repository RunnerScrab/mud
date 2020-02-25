#ifndef SQLITECOLUMN_H_
#define SQLITECOLUMN_H_

#include <cstdio>
#include <cstdlib>
#include <string>

#include "sqlite/sqlite3.h"

#include "sqlitevariant.h"

class SQLiteColumn
{
protected:
	bool m_bPrimaryKey;
	std::string m_property_name;
	SQLiteVariant m_value;
public:
	SQLiteColumn(const std::string& str, SQLiteVariant::StoredType vartype, bool bIsPrimaryKey = false);
	~SQLiteColumn();

	std::string GetPropertyName() const
	{
		return m_property_name;
	}

	std::string GetTypeAsString()
	{
		return m_value.GetTypeAsString();
	}

	bool IsPrimaryKey() const
	{
		return m_bPrimaryKey;
	}

	int BindToSQLiteStatement(sqlite3_stmt* stmt, int pos);

	template<typename T> void GetValue(T& out)
	{
		m_value.GetValue(out);
	}

	void SetValue(int v){m_value.SetValue(v);}
	void SetValue(long long v){m_value.SetValue(v);}
	void SetValue(unsigned int v){m_value.SetValue(v);}
	void SetValue(unsigned long long v){m_value.SetValue(v);}
	void SetValue(float v){m_value.SetValue(v);}
	void SetValue(double v){m_value.SetValue(v);}
	void SetValue(const std::string& v){m_value.SetValue(v);}
	void SetValue(const char* data, size_t len){m_value.SetValue(data, len);}

	void ClearValue(){m_value.ClearValue();}
};


#endif
