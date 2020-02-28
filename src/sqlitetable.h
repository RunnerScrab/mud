#ifndef SQLITETABLE_H_
#define SQLITETABLE_H_
#include <vector>
#include <string>
#include "sqlitevariant.h"

class SQLiteRow;

//Contains column name, value type, key type, and may be used to
//bind values to parameterized sqlite statements
class SQLiteColumn
{
public:
  	typedef enum {KEY_NONE = 0, KEY_PRIMARY, KEY_AUTO_PRIMARY, KEY_FOREIGN} KeyType;

private:
	SQLiteVariant::StoredType m_coltype;
	std::string m_name;
	KeyType m_keytype;
public:
	SQLiteColumn(const std::string& name, SQLiteVariant::StoredType vartype, KeyType keytype = KEY_NONE)
	{
		m_name = name;
		m_coltype = vartype;
		m_keytype = keytype;
	}

	SQLiteColumn(SQLiteColumn&& other)
	{
		m_coltype = std::move(other.m_coltype);
		m_name = std::move(other.m_name);
		m_keytype = std::move(other.m_keytype);
	}

	void SetColumnType(SQLiteVariant::StoredType type)
	{
		m_coltype = type;
	}

	SQLiteVariant::StoredType GetType()
	{
		return m_coltype;
	}

	std::string& GetName()
	{
		return m_name;
	}

	const std::string GetTypeAsString()
	{
		return VariantTypeToString(m_coltype);
	}

	KeyType GetKeyType()
	{
		return m_keytype;
	}

	bool IsPrimaryKey()
	{
		return KEY_PRIMARY == m_keytype || KEY_AUTO_PRIMARY == m_keytype;
	}

	bool IsForeignKey()
	{
		return KEY_FOREIGN == m_keytype;
	}

};

//Contains the columns of the table and is responsible for using their
//information to construct and execute SQL statements against the database,
//using row data as input
class SQLiteTable
{
	friend class SQLiteRow;
	std::vector<SQLiteColumn*> m_columns;
	SQLiteColumn* m_primary_keycol, *m_foreign_keycol;
	SQLiteTable* m_parent_table;
	std::string m_tablename;
	sqlite3* m_pDB;
public:
	SQLiteTable(sqlite3* pDB, const char* tablename, SQLiteTable* parent_table = 0);
	~SQLiteTable();

	void AddColumn(const std::string& name, SQLiteVariant::StoredType vartype,
		       SQLiteColumn::KeyType keytype = SQLiteColumn::KeyType::KEY_NONE);

	int LoadRow(SQLiteRow* row);
	int StoreRow(SQLiteRow* row, SQLiteRow* pParentRow = 0);
private:
	int PerformUpsert(SQLiteRow* row, SQLiteRow* parent_row = 0);
	//The a list of SQLite assignments to all the columns during an upsert (update/insert)
	std::string ProduceUpdateList();
	//The names of all the columns prepended by $, without type declarations
	std::string ProducePlaceholderList();
	//The names of all the columns in a list, without type declarations
	std::string ProduceInsertValuesNameList();
	//The names of all the columns along with type declarations, for CREATE operation
	std::string ProducePropertyNameList();
};

#endif
