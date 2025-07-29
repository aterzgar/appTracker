#include <QtTest/QtTest>
#include <QTemporaryDir>
#include "db_manager.h"

class TestAnalytics : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Analytics calculation tests
    void testEmptyDatabaseAnalytics();
    void testSingleApplicationAnalytics();
    void testMultipleApplicationsAnalytics();
    void testStatusDistributionAnalytics();
    void testInterviewCalculations();
    void testRejectionRateCalculations();
    void testOfferSuccessRate();

    // Edge case tests
    void testAnalyticsWithAllSameStatus();
    void testAnalyticsWithUnknownStatus();
    void testAnalyticsAfterDeletion();
    void testAnalyticsAfterUpdate();

private:
    DatabaseManager* dbManager;
    QTemporaryDir* tempDir;
    QString testDbPath;

    Application createApplication(const std::string& company,
                                const std::string& status,
                                const std::string& date = "2024-01-15");
    void addTestApplications();
    void verifyStatusCount(const std::vector<std::pair<std::string, int>>& statusCounts,
                          const std::string& status, int expectedCount);
};

void TestAnalytics::initTestCase()
{
    tempDir = new QTemporaryDir();
    QVERIFY(tempDir->isValid());
}

void TestAnalytics::cleanupTestCase()
{
    delete tempDir;
}

void TestAnalytics::init()
{
    dbManager = new DatabaseManager();
    testDbPath = tempDir->path() + "/analytics_test.db";
    QVERIFY(dbManager->openDatabase(testDbPath.toStdString()));
}

void TestAnalytics::cleanup()
{
    if (dbManager) {
        dbManager->closeDatabase();
        delete dbManager;
        dbManager = nullptr;
    }
    QFile::remove(testDbPath);
}

Application TestAnalytics::createApplication(const std::string& company,
                                           const std::string& status,
                                           const std::string& date)
{
    Application app;
    app.companyName = company;
    app.applicationDate = date;
    app.position = "Software Engineer";
    app.contactPerson = "Contact Person";
    app.status = status;
    app.notes = "Test notes";
    return app;
}

void TestAnalytics::addTestApplications()
{
    // Add a diverse set of applications for testing
    std::vector<Application> apps = {
        createApplication("Company A", "Applied", "2024-01-01"),
        createApplication("Company B", "Applied", "2024-01-02"),
        createApplication("Company C", "Interview Scheduled", "2024-01-03"),
        createApplication("Company D", "Interview Completed", "2024-01-04"),
        createApplication("Company E", "Offer Received", "2024-01-05"),
        createApplication("Company F", "Rejected", "2024-01-06"),
        createApplication("Company G", "Rejected", "2024-01-07"),
        createApplication("Company H", "Withdrawn", "2024-01-08"),
        createApplication("Company I", "Interview Scheduled", "2024-01-09"),
        createApplication("Company J", "Applied", "2024-01-10")
    };
    
    for (const auto& app : apps) {
        QVERIFY(dbManager->addApplication(app));
    }
}

void TestAnalytics::verifyStatusCount(const std::vector<std::pair<std::string, int>>& statusCounts,
                                    const std::string& status, int expectedCount)
{
    bool found = false;
    for (const auto& pair : statusCounts) {
        if (pair.first == status) {
            QCOMPARE(pair.second, expectedCount);
            found = true;
            break;
        }
    }
    if (expectedCount > 0) {
        QVERIFY2(found, QString("Status '%1' not found in status counts").arg(QString::fromStdString(status)).toLocal8Bit());
    }
}

void TestAnalytics::testEmptyDatabaseAnalytics()
{
    // Test analytics on empty database
    QCOMPARE(dbManager->getTotalApplications(), 0);
    QCOMPARE(dbManager->getApplicationsByStatus("Applied"), 0);
    QCOMPARE(dbManager->getApplicationsByStatus("Rejected"), 0);
    QCOMPARE(dbManager->getApplicationsByStatus("Interview Scheduled"), 0);
    QCOMPARE(dbManager->getApplicationsByStatus("Offer Received"), 0);
    
    auto statusCounts = dbManager->getStatusCounts();
    QCOMPARE(statusCounts.size(), 0);
}

void TestAnalytics::testSingleApplicationAnalytics()
{
    Application app = createApplication("Single Company", "Applied");
    QVERIFY(dbManager->addApplication(app));
    
    QCOMPARE(dbManager->getTotalApplications(), 1);
    QCOMPARE(dbManager->getApplicationsByStatus("Applied"), 1);
    QCOMPARE(dbManager->getApplicationsByStatus("Rejected"), 0);
    
    auto statusCounts = dbManager->getStatusCounts();
    QCOMPARE(statusCounts.size(), 1);
    QCOMPARE(statusCounts[0].first, std::string("Applied"));
    QCOMPARE(statusCounts[0].second, 1);
}

void TestAnalytics::testMultipleApplicationsAnalytics()
{
    addTestApplications();
    
    // Total should be 10
    QCOMPARE(dbManager->getTotalApplications(), 10);
    
    // Verify individual status counts
    QCOMPARE(dbManager->getApplicationsByStatus("Applied"), 3);
    QCOMPARE(dbManager->getApplicationsByStatus("Interview Scheduled"), 2);
    QCOMPARE(dbManager->getApplicationsByStatus("Interview Completed"), 1);
    QCOMPARE(dbManager->getApplicationsByStatus("Offer Received"), 1);
    QCOMPARE(dbManager->getApplicationsByStatus("Rejected"), 2);
    QCOMPARE(dbManager->getApplicationsByStatus("Withdrawn"), 1);
    
    // Test non-existent status
    QCOMPARE(dbManager->getApplicationsByStatus("Non-existent"), 0);
}

void TestAnalytics::testStatusDistributionAnalytics()
{
    addTestApplications();
    
    auto statusCounts = dbManager->getStatusCounts();
    
    // Should have 6 different statuses
    QCOMPARE(statusCounts.size(), 6);
    
    // Verify each status count
    verifyStatusCount(statusCounts, "Applied", 3);
    verifyStatusCount(statusCounts, "Interview Scheduled", 2);
    verifyStatusCount(statusCounts, "Interview Completed", 1);
    verifyStatusCount(statusCounts, "Offer Received", 1);
    verifyStatusCount(statusCounts, "Rejected", 2);
    verifyStatusCount(statusCounts, "Withdrawn", 1);
    
    // Verify ordering (should be ordered by count DESC)
    // The first entry should have the highest count
    QVERIFY(statusCounts[0].second >= statusCounts[1].second);
    if (statusCounts.size() > 2) {
        QVERIFY(statusCounts[1].second >= statusCounts[2].second);
    }
}

void TestAnalytics::testInterviewCalculations()
{
    addTestApplications();
    
    // Calculate total interviews (Interview Scheduled + Interview Completed)
    int interviewScheduled = dbManager->getApplicationsByStatus("Interview Scheduled");
    int interviewCompleted = dbManager->getApplicationsByStatus("Interview Completed");
    int totalInterviews = interviewScheduled + interviewCompleted;
    
    QCOMPARE(interviewScheduled, 2);
    QCOMPARE(interviewCompleted, 1);
    QCOMPARE(totalInterviews, 3);
    
    // Interview rate calculation (interviews / total applications)
    int total = dbManager->getTotalApplications();
    double interviewRate = static_cast<double>(totalInterviews) / total;
    QCOMPARE(interviewRate, 0.3); // 3/10 = 0.3 or 30%
}

void TestAnalytics::testRejectionRateCalculations()
{
    addTestApplications();
    
    int rejected = dbManager->getApplicationsByStatus("Rejected");
    int total = dbManager->getTotalApplications();
    
    QCOMPARE(rejected, 2);
    QCOMPARE(total, 10);
    
    // Rejection rate calculation
    double rejectionRate = static_cast<double>(rejected) / total;
    QCOMPARE(rejectionRate, 0.2); // 2/10 = 0.2 or 20%
}

void TestAnalytics::testOfferSuccessRate()
{
    addTestApplications();
    
    int offers = dbManager->getApplicationsByStatus("Offer Received");
    int total = dbManager->getTotalApplications();
    
    QCOMPARE(offers, 1);
    QCOMPARE(total, 10);
    
    // Offer success rate calculation
    double offerRate = static_cast<double>(offers) / total;
    QCOMPARE(offerRate, 0.1); // 1/10 = 0.1 or 10%
}

void TestAnalytics::testAnalyticsWithAllSameStatus()
{
    // Add multiple applications with the same status
    for (int i = 0; i < 5; ++i) {
        Application app = createApplication("Company " + std::to_string(i), "Applied");
        QVERIFY(dbManager->addApplication(app));
    }
    
    QCOMPARE(dbManager->getTotalApplications(), 5);
    QCOMPARE(dbManager->getApplicationsByStatus("Applied"), 5);
    QCOMPARE(dbManager->getApplicationsByStatus("Rejected"), 0);
    
    auto statusCounts = dbManager->getStatusCounts();
    QCOMPARE(statusCounts.size(), 1);
    QCOMPARE(statusCounts[0].first, std::string("Applied"));
    QCOMPARE(statusCounts[0].second, 5);
}

void TestAnalytics::testAnalyticsWithUnknownStatus()
{
    // Add application with custom status
    Application app = createApplication("Test Company", "Custom Status");
    QVERIFY(dbManager->addApplication(app));
    
    QCOMPARE(dbManager->getTotalApplications(), 1);
    QCOMPARE(dbManager->getApplicationsByStatus("Custom Status"), 1);
    QCOMPARE(dbManager->getApplicationsByStatus("Applied"), 0);
    
    auto statusCounts = dbManager->getStatusCounts();
    QCOMPARE(statusCounts.size(), 1);
    QCOMPARE(statusCounts[0].first, std::string("Custom Status"));
    QCOMPARE(statusCounts[0].second, 1);
}

void TestAnalytics::testAnalyticsAfterDeletion()
{
    addTestApplications();
    
    // Initial state
    QCOMPARE(dbManager->getTotalApplications(), 10);
    QCOMPARE(dbManager->getApplicationsByStatus("Applied"), 3);
    
    // Get an application to delete
    auto applications = dbManager->getAllApplications();
    QVERIFY(!applications.empty());
    
    // Find and delete an "Applied" application
    int deletedId = -1;
    for (const auto& app : applications) {
        if (app.status == "Applied") {
            deletedId = app.id;
            break;
        }
    }
    
    QVERIFY(deletedId != -1);
    QVERIFY(dbManager->deleteApplication(deletedId));
    
    // Verify analytics updated
    QCOMPARE(dbManager->getTotalApplications(), 9);
    QCOMPARE(dbManager->getApplicationsByStatus("Applied"), 2);
}

void TestAnalytics::testAnalyticsAfterUpdate()
{
    addTestApplications();
    
    // Initial state
    QCOMPARE(dbManager->getApplicationsByStatus("Applied"), 3);
    QCOMPARE(dbManager->getApplicationsByStatus("Interview Scheduled"), 2);
    
    // Get an application to update
    auto applications = dbManager->getAllApplications();
    QVERIFY(!applications.empty());
    
    // Find an "Applied" application and update it to "Interview Scheduled"
    Application appToUpdate;
    bool found = false;
    for (const auto& app : applications) {
        if (app.status == "Applied") {
            appToUpdate = app;
            found = true;
            break;
        }
    }
    
    QVERIFY(found);
    appToUpdate.status = "Interview Scheduled";
    QVERIFY(dbManager->updateApplication(appToUpdate));
    
    // Verify analytics updated
    QCOMPARE(dbManager->getApplicationsByStatus("Applied"), 2);
    QCOMPARE(dbManager->getApplicationsByStatus("Interview Scheduled"), 3);
    QCOMPARE(dbManager->getTotalApplications(), 10); // Total should remain the same
}

#include "test_analytics.moc"

// Main function for this test
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    TestAnalytics test;
    return QTest::qExec(&test, argc, argv);
}