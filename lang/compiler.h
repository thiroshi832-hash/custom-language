#pragma once
#include "ast.h"
#include "../termrunner/opcodestream.h"
#include <QString>
#include <QStack>

class Compiler {
public:
    Compiler();

    bool compile(Program *prog);
    bool hasError()    const { return !m_error.isEmpty(); }
    QString errorMessage() const { return m_error; }

    OpcodeStream &stream() { return m_stream; }

private:
    OpcodeStream m_stream;
    QString      m_error;

    // Stacks for loop exit jump-patch lists
    QStack<QVector<int>> m_forExitStack;
    QStack<QVector<int>> m_whileExitStack;
    QStack<QVector<int>> m_doExitStack;

    // ── Emit helpers ────────────────────────────────────────────────────
    int  emitOp(Opcode op, int line = 0);
    int  emitOp(Opcode op, const QVariant &v, int line = 0);
    int  emitOp(Opcode op, const QString &s, int line = 0);
    int  emitOp(Opcode op, const QString &s, const QVariant &v, int line = 0);
    void patch(int idx, int target);
    int  pos() const;

    // ── Compile helpers ─────────────────────────────────────────────────
    void compileStatements(const QVector<ASTNode*> &stmts);
    void compileStatement(ASTNode *node);
    void compileDim(DimStmt *n);
    void compileAssign(AssignStmt *n);
    void compileIf(IfStmt *n);
    void compileFor(ForStmt *n);
    void compileWhile(WhileStmt *n);
    void compileDoLoop(DoLoopStmt *n);
    void compilePrint(PrintStmt *n);
    void compileCall(CallStmt *n);
    void compileReturn(ReturnStmt *n);
    void compileExit(ExitStmt *n);
    void compileSub(SubDecl *n);
    void compileFunc(FuncDecl *n);
    void compileExpr(ASTNode *node);

    void setError(const QString &msg, int line = 0);
};
