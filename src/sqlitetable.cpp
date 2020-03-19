#include "sqlitetable.h"
#include "sqliterow.h"
#include "sqliteutil.h"
#include "sqlitevariant.h"
#include "angelscript.h"
#include "database.h"

#define RETURNFAIL_IF(a) if(a){return -1;}

sqlite3* SQLiteTable::m_static_pDB = 0;

void SQLiteTable::SetDBConnection(sqlite3* pDB)
{
	m_static_pDB = pDB;
}

sqlite3* SQLiteTable::GetDBConnection()
{
	return m_static_pDB;
}

int RegisterDBTable(sqlite3* sqldb, asIScriptEngine* sengine)
{
	int result = 0;
	result = sengine->RegisterObjectType("DBTable", 0, asOBJ_REF);

	RETURNFAIL_IF(result < 0);
	result = sengine->RegisterObjectBehaviour("DBTable", asBEHAVE_FACTORY, "DBTable@ f(string& in)",
						asFUNCTION(SQLiteTable::Factory), asCALL_CDECL);
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterObjectBehaviour("DBTable", asBEHAVE_ADDREF, "void f()", asMETHOD(SQLiteTable, AddRef),
						asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterObjectBehaviour("DBTable", asBEHAVE_RELEASE, "void f()", asMETHOD(SQLiteTable, Release),
						asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterObjectMethod("DBTable", "DBRow@ MakeRow()",
						asMETHODPR(SQLiteTable, CreateRow, (void), SQLiteRow*), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterEnum("DBColKeyType");
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterEnumValue("DBColKeyType", "COLKEYTYPE_NOTKEY",
					SQLiteColumn::KeyType::KEY_NONE);
	RETURNFAIL_IF(result < 0);
	result = sengine->RegisterEnumValue("DBColKeyType", "COLKEYTYPE_PRIMARYKEY",
					SQLiteColumn::KeyType::KEY_PRIMARY);
	RETURNFAIL_IF(result < 0);
	result = sengine->RegisterEnumValue("DBColKeyType",
					"COLKEYTYPE_AUTOKEY",
					SQLiteColumn::KeyType::KEY_AUTO_PRIMARY);
	RETURNFAIL_IF(result < 0);
	result = sengine->RegisterEnumValue("DBColKeyType", "COLKEYTYPE_FOREIGNKEY",
					SQLiteColumn::KeyType::KEY_FOREIGN);
	RETURNFAIL_IF(result < 0);

	sengine->RegisterObjectMethod("DBTable",
				"void AddIntColumn(const string& in, DBColKeyType keytype = COLKEYTYPE_NOTKEY)",
				asMETHODPR(SQLiteTable, AddIntColumn,
					(const std::string&, SQLiteColumn::KeyType), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	sengine->RegisterObjectMethod("DBTable",
				"void AddRealColumn(const string& in, DBColKeyType keytype= COLKEYTYPE_NOTKEY)",
				asMETHODPR(SQLiteTable, AddRealColumn,
					(const std::string&, SQLiteColumn::KeyType), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	sengine->RegisterObjectMethod("DBTable",
				"void AddTextColumn(const string& in, DBColKeyType keytype= COLKEYTYPE_NOTKEY)",
				asMETHODPR(SQLiteTable, AddTextColumn,
					(const std::string&, SQLiteColumn::KeyType), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);


	sengine->RegisterObjectMethod("DBTable",
				"void AddUUIDColumn(const string& in, DBColKeyType keytype= COLKEYTYPE_NOTKEY)",
				asMETHODPR(SQLiteTable, AddBlobColumn,
					(const std::string&, SQLiteColumn::KeyType), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	return result;
}

SQLiteTable::SQLiteTable(sqlite3* pDB, const char* tablename,
			SQLiteTable* parent_table)
{
	m_refcount = 1;
	m_pDB = pDB;
	m_tablename = tablename;
	m_parent_table = parent_table;
	m_primary_keycol = 0;
	m_foreign_keycol = 0;
}

void SQLiteTable::AddRef()
{
	asAtomicInc(m_refcount);
}

void SQLiteTable::Release()
{
	asAtomicDec(m_refcount);
	if(!m_refcount)
	{
		delete this;
	}
}

SQLiteTable* SQLiteTable::SubTableFactory(const std::string& tablename, SQLiteTable* parent_table)
{
	sqlite3* pdb = GetDBConnection();
	if(pdb)
	{
		return new SQLiteTable(pdb, tablename.c_str(), parent_table);
	}
	else
	{
		return 0;
	}
}

SQLiteTable* SQLiteTable::Factory(const std::string& tablename)
{
	//A parent table is just a sub table with no parent
	return SubTableFactory(tablename.c_str(), 0);
}


SQLiteTable::~SQLiteTable()
{
	printf("Sqlitetable destroyed!");
	size_t idx = 0, len = m_columns.size();
	for(; idx < len; ++idx)
	{
		delete m_columns[idx];
	}
}



void SQLiteTable::AddColumn(const std::string& name, SQLiteVariant::StoredType vartype, SQLiteColumn::KeyType keytype)
{
	m_columns.push_back(new SQLiteColumn(name, vartype, keytype));
	switch(keytype)
	{
	case SQLiteColumn::KeyType::KEY_NONE:
		break;

	case SQLiteColumn::KeyType::KEY_AUTO_PRIMARY:
		//An automatic key in sqlite is just an INTEGER PRIMARY KEY, though it will reuse previously used ROWIDs.
		//Autoincrementing keys will not reuse previously used ROWIDs.

		m_columns.back()->SetColumnType(SQLiteVariant::StoredType::VARINT);

		//We must drop through to the next case!
	case SQLiteColumn::KeyType::KEY_PRIMARY:
		if(!m_primary_keycol)
		{
			//Only use the first provided primary key if the user for some reason is a numbskull
			printf("Setting primary key for column %s\n", name.c_str());
			m_primary_keycol = m_columns.back();
		}
		break;
	case SQLiteColumn::KeyType::KEY_FOREIGN:
		if(!m_foreign_keycol)
		{
			m_foreign_keycol = m_columns.back();
		}
		break;
	default:
		break;
	}
}

std::string SQLiteTable::ProduceUpdateList()
{
	std::string buffer;
	size_t idx = 0, len = m_columns.size();
	for(; idx < len; ++idx)
	{
		if(m_columns[idx]->IsPrimaryKey())
			continue;
		const std::string& propname = m_columns[idx]->GetName();
		buffer += propname + "=$" + propname;
		if(idx < len - 1)
		{
			buffer.append(", ");
		}
	}
	return buffer;
}

std::string SQLiteTable::ProducePlaceholderList()
{
	std::string buffer;
	size_t idx = 0, len = m_columns.size();
	for(; idx < len; ++idx)
	{
		buffer += "$" + m_columns[idx]->GetName();
		if(idx < len - 1)
		{
			buffer.append(", ");
		}
	}
	return buffer;
}

std::string SQLiteTable::ProduceInsertValuesNameList()
{
	std::string buffer;
	size_t idx = 0, len = m_columns.size();
	for(; idx < len; ++idx)
	{
		buffer += m_columns[idx]->GetName();
		if(idx < len - 1)
		{
			buffer.append(", ");
		}
	}
	return buffer;
}

std::string SQLiteTable::ProducePropertyNameList()
{
	std::string buffer;
	size_t idx = 0, len = m_columns.size();

	for(; idx < len; ++idx)
	{
		SQLiteColumn* pcol = m_columns[idx];
		if(pcol->IsForeignKey())
		{
			if(m_parent_table && m_parent_table->m_primary_keycol)
			{
				buffer += pcol->GetName() + " REFERENCES " +
					m_parent_table->m_tablename;
			}
			//If we can't get the information here, the name list will omit a required
			//value and the sqlite statement will fail execution.
		}
		else
		{
			buffer += pcol->GetName() + " " + pcol->GetTypeAsString();
			if(pcol->IsPrimaryKey())
			{
				buffer.append(" PRIMARY KEY");
			}
		}

		if(idx < len - 1)
		{
			buffer.append(", ");
		}
	}

	return buffer;
}


int SQLiteTable::LoadRow(SQLiteRow* pRow)
{
	if(!m_primary_keycol)
	{
		return SQLITE_ERROR;
	}

	int result = DoesSQLiteTableExist(m_pDB, m_tablename.c_str());

	if(result <= 0)
	{
		return SQLITE_ERROR;
	}

	sqlite3_stmt* query = 0;
	std::string selectquerystr = "select " + ProduceInsertValuesNameList() + " from " +
		m_tablename + " where " + m_primary_keycol->GetName() + "=$" + m_primary_keycol->GetName() + ";";
	printf("Query: %s\n", selectquerystr.c_str());
	result = sqlite3_prepare_v2(m_pDB, selectquerystr.c_str(), -1, &query, 0);
	if(SQLITE_OK != result)
	{
		printf("LoadRow prepare statement failure.\n");
		return result;
	}

	if(SQLITE_OK != BindVariantToStatement(query, pRow->GetColumnValue(m_primary_keycol->GetName()), 1))
	{
		printf("Failed to bind key value to sqlite statement.\n");
		sqlite3_finalize(query);
		return result;
	}
	result = sqlite3_step(query);
	if(SQLITE_ROW != result)
	{
		printf("sqlite step had no result: %d\n", result);
		sqlite3_finalize(query);
		return result;
	}

	int columns = sqlite3_column_count(query);
	int idx = 0;
	for(; idx < columns; ++idx)
	{
		switch(sqlite3_column_type(query, idx))
		{
		case SQLITE_INTEGER:
			pRow->SetColumnValue(m_columns[idx]->GetName(), sqlite3_column_int(query, idx));
			break;
		case SQLITE_FLOAT:
			pRow->SetColumnValue(m_columns[idx]->GetName(), sqlite3_column_double(query, idx));
			break;
		case SQLITE_BLOB:
			pRow->SetColumnValue(m_columns[idx]->GetName(),
					(const char*) sqlite3_column_blob(query, idx), sqlite3_column_bytes(query, idx));
			break;
		case SQLITE_NULL:
			break;
		case SQLITE3_TEXT:
		{
			std::string tmp(reinterpret_cast<const char*>(sqlite3_column_text(query, idx)));
			pRow->SetColumnValue(m_columns[idx]->GetName(), tmp);
		}
		break;
		default:
			printf("Unknown?\n");
			break;
		}
	}

	sqlite3_finalize(query);

	return SQLITE_OK;
}

int SQLiteTable::StoreRow(SQLiteRow* pRow, SQLiteRow* pParentRow)
{
	if(!m_primary_keycol)
	{
		printf("No primary key.\n");
		return SQLITE_ERROR;
	}

	int result = DoesSQLiteTableExist(m_pDB, m_tablename.c_str());
	if(0 == result)
	{
		std::string querystr = "CREATE TABLE " + m_tablename + "(" + ProducePropertyNameList() + ");";
		if(SQLITE_DONE != ExecSQLiteStatement(m_pDB, querystr.c_str()))
		{
			printf("Failed to create table. Query was: %s\n", querystr.c_str());
			return SQLITE_ERROR;
		}
		querystr = "CREATE UNIQUE INDEX idx_" + m_tablename + " ON " + m_tablename + " (" + m_primary_keycol->GetName()
			+ ");";
		if(SQLITE_DONE != ExecSQLiteStatement(m_pDB, querystr.c_str()))
		{
			printf("Failed to create table index.\n");
			return SQLITE_ERROR;
		}
	}
	else if(result < 0)
	{
		printf("Error executing sql statement to check table existence.\n");
		return SQLITE_ERROR;
	}

	std::set<std::string> columnset;
	if(SQLITE_OK != GetTableColumns(m_pDB, m_tablename.c_str(), columnset))
	{
		return SQLITE_ERROR;
	}

	for(SQLiteColumn* pcol : m_columns)
	{
		if(columnset.end() == columnset.find(pcol->GetName()))
		{
			printf("Table does not contain column %s\n", pcol->GetName().c_str());
			if(SQLITE_DONE == AddColumnToSQLiteTable(m_pDB, m_tablename.c_str(),
									pcol->GetName().c_str(),
									pcol->GetTypeAsString().c_str()))
			{
				printf("Added column %s successfully\n", pcol->GetName().c_str());
			}
			else
			{
				printf("Failed to add column.\n");
				return SQLITE_ERROR;
			}
		}
	}

	return PerformUpsert(pRow, pParentRow);
}

SQLiteRow* SQLiteTable::CreateRow()
{
	return new SQLiteRow(this);
}

int SQLiteTable::PerformUpsert(SQLiteRow* pRow, SQLiteRow* pParentRow)
{
	std::string insertstr = "INSERT INTO "+ m_tablename;
	insertstr += "(" + ProduceInsertValuesNameList() + ") VALUES(";
	insertstr += ProducePlaceholderList() + ") ON CONFLICT(";
	insertstr += m_primary_keycol->GetName() +") DO UPDATE SET ";
	insertstr += ProduceUpdateList() + ";";

	printf("Insert str: %s\n", insertstr.c_str());
	sqlite3_stmt* query = 0;
	if(SQLITE_OK != sqlite3_prepare_v2(m_pDB, insertstr.c_str(), insertstr.length(),  &query, 0))
	{
		printf("Failed to prepare insert statement\n");
		return SQLITE_ERROR;
	}

	if(SQLITE_OK != sqlite3_bind_text(query, 1, m_tablename.c_str(), m_tablename.length(), 0))
	{
		printf("Failed to bind tablename\n");
		sqlite3_finalize(query);
		return SQLITE_ERROR;
	}

	for(size_t idx = 0, len = m_columns.size(); idx < len; ++idx)
	{
		SQLiteColumn* pcol = m_columns[idx];
		SQLiteVariant* var = pRow->GetColumnValue(pcol->GetName());

		if(pcol->IsForeignKey() && !var)
		{
			if((m_parent_table && m_parent_table->m_primary_keycol)
				&& pParentRow && m_foreign_keycol)
			{
				std::string foreignkeyname = m_foreign_keycol->GetName();
				std::string parentcolvalue;
				pParentRow->GetColumnValue(m_parent_table->m_primary_keycol->GetName(), parentcolvalue);
				printf("Setting column value for foreign keycol %s to %s\n", foreignkeyname.c_str(), parentcolvalue.c_str());
				pRow->SetColumnValue(foreignkeyname, parentcolvalue);
				var = pRow->GetColumnValue(foreignkeyname);
			}
			else
			{
				printf("Table has a foreign key constraint but was not given primary key data from the parent table\n");
				if(!m_parent_table)
					printf("\tDidn't have parent table\n");
				if(!m_parent_table->m_primary_keycol)
					printf("\tParent table didn't have primary keycol set.\n");
				if(!pParentRow)
					printf("\tWasn't given the row from the parent table.\n");
				if(!m_foreign_keycol)
					printf("\tDidn't have a foreign keycol set.\n");
				sqlite3_finalize(query);
				return SQLITE_ERROR;
			}
		}

		if(SQLITE_OK != BindVariantToStatement(query, var, idx + 1))
		{
			sqlite3_finalize(query);
			printf("Failed to bind value %lu for column %s\n", idx, pcol->GetName().c_str());
			return SQLITE_ERROR;
		}
	}

	if(SQLITE_DONE != sqlite3_step(query))
	{
		sqlite3_finalize(query);
		return SQLITE_ERROR;
	}
	sqlite3_finalize(query);
	return SQLITE_OK;
}
