#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <string>
#include <vector>
#include <utility>
#include <sqlite3.h>

// ---------------------------------------------------------------------------
// Application – plain data record
// ---------------------------------------------------------------------------
struct Application {
    int         id              = 0;
    std::string companyName;
    std::string applicationDate;
    std::string position;
    std::string contactPerson;
    std::string status;
    std::string notes;
};

// ---------------------------------------------------------------------------
// DatabaseManager – thin SQLite wrapper
//
// Responsibilities:
//   • Lifecycle  : open / close the database file
//   • Schema     : create tables, indexes, schema-version tracking
//   • CRUD       : add / get / update / delete Application rows
//   • Queries    : search, status counts, total count
//
// NOT responsible for:
//   • Business validation (done in ApplicationService)
//   • Duplicate detection at the business level (done in ApplicationService)
// ---------------------------------------------------------------------------
class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();

    // Non-copyable, non-movable (owns a raw sqlite3* handle)
    DatabaseManager(const DatabaseManager&)            = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    // --- Lifecycle ---
    bool openDatabase(const std::string& dbPath);
    void closeDatabase();
    bool isOpen() const { return db != nullptr; }

    // --- CRUD ---
    bool addApplication(const Application& app);
    bool updateApplication(const Application& app);
    bool deleteApplication(int id);

    // --- Queries ---
    std::vector<Application>                    getAllApplications() const;
    std::vector<Application>                    searchApplications(const std::string& query) const;
    std::vector<std::pair<std::string, int>>    getStatusCounts() const;
    int                                         getTotalApplications() const;
    int                                         getApplicationsByStatus(const std::string& status) const;
    int                                         getApplicationsEverInStatus(const std::string& status) const;
    int                                         getInterviewsTotal() const;
    bool                                        isDuplicateEntry(const Application& app) const;

    // --- Validation helpers (pure, no DB access) ---
    static bool isValidDate(const std::string& date);
    static bool isValidDateTime(const std::string& dateTime);

    // --- Schema versioning ---
    bool getSchemaVersion(int& version) const;
    bool setSchemaVersion(int version);

private:
    // Schema
    bool createTables();

    // Status history logging (called internally on add/update)
    bool logStatusChange(int applicationId, const std::string& newStatus, const std::string& oldStatus);

    // Transaction helpers
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // Bind helpers
    void bindAppFields(sqlite3_stmt* stmt, const Application& app) const;

    // Row reader
    static Application readAppRow(sqlite3_stmt* stmt);

    sqlite3* db = nullptr;
};

#endif // DB_MANAGER_H