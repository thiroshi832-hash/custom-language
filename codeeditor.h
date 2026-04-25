#pragma once
#include <QPlainTextEdit>
#include <QWidget>
#include <QSet>

class LineNumberArea;

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

signals:
    void breakpointsChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    QWidget  *m_lineNumberArea;
    QSet<int> m_breakpoints;
    int       m_debugLine { 0 };
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
