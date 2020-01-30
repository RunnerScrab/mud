#include "database.h"
#include "server.h"
#include "sqlite/sqlite3.h"
#include "charvector.h"
#include <string.h>

int Database_Init(struct Database* asdb, const char* path)
{
	strcpy(asdb->path, path);
	int result = sqlite3_open(path, &asdb->pDB);

	if(result < 0)
	{
		return -1;
	}

	cv_t query;
	cv_init(&query, 64);
	cv_sprintf(&query, "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;");
	sqlite3_stmt* res = 0;

	result = sqlite3_prepare_v2(asdb->pDB, query.data, -1, &res, 0);
	if(SQLITE_OK == result)
	{
		asdb->table_count = sqlite3_column_count(res);
		ServerLog(SERVERLOG_STATUS, "The database had %lu tables.", asdb->table_count);
	}

	sqlite3_finalize(res);
	cv_destroy(&query);
	return result;
}

void Database_Release(struct Database* asdb)
{
	sqlite3_close(asdb->pDB);
}
