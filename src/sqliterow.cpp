#include "sqliterow.h"

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

SQLiteRow::SQLiteRow(sqlite3* pDB, const char* tablename, SQLiteRow* parenttablerow) : m_pDB(pDB), m_primary_keycol(0)
{
	m_foreign_keycol = 0;
	m_parenttablerow = parenttablerow;
	m_tablename = tablename;
	SQLStripString(m_tablename);
}

SQLiteRow::~SQLiteRow()
{
	size_t idx = 0, len = m_operations.size();
	for(; idx < len; ++idx)
	{
		delete m_operations[idx];
	}
}

void SQLiteRow::ClearValues()
{
	size_t idx = 0, len = m_operations.size();
	for(; idx < len; ++idx)
	{
		m_operations[idx]->ClearValue();
	}
}

void SQLiteRow::AddColumn(SQLiteColumn* op)
{
	m_operations.emplace_back(op);
	if(op->IsPrimaryKey())
	{
		m_primary_keycol = op;
	}
	else if(op->IsForeignKey())
	{
		m_foreign_keycol = op;
	}
	m_columns[op->GetPropertyName()] = op;
}

bool SQLiteRow::SetPropertyValue(const std::string& propname, const int v)
{
	SQLiteColumn* col = m_columns[propname];
	if(col)
	{
		col->SetValue(v);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::SetPropertyValue(const std::string& propname, const long long v)
{
	SQLiteColumn* col = m_columns[propname];
	if(col)
	{
		col->SetValue(v);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::SetPropertyValue(const std::string& propname, const unsigned int v)
{
	SQLiteColumn* col = m_columns[propname];
	if(col)
	{
		col->SetValue(v);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::SetPropertyValue(const std::string& propname, const unsigned long long v)
{
	SQLiteColumn* col = m_columns[propname];
	if(col)
	{
		col->SetValue(v);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::SetPropertyValue(const std::string& propname, const float v)
{
	SQLiteColumn* col = m_columns[propname];
	if(col)
	{
		col->SetValue(v);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::SetPropertyValue(const std::string& propname, const double v)
{
	SQLiteColumn* col = m_columns[propname];
	if(col)
	{
		col->SetValue(v);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::SetPropertyValue(const std::string& propname, const std::string& v)
{
	SQLiteColumn* col = m_columns[propname];
	if(col)
	{
		col->SetValue(v);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::SetPropertyValue(const std::string& propname, const char* data, const size_t datalen)
{
	SQLiteColumn* col = m_columns[propname];
	if(col)
	{
		col->SetValue(data, datalen);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetPropertyValue(const std::string& propname, int& out)
{
	SQLiteColumn* col = m_columns[propname];
	if(col)
	{
		col->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetPropertyValue(const std::string& propname, unsigned int& out)
{
	SQLiteColumn* col = m_columns[propname];
	if(col)
	{
		col->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetPropertyValue(const std::string& propname, long long& out)
{
	SQLiteColumn* col = m_columns[propname];
	if(col)
	{
		col->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetPropertyValue(const std::string& propname, unsigned long long& out)
{
	SQLiteColumn* col = m_columns[propname];
	if(col)
	{
		col->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetPropertyValue(const std::string& propname, float& out)
{
	SQLiteColumn* col = m_columns[propname];
	if(col)
	{
		col->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetPropertyValue(const std::string& propname, double& out)
{
	SQLiteColumn* col = m_columns[propname];
	if(col)
	{
		col->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetPropertyValue(const std::string& propname, std::string& out)
{
	SQLiteColumn* col = m_columns[propname];
	if(col)
	{
		col->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetPropertyValue(const std::string& propname, std::vector<char>& out)
{
	SQLiteColumn* col = m_columns[propname];
	if(col)
	{
		col->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

int SQLiteRow::LoadRow()
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
		m_tablename + " where " + m_primary_keycol->GetPropertyName() + "=$" + m_primary_keycol->GetPropertyName() + ";";
	printf("Query: %s\n", selectquerystr.c_str());
	result = sqlite3_prepare_v2(m_pDB, selectquerystr.c_str(), -1, &query, 0);
	if(SQLITE_OK != result)
	{
		printf("LoadRow prepare statement failure.\n");
		return result;
	}

	if(SQLITE_OK != m_primary_keycol->BindToSQLiteStatement(query, 1))
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
			m_operations[idx]->SetValue(sqlite3_column_int(query, idx));
			printf("%d\n", sqlite3_column_int(query, idx));
			break;
		case SQLITE_FLOAT:
			printf("%f\n", sqlite3_column_double(query, idx));
			m_operations[idx]->SetValue(sqlite3_column_double(query, idx));
			break;
		case SQLITE_BLOB:
			m_operations[idx]->SetValue((const char*) sqlite3_column_blob(query, idx), sqlite3_column_bytes(query, idx));
			printf("Blob\n");
			break;
		case SQLITE_NULL:
			printf("Null\n");
			break;
		case SQLITE3_TEXT:
		{
			std::string tmp(reinterpret_cast<const char*>(sqlite3_column_text(query, idx)));
			printf("%s\n", tmp.c_str());
			m_operations[idx]->SetValue(tmp);
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

int SQLiteRow::StoreRow()
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
			printf("Failed to create table.\n");
			return SQLITE_ERROR;
		}
		querystr = "CREATE UNIQUE INDEX idx_" + m_tablename + " ON " + m_tablename + " (" + m_primary_keycol->GetPropertyName()
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

	for(SQLiteColumn* pcol : m_operations)
	{
		if(columnset.end() == columnset.find(pcol->GetPropertyName()))
		{
			printf("Table does not contain column %s\n", pcol->GetPropertyName().c_str());
			if(SQLITE_DONE == AddColumnToSQLiteTable(m_pDB, m_tablename.c_str(),
									pcol->GetPropertyName().c_str(),
									pcol->GetTypeAsString().c_str()))
			{
				printf("Added column %s successfully\n", pcol->GetPropertyName().c_str());
			}
			else
			{
				printf("Failed to add column.\n");
				return SQLITE_ERROR;
			}
		}
	}


	return InsertSQLiteRow();
}

int SQLiteRow::InsertSQLiteRow()
{
	std::string insertstr = "INSERT INTO "+m_tablename+"(" + ProduceInsertValuesNameList() + ") VALUES(" +
		ProducePlaceholderList() + ") ON CONFLICT(" + m_primary_keycol->GetPropertyName() +") DO UPDATE SET " + ProduceUpdateList() + ";";
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

	for(size_t idx = 0, len = m_operations.size(); idx < len; ++idx)
	{
		SQLiteColumn* pcol = m_operations[idx];
		if(pcol->IsForeignKey())
		{
			if(m_parenttablerow && m_parenttablerow->m_primary_keycol)
			{
				pcol->CopyOtherColumnValue(*(m_parenttablerow->m_primary_keycol));
			}
			else
			{
				printf("Table has a foreign key constraint but was not given a primary key column from the parent table\n");
				sqlite3_finalize(query);
				return SQLITE_ERROR;
			}
		}
		if(SQLITE_OK !=
			pcol->BindToSQLiteStatement(query, idx + 1))
		{
			sqlite3_finalize(query);
			printf("Failed to bind value %lu\n", idx);
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

std::string SQLiteRow::ProduceUpdateList()
{
	std::string buffer;
	size_t idx = 0, len = m_operations.size();
	for(; idx < len; ++idx)
	{
		if(m_operations[idx]->IsPrimaryKey())
			continue;
		const std::string& propname = m_operations[idx]->GetPropertyName();
		buffer += propname + "=$" + propname;
		if(idx < len - 1)
		{
			buffer.append(", ");
		}
	}
	return buffer;
}

std::string SQLiteRow::ProducePlaceholderList()
{
	std::string buffer;
	size_t idx = 0, len = m_operations.size();
	for(; idx < len; ++idx)
	{
		buffer += "$" + m_operations[idx]->GetPropertyName();
		if(idx < len - 1)
		{
			buffer.append(", ");
		}
	}
	return buffer;
}

std::string SQLiteRow::ProduceInsertValuesNameList()
{
	std::string buffer;
	size_t idx = 0, len = m_operations.size();
	for(; idx < len; ++idx)
	{
		buffer += m_operations[idx]->GetPropertyName();
		if(idx < len - 1)
		{
			buffer.append(", ");
		}
	}
	return buffer;
}

std::string SQLiteRow::ProducePropertyNameList()
{
	std::string buffer;
	size_t idx = 0, len = m_operations.size();

	for(; idx < len; ++idx)
	{
		SQLiteColumn* pcol = m_operations[idx];
		if(pcol->IsForeignKey())
		{
			if(m_parenttablerow && m_parenttablerow->m_primary_keycol)
			{
				buffer += pcol->GetPropertyName() + " REFERENCES " +
					m_parenttablerow->m_tablename;
			}
		}
		else
		{
			buffer += pcol->GetPropertyName() + " " + pcol->GetTypeAsString();
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
	printf("%s\n", buffer.c_str());
	return buffer;
}


void DebugPrintQueryResult(sqlite3_stmt* query)
{
	//TODO: Remove this; was for debugging (obviously)
	int columns = sqlite3_column_count(query);
	int idx = 0;
	for(; idx < columns; ++idx)
	{
		switch(sqlite3_column_type(query, idx))
		{
		case SQLITE_INTEGER:
			printf("%d\n", sqlite3_column_int(query, idx));
			break;
		case SQLITE_FLOAT:
			printf("%f\n", sqlite3_column_double(query, idx));
			break;
		case SQLITE_BLOB:
			printf("Blob\n");
			break;
		case SQLITE_NULL:
			printf("Null\n");
			break;
		case SQLITE3_TEXT:
			printf("%s\n", sqlite3_column_text(query, idx));
			break;
		default:
			printf("Unknown?\n");
			break;
		}
	}
}

/*
  int GetNameFromSQLTable(sqlite3* pDB, struct Int128* keydata)
  {
  sqlite3_stmt* query = 0;
  sqlite3_prepare_v2(pDB, "SELECT age FROM testtable WHERE uuid=$uuid;", -1, &query, 0);
  sqlite3_bind_blob(query, 1, keydata, sizeof(struct Int128), 0);
  int result = sqlite3_step(query);
  DebugPrintQueryResult(query);
  sqlite3_finalize(query);
  return result;
  }
*/
int GetRowFromSQLTable(sqlite3* pDB, const char* uuid)
{
	//TODO: Remove this; it was for debugging
	sqlite3_stmt* query = 0;
	if(SQLITE_OK != sqlite3_prepare_v2(pDB, "SELECT age FROM testtable WHERE uuid=$uuid;", -1, &query, 0))
	{
		printf("statement preparation failure.\n");
		return SQLITE_ERROR;
	}
	//sqlite3_bind_blob(query, 1, keydata, sizeof(struct Int128), 0);
	if(SQLITE_OK != sqlite3_bind_text(query, 1, uuid, strlen(uuid), 0))
	{
		printf("statement bind failure\n");
		return SQLITE_ERROR;
	}
	int result = 0;
	for( ;SQLITE_DONE != (result = sqlite3_step(query));)
		DebugPrintQueryResult(query);
	sqlite3_finalize(query);
	return result;
}
