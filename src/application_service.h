#ifndef APPLICATION_SERVICE_H
#define APPLICATION_SERVICE_H

#include <QString>
#include <string>
#include <vector>
#include <utility>
#include "db_manager.h"

// ---------------------------------------------------------------------------
// ApplicationAnalytics – aggregated read-only snapshot
// ---------------------------------------------------------------------------
struct ApplicationAnalytics {
    int total               = 0;
    int rejected            = 0;
    int interviews          = 0;
    int offers              = 0;
    std::vector<std::pair<std::string, int>> statusCounts;
};

// ---------------------------------------------------------------------------
// ApplicationService – business logic layer
//
// Owns no data itself; delegates all persistence to DatabaseManager.
// Returns {bool, message} pairs so callers always know why an operation failed.
// ---------------------------------------------------------------------------
class ApplicationService {
public:
    explicit ApplicationService(DatabaseManager* db);

    // --- Write operations ---
    std::pair<bool, std::string> addApplication   (const Application& app);
    std::pair<bool, std::string> updateApplication(const Application& app);
    std::pair<bool, std::string> deleteApplication(int id);

    // --- Read operations ---
    std::vector<Application> listApplications()                        const;
    std::vector<Application> searchApplications(const QString& query) const;
    ApplicationAnalytics     getAnalytics()                            const;

private:
    // Returns true when app data is acceptable; fills `error` on failure.
    bool validateApplication(const Application& app, std::string& error) const;

    // Checks for an exact duplicate (same company + date + position + contact + status).
    bool isDuplicate(const Application& app) const;

    DatabaseManager* m_db; // non-owning
};

#endif // APPLICATION_SERVICE_H