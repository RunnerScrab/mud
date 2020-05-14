#ifndef SQLITETABLE_H_
#define SQLITETABLE_H_
#include <vector>
#include <map>
#include <string>
#include "sqlitevariant.h"

#ifndef TESTING_
class asIScriptEngine;
class CScriptArray;
#endif

struct Database;
class SQLiteRow;
class SQLiteTable;
//Contains column name, value type, key type, and may be used to
//bind values to parameterized sqlite statements
class SQLiteColumn
{
public:
	typedef enum
	{
		KEY_NONE = 0,
		KEY_PRIMARY,
		KEY_AUTO_PRIMARY,
		KEY_FOREIGN,
		KEY_PRIMARY_AND_FOREIGN
	} KeyType;

private:
	SQLiteVariant::StoredType m_coltype;

	//If this column is a foreign key, its name in the foreign table also
	//must be provided since they are not necessarily the same
	std::string m_name, m_foreignkeyname;
	KeyType m_keytype;
	SQLiteTable *m_foreigntable;
public:
	SQLiteColumn(const std::string &name, SQLiteVariant::StoredType vartype,
			KeyType keytype = KEY_NONE, SQLiteTable *foreigntable = 0,
			const std::string &foreignname = "");

	~SQLiteColumn();

	SQLiteColumn(SQLiteColumn &&other);

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

	const std::string& GetForeignName()
	{
		return m_foreignkeyname;
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
		return KEY_FOREIGN == m_keytype || KEY_PRIMARY_AND_FOREIGN == m_keytype;
	}

};

//Contains the columns of the table and is responsible for using their
//information to construct and execute SQL statements against the database,
//using row data as input
class SQLiteTable
{
	friend class SQLiteRow;
public:
	static sqlite3 *m_static_pDB;
	static void SetDBConnection(sqlite3 *pDB);
	static sqlite3* GetDBConnection();
	static Database* m_pDatabaseMetadata;

	static void SetDatabaseMetadataPtr(struct Database* dbd);
	static struct Database* GetDatabaseMetadataPtr();

	sqlite3* GetDBPtr()
	{
		return m_pDB;
	}

#ifndef TESTING_
	void AddRef();
	void Release();
	static SQLiteTable* Factory(const std::string &tablename);
#endif

private:
	std::vector<SQLiteColumn*> m_columns;

	std::map<const std::string, SQLiteTable*> m_subtablemap;

	std::vector<SQLiteColumn*> m_primary_keycols;
	std::vector<SQLiteColumn*> m_foreign_keycols;
	std::string m_tablename;
	std::string m_cachedloadquery, m_cachedstorequery;
	std::string m_cachedsubtableloadquery;

	sqlite3 *m_pDB;
	bool m_bIsSubTable;
#ifndef TESTING_
	int m_refcount;
#endif

	int PerformUpsert(SQLiteRow *row, SQLiteRow *parent_row = 0);
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
	std::string ProduceSelectConditionString();
	std::string ProduceSubTableSelectConditionString();

public:
	SQLiteColumn* GetPrimaryKeyColAt(size_t idx)
	{
		return m_primary_keycols[idx];
	}

	SQLiteColumn* GetForeignKeyColAt(size_t idx)
	{
		return m_foreign_keycols[idx];
	}

	size_t GetPrimaryKeyCount()
	{
		return m_primary_keycols.size();
	}
	size_t GetForeignKeyCount()
	{
		return m_foreign_keycols.size();
	}

	SQLiteTable(sqlite3 *pDB, const char *tablename);
	~SQLiteTable();

	std::string GetName() const
	{
		return m_tablename;
	}

	SQLiteTable* CreateSubTable(const std::string &name);
	SQLiteTable* GetSubTable(const std::string &name);

#ifndef TESTING_
	bool LoadSubTable(SQLiteRow *parent_row, CScriptArray *resultarray);
#else
	bool LoadSubTable(SQLiteRow* parent_row, std::vector<SQLiteRow*>& resultarray);
#endif

	bool AddColumn(const std::string &name, SQLiteVariant::StoredType vartype,
			SQLiteColumn::KeyType keytype = SQLiteColumn::KeyType::KEY_NONE,
			SQLiteTable *foreigntable = 0, const std::string &foreignname = "");

	bool AddIntColumn(const std::string &name, SQLiteColumn::KeyType keytype =
			SQLiteColumn::KeyType::KEY_NONE, SQLiteTable *foreigntable = 0,
			const std::string &foreignname = "")
	{
		return AddColumn(name, SQLiteVariant::StoredType::VARINT, keytype,
				foreigntable, foreignname);
	}

	bool AddRealColumn(const std::string &name, SQLiteColumn::KeyType keytype =
			SQLiteColumn::KeyType::KEY_NONE, SQLiteTable *foreigntable = 0,
			const std::string &foreignname = "")
	{
		return AddColumn(name, SQLiteVariant::StoredType::VARREAL, keytype,
				foreigntable, foreignname);
	}

	bool AddTextColumn(const std::string &name, SQLiteColumn::KeyType keytype =
			SQLiteColumn::KeyType::KEY_NONE, SQLiteTable *foreigntable = 0,
			const std::string &foreignname = "")
	{
		return AddColumn(name, SQLiteVariant::StoredType::VARTEXT, keytype,
				foreigntable, foreignname);
	}

	bool AddBlobColumn(const std::string &name, SQLiteColumn::KeyType keytype =
			SQLiteColumn::KeyType::KEY_NONE, SQLiteTable *foreigntable = 0,
			const std::string &foreignname = "")
	{
		return AddColumn(name, SQLiteVariant::StoredType::VARBLOB, keytype,
				foreigntable, foreignname);
	}

	int LoadRow(SQLiteRow *row);
	int StoreRow(SQLiteRow *row, SQLiteRow *pParentRow = 0);

	SQLiteRow* CreateRow();
};

#ifndef TESTING_
int RegisterDBTable(sqlite3 *sqldb, asIScriptEngine *sengine);
#endif

#endif
