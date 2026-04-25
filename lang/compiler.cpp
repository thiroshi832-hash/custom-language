#include "compiler.h"
#include <cmath>

Compiler::Compiler() {}

// ── Emit helpers ─────────────────────────────────────────────────────────

int Compiler::emitOp(Opcode op, int line) {
    m_stream.append(Instruction(op, line)); return m_stream.size() - 1;
}
int Compiler::emitOp(Opcode op, const QVariant &v, int line) {
    m_stream.append(Instruction(op, v, line)); return m_stream.size() - 1;
}
int Compiler::emitOp(Opcode op, const QString &s, int line) {
    m_stream.append(Instruction(op, s, line)); return m_stream.size() - 1;
}
int Compiler::emitOp(Opcode op, const QString &s, const QVariant &v, int line) {
    m_stream.append(Instruction(op, s, v, line)); return m_stream.size() - 1;
}
void Compiler::patch(int idx, int target) { m_stream.patch(idx, target); }
int  Compiler::pos()  const { return m_stream.size(); }

void Compiler::setError(const QString &msg, int line) {
    if (m_error.isEmpty())
        m_error = (line > 0) ? QString("Line %1: %2").arg(line).arg(msg) : msg;
}

// ── Top-level compile ────────────────────────────────────────────────────

bool Compiler::compile(Program *prog) {
    m_stream.clear();
    m_error.clear();

    // First pass: main body (skip Sub/Function declarations)
    for (ASTNode *node : prog->statements) {
        if (node->kind != NodeKind::SubDecl && node->kind != NodeKind::FuncDecl) {
            compileStatement(node);
        }
    }
    emitOp(Opcode::HALT);

    // Second pass: compile all Sub/Function bodies
    for (ASTNode *node : prog->statements) {
        if (node->kind == NodeKind::SubDecl)
            compileSub(static_cast<SubDecl*>(node));
        else if (node->kind == NodeKind::FuncDecl)
            compileFunc(static_cast<FuncDecl*>(node));
    }

    return !hasError();
}

void Compiler::compileStatements(const QVector<ASTNode*> &stmts) {
    for (ASTNode *s : stmts) {
        compileStatement(s);
        if (hasError()) return;
    }
}

void Compiler::compileStatement(ASTNode *node) {
    if (!node || hasError()) return;
    switch (node->kind) {
    case NodeKind::DimStmt:    compileDim   (static_cast<DimStmt*>(node));    break;
    case NodeKind::AssignStmt: compileAssign(static_cast<AssignStmt*>(node)); break;
    case NodeKind::IfStmt:     compileIf    (static_cast<IfStmt*>(node));     break;
    case NodeKind::ForStmt:    compileFor   (static_cast<ForStmt*>(node));    break;
    case NodeKind::WhileStmt:  compileWhile (static_cast<WhileStmt*>(node));  break;
    case NodeKind::DoLoopStmt: compileDoLoop(static_cast<DoLoopStmt*>(node)); break;
    case NodeKind::PrintStmt:  compilePrint (static_cast<PrintStmt*>(node));  break;
    case NodeKind::CallStmt:   compileCall  (static_cast<CallStmt*>(node));   break;
    case NodeKind::ReturnStmt: compileReturn(static_cast<ReturnStmt*>(node)); break;
    case NodeKind::ExitStmt:   compileExit  (static_cast<ExitStmt*>(node));   break;
    case NodeKind::SubDecl:
    case NodeKind::FuncDecl:
        break; // handled in second pass
    default:
        // Expression-as-statement (rare, pop result)
        compileExpr(node);
        emitOp(Opcode::POP, node->line);
        break;
    }
}

// ── Dim ───────────────────────────────────────────────────────────────────

void Compiler::compileDim(DimStmt *n) {
    if (n->initializer) {
        compileExpr(n->initializer);
        emitOp(Opcode::STORE_VAR, n->name, n->line);
    }
    emitOp(Opcode::DECLARE_VAR, n->name, n->line);
}

// ── Assign ────────────────────────────────────────────────────────────────

void Compiler::compileAssign(AssignStmt *n) {
    compileExpr(n->value);
    emitOp(Opcode::STORE_VAR, n->name, n->line);
}

// ── If ────────────────────────────────────────────────────────────────────

void Compiler::compileIf(IfStmt *n) {
    compileExpr(n->condition);
    int jumpFalse = emitOp(Opcode::JUMP_IF_FALSE, QVariant(-1), n->line);

    compileStatements(n->thenBlock);

    if (n->hasElse) {
        int jumpEnd = emitOp(Opcode::JUMP, QVariant(-1), n->line);
        patch(jumpFalse, pos());
        compileStatements(n->elseBlock);
        patch(jumpEnd, pos());
    } else {
        patch(jumpFalse, pos());
    }
}

// ── For ───────────────────────────────────────────────────────────────────

void Compiler::compileFor(ForStmt *n) {
    // Initialise loop variable
    compileExpr(n->start);
    emitOp(Opcode::STORE_VAR, n->varName, n->line);

    // Store limit and step in hidden variables
    QString limitVar = QString("__lim_%1").arg(n->varName);
    QString stepVar  = QString("__stp_%1").arg(n->varName);

    compileExpr(n->limit);
    emitOp(Opcode::STORE_VAR, limitVar, n->line);

    if (n->step) {
        compileExpr(n->step);
    } else {
        emitOp(Opcode::PUSH_CONST, QVariant(1), n->line);
    }
    emitOp(Opcode::STORE_VAR, stepVar, n->line);

    // Loop top
    int loopTop = pos();

    // Check step sign first (stack stays clean — JUMP_IF_FALSE consumes the bool)
    emitOp(Opcode::LOAD_VAR, stepVar,  n->line);
    emitOp(Opcode::PUSH_CONST, QVariant(0), n->line);
    emitOp(Opcode::CMP_GTE, n->line);                               // step >= 0 ?
    int jumpNegStep = emitOp(Opcode::JUMP_IF_FALSE, QVariant(-1), n->line);

    // Positive step: exit if i > limit
    // VM CMP_GT: b = pop() = limit, a = pop() = i  →  result = (i > limit)
    emitOp(Opcode::LOAD_VAR, n->varName, n->line);                  // push i
    emitOp(Opcode::LOAD_VAR, limitVar,   n->line);                  // push limit
    emitOp(Opcode::CMP_GT,   n->line);                              // i > limit ?
    int jumpEndPos = emitOp(Opcode::JUMP_IF_TRUE, QVariant(-1), n->line);
    int jumpBody   = emitOp(Opcode::JUMP, QVariant(-1), n->line);

    // Negative step: exit if i < limit
    patch(jumpNegStep, pos());
    emitOp(Opcode::LOAD_VAR, n->varName, n->line);                  // push i
    emitOp(Opcode::LOAD_VAR, limitVar,   n->line);                  // push limit
    emitOp(Opcode::CMP_LT,   n->line);                              // i < limit ?
    int jumpEndNeg = emitOp(Opcode::JUMP_IF_TRUE, QVariant(-1), n->line);

    patch(jumpBody, pos());

    // Body
    m_forExitStack.push(QVector<int>());
    compileStatements(n->body);
    QVector<int> exits = m_forExitStack.pop();

    // Step
    emitOp(Opcode::LOAD_VAR, n->varName, n->line);
    emitOp(Opcode::LOAD_VAR, stepVar,    n->line);
    emitOp(Opcode::ADD, n->line);
    emitOp(Opcode::STORE_VAR, n->varName, n->line);
    emitOp(Opcode::JUMP, QVariant(loopTop), n->line);

    // Patch exits
    int loopEnd = pos();
    patch(jumpEndPos, loopEnd);
    patch(jumpEndNeg, loopEnd);
    for (int idx : exits) patch(idx, loopEnd);
}

// ── While ─────────────────────────────────────────────────────────────────

void Compiler::compileWhile(WhileStmt *n) {
    int loopTop = pos();
    compileExpr(n->condition);
    int jumpEnd = emitOp(Opcode::JUMP_IF_FALSE, QVariant(-1), n->line);

    m_whileExitStack.push(QVector<int>());
    compileStatements(n->body);
    QVector<int> exits = m_whileExitStack.pop();

    emitOp(Opcode::JUMP, QVariant(loopTop), n->line);

    int loopEnd = pos();
    patch(jumpEnd, loopEnd);
    for (int idx : exits) patch(idx, loopEnd);
}

// ── Do Loop ───────────────────────────────────────────────────────────────

void Compiler::compileDoLoop(DoLoopStmt *n) {
    int loopTop = pos();
    int jumpEnd = -1;

    if (n->condition && n->condAtTop) {
        compileExpr(n->condition);
        if (n->isUntil) {
            jumpEnd = emitOp(Opcode::JUMP_IF_TRUE, QVariant(-1), n->line);
        } else {
            jumpEnd = emitOp(Opcode::JUMP_IF_FALSE, QVariant(-1), n->line);
        }
    }

    m_doExitStack.push(QVector<int>());
    compileStatements(n->body);
    QVector<int> exits = m_doExitStack.pop();

    if (n->condition && !n->condAtTop) {
        compileExpr(n->condition);
        if (n->isUntil) {
            emitOp(Opcode::JUMP_IF_FALSE, QVariant(loopTop), n->line);
        } else {
            emitOp(Opcode::JUMP_IF_TRUE, QVariant(loopTop), n->line);
        }
    } else {
        emitOp(Opcode::JUMP, QVariant(loopTop), n->line);
    }

    int loopEnd = pos();
    if (jumpEnd >= 0) patch(jumpEnd, loopEnd);
    for (int idx : exits) patch(idx, loopEnd);
}

// ── Print ─────────────────────────────────────────────────────────────────

void Compiler::compilePrint(PrintStmt *n) {
    for (ASTNode *arg : n->args)
        compileExpr(arg);
    emitOp(Opcode::CALL_BUILTIN, "Print", QVariant(n->args.size()), n->line);
}

// ── Call statement ────────────────────────────────────────────────────────

void Compiler::compileCall(CallStmt *n) {
    for (ASTNode *arg : n->args)
        compileExpr(arg);
    // Try builtin first (resolved at VM runtime)
    emitOp(Opcode::CALL, n->name, QVariant(n->args.size()), n->line);
}

// ── Return ────────────────────────────────────────────────────────────────

void Compiler::compileReturn(ReturnStmt *n) {
    if (n->value) {
        compileExpr(n->value);
        emitOp(Opcode::RETURN, QVariant(1), n->line);
    } else {
        emitOp(Opcode::RETURN, QVariant(0), n->line);
    }
}

// ── Exit ─────────────────────────────────────────────────────────────────

void Compiler::compileExit(ExitStmt *n) {
    int jumpIdx = emitOp(Opcode::JUMP, QVariant(-1), n->line);
    switch (n->target) {
    case ExitTarget::For:
        if (!m_forExitStack.isEmpty())
            m_forExitStack.top().append(jumpIdx);
        break;
    case ExitTarget::While:
        if (!m_whileExitStack.isEmpty())
            m_whileExitStack.top().append(jumpIdx);
        break;
    case ExitTarget::Do:
        if (!m_doExitStack.isEmpty())
            m_doExitStack.top().append(jumpIdx);
        break;
    case ExitTarget::Sub:
    case ExitTarget::Function:
        emitOp(Opcode::RETURN, QVariant(0), n->line);
        break;
    }
}

// ── Sub / Function compilation ───────────────────────────────────────────

void Compiler::compileSub(SubDecl *n) {
    int startPos = pos();
    m_stream.registerFunc(n->name, startPos, n->params, false);

    // Bind parameters: they are pushed left-to-right before CALL,
    // so we store them in reverse order into locals
    for (int i = n->params.size() - 1; i >= 0; --i) {
        emitOp(Opcode::STORE_VAR, n->params[i], n->line);
    }

    compileStatements(n->body);
    emitOp(Opcode::RETURN, QVariant(0), n->line);
}

void Compiler::compileFunc(FuncDecl *n) {
    int startPos = pos();
    m_stream.registerFunc(n->name, startPos, n->params, true);

    for (int i = n->params.size() - 1; i >= 0; --i) {
        emitOp(Opcode::STORE_VAR, n->params[i], n->line);
    }

    compileStatements(n->body);
    emitOp(Opcode::PUSH_NULL, n->line); // default return value
    emitOp(Opcode::RETURN, QVariant(1), n->line);
}

// ── Expressions ──────────────────────────────────────────────────────────

void Compiler::compileExpr(ASTNode *node) {
    if (!node || hasError()) return;

    switch (node->kind) {
    case NodeKind::LiteralExpr: {
        auto *n = static_cast<LiteralExpr*>(node);
        if (n->value.isNull())
            emitOp(Opcode::PUSH_NULL, n->line);
        else
            emitOp(Opcode::PUSH_CONST, n->value, n->line);
        break;
    }
    case NodeKind::VarExpr: {
        auto *n = static_cast<VarExpr*>(node);
        emitOp(Opcode::LOAD_VAR, n->name, n->line);
        break;
    }
    case NodeKind::CallExpr: {
        auto *n = static_cast<CallExpr*>(node);
        for (ASTNode *arg : n->args) compileExpr(arg);
        emitOp(Opcode::CALL, n->name, QVariant(n->args.size()), n->line);
        break;
    }
    case NodeKind::UnaryExpr: {
        auto *n = static_cast<UnaryExpr*>(node);
        compileExpr(n->operand);
        if      (n->op == "-")   emitOp(Opcode::NEG,     n->line);
        else if (n->op == "Not") emitOp(Opcode::LOG_NOT, n->line);
        break;
    }
    case NodeKind::BinaryExpr: {
        auto *n = static_cast<BinaryExpr*>(node);
        compileExpr(n->left);
        compileExpr(n->right);
        if      (n->op == "+")  emitOp(Opcode::ADD,     n->line);
        else if (n->op == "-")  emitOp(Opcode::SUB,     n->line);
        else if (n->op == "*")  emitOp(Opcode::MUL,     n->line);
        else if (n->op == "/")  emitOp(Opcode::DIV,     n->line);
        else if (n->op == "\\") emitOp(Opcode::INT_DIV, n->line);
        else if (n->op == "Mod"|| n->op == "mod") emitOp(Opcode::MOD, n->line);
        else if (n->op == "^")  emitOp(Opcode::POW,     n->line);
        else if (n->op == "&")  emitOp(Opcode::CONCAT,  n->line);
        else if (n->op == "=")  emitOp(Opcode::CMP_EQ,  n->line);
        else if (n->op == "<>") emitOp(Opcode::CMP_NEQ, n->line);
        else if (n->op == "<")  emitOp(Opcode::CMP_LT,  n->line);
        else if (n->op == ">")  emitOp(Opcode::CMP_GT,  n->line);
        else if (n->op == "<=") emitOp(Opcode::CMP_LTE, n->line);
        else if (n->op == ">=") emitOp(Opcode::CMP_GTE, n->line);
        else if (n->op == "And"||n->op=="and") emitOp(Opcode::LOG_AND, n->line);
        else if (n->op == "Or" ||n->op=="or")  emitOp(Opcode::LOG_OR,  n->line);
        break;
    }
    default:
        setError("Cannot compile expression node", node->line);
        break;
    }
}
