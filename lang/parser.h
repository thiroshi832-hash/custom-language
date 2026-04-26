#pragma once
#include "token.h"
#include "ast.h"
#include <QVector>
#include <QString>
#include <initializer_list>

class Parser {
public:
    explicit Parser(const QVector<Token> &tokens);

    Program *parse();
    bool     hasError()    const { return !m_error.isEmpty(); }
    QString  errorMessage()const { return m_error; }

private:
    QVector<Token> m_tokens;
    int            m_pos;
    QString        m_error;

    // ── Navigation ──────────────────────────────────────────────────────
    const Token &current()           const;
    const Token &lookAhead(int off=1)const;
    bool  check(TokenType t)         const;
    Token consume();
    Token expect(TokenType t, const QString &msg);
    void  skipNewlines();
    bool  isBlockEnd()               const;

    // ── Statement helpers ───────────────────────────────────────────────
    QVector<ASTNode*> parseBlock();
    ASTNode  *parseStatement();
    DimStmt  *parseDim();
    IfStmt   *parseIf();
    ForStmt  *parseFor();
    WhileStmt*parseWhile();
    DoLoopStmt *parseDoLoop();
    SubDecl  *parseSub();
    FuncDecl *parseFunction();
    ASTNode  *parseReturn();
    ASTNode  *parseExit();
    PrintStmt*parsePrint();
    ASTNode  *parseAssignOrCall();
    ASTNode  *parseCallStmt();      // handles the  Call  keyword

    // ── Expression helpers (recursive-descent) ──────────────────────────
    ASTNode *parseExpr();
    ASTNode *parseOr();
    ASTNode *parseAnd();
    ASTNode *parseNot();
    ASTNode *parseComparison();
    ASTNode *parseConcat();
    ASTNode *parseAddSub();
    ASTNode *parseMulDiv();
    ASTNode *parseUnary();
    ASTNode *parsePower();
    ASTNode *parsePrimary();

    void setError(const QString &msg);
};
