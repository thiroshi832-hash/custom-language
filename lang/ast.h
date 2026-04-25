#pragma once
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>

// ── Node kind enum ─────────────────────────────────────────────────────────
enum class NodeKind {
    Program,
    DimStmt, AssignStmt, IfStmt, ForStmt, WhileStmt, DoLoopStmt,
    CallStmt, PrintStmt, ReturnStmt, ExitStmt,
    SubDecl, FuncDecl,
    BinaryExpr, UnaryExpr, LiteralExpr, VarExpr, CallExpr
};

// ── Base ──────────────────────────────────────────────────────────────────
struct ASTNode {
    NodeKind kind;
    int      line;
    ASTNode(NodeKind k, int l) : kind(k), line(l) {}
    virtual ~ASTNode() = default;
};

// ── Top-level program ────────────────────────────────────────────────────
struct Program : ASTNode {
    QVector<ASTNode*> statements;
    Program() : ASTNode(NodeKind::Program, 0) {}
    ~Program() { qDeleteAll(statements); }
};

// ── Statements ───────────────────────────────────────────────────────────
struct DimStmt : ASTNode {
    QString   name;
    ASTNode  *initializer; // may be nullptr
    DimStmt(const QString &n, ASTNode *init, int l)
        : ASTNode(NodeKind::DimStmt, l), name(n), initializer(init) {}
    ~DimStmt() { delete initializer; }
};

struct AssignStmt : ASTNode {
    QString  name;
    ASTNode *value;
    AssignStmt(const QString &n, ASTNode *v, int l)
        : ASTNode(NodeKind::AssignStmt, l), name(n), value(v) {}
    ~AssignStmt() { delete value; }
};

struct IfStmt : ASTNode {
    ASTNode          *condition;
    QVector<ASTNode*> thenBlock;
    QVector<ASTNode*> elseBlock;
    bool              hasElse;
    IfStmt(ASTNode *cond, int l)
        : ASTNode(NodeKind::IfStmt, l), condition(cond), hasElse(false) {}
    ~IfStmt() { delete condition; qDeleteAll(thenBlock); qDeleteAll(elseBlock); }
};

struct ForStmt : ASTNode {
    QString           varName;
    ASTNode          *start;
    ASTNode          *limit;
    ASTNode          *step;  // nullptr → default 1
    QVector<ASTNode*> body;
    ForStmt(const QString &v, ASTNode *s, ASTNode *l, ASTNode *st, int ln)
        : ASTNode(NodeKind::ForStmt, ln), varName(v), start(s), limit(l), step(st) {}
    ~ForStmt() { delete start; delete limit; delete step; qDeleteAll(body); }
};

struct WhileStmt : ASTNode {
    ASTNode          *condition;
    QVector<ASTNode*> body;
    WhileStmt(ASTNode *cond, int l)
        : ASTNode(NodeKind::WhileStmt, l), condition(cond) {}
    ~WhileStmt() { delete condition; qDeleteAll(body); }
};

struct DoLoopStmt : ASTNode {
    ASTNode          *condition;  // may be nullptr (infinite)
    QVector<ASTNode*> body;
    bool              isUntil;   // Do Until vs Do While
    bool              condAtTop; // condition checked at top or bottom
    DoLoopStmt(int l)
        : ASTNode(NodeKind::DoLoopStmt, l),
          condition(nullptr), isUntil(false), condAtTop(true) {}
    ~DoLoopStmt() { delete condition; qDeleteAll(body); }
};

struct PrintStmt : ASTNode {
    QVector<ASTNode*> args;
    explicit PrintStmt(int l) : ASTNode(NodeKind::PrintStmt, l) {}
    ~PrintStmt() { qDeleteAll(args); }
};

struct CallStmt : ASTNode {
    QString           name;
    QVector<ASTNode*> args;
    CallStmt(const QString &n, int l) : ASTNode(NodeKind::CallStmt, l), name(n) {}
    ~CallStmt() { qDeleteAll(args); }
};

struct ReturnStmt : ASTNode {
    ASTNode *value; // nullptr for Sub return
    ReturnStmt(ASTNode *v, int l) : ASTNode(NodeKind::ReturnStmt, l), value(v) {}
    ~ReturnStmt() { delete value; }
};

enum class ExitTarget { Sub, Function, For, While, Do };
struct ExitStmt : ASTNode {
    ExitTarget target;
    ExitStmt(ExitTarget t, int l) : ASTNode(NodeKind::ExitStmt, l), target(t) {}
};

// ── Declarations ─────────────────────────────────────────────────────────
struct SubDecl : ASTNode {
    QString           name;
    QStringList       params;
    QVector<ASTNode*> body;
    SubDecl(const QString &n, int l) : ASTNode(NodeKind::SubDecl, l), name(n) {}
    ~SubDecl() { qDeleteAll(body); }
};

struct FuncDecl : ASTNode {
    QString           name;
    QStringList       params;
    QVector<ASTNode*> body;
    FuncDecl(const QString &n, int l) : ASTNode(NodeKind::FuncDecl, l), name(n) {}
    ~FuncDecl() { qDeleteAll(body); }
};

// ── Expressions ──────────────────────────────────────────────────────────
struct BinaryExpr : ASTNode {
    ASTNode *left, *right;
    QString  op;
    BinaryExpr(ASTNode *l, ASTNode *r, const QString &o, int ln)
        : ASTNode(NodeKind::BinaryExpr, ln), left(l), right(r), op(o) {}
    ~BinaryExpr() { delete left; delete right; }
};

struct UnaryExpr : ASTNode {
    ASTNode *operand;
    QString  op;
    UnaryExpr(ASTNode *o, const QString &op_, int l)
        : ASTNode(NodeKind::UnaryExpr, l), operand(o), op(op_) {}
    ~UnaryExpr() { delete operand; }
};

struct LiteralExpr : ASTNode {
    QVariant value;
    LiteralExpr(const QVariant &v, int l)
        : ASTNode(NodeKind::LiteralExpr, l), value(v) {}
};

struct VarExpr : ASTNode {
    QString name;
    VarExpr(const QString &n, int l) : ASTNode(NodeKind::VarExpr, l), name(n) {}
};

struct CallExpr : ASTNode {
    QString           name;
    QVector<ASTNode*> args;
    CallExpr(const QString &n, int l) : ASTNode(NodeKind::CallExpr, l), name(n) {}
    ~CallExpr() { qDeleteAll(args); }
};
