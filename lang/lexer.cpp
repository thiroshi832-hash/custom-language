#include "lexer.h"
#include <QMap>

static QMap<QString, TokenType> buildKeywords() {
    QMap<QString, TokenType> m;
    m["dim"]      = TokenType::KW_DIM;
    m["if"]       = TokenType::KW_IF;
    m["then"]     = TokenType::KW_THEN;
    m["else"]     = TokenType::KW_ELSE;
    m["elseif"]   = TokenType::KW_ELSEIF;
    m["end"]      = TokenType::KW_END;
    m["for"]      = TokenType::KW_FOR;
    m["to"]       = TokenType::KW_TO;
    m["step"]     = TokenType::KW_STEP;
    m["next"]     = TokenType::KW_NEXT;
    m["while"]    = TokenType::KW_WHILE;
    m["wend"]     = TokenType::KW_WEND;
    m["do"]       = TokenType::KW_DO;
    m["loop"]     = TokenType::KW_LOOP;
    m["until"]    = TokenType::KW_UNTIL;
    m["sub"]      = TokenType::KW_SUB;
    m["function"] = TokenType::KW_FUNCTION;
    m["return"]   = TokenType::KW_RETURN;
    m["exit"]     = TokenType::KW_EXIT;
    m["call"]     = TokenType::KW_CALL;
    m["and"]      = TokenType::KW_AND;
    m["or"]       = TokenType::KW_OR;
    m["not"]      = TokenType::KW_NOT;
    m["mod"]      = TokenType::KW_MOD;
    m["true"]     = TokenType::KW_TRUE;
    m["false"]    = TokenType::KW_FALSE;
    m["null"]     = TokenType::KW_NULL;
    m["print"]    = TokenType::KW_PRINT;
    return m;
}
static const QMap<QString, TokenType> s_keywords = buildKeywords();

Lexer::Lexer(const QString &source)
    : m_source(source), m_pos(0), m_line(1), m_column(1) {}

QChar Lexer::current() const {
    return (m_pos < m_source.length()) ? m_source[m_pos] : QChar(0);
}

QChar Lexer::peek(int offset) const {
    int p = m_pos + offset;
    return (p < m_source.length()) ? m_source[p] : QChar(0);
}

void Lexer::advance() {
    if (m_pos < m_source.length()) {
        if (m_source[m_pos] == '\n') { m_line++; m_column = 1; }
        else { m_column++; }
        m_pos++;
    }
}

void Lexer::skipWhitespace() {
    while (m_pos < m_source.length()) {
        QChar c = current();
        if (c == ' ' || c == '\t' || c == '\r') advance();
        else break;
    }
}

void Lexer::skipComment() {
    while (m_pos < m_source.length() && current() != '\n')
        advance();
}

Token Lexer::readString() {
    int sl = m_line, sc = m_column;
    advance(); // skip opening "
    QString val;
    while (m_pos < m_source.length() && current() != '\n') {
        if (current() == '"') {
            if (peek() == '"') { val += '"'; advance(); advance(); }
            else { advance(); break; }
        } else {
            val += current(); advance();
        }
    }
    return Token(TokenType::STRING, val, sl, sc);
}

Token Lexer::readNumber() {
    int sl = m_line, sc = m_column;
    QString val;
    bool isFloat = false;
    while (m_pos < m_source.length() && current().isDigit()) {
        val += current(); advance();
    }
    if (current() == '.' && peek().isDigit()) {
        isFloat = true; val += current(); advance();
        while (m_pos < m_source.length() && current().isDigit()) {
            val += current(); advance();
        }
    }
    return Token(isFloat ? TokenType::FLOAT : TokenType::INTEGER, val, sl, sc);
}

Token Lexer::readIdentifierOrKeyword() {
    int sl = m_line, sc = m_column;
    QString val;
    while (m_pos < m_source.length() &&
           (current().isLetterOrNumber() || current() == '_')) {
        val += current(); advance();
    }
    auto it = s_keywords.find(val.toLower());
    if (it != s_keywords.end())
        return Token(it.value(), val, sl, sc);
    return Token(TokenType::IDENTIFIER, val, sl, sc);
}

TokenType Lexer::keywordType(const QString &word) {
    auto it = s_keywords.find(word.toLower());
    return (it != s_keywords.end()) ? it.value() : TokenType::IDENTIFIER;
}

QVector<Token> Lexer::tokenize() {
    QVector<Token> tokens;

    while (m_pos < m_source.length() && m_error.isEmpty()) {
        skipWhitespace();
        if (m_pos >= m_source.length()) break;

        QChar c = current();
        int   line = m_line, col = m_column;

        // Line continuation: _ at end of line
        if (c == '_' && (peek() == '\n' || peek() == '\r' || peek() == QChar(0))) {
            advance();
            if (current() == '\r') advance();
            if (current() == '\n') advance();
            continue;
        }

        if (c == '\n') {
            tokens.append(Token(TokenType::NEWLINE, "\\n", line, col));
            advance(); continue;
        }

        if (c == '\'') { skipComment(); continue; }
        if (c == '"')  { tokens.append(readString()); continue; }
        if (c.isDigit()){ tokens.append(readNumber()); continue; }
        if (c.isLetter() || c == '_') { tokens.append(readIdentifierOrKeyword()); continue; }

        switch (c.toLatin1()) {
        case '+': tokens.append(Token(TokenType::OP_PLUS,  "+", line, col)); advance(); break;
        case '-': tokens.append(Token(TokenType::OP_MINUS, "-", line, col)); advance(); break;
        case '*': tokens.append(Token(TokenType::OP_STAR,  "*", line, col)); advance(); break;
        case '/': tokens.append(Token(TokenType::OP_SLASH, "/", line, col)); advance(); break;
        case '\\':tokens.append(Token(TokenType::OP_BACKSLASH,"\\",line,col));advance();break;
        case '^': tokens.append(Token(TokenType::OP_CARET, "^", line, col)); advance(); break;
        case '&': tokens.append(Token(TokenType::OP_AMPERSAND,"&",line,col)); advance(); break;
        case '(': tokens.append(Token(TokenType::LPAREN,   "(", line, col)); advance(); break;
        case ')': tokens.append(Token(TokenType::RPAREN,   ")", line, col)); advance(); break;
        case ',': tokens.append(Token(TokenType::COMMA,    ",", line, col)); advance(); break;
        case '.': tokens.append(Token(TokenType::DOT,      ".", line, col)); advance(); break;
        case ':': tokens.append(Token(TokenType::COLON,    ":", line, col)); advance(); break;
        case '=': tokens.append(Token(TokenType::OP_EQ,    "=", line, col)); advance(); break;
        case '<':
            advance();
            if (current() == '>') { tokens.append(Token(TokenType::OP_NEQ,"<>",line,col)); advance(); }
            else if (current() == '=') { tokens.append(Token(TokenType::OP_LTE,"<=",line,col)); advance(); }
            else   tokens.append(Token(TokenType::OP_LT, "<", line, col));
            break;
        case '>':
            advance();
            if (current() == '=') { tokens.append(Token(TokenType::OP_GTE,">=",line,col)); advance(); }
            else   tokens.append(Token(TokenType::OP_GT, ">", line, col));
            break;
        default:
            m_error = QString("Unknown character '%1' at line %2").arg(c).arg(line);
            advance();
            break;
        }
    }

    tokens.append(Token(TokenType::END_OF_FILE, "", m_line, m_column));
    return tokens;
}
