#include "database.h"
#include "server.h"
#include "sqlite/sqlite3.h"
#include "charvector.h"
#include <string.h>

int Database_Init(struct Database* asdb, const char* path)
{
	strcpy(asdb->path, path);
	int resultcode = sqlite3_open(path, &asdb->pDB);

	if(resultcode < 0)
	{
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
	return 0;
}


void Database_Release(struct Database* asdb)
{
	sqlite3_close(asdb->pDB);
}
