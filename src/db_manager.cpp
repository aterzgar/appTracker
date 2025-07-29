#include "db_manager.h"
#include <iostream>
#include <regex>
#include <memory>

// Helper RAII for sqlite3_stmt
struct SqliteStatementGuard {
    sqlite3_stmt* stmt = nullptr;
    ~SqliteStatementGuard() { if (stmt) sqlite3_finalize(stmt); }
    operator sqlite3_stmt*() { return stmt; }
    sqlite3_stmt* operator->() { return stmt; }
};

DatabaseManager::DatabaseManager() : db(nullptr) {}

DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

bool DatabaseManager::openDatabase(const std::string& dbPath) {
    closeDatabase();
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        closeDatabase();
        return false;
    }
    return createTables();
}

void DatabaseManager::closeDatabase() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool DatabaseManager::createTables() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS APPLICATION_TRACKER("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "COMPANY_NAME TEXT NOT NULL,"
        "APPLICATION_DATE TEXT NOT NULL,"
        "POSITION TEXT NOT NULL,"
        "CONTACT_PERSON TEXT NOT NULL,"
        "STATUS TEXT,"
        "NOTES TEXT);";
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << (errMsg ? errMsg : "") << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool DatabaseManager::addApplication(const Application& app) {
    if (!isValidDateTime(app.applicationDate)) return false;
    if (isDuplicateEntry(app)) return false;

    const char* sql =
        "INSERT INTO APPLICATION_TRACKER "
        "(COMPANY_NAME, APPLICATION_DATE, POSITION, CONTACT_PERSON, STATUS, NOTES) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    SqliteStatementGuard stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    bindCommonFields(stmt, app);

    rc = sqlite3_step(stmt);
    return rc == SQLITE_DONE;
}

std::vector<Application> DatabaseManager::getAllApplications() {
    std::vector<Application> applications;
    const char* sql = "SELECT ID, COMPANY_NAME, APPLICATION_DATE, POSITION, CONTACT_PERSON, STATUS, NOTES FROM APPLICATION_TRACKER ORDER BY ID DESC;";

    SqliteStatementGuard stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return applications;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Application app;
        app.id = sqlite3_column_int(stmt, 0);
        app.companyName   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        app.applicationDate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        app.position      = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        app.contactPerson = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        app.status        = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        app.notes         = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        applications.push_back(std::move(app));
    }

    return applications;
}

bool DatabaseManager::deleteApplication(int id) {
    const char* sql = "DELETE FROM APPLICATION_TRACKER WHERE ID = ?;";
    SqliteStatementGuard stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    rc = sqlite3_step(stmt);

    return rc == SQLITE_DONE;
}

bool DatabaseManager::updateApplication(const Application& app) {
    if (!isValidDateTime(app.applicationDate)) return false;

    const char* sql =
        "UPDATE APPLICATION_TRACKER SET "
        "COMPANY_NAME = ?, APPLICATION_DATE = ?, POSITION = ?, CONTACT_PERSON = ?, STATUS = ?, NOTES = ? "
        "WHERE ID = ?;";

    SqliteStatementGuard stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    bindCommonFields(stmt, app);
    sqlite3_bind_int(stmt, 7, app.id);

    rc = sqlite3_step(stmt);

    return rc == SQLITE_DONE;
}

bool DatabaseManager::isValidDateTime(const std::string& date) const {
    // Year: 0000-9999, Month: 01-12, Day: 01-31 (basic check only)
    const std::regex pattern(R"(^\d{4}-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01])$)");
    return std::regex_match(date, pattern);
}

bool DatabaseManager::isDuplicateEntry(const Application& app) const {
    const char* sql =
        "SELECT COUNT(*) FROM APPLICATION_TRACKER WHERE COMPANY_NAME = ? AND APPLICATION_DATE = ? AND POSITION = ? AND CONTACT_PERSON = ? AND STATUS = ?;";

    SqliteStatementGuard stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return false; // treat as not duplicate but error
    }

    bindCommonFieldsForDuplicate(stmt, app);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    return count > 0;
}

int DatabaseManager::getTotalApplications() const {
    const char* sql = "SELECT COUNT(*) FROM APPLICATION_TRACKER;";
    
    SqliteStatementGuard stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return 0;
    }
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    return count;
}

int DatabaseManager::getApplicationsByStatus(const std::string& status) const {
    const char* sql = "SELECT COUNT(*) FROM APPLICATION_TRACKER WHERE STATUS = ?;";
    
    SqliteStatementGuard stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return 0;
    }
    
    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    return count;
}

std::vector<std::pair<std::string, int>> DatabaseManager::getStatusCounts() const {
    std::vector<std::pair<std::string, int>> statusCounts;
    const char* sql = "SELECT STATUS, COUNT(*) FROM APPLICATION_TRACKER GROUP BY STATUS ORDER BY COUNT(*) DESC;";
    
    SqliteStatementGuard stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg(db) << std::endl;
        return statusCounts;
    }
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int count = sqlite3_column_int(stmt, 1);
        statusCounts.emplace_back(status, count);
    }
    
    return statusCounts;
}

// ---------- Private helper definitions ----------

/**
 * Helper to bind the six main fields in the same order as all common queries, except for ID.
 */
void DatabaseManager::bindCommonFields(sqlite3_stmt* stmt, const Application& app) const {
    sqlite3_bind_text(stmt, 1, app.companyName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, app.applicationDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, app.position.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, app.contactPerson.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, app.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, app.notes.c_str(), -1, SQLITE_TRANSIENT);
}

void DatabaseManager::bindCommonFieldsForDuplicate(sqlite3_stmt* stmt, const Application& app) const {
    sqlite3_bind_text(stmt, 1, app.companyName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, app.applicationDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, app.position.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, app.contactPerson.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, app.status.c_str(), -1, SQLITE_TRANSIENT);
}
