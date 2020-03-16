#include "sqliterow.h"
#include "sqlitetable.h"
#include "angelscript.h"
#include "database.h"
#include "server.h"

#define RETURNFAIL_IF(a) if(a){return -1;}

int RegisterDBRow(sqlite3* sqldb, asIScriptEngine* engine)
{
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

	result = engine->RegisterObjectMethod("DBRow", "void SetColValue(const string& in, uint32 v)",
					asMETHODPR(SQLiteRow, SetColumnValue,
						(const std::string&, unsigned int), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "void SetColValue(const string& in, uint64 v)",
					asMETHODPR(SQLiteRow, SetColumnValue,
						(const std::string&, unsigned long long), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "void SetColValue(const string& in, float v)",
					asMETHODPR(SQLiteRow, SetColumnValue,
						(const std::string&, float), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "void SetColValue(const string& in, double v)",
					asMETHODPR(SQLiteRow, SetColumnValue,
						(const std::string&, double), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "void SetColValue(const string& in, const string& in)",
					asMETHODPR(SQLiteRow, SetColumnValue,
						(const std::string&, const std::string&), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "void SetColValue(const string& in, const uuid& in)",
					asMETHODPR(SQLiteRow, SetColumnValue,
						(const std::string&, const UUID&), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "bool GetColValue(const string& in, int& out)",
					asMETHODPR(SQLiteRow, GetColumnValue,
						(const std::string&, int&), bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "bool GetColValue(const string& in, uint32& out)",
					asMETHODPR(SQLiteRow, GetColumnValue,
						(const std::string&, unsigned int&), bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "bool GetColValue(const string& in, int64& out)",
					asMETHODPR(SQLiteRow, GetColumnValue,
						(const std::string&, long long&), bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "bool GetColValue(const string& in, uint64& out)",
					asMETHODPR(SQLiteRow, GetColumnValue,
						(const std::string&, unsigned long long&), bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "bool GetColValue(const string& in, float& out)",
					asMETHODPR(SQLiteRow, GetColumnValue,
						(const std::string&, float&), bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "bool GetColValue(const string& in, double& out)",
					asMETHODPR(SQLiteRow, GetColumnValue,
						(const std::string&, double&), bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "bool GetColValue(const string& in, string& out)",
					asMETHODPR(SQLiteRow, GetColumnValue,
						(const std::string&, std::string&), bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "bool GetColValue(const string& in, uuid& out)",
					asMETHODPR(SQLiteRow, GetColumnValue,
						(const std::string&, UUID&), bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "bool LoadFromDB()",
					asMETHODPR(SQLiteRow, LoadFromDB,
						(void), bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "bool StoreIntoDB()",
					asMETHODPR(SQLiteRow, StoreIntoDB,
						(void), bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "bool StoreChildRowIntoDB(DBRow@ parent_row)",
					asMETHODPR(SQLiteRow, StoreChildRowIntoDB,
						(SQLiteRow*), bool), asCALL_THISCALL);
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

void SQLiteRow::SetColumnValue(const std::string& colname, const UUID& uuid)
{
	SetColumnValue(colname, uuid.GetData(), uuid.GetDataSize());
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

bool SQLiteRow::GetColumnValue(const std::string& colname, UUID& uuidout)
{
	SQLiteVariant* var = m_valuemap[colname];
	if(var)
	{
		uuidout.CopyFromByteArray(reinterpret_cast<const unsigned char*>(var->GetValueBlobPtr()),
					var->GetDataLength());
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::LoadFromDB()
{
	if(m_table)
	{

		return (m_table->LoadRow(this) != SQLITE_ERROR) ? true : false;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::StoreChildRowIntoDB(SQLiteRow* parent_row)
{
	if(m_table)
	{
		return (m_table->StoreRow(this, parent_row) != SQLITE_ERROR) ? true : false;
	}
	else
	{
		return SQLITE_ERROR;
	}
}

bool SQLiteRow::StoreIntoDB()
{
	return StoreChildRowIntoDB(0);
}
