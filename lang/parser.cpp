#include "parser.h"

// ── Helpers ───────────────────────────────────────────────────────────────

static Token s_eof(TokenType::END_OF_FILE, "", 0, 0);

Parser::Parser(const QVector<Token> &tokens)
    : m_tokens(tokens), m_pos(0) {}

const Token &Parser::current() const {
    return (m_pos < m_tokens.size()) ? m_tokens[m_pos] : s_eof;
}

const Token &Parser::lookAhead(int off) const {
    int p = m_pos + off;
    return (p < m_tokens.size()) ? m_tokens[p] : s_eof;
}

bool Parser::check(TokenType t) const { return current().type == t; }

Token Parser::consume() {
    Token t = current();
    if (m_pos < m_tokens.size()) m_pos++;
    return t;
}

Token Parser::expect(TokenType t, const QString &msg) {
    if (!check(t)) { setError(msg); return Token(); }
    return consume();
}

void Parser::skipNewlines() {
    while (check(TokenType::NEWLINE) || check(TokenType::COLON))
        consume();
}

void Parser::setError(const QString &msg) {
    if (m_error.isEmpty())
        m_error = QString("Line %1: %2").arg(current().line).arg(msg);
}

bool Parser::isBlockEnd() const {
    TokenType t = current().type;
    if (t == TokenType::KW_END)    return true;
    if (t == TokenType::KW_NEXT)   return true;
    if (t == TokenType::KW_WEND)   return true;
    if (t == TokenType::KW_LOOP)   return true;
    if (t == TokenType::KW_ELSE)   return true;
    if (t == TokenType::KW_ELSEIF) return true;
    if (t == TokenType::END_OF_FILE) return true;
    return false;
}

// ── Block ─────────────────────────────────────────────────────────────────

QVector<ASTNode*> Parser::parseBlock() {
    QVector<ASTNode*> block;
    skipNewlines();
    while (!isBlockEnd() && m_error.isEmpty()) {
        ASTNode *s = parseStatement();
        if (s) block.append(s);
        skipNewlines();
    }
    return block;
}

// ── Top-level parse ───────────────────────────────────────────────────────

Program *Parser::parse() {
    auto *prog = new Program();
    skipNewlines();
    while (!check(TokenType::END_OF_FILE) && m_error.isEmpty()) {
        ASTNode *s = parseStatement();
        if (s) prog->statements.append(s);
        skipNewlines();
    }
    return prog;
}

// ── Statement dispatcher ──────────────────────────────────────────────────

ASTNode *Parser::parseStatement() {
    skipNewlines();
    TokenType t = current().type;

    if (t == TokenType::KW_DIM)       return parseDim();
    if (t == TokenType::KW_IF)        return parseIf();
    if (t == TokenType::KW_FOR)       return parseFor();
    if (t == TokenType::KW_WHILE)     return parseWhile();
    if (t == TokenType::KW_DO)        return parseDoLoop();
    if (t == TokenType::KW_SUB)       return parseSub();
    if (t == TokenType::KW_FUNCTION)  return parseFunction();
    if (t == TokenType::KW_RETURN)    return parseReturn();
    if (t == TokenType::KW_EXIT)      return parseExit();
    if (t == TokenType::KW_PRINT)     return parsePrint();
    if (t == TokenType::IDENTIFIER)   return parseAssignOrCall();

    // Bare newline / colon
    if (t == TokenType::NEWLINE || t == TokenType::COLON) {
        consume(); return nullptr;
    }

    setError(QString("Unexpected token '%1'").arg(current().value));
    consume();
    return nullptr;
}

// ── Dim ───────────────────────────────────────────────────────────────────

DimStmt *Parser::parseDim() {
    int line = current().line;
    consume(); // Dim
    QString name = current().value;
    expect(TokenType::IDENTIFIER, "Expected variable name after 'Dim'");
    ASTNode *init = nullptr;
    if (check(TokenType::OP_EQ)) {
        consume();
        init = parseExpr();
    }
    return new DimStmt(name, init, line);
}

// ── If ────────────────────────────────────────────────────────────────────

IfStmt *Parser::parseIf() {
    int line = current().line;
    consume(); // If
    ASTNode *cond = parseExpr();
    expect(TokenType::KW_THEN, "Expected 'Then' after condition");

    bool singleLine = !check(TokenType::NEWLINE) &&
                      !check(TokenType::COLON)   &&
                      !check(TokenType::END_OF_FILE);

    auto *stmt = new IfStmt(cond, line);

    if (singleLine) {
        ASTNode *s = parseStatement();
        if (s) stmt->thenBlock.append(s);
        if (check(TokenType::KW_ELSE)) {
            consume();
            stmt->hasElse = true;
            ASTNode *e = parseStatement();
            if (e) stmt->elseBlock.append(e);
        }
        return stmt;
    }

    // Multi-line block
    skipNewlines();
    stmt->thenBlock = parseBlock();

    // Handle ElseIf chain by nesting into elseBlock
    IfStmt *tail = stmt;
    while (check(TokenType::KW_ELSEIF) && m_error.isEmpty()) {
        int eLine = current().line;
        consume(); // ElseIf
        ASTNode *elseifCond = parseExpr();
        expect(TokenType::KW_THEN, "Expected 'Then' after ElseIf condition");
        skipNewlines();
        auto *nested = new IfStmt(elseifCond, eLine);
        nested->thenBlock = parseBlock();
        tail->hasElse = true;
        tail->elseBlock.append(nested);
        tail = nested;
    }

    if (check(TokenType::KW_ELSE)) {
        consume();
        skipNewlines();
        tail->hasElse = true;
        tail->elseBlock = parseBlock();
    }

    expect(TokenType::KW_END, "Expected 'End If'");
    expect(TokenType::KW_IF,  "Expected 'If' after 'End'");
    return stmt;
}

// ── For ───────────────────────────────────────────────────────────────────

ForStmt *Parser::parseFor() {
    int line = current().line;
    consume(); // For
    QString varName = current().value;
    expect(TokenType::IDENTIFIER, "Expected loop variable");
    expect(TokenType::OP_EQ, "Expected '=' in For statement");
    ASTNode *start = parseExpr();
    expect(TokenType::KW_TO, "Expected 'To'");
    ASTNode *limit = parseExpr();
    ASTNode *step  = nullptr;
    if (check(TokenType::KW_STEP)) {
        consume();
        step = parseExpr();
    }
    skipNewlines();
    QVector<ASTNode*> body = parseBlock();
    expect(TokenType::KW_NEXT, "Expected 'Next'");
    // Optional: Next varName
    if (check(TokenType::IDENTIFIER)) consume();

    auto *fs = new ForStmt(varName, start, limit, step, line);
    fs->body = body;
    return fs;
}

// ── While ─────────────────────────────────────────────────────────────────

WhileStmt *Parser::parseWhile() {
    int line = current().line;
    consume(); // While
    ASTNode *cond = parseExpr();
    skipNewlines();
    QVector<ASTNode*> body = parseBlock();
    expect(TokenType::KW_WEND, "Expected 'Wend'");
    auto *ws = new WhileStmt(cond, line);
    ws->body = body;
    return ws;
}

// ── Do Loop ───────────────────────────────────────────────────────────────

DoLoopStmt *Parser::parseDoLoop() {
    int line = current().line;
    consume(); // Do
    auto *dl = new DoLoopStmt(line);

    if (check(TokenType::KW_WHILE) || check(TokenType::KW_UNTIL)) {
        dl->condAtTop = true;
        dl->isUntil   = check(TokenType::KW_UNTIL);
        consume();
        dl->condition = parseExpr();
    }

    skipNewlines();
    dl->body = parseBlock();
    expect(TokenType::KW_LOOP, "Expected 'Loop'");

    if (!dl->condition) {
        if (check(TokenType::KW_WHILE) || check(TokenType::KW_UNTIL)) {
            dl->condAtTop = false;
            dl->isUntil   = check(TokenType::KW_UNTIL);
            consume();
            dl->condition = parseExpr();
        }
    }
    return dl;
}

// ── Sub / Function ────────────────────────────────────────────────────────

SubDecl *Parser::parseSub() {
    int line = current().line;
    consume(); // Sub
    QString name = current().value;
    expect(TokenType::IDENTIFIER, "Expected sub name");
    auto *sd = new SubDecl(name, line);

    expect(TokenType::LPAREN, "Expected '(' after sub name");
    while (!check(TokenType::RPAREN) && !check(TokenType::END_OF_FILE)) {
        sd->params << current().value;
        expect(TokenType::IDENTIFIER, "Expected parameter name");
        if (check(TokenType::COMMA)) consume();
    }
    expect(TokenType::RPAREN, "Expected ')'");
    skipNewlines();
    sd->body = parseBlock();
    expect(TokenType::KW_END, "Expected 'End Sub'");
    expect(TokenType::KW_SUB, "Expected 'Sub' after 'End'");
    return sd;
}

FuncDecl *Parser::parseFunction() {
    int line = current().line;
    consume(); // Function
    QString name = current().value;
    expect(TokenType::IDENTIFIER, "Expected function name");
    auto *fd = new FuncDecl(name, line);

    expect(TokenType::LPAREN, "Expected '(' after function name");
    while (!check(TokenType::RPAREN) && !check(TokenType::END_OF_FILE)) {
        fd->params << current().value;
        expect(TokenType::IDENTIFIER, "Expected parameter name");
        if (check(TokenType::COMMA)) consume();
    }
    expect(TokenType::RPAREN, "Expected ')'");
    skipNewlines();
    fd->body = parseBlock();
    expect(TokenType::KW_END,      "Expected 'End Function'");
    expect(TokenType::KW_FUNCTION, "Expected 'Function' after 'End'");
    return fd;
}

// ── Return / Exit ─────────────────────────────────────────────────────────

ASTNode *Parser::parseReturn() {
    int line = current().line;
    consume(); // Return
    ASTNode *val = nullptr;
    if (!check(TokenType::NEWLINE) && !check(TokenType::COLON) &&
        !check(TokenType::END_OF_FILE)) {
        val = parseExpr();
    }
    return new ReturnStmt(val, line);
}

ASTNode *Parser::parseExit() {
    int line = current().line;
    consume(); // Exit
    ExitTarget tgt = ExitTarget::Sub;
    if (check(TokenType::KW_SUB))      { tgt = ExitTarget::Sub;      consume(); }
    else if (check(TokenType::KW_FUNCTION)){ tgt = ExitTarget::Function; consume(); }
    else if (check(TokenType::KW_FOR)) { tgt = ExitTarget::For;      consume(); }
    else if (check(TokenType::KW_WHILE)){ tgt = ExitTarget::While;   consume(); }
    else if (check(TokenType::KW_DO))  { tgt = ExitTarget::Do;       consume(); }
    return new ExitStmt(tgt, line);
}

// ── Print ─────────────────────────────────────────────────────────────────

PrintStmt *Parser::parsePrint() {
    int line = current().line;
    consume(); // Print
    auto *ps = new PrintStmt(line);
    if (!check(TokenType::NEWLINE) && !check(TokenType::COLON) &&
        !check(TokenType::END_OF_FILE)) {
        ps->args.append(parseExpr());
        while (check(TokenType::COMMA)) {
            consume();
            ps->args.append(parseExpr());
        }
    }
    return ps;
}

// ── Assign or Call ────────────────────────────────────────────────────────

ASTNode *Parser::parseAssignOrCall() {
    int     line = current().line;
    QString name = consume().value; // consume identifier

    if (check(TokenType::OP_EQ)) {
        consume(); // =
        ASTNode *val = parseExpr();
        return new AssignStmt(name, val, line);
    }

    // Call statement
    auto *cs = new CallStmt(name, line);
    bool hasParen = check(TokenType::LPAREN);
    if (hasParen) consume();

    bool hasArgs = !check(TokenType::NEWLINE) && !check(TokenType::COLON) &&
                   !check(TokenType::END_OF_FILE) &&
                   !(hasParen && check(TokenType::RPAREN));

    if (hasArgs) {
        cs->args.append(parseExpr());
        while (check(TokenType::COMMA)) {
            consume();
            cs->args.append(parseExpr());
        }
    }
    if (hasParen) expect(TokenType::RPAREN, "Expected ')'");
    return cs;
}

// ── Expressions ──────────────────────────────────────────────────────────

ASTNode *Parser::parseExpr()       { return parseOr(); }

ASTNode *Parser::parseOr() {
    ASTNode *left = parseAnd();
    while (check(TokenType::KW_OR) && m_error.isEmpty()) {
        int l = current().line; consume();
        left = new BinaryExpr(left, parseAnd(), "Or", l);
    }
    return left;
}

ASTNode *Parser::parseAnd() {
    ASTNode *left = parseNot();
    while (check(TokenType::KW_AND) && m_error.isEmpty()) {
        int l = current().line; consume();
        left = new BinaryExpr(left, parseNot(), "And", l);
    }
    return left;
}

ASTNode *Parser::parseNot() {
    if (check(TokenType::KW_NOT)) {
        int l = current().line; consume();
        return new UnaryExpr(parseNot(), "Not", l);
    }
    return parseComparison();
}

ASTNode *Parser::parseComparison() {
    ASTNode *left = parseConcat();
    for (;;) {
        TokenType t = current().type;
        QString op;
        if      (t == TokenType::OP_EQ)  op = "=";
        else if (t == TokenType::OP_NEQ) op = "<>";
        else if (t == TokenType::OP_LT)  op = "<";
        else if (t == TokenType::OP_GT)  op = ">";
        else if (t == TokenType::OP_LTE) op = "<=";
        else if (t == TokenType::OP_GTE) op = ">=";
        else break;
        int l = current().line; consume();
        left = new BinaryExpr(left, parseConcat(), op, l);
    }
    return left;
}

ASTNode *Parser::parseConcat() {
    ASTNode *left = parseAddSub();
    while (check(TokenType::OP_AMPERSAND) && m_error.isEmpty()) {
        int l = current().line; consume();
        left = new BinaryExpr(left, parseAddSub(), "&", l);
    }
    return left;
}

ASTNode *Parser::parseAddSub() {
    ASTNode *left = parseMulDiv();
    while ((check(TokenType::OP_PLUS) || check(TokenType::OP_MINUS)) && m_error.isEmpty()) {
        int l = current().line;
        QString op = consume().value;
        left = new BinaryExpr(left, parseMulDiv(), op, l);
    }
    return left;
}

ASTNode *Parser::parseMulDiv() {
    ASTNode *left = parseUnary();
    while ((check(TokenType::OP_STAR)      || check(TokenType::OP_SLASH) ||
            check(TokenType::OP_BACKSLASH) || check(TokenType::KW_MOD)) &&
           m_error.isEmpty()) {
        int l = current().line;
        QString op = consume().value;
        left = new BinaryExpr(left, parseUnary(), op, l);
    }
    return left;
}

ASTNode *Parser::parseUnary() {
    if (check(TokenType::OP_MINUS)) {
        int l = current().line; consume();
        return new UnaryExpr(parsePower(), "-", l);
    }
    if (check(TokenType::OP_PLUS)) {
        consume(); return parsePower();
    }
    return parsePower();
}

ASTNode *Parser::parsePower() {
    ASTNode *base = parsePrimary();
    if (check(TokenType::OP_CARET) && m_error.isEmpty()) {
        int l = current().line; consume();
        return new BinaryExpr(base, parseUnary(), "^", l);
    }
    return base;
}

ASTNode *Parser::parsePrimary() {
    int line = current().line;

    if (check(TokenType::INTEGER)) {
        QVariant v = consume().value.toInt();
        return new LiteralExpr(v, line);
    }
    if (check(TokenType::FLOAT)) {
        QVariant v = consume().value.toDouble();
        return new LiteralExpr(v, line);
    }
    if (check(TokenType::STRING)) {
        QVariant v = consume().value;
        return new LiteralExpr(v, line);
    }
    if (check(TokenType::KW_TRUE))  { consume(); return new LiteralExpr(true,  line); }
    if (check(TokenType::KW_FALSE)) { consume(); return new LiteralExpr(false, line); }
    if (check(TokenType::KW_NULL))  { consume(); return new LiteralExpr(QVariant(), line); }

    if (check(TokenType::IDENTIFIER)) {
        QString name = consume().value;
        if (check(TokenType::LPAREN)) {
            consume(); // (
            auto *ce = new CallExpr(name, line);
            if (!check(TokenType::RPAREN)) {
                ce->args.append(parseExpr());
                while (check(TokenType::COMMA)) {
                    consume();
                    ce->args.append(parseExpr());
                }
            }
            expect(TokenType::RPAREN, "Expected ')'");
            return ce;
        }
        return new VarExpr(name, line);
    }

    if (check(TokenType::LPAREN)) {
        consume();
        ASTNode *e = parseExpr();
        expect(TokenType::RPAREN, "Expected ')'");
        return e;
    }

    setError(QString("Unexpected token '%1' in expression").arg(current().value));
    consume();
    return new LiteralExpr(QVariant(), line);
}
