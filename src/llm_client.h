#ifndef LLM_CLIENT_H
#define LLM_CLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QString>

// ---------------------------------------------------------------------------
// LLMClient – thin async wrapper around Ollama local API
//
// Responsibilities:
//   • Build Ollama HTTP requests
//   • Send the prompt and parse the JSON response
//   • Emit the extracted text (or an error string)
//
// NOT responsible for:
//   • Prompt engineering / content construction (caller's job)
//   • Caching or retry logic (Phase 2)
// ---------------------------------------------------------------------------
class LLMClient : public QObject {
    Q_OBJECT
public:
    explicit LLMClient(QObject* parent = nullptr);
    ~LLMClient() override;

    // --- Configuration ---
    void setModel(const QString& model);
    QString model() const { return m_model; }

    // Returns true when the client is ready.
    bool isConfigured() const;

    // --- Core operation ---
    // Sends `prompt` to Ollama. On completion emits
    // `insightsReady` (text) or `errorOccurred` (reason).
    void generateInsights(const QString& prompt);

signals:
    void insightsReady(const QString& insights);
    void errorOccurred(const QString& error);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    void sendRequest(const QString& prompt);
    QString parseResponse(QNetworkReply* reply);

    QNetworkAccessManager* m_network = nullptr;
    QNetworkReply*         m_reply   = nullptr;

    QString m_model;

    // System-level instruction prepended to every prompt
    static constexpr const char* SYSTEM_INSTRUCTION =
        "You are a job-application analytics assistant. "
        "Analyze the provided metrics and return concise, actionable insights "
        "for a job seeker. Use short bullet points. Be specific and direct. "
        "Do NOT include generic advice like 'keep networking' unless the data supports it.";
};

#endif // LLM_CLIENT_H
