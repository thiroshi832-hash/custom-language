#pragma once
#include <QPlainTextEdit>
#include <QWidget>
#include <QSet>

class LineNumberArea;
class QTimer;

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    void lineNumberAreaMousePressEvent(QMouseEvent *event);
    int  lineNumberAreaWidth() const;

    // ── Breakpoints ──────────────────────────────────────────────────────
    void            toggleBreakpoint(int line);
    void            clearBreakpoints();
    const QSet<int> &breakpoints() const { return m_breakpoints; }

    // ── Debug current-execution line (yellow arrow in gutter) ────────────
    void setDebugLine(int line);   // 0 = clear
    int  debugLine()  const { return m_debugLine; }

    // ── Copilot ghost text ────────────────────────────────────────────────
    // Called by MainWindow when a completion arrives from CopilotClient.
    // The text is shown as translucent gray overlay starting at the cursor;
    // Tab accepts it, Escape / any edit key dismisses it.
    void setGhostText(const QString &text);
    void clearGhostText();
    bool hasGhostText() const { return !m_ghostText.isEmpty(); }

    // Enable / disable auto-trigger (survives runtime, not persisted here)
    void setCopilotEnabled(bool on);
    bool copilotEnabled() const { return m_copilotEnabled; }

signals:
    void breakpointsChanged();

    // Emitted after the debounce timer fires so MainWindow can forward the
    // prefix/suffix to CopilotClient::requestCompletion().
    // cursorPos is stored so the ghost text can be discarded if the cursor
    // has moved by the time the response arrives.
    void completionRequested(const QString &prefix, const QString &suffix,
                             int cursorPos);

protected:
    void resizeEvent  (QResizeEvent *event)  override;
    void keyPressEvent(QKeyEvent   *event)  override;
    void paintEvent   (QPaintEvent *event)  override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
    void onContentsChanged();
    void onCopilotTimerFired();

private:
    void acceptGhostText();

    QWidget  *m_lineNumberArea;
    QSet<int> m_breakpoints;
    int       m_debugLine { 0 };

    // Copilot
    QString   m_ghostText;
    int       m_ghostCursorPos  { -1 }; // cursor pos when request was made
    QTimer   *m_copilotTimer    { nullptr };
    bool      m_copilotEnabled  { true };
};

// ── LineNumberArea (paint + mouse delegate) ───────────────────────────────

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CodeEditor *editor)
        : QWidget(editor), m_editor(editor) {}

    QSize sizeHint() const override {
        return QSize(m_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        m_editor->lineNumberAreaPaintEvent(event);
    }
    void mousePressEvent(QMouseEvent *event) override {
        m_editor->lineNumberAreaMousePressEvent(event);
    }

private:
    CodeEditor *m_editor;
};
