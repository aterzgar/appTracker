#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <sqlite3.h>

sqlite3* createDB(const char* s);
void close_db(sqlite3* DB);
int execute_sql(sqlite3* db, const char* sql, int (*callback)(void*, int, char**, char**), void* data, char** errMsg);

#endif // DB_MANAGER_H