#include <iostream>
#include <fstream>
#include "db_manager.h"
#include "application_manager.h"

using namespace std;

void show_menu() {
    cout << "Application Tracker Menu:" << endl;
    cout << "1. Add Application" << endl;
    cout << "2. Display Applications" << endl;
    cout << "3. Delete Application" << endl;
    cout << "4. Exit" << endl;
    cout << "Enter your choice: ";
}

int main() {
    ofstream log("application_log.txt");
    sqlite3 *DB = createDB("resume.db");
    if (DB == nullptr) {
        return 1;
    }

    int choice;
    do {
        show_menu();
        cin >> choice;
        if (cin.fail()){
            cout <<"Error" << endl;
            cin.clear();
            cin >> choice;
        }
        cin.ignore(); // to handle the newline character after entering choice

        switch (choice) {
            case 1:
                log << "Adding application..." << endl;
                add_application(DB);
                break;
            case 2:
                log << "Displaying applications..." << endl;
                display_applications(DB);
                break;
            case 3:
                log << "Deleting application..." << endl;
                delete_application(DB);
                break;
            case 4:
                cout << "Exiting..." << endl;
                break;
            default:
                log << "Invalid choice: " << choice << endl;
                cout << "Invalid choice, please try again." << endl;
        }
    } while (choice != 4);

    close_db(DB);
    return 0;
}
