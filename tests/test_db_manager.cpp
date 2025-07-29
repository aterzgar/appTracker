#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QDir>
#include "db_manager.h"

class TestDatabaseManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Database connection tests
    void testOpenDatabase();
    void testOpenInvalidDatabase();
    void testCloseDatabase();

    // Application CRUD tests
    void testAddApplication();
    void testAddInvalidApplication();
    void testAddDuplicateApplication();
    void testGetAllApplications();
    void testUpdateApplication();
    void testDeleteApplication();

    // Validation tests
    void testDateValidation();
    void testDuplicateDetection();

    // Analytics tests
    void testGetTotalApplications();
    void testGetApplicationsByStatus();
    void testGetStatusCounts();

private:
    DatabaseManager* dbManager;
    QTemporaryDir* tempDir;
    QString testDbPath;

    Application createTestApplication(const std::string& company = "Test Company",
                                    const std::string& position = "Software Engineer",
                                    const std::string& status = "Applied");
};

void TestDatabaseManager::initTestCase()
{
    tempDir = new QTemporaryDir();
    QVERIFY(tempDir->isValid());
}

void TestDatabaseManager::cleanupTestCase()
{
    delete tempDir;
}

void TestDatabaseManager::init()
{
    dbManager = new DatabaseManager();
    testDbPath = tempDir->path() + "/test_applications.db";
}

void TestDatabaseManager::cleanup()
{
    if (dbManager) {
        dbManager->closeDatabase();
        delete dbManager;
        dbManager = nullptr;
    }
    
    // Clean up test database file
    QFile::remove(testDbPath);
}

Application TestDatabaseManager::createTestApplication(const std::string& company,
                                                     const std::string& position,
                                                     const std::string& status)
{
    Application app;
    app.companyName = company;
    app.applicationDate = "2024-01-15";
    app.position = position;
    app.contactPerson = "John Doe";
    app.status = status;
    app.notes = "Test notes";
    return app;
}

void TestDatabaseManager::testOpenDatabase()
{
    QVERIFY(dbManager->openDatabase(testDbPath.toStdString()));
    
    // Verify database file was created
    QVERIFY(QFile::exists(testDbPath));
}

void TestDatabaseManager::testOpenInvalidDatabase()
{
    // Try to open database in non-existent directory
    QString invalidPath = "/non/existent/path/test.db";
    QVERIFY(!dbManager->openDatabase(invalidPath.toStdString()));
}

void TestDatabaseManager::testCloseDatabase()
{
    QVERIFY(dbManager->openDatabase(testDbPath.toStdString()));
    dbManager->closeDatabase();
    
    // Should be able to open again after closing
    QVERIFY(dbManager->openDatabase(testDbPath.toStdString()));
}

void TestDatabaseManager::testAddApplication()
{
    QVERIFY(dbManager->openDatabase(testDbPath.toStdString()));
    
    Application app = createTestApplication();
    QVERIFY(dbManager->addApplication(app));
    
    // Verify application was added
    auto applications = dbManager->getAllApplications();
    QCOMPARE(applications.size(), 1);
    QCOMPARE(applications[0].companyName, app.companyName);
    QCOMPARE(applications[0].position, app.position);
    QCOMPARE(applications[0].status, app.status);
}

void TestDatabaseManager::testAddInvalidApplication()
{
    QVERIFY(dbManager->openDatabase(testDbPath.toStdString()));
    
    // Test with invalid date
    Application app = createTestApplication();
    app.applicationDate = "invalid-date";
    QVERIFY(!dbManager->addApplication(app));
    
    // Test with empty required fields
    app = createTestApplication();
    app.companyName = "";
    QVERIFY(!dbManager->addApplication(app));
}

void TestDatabaseManager::testAddDuplicateApplication()
{
    QVERIFY(dbManager->openDatabase(testDbPath.toStdString()));
    
    Application app = createTestApplication();
    QVERIFY(dbManager->addApplication(app));
    
    // Try to add the same application again
    QVERIFY(!dbManager->addApplication(app));
    
    // Verify only one application exists
    auto applications = dbManager->getAllApplications();
    QCOMPARE(applications.size(), 1);
}

void TestDatabaseManager::testGetAllApplications()
{
    QVERIFY(dbManager->openDatabase(testDbPath.toStdString()));
    
    // Initially should be empty
    auto applications = dbManager->getAllApplications();
    QCOMPARE(applications.size(), 0);
    
    // Add multiple applications
    Application app1 = createTestApplication("Company A", "Developer", "Applied");
    Application app2 = createTestApplication("Company B", "Engineer", "Interview Scheduled");
    Application app3 = createTestApplication("Company C", "Analyst", "Rejected");
    
    QVERIFY(dbManager->addApplication(app1));
    QVERIFY(dbManager->addApplication(app2));
    QVERIFY(dbManager->addApplication(app3));
    
    // Verify all applications are retrieved
    applications = dbManager->getAllApplications();
    QCOMPARE(applications.size(), 3);
}

void TestDatabaseManager::testUpdateApplication()
{
    QVERIFY(dbManager->openDatabase(testDbPath.toStdString()));
    
    // Add an application
    Application app = createTestApplication();
    QVERIFY(dbManager->addApplication(app));
    
    // Get the application with its ID
    auto applications = dbManager->getAllApplications();
    QCOMPARE(applications.size(), 1);
    
    Application updatedApp = applications[0];
    updatedApp.status = "Interview Scheduled";
    updatedApp.notes = "Updated notes";
    
    // Update the application
    QVERIFY(dbManager->updateApplication(updatedApp));
    
    // Verify the update
    applications = dbManager->getAllApplications();
    QCOMPARE(applications.size(), 1);
    QCOMPARE(applications[0].status, std::string("Interview Scheduled"));
    QCOMPARE(applications[0].notes, std::string("Updated notes"));
}

void TestDatabaseManager::testDeleteApplication()
{
    QVERIFY(dbManager->openDatabase(testDbPath.toStdString()));
    
    // Add an application
    Application app = createTestApplication();
    QVERIFY(dbManager->addApplication(app));
    
    // Get the application ID
    auto applications = dbManager->getAllApplications();
    QCOMPARE(applications.size(), 1);
    int appId = applications[0].id;
    
    // Delete the application
    QVERIFY(dbManager->deleteApplication(appId));
    
    // Verify deletion
    applications = dbManager->getAllApplications();
    QCOMPARE(applications.size(), 0);
}

void TestDatabaseManager::testDateValidation()
{
    QVERIFY(dbManager->openDatabase(testDbPath.toStdString()));
    
    // Valid dates
    QVERIFY(dbManager->isValidDateTime("2024-01-15"));
    QVERIFY(dbManager->isValidDateTime("2023-12-31"));
    QVERIFY(dbManager->isValidDateTime("2024-02-29")); // Leap year
    
    // Invalid dates
    QVERIFY(!dbManager->isValidDateTime("2024-13-01")); // Invalid month
    QVERIFY(!dbManager->isValidDateTime("2024-01-32")); // Invalid day
    QVERIFY(!dbManager->isValidDateTime("24-01-15"));   // Wrong format
    QVERIFY(!dbManager->isValidDateTime("2024/01/15")); // Wrong separator
    QVERIFY(!dbManager->isValidDateTime("invalid"));    // Completely invalid
    QVERIFY(!dbManager->isValidDateTime(""));           // Empty string
}

void TestDatabaseManager::testDuplicateDetection()
{
    QVERIFY(dbManager->openDatabase(testDbPath.toStdString()));
    
    Application app = createTestApplication();
    
    // Initially should not be duplicate
    QVERIFY(!dbManager->isDuplicateEntry(app));
    
    // Add the application
    QVERIFY(dbManager->addApplication(app));
    
    // Now should be detected as duplicate
    QVERIFY(dbManager->isDuplicateEntry(app));
    
    // Different company should not be duplicate
    Application differentApp = app;
    differentApp.companyName = "Different Company";
    QVERIFY(!dbManager->isDuplicateEntry(differentApp));
}

void TestDatabaseManager::testGetTotalApplications()
{
    QVERIFY(dbManager->openDatabase(testDbPath.toStdString()));
    
    // Initially should be 0
    QCOMPARE(dbManager->getTotalApplications(), 0);
    
    // Add applications
    Application app1 = createTestApplication("Company A");
    Application app2 = createTestApplication("Company B");
    Application app3 = createTestApplication("Company C");
    
    QVERIFY(dbManager->addApplication(app1));
    QCOMPARE(dbManager->getTotalApplications(), 1);
    
    QVERIFY(dbManager->addApplication(app2));
    QCOMPARE(dbManager->getTotalApplications(), 2);
    
    QVERIFY(dbManager->addApplication(app3));
    QCOMPARE(dbManager->getTotalApplications(), 3);
}

void TestDatabaseManager::testGetApplicationsByStatus()
{
    QVERIFY(dbManager->openDatabase(testDbPath.toStdString()));
    
    // Add applications with different statuses
    Application app1 = createTestApplication("Company A", "Developer", "Applied");
    Application app2 = createTestApplication("Company B", "Engineer", "Applied");
    Application app3 = createTestApplication("Company C", "Analyst", "Interview Scheduled");
    Application app4 = createTestApplication("Company D", "Manager", "Rejected");
    
    QVERIFY(dbManager->addApplication(app1));
    QVERIFY(dbManager->addApplication(app2));
    QVERIFY(dbManager->addApplication(app3));
    QVERIFY(dbManager->addApplication(app4));
    
    // Test status counts
    QCOMPARE(dbManager->getApplicationsByStatus("Applied"), 2);
    QCOMPARE(dbManager->getApplicationsByStatus("Interview Scheduled"), 1);
    QCOMPARE(dbManager->getApplicationsByStatus("Rejected"), 1);
    QCOMPARE(dbManager->getApplicationsByStatus("Offer Received"), 0);
}

void TestDatabaseManager::testGetStatusCounts()
{
    QVERIFY(dbManager->openDatabase(testDbPath.toStdString()));
    
    // Add applications with different statuses
    Application app1 = createTestApplication("Company A", "Developer", "Applied");
    Application app2 = createTestApplication("Company B", "Engineer", "Applied");
    Application app3 = createTestApplication("Company C", "Analyst", "Interview Scheduled");
    Application app4 = createTestApplication("Company D", "Manager", "Rejected");
    Application app5 = createTestApplication("Company E", "Lead", "Applied");
    
    QVERIFY(dbManager->addApplication(app1));
    QVERIFY(dbManager->addApplication(app2));
    QVERIFY(dbManager->addApplication(app3));
    QVERIFY(dbManager->addApplication(app4));
    QVERIFY(dbManager->addApplication(app5));
    
    auto statusCounts = dbManager->getStatusCounts();
    
    // Should have 3 different statuses
    QCOMPARE(statusCounts.size(), 3);
    
    // Find and verify counts (order might vary due to SQL ORDER BY COUNT(*) DESC)
    bool foundApplied = false, foundInterview = false, foundRejected = false;
    
    for (const auto& pair : statusCounts) {
        if (pair.first == "Applied") {
            QCOMPARE(pair.second, 3);
            foundApplied = true;
        } else if (pair.first == "Interview Scheduled") {
            QCOMPARE(pair.second, 1);
            foundInterview = true;
        } else if (pair.first == "Rejected") {
            QCOMPARE(pair.second, 1);
            foundRejected = true;
        }
    }
    
    QVERIFY(foundApplied);
    QVERIFY(foundInterview);
    QVERIFY(foundRejected);
}

#include "test_db_manager.moc"