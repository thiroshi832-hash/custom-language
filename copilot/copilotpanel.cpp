#include "copilotpanel.h"
#include "localcopilot.h"
#include "codeeditor.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QKeyEvent>
#include <QScrollBar>
#include <QTextCursor>
#include <QFont>
#include <QColor>

// ── Colour palette (Qt Creator dark) ────────────────────────────────────
namespace CP {
    static const char BG[]      = "#2b2b2b";
    static const char PANEL[]   = "#3c3f41";
    static const char BORDER[]  = "#323232";
    static const char TEXT[]    = "#a9b7c6";
    static const char DIM[]     = "#606366";
    static const char ACCENT[]  = "#3592c4";
    static const char CONSOLE[] = "#1e1e1e";
}

// ── CopilotPanel ─────────────────────────────────────────────────────────

CopilotPanel::CopilotPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void CopilotPanel::setEditor(CodeEditor *editor) {
    m_editor = editor;
}

void CopilotPanel::setLocalCopilot(LocalCopilot *copilot) {
    if (m_localCopilot) {
        disconnect(m_localCopilot, nullptr, this, nullptr);
    }
    m_localCopilot = copilot;
    if (!m_localCopilot) return;

    connect(m_localCopilot, &LocalCopilot::codeReady,
            this, &CopilotPanel::onCodeReady);
    connect(m_localCopilot, &LocalCopilot::noMatch,
            this, &CopilotPanel::onNoMatch);
}

// ── UI setup ─────────────────────────────────────────────────────────────

void CopilotPanel::setupUI() {
    auto *mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(0, 0, 0, 0);
    mainLay->setSpacing(0);

    setStyleSheet(
        QString("QWidget    { background:%1; color:%2; }"
                "QTextEdit  { background:%3; color:%2; border:none; }"
                "QPushButton{ background:%4; color:%2; border:none;"
                "  padding:5px 10px; border-radius:3px; }"
                "QPushButton:hover    { background:#4e5254; }"
                "QPushButton:disabled { color:%5; }"
                "QLabel { background:transparent; color:%2; }")
        .arg(CP::BG).arg(CP::TEXT).arg(CP::CONSOLE)
        .arg(CP::BORDER).arg(CP::DIM));

    // ── Top bar ───────────────────────────────────────────────────────────
    auto *topBar = new QWidget;
    topBar->setFixedHeight(28);
    topBar->setStyleSheet(
        QString("QWidget{ background:%1; border-bottom:1px solid %2; }")
        .arg(CP::PANEL).arg(CP::BORDER));
    auto *topLay = new QHBoxLayout(topBar);
    topLay->setContentsMargins(8, 0, 8, 0);
    topLay->setSpacing(6);

    auto *titleLbl = new QLabel("⚡ Copilot  —  offline, zero dependencies");
    titleLbl->setStyleSheet(
        QString("color:%1; font-size:8pt; font-style:italic;").arg(CP::DIM));
    topLay->addWidget(titleLbl);
    topLay->addStretch();

    mainLay->addWidget(topBar);

    // ── Conversation history ──────────────────────────────────────────────
    m_history = new QTextEdit;
    m_history->setReadOnly(true);
    m_history->setFont(QFont("Consolas", 9));
    m_history->setStyleSheet(
        QString("QTextEdit { background:%1; color:%2; padding:6px; }")
        .arg(CP::CONSOLE).arg(CP::TEXT));

    // Welcome message
    m_history->setHtml(
        "<body style='font-family:Consolas,monospace;font-size:9pt;"
        "color:#a9b7c6;background:#1e1e1e;'>"
        "<div style='background:#1d3557;padding:8px 10px;border-radius:4px;"
        "margin-bottom:8px;'>"
        "<b style='color:#89b4fa'>Copilot</b>"
        " <span style='color:#606366;font-size:8pt;'>(offline)</span><br><br>"
        "Describe what program you want and I'll pick the best matching "
        "CustomLanguage template for you — instantly, no network needed.<br><br>"
        "<span style='color:#606366'>Try asking for:</span>"
        "<ul style='margin:4px 0 0 0;color:#a9b7c6;'>"
        "<li>counter form with + and &minus; buttons</li>"
        "<li>calculator with add subtract multiply divide</li>"
        "<li>temperature converter Celsius Fahrenheit</li>"
        "<li>fibonacci sequence</li>"
        "<li>BMI calculator</li>"
        "<li>multiplication table</li>"
        "<li>guessing game</li>"
        "<li>todo list form</li>"
        "</ul></div></body>");

    mainLay->addWidget(m_history, 1);

    // ── Input area ────────────────────────────────────────────────────────
    auto *inputFrame = new QWidget;
    inputFrame->setStyleSheet(
        QString("QWidget{ background:%1; border-top:1px solid %2; }")
        .arg(CP::PANEL).arg(CP::BORDER));
    auto *inputLay = new QVBoxLayout(inputFrame);
    inputLay->setContentsMargins(6, 4, 6, 4);
    inputLay->setSpacing(4);

    auto *hint = new QLabel("Describe what you want  (Ctrl+Enter = generate):");
    hint->setStyleSheet(QString("color:%1;font-size:8pt;").arg(CP::DIM));
    inputLay->addWidget(hint);

    m_input = new QTextEdit;
    m_input->setFixedHeight(72);
    m_input->setFont(QFont("Consolas", 9));
    m_input->setPlaceholderText(
        "e.g.  Create a temperature converter form with Celsius and Fahrenheit fields…");
    m_input->setStyleSheet(
        QString("QTextEdit { background:%1; color:%2;"
                "  border:1px solid %3; border-radius:3px; padding:4px; }")
        .arg(CP::CONSOLE).arg(CP::TEXT).arg(CP::BORDER));
    m_input->installEventFilter(this);
    inputLay->addWidget(m_input);

    // Buttons row
    auto *btnRow  = new QWidget;
    auto *btnLay  = new QHBoxLayout(btnRow);
    btnLay->setContentsMargins(0, 0, 0, 0);
    btnLay->setSpacing(6);

    m_genBtn = new QPushButton("⚡  Generate");
    m_genBtn->setStyleSheet(
        QString("QPushButton { background:%1; color:#ffffff; border:none;"
                "  padding:6px 14px; border-radius:3px; font-weight:bold; }"
                "QPushButton:hover { background:#4aa3df; }"
                "QPushButton:disabled { background:%2; color:%3; }")
        .arg(CP::ACCENT).arg(CP::BORDER).arg(CP::DIM));
    connect(m_genBtn, &QPushButton::clicked, this, &CopilotPanel::onGenerate);
    btnLay->addWidget(m_genBtn, 1);

    m_insertBtn = new QPushButton("↓  Insert to Editor");
    m_insertBtn->setEnabled(false);
    m_insertBtn->setToolTip("Insert last generated code at cursor position in editor");
    connect(m_insertBtn, &QPushButton::clicked,
            this, &CopilotPanel::onInsertToEditor);
    btnLay->addWidget(m_insertBtn, 1);

    m_clearBtn = new QPushButton("✕");
    m_clearBtn->setMaximumWidth(32);
    m_clearBtn->setToolTip("Clear conversation");
    connect(m_clearBtn, &QPushButton::clicked,
            this, &CopilotPanel::onClearHistory);
    btnLay->addWidget(m_clearBtn);

    inputLay->addWidget(btnRow);

    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setStyleSheet(
        QString("color:%1;font-size:8pt;padding:1px 0;").arg(CP::DIM));
    inputLay->addWidget(m_statusLabel);

    mainLay->addWidget(inputFrame);
}

// ── Ctrl+Enter event filter ───────────────────────────────────────────────

bool CopilotPanel::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_input && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Return &&
            (ke->modifiers() & Qt::ControlModifier)) {
            onGenerate();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

// ── Generate ─────────────────────────────────────────────────────────────

void CopilotPanel::onGenerate() {
    if (!m_localCopilot) {
        m_statusLabel->setText("Copilot not initialised.");
        return;
    }

    QString request = m_input->toPlainText().trimmed();
    if (request.isEmpty()) {
        m_statusLabel->setText("Please describe what you want first.");
        return;
    }

    appendUserBubble(request);
    m_input->clear();
    m_lastCode.clear();
    m_insertBtn->setEnabled(false);
    m_statusLabel->setText("Searching templates…");

    // Instant — result arrives synchronously via queued signal
    m_localCopilot->generate(request);
}

// ── LocalCopilot result handlers ─────────────────────────────────────────

void CopilotPanel::onCodeReady(const QString &code,
                                const QString &title,
                                const QString &description)
{
    m_lastCode = code;
    appendAssistantBubble(title, description, code);
    m_insertBtn->setEnabled(true);
    m_statusLabel->setText(
        QString("✓ Template matched (%1 lines) — click Insert to Editor")
        .arg(code.count('\n') + 1));
}

void CopilotPanel::onNoMatch(const QString &helpText) {
    appendInfoBubble(helpText);
    m_statusLabel->setText("No match — try different keywords.");
}

// ── Clear ─────────────────────────────────────────────────────────────────

void CopilotPanel::onClearHistory() {
    m_history->clear();
    m_lastCode.clear();
    m_insertBtn->setEnabled(false);
    m_statusLabel->setText("History cleared.");
}

// ── Insert code ───────────────────────────────────────────────────────────

void CopilotPanel::onInsertToEditor() {
    if (m_lastCode.isEmpty() || !m_editor) return;
    QTextCursor cur = m_editor->textCursor();
    if (cur.columnNumber() > 0) cur.insertText("\n");
    cur.insertText(m_lastCode);
    m_editor->setTextCursor(cur);
    m_editor->setFocus();
    m_statusLabel->setText("✓ Code inserted into editor.");
}

// ── Chat bubble helpers ───────────────────────────────────────────────────

void CopilotPanel::appendUserBubble(const QString &text) {
    QTextCursor c = m_history->textCursor();
    c.movePosition(QTextCursor::End);
    m_history->setTextCursor(c);

    QTextCharFormat dimFmt;
    dimFmt.setForeground(QColor(CP::BORDER));
    c.insertText("\n────────────────────────────\n", dimFmt);

    QTextCharFormat labelFmt;
    labelFmt.setForeground(QColor("#89b4fa"));
    labelFmt.setFontWeight(QFont::Bold);
    c.insertText("You\n", labelFmt);

    QTextCharFormat msgFmt;
    msgFmt.setForeground(QColor(CP::TEXT));
    msgFmt.setFontWeight(QFont::Normal);
    c.insertText(text + "\n", msgFmt);

    m_history->verticalScrollBar()->setValue(
        m_history->verticalScrollBar()->maximum());
}

void CopilotPanel::appendAssistantBubble(const QString &title,
                                          const QString &description,
                                          const QString &code)
{
    QTextCursor c = m_history->textCursor();
    c.movePosition(QTextCursor::End);
    m_history->setTextCursor(c);

    // "Copilot" header
    QTextCharFormat headerFmt;
    headerFmt.setForeground(QColor("#a6e3a1"));
    headerFmt.setFontWeight(QFont::Bold);
    c.insertText("\nCopilot\n", headerFmt);

    // Template title + description
    QTextCharFormat titleFmt;
    titleFmt.setForeground(QColor(CP::ACCENT));
    titleFmt.setFontWeight(QFont::Bold);
    titleFmt.setFontItalic(false);
    c.insertText("Template: " + title + "\n", titleFmt);

    QTextCharFormat descFmt;
    descFmt.setForeground(QColor(CP::DIM));
    descFmt.setFontWeight(QFont::Normal);
    c.insertText(description + "\n\n", descFmt);

    // Code block — monospace, lighter colour
    QTextCharFormat codeFmt;
    codeFmt.setForeground(QColor(CP::TEXT));
    codeFmt.setFontWeight(QFont::Normal);
    codeFmt.setFontFamily("Consolas");
    c.insertText(code + "\n", codeFmt);

    m_history->verticalScrollBar()->setValue(
        m_history->verticalScrollBar()->maximum());
}

void CopilotPanel::appendInfoBubble(const QString &text) {
    QTextCursor c = m_history->textCursor();
    c.movePosition(QTextCursor::End);
    m_history->setTextCursor(c);

    QTextCharFormat headerFmt;
    headerFmt.setForeground(QColor("#a6e3a1"));
    headerFmt.setFontWeight(QFont::Bold);
    c.insertText("\nCopilot\n", headerFmt);

    QTextCharFormat infoFmt;
    infoFmt.setForeground(QColor(CP::DIM));
    infoFmt.setFontWeight(QFont::Normal);
    c.insertText(text + "\n", infoFmt);

    m_history->verticalScrollBar()->setValue(
        m_history->verticalScrollBar()->maximum());
}
