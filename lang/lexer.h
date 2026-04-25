#pragma once
#include "token.h"
#include <QVector>
#include <QString>

class Lexer {
public:
    explicit Lexer(const QString &source);

    QVector<Token> tokenize();
    bool    hasError()    const { return !m_error.isEmpty(); }
    QString errorMessage()const { return m_error; }

private:
    QString m_source;
    int     m_pos;
    int     m_line;
    int     m_column;
    QString m_error;

    QChar current()           const;
    QChar peek(int offset = 1)const;
    void  advance();
    void  skipWhitespace();
    void  skipComment();

    Token readString();
    Token readNumber();
    Token readIdentifierOrKeyword();

    static TokenType keywordType(const QString &word);
};
