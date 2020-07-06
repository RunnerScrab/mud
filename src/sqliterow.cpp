#include "sqliterow.h"
#include "sqlitetable.h"

#include "angelscript.h"
#include "database.h"
#include "server.h"

#include "editabletext.h"
#include "mpint.h"
#include "mpfloat.h"
#include <vector>

#define RETURNFAIL_IF(a) if(a){return -1;}


int RegisterDBRow(sqlite3 *sqldb, asIScriptEngine *engine)
{
	int result = 0;
	result = engine->RegisterObjectType("DBRow", 0, asOBJ_REF);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectBehaviour("DBRow", asBEHAVE_ADDREF,
			"void f()", asMETHOD(SQLiteRow, AddRef), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectBehaviour("DBRow", asBEHAVE_RELEASE,
			"void f()", asMETHOD(SQLiteRow, Release), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	/* NOTE: DBRow's Factory function must be registered after the DBTable class,
	 * since it has DBTable as an argument */

	result = engine->RegisterObjectMethod("DBRow",
			"void SetColValue(const string& in, int v)",
			asMETHODPR(SQLiteRow, SetColumnValue, (const std::string&, int),
					void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"void SetColValue(const string& in, int64 v)",
			asMETHODPR(SQLiteRow, SetColumnValue,
					(const std::string&, long long), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"void SetColValue(const string& in, uint32 v)",
			asMETHODPR(SQLiteRow, SetColumnValue,
					(const std::string&, unsigned int), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"void SetColValue(const string& in, uint64 v)",
			asMETHODPR(SQLiteRow, SetColumnValue,
					(const std::string&, unsigned long long), void),
			asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"void SetColValue(const string& in, float v)",
			asMETHODPR(SQLiteRow, SetColumnValue, (const std::string&, float),
					void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"void SetColValue(const string& in, double v)",
			asMETHODPR(SQLiteRow, SetColumnValue, (const std::string&, double),
					void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"void SetColValue(const string& in, const string& in)",
			asMETHODPR(SQLiteRow, SetColumnValue,
					(const std::string&, const std::string&), void),
			asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"void SetColValue(const string& in, const EditableText& in)",
			asMETHODPR(SQLiteRow, SetColumnValue,
					(const std::string&, const EditableText&), void),
			asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"void SetColValue(const string& in, const uuid& in)",
			asMETHODPR(SQLiteRow, SetColumnValue,
					(const std::string&, const UUID&), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"void SetColValue(const string& in, const MPInt& in)",
			asMETHODPR(SQLiteRow, SetColumnValue,
					(const std::string&, const MPInt&), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"void SetColValue(const string& in, const MPFloat& in)",
			asMETHODPR(SQLiteRow, SetColumnValue,
					(const std::string&, const MPFloat&), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"bool GetColValue(const string& in, int& out, int dv = 0)",
			asMETHODPR(SQLiteRow, GetColumnValue,
					(const std::string&, int&, int), bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"bool GetColValue(const string& in, uint32& out, uint32 dv = 0)",
			asMETHODPR(SQLiteRow, GetColumnValue,
					(const std::string&, unsigned int&, unsigned int), bool),
			asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"bool GetColValue(const string& in, int64& out, int64 dv = 0)",
			asMETHODPR(SQLiteRow, GetColumnValue,
					(const std::string&, long long&, long long), bool),
			asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result =
			engine->RegisterObjectMethod("DBRow",
					"bool GetColValue(const string& in, uint64& out, uint64 dv = 0)",
					asMETHODPR(SQLiteRow, GetColumnValue,
							(const std::string&, unsigned long long&, unsigned long long),
							bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"bool GetColValue(const string& in, float& out, float dv = 0.f)",
			asMETHODPR(SQLiteRow, GetColumnValue,
					(const std::string&, float&, float), bool),
			asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"bool GetColValue(const string& in, double& out, double dv = 0.0)",
			asMETHODPR(SQLiteRow, GetColumnValue,
					(const std::string&, double&, double), bool),
			asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"bool GetColValue(const string& in, string& out, string dv = \"\")",
			asMETHODPR(SQLiteRow, GetColumnValue,
					(const std::string&, std::string&, std::string), bool),
			asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"bool GetColValue(const string& in, EditableText& out, string dv = \"\")",
			asMETHODPR(SQLiteRow, GetColumnValue,
					(const std::string&, EditableText&, std::string), bool),
			asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"bool GetColValue(const string& in, uuid& out)",
			asMETHODPR(SQLiteRow, GetColumnValue, (const std::string&, UUID&),
					bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"bool GetColValue(const string& in, MPInt& out, int defval = 0)",
					      asMETHODPR(SQLiteRow, GetColumnValue, (const std::string&, MPInt&,
								 int),
					bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow",
			"bool GetColValue(const string& in, MPFloat& out, double defval = 0.0)",
					      asMETHODPR(SQLiteRow, GetColumnValue, (const std::string&, MPFloat&, double),
					bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "void ClearValues()",
			asMETHODPR(SQLiteRow, ClearValues, (void), void), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "bool LoadFromDB()",
			asMETHODPR(SQLiteRow, LoadFromDB, (void), bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	result = engine->RegisterObjectMethod("DBRow", "bool StoreIntoDB()",
			asMETHODPR(SQLiteRow, StoreIntoDB, (void), bool), asCALL_THISCALL);
	RETURNFAIL_IF(result < 0);

	/*	result = engine->RegisterObjectMethod("DBRow", "bool StoreChildRowIntoDB(DBRow@ parent_row)",
	 asMETHODPR(SQLiteRow, StoreChildRowIntoDB,
	 (SQLiteRow*), bool), asCALL_THISCALL);
	 RETURNFAIL_IF(result < 0);
	 */
	result = engine->RegisterObjectMethod("DBRow",
			"void StoreChildRow(DBRow@ childrow)",
			asMETHODPR(SQLiteRow, StoreChildRow, (SQLiteRow*), void),
			asCALL_THISCALL);
	return result;
}

void SQLiteRow::StoreChildRow(SQLiteRow *childrow)
{
	if (childrow)
	{
		childrow->AddRef();
		m_childrows.push_back(childrow);
	}
}

void SQLiteRow::InitFromTable(SQLiteTable *table)
{
	for (SQLiteColumn *col : table->m_columns)
	{
		if (!col || SQLiteColumn::KeyType::KEY_NONE != col->GetKeyType())
		{
			//Don't set a default value for a key type (what we're
			//doing below) so we can tell easily if one was not
			//provided by the user
			continue;
		}

		switch (col->GetType())
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
	if (!m_refcount)
	{
		delete this;
	}
}

SQLiteRow* SQLiteRow::Factory(SQLiteTable *table)
{
	return new SQLiteRow(table);
}

SQLiteRow::SQLiteRow(SQLiteTable *table)
{
	m_refcount = 1;
	m_table = table;
	InitFromTable(table);
}

SQLiteRow::~SQLiteRow()
{
	for (std::unordered_map<std::string, SQLiteVariant*>::iterator it =
			m_valuemap.begin(), end = m_valuemap.end(); it != end; ++it)
	{
		delete it->second;
	}

	for (SQLiteRow *childrow : m_childrows)
	{
		delete childrow;
	}
}

void SQLiteRow::ClearValues()
{
	for (std::unordered_map<std::string, SQLiteVariant*>::iterator it =
			m_valuemap.begin(), end = m_valuemap.end(); it != end; ++it)
	{
		if (it->second)
		{
			it->second->ClearValue();
		}
	}
}

void SQLiteRow::SetColumnValue(const std::string &colname,
		const SQLiteVariant *value)
{
	if (!m_valuemap[colname])
	{
		m_valuemap[colname] = new SQLiteVariant();
	}

	*(m_valuemap[colname]) = *value;
}

void SQLiteRow::SetColumnValue(const std::string &colname, const int v)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (!var)
	{
		var = new SQLiteVariant();
		m_valuemap[colname] = var;
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string &colname, const long long v)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (!var)
	{
		var = new SQLiteVariant();
		m_valuemap[colname] = var;
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string &colname, const unsigned int v)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (!var)
	{
		var = new SQLiteVariant();
		m_valuemap[colname] = var;
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string &colname,
		const unsigned long long v)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (!var)
	{
		var = new SQLiteVariant();
		m_valuemap[colname] = var;
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string &colname, const float v)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (!var)
	{
		var = new SQLiteVariant();
		m_valuemap[colname] = var;
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string &colname, const double v)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (!var)
	{
		var = new SQLiteVariant();
		m_valuemap[colname] = var;
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string &colname, const std::string &v)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (!var)
	{
		var = new SQLiteVariant();
		m_valuemap[colname] = var;
	}

	var->SetValue(v);
}

void SQLiteRow::SetColumnValue(const std::string &colname, const EditableText &v)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (!var)
	{
		var = new SQLiteVariant();
		m_valuemap[colname] = var;
	}

	var->SetValue(v.GetString());
}

void SQLiteRow::SetColumnValue(const std::string &colname, const char *data,
		const size_t datalen)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (!var)
	{
		var = new SQLiteVariant();
		m_valuemap[colname] = var;
	}

	var->SetValue(data, datalen);
}

void SQLiteRow::SetColumnValue(const std::string &colname, const UUID &uuid)
{
	SetColumnValue(colname, uuid.GetData(), uuid.GetDataSize());
}

void SQLiteRow::SetColumnValue(const std::string& colname, const MPFloat& mpfin)
{
	std::vector<char> buffer;
	mpfin.SerializeOut(buffer);
	SetColumnValue(colname, &buffer[0], buffer.size());
}

void SQLiteRow::SetColumnValue(const std::string& colname, const MPInt& mpzin)
{
	std::vector<char> buffer;
	mpzin.SerializeOut(buffer);
	SetColumnValue(colname, &buffer[0], buffer.size());
}

bool SQLiteRow::GetColumnValue(const std::string& colname, MPFloat& mpfout, double defvalue)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (var)
	{
		mpfout.SerializeIn(var->GetValueBlobPtr(), var->GetDataLength());
		return true;
	}
	else
	{
		mpfout = defvalue;
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string& colname, MPInt& mpzout, int defval)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (var)
	{
		mpzout.SerializeIn(var->GetValueBlobPtr(), var->GetDataLength());
		return true;
	}
	else
	{
		mpzout = defval;
		return false;
	}
}

SQLiteVariant* SQLiteRow::GetColumnValue(const std::string &colname)
{
	return m_valuemap[colname];
}

bool SQLiteRow::GetColumnValue(const std::string &colname, int &out, int defval)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		out = defval;
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string &colname, unsigned int &out,
		unsigned int defval)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		out = defval;
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string &colname, long long &out,
		long long defval)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		out = defval;
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string &colname,
		unsigned long long &out, unsigned long long defval)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		out = defval;
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string &colname, float &out,
		float defval)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		out = defval;
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string &colname, double &out,
		double defval)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		out = defval;
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string &colname, std::string &out,
		std::string defval)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		out = defval;
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string &colname, EditableText &out,
		std::string defval)
{
	SQLiteVariant *var = m_valuemap[colname];
	std::string temp;
	if (var)
	{
		var->GetValue(temp);
		out = temp;
		return true;
	}
	else
	{
		out = temp;
		return false;
	}
}


bool SQLiteRow::GetColumnValue(const std::string &colname,
		std::vector<char> &out)
{
	//A default value for a raw data makes no sense
	SQLiteVariant *var = m_valuemap[colname];
	if (var)
	{
		var->GetValue(out);
		return true;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::GetColumnValue(const std::string &colname, UUID &uuidout)
{
	SQLiteVariant *var = m_valuemap[colname];
	if (var)
	{
		uuidout.CopyFromByteArray(
				reinterpret_cast<const unsigned char*>(var->GetValueBlobPtr()),
				var->GetDataLength());
		return true;
	}
	else
	{
		//The default value provided by the UUID class is
		//the only default value which makes sense
		return false;
	}
}

bool SQLiteRow::LoadFromDB()
{
	if (m_table)
	{
		return (m_table->LoadRow(this) != SQLITE_ERROR) ? true : false;
	}
	else
	{
		return false;
	}
}

bool SQLiteRow::StoreChildRowIntoDB(SQLiteRow *parent_row)
{
	if (m_table)
	{
		return (m_table->StoreRow(this, parent_row) != SQLITE_ERROR) ?
				true : false;
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
