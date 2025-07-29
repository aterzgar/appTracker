#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <string>
#include <vector>

// Forward declaration is not enough for pointer manipulation with sqlite3;
// we need to include it for sqlite3* and binding.
#include <sqlite3.h>

/**
 * @brief Data structure for a single job application record.
 */
struct Application {
    int id = 0;
    std::string companyName;
    std::string applicationDate;
    std::string position;
    std::string contactPerson;
    std::string status;
    std::string notes;
};

/**
 * @brief Handles SQLite storage and retrieval for job applications.
 */
class DatabaseManager {
public:
    explicit DatabaseManager();
    ~DatabaseManager();

    bool openDatabase(const std::string& dbPath);
    void closeDatabase();

    bool addApplication(const Application& app);
    std::vector<Application> getAllApplications();
    bool deleteApplication(int id);
    bool updateApplication(const Application& app);

    bool isValidDateTime(const std::string& date) const;
    bool isDuplicateEntry(const Application& app) const;
    
    // Analytics methods
    int getTotalApplications() const;
    int getApplicationsByStatus(const std::string& status) const;
    std::vector<std::pair<std::string, int>> getStatusCounts() const;

private:
    // Low-level table creation
    bool createTables();

    // Field binding helpers for queries
    void bindCommonFields(sqlite3_stmt* stmt, const Application& app) const;
    void bindCommonFieldsForDuplicate(sqlite3_stmt* stmt, const Application& app) const;

    sqlite3* db = nullptr;
};

#endif // DB_MANAGER_H