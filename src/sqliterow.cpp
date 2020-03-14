#include "sqliterow.h"
#include "sqlitetable.h"
#include "angelscript.h"
#include "database.h"
#include "server.h"

#define RETURNFAIL_IF(a) if(a){return -1;}

int RegisterDBRow(struct Database* db)
{
	sqlite3* sqldb = db->pDB;
	asIScriptEngine* engine = (db->pServer) ? db->pServer->as_manager.engine : 0;
	if(!sqldb || !engine)
	{
		return -1;
	}

	int result = 0;
	result = engine->RegisterObjectType("DBRow", 0, asOBJ_REF);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectBehaviour("DBRow", asBEHAVE_ADDREF, "void f()", asMETHOD(SQLiteRow, AddRef),
						asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectBehaviour("DBRow", asBEHAVE_RELEASE, "void f()", asMETHOD(SQLiteRow, Release),
						asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "void SetColValue(const string& in, int v)",
					asMETHODPR(SQLiteRow, SetColumnValue,
						(const std::string&, int), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "void SetColValue(const string& in, int64 v)",
					asMETHODPR(SQLiteRow, SetColumnValue,
						(const std::string&, long long), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);
	return result;
}

void SQLiteRow::InitFromTable(SQLiteTable* table)
{
	for(SQLiteColumn* col : table->m_columns)
	{
		if(SQLiteColumn::KeyType::KEY_NONE != col->GetKeyType())
		{
			//Don't set a default value for a key type so we can tell
			//easily if one was not provided by the user
			continue;
		}

		switch(col->GetType())
		{
		case SQLiteVariant::StoredType::VARNONE:
			break;
		case SQLiteVariant::StoredType::VARINT:
			SetColumnValue(col->GetName(), 0);
			break;
		case SQLiteVariant::StoredType::VARREAL:
			SetColumnValue(col->GetName(), 0.0);
			break;
		case SQLiteVariant::StoredType::VARBLOB:
			SetColumnValue(col->GetName(), 0, 0);
			break;
		case SQLiteVariant::StoredType::VARTEXT:
			SetColumnValue(col->GetName(), std::string(""));
			break;
		}
	}
}

void SQLiteRow::AddRef()
{
	asAtomicInc(m_refcount);
}

void SQLiteRow::Release()
{
	asAtomicDec(m_refcount);
	if(!m_refcount)
	{
		delete this;
	}
}

SQLiteRow* SQLiteRow::Factory(SQLiteTable* table)
{
	return new SQLiteRow(table);
}

SQLiteRow::SQLiteRow(SQLiteTable* table)
{
	m_refcount = 1;
	m_table = table;
	InitFromTable(table);
}

SQLiteRow::~SQLiteRow()
{
	size_t idx = 0, len = m_values.size();
	for(; idx < len; ++idx)
	{
		delete m_values[idx];
	}
}

void SQLiteRow::ClearValues()
{
	for(SQLiteVariant* var : m_values)
	{
		var->ClearValue();
	}
	m_valuemap.clear();
}

void SQLiteRow::SetColumnValue(const std::string& colname, const SQLiteVariant* value)
{
	*(m_valuemap[colname]) = *value;
}

void SQLiteRow::SetColumnValue(const std::string& colname, const int v)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(!var)
	{
		m_values.emplace_back(new SQLiteVariant());
		m_valuemap[colname] = m_values.back();
		var = m_values.back();
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string& colname, const long long v)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(!var)
	{
		m_values.emplace_back(new SQLiteVariant());
		m_valuemap[colname] = m_values.back();
		var = m_values.back();
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string& colname, const unsigned int v)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(!var)
	{
		m_values.emplace_back(new SQLiteVariant());
		m_valuemap[colname] = m_values.back();
		var = m_values.back();
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string& colname, const unsigned long long v)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(!var)
	{
		m_values.emplace_back(new SQLiteVariant());
		m_valuemap[colname] = m_values.back();
		var = m_values.back();
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string& colname, const float v)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(!var)
	{
		m_values.emplace_back(new SQLiteVariant());
		m_valuemap[colname] = m_values.back();
		var = m_values.back();
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string& colname, const double v)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(!var)
	{
		m_values.emplace_back(new SQLiteVariant());
		m_valuemap[colname] = m_values.back();
		var = m_values.back();
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string& colname, const std::string& v)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(!var)
	{
		m_values.emplace_back(new SQLiteVariant());
		m_valuemap[colname] = m_values.back();
		var = m_values.back();
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string& colname, const char* data, const size_t datalen)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(!var)
	{
		m_values.emplace_back(new SQLiteVariant());
		m_valuemap[colname] = m_values.back();
		var = m_values.back();
	}

	var->SetValue(data, datalen);
}

SQLiteVariant* SQLiteRow::GetColumnValue(const std::string& colname)
{
	return m_valuemap[colname];
}

bool SQLiteRow::GetColumnValue(const std::string& colname, int& out)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string& colname, unsigned int& out)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string& colname, long long& out)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string& colname, unsigned long long& out)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string& colname, float& out)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string& colname, double& out)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string& colname, std::string& out)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string& colname, std::vector<char>& out)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

int SQLiteRow::Load()
{
	if(m_table)
	{
		return m_table->LoadRow(this);
	}
	else
	{
		return SQLITE_ERROR;
	}
}

int SQLiteRow::Store(SQLiteRow* parent_row)
{
	if(m_table)
	{
		return m_table->StoreRow(this, parent_row);
	}
	else
	{
		return SQLITE_ERROR;
	}
}
