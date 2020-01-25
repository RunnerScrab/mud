#ifndef DATABASE_H_
#define DATABASE_H_
#include "sqlite/sqlite3.h"

struct Database
{
	sqlite3* ppdb;
	char path[256];
};

int Database_Init(struct ASDatabase* asdb, const char* path);
void Database_Destroy(struct ASDatabase* asdb);

#endif
