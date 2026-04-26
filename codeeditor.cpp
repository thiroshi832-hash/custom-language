#include "codeeditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPolygon>
#include <QTimer>
#include <QScrollBar>

static const int BP_AREA = 16; // pixels reserved for breakpoint / arrow indicator

// ── Constructor ───────────────────────────────────────────────────────────

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    m_lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest,
            this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    QFont font("Courier New", 10);
    font.setFixedPitch(true);
    setFont(font);
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
    setLineWrapMode(QPlainTextEdit::NoWrap);

    // ── Copilot debounce timer ────────────────────────────────────────────
    m_copilotTimer = new QTimer(this);
    m_copilotTimer->setSingleShot(true);
    m_copilotTimer->setInterval(800); // ms after last keystroke
    connect(m_copilotTimer, &QTimer::timeout,
            this, &CodeEditor::onCopilotTimerFired);

    connect(document(), &QTextDocument::contentsChanged,
            this, &CodeEditor::onContentsChanged);
}

// ── Copilot enable/disable ────────────────────────────────────────────────

void CodeEditor::setCopilotEnabled(bool on) {
    m_copilotEnabled = on;
    if (!on) {
        m_copilotTimer->stop();
        clearGhostText();
    }
}

// ── Geometry ──────────────────────────────────────────────────────────────

int CodeEditor::lineNumberAreaWidth() const {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    return BP_AREA + 4 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(),
                                  m_lineNumberArea->width(), rect.height());
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e) {
    QPlainTextEdit::resizeEvent(e);
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(),
                                        lineNumberAreaWidth(), cr.height()));
}

// ── Current-line highlight ────────────────────────────────────────────────

void CodeEditor::highlightCurrentLine() {
    if (m_debugLine > 0) return;

    QList<QTextEdit::ExtraSelection> sels;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(QColor("#323232"));
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = textCursor();
        sel.cursor.clearSelection();
        sels.append(sel);
    }
    setExtraSelections(sels);
}

// ── Debug execution line ──────────────────────────────────────────────────

void CodeEditor::setDebugLine(int line) {
    m_debugLine = line;
    m_lineNumberArea->update();

    if (line > 0) {
        QTextBlock block = document()->findBlockByLineNumber(line - 1);
        if (block.isValid()) {
            QTextCursor cur(block);
            setTextCursor(cur);
            ensureCursorVisible();

            QTextEdit::ExtraSelection sel;
            sel.format.setBackground(QColor("#332b00"));
            sel.format.setProperty(QTextFormat::FullWidthSelection, true);
            sel.cursor = cur;
            sel.cursor.clearSelection();
            setExtraSelections({ sel });
        }
    } else {
        highlightCurrentLine();
    }
}

// ── Breakpoints ───────────────────────────────────────────────────────────

void CodeEditor::toggleBreakpoint(int line) {
    if (m_breakpoints.contains(line))
        m_breakpoints.remove(line);
    else
        m_breakpoints.insert(line);

    m_lineNumberArea->update();
    emit breakpointsChanged();
}

void CodeEditor::clearBreakpoints() {
    m_breakpoints.clear();
    m_lineNumberArea->update();
    emit breakpointsChanged();
}

void CodeEditor::lineNumberAreaMousePressEvent(QMouseEvent *event) {
    QTextBlock block  = firstVisibleBlock();
    int        bnum   = block.blockNumber();
    int        top    = qRound(blockBoundingGeometry(block)
                               .translated(contentOffset()).top());

    while (block.isValid()) {
        int bottom = top + qRound(blockBoundingRect(block).height());
        if (event->y() >= top && event->y() < bottom) {
            toggleBreakpoint(bnum + 1);
            return;
        }
        block = block.next();
        top   = bottom;
        ++bnum;
    }
}

// ── Gutter paint ─────────────────────────────────────────────────────────

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor("#313335"));

    QTextBlock block  = firstVisibleBlock();
    int        bnum   = block.blockNumber();
    int        top    = qRound(blockBoundingGeometry(block)
                               .translated(contentOffset()).top());
    int        bottom = top + qRound(blockBoundingRect(block).height());
    int        lh     = fontMetrics().height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            int lineNum = bnum + 1;
            int cy      = top + lh / 2;

            if (m_breakpoints.contains(lineNum)) {
                painter.save();
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor("#f38ba8"));
                painter.setRenderHint(QPainter::Antialiasing);
                painter.drawEllipse(2, cy - 5, 10, 10);
                painter.restore();
            }

            if (m_debugLine == lineNum) {
                painter.save();
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor("#f9e2af"));
                painter.setRenderHint(QPainter::Antialiasing);
                QPolygon arrow;
                arrow << QPoint(2, cy - 5)
                      << QPoint(13, cy)
                      << QPoint(2, cy + 5);
                painter.drawPolygon(arrow);
                painter.restore();
            }

            painter.setPen(QColor("#606366"));
            painter.drawText(BP_AREA, top,
                             m_lineNumberArea->width() - BP_AREA - 2,
                             lh, Qt::AlignRight,
                             QString::number(lineNum));
        }

        block  = block.next();
        top    = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++bnum;
    }
}

// ── Copilot – ghost text ──────────────────────────────────────────────────

void CodeEditor::setGhostText(const QString &text) {
    // Discard if cursor has moved since the request was made
    if (m_ghostCursorPos >= 0 && textCursor().position() != m_ghostCursorPos)
        return;

    m_ghostText = text;
    viewport()->update();
}

void CodeEditor::clearGhostText() {
    if (m_ghostText.isEmpty()) return;
    m_ghostText.clear();
    viewport()->update();
}

void CodeEditor::acceptGhostText() {
    if (m_ghostText.isEmpty()) return;
    QTextCursor cur = textCursor();
    cur.insertText(m_ghostText);
    setTextCursor(cur);
    m_ghostText.clear();
    m_ghostCursorPos = -1;
    viewport()->update();
}

// ── Copilot – debounce timer ──────────────────────────────────────────────

void CodeEditor::onContentsChanged() {
    if (!m_copilotEnabled || isReadOnly()) return;
    if (!m_ghostText.isEmpty()) return; // don't re-trigger while suggestion showing
    m_copilotTimer->start(); // restart 800 ms window
}

void CodeEditor::onCopilotTimerFired() {
    if (!m_copilotEnabled || isReadOnly()) return;
    if (!m_ghostText.isEmpty()) return;

    QString src = toPlainText();
    if (src.trimmed().length() < 6) return; // too little context

    QTextCursor cur = textCursor();
    int pos = cur.position();

    QString prefix = src.left(pos);
    QString suffix = src.mid(pos);

    m_ghostCursorPos = pos;
    emit completionRequested(prefix, suffix, pos);
}

// ── Key press – Tab accepts, Esc dismisses, others clear suggestion ───────

void CodeEditor::keyPressEvent(QKeyEvent *e) {
    // Tab key: accept ghost text if present; otherwise fall through
    if (e->key() == Qt::Key_Tab && !m_ghostText.isEmpty()) {
        acceptGhostText();
        e->accept();
        return;
    }

    // Escape: dismiss ghost text explicitly (still process Escape normally)
    if (e->key() == Qt::Key_Escape) {
        clearGhostText();
        m_copilotTimer->stop();
        QPlainTextEdit::keyPressEvent(e);
        return;
    }

    // Any edit key clears the ghost text before the edit is applied
    if (!m_ghostText.isEmpty()) {
        bool navigationOnly =
            (e->key() == Qt::Key_Left  || e->key() == Qt::Key_Right ||
             e->key() == Qt::Key_Up    || e->key() == Qt::Key_Down  ||
             e->key() == Qt::Key_Home  || e->key() == Qt::Key_End   ||
             e->key() == Qt::Key_PageUp || e->key() == Qt::Key_PageDown ||
             (e->modifiers() & Qt::ControlModifier));
        if (!navigationOnly)
            clearGhostText();
    }

    QPlainTextEdit::keyPressEvent(e);
}

// ── Paint event – draw ghost text overlay ────────────────────────────────
// Strategy: call base first (renders real content), then draw the ghost
// text in dim gray on top of the viewport at the cursor position.

void CodeEditor::paintEvent(QPaintEvent *event) {
    QPlainTextEdit::paintEvent(event); // draw real content first

    if (m_ghostText.isEmpty()) return;

    QPainter painter(viewport());
    painter.setFont(font());

    // Ghost text colour: dim, italic, similar to a comment
    QColor ghostColor("#6a7a8a"); // muted blue-gray
    QFont  ghostFont = font();
    ghostFont.setItalic(true);
    painter.setFont(ghostFont);
    painter.setPen(ghostColor);

    QRect  curRect  = cursorRect(textCursor());
    int    lineH    = fontMetrics().height();
    int    ascent   = fontMetrics().ascent();
    int    leftMargin = (int)(document()->documentMargin());

    QStringList lines = m_ghostText.split('\n');

    for (int i = 0; i < lines.size(); ++i) {
        int x = (i == 0) ? curRect.left() : leftMargin;
        int y = curRect.top() + i * lineH + ascent;

        // Clip check: don't paint below visible area
        if (y - ascent > viewport()->height()) break;

        painter.drawText(x, y, lines[i]);
    }
}
