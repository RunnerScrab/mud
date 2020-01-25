#include "database.h"
#include "sqlite/sqlite3.h"

int Database_Init(struct Database* asdb, const char* path)
{
	int result = sqlite3_open(path, &asdb->ppdb);

	return 0;
}

void Database_Destroy(struct Database* asdb)
{
	sqlite3_close(asdb->ppdb);
}
