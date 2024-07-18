#ifndef APPLICATION_MANAGER_H
#define APPLICATION_MANAGER_H

#include <sqlite3.h>
#include <string>

bool isValid_DateTime(const std::string& date);
bool is_duplicate_entry(sqlite3* db, const string& companyName, const string& applicationDate, const string& position, const string& contactPerson, const string& status);
bool check_id(sqlite3* DB, int id);
void add_application(sqlite3* DB);
void display_applications(sqlite3* DB);
void delete_application(sqlite3* DB);
int display_callback(void* NotUsed, int argc, char** argv, char** azColName);

#endif // APPLICATION_MANAGER_H
