#include "database.h"
extern "C"
{
#include "server.h"
#include "sqlite/sqlite3.h"
#include "charvector.h"
}
#include <cstring>

#include "sqlitetable.h"
#include "sqliterow.h"
#include "angelscript.h"

#define RETURNFAIL_IF(a) if(a){return -1;}

static int EnableForeignKeys(sqlite3* pDB)
{
	return sqlite3_exec(pDB, "PRAGMA foreign_keys=ON;", 0, 0, 0);
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

	result = RegisterDBRow(asdb);
	RETURNFAIL_IF(result < 0);

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

	result = sengine->RegisterObjectMethod("DBTable", "DBRow@ MakeRow()",
						asMETHODPR(SQLiteTable, CreateRow, (void), SQLiteRow*), asCALL_THISCALL);

	return result;
}

extern "C"
{
	int Database_Init(struct Database* asdb, struct Server* server, const char* path)
	{
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
