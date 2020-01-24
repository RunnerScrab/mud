#ifndef DATABASE_H_
#define DATABASE_H_
#include "sqlite/sqlite3.h"

struct Database
{

};

int Database_Init(struct ASDatabase* asdb);
void Database_Destroy(struct ASDatabase* asdb);

#endif
