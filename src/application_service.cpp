#include "application_service.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

ApplicationService::ApplicationService(DatabaseManager* db)
    : m_db(db)
{}

// ---------------------------------------------------------------------------
// Write operations
// ---------------------------------------------------------------------------

std::pair<bool, std::string> ApplicationService::addApplication(const Application& app) {
    if (!m_db) return {false, "Database unavailable."};

    std::string error;
    if (!validateApplication(app, error))   return {false, error};
    if (isDuplicate(app))                   return {false, "A duplicate application already exists."};
    if (!m_db->addApplication(app))         return {false, "Failed to save the application."};

    return {true, {}};
}

std::pair<bool, std::string> ApplicationService::updateApplication(const Application& app) {
    if (!m_db) return {false, "Database unavailable."};

    std::string error;
    if (!validateApplication(app, error))   return {false, error};
    if (!m_db->updateApplication(app))      return {false, "Failed to update the application."};

    return {true, {}};
}

std::pair<bool, std::string> ApplicationService::deleteApplication(int id) {
    if (!m_db)                          return {false, "Database unavailable."};
    if (!m_db->deleteApplication(id))   return {false, "Failed to delete the application."};
    return {true, {}};
}

// ---------------------------------------------------------------------------
// Read operations
// ---------------------------------------------------------------------------

std::vector<Application> ApplicationService::listApplications() const {
    if (!m_db) return {};
    return m_db->getAllApplications();
}

std::vector<Application> ApplicationService::searchApplications(const QString& query) const {
    if (!m_db) return {};

    const QString trimmed = query.trimmed();
    if (trimmed.isEmpty()) return m_db->getAllApplications();

    return m_db->searchApplications(trimmed.toStdString());
}

ApplicationAnalytics ApplicationService::getAnalytics() const {
    ApplicationAnalytics a;
    if (!m_db) return a;

    a.total              = m_db->getTotalApplications();
    a.rejected           = m_db->getApplicationsByStatus("Rejected");
    a.interviews         = m_db->getApplicationsByStatus("Interviews");
    a.offers             = m_db->getApplicationsByStatus("Offer Received");
    a.statusCounts       = m_db->getStatusCounts();

    return a;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool ApplicationService::validateApplication(const Application& app, std::string& error) const {
    if (app.companyName.empty() || app.position.empty() || app.contactPerson.empty()) {
        error = "Please fill in all required fields: Company, Position, Contact Person.";
        return false;
    }

    if (!DatabaseManager::isValidDate(app.applicationDate)) {
        error = "The application date is invalid or is set in the future.";
        return false;
    }

    return true;
}

bool ApplicationService::isDuplicate(const Application& app) const {
    // Re-uses the search query: look for an exact match on the five key fields.
    // We do this at the service layer so DatabaseManager stays a pure persistence layer.
    for (const auto& existing : m_db->getAllApplications()) {
        if (existing.companyName     == app.companyName     &&
            existing.applicationDate == app.applicationDate &&
            existing.position        == app.position        &&
            existing.contactPerson   == app.contactPerson   &&
            existing.status          == app.status)
        {
            return true;
        }
    }
    return false;
}