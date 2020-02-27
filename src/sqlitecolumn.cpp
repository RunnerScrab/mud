#include "sqlitecolumn.h"

SQLiteColumn::SQLiteColumn(const std::string& str, SQLiteVariant::StoredType vartype, KeyType keytype)
{
	m_keytype = keytype;
	m_property_name = str;
	m_value.ForceSetType(vartype);
}

SQLiteColumn::~SQLiteColumn()
{

}

int SQLiteColumn::BindToSQLiteStatement(sqlite3_stmt* stmt, int pos)
{
	switch(m_value.GetType())
	{
	case SQLiteVariant::StoredType::VARINT:

		switch(m_value.GetDataLength())
		{
		case sizeof(unsigned char):
		case sizeof(unsigned short):
		case sizeof(unsigned int):
		{
			//sqlite only stores data as signed anyway so we just have to cast it back
			//if we know that's what we want
			unsigned int val = 0;
			m_value.GetValue(val);
			return sqlite3_bind_int(stmt, pos, val);

		}
		break;
		case sizeof(unsigned long long):
		{
			unsigned long long val = 0;
			m_value.GetValue(val);
			return sqlite3_bind_int64(stmt, pos, val);
			break;
		}
		default:
			return SQLITE_ERROR;
			break;
		}
		break;
	case SQLiteVariant::StoredType::VARREAL:
		if(sizeof(float) == m_value.GetDataLength())
		{
			float val = 0.f;
			m_value.GetValue(val);
			return sqlite3_bind_double(stmt, pos, static_cast<double>(val));
		}
		else
		{
			double val = 0.0;
			m_value.GetValue(val);
			return sqlite3_bind_double(stmt, pos, val);
		}

		break;
	case SQLiteVariant::StoredType::VARTEXT:
		return sqlite3_bind_text(stmt, pos, m_value.GetValueBlobPtr(), m_value.GetDataLength() - 1, 0);
	case SQLiteVariant::StoredType::VARBLOB:
		return sqlite3_bind_blob(stmt, pos, m_value.GetValueBlobPtr(), m_value.GetDataLength(), 0);
	default:
		return SQLITE_ERROR;
	}
}
