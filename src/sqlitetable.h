#ifndef SQLITETABLE_H_
#define SQLITETABLE_H_
#include <vector>
#include <map>
#include <string>
#include "sqlitevariant.h"

class asIScriptEngine;
class CScriptArray;

class SQLiteRow;
class SQLiteTable;
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
	SQLiteTable* m_foreigntable;
public:
	SQLiteColumn(const std::string& name, SQLiteVariant::StoredType vartype,
		     KeyType keytype = KEY_NONE, SQLiteTable* foreigntable = 0);

	~SQLiteColumn();

	SQLiteColumn(SQLiteColumn&& other);

	SQLiteTable* GetForeignTable()
	{
		return m_foreigntable;
	}

	void SetColumnType(SQLiteVariant::StoredType type)
	{
		m_coltype = type;
	}

	SQLiteVariant::StoredType GetType()
	{
		return m_coltype;
	}

	const std::string& GetName()
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
public:
	static sqlite3* m_static_pDB;
	static void SetDBConnection(sqlite3* pDB);
	static sqlite3* GetDBConnection();

	void AddRef();
	void Release();
	static SQLiteTable* Factory(const std::string& tablename);
private:
	friend class SQLiteRow;
	std::vector<SQLiteColumn*> m_columns;
	//A subtable is simply another table whose foreign key is set to the
	//primary key of the primary table
	std::map<const std::string, SQLiteTable*> m_subtablemap;

	//A table may have multiple foreign keys, however,
	//m_subtableforeign key is the key to the owning table for a table
	//representing an array
	std::vector<SQLiteColumn*> m_primary_keycols;
	std::vector<SQLiteColumn*> m_foreign_keycols;

	SQLiteColumn* m_primary_keycol, *m_subtableforeignkey;
	std::string m_tablename;
	sqlite3* m_pDB;
	bool m_bIsSubTable;
	int m_refcount;
public:

	SQLiteTable(sqlite3* pDB, const char* tablename);
	~SQLiteTable();

	std::string GetName() const
	{
		return m_tablename;
	}

	SQLiteColumn* GetPrimaryKeyCol()
	{
		return m_primary_keycol;
	}

	SQLiteTable* CreateSubTable(const std::string& name);
	SQLiteTable* GetSubTable(const std::string& name);

	bool AddColumn(const std::string& name, SQLiteVariant::StoredType vartype,
		       SQLiteColumn::KeyType keytype = SQLiteColumn::KeyType::KEY_NONE,
		       SQLiteTable* foreigntable = 0);

	bool AddIntColumn(const std::string& name,
			  SQLiteColumn::KeyType keytype = SQLiteColumn::KeyType::KEY_NONE,
			  SQLiteTable* foreigntable = 0)
	{
		return AddColumn(name, SQLiteVariant::StoredType::VARINT, keytype, foreigntable);
	}

	bool AddRealColumn(const std::string& name,
			   SQLiteColumn::KeyType keytype = SQLiteColumn::KeyType::KEY_NONE,
			   SQLiteTable* foreigntable = 0)
	{
		return AddColumn(name, SQLiteVariant::StoredType::VARREAL, keytype, foreigntable);
	}

	bool AddTextColumn(const std::string& name,
			   SQLiteColumn::KeyType keytype = SQLiteColumn::KeyType::KEY_NONE,
			   SQLiteTable* foreigntable = 0)
	{
		return AddColumn(name, SQLiteVariant::StoredType::VARTEXT, keytype, foreigntable);
	}

	bool AddBlobColumn(const std::string& name,
			   SQLiteColumn::KeyType keytype = SQLiteColumn::KeyType::KEY_NONE,
			   SQLiteTable* foreigntable = 0)
	{
		return AddColumn(name, SQLiteVariant::StoredType::VARBLOB, keytype, foreigntable);
	}

	bool LoadSubTable(SQLiteRow* parent_row, CScriptArray* resultarray);
	int LoadRow(SQLiteRow* row);
	int StoreRow(SQLiteRow* row, SQLiteRow* pParentRow = 0);

	SQLiteRow* CreateRow();
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
	std::string GetPrimaryKeyStringList();
	std::string GetForeignKeyStringList();
};

int RegisterDBTable(sqlite3* sqldb, asIScriptEngine* sengine);
#endif
