#pragma once
#include <QString>

enum class TokenType {
    // Literals
    INTEGER, FLOAT, STRING,
    // Identifier
    IDENTIFIER,
    // Keywords
    KW_DIM,
    KW_IF, KW_THEN, KW_ELSE, KW_ELSEIF, KW_END,
    KW_FOR, KW_TO, KW_STEP, KW_NEXT,
    KW_WHILE, KW_WEND,
    KW_DO, KW_LOOP, KW_UNTIL,
    KW_SUB, KW_FUNCTION, KW_RETURN,
    KW_EXIT,
    KW_AND, KW_OR, KW_NOT, KW_MOD,
    KW_TRUE, KW_FALSE, KW_NULL,
    KW_PRINT,
    // Operators
    OP_PLUS, OP_MINUS, OP_STAR, OP_SLASH, OP_BACKSLASH, OP_CARET,
    OP_EQ, OP_NEQ, OP_LT, OP_GT, OP_LTE, OP_GTE,
    OP_AMPERSAND,
    // Punctuation
    LPAREN, RPAREN, COMMA, DOT, COLON,
    // Structure
    NEWLINE,
    END_OF_FILE,
    INVALID
};

struct Token {
    TokenType type;
    QString   value;
    int       line;
    int       column;

    Token() : type(TokenType::INVALID), line(0), column(0) {}
    Token(TokenType t, const QString &v, int l, int c)
        : type(t), value(v), line(l), column(c) {}
};
