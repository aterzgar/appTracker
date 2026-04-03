#include "llm_client.h"

#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

LLMClient::LLMClient(QObject* parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
    connect(m_network, &QNetworkAccessManager::finished,
            this,       &LLMClient::onReplyFinished);
}

LLMClient::~LLMClient() {
    if (m_reply && m_reply->isRunning()) {
        m_reply->abort();
    }
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void LLMClient::setModel(const QString& model) {
    m_model = model;
}

bool LLMClient::isConfigured() const {
    return true;  // Ollama runs locally — always ready
}

// ---------------------------------------------------------------------------
// Core operation
// ---------------------------------------------------------------------------

void LLMClient::generateInsights(const QString& prompt) {
    if (!isConfigured()) {
        emit errorOccurred("LLM client is not configured.");
        return;
    }

    if (m_reply && m_reply->isRunning()) {
        m_reply->abort();
    }

    sendRequest(prompt);
}

// ---------------------------------------------------------------------------
// Internal – send Ollama request
// ---------------------------------------------------------------------------

void LLMClient::sendRequest(const QString& prompt) {
    const QString model = m_model.isEmpty() ? "llama3.2" : m_model;
    QUrl url("http://localhost:11434/api/generate");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject payload;
    payload["model"] = model;
    payload["prompt"] = QString("%1\n\n%2").arg(SYSTEM_INSTRUCTION, prompt);
    payload["stream"] = false;  // single response, no streaming

    QJsonDocument doc(payload);
    m_reply = m_network->post(request, doc.toJson(QJsonDocument::Compact));
}

// ---------------------------------------------------------------------------
// Private slot – reply handler
// ---------------------------------------------------------------------------

void LLMClient::onReplyFinished(QNetworkReply* reply) {
    m_reply = reply;

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        reply->deleteLater();
        m_reply = nullptr;
        return;
    }

    try {
        QString text = parseResponse(reply);

        if (text.isEmpty()) {
            emit errorOccurred("Received empty response from Ollama.");
        } else {
            emit insightsReady(text.trimmed());
        }
    } catch (const std::exception& e) {
        emit errorOccurred(QString("Failed to parse response: %1").arg(e.what()));
    } catch (...) {
        emit errorOccurred("Unexpected error while parsing Ollama response.");
    }

    reply->deleteLater();
    m_reply = nullptr;
}

// ---------------------------------------------------------------------------
// Response parser – Ollama
// ---------------------------------------------------------------------------

QString LLMClient::parseResponse(QNetworkReply* reply) {
    const QByteArray raw = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    if (doc.isNull() || !doc.isObject()) {
        throw std::runtime_error("Invalid JSON from Ollama");
    }

    const QJsonObject obj = doc.object();

    if (obj.contains("error")) {
        throw std::runtime_error(
            obj.value("error").toString().toStdString());
    }

    return obj.value("response").toString();
}
