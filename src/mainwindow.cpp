#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTextStream>
#include <QFile>
#include <QKeySequence>
#include <QPainterPath>
#include <QDebug>

#include <algorithm>
#include <map>

// ===========================================================================
// AppColors
// ===========================================================================

namespace AppColors {

QString forStatus(const std::string& status) {
    if (status == "Rejected")      return "#e74c3c";
    if (status == "Interviews")    return "#f39c12";
    if (status == "Offer Received")return "#27ae60";
    if (status == "Applied")       return "#3498db";
    if (status == "Withdrawn")     return "#95a5a6";
    return "#34495e";
}

QColor badgeBackground(const std::string& status) {
    if (status == "Rejected")      return QColor("#fdeaea");
    if (status == "Interviews")    return QColor("#fef3e0");
    if (status == "Offer Received")return QColor("#eaf7ee");
    if (status == "Applied")       return QColor("#e8f4fd");
    if (status == "Withdrawn")     return QColor("#f5f4f2");
    return QColor("#f0f0f0");
}

QColor badgeForeground(const std::string& status) {
    if (status == "Rejected")      return QColor("#791f1f");
    if (status == "Interviews")    return QColor("#854f0b");
    if (status == "Offer Received")return QColor("#3b6d11");
    if (status == "Applied")       return QColor("#185fa5");
    if (status == "Withdrawn")     return QColor("#5f5e5a");
    return QColor("#333333");
}

} // namespace AppColors

// ===========================================================================
// Shared form helpers (file-local)
// ===========================================================================

namespace {

static const QStringList kStatusOptions = {
    "Applied", "Interviews", "Offer Received", "Rejected", "Withdrawn"
};

QComboBox* makeStatusCombo(QWidget* parent, const QString& current = {}) {
    auto* box = new QComboBox(parent);
    box->addItems(kStatusOptions);
    if (!current.isEmpty()) box->setCurrentText(current);
    return box;
}

// Returns true if all three required fields are non-empty; shows a warning otherwise.
bool requireFields(const QLineEdit* company,
                   const QLineEdit* position,
                   const QLineEdit* contact,
                   QWidget*         parent)
{
    if (company->text().trimmed().isEmpty()  ||
        position->text().trimmed().isEmpty() ||
        contact->text().trimmed().isEmpty())
    {
        QMessageBox::warning(parent, "Validation Error",
            "Please fill in all required fields: Company, Position, Contact Person.");
        return false;
    }
    return true;
}

// Builds a small separator widget for the toolbar.
QWidget* makeSeparator(QWidget* parent) {
    auto* sep = new QWidget(parent);
    sep->setFixedSize(1, 22);
    sep->setStyleSheet("background: palette(mid);");
    return sep;
}

} // namespace

// ===========================================================================
// StatusBadgeDelegate
// ===========================================================================

StatusBadgeDelegate::StatusBadgeDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

void StatusBadgeDelegate::paint(QPainter* painter,
                                const QStyleOptionViewItem& option,
                                const QModelIndex& index) const
{
    const QString text   = index.data(Qt::DisplayRole).toString();
    const std::string s  = text.toStdString();
    const QColor   bg    = AppColors::badgeBackground(s);
    const QColor   fg    = AppColors::badgeForeground(s);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Draw row selection/hover background first (delegate is responsible for this).
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    } else if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(option.rect, option.palette.alternateBase());
    }

    // Compute pill geometry — vertically centered in the cell.
    const QFontMetrics fm(option.font);
    const int textW = fm.horizontalAdvance(text);
    const int padH  = 4;   // vertical padding inside pill
    const int padW  = 10;  // horizontal padding inside pill
    const int pillW = textW + padW * 2;
    const int pillH = fm.height() + padH * 2;
    const int x     = option.rect.left() + 8;
    const int y     = option.rect.top()  + (option.rect.height() - pillH) / 2;

    // Draw the colored dot before the text
    const int dotR  = 4;
    const int dotCX = x + dotR;
    const int dotCY = y + pillH / 2;

    QPainterPath pill;
    pill.addRoundedRect(QRectF(x, y, pillW + dotR * 2 + 6, pillH), pillH / 2.0, pillH / 2.0);

    painter->setPen(Qt::NoPen);
    painter->setBrush(bg);
    painter->drawPath(pill);

    painter->setBrush(fg);
    painter->drawEllipse(QPointF(dotCX + 2, dotCY), dotR - 1, dotR - 1);

    painter->setPen(fg);
    painter->setFont(option.font);
    painter->drawText(QRect(dotCX + dotR + 5, y, textW + padW, pillH),
                      Qt::AlignVCenter | Qt::AlignLeft, text);

    painter->restore();
}

QSize StatusBadgeDelegate::sizeHint(const QStyleOptionViewItem& option,
                                     const QModelIndex& index) const
{
    const QSize base = QStyledItemDelegate::sizeHint(option, index);
    return { base.width(), qMax(base.height(), 32) };
}

// ===========================================================================
// AddApplicationDialog
// ===========================================================================

AddApplicationDialog::AddApplicationDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Add New Application");
    setModal(true);
    resize(440, 340);
    buildForm();
}

void AddApplicationDialog::buildForm() {
    auto* layout = new QFormLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(20, 20, 20, 20);

    m_company  = new QLineEdit(this);
    m_company->setPlaceholderText("e.g. Acme Corp");
    m_date     = new QDateEdit(QDate::currentDate(), this);
    m_date->setCalendarPopup(true);
    m_date->setDisplayFormat("yyyy-MM-dd");
    m_position = new QLineEdit(this);
    m_position->setPlaceholderText("e.g. Senior Engineer");
    m_contact  = new QLineEdit(this);
    m_contact->setPlaceholderText("e.g. Jane Smith");
    m_status   = makeStatusCombo(this);
    m_notes    = new QTextEdit(this);
    m_notes->setPlaceholderText("Optional notes…");
    m_notes->setMaximumHeight(90);

    layout->addRow("Company Name *",     m_company);
    layout->addRow("Application Date",   m_date);
    layout->addRow("Position *",         m_position);
    layout->addRow("Contact Person *",   m_contact);
    layout->addRow("Status",             m_status);
    layout->addRow("Notes",              m_notes);

    auto* okBtn     = new QPushButton("Add Application", this);
    auto* cancelBtn = new QPushButton("Cancel", this);
    okBtn->setDefault(true);

    auto* btns = new QHBoxLayout();
    btns->addStretch();
    btns->addWidget(cancelBtn);
    btns->addWidget(okBtn);
    layout->addRow(btns);

    connect(okBtn,     &QPushButton::clicked, this, &AddApplicationDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void AddApplicationDialog::accept() {
    if (!requireFields(m_company, m_position, m_contact, this)) return;
    QDialog::accept();
}

Application AddApplicationDialog::getApplication() const {
    Application app;
    app.companyName     = m_company->text().trimmed().toStdString();
    app.applicationDate = m_date->date().toString("yyyy-MM-dd").toStdString();
    app.position        = m_position->text().trimmed().toStdString();
    app.contactPerson   = m_contact->text().trimmed().toStdString();
    app.status          = m_status->currentText().toStdString();
    app.notes           = m_notes->toPlainText().toStdString();
    return app;
}

// ===========================================================================
// EditApplicationDialog
// ===========================================================================

EditApplicationDialog::EditApplicationDialog(const Application& app, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Edit Application");
    setModal(true);
    resize(440, 340);
    buildForm(app);
}

void EditApplicationDialog::buildForm(const Application& app) {
    m_id = app.id;

    auto* layout = new QFormLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(20, 20, 20, 20);

    m_company  = new QLineEdit(QString::fromStdString(app.companyName), this);
    m_date     = new QDateEdit(
        QDate::fromString(QString::fromStdString(app.applicationDate), "yyyy-MM-dd"), this);
    m_date->setCalendarPopup(true);
    m_date->setDisplayFormat("yyyy-MM-dd");
    m_position = new QLineEdit(QString::fromStdString(app.position), this);
    m_contact  = new QLineEdit(QString::fromStdString(app.contactPerson), this);
    m_status   = makeStatusCombo(this, QString::fromStdString(app.status));
    m_notes    = new QTextEdit(QString::fromStdString(app.notes), this);
    m_notes->setMaximumHeight(90);

    layout->addRow("Company Name *",   m_company);
    layout->addRow("Application Date", m_date);
    layout->addRow("Position *",       m_position);
    layout->addRow("Contact Person *", m_contact);
    layout->addRow("Status",           m_status);
    layout->addRow("Notes",            m_notes);

    auto* okBtn     = new QPushButton("Save Changes", this);
    auto* cancelBtn = new QPushButton("Cancel", this);
    okBtn->setDefault(true);

    auto* btns = new QHBoxLayout();
    btns->addStretch();
    btns->addWidget(cancelBtn);
    btns->addWidget(okBtn);
    layout->addRow(btns);

    connect(okBtn,     &QPushButton::clicked, this, &EditApplicationDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void EditApplicationDialog::accept() {
    if (!requireFields(m_company, m_position, m_contact, this)) return;
    QDialog::accept();
}

Application EditApplicationDialog::getApplication() const {
    Application app;
    app.id              = m_id;
    app.companyName     = m_company->text().trimmed().toStdString();
    app.applicationDate = m_date->date().toString("yyyy-MM-dd").toStdString();
    app.position        = m_position->text().trimmed().toStdString();
    app.contactPerson   = m_contact->text().trimmed().toStdString();
    app.status          = m_status->currentText().toStdString();
    app.notes           = m_notes->toPlainText().toStdString();
    return app;
}

// ===========================================================================
// AnalyticsDialog
// ===========================================================================

AnalyticsDialog::AnalyticsDialog(ApplicationService* service, QWidget* parent)
    : QDialog(parent), m_service(service), m_llmClient(new LLMClient(this))
{
    setWindowTitle("Application Analytics");
    resize(1000, 900);
    setupUI();

    // Wire LLM signals
    connect(m_llmClient,  &LLMClient::insightsReady, this, &AnalyticsDialog::onInsightsReady);
    connect(m_llmClient,  &LLMClient::errorOccurred, this, &AnalyticsDialog::onInsightsError);
}

void AnalyticsDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* summaryGroup = new QGroupBox("Summary Statistics", this);
    auto* summaryGrid  = new QGridLayout(summaryGroup);

    m_totalLabel      = new QLabel(this);
    m_rejectedLabel   = new QLabel(this);
    m_interviewsLabel = new QLabel(this);
    m_offersLabel     = new QLabel(this);

    m_totalLabel->setStyleSheet("font-weight:bold; font-size:14px; color:#6014bc;");
    m_rejectedLabel->setStyleSheet("font-size:12px; color:#e74c3c;");
    m_interviewsLabel->setStyleSheet("font-size:12px; color:#f39c12;");
    m_offersLabel->setStyleSheet("font-size:12px; color:#27ae60;");

    summaryGrid->addWidget(m_totalLabel,      0, 0, 1, 2);
    summaryGrid->addWidget(m_rejectedLabel,   1, 0);
    summaryGrid->addWidget(m_interviewsLabel, 1, 1);
    summaryGrid->addWidget(m_offersLabel,     2, 0);
    mainLayout->addWidget(summaryGroup);

    auto* scroll       = new QScrollArea(this);
    auto* scrollWidget = new QWidget();
    m_scrollLayout     = new QVBoxLayout(scrollWidget);
    scroll->setWidget(scrollWidget);
    scroll->setWidgetResizable(true);
    mainLayout->addWidget(scroll);

    m_pieChart  = new PieChart(scrollWidget);
    m_barChart  = new BarChart(scrollWidget);
    m_lineChart = new LineChart(scrollWidget);
    m_scrollLayout->addWidget(m_pieChart);
    m_scrollLayout->addWidget(m_barChart);
    m_scrollLayout->addWidget(m_lineChart);

    auto* progressGroup  = new QGroupBox("Progress Tracking", scrollWidget);
    auto* progressLayout = new QHBoxLayout(progressGroup);
    m_interviewProgress  = new LabeledProgressBar("Interviews", 0, 5, "#f39c12", progressGroup);
    m_offerProgress      = new LabeledProgressBar("Offers",     0, 3, "#27ae60", progressGroup);
    progressLayout->addWidget(m_interviewProgress);
    progressLayout->addWidget(m_offerProgress);
    m_scrollLayout->addWidget(progressGroup);

    m_statusGroup  = new QGroupBox("Detailed Status Breakdown", scrollWidget);
    m_statusLayout = new QGridLayout(m_statusGroup);
    m_scrollLayout->addWidget(m_statusGroup);
    m_scrollLayout->addStretch();

    // --- AI Insights section ---
    m_insightsGroup = new QGroupBox("✨ AI Insights", this);
    auto* insightsLayout = new QVBoxLayout(m_insightsGroup);

    m_generateInsightsBtn = new QPushButton("Generate AI Insights", this);
    m_generateInsightsBtn->setToolTip("Analyze your application data and get AI-powered recommendations");
    m_generateInsightsBtn->setEnabled(false); // enabled only when LLM is configured
    insightsLayout->addWidget(m_generateInsightsBtn);

    m_insightsOutput = new QTextEdit(this);
    m_insightsOutput->setReadOnly(true);
    m_insightsOutput->setPlaceholderText("Click 'Generate AI Insights' to get AI-powered analysis of your job application data...");
    m_insightsOutput->setMinimumHeight(150);
    m_insightsOutput->setStyleSheet("QTextEdit { background-color: #1e1e2e; color: #cdd6f4; font-family: monospace; font-size: 12px; border: 1px solid #45475a; border-radius: 4px; padding: 8px; }");
    insightsLayout->addWidget(m_insightsOutput);

    connect(m_generateInsightsBtn, &QPushButton::clicked, this, &AnalyticsDialog::generateInsights);

    mainLayout->addWidget(m_insightsGroup);

    auto* closeBtn = new QPushButton("Close", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    mainLayout->addWidget(closeBtn);
}

void AnalyticsDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    loadData();
}

void AnalyticsDialog::loadData() {
    if (!m_service) return;

    const auto analytics = m_service->getAnalytics();
    const auto apps      = m_service->listApplications();
    const auto merged    = aggregateInterviews(analytics.statusCounts);

    m_totalLabel->setText     (QString("Total Applications: %1").arg(analytics.total));
    m_rejectedLabel->setText  (QString("🔴 Rejected: %1").arg(analytics.rejected));
    m_interviewsLabel->setText(QString("🟠 Interviews: %1").arg(analytics.interviews));
    m_offersLabel->setText    (QString("🟢 Offers: %1").arg(analytics.offers));

    populatePieChart    (analytics);
    populateBarChart    (apps);
    populateTrendChart  (apps);
    populateProgressBars(analytics);
    populateStatusGrid  (merged);
}

// static
std::map<std::string, int> AnalyticsDialog::aggregateInterviews(
    const std::vector<std::pair<std::string, int>>& raw)
{
    std::map<std::string, int> result;
    int interviewTotal = 0;
    for (const auto& [status, count] : raw) {
        if (status == "Interviews") interviewTotal += count;
        else                        result[status]  = count;
    }
    if (interviewTotal > 0) result["Interviews"] = interviewTotal;
    return result;
}

void AnalyticsDialog::populatePieChart(const ApplicationAnalytics& a) {
    const auto merged = aggregateInterviews(a.statusCounts);
    std::vector<PieChart::DataPoint> data;
    data.reserve(merged.size());
    for (const auto& [status, count] : merged)
        data.push_back({ QString::fromStdString(status), count,
                         AppColors::forStatus(status) });
    m_pieChart->setData(data);
    m_pieChart->setTitle(QString("Status Breakdown (Total: %1)").arg(a.total));
}

void AnalyticsDialog::populateBarChart(const std::vector<Application>& apps) {
    std::map<std::string, int> counts;
    for (const auto& app : apps) counts[app.companyName]++;

    std::vector<std::pair<std::string, int>> sorted(counts.begin(), counts.end());
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b){ return a.second > b.second; });

    std::vector<BarChart::Bar> bars;
    const int limit = std::min(5, static_cast<int>(sorted.size()));
    bars.reserve(limit);
    for (int i = 0; i < limit; ++i) {
        QString label = QString::fromStdString(sorted[i].first);
        if (label.length() > 15) label = label.left(12) + "…";
        bars.push_back({ label, sorted[i].second, "#3498db" });
    }
    m_barChart->setData(bars);
}

void AnalyticsDialog::populateTrendChart(const std::vector<Application>& apps) {
    std::map<std::string, int> byMonth;
    for (const auto& app : apps)
        if (app.applicationDate.size() >= 7)
            byMonth[app.applicationDate.substr(0, 7)]++;

    std::vector<LineChart::Point> points;
    points.reserve(byMonth.size());
    for (const auto& [month, count] : byMonth)
        points.push_back({ QString::fromStdString(month.substr(5, 2)), count });

    m_lineChart->setData(points);
}

void AnalyticsDialog::populateProgressBars(const ApplicationAnalytics& a) {
    m_interviewProgress->setValue(a.interviews, std::max(5, a.interviews + 2));
    m_offerProgress->setValue    (a.offers,     std::max(3, a.offers    + 1));
}

void AnalyticsDialog::populateStatusGrid(const std::map<std::string, int>& counts) {
    while (QLayoutItem* item = m_statusLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    if (counts.empty()) {
        auto* lbl = new QLabel("No applications found.", m_statusGroup);
        lbl->setStyleSheet("color:#7f8c8d; font-style:italic;");
        m_statusLayout->addWidget(lbl, 0, 0, 1, 2);
        return;
    }

    int row = 0;
    for (const auto& [status, count] : counts) {
        const QString color = AppColors::forStatus(status);
        const QString style = QString("font-weight:bold; color:%1;").arg(color);

        auto* keyLbl = new QLabel(QString::fromStdString(status) + ":", m_statusGroup);
        auto* valLbl = new QLabel(QString::number(count), m_statusGroup);
        keyLbl->setStyleSheet(style);
        valLbl->setStyleSheet(style);

        m_statusLayout->addWidget(keyLbl, row, 0);
        m_statusLayout->addWidget(valLbl, row, 1);
        ++row;
    }
}

// ---------------------------------------------------------------------------
// AnalyticsDialog — LLM integration
// ---------------------------------------------------------------------------

void AnalyticsDialog::configureLLM(const QString& model) {
    if (m_llmClient) {
        if (!model.isEmpty()) m_llmClient->setModel(model);
        m_generateInsightsBtn->setEnabled(m_llmClient->isConfigured());
    }
}

void AnalyticsDialog::generateInsights() {
    if (!m_service || !m_llmClient || !m_llmClient->isConfigured()) return;

    const auto analytics = m_service->getAnalytics();
    const auto apps      = m_service->listApplications();
    const QString prompt = QString::fromStdString(
        m_service->buildInsightsPrompt(analytics, apps));

    m_generateInsightsBtn->setEnabled(false);
    m_generateInsightsBtn->setText("⏳ Generating...");
    m_insightsOutput->setText("Analyzing your application data...");

    m_llmClient->generateInsights(prompt);
}

void AnalyticsDialog::onInsightsReady(const QString& insights) {
    m_insightsOutput->setText(insights);
    m_generateInsightsBtn->setEnabled(m_llmClient->isConfigured());
    m_generateInsightsBtn->setText("Generate AI Insights");
}

void AnalyticsDialog::onInsightsError(const QString& error) {
    QString msg = error;
    if (msg.contains("connection", Qt::CaseInsensitive) || msg.contains("refused", Qt::CaseInsensitive)) {
        msg = "Cannot connect to Ollama.\n\n"
              "Make sure Ollama is running locally:\n"
              "  Install: curl -fsSL https://ollama.com/install.sh | sh\n"
              "  Run:     ollama run llama3.2";
    }
    m_insightsOutput->setText(QString("⚠️ Error: %1").arg(msg));
    m_generateInsightsBtn->setEnabled(true);
    m_generateInsightsBtn->setText("Generate AI Insights");
}

// ===========================================================================
// MainWindow — construction
// ===========================================================================

MainWindow::MainWindow(const QString& dbFileName, QWidget* parent)
    : QMainWindow(parent)
    , m_dbFileName(dbFileName)
    , m_settings("AppTracker", "AppTracker")
{
    // --- LLM configuration (Ollama local) ---
    // Install: curl -fsSL https://ollama.com/install.sh | sh
    // Run:     ollama run llama3.2
    m_settings.setValue("llm/model", "llama3.2");

    setupUI();
    // initDatabase() is deferred so the window is fully constructed first.
    // Use a zero-delay timer so it runs on the next event-loop iteration.
    QTimer::singleShot(0, this, &MainWindow::initDatabase);
}

MainWindow::~MainWindow() {
    saveColumnWidths();
    m_db.closeDatabase();
}

// ===========================================================================
// MainWindow — UI setup
// ===========================================================================

void MainWindow::setupUI() {
    setWindowTitle("Application Tracker");
    setMinimumSize(900, 600);

    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    setupToolbar();
    setupTable();
    setupStatusBar();
    setupShortcuts();

    m_mainLayout->addLayout(m_toolbarLayout);
    m_mainLayout->addWidget(m_table);

    connectSignals();
}

void MainWindow::setupToolbar() {
    m_toolbarLayout = new QHBoxLayout();
    m_toolbarLayout->setContentsMargins(8, 6, 8, 6);
    m_toolbarLayout->setSpacing(4);

    m_addBtn       = new QPushButton("+ New Application", this);
    m_editBtn      = new QPushButton("Edit",              this);
    m_deleteBtn    = new QPushButton("Delete",            this);
    m_refreshBtn   = new QPushButton("Refresh",           this);
    m_analyticsBtn = new QPushButton("Analytics",         this);
    m_exportBtn    = new QPushButton("Export CSV",        this);

    // Edit and Delete are disabled until the user selects a row.
    m_editBtn->setEnabled(false);
    m_deleteBtn->setEnabled(false);

    // Status filter combo
    m_statusFilter = new QComboBox(this);
    m_statusFilter->addItem("All Statuses", "");
    for (const auto& s : { "Applied", "Interviews",
                            "Offer Received", "Rejected", "Withdrawn" })
        m_statusFilter->addItem(s, s);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search…");
    m_searchEdit->setFixedWidth(200);
    m_searchEdit->setClearButtonEnabled(true);

    m_searchDebounce = new QTimer(this);
    m_searchDebounce->setSingleShot(true);
    m_searchDebounce->setInterval(250);   // ms

    m_toolbarLayout->addWidget(m_addBtn);
    m_toolbarLayout->addWidget(makeSeparator(this));
    m_toolbarLayout->addWidget(m_editBtn);
    m_toolbarLayout->addWidget(m_deleteBtn);
    m_toolbarLayout->addWidget(makeSeparator(this));
    m_toolbarLayout->addWidget(m_refreshBtn);
    m_toolbarLayout->addWidget(m_analyticsBtn);
    m_toolbarLayout->addWidget(m_exportBtn);
    m_toolbarLayout->addStretch();
    m_toolbarLayout->addWidget(m_statusFilter);
    m_toolbarLayout->addWidget(new QLabel("Search:", this));
    m_toolbarLayout->addWidget(m_searchEdit);
}

void MainWindow::setupTable() {
    m_table = new QTableWidget(this);
    m_table->setColumnCount(Col::Count);
    m_table->setHorizontalHeaderLabels({
        "ID", "Company", "Date", "Position", "Contact", "Status", "Notes"
    });

    // Hide the ID column — still present for data lookup, just invisible.
    m_table->setColumnHidden(Col::Id, true);

    m_table->horizontalHeader()->setSectionResizeMode(Col::Notes, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSortIndicatorShown(true);
    m_table->horizontalHeader()->setSectionsClickable(true);

    // Install the pill-badge delegate on the Status column.
    m_table->setItemDelegateForColumn(Col::Status, new StatusBadgeDelegate(m_table));

    m_table->setSelectionBehavior (QAbstractItemView::SelectRows);
    m_table->setEditTriggers      (QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled    (true);
    m_table->setShowGrid          (false);
    m_table->verticalHeader()->setVisible(false);
    m_table->setContextMenuPolicy (Qt::CustomContextMenu);

    // Restore column widths from the previous session.
    restoreColumnWidths();
}

void MainWindow::setupStatusBar() {
    auto* sb = statusBar();   // QMainWindow provides this

    m_sbTotal      = new QLabel(this);
    m_sbApplied    = new QLabel(this);
    m_sbInterviews = new QLabel(this);
    m_sbOffers     = new QLabel(this);
    m_sbRejected   = new QLabel(this);

    m_sbInterviews->setStyleSheet("color:#f39c12;");
    m_sbOffers->setStyleSheet    ("color:#27ae60;");
    m_sbRejected->setStyleSheet  ("color:#e74c3c;");

    sb->addWidget(m_sbTotal);
    sb->addWidget(new QLabel("·", this));
    sb->addWidget(m_sbApplied);
    sb->addWidget(new QLabel("·", this));
    sb->addWidget(m_sbInterviews);
    sb->addWidget(new QLabel("·", this));
    sb->addWidget(m_sbOffers);
    sb->addWidget(new QLabel("·", this));
    sb->addWidget(m_sbRejected);
}

void MainWindow::setupShortcuts() {
    // Ctrl+N — new application
    new QShortcut(QKeySequence::New, this, this, &MainWindow::onAddApplication);

    // Delete key — delete selected application
    new QShortcut(Qt::Key_Delete, this, this, &MainWindow::onDeleteApplication);

    // F5 — refresh
    new QShortcut(Qt::Key_F5, this, this, &MainWindow::onRefresh);

    // Ctrl+E — edit selected application
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_E), this, this,
                  &MainWindow::onEditApplication);

    // Escape — clear search
    new QShortcut(Qt::Key_Escape, m_searchEdit, m_searchEdit, &QLineEdit::clear);
}

void MainWindow::connectSignals() {
    connect(m_addBtn,       &QPushButton::clicked,
            this, &MainWindow::onAddApplication);
    connect(m_editBtn,      &QPushButton::clicked,
            this, &MainWindow::onEditApplication);
    connect(m_deleteBtn,    &QPushButton::clicked,
            this, &MainWindow::onDeleteApplication);
    connect(m_refreshBtn,   &QPushButton::clicked,
            this, &MainWindow::onRefresh);
    connect(m_analyticsBtn, &QPushButton::clicked,
            this, &MainWindow::onShowAnalytics);
    connect(m_exportBtn,    &QPushButton::clicked,
            this, &MainWindow::onExportCsv);

    // Table signals
    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, &MainWindow::onTableDoubleClicked);
    connect(m_table, &QTableWidget::customContextMenuRequested,
            this, &MainWindow::onContextMenuRequested);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onSelectionChanged);

    // Search debounce
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &MainWindow::onSearchTextChanged);
    connect(m_searchDebounce, &QTimer::timeout,
            this, &MainWindow::onSearchCommit);

    // Status filter triggers an immediate search (no debounce needed)
    connect(m_statusFilter, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &MainWindow::onSearchCommit);
}

// ===========================================================================
// MainWindow — database initialisation
// ===========================================================================

void MainWindow::initDatabase() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    const QString path = dir + "/" + m_dbFileName;
    qDebug() << "Database path:" << path;

    if (!m_db.openDatabase(path.toStdString())) {
        QMessageBox::critical(this, "Database Error",
            "Failed to open the database. The application will now close.");
        // Schedule quit so we don't exit from inside a constructor call chain.
        QTimer::singleShot(0, qApp, &QApplication::quit);
        return;
    }

    m_service = std::make_unique<ApplicationService>(&m_db);
    onRefresh();
}

// ===========================================================================
// MainWindow — column-width persistence
// ===========================================================================

void MainWindow::saveColumnWidths() {
    if (!m_table) return;
    for (int c = 0; c < m_table->columnCount(); ++c)
        m_settings.setValue(QString("col/%1").arg(c), m_table->columnWidth(c));
}

void MainWindow::restoreColumnWidths() {
    // Defaults if no saved settings exist yet.
    const QList<int> defaults = { 0, 160, 100, 160, 130, 130, 200 };
    for (int c = 0; c < m_table->columnCount(); ++c) {
        const int w = m_settings.value(QString("col/%1").arg(c),
                                       defaults.value(c, 120)).toInt();
        if (w > 0) m_table->setColumnWidth(c, w);
    }
}

// ===========================================================================
// MainWindow — table population & helpers
// ===========================================================================

void MainWindow::populateTable(const std::vector<Application>& apps) {
    // Disable sorting while inserting rows to avoid O(n²) re-sorts.
    m_table->setSortingEnabled(false);
    m_table->setRowCount(static_cast<int>(apps.size()));

    for (int i = 0; i < static_cast<int>(apps.size()); ++i) {
        const auto& a = apps[i];

        auto makeItem = [](const QString& val) {
            auto* item = new QTableWidgetItem(val);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            return item;
        };

        m_table->setItem(i, Col::Id,       makeItem(QString::number(a.id)));
        m_table->setItem(i, Col::Company,  makeItem(QString::fromStdString(a.companyName)));
        m_table->setItem(i, Col::Date,     makeItem(QString::fromStdString(a.applicationDate)));
        m_table->setItem(i, Col::Position, makeItem(QString::fromStdString(a.position)));
        m_table->setItem(i, Col::Contact,  makeItem(QString::fromStdString(a.contactPerson)));
        m_table->setItem(i, Col::Status,   makeItem(QString::fromStdString(a.status)));
        m_table->setItem(i, Col::Notes,    makeItem(QString::fromStdString(a.notes)));
    }

    m_table->setSortingEnabled(true);
    updateStatusBar(apps);
}

Application MainWindow::appFromRow(int row) const {
    // Guard against null cells (e.g. if a sort causes a partially-constructed row).
    auto text = [&](int col) -> QString {
        const QTableWidgetItem* item = m_table->item(row, col);
        return item ? item->text() : QString{};
    };

    Application app;
    app.id              = text(Col::Id).toInt();
    app.companyName     = text(Col::Company).toStdString();
    app.applicationDate = text(Col::Date).toStdString();
    app.position        = text(Col::Position).toStdString();
    app.contactPerson   = text(Col::Contact).toStdString();
    app.status          = text(Col::Status).toStdString();
    app.notes           = text(Col::Notes).toStdString();
    return app;
}

void MainWindow::updateToolbarState() {
    const bool hasSelection = m_table->currentRow() >= 0 &&
                              !m_table->selectedItems().isEmpty();
    m_editBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection);

    if (m_ctxEditAction)   m_ctxEditAction->setEnabled(hasSelection);
    if (m_ctxDeleteAction) m_ctxDeleteAction->setEnabled(hasSelection);
}

void MainWindow::updateStatusBar(const std::vector<Application>& apps) {
    int applied = 0, interviews = 0, offers = 0, rejected = 0;
    for (const auto& a : apps) {
        if      (a.status == "Applied")       ++applied;
        else if (a.status == "Interviews")    ++interviews;
        else if (a.status == "Offer Received")++offers;
        else if (a.status == "Rejected")      ++rejected;
    }

    m_sbTotal->setText     (QString("%1 application%2")
                            .arg(apps.size()).arg(apps.size() == 1 ? "" : "s"));
    m_sbApplied->setText   (QString("%1 applied").arg(applied));
    m_sbInterviews->setText(QString("%1 interviews").arg(interviews));
    m_sbOffers->setText    (QString("%1 offers").arg(offers));
    m_sbRejected->setText  (QString("%1 rejected").arg(rejected));
}

// ===========================================================================
// MainWindow — slots
// ===========================================================================

void MainWindow::onRefresh() {
    if (!m_service) return;
    populateTable(m_service->listApplications());
    updateToolbarState();
}

void MainWindow::onAddApplication() {
    AddApplicationDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    auto [ok, err] = m_service->addApplication(dlg.getApplication());
    if (ok) {
        onRefresh();
        statusBar()->showMessage("Application added.", 3000);
    } else {
        QMessageBox::warning(this, "Error", QString::fromStdString(err));
    }
}

void MainWindow::onEditApplication() {
    const int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "No Selection",
            "Please select an application to edit.");
        return;
    }

    EditApplicationDialog dlg(appFromRow(row), this);
    if (dlg.exec() != QDialog::Accepted) return;

    auto [ok, err] = m_service->updateApplication(dlg.getApplication());
    if (ok) {
        onRefresh();
        statusBar()->showMessage("Application updated.", 3000);
    } else {
        QMessageBox::warning(this, "Error", QString::fromStdString(err));
    }
}

void MainWindow::onDeleteApplication() {
    const int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "No Selection",
            "Please select an application to delete.");
        return;
    }

    const int     id      = m_table->item(row, Col::Id)->text().toInt();
    const QString company = m_table->item(row, Col::Company)->text();

    const auto reply = QMessageBox::question(
        this, "Confirm Delete",
        QString("Delete the application for \"%1\"?\n\nThis cannot be undone.").arg(company),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    auto [ok, err] = m_service->deleteApplication(id);
    if (ok) {
        onRefresh();
        statusBar()->showMessage(
            QString("Deleted application for \"%1\".").arg(company), 3000);
    } else {
        QMessageBox::warning(this, "Error", QString::fromStdString(err));
    }
}

void MainWindow::onShowAnalytics() {
    // Reuse an existing open dialog instead of stacking multiple instances.
    if (m_analyticsDialog) {
        m_analyticsDialog->raise();
        m_analyticsDialog->activateWindow();
        return;
    }
    m_analyticsDialog = new AnalyticsDialog(m_service.get(), this);
    m_analyticsDialog->setAttribute(Qt::WA_DeleteOnClose);

    // Pass LLM model from settings
    m_analyticsDialog->configureLLM(
        m_settings.value("llm/model").toString()
    );

    m_analyticsDialog->show();
}

void MainWindow::onExportCsv() {
    if (!m_service) return;

    const QString path = QFileDialog::getSaveFileName(
        this, "Export to CSV", QDir::homePath() + "/applications.csv",
        "CSV Files (*.csv)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Failed",
            "Could not open the file for writing:\n" + path);
        return;
    }

    QTextStream out(&file);
    out << "Company,Date,Position,Contact,Status,Notes\n";

    for (const auto& a : m_service->listApplications()) {
        // Quote fields that may contain commas.
        auto q = [](const std::string& s) -> QString {
            QString v = QString::fromStdString(s);
            if (v.contains(',') || v.contains('"') || v.contains('\n'))
                return "\"" + v.replace("\"", "\"\"") + "\"";
            return v;
        };
        out << q(a.companyName) << ","
            << q(a.applicationDate) << ","
            << q(a.position) << ","
            << q(a.contactPerson) << ","
            << q(a.status) << ","
            << q(a.notes) << "\n";
    }

    statusBar()->showMessage(
        QString("Exported %1 rows to %2").arg(m_service->listApplications().size()).arg(path),
        4000);
}

void MainWindow::onTableDoubleClicked(int row, int /*column*/) {
    m_table->selectRow(row);
    onEditApplication();
}

void MainWindow::onSelectionChanged() {
    updateToolbarState();
}

void MainWindow::onContextMenuRequested(const QPoint& pos) {
    const QModelIndex index = m_table->indexAt(pos);
    if (index.isValid()) m_table->selectRow(index.row());

    QMenu menu(this);

    m_ctxEditAction   = menu.addAction("Edit Application");
    m_ctxDeleteAction = menu.addAction("Delete Application");
    menu.addSeparator();
    QAction* analyticsAction = menu.addAction("View Analytics");

    const bool hasSelection = index.isValid();
    m_ctxEditAction->setEnabled(hasSelection);
    m_ctxDeleteAction->setEnabled(hasSelection);

    connect(m_ctxEditAction,   &QAction::triggered, this, &MainWindow::onEditApplication);
    connect(m_ctxDeleteAction, &QAction::triggered, this, &MainWindow::onDeleteApplication);
    connect(analyticsAction,   &QAction::triggered, this, &MainWindow::onShowAnalytics);

    menu.exec(m_table->viewport()->mapToGlobal(pos));

    // Reset raw pointers — the menu (and its actions) are about to be destroyed.
    m_ctxEditAction   = nullptr;
    m_ctxDeleteAction = nullptr;
}

void MainWindow::onSearchTextChanged(const QString& /*text*/) {
    // Restart the debounce timer on every keystroke.
    m_searchDebounce->start();
}

void MainWindow::onSearchCommit() {
    if (!m_service) return;

    const QString query      = m_searchEdit->text().trimmed();
    const QString statusFilter = m_statusFilter->currentData().toString();

    // If there's no text filter, we can use the faster listApplications() path.
    std::vector<Application> apps = query.isEmpty()
        ? m_service->listApplications()
        : m_service->searchApplications(query);

    // Apply status filter client-side (avoids a second DB round-trip).
    if (!statusFilter.isEmpty()) {
        apps.erase(
            std::remove_if(apps.begin(), apps.end(), [&](const Application& a) {
                return QString::fromStdString(a.status) != statusFilter;
            }),
            apps.end());
    }

    populateTable(apps);
    updateToolbarState();
}