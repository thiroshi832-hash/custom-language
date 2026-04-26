#pragma once
#include <QObject>
#include <QString>
#include <QByteArray>

class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;

// ── CopilotClient ─────────────────────────────────────────────────────────
// Talks to the Anthropic Messages API (api.anthropic.com).
// No other program needs to be installed — just an API key.
//
// Two modes:
//  1. requestCompletion()  – inline ghost-text (non-streaming)
//  2. generateCode()       – chat panel code generation (streaming SSE)
//     Streams tokens via codeTokenReady() for a live "typing" effect,
//     then emits codeGenerationDone() with the full text.

class CopilotClient : public QObject {
    Q_OBJECT
public:
    explicit CopilotClient(QObject *parent = nullptr);
    ~CopilotClient();

    void    setApiKey(const QString &key);
    QString apiKey()      const { return m_apiKey; }
    bool    hasApiKey()   const { return !m_apiKey.isEmpty(); }

    bool    isBusy()      const { return m_reply    != nullptr; }
    bool    isGenerating()const { return m_genReply != nullptr; }

    // ── Inline completion (non-streaming) ─────────────────────────────────
    void requestCompletion(const QString &prefix, const QString &suffix);
    void cancelCompletion();
    void cancel(); // alias for cancelCompletion() — backward compat

    // ── Chat-panel code generation (streaming SSE) ────────────────────────
    // description : natural-language request from the user
    void generateCode(const QString &description);
    void cancelGenerate();

signals:
    // — completion —
    void completionReady (const QString &text);
    void requestFailed   (const QString &msg);
    void busyChanged     (bool busy);

    // — generation (streaming) —
    void codeTokenReady      (const QString &token);   // each streamed chunk
    void codeGenerationDone  (const QString &fullText);// finished
    void codeGenerationFailed(const QString &msg);
    void generatingChanged   (bool generating);

private slots:
    void onReplyFinished();    // completion
    void onGenReadyRead();     // streaming data ready
    void onGenReplyFinished(); // streaming done

private:
    QNetworkRequest buildRequest() const;

    QNetworkAccessManager *m_nam      { nullptr };
    QNetworkReply         *m_reply    { nullptr }; // completion reply
    QNetworkReply         *m_genReply { nullptr }; // streaming reply
    QByteArray             m_genBuf;               // partial SSE line buffer
    QString                m_genAccum;             // accumulated full response
    QString                m_apiKey;
};
