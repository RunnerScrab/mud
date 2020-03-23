#include "database.h"
extern "C"
{
#include "server.h"
#include "utils.h"
#include "sqlite/sqlite3.h"
#include "charvector.h"
}
#include <cstring>
#include <memory>

#include "sqlitetable.h"
#include "sqliterow.h"
#include "angelscript.h"

#define RETURNFAIL_IF(a) if(a){return -1;}

static int EnableForeignKeys(sqlite3* pDB)
{
	return sqlite3_exec(pDB, "PRAGMA foreign_keys=ON;", 0, 0, 0);
}


static bool LoadObject(asIScriptObject* obj, asITypeInfo* obj_type,
		asIScriptContext* ctx, SQLiteRow* obj_row)
{
	if(ctx->PushState() < 0)
	{
		//Couldn't push state
		if(ctx->SetException("Out of memory error.") < 0)
		{
			ServerLog(SERVERLOG_ERROR, "Couldn't set an out of memory exception.");
		}
		return false;
	}

	//Call the Load function of each part of the object's class hierarchy,
	//then commit the changes to the row to the database
	if(obj_row->LoadFromDB())
	{
		asITypeInfo* obj_ti = obj_type;
		while(obj_ti)
		{
			asIScriptFunction* pLoadFun = obj_ti->GetMethodByName("OnLoad", false);
			if(pLoadFun)
			{
				ctx->Prepare(pLoadFun);
				ctx->SetObject(obj);
				ctx->SetArgObject(0, obj_row);
				ctx->Execute();
			}
			obj_ti = obj_ti->GetBaseType();
		}
	}
	else
	{
		if(ctx->PopState() < 0)
		{
			if(ctx->SetException("Couldn't restore context state.") < 0)
			{
				ServerLog(SERVERLOG_ERROR, "Couldn't set exception for context restoration error.");
			}
		}
		return false;
	}

	if(ctx->PopState() < 0)
	{
		if(ctx->SetException("Couldn't restore context state.") < 0)
		{
			ServerLog(SERVERLOG_ERROR, "Couldn't set exception for context restoration error.");
		}
		return false;
	}

	return true;

}

bool ASAPI_LoadObjectStrKey(asIScriptObject* obj, const std::string& key)
{
	if(obj)
	{
		asITypeInfo* obj_ti = obj->GetObjectType();
		SQLiteTable* type_table = reinterpret_cast<SQLiteTable*>(obj_ti->GetUserData(AS_USERDATA_TYPESCHEMA));
		SQLiteColumn* primary_keycol = type_table->GetPrimaryKeyCol();
		//Use the calling context
		asIScriptContext* ctx = asGetActiveContext();

		if(!primary_keycol)
		{
			if(ctx->SetException("Table has no primary key set.") < 0)
			{
				ServerLog(SERVERLOG_ERROR, "Couldn't set a primary key exception.");
			}
			obj->Release();
			return false;
		}

		if(SQLiteVariant::VARTEXT != primary_keycol->GetType())
		{
			if(ctx->SetException("Key type mismatch.") < 0)
			{
				ServerLog(SERVERLOG_ERROR, "Couldn't set a key type mismatch exception.");
			}
			obj->Release();
			return false;
		}

		std::unique_ptr<SQLiteRow> obj_row(type_table->CreateRow());
		obj_row->SetColumnValue(primary_keycol->GetName(), key);
		bool result = LoadObject(obj, obj_ti, ctx, obj_row.get());
		obj->Release();
		return result;
	}
	return false;
}

bool ASAPI_LoadObjectUUIDKey(asIScriptObject* obj, const UUID& key)
{
	if(obj)
	{
		asITypeInfo* obj_ti = obj->GetObjectType();
		SQLiteTable* type_table = reinterpret_cast<SQLiteTable*>(obj_ti->GetUserData(AS_USERDATA_TYPESCHEMA));
		SQLiteColumn* primary_keycol = type_table->GetPrimaryKeyCol();
		//Use the calling context
		asIScriptContext* ctx = asGetActiveContext();

		if(!primary_keycol)
		{
			if(ctx->SetException("Table has no primary key set.") < 0)
			{
				ServerLog(SERVERLOG_ERROR, "Couldn't set a primary key exception.");
			}
			return false;
		}

		if(SQLiteVariant::VARBLOB != primary_keycol->GetType())
		{
			if(ctx->SetException("Key type mismatch.") < 0)
			{
				ServerLog(SERVERLOG_ERROR, "Couldn't set a key type mismatch exception.");
			}
			return false;
		}

		std::unique_ptr<SQLiteRow> obj_row(type_table->CreateRow());
		obj_row->SetColumnValue(primary_keycol->GetName(), key);
		bool result = LoadObject(obj, obj_ti, ctx, obj_row.get());
		obj->Release();
		return result;
	}
	return false;
}

template<typename T> bool ASAPI_LoadObjectIntKey(asIScriptObject* obj, const T key)
{
	if(obj)
	{
		asITypeInfo* obj_ti = obj->GetObjectType();
		SQLiteTable* type_table = reinterpret_cast<SQLiteTable*>(obj_ti->GetUserData(AS_USERDATA_TYPESCHEMA));
		SQLiteColumn* primary_keycol = type_table->GetPrimaryKeyCol();
		//Use the calling context
		asIScriptContext* ctx = asGetActiveContext();

		if(!primary_keycol)
		{
			if(ctx->SetException("Table has no primary key set.") < 0)
			{
				ServerLog(SERVERLOG_ERROR, "Couldn't set a primary key exception.");
			}
			obj->Release();
			return false;
		}

		if(SQLiteVariant::VARINT != primary_keycol->GetType())
		{
			if(ctx->SetException("Key type mismatch.") < 0)
			{
				ServerLog(SERVERLOG_ERROR, "Couldn't set a key type mismatch exception.");
			}
			obj->Release();
			return false;
		}

		std::unique_ptr<SQLiteRow> obj_row(type_table->CreateRow());
		obj_row->SetColumnValue(primary_keycol->GetName(), key);
		bool result = LoadObject(obj, obj_ti, ctx, obj_row.get());
		obj->Release();
		return result;
	}
	return false;
}

bool ASAPI_SaveObject(asIScriptObject* obj)
{
	dbgprintf("Calling SaveObject()!\n");
	if(obj)
	{
		asITypeInfo* obj_ti = obj->GetObjectType();
		SQLiteTable* type_table = reinterpret_cast<SQLiteTable*>(obj_ti->GetUserData(AS_USERDATA_TYPESCHEMA));

		//Use the calling context
		asIScriptContext* ctx = asGetActiveContext();
		if(ctx->PushState() < 0)
		{
			//Couldn't push state
			if(ctx->SetException("Out of memory error.") < 0)
			{
				ServerLog(SERVERLOG_ERROR, "Couldn't set an out of memory exception.");
			}
			obj->Release();
			return false;
		}

		//Call the Save function of each part of the object's class hierarchy,
		//then commit the changes to the row to the database
		SQLiteRow* obj_row = new SQLiteRow(type_table);////type_table->CreateRow();
		if(!obj_row)
		{
			if(ctx->SetException("Unable to create row from table.") < 0)
				ServerLog(SERVERLOG_ERROR, "Couldn't set a row failure exception.");
			ctx->PopState();
			obj->Release();
			return false;
		}
		while(obj_ti)
		{
			asIScriptFunction* pSaveFun = obj_ti->GetMethodByName("OnSave", false);
			if(pSaveFun)
			{
				ctx->Prepare(pSaveFun);
				ctx->SetObject(obj);
				ctx->SetArgObject(0, (void*) obj_row);
				ctx->Execute();
			}
			else
			{
				ServerLog(SERVERLOG_ERROR, "Class didn't have expected OnSave event.");
			}
			obj_ti = obj_ti->GetBaseType();
		}

		if(!obj_row->StoreIntoDB())
		{
			if(ctx->SetException("Couldn't save row to database.") < 0)
			{
				ServerLog(SERVERLOG_ERROR, "Couldn't set exception for db storage error.");
			}
			obj->Release();
			delete obj_row;
			ctx->PopState();
			return false;
		}

		if(ctx->PopState() < 0)
		{
			if(ctx->SetException("Couldn't restore context state.") < 0)
			{
				ServerLog(SERVERLOG_ERROR, "Couldn't set exception for context restoration error.");
			}
			delete obj_row;
			obj->Release();
			return false;
		}

		delete obj_row;
		obj->Release();
		return true;
	}
	return false;
}

static int RegisterDatabaseAPI(struct Database* asdb)
{
	sqlite3* sqldb = asdb->pDB;
	asIScriptEngine* sengine = asdb->pServer ? asdb->pServer->as_manager.engine : 0;

	if(!sqldb || !sengine)
	{
		return -1;
	}

	SQLiteTable::SetDBConnection(sqldb);
	int result = 0;

	result = RegisterDBRow(sqldb, sengine);
	RETURNFAIL_IF(result < 0);
	ServerLog(SERVERLOG_STATUS, "Registered DBRow.");
	result = RegisterDBTable(sqldb, sengine);
	RETURNFAIL_IF(result < 0);
	ServerLog(SERVERLOG_STATUS, "Registered DBTable.");

	result = sengine->RegisterInterface("IPersistent");
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterInterfaceMethod("IPersistent", "void OnSave(DBRow@ row)");
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterInterfaceMethod("IPersistent", "void OnLoad(DBRow@ row)");
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterInterfaceMethod("IPersistent", "void OnDefineSchema(DBTable@ table)");

	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterGlobalFunction("bool SaveObject(IPersistent@ obj)",
						asFUNCTION(ASAPI_SaveObject), asCALL_CDECL);

	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterGlobalFunction("bool LoadObject(IPersistent@ obj, const uuid& in)",
						asFUNCTION(ASAPI_LoadObjectUUIDKey), asCALL_CDECL);
	RETURNFAIL_IF(result < 0);
	result = sengine->RegisterGlobalFunction("bool LoadObject(IPersistent@ obj, const string& in)",
						asFUNCTION(ASAPI_LoadObjectStrKey), asCALL_CDECL);
	RETURNFAIL_IF(result < 0);
	result = sengine->RegisterGlobalFunction("bool LoadObject(IPersistent@ obj, const int key)",
						asFUNCTIONPR(ASAPI_LoadObjectIntKey, (asIScriptObject*, const int), bool),
						asCALL_CDECL);
	RETURNFAIL_IF(result < 0);
	result = sengine->RegisterGlobalFunction("bool LoadObject(IPersistent@ obj, const uint32 key)",
						asFUNCTIONPR(ASAPI_LoadObjectIntKey,
							(asIScriptObject*, const unsigned int), bool),
						asCALL_CDECL);
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterGlobalFunction("bool LoadObject(IPersistent@ obj, const int64 key)",
						asFUNCTIONPR(ASAPI_LoadObjectIntKey,
							(asIScriptObject*, const long long), bool),
						asCALL_CDECL);
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterGlobalFunction("bool LoadObject(IPersistent@ obj, const uint64 key)",
						asFUNCTIONPR(ASAPI_LoadObjectIntKey,
							(asIScriptObject*, const unsigned long long), bool),
						asCALL_CDECL);
	RETURNFAIL_IF(result < 0);


	return result;
}

extern "C"
{
	int Database_Init(struct Database* asdb, struct Server* server, const char* path)
	{
		ServerLog(SERVERLOG_STATUS, "Initializing database.");
		asdb->pServer = server;
		strcpy(asdb->path, path);
		int resultcode = sqlite3_open(path, &asdb->pDB);

		if(SQLITE_OK != resultcode)
		{
			return -1;
		}
		ServerLog(SERVERLOG_STATUS, "Opened database at path '%s'.", asdb->path);

		if(SQLITE_OK != EnableForeignKeys(asdb->pDB))
		{
			Database_Release(asdb);
			ServerLog(SERVERLOG_ERROR, "Failed to enable sqlite foreign key constraints.");
			return -1;
		}
		cv_t query;
		cv_init(&query, 64);
		cv_sprintf(&query,
			"SELECT Count(*) from sqlite_master WHERE type='table';");

		sqlite3_stmt* res = 0;
		resultcode = sqlite3_prepare_v2(asdb->pDB, query.data, -1, &res, 0);

		int row_count;
		if(SQLITE_OK == resultcode && SQLITE_ROW == sqlite3_step(res))
		{
			row_count = sqlite3_column_int(res, 0);
			ServerLog(SERVERLOG_STATUS, "Found %lu tables in database.", row_count);
		}
		else
		{
			ServerLog(SERVERLOG_ERROR, "Could not get the number of tables in the database.");
			sqlite3_finalize(res);
			cv_destroy(&query);
			return -1;
		}

		sqlite3_finalize(res);
		cv_destroy(&query);

		return RegisterDatabaseAPI(asdb);
	}

	void Database_Release(struct Database* asdb)
	{
		sqlite3_close(asdb->pDB);
	}
}
