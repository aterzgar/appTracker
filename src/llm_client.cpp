#include "llm_client.h"

#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QTimer>

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
    // Use a temporary QNetworkAccessManager so the /api/tags check
    // doesn't trigger onReplyFinished (which is connected to m_network).
    QNetworkAccessManager checker;
    QNetworkReply* reply = checker.get(
        QNetworkRequest(QUrl("http://localhost:11434/api/tags")));

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished,
                     &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout,
                     reply, &QNetworkReply::abort);
    timer.start(2000);
    loop.exec();
    // loop.exec() blocks until the reply finishes or times out, so
    // by the time we reach here the reply is already done and
    // 'checker' can be safely destroyed.

    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return false;
    }

    // Parse /api/tags and verify the requested model is available.
    const QString desired = m_model.isEmpty() ? "llama3.2" : m_model;
    bool found = false;

    try {
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isObject() && doc.object().contains("models")) {
            for (const QJsonValue& v : doc.object()["models"].toArray()) {
                QString name = v.toObject().value("name").toString();
                if (name.endsWith(":latest")) name.chop(7);
                QString wanted = desired;
                if (wanted.endsWith(":latest")) wanted.chop(7);
                if (name == wanted) { found = true; break; }
            }
        }
    } catch (...) {
        found = false;
    }

    reply->deleteLater();
    return found;
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
    payload["stream"] = false;

    QJsonDocument doc(payload);
    m_reply = m_network->post(request, doc.toJson(QJsonDocument::Compact));
}

// ---------------------------------------------------------------------------
// Private slot – reply handler
// ---------------------------------------------------------------------------

void LLMClient::onReplyFinished(QNetworkReply* reply) {
    // Ignore /api/tags responses — those are handled by isConfigured()
    // using its own temporary manager.
    if (reply->url().path() == "/api/tags") {
        reply->deleteLater();
        return;
    }

    // /api/generate response
    m_reply = reply;

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        reply->deleteLater();
        m_reply = nullptr;
        return;
    }

    const QByteArray raw = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    if (doc.isNull() || !doc.isObject()) {
        emit errorOccurred("Invalid JSON from Ollama.");
        reply->deleteLater();
        m_reply = nullptr;
        return;
    }

    const QJsonObject obj = doc.object();

    if (obj.contains("error")) {
        emit errorOccurred(QString("Ollama error: %1")
            .arg(obj.value("error").toString()));
        reply->deleteLater();
        m_reply = nullptr;
        return;
    }

    QString text = obj.value("response").toString();
    if (text.isEmpty()) {
        emit errorOccurred("Received empty response from Ollama. "
            "Model may still be loading — try again in a moment.");
    } else {
        emit insightsReady(text.trimmed());
    }

    reply->deleteLater();
    m_reply = nullptr;
}
