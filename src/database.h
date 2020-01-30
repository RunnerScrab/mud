#ifndef DATABASE_H_
#define DATABASE_H_
#include "sqlite/sqlite3.h"
#include <stddef.h>

struct Database
{
	sqlite3* pDB;
	char path[256];

	size_t table_count;
};

int Database_Init(struct Database* asdb, const char* path);
void Database_Release(struct Database* asdb);

#endif
