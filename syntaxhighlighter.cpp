#include "syntaxhighlighter.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    Rule rule;

    // Keywords
    QTextCharFormat kwFmt;
    kwFmt.setForeground(QColor("#569cd6"));
    kwFmt.setFontWeight(QFont::Bold);
    const QStringList keywords = {
        "\\bDim\\b", "\\bIf\\b", "\\bThen\\b", "\\bElse\\b", "\\bElseIf\\b", "\\bEnd\\b",
        "\\bFor\\b", "\\bTo\\b", "\\bStep\\b", "\\bNext\\b",
        "\\bWhile\\b", "\\bWend\\b", "\\bDo\\b", "\\bLoop\\b", "\\bUntil\\b",
        "\\bSub\\b", "\\bFunction\\b", "\\bReturn\\b", "\\bExit\\b",
        "\\bAnd\\b", "\\bOr\\b", "\\bNot\\b", "\\bMod\\b",
        "\\bTrue\\b", "\\bFalse\\b", "\\bNull\\b", "\\bPrint\\b"
    };
    for (const QString &kw : keywords) {
        rule.pattern = QRegularExpression(kw, QRegularExpression::CaseInsensitiveOption);
        rule.format  = kwFmt;
        m_rules.append(rule);
    }

    // Also highlight Call as a keyword
    rule.pattern = QRegularExpression("\\bCall\\b", QRegularExpression::CaseInsensitiveOption);
    rule.format  = kwFmt;
    m_rules.append(rule);

    // Built-in function names (standard + form UI)
    QTextCharFormat builtinFmt;
    builtinFmt.setForeground(QColor("#dcdcaa"));
    const QStringList builtins = {
        // String
        "\\bLen\\b", "\\bLeft\\b", "\\bRight\\b", "\\bMid\\b",
        "\\bUCase\\b", "\\bLCase\\b", "\\bTrim\\b", "\\bLTrim\\b", "\\bRTrim\\b",
        "\\bInStr\\b", "\\bReplace\\b", "\\bString\\b", "\\bSpace\\b", "\\bStrReverse\\b",
        // Math
        "\\bAbs\\b", "\\bInt\\b", "\\bFix\\b", "\\bSgn\\b", "\\bSqr\\b",
        "\\bLog\\b", "\\bExp\\b", "\\bSin\\b", "\\bCos\\b", "\\bTan\\b", "\\bAtn\\b",
        "\\bRnd\\b", "\\bRandomize\\b",
        // Conversion
        "\\bCStr\\b", "\\bCInt\\b", "\\bCLng\\b", "\\bCDbl\\b", "\\bCBool\\b",
        "\\bVal\\b", "\\bStr\\b",
        // Type checks
        "\\bIsNumeric\\b", "\\bIsNull\\b", "\\bIsEmpty\\b", "\\bTypeName\\b",
        // Form / UI built-ins
        "\\bCreateControl\\b", "\\bSetProperty\\b", "\\bGetProperty\\b",
        "\\bAddItem\\b", "\\bShowForm\\b", "\\bInitForm\\b"
    };
    for (const QString &fn : builtins) {
        rule.pattern = QRegularExpression(fn, QRegularExpression::CaseInsensitiveOption);
        rule.format  = builtinFmt;
        m_rules.append(rule);
    }

    // Numbers
    QTextCharFormat numFmt;
    numFmt.setForeground(QColor("#b5cea8"));
    rule.pattern = QRegularExpression("\\b[0-9]+(\\.[0-9]+)?\\b");
    rule.format  = numFmt;
    m_rules.append(rule);

    // Operators
    QTextCharFormat opFmt;
    opFmt.setForeground(QColor("#d4d4d4"));
    rule.pattern = QRegularExpression("[+\\-*/\\\\^&<>=]");
    rule.format  = opFmt;
    m_rules.append(rule);

    // String format (handled separately in highlightBlock)
    m_stringFmt.setForeground(QColor("#ce9178"));

    // Comment format
    m_commentFmt.setForeground(QColor("#6a9955"));
    m_commentFmt.setFontItalic(true);
}

void SyntaxHighlighter::highlightBlock(const QString &text) {
    // Apply keyword / number / operator rules first
    for (const Rule &r : m_rules) {
        QRegularExpressionMatchIterator it = r.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            setFormat(m.capturedStart(), m.capturedLength(), r.format);
        }
    }

    // Strings: "..."
    int i = 0;
    while (i < text.length()) {
        if (text[i] == '"') {
            int start = i++;
            while (i < text.length()) {
                if (text[i] == '"') {
                    if (i + 1 < text.length() && text[i+1] == '"') { i += 2; continue; }
                    i++; break;
                }
                i++;
            }
            setFormat(start, i - start, m_stringFmt);
        } else if (text[i] == '\'') {
            // Comment from here to end
            setFormat(i, text.length() - i, m_commentFmt);
            break;
        } else {
            i++;
        }
    }
}
