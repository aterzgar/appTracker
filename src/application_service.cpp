#include "application_service.h"

#include <sstream>
#include <algorithm>
#include <numeric>
#include <map>
#include <set>
#include <ctime>
#include <iomanip>

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
    std::time_t now = std::time(nullptr);

    // --- Precompute age buckets ---
    int recent0_3  = 0;
    int recent4_10 = 0;
    int recent11_13 = 0;
    int older14    = 0;
    int oldPending = 0;

    for (const auto& app : apps) {
        std::tm tm = {};
        std::istringstream ssDate(app.applicationDate);
        ssDate >> std::get_time(&tm, "%Y-%m-%d");
        tm.tm_isdst = -1;

        std::time_t appTime = std::mktime(&tm);
        if (appTime == -1) continue; // skip malformed dates

        double days = std::difftime(now, appTime) / (60.0 * 60.0 * 24.0);

        if      (days <= 3)  recent0_3++;
        else if (days <= 10) recent4_10++;
        else if (days <= 13) recent11_13++;
        else                 older14++;

        if (days >= 14 && app.status == "Applied") oldPending++;
    }

    // --- Compute date strings for velocity (thread-safe) ---
    std::tm localNow = {};
#if defined(_WIN32)
    localtime_s(&localNow, &now);
#else
    localtime_r(&now, &localNow);
#endif

    char today[11];
    std::strftime(today, sizeof(today), "%Y-%m-%d", &localNow);
    const std::string todayStr(today);

    std::tm localWeekAgo = localNow;
    localWeekAgo.tm_mday -= 7;
    std::mktime(&localWeekAgo);
    char weekAgo[11];
    std::strftime(weekAgo, sizeof(weekAgo), "%Y-%m-%d", &localWeekAgo);
    const std::string weekAgoStr(weekAgo);

    std::tm localTwoWeeksAgo = localWeekAgo;
    localTwoWeeksAgo.tm_mday -= 7;
    std::mktime(&localTwoWeeksAgo);
    char twoWeeksAgo[11];
    std::strftime(twoWeeksAgo, sizeof(twoWeeksAgo), "%Y-%m-%d", &localTwoWeeksAgo);
    const std::string twoWeeksAgoStr(twoWeeksAgo);

    int thisWeek = 0;
    int lastWeek = 0;
    for (const auto& app : apps) {
        if (app.applicationDate >= weekAgoStr && app.applicationDate <= todayStr)
            thisWeek++;
        else if (app.applicationDate >= twoWeeksAgoStr && app.applicationDate < weekAgoStr)
            lastWeek++;
    }

    // --- Precompute company and position maps ---
    std::map<std::string, int> companyCounts;
    std::map<std::string, int> companyInterviews;
    std::map<std::string, int> companyRejections;
    std::map<std::string, int> positionTotal;
    std::map<std::string, int> positionInterviews;
    std::map<std::string, int> positionRejections;
    std::map<std::string, int> positionOffers;

    for (const auto& app : apps) {
        companyCounts[app.companyName]++;
        if (app.status == "Interviews")     companyInterviews[app.companyName]++;
        if (app.status == "Rejected")       companyRejections[app.companyName]++;

        positionTotal[app.position]++;
        if (app.status == "Interviews")     positionInterviews[app.position]++;
        if (app.status == "Rejected")       positionRejections[app.position]++;
        if (app.status == "Offer Received") positionOffers[app.position]++;
    }

    // --- Precompute rates ---
    const double interviewRate = analytics.total > 0
        ? static_cast<double>(analytics.interviews) / analytics.total * 100.0 : 0.0;
    const double offerRate = analytics.total > 0
        ? static_cast<double>(analytics.offers) / analytics.total * 100.0 : 0.0;
    const double rejectionRate = analytics.total > 0
        ? static_cast<double>(analytics.rejected) / analytics.total * 100.0 : 0.0;
    const double interviewToOfferRate = analytics.interviews > 0
        ? static_cast<double>(analytics.offers) / analytics.interviews * 100.0 : 0.0;

    // ===========================================================
    // BEGIN PROMPT
    // ===========================================================

    // --- Persona ---
    ss << "You are a career coach analyzing a job seeker's application funnel data.\n";
    ss << "Your goal is to identify what is and isn't working, and give concrete next steps.\n\n";

    // --- Early caveat (high weight position) ---
    ss << "IMPORTANT CONTEXT (read before analyzing):\n";
    ss << "- Applications submitted 0-3 days ago are too recent to evaluate. "
       << "Do not treat silence as rejection for these.\n";
    ss << "- Typical hiring response time is 7-14 days.\n";
    ss << "- " << recent0_3 << " application(s) are under 3 days old — exclude them from negative signals.\n\n";

    // --- Core metrics ---
    ss << "=== Job Application Analytics ===\n\n";
    ss << "Total applications: " << analytics.total << "\n";

    if (analytics.total > 0) {
        ss << std::fixed << std::setprecision(1);
        ss << "Interview rate:  " << interviewRate  << "% (industry benchmark: 20-30%)\n";
        ss << "Offer rate:      " << offerRate      << "% (industry benchmark: 5-10%)\n";
        ss << "Rejection rate:  " << rejectionRate  << "%\n";
    } else {
        ss << "Interview rate:  N/A (no applications)\n";
        ss << "Offer rate:      N/A (no applications)\n";
        ss << "Rejection rate:  N/A (no applications)\n";
    }

    // --- Conversion funnel ---
    ss << "\nConversion funnel:\n";
    ss << "  Applied → Interview: " << std::fixed << std::setprecision(1) << interviewRate << "%\n";
    ss << "  Interview → Offer:   ";
    if (analytics.interviews > 0)
        ss << interviewToOfferRate << "%\n";
    else
        ss << "N/A (no interviews yet)\n";

    // --- Application age buckets ---
    ss << "\nApplication age distribution:\n";
    ss << "  0-3 days:   " << recent0_3   << " (too recent to evaluate)\n";
    ss << "  4-10 days:  " << recent4_10  << "\n";
    ss << "  11-13 days: " << recent11_13 << "\n";
    ss << "  14+ days:   " << older14     << "\n";
    ss << "  14+ days old, still no response (Applied status): " << oldPending << "\n";

    // --- Status distribution ---
    ss << "\nStatus distribution:\n";
    for (const auto& [status, count] : analytics.statusCounts) {
        ss << "  " << status << ": " << count << "\n";
    }

    // --- Company analysis ---
    if (!companyCounts.empty()) {
        ss << "\nTop companies (by application count):\n";
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
               << ", " << interviews << " interviews"
               << ", " << rejections << " rejected)\n";
        }
    }

    // --- Position analysis ---
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

    // --- Application velocity ---
    ss << "\nApplication velocity:\n";
    ss << "  Last 7 days:     " << thisWeek << " applications\n";
    ss << "  Previous 7 days: " << lastWeek << " applications\n";
    if (lastWeek > 0 && thisWeek > 0) {
        const double change = static_cast<double>(thisWeek - lastWeek) / lastWeek * 100.0;
        ss << "  Trend: " << (change >= 0 ? "+" : "") << static_cast<int>(change) << "%\n";
    } else if (thisWeek > 0 && lastWeek == 0) {
        ss << "  Trend: New activity this week\n";
    } else {
        ss << "  Trend: Insufficient data\n";
    }

    // --- Late caveat reinforcement + structured output instructions ---
    ss << "\n---\n";
    ss << "Reminder: " << recent0_3 << " application(s) are under 3 days old. "
       << "Do not count silence from these as a negative signal.\n\n";

    ss << "Respond in exactly this structure (max 250 words total):\n\n";
    ss << "1. Funnel Health — 1-2 sentences on overall conversion rates vs benchmarks.\n";
    ss << "2. Top Bottleneck — The single biggest issue, citing specific numbers from the data.\n";
    ss << "3. What's Working — One positive signal from the data.\n";
    ss << "4. This Week's Actions — 2-3 specific, concrete steps. No generic advice.\n\n";
    ss << "Be direct. No filler sentences.\n";

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