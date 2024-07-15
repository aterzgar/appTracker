#include "db_manager.h"
#include <iostream>

using namespace std;

sqlite3* createDB(const char *s){
    sqlite3* DB;
    int rc = sqlite3_open(s, &DB);
    if (rc) {
        cerr << "Can't open database: " << sqlite3_errmsg(DB) << endl;
        sqlite3_close(DB);
        return nullptr;
    }
    const char *sql = "CREATE TABLE IF NOT EXISTS APPLICATION_TRACKER("
                      "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "COMPANY_NAME TEXT NOT NULL,"
                      "APPLICATION_DATE DATE NOT NULL,"
                      "POSITION TEXT NOT NULL,"
                      "CONTACT_PERSON TEXT NOT NULL,"
                      "STATUS TEXT,"
                      "NOTES TEXT);";

    char *errMsg = 0;
    rc = sqlite3_exec(DB, sql, nullptr, 0, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
        sqlite3_close(DB);
        return nullptr;
    }
    cout << "Database successfully initialized!" << endl;
    return DB;
}

void close_db(sqlite3* DB){
    sqlite3_close(DB);
}

int execute_sql(sqlite3* db, const char* sql, int (*callback)(void*, int, char**, char**), void* data, char** errMsg) {
    int rc = sqlite3_exec(db, sql, callback, data, errMsg);
    if (rc != SQLITE_OK && errMsg) {
        cerr << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
    return rc;
}