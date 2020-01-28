#include "database.h"
#include "sqlite/sqlite3.h"

#include <string.h>

int Database_Init(struct Database* asdb, const char* path)
{
	strcpy(asdb->path, path);
	int result = sqlite3_open(path, &asdb->ppdb);

	return result;
}

void Database_Release(struct Database* asdb)
{
	sqlite3_close(asdb->ppdb);
}
