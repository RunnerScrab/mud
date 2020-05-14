#ifndef DATABASE_H_
#define DATABASE_H_
#include "sqlite/sqlite3.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct asIScriptModule asIScriptModule;

struct Database
{
	sqlite3 *pDB;
	char path[256];
	size_t table_count;

	struct Server *pServer;
	asIScriptModule *pMainScriptModule; //Persistent classes should always be in the main script module
};

int Database_Init(struct Database *asdb, struct Server *server,
		const char *path);
void Database_Release(struct Database *asdb);
#ifdef __cplusplus
}
#endif

#endif
