#include <QtTest/QtTest>
#include <QApplication>

// Forward declarations of test classes
class TestDatabaseManager;
class TestApplicationLogic;
class TestAnalytics;

// We need to declare the test classes here and link them separately
// This avoids multiple definition errors

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    int result = 0;
    
    // Note: Each test class will be run as a separate executable
    // or we need to restructure to avoid multiple definitions
    
    qDebug() << "CV Tracker Test Suite";
    qDebug() << "Run individual test executables:";
    qDebug() << "- test_db_manager";
    qDebug() << "- test_application_logic"; 
    qDebug() << "- test_analytics";
    
    return 0;
}