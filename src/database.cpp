#include "database.h"
extern "C"
{
#include "server.h"
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

bool ASAPI_SaveObject(asIScriptObject* obj)
{
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
			return false;
		}

		//Call the Save function of each part of the object's class hierarchy,
		//then commit the changes to the row to the database
		std::unique_ptr<SQLiteRow> obj_row(type_table->CreateRow());
		while(obj_ti)
		{
			asIScriptFunction* pSaveFun = obj_ti->GetMethodByName("OnSave", false);
			if(pSaveFun)
			{
				ctx->Prepare(pSaveFun);
				ctx->SetArgObject(0, obj_row.get());
				ctx->Execute();
			}
			obj_ti = obj_ti->GetBaseType();
		}

		if(!obj_row->StoreIntoDB())
		{
			if(ctx->SetException("Couldn't save row to database.") < 0)
			{
				ServerLog(SERVERLOG_ERROR, "Couldn't set exception for db storage error.");
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

	result = sengine->RegisterInterfaceMethod("IPersistent", "void OnLoad(uuid key)");
	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterInterfaceMethod("IPersistent", "void OnDefineSchema(DBTable@ table)");

	RETURNFAIL_IF(result < 0);

	result = sengine->RegisterGlobalFunction("bool SaveObject(IPersistent@ obj)",
						asFUNCTION(ASAPI_SaveObject), asCALL_CDECL);
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

		if(resultcode < 0)
		{
			return -1;
		}

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
