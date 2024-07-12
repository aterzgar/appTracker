#include "application_manager.h"
#include <iostream>
using namespace std;

void add_application(sqlite3* DB){
    string companyName, applicationDate, position, contactPerson, status, notes;
    cout << "Enter company name: ";
    getline(cin, companyName);
    if (companyName.empty()){
        cout << "Error: Missing required field 'company name'" << endl;
        cout << "Enter company name: ";
        getline(cin, companyName);
    }
    
    cout << "Enter application date (YYYY-MM-DD): ";
    getline(cin, applicationDate);
    if (applicationDate.empty()){
        cout << "Error: Missing required field 'application date'" << endl;
        cout << "Enter application date (YYYY-MM-DD): ";
        getline(cin, applicationDate);
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

    string sqlDelete = "DELETE FROM APPLICATION_TRACKER WHERE ID = " + to_string(id) + ";";
    char *errMsg = 0;
    int rc = sqlite3_exec(DB, sqlDelete.c_str(), nullptr, 0, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
    } else {
        cout << "Record deleted successfully" << endl;
    }
}

int display_callback(void* NotUsed, int argc, char** argv, char** azColName) {
    for (int i = 0; i < argc; i++) {
        cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << endl;
    }
    cout << endl;
    return 0;
}