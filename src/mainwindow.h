#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QDateEdit>
#include <QTextEdit>
#include <QLabel>
#include <QFormLayout>
#include <QDialog>
#include <QComboBox>
#include <QGroupBox>
#include <QGridLayout>
#include "db_manager.h"

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

class AddApplicationDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddApplicationDialog(QWidget *parent = nullptr);
    Application getApplication() const;

private slots:
    void accept() override;

private:
    QLineEdit *companyEdit;
    QDateEdit *dateEdit;
    QLineEdit *positionEdit;
    QLineEdit *contactEdit;
    QComboBox *statusCombo;
    QTextEdit *notesEdit;
};

class EditApplicationDialog : public QDialog {
    Q_OBJECT

public:
    explicit EditApplicationDialog(const Application& app, QWidget *parent = nullptr);
    Application getApplication() const;

private slots:
    void accept() override;

private:
    QLineEdit *companyEdit;
    QDateEdit *dateEdit;
    QLineEdit *positionEdit;
    QLineEdit *contactEdit;
    QComboBox *statusCombo;
    QTextEdit *notesEdit;
    int applicationId;
};

class AnalyticsDialog : public QDialog {
    Q_OBJECT

public:
    explicit AnalyticsDialog(DatabaseManager* dbManager, QWidget *parent = nullptr);

private:
    void setupUI();
    void loadAnalytics();
    
    DatabaseManager* dbManager;
    QVBoxLayout *mainLayout;
    QGroupBox *summaryGroup;
    QGroupBox *statusGroup;
    QLabel *totalLabel;
    QLabel *rejectedLabel;
    QLabel *interviewsLabel;
    QLabel *offersLabel;
    QGridLayout *statusLayout;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QString& dbFileName, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addApplication();
    void editApplication();
    void deleteApplication();
    void refreshTable();
    void showAnalytics();
    void onTableDoubleClick(int row, int column);
    void onSearchTextChanged(const QString& text);

private:
    void setupUI();
    void setupTable();
    void setupButtons();
    void connectButtons();
    void initDatabase();
    void loadApplications();
    void populateTable(const std::vector<Application>& applications);
    Application getApplicationFromRow(int row) const;

    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    QHBoxLayout *buttonLayout;
    QPushButton *addButton;
    QPushButton *editButton;
    QPushButton *deleteButton;
    QPushButton *refreshButton;
    QPushButton *analyticsButton;
    QTableWidget *tableWidget;
    QLineEdit *searchLineEdit;
    QString m_dbFileName;
    
    DatabaseManager dbManager;
};

#endif // MAINWINDOW_H