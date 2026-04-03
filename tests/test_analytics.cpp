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
                                const std::string& date = "2024-01-15",
                                const std::string& position = "Software Engineer");
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
                                           const std::string& date,
                                           const std::string& position)
{
    Application app;
    app.companyName = company;
    app.applicationDate = date;
    app.position = position;
    app.contactPerson = "Contact Person";
    app.status = status;
    app.notes = "Test notes";
    return app;
}

void TestAnalytics::addTestApplications()
{
    // Updated to reflect current status schema: 
    // Applied, Interviews, Offer Received, Rejected, Withdrawn
    std::vector<Application> apps = {
        createApplication("Company A", "Applied", "2024-01-01"),
        createApplication("Company B", "Applied", "2024-01-02"),
        createApplication("Company C", "Interviews", "2024-01-03"), // 1
        createApplication("Company D", "Interviews", "2024-01-04"), // 2
        createApplication("Company E", "Offer Received", "2024-01-05"),
        createApplication("Company F", "Rejected", "2024-01-06"),
        createApplication("Company G", "Rejected", "2024-01-07"),
        createApplication("Company H", "Withdrawn", "2024-01-08"),
        createApplication("Company I", "Interviews", "2024-01-09"), // 3
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
        QVERIFY2(found, QString("Status '%1' not found").arg(QString::fromStdString(status)).toLocal8Bit());
    }
}

void TestAnalytics::testEmptyDatabaseAnalytics()
{
    QCOMPARE(dbManager->getTotalApplications(), 0);
    QCOMPARE(dbManager->getApplicationsByStatus("Applied"), 0);
    QCOMPARE(dbManager->getApplicationsByStatus("Interviews"), 0);
    
    auto statusCounts = dbManager->getStatusCounts();
    QCOMPARE(statusCounts.size(), 0);
}

void TestAnalytics::testSingleApplicationAnalytics()
{
    Application app = createApplication("Single Company", "Applied");
    QVERIFY(dbManager->addApplication(app));
    
    QCOMPARE(dbManager->getTotalApplications(), 1);
    QCOMPARE(dbManager->getApplicationsByStatus("Applied"), 1);
    
    auto statusCounts = dbManager->getStatusCounts();
    QCOMPARE(statusCounts.size(), 1);
    QCOMPARE(statusCounts[0].first, std::string("Applied"));
}

void TestAnalytics::testMultipleApplicationsAnalytics()
{
    addTestApplications();
    
    QCOMPARE(dbManager->getTotalApplications(), 10);
    
    // Verify unified status counts
    QCOMPARE(dbManager->getApplicationsByStatus("Applied"), 3);
    QCOMPARE(dbManager->getApplicationsByStatus("Interviews"), 3);
    QCOMPARE(dbManager->getApplicationsByStatus("Offer Received"), 1);
    QCOMPARE(dbManager->getApplicationsByStatus("Rejected"), 2);
    QCOMPARE(dbManager->getApplicationsByStatus("Withdrawn"), 1);
}

void TestAnalytics::testStatusDistributionAnalytics()
{
    addTestApplications();
    
    auto statusCounts = dbManager->getStatusCounts();
    
    // We have 5 distinct statuses in addTestApplications()
    QCOMPARE(statusCounts.size(), 5);
    
    verifyStatusCount(statusCounts, "Applied", 3);
    verifyStatusCount(statusCounts, "Interviews", 3);
    verifyStatusCount(statusCounts, "Offer Received", 1);
    verifyStatusCount(statusCounts, "Rejected", 2);
    verifyStatusCount(statusCounts, "Withdrawn", 1);
}

void TestAnalytics::testInterviewCalculations()
{
    addTestApplications();
    
    // Retrieve total count from the specialized method
    int totalInterviews = dbManager->getInterviewsTotal();
    QCOMPARE(totalInterviews, 3);
    
    int totalApplications = dbManager->getTotalApplications();
    QCOMPARE(totalApplications, 10);
    
    // Interview rate: 3/10 = 0.3
    double interviewRate = static_cast<double>(totalInterviews) / totalApplications;
    QCOMPARE(interviewRate, 0.3); 
}

void TestAnalytics::testRejectionRateCalculations()
{
    addTestApplications();
    int rejected = dbManager->getApplicationsByStatus("Rejected");
    int total = dbManager->getTotalApplications();
    
    double rejectionRate = static_cast<double>(rejected) / total;
    QCOMPARE(rejectionRate, 0.2); // 2/10
}

void TestAnalytics::testOfferSuccessRate()
{
    addTestApplications();
    int offers = dbManager->getApplicationsByStatus("Offer Received");
    int total = dbManager->getTotalApplications();
    
    double offerRate = static_cast<double>(offers) / total;
    QCOMPARE(offerRate, 0.1); // 1/10
}

void TestAnalytics::testAnalyticsWithAllSameStatus()
{
    for (int i = 0; i < 5; ++i) {
        Application app = createApplication("Company " + std::to_string(i), "Interviews");
        QVERIFY(dbManager->addApplication(app));
    }
    
    QCOMPARE(dbManager->getApplicationsByStatus("Interviews"), 5);
    auto statusCounts = dbManager->getStatusCounts();
    QCOMPARE(statusCounts.size(), 1);
}

void TestAnalytics::testAnalyticsWithUnknownStatus()
{
    // If your DB manager rejects non-standard statuses
    Application app = createApplication("Test Company", "InvalidStatus");
    bool added = dbManager->addApplication(app);
    
    if (!added) {
        QCOMPARE(dbManager->getTotalApplications(), 0);
    }
}

void TestAnalytics::testAnalyticsAfterDeletion()
{
    addTestApplications();
    
    auto applications = dbManager->getAllApplications();
    int deletedId = -1;
    for (const auto& app : applications) {
        if (app.status == "Interviews") {
            deletedId = app.id;
            break;
        }
    }
    
    QVERIFY(deletedId != -1);
    QVERIFY(dbManager->deleteApplication(deletedId));
    
    QCOMPARE(dbManager->getApplicationsByStatus("Interviews"), 2);
    QCOMPARE(dbManager->getTotalApplications(), 9);
}

void TestAnalytics::testAnalyticsAfterUpdate()
{
    addTestApplications();
    
    // Start: Applied (3), Interviews (3)
    auto applications = dbManager->getAllApplications();
    
    Application appToUpdate;
    for (const auto& app : applications) {
        if (app.status == "Applied") {
            appToUpdate = app;
            break;
        }
    }
    
    appToUpdate.status = "Interviews";
    QVERIFY(dbManager->updateApplication(appToUpdate));
    
    // End: Applied (2), Interviews (4)
    QCOMPARE(dbManager->getApplicationsByStatus("Applied"), 2);
    QCOMPARE(dbManager->getApplicationsByStatus("Interviews"), 4);
}

#include "test_analytics.moc"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    TestAnalytics test;
    return QTest::qExec(&test, argc, argv);
}