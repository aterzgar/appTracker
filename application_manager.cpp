#include "application_manager.h"
#include <iostream>
#include <string>
#include <regex>
#include <fstream>

using namespace std;

bool isValid_DateTime(const string& date) {
    const regex pattern("^([0-9]{4})-((01|02|03|04|05|06|07|08|09|10|11|12))-([0-2][0-9]|3[01])$");
    return regex_match(date, pattern);
}

bool is_duplicate_entry(sqlite3* db, const string& companyName, const string& applicationDate, const string& position, const string& contactPerson, const string& status) {
    string sqlQuery = "SELECT COUNT(*) FROM APPLICATION_TRACKER WHERE COMPANY_NAME = '" + companyName +
                      "' AND APPLICATION_DATE = '" + applicationDate +
                      "' AND POSITION = '" + position +
                      "' AND CONTACT_PERSON = '" + contactPerson +
                      "' AND STATUS = '" + status + "';";
    char *errMsg = 0;
    int count = 0;

    auto callback = [](void* data, int argc, char** argv, char** azColName) -> int {
        int* count = static_cast<int*>(data);
        if (argc > 0 && argv[0]) {
            *count = stoi(argv[0]);
        }
        return 0;
    };

    int rc = sqlite3_exec(db, sqlQuery.c_str(), callback, &count, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
        return false;
    }
    return count > 0;
}

bool check_id(sqlite3 *DB, int id){
    string sqlCheck = "SELECT 1 FROM APPLICATION_TRACKER WHERE ID = ? LIMIT 1;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(DB, sqlCheck.c_str(), -1, &stmt, nullptr);
    if(rc != SQLITE_OK){
        cerr << "SQL Error: " << sqlite3_errmsg(DB) << endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);
    bool exists = (rc == SQLITE_ROW);

    sqlite3_finalize(stmt);
    return exists;
}

void add_application(sqlite3* DB){
    string companyName, applicationDate, position, contactPerson, status, notes;
    cout << "Enter company name: ";
    cin.ignore(); // Clear input buffer before using getline
    getline(cin, companyName);
    if (companyName.empty()){
        cout << "Error: Missing required field 'company name'" << endl;
        return;
    }
    cout << "Enter application date (YYYY-MM-DD): ";
    getline(cin, applicationDate);
    if (!isValid_DateTime(applicationDate)){
        cout << "Error: Invalid date format" << endl;
        return;
    }
    cout << "Enter position: ";
    getline(cin, position);
    if (position.empty()){
        cout << "Error: Missing required field 'position'" << endl;
        return;
    }
    cout << "Enter contact person: ";
    getline(cin, contactPerson);
    if(contactPerson.empty()){
        cout << "Error: Missing required field 'contact person'" << endl;
        return;
    }
    cout << "Enter status: ";
    getline(cin, status);
    if(status.empty()){
        cout << "Error: Missing required field 'status'" << endl;
        return;
    }
    cout << "Enter notes: ";
    getline(cin, notes);
    if(notes.empty()){
        cout << "Error: Missing required field 'notes'" << endl;
        return;
    }

    if (is_duplicate_entry(DB, companyName, applicationDate, position, contactPerson, status)) {
        cout << "Error: Duplicate entry" << endl;
        return;
    }

    string sqlInsert = "INSERT INTO APPLICATION_TRACKER (COMPANY_NAME, APPLICATION_DATE, POSITION, CONTACT_PERSON, STATUS, NOTES) VALUES ('" +
                       companyName + "', '" +
                       applicationDate + "', '" +
                       position + "', '" +
                       contactPerson + "', '" +
                       status + "', '" +
                       notes + "');";

    char *errMsg = 0;
    int rc = sqlite3_exec(DB, sqlInsert.c_str(), nullptr, 0, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
    } else {
        cout << "Record inserted successfully" << endl;
    }
}

void display_applications(sqlite3* DB) {
    const char *sql = "SELECT * FROM APPLICATION_TRACKER;";
    char *errMsg = 0;
    int rc = sqlite3_exec(DB, sql, display_callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
}

void delete_application(sqlite3* DB) {
    int id;
    cout << "Enter the ID of the application to delete: ";
    cin >> id;
    cin.ignore();  // Clear the input buffer

    if(check_id(DB, id)){
        string sqlDelete = "DELETE FROM APPLICATION_TRACKER WHERE ID = " + to_string(id) + ";";
        char *errMsg = 0;
        int rc = sqlite3_exec(DB, sqlDelete.c_str(), nullptr, 0, &errMsg);
        if (rc != SQLITE_OK) {
            cerr << "SQL error: " << errMsg << endl;
            sqlite3_free(errMsg);
        } else {
            cout << "Record deleted successfully" << endl;
        }    
    }else {
        cout << "Record with ID " << id << " does not exist." << endl;
    }
}

int display_callback(void* NotUsed, int argc, char** argv, char** azColName) {
    for (int i = 0; i < argc; i++) {
        cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << endl;
    }
    cout << endl;
    return 0;
}