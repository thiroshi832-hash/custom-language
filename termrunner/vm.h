#pragma once
#include "opcodestream.h"
#include "formruntime.h"
#include <QObject>
#include <QStack>
#include <QHash>
#include <QVector>
#include <QVariant>
#include <QString>
#include <QEventLoop>

// ── Call frame ────────────────────────────────────────────────────────────

struct CallFrame {
    int                      returnAddress;
    QHash<QString, QVariant> locals;
    QString                  funcName;
    bool                     isFunction;
};

// ── VM ────────────────────────────────────────────────────────────────────

class VM : public QObject {
    Q_OBJECT
public:
    explicit VM(QObject *parent = nullptr);

    void setStream(const OpcodeStream *stream);
    void setFormRuntime(FormRuntime *fr) { m_formRuntime = fr; }
    void run();
    void reset();

    bool    hasError()     const { return !m_error.isEmpty(); }
    QString errorMessage() const { return m_error; }

signals:
    void output(const QString &text);
    void errorOccurred(const QString &msg);
    void executionFinished();

private slots:
    void onEventFired(const QString &handlerName);
    void onFormClosed();

private:
    const OpcodeStream      *m_stream      { nullptr };
    int                      m_pc          { 0 };
    QStack<QVariant>         m_stack;
    QVector<CallFrame>       m_callStack;
    QHash<QString, QVariant> m_globals;
    QString                  m_error;
    FormRuntime             *m_formRuntime { nullptr };
    QEventLoop              *m_eventLoop   { nullptr };

    // ── Stack helpers ────────────────────────────────────────────────────
    void     push(const QVariant &v);
    QVariant pop();
    QVariant top() const;

    // ── Variable access ──────────────────────────────────────────────────
    QVariant loadVar(const QString &name) const;
    void     storeVar(const QString &name, const QVariant &val);
    void     declareVar(const QString &name);

    // ── Arithmetic helpers ───────────────────────────────────────────────
    QVariant doAdd(const QVariant &a, const QVariant &b);
    QVariant doSub(const QVariant &a, const QVariant &b);
    QVariant doMul(const QVariant &a, const QVariant &b);
    QVariant doDiv(const QVariant &a, const QVariant &b);
    QVariant doIntDiv(const QVariant &a, const QVariant &b);
    QVariant doMod(const QVariant &a, const QVariant &b);
    QVariant doPow(const QVariant &a, const QVariant &b);
    QVariant doConcat(const QVariant &a, const QVariant &b);
    bool     toBool(const QVariant &v) const;
    double   toNum(const QVariant &v)  const;

    // ── Built-in dispatch ────────────────────────────────────────────────
    bool callBuiltin(const QString &name, int argc);

    // ── Execution helpers ────────────────────────────────────────────────
    void stepInstruction();
    void executeSubByName(const QString &name);

    void setError(const QString &msg, int line = 0);
};
