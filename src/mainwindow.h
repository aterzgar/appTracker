#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDialog>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QStyledItemDelegate>
#include <QPushButton>
#include <QLineEdit>
#include <QDateEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QShowEvent>
#include <QTimer>
#include <QAction>
#include <QMenu>
#include <QStatusBar>
#include <QShortcut>
#include <QSettings>
#include <QHeaderView>
#include <QPointer>
#include <QPainter>

#include <map>
#include <memory>

#include "db_manager.h"
#include "application_service.h"
#include "analytics_visualizer.h"
#include "llm_client.h"

// ---------------------------------------------------------------------------
// Column indices — single source of truth; never use magic numbers elsewhere
// ---------------------------------------------------------------------------
namespace Col {
    constexpr int Id       = 0;
    constexpr int Company  = 1;
    constexpr int Date     = 2;
    constexpr int Position = 3;
    constexpr int Contact  = 4;
    constexpr int Status   = 5;
    constexpr int Notes    = 6;
    constexpr int Count    = 7;
}

// ---------------------------------------------------------------------------
// AppColors — shared status-to-color mapping
// ---------------------------------------------------------------------------
namespace AppColors {
    // Returns a hex color string for the given status label.
    QString forStatus(const std::string& status);

    // Returns the background color for a status badge (lighter tint).
    QColor badgeBackground(const std::string& status);

    // Returns the foreground/text color for a status badge (darker shade).
    QColor badgeForeground(const std::string& status);
}

// ---------------------------------------------------------------------------
// StatusBadgeDelegate
// Paints a rounded-rect pill badge instead of plain text in the Status column.
// ---------------------------------------------------------------------------
class StatusBadgeDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit StatusBadgeDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;
};

// ---------------------------------------------------------------------------
// AddApplicationDialog
// ---------------------------------------------------------------------------
class AddApplicationDialog : public QDialog {
    Q_OBJECT
public:
    explicit AddApplicationDialog(QWidget* parent = nullptr);
    Application getApplication() const;

private slots:
    void accept() override;

private:
    void buildForm();

    QLineEdit* m_company  = nullptr;
    QDateEdit* m_date     = nullptr;
    QLineEdit* m_position = nullptr;
    QLineEdit* m_contact  = nullptr;
    QComboBox* m_status   = nullptr;
    QTextEdit* m_notes    = nullptr;
};

// ---------------------------------------------------------------------------
// EditApplicationDialog
// ---------------------------------------------------------------------------
class EditApplicationDialog : public QDialog {
    Q_OBJECT
public:
    explicit EditApplicationDialog(const Application& app, QWidget* parent = nullptr);
    Application getApplication() const;

private slots:
    void accept() override;

private:
    void buildForm(const Application& app);

    int        m_id       = 0;
    QLineEdit* m_company  = nullptr;
    QDateEdit* m_date     = nullptr;
    QLineEdit* m_position = nullptr;
    QLineEdit* m_contact  = nullptr;
    QComboBox* m_status   = nullptr;
    QTextEdit* m_notes    = nullptr;
};

// ---------------------------------------------------------------------------
// AnalyticsDialog
// ---------------------------------------------------------------------------
class AnalyticsDialog : public QDialog {
    Q_OBJECT
public:
    explicit AnalyticsDialog(ApplicationService* service, QWidget* parent = nullptr);

    // Pass LLM model from MainWindow's QSettings storage.
    void configureLLM(const QString& model = {});

protected:
    void showEvent(QShowEvent* event) override;

private:
    void setupUI();
    void loadData();
    void populatePieChart    (const ApplicationAnalytics& a);
    void populateBarChart    (const std::vector<Application>& apps);
    void populateTrendChart  (const std::vector<Application>& apps);
    void populateProgressBars(const ApplicationAnalytics& a);
    void populateStatusGrid  (const std::map<std::string, int>& counts);

    void generateInsights();
    void onInsightsReady(const QString& insights);
    void onInsightsError(const QString& error);

    static std::map<std::string, int> aggregateInterviews(
        const std::vector<std::pair<std::string, int>>& raw);

    ApplicationService* m_service = nullptr;

    // LLM
    LLMClient* m_llmClient = nullptr;

    QLabel* m_totalLabel      = nullptr;
    QLabel* m_rejectedLabel   = nullptr;
    QLabel* m_interviewsLabel = nullptr;
    QLabel* m_offersLabel     = nullptr;

    PieChart*  m_pieChart  = nullptr;
    BarChart*  m_barChart  = nullptr;
    LineChart* m_lineChart = nullptr;

    LabeledProgressBar* m_interviewProgress = nullptr;
    LabeledProgressBar* m_offerProgress     = nullptr;

    QGroupBox*   m_statusGroup  = nullptr;
    QGridLayout* m_statusLayout = nullptr;
    QVBoxLayout* m_scrollLayout = nullptr;

    // LLM UI
    QPushButton* m_generateInsightsBtn = nullptr;
    QTextEdit*   m_insightsOutput      = nullptr;
    QGroupBox*   m_insightsGroup       = nullptr;
};

// ---------------------------------------------------------------------------
// MainWindow
// ---------------------------------------------------------------------------
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString& dbFileName, QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // Toolbar / menu actions
    void onAddApplication();
    void onEditApplication();
    void onDeleteApplication();
    void onRefresh();
    void onShowAnalytics();
    void onExportCsv();

    // Table interactions
    void onTableDoubleClicked(int row, int column);
    void onSelectionChanged();
    void onContextMenuRequested(const QPoint& pos);

    // Search with debounce — textChanged triggers the timer, the timer fires the query
    void onSearchTextChanged(const QString& text);
    void onSearchCommit();

private:
    // ---- Setup ----
    void setupUI();
    void setupToolbar();
    void setupTable();
    void setupStatusBar();
    void setupShortcuts();
    void connectSignals();
    void initDatabase();

    // ---- Column-layout persistence ----
    void saveColumnWidths();
    void restoreColumnWidths();

    // ---- Table helpers ----
    void        populateTable(const std::vector<Application>& apps);
    Application appFromRow(int row) const;
    void        updateToolbarState();
    void        updateStatusBar(const std::vector<Application>& apps);

    // ---- Widgets ----
    QWidget*      m_centralWidget = nullptr;
    QVBoxLayout*  m_mainLayout    = nullptr;
    QHBoxLayout*  m_toolbarLayout = nullptr;
    QTableWidget* m_table         = nullptr;

    // Toolbar buttons
    QPushButton* m_addBtn        = nullptr;
    QPushButton* m_editBtn       = nullptr;
    QPushButton* m_deleteBtn     = nullptr;
    QPushButton* m_refreshBtn    = nullptr;
    QPushButton* m_analyticsBtn  = nullptr;
    QPushButton* m_exportBtn     = nullptr;

    // Search + filter
    QLineEdit* m_searchEdit     = nullptr;
    QComboBox* m_statusFilter   = nullptr;
    QTimer*    m_searchDebounce = nullptr;

    // Status bar labels
    QLabel* m_sbTotal      = nullptr;
    QLabel* m_sbApplied    = nullptr;
    QLabel* m_sbInterviews = nullptr;
    QLabel* m_sbOffers     = nullptr;
    QLabel* m_sbRejected   = nullptr;

    // Context-menu actions (owned by the QMenu, held here to enable/disable)
    QAction* m_ctxEditAction   = nullptr;
    QAction* m_ctxDeleteAction = nullptr;

    // Analytics dialog — single instance at a time
    QPointer<AnalyticsDialog> m_analyticsDialog;

    // ---- Data ----
    QString                             m_dbFileName;
    DatabaseManager                     m_db;
    std::unique_ptr<ApplicationService> m_service;

    // ---- Settings (column-width persistence) ----
    QSettings m_settings;
};

#endif // MAINWINDOW_H