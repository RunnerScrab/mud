#ifndef DATABASE_H_
#define DATABASE_H_
#include "sqlite/sqlite3.h"

struct Database
{
	sqlite3* ppdb;
	char path[256];
};

int Database_Init(struct Database* asdb, const char* path);
void Database_Release(struct Database* asdb);

#endif
