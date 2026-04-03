#include "db_manager.h"

#include <iostream>
#include <regex>
#include <QDate>
#include <QString>

// ---------------------------------------------------------------------------
// RAII guard for sqlite3_stmt
// ---------------------------------------------------------------------------
struct StmtGuard {
    sqlite3_stmt* stmt = nullptr;
    explicit StmtGuard() = default;
    ~StmtGuard() { if (stmt) sqlite3_finalize(stmt); }

    // Convenience implicit conversions used by sqlite3_* C API
    operator sqlite3_stmt*()  const { return stmt; }
    sqlite3_stmt** operator&()      { return &stmt; }

    StmtGuard(const StmtGuard&)            = delete;
    StmtGuard& operator=(const StmtGuard&) = delete;
};

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

DatabaseManager::DatabaseManager() : db(nullptr) {}

DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool DatabaseManager::openDatabase(const std::string& dbPath) {
    closeDatabase();

    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << '\n';
        closeDatabase();
        return false;
    }

    if (!createTables())    return false;

    int version = 0;
    if (!getSchemaVersion(version)) return false;

    // Migration hook – increment kRequiredVersion when adding schema changes
    constexpr int kRequiredVersion = 1;
    if (version < kRequiredVersion) {
        if (!setSchemaVersion(kRequiredVersion)) return false;
    }

    return true;
}

void DatabaseManager::closeDatabase() {
    if (!db) return;

    if (sqlite3_close(db) != SQLITE_OK) {
        std::cerr << "Error closing database: " << sqlite3_errmsg(db) << '\n';
        sqlite3_close_v2(db); // force-close
    }
    db = nullptr;
}

// ---------------------------------------------------------------------------
// Transaction helpers
// ---------------------------------------------------------------------------

bool DatabaseManager::beginTransaction() {
    char* err = nullptr;
    if (sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, &err) != SQLITE_OK) {
        std::cerr << "BEGIN TRANSACTION failed: " << (err ? err : "?") << '\n';
        sqlite3_free(err);
        return false;
    }
    return true;
}

bool DatabaseManager::commitTransaction() {
    char* err = nullptr;
    if (sqlite3_exec(db, "COMMIT", nullptr, nullptr, &err) != SQLITE_OK) {
        std::cerr << "COMMIT failed: " << (err ? err : "?") << '\n';
        sqlite3_free(err);
        rollbackTransaction();
        return false;
    }
    return true;
}

bool DatabaseManager::rollbackTransaction() {
    sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
    return false; // always returns false so callers can: return rollbackTransaction();
}

// ---------------------------------------------------------------------------
// Schema
// ---------------------------------------------------------------------------

bool DatabaseManager::createTables() {
    if (!beginTransaction()) return false;

    // clang-format off
    const char* kStatements[] = {
        // Main table
        "CREATE TABLE IF NOT EXISTS APPLICATION_TRACKER ("
        "  ID               INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  COMPANY_NAME     TEXT NOT NULL,"
        "  APPLICATION_DATE TEXT NOT NULL,"
        "  POSITION         TEXT NOT NULL,"
        "  CONTACT_PERSON   TEXT NOT NULL,"
        "  STATUS           TEXT NOT NULL CHECK(STATUS IN ("
        "    'Applied','Interviews',"
        "    'Offer Received','Rejected','Withdrawn')),"
        "  NOTES            TEXT"
        ");",

        // Schema version table
        "CREATE TABLE IF NOT EXISTS SCHEMA_VERSION (VERSION INTEGER NOT NULL);",
        "INSERT OR IGNORE INTO SCHEMA_VERSION (VERSION) VALUES (1);",

        // Indexes
        "CREATE INDEX IF NOT EXISTS idx_company  ON APPLICATION_TRACKER(COMPANY_NAME);",
        "CREATE INDEX IF NOT EXISTS idx_status   ON APPLICATION_TRACKER(STATUS);",
        "CREATE INDEX IF NOT EXISTS idx_date     ON APPLICATION_TRACKER(APPLICATION_DATE);",
        "CREATE INDEX IF NOT EXISTS idx_position ON APPLICATION_TRACKER(POSITION);",
    };
    // clang-format on

    for (const char* sql : kStatements) {
        char* err = nullptr;
        if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
            std::cerr << "Schema error: " << (err ? err : "?") << '\n';
            sqlite3_free(err);
            return rollbackTransaction();
        }
    }

    return commitTransaction();
}

// ---------------------------------------------------------------------------
// CRUD
// ---------------------------------------------------------------------------

bool DatabaseManager::addApplication(const Application& app) {
    if (!isValidDateTime(app.applicationDate)) {
        std::cerr << "Validation error (add): invalid date or future date: " << app.applicationDate << '\n';
        return false;
    }

    if (isDuplicateEntry(app)) {
        std::cerr << "Validation error (add): duplicate application" << '\n';
        return false;
    }

    if (!beginTransaction()) return false;

    const char* sql =
        "INSERT INTO APPLICATION_TRACKER "
        "(COMPANY_NAME, APPLICATION_DATE, POSITION, CONTACT_PERSON, STATUS, NOTES) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    StmtGuard stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare error (add): " << sqlite3_errmsg(db) << '\n';
        return rollbackTransaction();
    }

    bindAppFields(stmt, app);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Step error (add): " << sqlite3_errmsg(db) << '\n';
        return rollbackTransaction();
    }

    return commitTransaction();
}

bool DatabaseManager::updateApplication(const Application& app) {
    if (!isValidDateTime(app.applicationDate)) {
        std::cerr << "Validation error (update): invalid date or future date: " << app.applicationDate << '\n';
        return false;
    }

    if (!beginTransaction()) return false;

    const char* sql =
        "UPDATE APPLICATION_TRACKER "
        "SET COMPANY_NAME=?, APPLICATION_DATE=?, POSITION=?, CONTACT_PERSON=?, STATUS=?, NOTES=? "
        "WHERE ID=?;";

    StmtGuard stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare error (update): " << sqlite3_errmsg(db) << '\n';
        return rollbackTransaction();
    }

    bindAppFields(stmt, app);
    sqlite3_bind_int(stmt, 7, app.id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Step error (update): " << sqlite3_errmsg(db) << '\n';
        return rollbackTransaction();
    }

    return commitTransaction();
}

bool DatabaseManager::deleteApplication(int id) {
    if (!beginTransaction()) return false;

    const char* sql = "DELETE FROM APPLICATION_TRACKER WHERE ID=?;";

    StmtGuard stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare error (delete): " << sqlite3_errmsg(db) << '\n';
        return rollbackTransaction();
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Step error (delete): " << sqlite3_errmsg(db) << '\n';
        return rollbackTransaction();
    }

    return commitTransaction();
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

std::vector<Application> DatabaseManager::getAllApplications() const {
    const char* sql =
        "SELECT ID, COMPANY_NAME, APPLICATION_DATE, POSITION, CONTACT_PERSON, STATUS, NOTES "
        "FROM APPLICATION_TRACKER ORDER BY ID DESC;";

    StmtGuard stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare error (getAll): " << sqlite3_errmsg(db) << '\n';
        return {};
    }

    std::vector<Application> results;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        results.push_back(readAppRow(stmt));
    return results;
}

std::vector<Application> DatabaseManager::searchApplications(const std::string& query) const {
    const char* sql =
        "SELECT ID, COMPANY_NAME, APPLICATION_DATE, POSITION, CONTACT_PERSON, STATUS, NOTES "
        "FROM APPLICATION_TRACKER "
        "WHERE COMPANY_NAME LIKE ? OR POSITION LIKE ? OR CONTACT_PERSON LIKE ? "
        "   OR STATUS LIKE ? OR NOTES LIKE ? OR APPLICATION_DATE LIKE ? "
        "ORDER BY ID DESC;";

    StmtGuard stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare error (search): " << sqlite3_errmsg(db) << '\n';
        return {};
    }

    const std::string wildcard = "%" + query + "%";
    for (int i = 1; i <= 6; ++i)
        sqlite3_bind_text(stmt, i, wildcard.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<Application> results;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        results.push_back(readAppRow(stmt));
    return results;
}

int DatabaseManager::getTotalApplications() const {
    const char* sql = "SELECT COUNT(*) FROM APPLICATION_TRACKER;";
    StmtGuard stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr) != SQLITE_OK) return 0;
    return (sqlite3_step(stmt) == SQLITE_ROW) ? sqlite3_column_int(stmt, 0) : 0;
}

int DatabaseManager::getApplicationsByStatus(const std::string& status) const {
    const char* sql = "SELECT COUNT(*) FROM APPLICATION_TRACKER WHERE STATUS=?;";
    StmtGuard stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr) != SQLITE_OK) return 0;
    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    return (sqlite3_step(stmt) == SQLITE_ROW) ? sqlite3_column_int(stmt, 0) : 0;
}

int DatabaseManager::getInterviewsTotal() const {
    // Historical tests expect a specialized method to return the total
    // number of applications currently in the "Interviews" status.
    return getApplicationsByStatus("Interviews");
}

bool DatabaseManager::isDuplicateEntry(const Application& app) const {
    const char* sql = "SELECT COUNT(*) FROM APPLICATION_TRACKER "
                      "WHERE COMPANY_NAME=? AND APPLICATION_DATE=? AND POSITION=? "
                      "AND CONTACT_PERSON=? AND STATUS=? AND NOTES=?;";
    StmtGuard stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, app.companyName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, app.applicationDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, app.position.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, app.contactPerson.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, app.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, app.notes.c_str(), -1, SQLITE_TRANSIENT);

    int count = (sqlite3_step(stmt) == SQLITE_ROW) ? sqlite3_column_int(stmt, 0) : 0;
    return count > 0;
}

std::vector<std::pair<std::string, int>> DatabaseManager::getStatusCounts() const {
    const char* sql =
        "SELECT STATUS, COUNT(*) FROM APPLICATION_TRACKER "
        "GROUP BY STATUS ORDER BY COUNT(*) DESC;";

    StmtGuard stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr) != SQLITE_OK) return {};

    std::vector<std::pair<std::string, int>> counts;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string s = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int         c = sqlite3_column_int(stmt, 1);
        counts.emplace_back(std::move(s), c);
    }
    return counts;
}

// ---------------------------------------------------------------------------
// Validation (static, no DB access)
// ---------------------------------------------------------------------------

bool DatabaseManager::isValidDate(const std::string& date) {
    static const std::regex kPattern(
        R"(^\d{4}-(0[1-9]|1[0-2])-(0[1-9]|[12]\d|3[01])$)");

    if (!std::regex_match(date, kPattern)) return false;

    QDate d = QDate::fromString(QString::fromStdString(date), Qt::ISODate);
    return d.isValid() && d <= QDate::currentDate();
}
bool DatabaseManager::isValidDateTime(const std::string& dateTime) {
    // Alias for historical naming (used by tests and older code paths)
    return isValidDate(dateTime);
}
// ---------------------------------------------------------------------------
// Schema versioning
// ---------------------------------------------------------------------------

bool DatabaseManager::getSchemaVersion(int& version) const {
    version = 0;
    const char* sql = "SELECT VERSION FROM SCHEMA_VERSION LIMIT 1;";
    StmtGuard stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr) != SQLITE_OK) return false;
    if (sqlite3_step(stmt) != SQLITE_ROW) return false;
    version = sqlite3_column_int(stmt, 0);
    return true;
}

bool DatabaseManager::setSchemaVersion(int version) {
    const char* sql = "UPDATE SCHEMA_VERSION SET VERSION=?;";
    StmtGuard stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt.stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, version);
    return sqlite3_step(stmt) == SQLITE_DONE;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void DatabaseManager::bindAppFields(sqlite3_stmt* stmt, const Application& app) const {
    sqlite3_bind_text(stmt, 1, app.companyName.c_str(),      -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, app.applicationDate.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, app.position.c_str(),         -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, app.contactPerson.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, app.status.c_str(),           -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, app.notes.c_str(),            -1, SQLITE_TRANSIENT);
}

Application DatabaseManager::readAppRow(sqlite3_stmt* stmt) {
    Application app;
    app.id              = sqlite3_column_int (stmt, 0);
    app.companyName     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    app.applicationDate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    app.position        = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    app.contactPerson   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    app.status          = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    app.notes           = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    return app;
}