#include "ollamaclient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

OllamaClient::OllamaClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{}

OllamaClient::~OllamaClient() { cancel(); }

// ── Cancel ────────────────────────────────────────────────────────────────

void OllamaClient::cancel() {
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
        m_buf.clear();
        m_accumulated.clear();
        emit busyChanged(false);
    }
}

// ── Generate (streaming) ──────────────────────────────────────────────────
// Uses POST /api/generate with stream:true.
// Ollama sends newline-delimited JSON chunks.

void OllamaClient::generate(const QString &systemPrompt,
                             const QString &userMessage)
{
    cancel();

    // Combine system + user into a single prompt string (Modelfile-style)
    QString prompt =
        QString("### System:\n%1\n\n### User:\n%2\n\n### Assistant:\n")
        .arg(systemPrompt, userMessage);

    QJsonObject body;
    body["model"]  = m_model;
    body["prompt"] = prompt;
    body["stream"] = true;

    QUrl url = QUrl(m_baseUrl + "/api/generate");
    QNetworkRequest req = QNetworkRequest(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    m_buf.clear();
    m_accumulated.clear();

    m_reply = m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    emit busyChanged(true);

    connect(m_reply, &QNetworkReply::readyRead,
            this,    &OllamaClient::onReadyRead);
    connect(m_reply, &QNetworkReply::finished,
            this,    &OllamaClient::onReplyFinished);
}

// ── Streaming read ────────────────────────────────────────────────────────

void OllamaClient::onReadyRead() {
    if (!m_reply) return;
    m_buf += m_reply->readAll();

    // Process all complete JSON lines
    while (true) {
        int nl = m_buf.indexOf('\n');
        if (nl < 0) break;

        QByteArray line = m_buf.left(nl).trimmed();
        m_buf = m_buf.mid(nl + 1);
        if (line.isEmpty()) continue;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError) continue;

        QJsonObject obj = doc.object();
        QString token = obj["response"].toString(); // /api/generate field
        if (!token.isEmpty()) {
            m_accumulated += token;
            emit tokenReceived(token);
        }
        // "done":true means stream is over — handled in onReplyFinished
    }
}

// ── Stream finished ───────────────────────────────────────────────────────

void OllamaClient::onReplyFinished() {
    if (!m_reply) return;

    QNetworkReply::NetworkError netErr = m_reply->error();
    m_reply->deleteLater();
    m_reply = nullptr;
    emit busyChanged(false);

    if (netErr != QNetworkReply::NoError &&
        netErr != QNetworkReply::OperationCanceledError)
    {
        QString hint;
        if (netErr == QNetworkReply::ConnectionRefusedError)
            hint = "\n\nIs Ollama running?  Run:  ollama serve";
        emit errorOccurred(
            QString("Ollama error %1%2").arg((int)netErr).arg(hint));
        return;
    }

    emit responseFinished(m_accumulated);
    m_accumulated.clear();
    m_buf.clear();
}

// ── Fetch installed models ────────────────────────────────────────────────

void OllamaClient::fetchModels() {
    QUrl url = QUrl(m_baseUrl + "/api/tags");
    QNetworkRequest req = QNetworkRequest(url);

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred("Cannot reach Ollama at " + m_baseUrl +
                               "\nRun:  ollama serve");
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QStringList names;
        for (const QJsonValue &v : doc.object()["models"].toArray())
            names << v.toObject()["name"].toString();
        if (names.isEmpty()) names << "(no models installed — run: ollama pull codellama)";
        emit modelsReady(names);
    });
}
