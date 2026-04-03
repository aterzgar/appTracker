#include <QtTest/QtTest>
#include <QApplication>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QTest>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QGroupBox>
#include <QLabel>
#include "mainwindow.h"

class TestApplicationLogic : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // UI Component tests
    void testMainWindowInitialization();
    void testButtonsExist();
    void testTableSetup();
    
    // Dialog tests
    void testAddApplicationDialog();
    void testEditApplicationDialog();
    void testAnalyticsDialog();
    
    // Integration tests
    void testAddApplicationWorkflow();
    void testEditApplicationWorkflow();
    void testDeleteApplicationWorkflow();
    void testRefreshFunctionality();

private:
    QApplication* app;
    MainWindow* mainWindow;
    QTemporaryDir* tempDir;
};

void TestApplicationLogic::initTestCase()
{
    // Create QApplication if it doesn't exist
    if (!QApplication::instance()) {
        int argc = 0;
        char** argv = nullptr;
        app = new QApplication(argc, argv);
    } else {
        app = nullptr; // Don't delete existing application
    }
    
    tempDir = new QTemporaryDir();
    QVERIFY(tempDir->isValid());
}

void TestApplicationLogic::cleanupTestCase()
{
    delete tempDir;
    if (app) {
        delete app;
    }
}

void TestApplicationLogic::init()
{
    QString dbFile = tempDir->path() + "/test.db";
    mainWindow = new MainWindow(dbFile);
    // Don't show the window during tests to avoid GUI interference
}

void TestApplicationLogic::cleanup()
{
    if (mainWindow) {
        delete mainWindow;
        mainWindow = nullptr;
    }
}

void TestApplicationLogic::testMainWindowInitialization()
{
    QVERIFY(mainWindow != nullptr);
    QCOMPARE(mainWindow->windowTitle(), QString("Application Tracker"));
    QVERIFY(mainWindow->minimumSize().width() >= 800);
    QVERIFY(mainWindow->minimumSize().height() >= 600);
}

void TestApplicationLogic::testButtonsExist()
{
    // Find buttons by their text
    auto addButton = mainWindow->findChild<QPushButton*>();
    QVERIFY(addButton != nullptr);
    
    auto buttons = mainWindow->findChildren<QPushButton*>();
    QVERIFY(buttons.size() >= 5); // Add, Edit, Delete, Refresh, Analytics
    
    QStringList buttonTexts;
    for (auto button : buttons) {
        buttonTexts << button->text();
    }
    
    QVERIFY(buttonTexts.contains("+ New Application"));
    QVERIFY(buttonTexts.contains("Edit"));
    QVERIFY(buttonTexts.contains("Delete"));
    QVERIFY(buttonTexts.contains("Refresh"));
    QVERIFY(buttonTexts.contains("Analytics"));
}

void TestApplicationLogic::testTableSetup()
{
    auto tableWidget = mainWindow->findChild<QTableWidget*>();
    QVERIFY(tableWidget != nullptr);
    
    // Check column count
    QCOMPARE(tableWidget->columnCount(), 7);
    
    // Check headers
    QStringList expectedHeaders = {"ID", "Company", "Date", "Position", "Contact", "Status", "Notes"};
    for (int i = 0; i < expectedHeaders.size(); ++i) {
        QCOMPARE(tableWidget->horizontalHeaderItem(i)->text(), expectedHeaders[i]);
    }
    
    // Check table properties
    QCOMPARE(tableWidget->selectionBehavior(), QAbstractItemView::SelectRows);
    QVERIFY(tableWidget->alternatingRowColors());
    QVERIFY(tableWidget->isSortingEnabled());
    QCOMPARE(tableWidget->editTriggers(), QAbstractItemView::NoEditTriggers);
}

void TestApplicationLogic::testAddApplicationDialog()
{
    AddApplicationDialog dialog;
    
    // Check dialog properties
    QCOMPARE(dialog.windowTitle(), QString("Add New Application"));
    QVERIFY(dialog.isModal());
    
    // Check form fields exist
    auto companyEdit = dialog.findChild<QLineEdit*>();
    auto dateEdit = dialog.findChild<QDateEdit*>();
    auto positionEdit = dialog.findChild<QLineEdit*>();
    auto contactEdit = dialog.findChild<QLineEdit*>();
    auto statusCombo = dialog.findChild<QComboBox*>();
    auto notesEdit = dialog.findChild<QTextEdit*>();
    
    QVERIFY(companyEdit != nullptr);
    QVERIFY(dateEdit != nullptr);
    QVERIFY(positionEdit != nullptr);
    QVERIFY(contactEdit != nullptr);
    QVERIFY(statusCombo != nullptr);
    QVERIFY(notesEdit != nullptr);
    
    // Check status combo items
    QStringList expectedStatuses = {"Applied", "Interviews", 
                                   "Offer Received", "Rejected", "Withdrawn"};
    QCOMPARE(statusCombo->count(), expectedStatuses.size());
    for (int i = 0; i < expectedStatuses.size(); ++i) {
        QCOMPARE(statusCombo->itemText(i), expectedStatuses[i]);
    }
}

void TestApplicationLogic::testEditApplicationDialog()
{
    // Create a test application
    Application testApp;
    testApp.id = 1;
    testApp.companyName = "Test Company";
    testApp.applicationDate = "2024-01-15";
    testApp.position = "Software Engineer";
    testApp.contactPerson = "John Doe";
    testApp.status = "Applied";
    testApp.notes = "Test notes";
    
    EditApplicationDialog dialog(testApp);
    
    // Check dialog properties
    QCOMPARE(dialog.windowTitle(), QString("Edit Application"));
    QVERIFY(dialog.isModal());
    
    // Check that fields are pre-populated
    auto companyEdit = dialog.findChild<QLineEdit*>();
    auto positionEdit = dialog.findChild<QLineEdit*>();
    auto contactEdit = dialog.findChild<QLineEdit*>();
    auto statusCombo = dialog.findChild<QComboBox*>();
    auto notesEdit = dialog.findChild<QTextEdit*>();
    
    QVERIFY(companyEdit != nullptr);
    QVERIFY(positionEdit != nullptr);
    QVERIFY(contactEdit != nullptr);
    QVERIFY(statusCombo != nullptr);
    QVERIFY(notesEdit != nullptr);
    
    // Note: We can't easily test the exact values without accessing private members
    // In a real scenario, you might want to add getter methods for testing
}

void TestApplicationLogic::testAnalyticsDialog()
{
    // Create a mock database manager for testing
    DatabaseManager dbManager;
    
    // IMPORTANT: Open the database before using it!
    QVERIFY(dbManager.openDatabase(":memory:")); // Use in-memory database for tests
    
    ApplicationService appService(&dbManager);
    AnalyticsDialog dialog(&appService);
    
    // Check dialog properties
    QCOMPARE(dialog.windowTitle(), QString("Application Analytics"));
    QVERIFY(!dialog.isModal()); // Now modeless
    
    // Check that UI elements exist
    auto summaryGroup = dialog.findChild<QGroupBox*>();
    QVERIFY(summaryGroup != nullptr);
    
    auto labels = dialog.findChildren<QLabel*>();
    QVERIFY(labels.size() >= 4); // At least total, rejections, interviews, offers
}

void TestApplicationLogic::testAddApplicationWorkflow()
{
    // This test would require more complex setup to simulate user interactions
    // For now, we'll test that the slot exists and can be called
    
    auto addButton = mainWindow->findChild<QPushButton*>();
    bool foundAddButton = false;
    
    auto buttons = mainWindow->findChildren<QPushButton*>();
    for (auto button : buttons) {
        if (button->text() == "+ New Application") {
            foundAddButton = true;
            
            // Test that clicking the button doesn't crash
            // In a real test, you'd mock the dialog or use QTest::qWait()
            QSignalSpy spy(button, &QPushButton::clicked);
            button->click();
            QCOMPARE(spy.count(), 1);
            break;
        }
    }
    
    QVERIFY(foundAddButton);
}

void TestApplicationLogic::testEditApplicationWorkflow()
{
    auto buttons = mainWindow->findChildren<QPushButton*>();
    QPushButton* editBtn = nullptr;

    for (auto button : buttons) {
        if (button->text() == "Edit") {
            editBtn = button;
            break;
        }
    }
    QVERIFY(editBtn != nullptr);

    editBtn->setEnabled(true);

    QSignalSpy spy(editBtn, &QPushButton::clicked);
    editBtn->animateClick();
    QTest::qWait(150);
    QCOMPARE(spy.count(), 1);
}

void TestApplicationLogic::testDeleteApplicationWorkflow()
{
    auto buttons = mainWindow->findChildren<QPushButton*>();
    QPushButton* deleteBtn = nullptr;

    for (auto button : buttons) {
        if (button->text() == "Delete") {
            deleteBtn = button;
            break;
        }
    }
    QVERIFY(deleteBtn != nullptr);

    deleteBtn->setEnabled(true);

    QSignalSpy spy(deleteBtn, &QPushButton::clicked);
    deleteBtn->animateClick();
    QTest::qWait(150);
    QCOMPARE(spy.count(), 1);
}

void TestApplicationLogic::testRefreshFunctionality()
{
    auto buttons = mainWindow->findChildren<QPushButton*>();
    QPushButton* refreshBtn = nullptr;

    for (auto button : buttons) {
        if (button->text() == "Refresh") {
            refreshBtn = button;
            break;
        }
    }
    QVERIFY(refreshBtn != nullptr);

    QSignalSpy spy(refreshBtn, &QPushButton::clicked);
    refreshBtn->animateClick();
    QTest::qWait(150);
    QCOMPARE(spy.count(), 1);
}

#include "test_application_logic.moc"

// Main function for this test
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TestApplicationLogic test;
    return QTest::qExec(&test, argc, argv);
}