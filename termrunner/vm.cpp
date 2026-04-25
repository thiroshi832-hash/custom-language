#include "vm.h"
#include "formruntime.h"
#include <QtMath>
#include <QDateTime>
#include <QMetaObject>
#include <cmath>

VM::VM(QObject *parent)
    : QObject(parent)
    , m_stream(nullptr), m_pc(0)
    , m_formRuntime(nullptr), m_eventLoop(nullptr)
{}

void VM::setStream(const OpcodeStream *stream) { m_stream = stream; }

void VM::reset() {
    m_pc = 0;
    while (!m_stack.isEmpty()) m_stack.pop();
    m_callStack.clear();
    m_globals.clear();
    m_error.clear();
}

// ── Stack helpers ─────────────────────────────────────────────────────────

void VM::push(const QVariant &v) { m_stack.push(v); }

QVariant VM::pop() {
    if (m_stack.isEmpty()) { setError("Stack underflow"); return QVariant(); }
    return m_stack.pop();
}

QVariant VM::top() const {
    return m_stack.isEmpty() ? QVariant() : m_stack.top();
}

// ── Variable helpers ──────────────────────────────────────────────────────

QVariant VM::loadVar(const QString &name) const {
    QString key = name.toLower();
    if (!m_callStack.isEmpty()) {
        const auto &locals = m_callStack.last().locals;
        if (locals.contains(key)) return locals[key];
    }
    if (m_globals.contains(key)) return m_globals[key];
    return QVariant();
}

void VM::storeVar(const QString &name, const QVariant &val) {
    QString key = name.toLower();
    if (!m_callStack.isEmpty()) {
        m_callStack.last().locals[key] = val;
    } else {
        m_globals[key] = val;
    }
}

void VM::declareVar(const QString &name) {
    QString key = name.toLower();
    if (!m_callStack.isEmpty()) {
        if (!m_callStack.last().locals.contains(key))
            m_callStack.last().locals[key] = QVariant();
    } else {
        if (!m_globals.contains(key))
            m_globals[key] = QVariant();
    }
}

// ── Type helpers ──────────────────────────────────────────────────────────

bool VM::toBool(const QVariant &v) const {
    if (!v.isValid() || v.isNull()) return false;
    if (v.type() == QVariant::Bool)   return v.toBool();
    if (v.type() == QVariant::Int)    return v.toInt() != 0;
    if (v.type() == QVariant::Double) return v.toDouble() != 0.0;
    if (v.type() == QVariant::String) return !v.toString().isEmpty();
    return v.toBool();
}

double VM::toNum(const QVariant &v) const {
    if (!v.isValid() || v.isNull()) return 0.0;
    if (v.type() == QVariant::Bool) return v.toBool() ? 1.0 : 0.0;
    bool ok; double d = v.toDouble(&ok);
    return ok ? d : 0.0;
}

// ── Arithmetic helpers ────────────────────────────────────────────────────

QVariant VM::doAdd(const QVariant &a, const QVariant &b) {
    if (a.type() == QVariant::String || b.type() == QVariant::String)
        return doConcat(a, b);
    double da = toNum(a), db = toNum(b), result = da + db;
    if (a.type() == QVariant::Int && b.type() == QVariant::Int && result == (int)result)
        return QVariant((int)result);
    return QVariant(result);
}
QVariant VM::doSub(const QVariant &a, const QVariant &b) {
    double r = toNum(a) - toNum(b);
    return (r == (int)r) ? QVariant((int)r) : QVariant(r);
}
QVariant VM::doMul(const QVariant &a, const QVariant &b) {
    double r = toNum(a) * toNum(b);
    return (r == (int)r) ? QVariant((int)r) : QVariant(r);
}
QVariant VM::doDiv(const QVariant &a, const QVariant &b) {
    double db = toNum(b);
    if (db == 0.0) { setError("Division by zero"); return QVariant(); }
    return QVariant(toNum(a) / db);
}
QVariant VM::doIntDiv(const QVariant &a, const QVariant &b) {
    int ib = (int)toNum(b);
    if (ib == 0) { setError("Division by zero"); return QVariant(); }
    return QVariant((int)toNum(a) / ib);
}
QVariant VM::doMod(const QVariant &a, const QVariant &b) {
    double db = toNum(b);
    if (db == 0.0) { setError("Mod by zero"); return QVariant(); }
    return QVariant(std::fmod(toNum(a), db));
}
QVariant VM::doPow(const QVariant &a, const QVariant &b) {
    return QVariant(std::pow(toNum(a), toNum(b)));
}
QVariant VM::doConcat(const QVariant &a, const QVariant &b) {
    return QVariant(a.toString() + b.toString());
}

// ── Error ─────────────────────────────────────────────────────────────────

void VM::setError(const QString &msg, int line) {
    if (m_error.isEmpty()) {
        m_error = (line > 0)
            ? QString("Runtime error at line %1: %2").arg(line).arg(msg)
            : QString("Runtime error: %1").arg(msg);
    }
}

// ── Built-in functions ────────────────────────────────────────────────────

bool VM::callBuiltin(const QString &name, int argc) {
    QString n = name.toLower();

    // Collect args pushed left-to-right → pop in reverse
    QVector<QVariant> args(argc);
    for (int i = argc - 1; i >= 0; --i) args[i] = pop();

    // ── Form built-ins ────────────────────────────────────────────────────

    // ShowForm ["title" [, w [, h]]]
    // Initialises the form (or updates title/size) then shows it.
    // The VM's run() method will enter a QEventLoop after the main body.
    if (n == "showform") {
        QString title = (argc > 0) ? args[0].toString() : "Form";
        int w = (argc > 1) ? args[1].toInt() : 640;
        int h = (argc > 2) ? args[2].toInt() : 480;
        if (m_formRuntime) {
            QMetaObject::invokeMethod(m_formRuntime, "initForm",
                Qt::BlockingQueuedConnection,
                Q_ARG(QString, title), Q_ARG(int, w), Q_ARG(int, h));
            QMetaObject::invokeMethod(m_formRuntime, "showForm",
                Qt::BlockingQueuedConnection);
        }
        return true; // no push — statement-only built-in
    }

    // id = CreateControl(type, name, x, y, w, h)
    if (n == "createcontrol") {
        if (argc < 6) { setError("CreateControl requires 6 arguments"); return false; }
        QString type = args[0].toString();
        QString cname = args[1].toString();
        int x = args[2].toInt(), y = args[3].toInt();
        int w = args[4].toInt(), h = args[5].toInt();
        int id = -1;
        if (m_formRuntime) {
            QMetaObject::invokeMethod(m_formRuntime, "createControl",
                Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(int, id),
                Q_ARG(QString, type), Q_ARG(QString, cname),
                Q_ARG(int, x), Q_ARG(int, y), Q_ARG(int, w), Q_ARG(int, h));
        }
        push(QVariant(id));
        return true;
    }

    // SetProperty id, "key", "value"
    if (n == "setproperty") {
        if (argc < 3) { setError("SetProperty requires 3 arguments"); return false; }
        int id        = args[0].toInt();
        QString key   = args[1].toString();
        QString val   = args[2].toString();
        if (m_formRuntime) {
            QMetaObject::invokeMethod(m_formRuntime, "setProperty",
                Qt::BlockingQueuedConnection,
                Q_ARG(int, id), Q_ARG(QString, key), Q_ARG(QString, val));
        }
        return true; // no push
    }

    // AddItem id, "value"
    if (n == "additem") {
        if (argc < 2) { setError("AddItem requires 2 arguments"); return false; }
        int id      = args[0].toInt();
        QString val = args[1].toString();
        if (m_formRuntime) {
            QMetaObject::invokeMethod(m_formRuntime, "addItem",
                Qt::BlockingQueuedConnection,
                Q_ARG(int, id), Q_ARG(QString, val));
        }
        return true; // no push
    }

    // InitForm "title", w, h  (explicit form init without showing)
    if (n == "initform") {
        QString title = (argc > 0) ? args[0].toString() : "Form";
        int w = (argc > 1) ? args[1].toInt() : 640;
        int h = (argc > 2) ? args[2].toInt() : 480;
        if (m_formRuntime) {
            QMetaObject::invokeMethod(m_formRuntime, "initForm",
                Qt::BlockingQueuedConnection,
                Q_ARG(QString, title), Q_ARG(int, w), Q_ARG(int, h));
        }
        return true; // no push
    }

    // ── Print ──────────────────────────────────────────────────────────────
    if (n == "print") {
        QStringList parts;
        for (const QVariant &v : args) parts << v.toString();
        emit output(parts.join(" "));
        return true;
    }

    // ── String functions ──────────────────────────────────────────────────
    if (n == "len") {
        if (argc < 1) { setError("Len() requires 1 argument"); return false; }
        push(QVariant(args[0].toString().length())); return true;
    }
    if (n == "left") {
        if (argc < 2) { setError("Left() requires 2 arguments"); return false; }
        push(QVariant(args[0].toString().left(args[1].toInt()))); return true;
    }
    if (n == "right") {
        if (argc < 2) { setError("Right() requires 2 arguments"); return false; }
        push(QVariant(args[0].toString().right(args[1].toInt()))); return true;
    }
    if (n == "mid") {
        if (argc < 2) { setError("Mid() requires at least 2 arguments"); return false; }
        QString s = args[0].toString();
        int start = args[1].toInt() - 1;
        if (start < 0) start = 0;
        push(QVariant(argc >= 3 ? s.mid(start, args[2].toInt()) : s.mid(start)));
        return true;
    }
    if (n == "ucase") { push(QVariant(args[0].toString().toUpper())); return true; }
    if (n == "lcase") { push(QVariant(args[0].toString().toLower())); return true; }
    if (n == "trim")  { push(QVariant(args[0].toString().trimmed())); return true; }
    if (n == "ltrim") {
        QString s = args[0].toString(); int i = 0;
        while (i < s.length() && s[i] == ' ') i++;
        push(QVariant(s.mid(i))); return true;
    }
    if (n == "rtrim") {
        QString s = args[0].toString(); int i = s.length();
        while (i > 0 && s[i-1] == ' ') i--;
        push(QVariant(s.left(i))); return true;
    }
    if (n == "instr") {
        if (argc < 2) { setError("InStr() requires at least 2 arguments"); return false; }
        int startIdx = 0; QString haystack, needle;
        if (argc >= 3) { startIdx = args[0].toInt()-1; haystack = args[1].toString(); needle = args[2].toString(); }
        else           { haystack = args[0].toString(); needle   = args[1].toString(); }
        int pos = haystack.indexOf(needle, startIdx, Qt::CaseInsensitive);
        push(QVariant(pos >= 0 ? pos+1 : 0)); return true;
    }
    if (n == "replace") {
        if (argc < 3) { setError("Replace() requires 3 arguments"); return false; }
        push(QVariant(args[0].toString().replace(args[1].toString(), args[2].toString())));
        return true;
    }
    if (n == "string") {
        if (argc < 2) { setError("String() requires 2 arguments"); return false; }
        push(QVariant(QString(args[0].toInt(),
            args[1].toString().isEmpty() ? QChar(' ') : args[1].toString()[0])));
        return true;
    }
    if (n == "space") {
        if (argc < 1) { setError("Space() requires 1 argument"); return false; }
        push(QVariant(QString(args[0].toInt(), ' '))); return true;
    }
    if (n == "strreverse") {
        if (argc < 1) { setError("StrReverse() requires 1 argument"); return false; }
        QString s = args[0].toString(); std::reverse(s.begin(), s.end());
        push(QVariant(s)); return true;
    }

    // ── Math ──────────────────────────────────────────────────────────────
    if (n == "abs")  { push(QVariant(std::abs(toNum(args[0])))); return true; }
    if (n == "int")  { push(QVariant((int)std::floor(toNum(args[0])))); return true; }
    if (n == "fix")  {
        double d = toNum(args[0]);
        push(QVariant((int)(d>=0 ? std::floor(d) : std::ceil(d)))); return true;
    }
    if (n == "sgn")  {
        double d = toNum(args[0]);
        push(QVariant(d>0 ? 1 : d<0 ? -1 : 0)); return true;
    }
    if (n == "sqr")  {
        double d = toNum(args[0]);
        if (d<0) { setError("Sqr() of negative number"); return false; }
        push(QVariant(std::sqrt(d))); return true;
    }
    if (n == "log")  {
        double d = toNum(args[0]);
        if (d<=0) { setError("Log() of non-positive number"); return false; }
        push(QVariant(std::log(d))); return true;
    }
    if (n == "exp")  { push(QVariant(std::exp(toNum(args[0])))); return true; }
    if (n == "sin")  { push(QVariant(std::sin(toNum(args[0])))); return true; }
    if (n == "cos")  { push(QVariant(std::cos(toNum(args[0])))); return true; }
    if (n == "tan")  { push(QVariant(std::tan(toNum(args[0])))); return true; }
    if (n == "atn")  { push(QVariant(std::atan(toNum(args[0])))); return true; }
    if (n == "rnd")  { push(QVariant((double)qrand() / RAND_MAX)); return true; }
    if (n == "randomize") {
        qsrand(argc > 0 ? (uint)toNum(args[0])
                        : (uint)QDateTime::currentMSecsSinceEpoch());
        push(QVariant()); return true;
    }

    // ── Conversion ────────────────────────────────────────────────────────
    if (n == "cstr")  { push(QVariant(args[0].toString())); return true; }
    if (n == "cint")  { push(QVariant((int)toNum(args[0]))); return true; }
    if (n == "clng")  { push(QVariant((int)toNum(args[0]))); return true; }
    if (n == "cdbl")  { push(QVariant(toNum(args[0]))); return true; }
    if (n == "cbool") { push(QVariant(toBool(args[0]))); return true; }
    if (n == "val") {
        bool ok; double d = args[0].toString().toDouble(&ok);
        push(QVariant(ok ? d : 0.0)); return true;
    }
    if (n == "str") {
        push(QVariant(QString::number(toNum(args[0])))); return true;
    }

    // ── Type checks ───────────────────────────────────────────────────────
    if (n == "isnumeric") {
        bool ok; args[0].toString().toDouble(&ok);
        push(QVariant(ok)); return true;
    }
    if (n == "isnull")  { push(QVariant(args[0].isNull())); return true; }
    if (n == "isempty") { push(QVariant(!args[0].isValid() || args[0].isNull())); return true; }
    if (n == "typename") {
        QString t;
        switch (args[0].type()) {
        case QVariant::Int:    t = "Integer"; break;
        case QVariant::Double: t = "Double";  break;
        case QVariant::String: t = "String";  break;
        case QVariant::Bool:   t = "Boolean"; break;
        default: t = args[0].isNull() ? "Null" : "Variant"; break;
        }
        push(QVariant(t)); return true;
    }

    return false; // unknown built-in
}

// ── Debug pause ───────────────────────────────────────────────────────────

void VM::checkDebugPause(int line) {
    if (!m_debugMode || line <= 0 || m_shouldStop) return;

    bool shouldPause = false;
    if (m_stepMode && line != m_lastStepLine)
        shouldPause = true;
    else if (m_breakpoints.contains(line))
        shouldPause = true;

    if (!shouldPause) return;

    m_lastStepLine = line;
    m_stepMode = false; // reset; debugStep() re-arms it

    // Snapshot variable state for the watch panel
    QVariantMap locals, globals;
    if (!m_callStack.isEmpty()) {
        for (auto it = m_callStack.last().locals.constBegin();
             it != m_callStack.last().locals.constEnd(); ++it)
            locals[it.key()] = it.value();
    }
    for (auto it = m_globals.constBegin(); it != m_globals.constEnd(); ++it)
        globals[it.key()] = it.value();

    // Enter pause — blocks until debugContinue/debugStep/debugStop is called
    m_debugLoop = new QEventLoop(this);
    emit pausedAt(line, locals, globals);
    m_debugLoop->exec();
    delete m_debugLoop;
    m_debugLoop = nullptr;
}

void VM::debugContinue() {
    m_stepMode = false;
    if (m_debugLoop) m_debugLoop->quit();
}

void VM::debugStep() {
    m_stepMode = true;
    if (m_debugLoop) m_debugLoop->quit();
}

void VM::debugStop() {
    m_shouldStop = true;
    if (m_debugLoop) m_debugLoop->quit();
    if (m_eventLoop) m_eventLoop->quit();
}

// ── Single instruction step ───────────────────────────────────────────────

void VM::stepInstruction() {
    const Instruction &ins = m_stream->at(m_pc);
    checkDebugPause(ins.line);
    if (m_shouldStop) { m_pc = m_stream->size(); return; }
    m_pc++;

    switch (ins.op) {

    // ── Stack ─────────────────────────────────────────────────────────────
    case Opcode::PUSH_CONST: push(ins.vOperand); break;
    case Opcode::PUSH_NULL:  push(QVariant());   break;
    case Opcode::POP:        pop();               break;
    case Opcode::DUP:        push(top());         break;

    // ── Variables ─────────────────────────────────────────────────────────
    case Opcode::LOAD_VAR:   push(loadVar(ins.sOperand));        break;
    case Opcode::STORE_VAR:  storeVar(ins.sOperand, pop());      break;
    case Opcode::DECLARE_VAR:declareVar(ins.sOperand);           break;

    // ── Arithmetic ────────────────────────────────────────────────────────
    case Opcode::ADD:     { QVariant b=pop(),a=pop(); push(doAdd(a,b));    break; }
    case Opcode::SUB:     { QVariant b=pop(),a=pop(); push(doSub(a,b));    break; }
    case Opcode::MUL:     { QVariant b=pop(),a=pop(); push(doMul(a,b));    break; }
    case Opcode::DIV:     { QVariant b=pop(),a=pop(); push(doDiv(a,b));    break; }
    case Opcode::INT_DIV: { QVariant b=pop(),a=pop(); push(doIntDiv(a,b)); break; }
    case Opcode::MOD:     { QVariant b=pop(),a=pop(); push(doMod(a,b));    break; }
    case Opcode::POW:     { QVariant b=pop(),a=pop(); push(doPow(a,b));    break; }
    case Opcode::NEG:     { push(QVariant(-toNum(pop())));                  break; }
    case Opcode::CONCAT:  { QVariant b=pop(),a=pop(); push(doConcat(a,b)); break; }

    // ── Comparison ────────────────────────────────────────────────────────
    case Opcode::CMP_EQ:  { QVariant b=pop(),a=pop(); push(QVariant(a==b));                break; }
    case Opcode::CMP_NEQ: { QVariant b=pop(),a=pop(); push(QVariant(a!=b));                break; }
    case Opcode::CMP_LT:  { QVariant b=pop(),a=pop(); push(QVariant(toNum(a)<toNum(b)));   break; }
    case Opcode::CMP_GT:  { QVariant b=pop(),a=pop(); push(QVariant(toNum(a)>toNum(b)));   break; }
    case Opcode::CMP_LTE: { QVariant b=pop(),a=pop(); push(QVariant(toNum(a)<=toNum(b)));  break; }
    case Opcode::CMP_GTE: { QVariant b=pop(),a=pop(); push(QVariant(toNum(a)>=toNum(b)));  break; }

    // ── Logical ───────────────────────────────────────────────────────────
    case Opcode::LOG_AND: { QVariant b=pop(),a=pop(); push(QVariant(toBool(a)&&toBool(b))); break; }
    case Opcode::LOG_OR:  { QVariant b=pop(),a=pop(); push(QVariant(toBool(a)||toBool(b))); break; }
    case Opcode::LOG_NOT: { push(QVariant(!toBool(pop()))); break; }

    // ── Jumps ─────────────────────────────────────────────────────────────
    case Opcode::JUMP:           m_pc = ins.vOperand.toInt();                         break;
    case Opcode::JUMP_IF_FALSE:  if (!toBool(pop())) m_pc = ins.vOperand.toInt();     break;
    case Opcode::JUMP_IF_TRUE:   if ( toBool(pop())) m_pc = ins.vOperand.toInt();     break;

    // ── Calls ─────────────────────────────────────────────────────────────
    case Opcode::CALL_BUILTIN: {
        int argc = ins.vOperand.toInt();
        if (!callBuiltin(ins.sOperand, argc)) {
            if (m_error.isEmpty())
                setError(QString("Unknown built-in: %1").arg(ins.sOperand), ins.line);
        }
        break;
    }
    case Opcode::CALL: {
        int argc    = ins.vOperand.toInt();
        QString fn  = ins.sOperand;

        if (m_stream->hasFunc(fn)) {
            FuncInfo fi = m_stream->funcInfo(fn);
            CallFrame frame;
            frame.returnAddress = m_pc;
            frame.funcName      = fn;
            frame.isFunction    = fi.isFunction;
            m_callStack.append(frame);
            m_pc = fi.startAddress;
        } else {
            if (!callBuiltin(fn, argc)) {
                if (m_error.isEmpty())
                    setError(QString("Undefined function or sub: %1").arg(fn), ins.line);
            }
        }
        break;
    }
    case Opcode::RETURN: {
        bool hasVal = ins.vOperand.toInt() != 0;
        QVariant ret;
        if (hasVal) ret = pop();
        if (!m_callStack.isEmpty()) {
            int addr = m_callStack.last().returnAddress;
            m_callStack.removeLast();
            m_pc = addr;
        } else {
            m_pc = m_stream->size();
        }
        if (hasVal) push(ret);
        break;
    }

    // ── Misc ──────────────────────────────────────────────────────────────
    case Opcode::HALT: m_pc = m_stream->size(); break;
    case Opcode::NOP:  break;

    default:
        setError(QString("Unknown opcode %1").arg((int)ins.op), ins.line);
        break;
    }
}

// ── Execute a named sub (used for UI event handlers) ──────────────────────

void VM::executeSubByName(const QString &name) {
    if (!m_stream || !m_stream->hasFunc(name)) return; // handler not defined — OK

    FuncInfo fi      = m_stream->funcInfo(name);
    int savedPc      = m_pc;
    int savedDepth   = m_callStack.size();

    CallFrame frame;
    frame.returnAddress = m_stream->size(); // RETURN will set pc to size → exits loop
    frame.funcName      = name;
    frame.isFunction    = false;
    m_callStack.append(frame);
    m_pc = fi.startAddress;

    const int maxSteps = 1000000;
    int steps = 0;
    while (m_pc < m_stream->size() && m_error.isEmpty()
           && m_callStack.size() > savedDepth && !m_shouldStop) {
        if (++steps > maxSteps) {
            setError(QString("Handler '%1' exceeded step limit").arg(name));
            break;
        }
        stepInstruction();
    }

    m_pc = savedPc;
    while (m_callStack.size() > savedDepth) m_callStack.removeLast();

    if (hasError()) {
        // Report handler error but don't kill the event loop
        emit output(QString("<font color='#f38ba8'>Handler error: %1</font>").arg(m_error));
        m_error.clear();
    }
}

// ── Event handler slots (called by the worker-thread event loop) ──────────

void VM::onEventFired(const QString &handlerName) {
    executeSubByName(handlerName);
}

void VM::onFormClosed() {
    if (m_eventLoop) m_eventLoop->quit();
}

// ── Main execution loop ───────────────────────────────────────────────────

void VM::run() {
    if (!m_stream) {
        setError("No bytecode loaded");
        emit errorOccurred(m_error);
        return;
    }

    m_pc = 0;
    const int maxSteps = 10000000;
    int steps = 0;

    while (m_pc < m_stream->size() && m_error.isEmpty() && !m_shouldStop) {
        if (++steps > maxSteps) {
            setError("Execution limit reached (possible infinite loop)");
            break;
        }
        stepInstruction();
    }

    // If a visible form exists, enter the UI event loop on this worker thread.
    // Signals from FormRuntime (main thread) will be delivered here via
    // QueuedConnection, allowing event handlers to run without blocking the UI.
    if (!hasError() && m_formRuntime && m_formRuntime->isFormVisible()) {
        m_eventLoop = new QEventLoop(this);
        connect(m_formRuntime, &FormRuntime::eventFired,
                this, &VM::onEventFired, Qt::QueuedConnection);
        connect(m_formRuntime, &FormRuntime::formClosed,
                this, &VM::onFormClosed, Qt::QueuedConnection);
        m_eventLoop->exec();   // blocks until onFormClosed() calls quit()
        delete m_eventLoop;
        m_eventLoop = nullptr;
    }

    if (hasError()) {
        emit errorOccurred(m_error);
    } else {
        emit executionFinished();
    }
}
