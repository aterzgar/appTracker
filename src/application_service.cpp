#include "application_service.h"

#include <sstream>
#include <algorithm>
#include <numeric>
#include <map>
#include <set>
#include <ctime>

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
// LLM prompt builder
// ---------------------------------------------------------------------------

std::string ApplicationService::buildInsightsPrompt(
    const ApplicationAnalytics& analytics,
    const std::vector<Application>& apps) const
{
    std::ostringstream ss;

    // --- Core metrics ---
    ss << "=== Job Application Analytics ===\n\n";
    ss << "Total applications: " << analytics.total << "\n";

    if (analytics.total > 0) {
        const double interviewRate = static_cast<double>(analytics.interviews) / analytics.total * 100.0;
        const double offerRate     = static_cast<double>(analytics.offers)     / analytics.total * 100.0;
        const double rejectionRate = static_cast<double>(analytics.rejected)   / analytics.total * 100.0;

        ss << "Interview rate: "   << static_cast<int>(interviewRate) << "%\n";
        ss << "Offer rate: "       << static_cast<int>(offerRate)     << "%\n";
        ss << "Rejection rate: "   << static_cast<int>(rejectionRate) << "%\n";
    } else {
        ss << "Interview rate: N/A (no applications)\n";
        ss << "Offer rate: N/A (no applications)\n";
        ss << "Rejection rate: N/A (no applications)\n";
    }

    // --- Status distribution ---
    ss << "\nStatus distribution:\n";
    for (const auto& [status, count] : analytics.statusCounts) {
        ss << "  " << status << ": " << count << "\n";
    }

    // --- Company analysis ---
    std::map<std::string, int> companyCounts;
    std::map<std::string, int> companyInterviews;
    std::map<std::string, int> companyRejections;
    for (const auto& app : apps) {
        companyCounts[app.companyName]++;
        if (app.status == "Interviews")     companyInterviews[app.companyName]++;
        if (app.status == "Rejected")       companyRejections[app.companyName]++;
    }

    if (!companyCounts.empty()) {
        ss << "\nTop companies (by application count):\n";
        // Sort by count descending
        std::vector<std::pair<std::string, int>> sorted(
            companyCounts.begin(), companyCounts.end());
        std::sort(sorted.begin(), sorted.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        const int show = std::min(static_cast<int>(sorted.size()), 5);
        for (int i = 0; i < show; ++i) {
            const int interviews = companyInterviews[sorted[i].first];
            const int rejections = companyRejections[sorted[i].first];
            ss << "  " << sorted[i].first
               << " (" << sorted[i].second << " applied"
               << ", " << interviews     << " interviews"
               << ", " << rejections     << " rejected)\n";
        }
    }

    // --- Position analysis ---
    std::map<std::string, int> positionInterviews;
    std::map<std::string, int> positionRejections;
    std::map<std::string, int> positionOffers;
    std::map<std::string, int> positionTotal;
    for (const auto& app : apps) {
        positionTotal[app.position]++;
        if (app.status == "Interviews")     positionInterviews[app.position]++;
        if (app.status == "Rejected")       positionRejections[app.position]++;
        if (app.status == "Offer Received") positionOffers[app.position]++;
    }

    if (!positionTotal.empty()) {
        ss << "\nPosition performance:\n";
        for (const auto& [pos, total] : positionTotal) {
            const int interviews = positionInterviews[pos];
            const int rejections = positionRejections[pos];
            const int offers     = positionOffers[pos];
            ss << "  " << pos
               << " (" << total      << " applied"
               << ", " << interviews << " interviews"
               << ", " << offers     << " offers"
               << ", " << rejections << " rejected)\n";
        }
    }

    // --- Recent trend (last 7 days vs previous 7 days) ---
    std::time_t now = std::time(nullptr);
    std::tm* local  = std::localtime(&now);
    char today[11];
    std::strftime(today, sizeof(today), "%Y-%m-%d", local);
    const std::string todayStr(today);

    local->tm_mday -= 7;
    std::mktime(local);
    char weekAgo[11];
    std::strftime(weekAgo, sizeof(weekAgo), "%Y-%m-%d", local);
    const std::string weekAgoStr(weekAgo);

    local->tm_mday -= 7;
    std::mktime(local);
    char twoWeeksAgo[11];
    std::strftime(twoWeeksAgo, sizeof(twoWeeksAgo), "%Y-%m-%d", local);
    const std::string twoWeeksAgoStr(twoWeeksAgo);

    int thisWeek  = 0;
    int lastWeek  = 0;
    for (const auto& app : apps) {
        if (app.applicationDate >= weekAgoStr && app.applicationDate <= todayStr)
            thisWeek++;
        else if (app.applicationDate >= twoWeeksAgoStr && app.applicationDate < weekAgoStr)
            lastWeek++;
    }

    ss << "\nApplication velocity:\n";
    ss << "  Last 7 days: "  << thisWeek  << " applications\n";
    ss << "  Previous 7 days: " << lastWeek << " applications\n";
    if (lastWeek > 0 && thisWeek > 0) {
        const double change = static_cast<double>(thisWeek - lastWeek) / lastWeek * 100.0;
        ss << "  Trend: " << (change >= 0 ? "+" : "") << static_cast<int>(change) << "%\n";
    } else if (thisWeek > 0 && lastWeek == 0) {
        ss << "  Trend: New activity this week\n";
    } else {
        ss << "  Trend: Insufficient data\n";
    }

    ss << "\n---\nAnalyze these metrics and provide actionable insights for the job seeker.\n";
    ss << "Highlight strengths, weaknesses, and specific recommendations based on the data.\n";

    return ss.str();
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