#include "email_import_dialog.h"

#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDate>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

EmailImportDialog::EmailImportDialog(LLMClient* llmClient, QWidget* parent)
    : QDialog(parent)
    , m_llmClient(llmClient)
{
    setWindowTitle("Import Application from Email");
    resize(700, 750);
    setupUI();

    // Wire LLM signals
    connect(m_llmClient, &LLMClient::insightsReady, this, &EmailImportDialog::onParsingReady);
    connect(m_llmClient, &LLMClient::errorOccurred, this, &EmailImportDialog::onParsingError);
}

EmailImportDialog::~EmailImportDialog() = default;

void EmailImportDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    m_emailInput->setFocus();
}

// ---------------------------------------------------------------------------
// UI Setup
// ---------------------------------------------------------------------------

void EmailImportDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // --- Email input section ---
    auto* inputGroup = new QGroupBox("📧 Paste Email Content", this);
    auto* inputLayout = new QVBoxLayout(inputGroup);

    m_emailInput = new QTextEdit(this);
    m_emailInput->setPlaceholderText(
        "Paste the full email content here...\n\n"
        "This can be:\n"
        "  • Application confirmation email\n"
        "  • Interview invitation\n"
        "  • Rejection notice\n"
        "  • Any email related to your job application\n\n"
        "The AI will extract company name, position, status, and date automatically."
    );
    m_emailInput->setMinimumHeight(200);
    m_emailInput->setStyleSheet(
        "QTextEdit { background-color: #1e1e2e; color: #cdd6f4; "
        "font-family: monospace; font-size: 12px; border: 1px solid #45475a; "
        "border-radius: 4px; padding: 8px; }"
    );

    auto* btnLayout = new QHBoxLayout();
    m_parseBtn = new QPushButton("🔍 Extract Application Data", this);
    m_parseBtn->setToolTip("Use AI to parse the email and extract application details");
    m_statusLabel = new QLabel("Paste an email above, then click Extract", this);
    m_statusLabel->setStyleSheet("color: #7f8c8d; font-style: italic; font-size: 12px;");

    btnLayout->addWidget(m_parseBtn);
    btnLayout->addStretch();
    inputLayout->addWidget(m_emailInput);
    inputLayout->addLayout(btnLayout);
    inputLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(inputGroup);

    // --- Preview section (hidden until parsing succeeds) ---
    m_previewGroup = new QGroupBox("✏️ Review Extracted Data", this);
    m_previewGroup->setVisible(false);

    auto* previewLayout = new QFormLayout(m_previewGroup);
    previewLayout->setSpacing(10);

    m_companyEdit = new QLineEdit(this);
    m_companyEdit->setPlaceholderText("e.g. Acme Corp");

    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("yyyy-MM-dd");

    m_positionEdit = new QLineEdit(this);
    m_positionEdit->setPlaceholderText("e.g. Senior Engineer");

    m_contactEdit = new QLineEdit(this);
    m_contactEdit->setPlaceholderText("e.g. Jane Smith");

    static const QStringList kStatusOptions = {
        "Applied", "Interviews", "Offer Received", "Rejected", "Withdrawn"
    };
    m_statusCombo = new QComboBox(this);
    m_statusCombo->addItems(kStatusOptions);

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setPlaceholderText("Additional notes from the email (optional)");
    m_notesEdit->setMaximumHeight(80);

    previewLayout->addRow("Company Name *", m_companyEdit);
    previewLayout->addRow("Application Date", m_dateEdit);
    previewLayout->addRow("Position *", m_positionEdit);
    previewLayout->addRow("Contact Person *", m_contactEdit);
    previewLayout->addRow("Status", m_statusCombo);
    previewLayout->addRow("Notes", m_notesEdit);

    auto* confirmBtnLayout = new QHBoxLayout();
    m_confirmBtn = new QPushButton("✅ Add/Update Application", this);
    m_cancelPreviewBtn = new QPushButton("Cancel", this);
    confirmBtnLayout->addStretch();
    confirmBtnLayout->addWidget(m_cancelPreviewBtn);
    confirmBtnLayout->addWidget(m_confirmBtn);
    previewLayout->addRow(confirmBtnLayout);

    mainLayout->addWidget(m_previewGroup);
    mainLayout->addStretch();

    // --- Signal connections ---
    connect(m_parseBtn, &QPushButton::clicked, this, &EmailImportDialog::onParseEmail);
    connect(m_confirmBtn, &QPushButton::clicked, this, &EmailImportDialog::onConfirm);
    connect(m_cancelPreviewBtn, &QPushButton::clicked, this, &QDialog::reject);
}

// ---------------------------------------------------------------------------
// Parse email via LLM
// ---------------------------------------------------------------------------

void EmailImportDialog::onParseEmail() {
    const QString emailText = m_emailInput->toPlainText().trimmed();
    if (emailText.isEmpty()) {
        QMessageBox::warning(this, "No Email Content",
            "Please paste the email content before extracting.");
        return;
    }

    if (!m_llmClient || !m_llmClient->isConfigured()) {
        QMessageBox::critical(this, "LLM Unavailable",
            "The AI service is not configured. Make sure Ollama is running locally.");
        return;
    }

    // Build the extraction prompt
    const QString prompt = QString(
        "Extract job application details from the following email and return "
        "ONLY a valid JSON object (no markdown, no explanation, no code blocks). "
        "The JSON must have these exact keys:\n"
        "  \"company\" (string, required)\n"
        "  \"position\" (string, required)\n"
        "  \"contact\" (string, the recruiter/hiring manager name if mentioned; use \"Unknown\" if not found)\n"
        "  \"status\" (one of: \"Applied\", \"Interviews\", \"Offer Received\", \"Rejected\", \"Withdrawn\")\n"
        "  \"date\" (string in YYYY-MM-DD format; use today's date if not mentioned)\n"
        "  \"notes\" (string, brief summary of the email; empty string if nothing notable)\n\n"
        "Rules:\n"
        "  1. If the email mentions an interview invitation, set status to \"Interviews\".\n"
        "  2. If the email mentions a rejection, set status to \"Rejected\".\n"
        "  3. If the email mentions an offer, set status to \"Offer Received\".\n"
        "  4. If it's just a confirmation of application submission, set status to \"Applied\".\n"
        "  5. Infer the company name from the sender or email signature.\n"
        "  6. If a specific date is mentioned (e.g., interview date), use that. "
        "Otherwise use the application date or today's date.\n"
        "  7. Return ONLY the JSON object, nothing else.\n\n"
        "EMAIL CONTENT:\n%1").arg(emailText);

    m_parseBtn->setEnabled(false);
    m_parseBtn->setText("⏳ Parsing...");
    m_statusLabel->setText("Analyzing email with AI...");
    m_statusLabel->setStyleSheet("color: #f39c12; font-style: italic;");

    m_previewGroup->setVisible(false);
    m_isValid = false;

    m_llmClient->generateInsights(prompt);
}

void EmailImportDialog::onParsingReady(const QString& response) {
    m_parseBtn->setEnabled(true);
    m_parseBtn->setText("🔍 Extract Application Data");

    if (!parseLLMResponse(response)) {
        m_statusLabel->setText("Failed to parse email — check the response and try again");
        m_statusLabel->setStyleSheet("color: #e74c3c; font-style: italic;");
        return;
    }

    showPreview(m_parsedApp);
    m_statusLabel->setText("✅ Data extracted successfully — review and confirm below");
    m_statusLabel->setStyleSheet("color: #27ae60; font-style: italic;");
}

void EmailImportDialog::onParsingError(const QString& error) {
    m_parseBtn->setEnabled(true);
    m_parseBtn->setText("🔍 Extract Application Data");
    m_statusLabel->setText("❌ Parsing failed");
    m_statusLabel->setStyleSheet("color: #e74c3c; font-style: italic;");

    QString msg = error;
    if (msg.contains("connection", Qt::CaseInsensitive) ||
        msg.contains("refused", Qt::CaseInsensitive)) {
        msg = "Cannot connect to Ollama.\n\n"
              "Make sure Ollama is running locally:\n"
              "  Install: curl -fsSL https://ollama.com/install.sh | sh\n"
              "  Run:     ollama run llama3.2";
    }
    QMessageBox::critical(this, "Parsing Error", msg);
}

// ---------------------------------------------------------------------------
// JSON parser – extract Application from LLM response
// ---------------------------------------------------------------------------

bool EmailImportDialog::parseLLMResponse(const QString& response) {
    // Try to find JSON object in the response (strip markdown if present)
    QString jsonStr = response.trimmed();

    // Remove markdown code blocks if present
    const int jsonStart = jsonStr.indexOf("{");
    const int jsonEnd = jsonStr.lastIndexOf("}");
    if (jsonStart >= 0 && jsonEnd > jsonStart) {
        jsonStr = jsonStr.mid(jsonStart, jsonEnd - jsonStart + 1);
    }

    const QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::warning(this, "Invalid Response",
            "The AI returned invalid JSON. Please try again or enter data manually.\n\n"
            "Raw response:\n" + response.left(500));
        return false;
    }

    const QJsonObject obj = doc.object();

    // Validate required fields
    const QString company = obj.value("company").toString("").trimmed();
    const QString position = obj.value("position").toString("").trimmed();
    const QString contact = obj.value("contact").toString("Unknown").trimmed();

    if (company.isEmpty() || position.isEmpty()) {
        QMessageBox::warning(this, "Missing Data",
            "The AI could not identify the company name or position. "
            "Please try again with more detailed email content.");
        return false;
    }

    // Parse status
    QString status = obj.value("status").toString("Applied").trimmed();
    static const QStringList kValidStatuses = {
        "Applied", "Interviews", "Offer Received", "Rejected", "Withdrawn"
    };
    if (!kValidStatuses.contains(status)) {
        status = "Applied"; // fallback
    }

    // Parse date
    QString dateStr = obj.value("date").toString("").trimmed();
    QDate parsedDate;
    if (!dateStr.isEmpty()) {
        parsedDate = QDate::fromString(dateStr, "yyyy-MM-dd");
    }
    if (!parsedDate.isValid()) {
        parsedDate = QDate::currentDate();
    }
    // Prevent future dates
    if (parsedDate > QDate::currentDate()) {
        parsedDate = QDate::currentDate();
    }

    const QString notes = obj.value("notes").toString("").trimmed();

    m_parsedApp.companyName     = company.toStdString();
    m_parsedApp.applicationDate = parsedDate.toString("yyyy-MM-dd").toStdString();
    m_parsedApp.position        = position.toStdString();
    m_parsedApp.contactPerson   = contact.toStdString();
    m_parsedApp.status          = status.toStdString();
    m_parsedApp.notes           = notes.toStdString();
    m_isValid = true;

    return true;
}

// ---------------------------------------------------------------------------
// Show preview form
// ---------------------------------------------------------------------------

void EmailImportDialog::showPreview(const Application& app) {
    m_companyEdit->setText(QString::fromStdString(app.companyName));
    m_dateEdit->setDate(QDate::fromString(QString::fromStdString(app.applicationDate), "yyyy-MM-dd"));
    m_positionEdit->setText(QString::fromStdString(app.position));
    m_contactEdit->setText(QString::fromStdString(app.contactPerson));
    m_statusCombo->setCurrentText(QString::fromStdString(app.status));
    m_notesEdit->setText(QString::fromStdString(app.notes));

    m_previewGroup->setVisible(true);
    m_previewGroup->setChecked(true);
}

// ---------------------------------------------------------------------------
// Confirm – validate and return result
// ---------------------------------------------------------------------------

void EmailImportDialog::onConfirm() {
    const QString company = m_companyEdit->text().trimmed();
    const QString position = m_positionEdit->text().trimmed();
    const QString contact = m_contactEdit->text().trimmed();

    if (company.isEmpty() || position.isEmpty() || contact.isEmpty()) {
        QMessageBox::warning(this, "Validation Error",
            "Please fill in all required fields: Company, Position, Contact Person.");
        return;
    }

    m_parsedApp.companyName     = company.toStdString();
    m_parsedApp.applicationDate = m_dateEdit->date().toString("yyyy-MM-dd").toStdString();
    m_parsedApp.position        = position.toStdString();
    m_parsedApp.contactPerson   = contact.toStdString();
    m_parsedApp.status          = m_statusCombo->currentText().toStdString();
    m_parsedApp.notes           = m_notesEdit->toPlainText().toStdString();
    m_isValid = true;

    accept();
}

// ---------------------------------------------------------------------------
// Reset form (if needed for reuse)
// ---------------------------------------------------------------------------

void EmailImportDialog::resetForm() {
    m_emailInput->clear();
    m_previewGroup->setVisible(false);
    m_statusLabel->setText("Paste an email above, then click Extract");
    m_statusLabel->setStyleSheet("color: #7f8c8d; font-style: italic;");
    m_isValid = false;
    m_parsedApp = {};
}
