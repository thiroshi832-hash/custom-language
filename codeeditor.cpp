#include "codeeditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QMouseEvent>
#include <QPolygon>

static const int BP_AREA = 16; // pixels reserved for breakpoint / arrow indicator

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
    if (m_debugLine > 0) return; // let debug highlight take over

    QList<QTextEdit::ExtraSelection> sels;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(QColor("#2a2a3a"));
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
            sel.format.setBackground(QColor("#332b00")); // dark amber
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
    painter.fillRect(event->rect(), QColor("#1e1e2e"));

    QTextBlock block  = firstVisibleBlock();
    int        bnum   = block.blockNumber();
    int        top    = qRound(blockBoundingGeometry(block)
                               .translated(contentOffset()).top());
    int        bottom = top + qRound(blockBoundingRect(block).height());
    int        lh     = fontMetrics().height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            int lineNum = bnum + 1;
            int cy      = top + lh / 2; // vertical centre of this line

            // ── Breakpoint indicator (red circle) ────────────────────────
            if (m_breakpoints.contains(lineNum)) {
                painter.save();
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor("#f38ba8"));
                painter.setRenderHint(QPainter::Antialiasing);
                painter.drawEllipse(2, cy - 5, 10, 10);
                painter.restore();
            }

            // ── Debug arrow (yellow triangle) ────────────────────────────
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

            // ── Line number ───────────────────────────────────────────────
            painter.setPen(QColor("#6e6e8e"));
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
