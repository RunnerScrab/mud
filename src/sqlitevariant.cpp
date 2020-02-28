#include "sqlitevariant.h"
#include "sqlite/sqlite3.h"

int BindVariantToStatement(sqlite3_stmt* stmt, const SQLiteVariant* value, int pos)
{
	if(!value)
	{
		printf("Given a 0 variant pointer!\n");
		return SQLITE_ERROR;
	}
	switch(value->GetType())
	{
	case SQLiteVariant::StoredType::VARINT:

		switch(value->GetDataLength())
		{
		case sizeof(unsigned char):
		case sizeof(unsigned short):
		case sizeof(unsigned int):
		{
			//sqlite only stores data as signed anyway so we just have to cast it back
			//if we know that's what we want
			unsigned int val = 0;
			value->GetValue(val);
			return sqlite3_bind_int(stmt, pos, val);

		}
		break;
		case sizeof(unsigned long long):
		{
			unsigned long long val = 0;
			value->GetValue(val);
			return sqlite3_bind_int64(stmt, pos, val);
			break;
		}
		default:
			return SQLITE_ERROR;
			break;
		}
		break;
	case SQLiteVariant::StoredType::VARREAL:
		if(sizeof(float) == value->GetDataLength())
		{
			float val = 0.f;
			value->GetValue(val);
			return sqlite3_bind_double(stmt, pos, static_cast<double>(val));
		}
		else
		{
			double val = 0.0;
			value->GetValue(val);
			return sqlite3_bind_double(stmt, pos, val);
		}

		break;
	case SQLiteVariant::StoredType::VARTEXT:
		return sqlite3_bind_text(stmt, pos, value->GetValueBlobPtr(), value->GetDataLength() - 1, 0);
	case SQLiteVariant::StoredType::VARBLOB:
		return sqlite3_bind_blob(stmt, pos, value->GetValueBlobPtr(), value->GetDataLength(), 0);
	default:
		return SQLITE_ERROR;
	}
}

std::string VariantTypeToString(SQLiteVariant::StoredType type)
{
	static const char* typestrs[] = {"NULL", "INT", "REAL", "BLOB", "TEXT"};
	return std::string(typestrs[static_cast<int>(type)]);
}
