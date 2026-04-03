#include <QtTest/QtTest>
#include <QTemporaryDir>
#include "application_service.h"

class TestApplicationService : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testAddValidApplication();
    void testAddDuplicateApplication();
    void testAddInvalidApplication();
    void testUpdateApplication();
    void testDeleteApplication();
    void testListApplications();
    void testSearchApplications();
    void testGetAnalytics();
    void testBuildInsightsPrompt();

private:
    DatabaseManager*    m_db;
    ApplicationService* m_service;
};

void TestApplicationService::init()
{
    m_db      = new DatabaseManager;
    QVERIFY(m_db->openDatabase(":memory:"));
    m_service = new ApplicationService(m_db);
}

void TestApplicationService::cleanup()
{
    delete m_service;
    delete m_db;
    m_service = nullptr;
    m_db      = nullptr;
}

void TestApplicationService::testAddValidApplication()
{
    Application app;
    app.companyName     = "Test Corp";
    app.applicationDate = "2024-06-01";
    app.position        = "Developer";
    app.contactPerson   = "Jane Doe";
    app.status          = "Applied";
    app.notes           = "Via referral";

    auto result = m_service->addApplication(app);
    QVERIFY(result.first);
    QVERIFY(result.second.empty());

    auto apps = m_service->listApplications();
    QCOMPARE(apps.size(), 1u);
}

void TestApplicationService::testAddDuplicateApplication()
{
    Application app;
    app.companyName     = "Test Corp";
    app.applicationDate = "2024-06-01";
    app.position        = "Developer";
    app.contactPerson   = "Jane Doe";
    app.status          = "Applied";
    app.notes           = "Via referral";

    auto result1 = m_service->addApplication(app);
    QVERIFY(result1.first);

    auto result2 = m_service->addApplication(app);
    QVERIFY(!result2.first);
    QVERIFY(result2.second.find("duplicate") != std::string::npos);
}

void TestApplicationService::testAddInvalidApplication()
{
    Application app;  // all fields empty

    auto result = m_service->addApplication(app);
    QVERIFY(!result.first);
    QVERIFY(!result.second.empty());
}

void TestApplicationService::testUpdateApplication()
{
    Application app;
    app.companyName     = "Test Corp";
    app.applicationDate = "2024-06-01";
    app.position        = "Developer";
    app.contactPerson   = "Jane Doe";
    app.status          = "Applied";

    auto addResult = m_service->addApplication(app);
    QVERIFY(addResult.first);

    auto apps = m_service->listApplications();
    QVERIFY(!apps.empty());

    apps[0].position = "Senior Developer";
    apps[0].status   = "Interviews";

    auto updateResult = m_service->updateApplication(apps[0]);
    QVERIFY(updateResult.first);

    auto updated = m_service->listApplications();
    QCOMPARE(updated[0].position, std::string("Senior Developer"));
}

void TestApplicationService::testDeleteApplication()
{
    Application app;
    app.companyName     = "Test Corp";
    app.applicationDate = "2024-06-01";
    app.position        = "Developer";
    app.contactPerson   = "Jane Doe";
    app.status          = "Applied";

    m_service->addApplication(app);

    auto apps = m_service->listApplications();
    QVERIFY(!apps.empty());

    auto deleteResult = m_service->deleteApplication(apps[0].id);
    QVERIFY(deleteResult.first);

    QCOMPARE(m_service->listApplications().size(), 0u);
}

void TestApplicationService::testListApplications()
{
    QCOMPARE(m_service->listApplications().size(), 0u);

    Application app;
    app.companyName     = "A Corp";
    app.applicationDate = "2024-01-01";
    app.position        = "Dev";
    app.contactPerson   = "John";
    app.status          = "Applied";

    m_service->addApplication(app);
    QCOMPARE(m_service->listApplications().size(), 1u);
}

void TestApplicationService::testSearchApplications()
{
    Application app1;
    app1.companyName     = "Alpha Corp";
    app1.applicationDate = "2024-01-01";
    app1.position        = "Engineer";
    app1.contactPerson   = "John";
    app1.status          = "Applied";
    m_service->addApplication(app1);

    Application app2;
    app2.companyName     = "Beta Inc";
    app2.applicationDate = "2024-02-01";
    app2.position        = "Designer";
    app2.contactPerson   = "Jane";
    app2.status          = "Rejected";
    m_service->addApplication(app2);

    auto results = m_service->searchApplications("Alpha");
    QCOMPARE(results.size(), 1);
    QCOMPARE(results[0].companyName, std::string("Alpha Corp"));

    auto emptyResults = m_service->searchApplications("nonexistent");
    QVERIFY(emptyResults.empty());

    auto allResults = m_service->searchApplications("");
    QCOMPARE(allResults.size(), 2u);
}

void TestApplicationService::testGetAnalytics()
{
    auto analytics = m_service->getAnalytics();
    QCOMPARE(analytics.total, 0);

    std::vector<Application> apps;
    Application app1; app1.companyName = ""; app1.applicationDate = "2024-01-01"; app1.position = "Dev"; app1.contactPerson = "John"; app1.status = "Applied"; app1.notes = "";
    Application app2; app2.companyName = ""; app2.applicationDate = "2024-01-02"; app2.position = "Dev"; app2.contactPerson = "Jane"; app2.status = "Interviews"; app2.notes = "";
    Application app3; app3.companyName = ""; app3.applicationDate = "2024-01-03"; app3.position = "Dev"; app3.contactPerson = "Bob"; app3.status = "Rejected"; app3.notes = "";
    Application app4; app4.companyName = ""; app4.applicationDate = "2024-01-04"; app4.position = "Dev"; app4.contactPerson = "Alice"; app4.status = "Offer Received"; app4.notes = "";
    apps.push_back(app1);
    apps.push_back(app2);
    apps.push_back(app3);
    apps.push_back(app4);

    for (size_t i = 0; i < apps.size(); ++i) {
        apps[i].companyName = "Corp" + std::to_string(i);
        m_service->addApplication(apps[i]);
    }

    analytics = m_service->getAnalytics();
    QCOMPARE(analytics.total, 4);
    QCOMPARE(analytics.interviews, 1);
    QCOMPARE(analytics.rejected, 1);
    QCOMPARE(analytics.offers, 1);
}

void TestApplicationService::testBuildInsightsPrompt()
{
    ApplicationAnalytics analytics;
    analytics.total      = 3;
    analytics.rejected   = 1;
    analytics.interviews = 1;
    analytics.offers     = 0;
    analytics.statusCounts = {{"Applied", 1}, {"Interviews", 1}, {"Rejected", 1}};

    std::vector<Application> apps;
    Application app; app.companyName = "Corp"; app.applicationDate = "2024-06-01"; app.position = "Dev"; app.contactPerson = "John"; app.status = "Applied"; app.notes = "";
    apps.push_back(app);

    std::string prompt = m_service->buildInsightsPrompt(analytics, apps);
    QVERIFY(!prompt.empty());
    QVERIFY(prompt.find("Total applications: 3") != std::string::npos);
    QVERIFY(prompt.find("Interview rate:") != std::string::npos);
}

#include "test_application_service.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestApplicationService test;
    return QTest::qExec(&test, argc, argv);
}
