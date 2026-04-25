#pragma once
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>
#include <QMap>
#include "../lang/opcodes.h"

// ── Instruction ───────────────────────────────────────────────────────────

struct Instruction {
    Opcode   op;
    QString  sOperand; // variable / function name
    QVariant vOperand; // constant value / jump target / argc
    int      line;

    explicit Instruction(Opcode o, int ln = 0)
        : op(o), line(ln) {}
    Instruction(Opcode o, const QVariant &v, int ln = 0)
        : op(o), vOperand(v), line(ln) {}
    Instruction(Opcode o, const QString &s, int ln = 0)
        : op(o), sOperand(s), line(ln) {}
    Instruction(Opcode o, const QString &s, const QVariant &v, int ln = 0)
        : op(o), sOperand(s), vOperand(v), line(ln) {}
};

// ── Function metadata ─────────────────────────────────────────────────────

struct FuncInfo {
    QString     name;
    QStringList params;
    int         startAddress;
    bool        isFunction; // true = returns value
};

// ── OpcodeStream ──────────────────────────────────────────────────────────

class OpcodeStream {
public:
    void append(const Instruction &instr) { m_instructions.append(instr); }
    int  size()                    const { return m_instructions.size(); }
    int  currentPos()              const { return m_instructions.size(); }

    const Instruction &at(int i)   const { return m_instructions.at(i); }
    Instruction       &at(int i)         { return m_instructions[i]; }

    void patch(int index, int target) { m_instructions[index].vOperand = target; }

    void registerFunc(const QString &name, int addr,
                      const QStringList &params, bool isFunc) {
        FuncInfo fi;
        fi.name         = name;
        fi.params       = params;
        fi.startAddress = addr;
        fi.isFunction   = isFunc;
        m_funcTable[name.toLower()] = fi;
    }

    bool             hasFunc(const QString &name) const {
        return m_funcTable.contains(name.toLower());
    }
    FuncInfo funcInfo(const QString &name) const {
        return m_funcTable.value(name.toLower());
    }

    void clear() { m_instructions.clear(); m_funcTable.clear(); }

    const QVector<Instruction>    &instructions() const { return m_instructions; }
    const QMap<QString, FuncInfo> &funcTable()    const { return m_funcTable; }

    QString disassemble() const;

private:
    QVector<Instruction>    m_instructions;
    QMap<QString, FuncInfo> m_funcTable;
};
