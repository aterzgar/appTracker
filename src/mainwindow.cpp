#include "mainwindow.h"
#include <QApplication>
#include <QDate>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <vector>

// ---------- Utilities ----------

namespace {
    // Helper to validate required fields
    bool validateFields(const QLineEdit* companyEdit, const QLineEdit* positionEdit, const QLineEdit* contactEdit, QWidget* parent) {
        if (companyEdit->text().isEmpty() || positionEdit->text().isEmpty() || contactEdit->text().isEmpty()) {
            QMessageBox::warning(parent, "Validation Error",
                                 "Please fill in all required fields (Company, Position, Contact Person).");
            return false;
        }
        return true;
    }
}

// ---------- AddApplicationDialog ----------

AddApplicationDialog::AddApplicationDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Add New Application");
    setModal(true);
    resize(400, 300);

    auto layout = new QFormLayout(this);

    companyEdit = new QLineEdit(this);
    layout->addRow("Company Name:", companyEdit);

    dateEdit = new QDateEdit(QDate::currentDate(), this);
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    layout->addRow("Application Date:", dateEdit);

    positionEdit = new QLineEdit(this);
    layout->addRow("Position:", positionEdit);

    contactEdit = new QLineEdit(this);
    layout->addRow("Contact Person:", contactEdit);

    statusCombo = new QComboBox(this);
    statusCombo->addItems({"Applied", "Interview Scheduled", "Interview Completed", 
                          "Offer Received", "Rejected", "Withdrawn"});
    layout->addRow("Status:", statusCombo);

    notesEdit = new QTextEdit(this);
    notesEdit->setMaximumHeight(100);
    layout->addRow("Notes:", notesEdit);

    auto okButton = new QPushButton("Add Application", this);
    auto cancelButton = new QPushButton("Cancel", this);

    auto buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    layout->addRow(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &AddApplicationDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void AddApplicationDialog::accept()
{
    if (!validateFields(companyEdit, positionEdit, contactEdit, this))
        return;
    QDialog::accept();
}

Application AddApplicationDialog::getApplication() const
{
    Application app;
    app.companyName = companyEdit->text().toStdString();
    app.applicationDate = dateEdit->date().toString("yyyy-MM-dd").toStdString();
    app.position = positionEdit->text().toStdString();
    app.contactPerson = contactEdit->text().toStdString();
    app.status = statusCombo->currentText().toStdString();
    app.notes = notesEdit->toPlainText().toStdString();
    return app;
}

// ---------- EditApplicationDialog ----------

EditApplicationDialog::EditApplicationDialog(const Application& app, QWidget *parent)
    : QDialog(parent), applicationId(app.id)
{
    setWindowTitle("Edit Application");
    setModal(true);
    resize(400, 300);

    auto layout = new QFormLayout(this);

    companyEdit = new QLineEdit(QString::fromStdString(app.companyName), this);
    layout->addRow("Company Name:", companyEdit);

    dateEdit = new QDateEdit(QDate::fromString(QString::fromStdString(app.applicationDate), "yyyy-MM-dd"), this);
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    layout->addRow("Application Date:", dateEdit);

    positionEdit = new QLineEdit(QString::fromStdString(app.position), this);
    layout->addRow("Position:", positionEdit);

    contactEdit = new QLineEdit(QString::fromStdString(app.contactPerson), this);
    layout->addRow("Contact Person:", contactEdit);

    statusCombo = new QComboBox(this);
    statusCombo->addItems({"Applied", "Interview Scheduled", "Interview Completed",
                          "Offer Received", "Rejected", "Withdrawn"});
    statusCombo->setCurrentText(QString::fromStdString(app.status));
    layout->addRow("Status:", statusCombo);

    notesEdit = new QTextEdit(QString::fromStdString(app.notes), this);
    notesEdit->setMaximumHeight(100);
    layout->addRow("Notes:", notesEdit);

    auto okButton = new QPushButton("Update Application", this);
    auto cancelButton = new QPushButton("Cancel", this);

    auto buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    layout->addRow(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &EditApplicationDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void EditApplicationDialog::accept()
{
    if (!validateFields(companyEdit, positionEdit, contactEdit, this))
        return;
    QDialog::accept();
}

Application EditApplicationDialog::getApplication() const
{
    Application app;
    app.id = applicationId;
    app.companyName = companyEdit->text().toStdString();
    app.applicationDate = dateEdit->date().toString("yyyy-MM-dd").toStdString();
    app.position = positionEdit->text().toStdString();
    app.contactPerson = contactEdit->text().toStdString();
    app.status = statusCombo->currentText().toStdString();
    app.notes = notesEdit->toPlainText().toStdString();
    return app;
}

// ---------- AnalyticsDialog ----------

AnalyticsDialog::AnalyticsDialog(DatabaseManager* dbManager, QWidget *parent)
    : QDialog(parent), dbManager(dbManager)
{
    setWindowTitle("Application Analytics");
    setModal(true);
    resize(500, 400);
    
    setupUI();
    loadAnalytics();
}

void AnalyticsDialog::setupUI()
{
    mainLayout = new QVBoxLayout(this);
    
    // Summary statistics group
    summaryGroup = new QGroupBox("Summary Statistics", this);
    auto summaryLayout = new QGridLayout(summaryGroup);
    
    totalLabel = new QLabel("Total Applications: 0", this);
    totalLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #2c3e50;");
    summaryLayout->addWidget(totalLabel, 0, 0, 1, 2);
    
    rejectedLabel = new QLabel("Rejections: 0", this);
    rejectedLabel->setStyleSheet("font-size: 12px; color: #e74c3c;");
    summaryLayout->addWidget(rejectedLabel, 1, 0);
    
    interviewsLabel = new QLabel("Interviews: 0", this);
    interviewsLabel->setStyleSheet("font-size: 12px; color: #f39c12;");
    summaryLayout->addWidget(interviewsLabel, 1, 1);
    
    offersLabel = new QLabel("Offers: 0", this);
    offersLabel->setStyleSheet("font-size: 12px; color: #27ae60;");
    summaryLayout->addWidget(offersLabel, 2, 0);
    
    mainLayout->addWidget(summaryGroup);
    
    // Status breakdown group
    statusGroup = new QGroupBox("Status Breakdown", this);
    statusLayout = new QGridLayout(statusGroup);
    mainLayout->addWidget(statusGroup);
    
    // Close button
    auto closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(closeButton);
}

void AnalyticsDialog::loadAnalytics()
{
    if (!dbManager) return;
    
    // Get total applications
    int total = dbManager->getTotalApplications();
    totalLabel->setText(QString("Total Applications: %1").arg(total));
    
    // Get rejections
    int rejected = dbManager->getApplicationsByStatus("Rejected");
    rejectedLabel->setText(QString("Rejections: %1").arg(rejected));
    
    // Get interviews (Interview Scheduled + Interview Completed)
    int interviewScheduled = dbManager->getApplicationsByStatus("Interview Scheduled");
    int interviewCompleted = dbManager->getApplicationsByStatus("Interview Completed");
    int totalInterviews = interviewScheduled + interviewCompleted;
    interviewsLabel->setText(QString("Interviews: %1").arg(totalInterviews));
    
    // Get offers
    int offers = dbManager->getApplicationsByStatus("Offer Received");
    offersLabel->setText(QString("Offers: %1").arg(offers));
    
    // Clear existing status labels
    QLayoutItem* item;
    while ((item = statusLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    
    // Get detailed status breakdown
    auto statusCounts = dbManager->getStatusCounts();
    int row = 0;
    for (const auto& statusPair : statusCounts) {
        auto statusLabel = new QLabel(QString::fromStdString(statusPair.first) + ":", this);
        statusLabel->setStyleSheet("font-weight: bold;");
        auto countLabel = new QLabel(QString::number(statusPair.second), this);
        
        // Color code based on status
        QString color = "#34495e"; // default
        if (statusPair.first == "Rejected") color = "#e74c3c";
        else if (statusPair.first == "Interview Scheduled" || statusPair.first == "Interview Completed") color = "#f39c12";
        else if (statusPair.first == "Offer Received") color = "#27ae60";
        else if (statusPair.first == "Applied") color = "#3498db";
        
        countLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(color));
        
        statusLayout->addWidget(statusLabel, row, 0);
        statusLayout->addWidget(countLabel, row, 1);
        row++;
    }
    
    if (statusCounts.empty()) {
        auto noDataLabel = new QLabel("No applications found", this);
        noDataLabel->setStyleSheet("color: #7f8c8d; font-style: italic;");
        statusLayout->addWidget(noDataLabel, 0, 0, 1, 2);
    }
}

// ---------- MainWindow ----------

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    initDatabase();
}

MainWindow::~MainWindow()
{
    dbManager.closeDatabase();
}

void MainWindow::setupUI()
{
    setWindowTitle("CV Application Tracker");
    setMinimumSize(800, 600);

    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    mainLayout = new QVBoxLayout(centralWidget);

    setupButtons();
    setupTable();

    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(tableWidget);

    connectButtons();
}

void MainWindow::setupButtons()
{
    buttonLayout = new QHBoxLayout();
    addButton = new QPushButton("Add Application", this);
    editButton = new QPushButton("Edit Application", this);
    deleteButton = new QPushButton("Delete Application", this);
    refreshButton = new QPushButton("Refresh", this);
    analyticsButton = new QPushButton("Analytics", this);

    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(editButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(analyticsButton);
    buttonLayout->addStretch();
}

void MainWindow::connectButtons()
{
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addApplication);
    connect(editButton, &QPushButton::clicked, this, &MainWindow::editApplication);
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::deleteApplication);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refreshTable);
    connect(analyticsButton, &QPushButton::clicked, this, &MainWindow::showAnalytics);
    connect(tableWidget, &QTableWidget::cellDoubleClicked, this, &MainWindow::onTableDoubleClick);
}

void MainWindow::setupTable()
{
    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(7);

    const QStringList headers = {"ID", "Company", "Date", "Position", "Contact", "Status", "Notes"};
    tableWidget->setHorizontalHeaderLabels(headers);

    tableWidget->setColumnWidth(0, 50);   // ID
    tableWidget->setColumnWidth(1, 150);  // Company
    tableWidget->setColumnWidth(2, 100);  // Date
    tableWidget->setColumnWidth(3, 150);  // Position
    tableWidget->setColumnWidth(4, 120);  // Contact
    tableWidget->setColumnWidth(5, 120);  // Status
    tableWidget->setColumnWidth(6, 200);  // Notes

    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setSortingEnabled(true);

    // Make table read-only
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void MainWindow::initDatabase()
{
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dbPath);
    dbPath += "/applications.db";

    if (!dbManager.openDatabase(dbPath.toStdString())) {
        QMessageBox::critical(this, "Database Error", "Failed to open database!");
        QApplication::quit();
        return;
    }

    refreshTable();
}

void MainWindow::refreshTable()
{
    const auto& applications = dbManager.getAllApplications();
    populateTable(applications);
}

void MainWindow::populateTable(const std::vector<Application>& applications)
{
    tableWidget->setRowCount(static_cast<int>(applications.size()));
    for (size_t i = 0; i < applications.size(); ++i) {
        const auto& app = applications[i];
        tableWidget->setItem(static_cast<int>(i), 0, new QTableWidgetItem(QString::number(app.id)));
        tableWidget->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::fromStdString(app.companyName)));
        tableWidget->setItem(static_cast<int>(i), 2, new QTableWidgetItem(QString::fromStdString(app.applicationDate)));
        tableWidget->setItem(static_cast<int>(i), 3, new QTableWidgetItem(QString::fromStdString(app.position)));
        tableWidget->setItem(static_cast<int>(i), 4, new QTableWidgetItem(QString::fromStdString(app.contactPerson)));
        tableWidget->setItem(static_cast<int>(i), 5, new QTableWidgetItem(QString::fromStdString(app.status)));
        tableWidget->setItem(static_cast<int>(i), 6, new QTableWidgetItem(QString::fromStdString(app.notes)));
    }
}

Application MainWindow::getApplicationFromRow(int row) const
{
    Application app;
    app.id = tableWidget->item(row, 0)->text().toInt();
    app.companyName = tableWidget->item(row, 1)->text().toStdString();
    app.applicationDate = tableWidget->item(row, 2)->text().toStdString();
    app.position = tableWidget->item(row, 3)->text().toStdString();
    app.contactPerson = tableWidget->item(row, 4)->text().toStdString();
    app.status = tableWidget->item(row, 5)->text().toStdString();
    app.notes = tableWidget->item(row, 6)->text().toStdString();
    return app;
}

void MainWindow::addApplication()
{
    AddApplicationDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Application app = dialog.getApplication();
        if (dbManager.addApplication(app)) {
            QMessageBox::information(this, "Success", "Application added successfully!");
            refreshTable();
        } else {
            QMessageBox::warning(this, "Error", "Failed to add application. It might be a duplicate or have invalid data.");
        }
    }
}

void MainWindow::editApplication()
{
    int selectedRow = tableWidget->currentRow();
    if (selectedRow < 0) {
        QMessageBox::information(this, "No Selection", "Please select an application to edit.");
        return;
    }
    Application app = getApplicationFromRow(selectedRow);
    EditApplicationDialog dialog(app, this);
    if (dialog.exec() == QDialog::Accepted) {
        Application updatedApp = dialog.getApplication();
        if (dbManager.updateApplication(updatedApp)) {
            QMessageBox::information(this, "Success", "Application updated successfully!");
            refreshTable();
        } else {
            QMessageBox::warning(this, "Error", "Failed to update application.");
        }
    }
}

void MainWindow::deleteApplication()
{
    int selectedRow = tableWidget->currentRow();
    if (selectedRow < 0) {
        QMessageBox::information(this, "No Selection", "Please select an application to delete.");
        return;
    }
    int id = tableWidget->item(selectedRow, 0)->text().toInt();
    QString company = tableWidget->item(selectedRow, 1)->text();

    int ret = QMessageBox::question(this, "Confirm Delete",
                                    QString("Are you sure you want to delete the application for %1?").arg(company),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        if (dbManager.deleteApplication(id)) {
            QMessageBox::information(this, "Success", "Application deleted successfully!");
            refreshTable();
        } else {
            QMessageBox::warning(this, "Error", "Failed to delete application.");
        }
    }
}

void MainWindow::showAnalytics()
{
    AnalyticsDialog dialog(&dbManager, this);
    dialog.exec();
}

void MainWindow::onTableDoubleClick(int row, int)
{
    tableWidget->selectRow(row);
    editApplication();
}