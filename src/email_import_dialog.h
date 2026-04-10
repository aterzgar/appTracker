#ifndef EMAIL_IMPORT_DIALOG_H
#define EMAIL_IMPORT_DIALOG_H

#include <QDialog>
#include <QString>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>

#include "db_manager.h"
#include "llm_client.h"

// ---------------------------------------------------------------------------
// EmailImportDialog – paste email content, extract structured data via LLM
// ---------------------------------------------------------------------------
class EmailImportDialog : public QDialog {
    Q_OBJECT
public:
    explicit EmailImportDialog(LLMClient* llmClient, QWidget* parent = nullptr);
    ~EmailImportDialog() override;

    // Returns the parsed application data after successful extraction
    Application getApplication() const { return m_parsedApp; }

    // Returns true if the LLM found valid application data in the email
    bool hasValidData() const { return m_isValid; }

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void onParseEmail();
    void onParsingReady(const QString& response);
    void onParsingError(const QString& error);
    void onConfirm();

private:
    void setupUI();
    bool parseLLMResponse(const QString& response);
    void showPreview(const Application& app);
    void resetForm();

    LLMClient* m_llmClient = nullptr;

    // Input section
    QTextEdit*   m_emailInput      = nullptr;
    QPushButton* m_parseBtn        = nullptr;
    QLabel*      m_statusLabel     = nullptr;

    // Preview section (initially hidden)
    QGroupBox*   m_previewGroup    = nullptr;
    QLineEdit*   m_companyEdit     = nullptr;
    QDateEdit*   m_dateEdit        = nullptr;
    QLineEdit*   m_positionEdit    = nullptr;
    QLineEdit*   m_contactEdit     = nullptr;
    QComboBox*   m_statusCombo     = nullptr;
    QTextEdit*   m_notesEdit       = nullptr;
    QPushButton* m_confirmBtn      = nullptr;
    QPushButton* m_cancelPreviewBtn = nullptr;

    // Parsed result
    Application m_parsedApp;
    bool        m_isValid = false;
};

#endif // EMAIL_IMPORT_DIALOG_H
