#include "copilotclient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

// ── API constants ─────────────────────────────────────────────────────────

static const char API_URL[]     = "https://api.anthropic.com/v1/messages";
static const char API_VERSION[] = "2023-06-01";
static const char MODEL[]       = "claude-3-5-haiku-20241022";

// ── System prompts ────────────────────────────────────────────────────────

static const char COMPLETION_SYSTEM[] =
    "You are an inline code completion engine for CustomLanguage, a VBScript-like language.\n"
    "Rules: return ONLY the completion text — no explanations, no markdown fences.\n"
    "Fit naturally between <prefix> and <suffix>. Match indentation. Keep it concise.\n"
    "Language: Dim, If/Then/Else/End If, For/To/Step/Next, While/Wend, "
    "Do/Loop/Until, Sub/End Sub, Function/Return/End Function, "
    "Print, Call, Exit Sub/Function, &-concat, 'comment.\n"
    "Built-ins: Len Left Right Mid UCase LCase Trim InStr Replace Abs Sqr CStr CInt CDbl "
    "IsNumeric CreateControl SetProperty GetProperty AddItem ShowForm.";

static const char GENERATE_SYSTEM[] =
    "You are a code generator for CustomLanguage, a VBScript-like scripting language "
    "embedded in a custom Qt IDE.\n\n"

    "Language reference:\n"
    "  Dim x                      — variant variable\n"
    "  x = value                  — assignment\n"
    "  \"hello\" & name            — string concatenation (&)\n"
    "  If cond Then...ElseIf...Else...End If\n"
    "  For i = 1 To 10 Step 1 ... Next\n"
    "  While cond ... Wend\n"
    "  Do ... Loop Until cond\n"
    "  Sub Name(a, b) ... End Sub\n"
    "  Function Name(a) ... Return val ... End Function\n"
    "  Call SubName(args)   or   SubName args\n"
    "  Print expression\n"
    "  Exit Sub / Exit Function\n\n"

    "Built-in functions:\n"
    "  String  – Len Left Right Mid UCase LCase Trim LTrim RTrim InStr Replace Space StrReverse\n"
    "  Math    – Abs Int Fix Sgn Sqr Log Exp Sin Cos Tan Atn Rnd Randomize\n"
    "  Convert – CStr CInt CLng CDbl CBool Val Str\n"
    "  Check   – IsNumeric IsNull IsEmpty TypeName\n\n"

    "UI form built-ins (for interactive form-based applications):\n"
    "  CreateControl(\"Type\", \"name\", x, y, width, height)  → integer ID\n"
    "  SetProperty widgetId, \"property\", \"value\"\n"
    "  GetProperty(widgetId, \"property\")  → string value\n"
    "  AddItem widgetId, \"item text\"\n"
    "  ShowForm \"Window Title\", width, height   ← blocks until window closed\n"
    "  Event handlers: Sub WidgetName_Click() ... End Sub\n"
    "                  Sub WidgetName_Changed() ... End Sub\n\n"

    "Supported control types: Label, PushButton, CheckBox, RadioButton,\n"
    "  LineEdit, TextEdit, SpinBox, ComboBox, Slider, ProgressBar, ListWidget\n\n"

    "Comments: ' single-quote to end of line\n"
    "Booleans: True, False     Null literal: Null\n\n"

    "== Output rules ==\n"
    "1. Wrap ALL generated code in ```...``` fences.\n"
    "2. Keep a brief explanation outside the fences (1-2 sentences is fine).\n"
    "3. Use 4-space indentation consistently.\n"
    "4. For form apps: Dim all widget IDs at module level (before any code),\n"
    "   call CreateControl/SetProperty before ShowForm,\n"
    "   place Sub event handlers AFTER the ShowForm line.\n"
    "5. Always close every block: End If, End Sub, End Function, Next, Wend.\n"
    "6. Use meaningful variable and widget names.";

// ── CopilotClient ─────────────────────────────────────────────────────────

CopilotClient::CopilotClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{}

CopilotClient::~CopilotClient() {
    cancelCompletion();
    cancelGenerate();
}

void CopilotClient::setApiKey(const QString &key) {
    m_apiKey = key.trimmed();
}

// ── Shared: build the network request with auth headers ───────────────────

QNetworkRequest CopilotClient::buildRequest() const {
    QUrl url = QUrl(QLatin1String(API_URL));
    QNetworkRequest req = QNetworkRequest(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("x-api-key",         m_apiKey.toUtf8());
    req.setRawHeader("anthropic-version", API_VERSION);
    return req;
}

// ── Inline completion (non-streaming) ────────────────────────────────────

void CopilotClient::cancelCompletion() {
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
        emit busyChanged(false);
    }
}

// Keep old name for compatibility with any existing connections
void CopilotClient::cancel() { cancelCompletion(); }

void CopilotClient::requestCompletion(const QString &prefix,
                                       const QString &suffix) {
    if (m_apiKey.isEmpty()) {
        emit requestFailed("No API key — go to Tools > Configure Copilot…");
        return;
    }
    cancelCompletion();

    QString userContent =
        QString("<prefix>%1</prefix>\n<suffix>%2</suffix>\n\n"
                "Write the completion that goes immediately after the prefix.")
        .arg(prefix.right(2000))
        .arg(suffix.left(500));

    QJsonObject msg;
    msg["role"]    = "user";
    msg["content"] = userContent;

    QJsonObject body;
    body["model"]      = QLatin1String(MODEL);
    body["max_tokens"] = 200;
    body["stream"]     = false;
    body["system"]     = QLatin1String(COMPLETION_SYSTEM);
    body["messages"]   = QJsonArray{ msg };

    m_reply = m_nam->post(buildRequest(),
                          QJsonDocument(body).toJson(QJsonDocument::Compact));
    emit busyChanged(true);
    connect(m_reply, &QNetworkReply::finished,
            this,    &CopilotClient::onReplyFinished);
}

void CopilotClient::onReplyFinished() {
    if (!m_reply) return;
    QByteArray data = m_reply->readAll();
    QNetworkReply::NetworkError err = m_reply->error();
    m_reply->deleteLater();
    m_reply = nullptr;
    emit busyChanged(false);

    if (err != QNetworkReply::NoError) {
        if (err != QNetworkReply::OperationCanceledError)
            emit requestFailed(QString("Network error %1").arg((int)err));
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();
    if (root.contains("error")) {
        emit requestFailed("API: " + root["error"].toObject()["message"].toString());
        return;
    }
    for (const QJsonValue &v : root["content"].toArray()) {
        QJsonObject block = v.toObject();
        if (block["type"].toString() == "text") {
            QString text = block["text"].toString().trimmed();
            if (!text.isEmpty()) emit completionReady(text);
            return;
        }
    }
}

// ── Chat-panel code generation (streaming SSE) ───────────────────────────

void CopilotClient::cancelGenerate() {
    if (m_genReply) {
        m_genReply->abort();
        m_genReply->deleteLater();
        m_genReply = nullptr;
        m_genBuf.clear();
        m_genAccum.clear();
        emit generatingChanged(false);
    }
}

void CopilotClient::generateCode(const QString &description) {
    if (m_apiKey.isEmpty()) {
        emit codeGenerationFailed(
            "No API key configured.\n\n"
            "Go to  Tools → Configure Copilot API Key…\n"
            "and enter your Anthropic key (sk-ant-…).\n\n"
            "Get one free at https://console.anthropic.com/");
        return;
    }
    cancelGenerate();

    QJsonObject msg;
    msg["role"]    = "user";
    msg["content"] = description;

    QJsonObject body;
    body["model"]      = QLatin1String(MODEL);
    body["max_tokens"] = 1024;
    body["stream"]     = true;  // ← streaming SSE
    body["system"]     = QLatin1String(GENERATE_SYSTEM);
    body["messages"]   = QJsonArray{ msg };

    m_genBuf.clear();
    m_genAccum.clear();

    m_genReply = m_nam->post(buildRequest(),
                             QJsonDocument(body).toJson(QJsonDocument::Compact));
    emit generatingChanged(true);

    connect(m_genReply, &QNetworkReply::readyRead,
            this,        &CopilotClient::onGenReadyRead);
    connect(m_genReply, &QNetworkReply::finished,
            this,        &CopilotClient::onGenReplyFinished);
}

// ── SSE streaming parser ──────────────────────────────────────────────────
// Anthropic SSE format:
//   event: content_block_delta
//   data: {"type":"content_block_delta","delta":{"type":"text_delta","text":"..."}}
//
// We only care about lines that start with "data: ".

void CopilotClient::onGenReadyRead() {
    if (!m_genReply) return;
    m_genBuf += m_genReply->readAll();

    while (true) {
        int nl = m_genBuf.indexOf('\n');
        if (nl < 0) break;

        QByteArray line = m_genBuf.left(nl).trimmed();
        m_genBuf = m_genBuf.mid(nl + 1);

        if (!line.startsWith("data: ")) continue;
        QByteArray payload = line.mid(6); // strip "data: "
        if (payload.isEmpty() || payload == "[DONE]") continue;

        QJsonParseError jsonErr;
        QJsonDocument doc = QJsonDocument::fromJson(payload, &jsonErr);
        if (jsonErr.error != QJsonParseError::NoError) continue;

        QJsonObject obj = doc.object();
        if (obj["type"].toString() == "content_block_delta") {
            QJsonObject delta = obj["delta"].toObject();
            if (delta["type"].toString() == "text_delta") {
                QString token = delta["text"].toString();
                if (!token.isEmpty()) {
                    m_genAccum += token;
                    emit codeTokenReady(token);
                }
            }
        }
        // "message_stop" → handled in onGenReplyFinished
    }
}

void CopilotClient::onGenReplyFinished() {
    if (!m_genReply) return;

    QNetworkReply::NetworkError err = m_genReply->error();
    // Read any remaining buffered data
    m_genBuf += m_genReply->readAll();
    m_genReply->deleteLater();
    m_genReply = nullptr;
    emit generatingChanged(false);

    if (err != QNetworkReply::NoError &&
        err != QNetworkReply::OperationCanceledError)
    {
        // Try to parse an error body if present
        QJsonDocument doc = QJsonDocument::fromJson(m_genBuf);
        QString apiMsg = doc.object()["error"].toObject()["message"].toString();
        QString msg = apiMsg.isEmpty()
            ? QString("Network error %1").arg((int)err)
            : "API: " + apiMsg;
        emit codeGenerationFailed(msg);
        m_genBuf.clear();
        m_genAccum.clear();
        return;
    }

    // Process any leftover lines in the buffer
    while (true) {
        int nl = m_genBuf.indexOf('\n');
        if (nl < 0) break;
        QByteArray line = m_genBuf.left(nl).trimmed();
        m_genBuf = m_genBuf.mid(nl + 1);
        if (!line.startsWith("data: ")) continue;
        QByteArray payload = line.mid(6);
        if (payload.isEmpty()) continue;
        QJsonDocument doc = QJsonDocument::fromJson(payload);
        QJsonObject obj = doc.object();
        if (obj["type"].toString() == "content_block_delta") {
            QString token = obj["delta"].toObject()["text"].toString();
            if (!token.isEmpty()) {
                m_genAccum += token;
                emit codeTokenReady(token);
            }
        }
    }

    QString full = m_genAccum;
    m_genAccum.clear();
    m_genBuf.clear();
    emit codeGenerationDone(full);
}
