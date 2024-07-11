#include <iostream>
#include <sqlite3.h> 

using namespace std;

// Function prototype 
static int callback(void *NotUsed, int argc, char **argv, char **azColName);

int open_db() {
    sqlite3 *db;
    int rc;
    char *zErrMsg = 0;
    const char *sql;

    rc = sqlite3_open("resume.db", &db);

    if( rc ) {
        cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db); // Close the database connection even on error
        return rc; // Return the error code
    } else {
        cerr << "Opened database successfully\n";
    }

    /* Create a SQL statement*/
    sql = "CREATE TABLE APPLICATION_TRACKER("
          "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
          "COMPANY_NAME TEXT NOT NULL,"
          "APPLICATION_DATE DATE NOT NULL,"
          "POSITION TEXT NOT NULL,"
          "CONTACT PERSON TEXT NOT NULL,"
          "STATUS TEXT,"
          "NOTES TEXT);";

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

    if (rc != SQLITE_OK){
        cerr << "SQL Error\n" << zErrMsg;
        sqlite3_free(zErrMsg);
    }else {
        cout << "Table created successfully\n";
    }
    sqlite3_close(db);
    return 0; // Return 0 on success
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    for (i = 0; i<argc; i++){
        cout << azColName[i] << " = " << (argv[i] ? argv[i] : "NULL") << std::endl;
    }
    cout << endl;
    return 0;
}

void enter_data(){
    string companyName, applicationDate, position, contactPerson, status, notes;
    cout << "Enter company name: ";
    getline(cin, companyName);
    cout << "Enter application date (YYYY-MM-DD): ";
    getline(cin, applicationDate);
    cout << "Enter position: ";
    getline(cin, position);
    cout << "Enter contact person: ";
    getline(cin, contactPerson);
    cout << "Enter status: ";
    getline(cin, status);
    cout << "Enter notes: ";
    getline(cin, notes);

    string sqlInsert = "INSERT INTO APPLICATION_TRACKER (COMPANY_NAME, APPLICATION_DATE, POSITION, CONTACT_PERSON, STATUS, NOTES) VALUES ('" +
                            companyName + "', '" +
                            applicationDate + "', '" +
                            position + "', '" +
                            contactPerson + "', '" +
                            status + "', '" +
                            notes + "');";

    // Execute the SQL INSERT statement
    rc = sqlite3_exec(db, sqlInsert.c_str(), callback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
    } else {
        cout << "Record inserted successfully" << endl;
    }

    sqlite3_close(db);
    return 0;
}

int main() {
    int select;
    cout << "Welcome to Application Tracker! Please select the option below"<<endl;
    cin >> select;
    switch (select){
        case 1:
            cout <<"Enter new record: \n";
            break;
        case 2:
            cout <<"Delete the record: \n";
            break;
        case 3:
            cout <<"Display the record: \n";
            break;
        default:
            cout <<"Enter valid option!";
    }

    cout << "Call the function below\n";
    int result = open_db();
    if (result != 0) {
        cerr << "Failed to open database with error code: " << result << endl;
    }


}
