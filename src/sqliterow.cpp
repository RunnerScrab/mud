#include "sqliterow.h"
#include "sqlitetable.h"

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

SQLiteRow::SQLiteRow(SQLiteTable* table)
{
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
