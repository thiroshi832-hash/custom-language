#pragma once
#include <QObject>
#include <QString>
#include <QByteArray>

class QNetworkAccessManager;
class QNetworkReply;

// ── OllamaClient ──────────────────────────────────────────────────────────
// Talks to a locally-running Ollama instance (http://localhost:11434).
// All traffic is localhost — no internet connection required.
//
// Streaming: tokenReceived() fires for each chunk so the chat UI can
//            display output as it arrives, just like a real chat app.
//
// Model list: call fetchModels() → modelsReady(QStringList) to populate
//             a combo box with whatever the user has installed.

class OllamaClient : public QObject {
    Q_OBJECT
public:
    explicit OllamaClient(QObject *parent = nullptr);
    ~OllamaClient();

    // ── Configuration ─────────────────────────────────────────────────────
    void    setModel  (const QString &model)   { m_model   = model;   }
    void    setBaseUrl(const QString &baseUrl) { m_baseUrl = baseUrl; }
    QString model()   const { return m_model; }
    QString baseUrl() const { return m_baseUrl; }
    bool    isBusy()  const { return m_reply != nullptr; }

    // ── Actions ───────────────────────────────────────────────────────────
    // Send a single-turn chat (system + user message).
    // Streams tokens via tokenReceived(), then emits responseFinished().
    void generate(const QString &systemPrompt, const QString &userMessage);

    // GET /api/tags — lists installed models
    void fetchModels();

    // Abort any in-flight request
    void cancel();

signals:
    void tokenReceived   (const QString &token);    // streaming chunk
    void responseFinished(const QString &fullText); // complete when done
    void modelsReady     (const QStringList &models);
    void errorOccurred   (const QString &msg);
    void busyChanged     (bool busy);

private slots:
    void onReadyRead();
    void onReplyFinished();

private:
    QNetworkAccessManager *m_nam   { nullptr };
    QNetworkReply         *m_reply { nullptr };
    QByteArray             m_buf;          // partial-line accumulation
    QString                m_accumulated; // full streamed response
    QString  m_model   { "codellama" };
    QString  m_baseUrl { "http://localhost:11434" };
};
