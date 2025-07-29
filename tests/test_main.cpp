#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QDir>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "CV Tracker Test Suite Runner";
    qDebug() << "=============================";
    
    QStringList testExecutables = {
        "test_db_manager",
        "test_analytics", 
        "test_application_logic"
    };
    
    int totalTests = 0;
    int passedTests = 0;
    
    for (const QString& testName : testExecutables) {
        qDebug() << "\nRunning" << testName << "...";
        
        QProcess process;
        process.start(testName, QStringList());
        
        if (!process.waitForStarted()) {
            qDebug() << "Failed to start" << testName;
            qDebug() << "Make sure the test executable exists in the current directory";
            continue;
        }
        
        if (!process.waitForFinished(30000)) { // 30 second timeout
            qDebug() << "Test" << testName << "timed out";
            process.kill();
            continue;
        }
        
        totalTests++;
        int exitCode = process.exitCode();
        
        if (exitCode == 0) {
            qDebug() << testName << "PASSED";
            passedTests++;
        } else {
            qDebug() << testName << "FAILED with exit code" << exitCode;
            qDebug() << "Error output:" << process.readAllStandardError();
        }
    }
    
    qDebug() << "\n=============================";
    qDebug() << "Test Results:" << passedTests << "/" << totalTests << "passed";
    
    if (passedTests == totalTests && totalTests > 0) {
        qDebug() << "All tests PASSED!";
        return 0;
    } else {
        qDebug() << "Some tests FAILED!";
        return 1;
    }
}