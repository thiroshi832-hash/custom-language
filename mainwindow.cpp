#include "mainwindow.h"
#include "codeeditor.h"
#include "syntaxhighlighter.h"
#include "lang/lexer.h"
#include "lang/parser.h"
#include "lang/compiler.h"
#include "termrunner/vm.h"
#include "termrunner/formruntime.h"
#include "uidesigner/uidesigner.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QFileInfo>
#include <QApplication>
#include <QKeySequence>
#include <QAction>
#include <QIcon>
#include <QFont>
#include <QPalette>
#include <QDateTime>
#include <QThread>
#include <QTableWidget>
#include <QHeaderView>
#include <QDockWidget>
#include <QMetaObject>

// ── Constructor ───────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_modified(false)
{
    // FormRuntime lives on the main thread; the VM (worker) calls it via
    // BlockingQueuedConnection so widgets are always created on the right thread.
    m_formRuntime = new FormRuntime(this);

    setupUI();
    setupMenus();
    setupToolbar();
    setCurrentFile(QString());
    resize(1280, 780);

    // Dark palette
    QPalette p = qApp->palette();
    p.setColor(QPalette::Window,          QColor("#1e1e2e"));
    p.setColor(QPalette::WindowText,      QColor("#cdd6f4"));
    p.setColor(QPalette::Base,            QColor("#181825"));
    p.setColor(QPalette::AlternateBase,   QColor("#1e1e2e"));
    p.setColor(QPalette::Text,            QColor("#cdd6f4"));
    p.setColor(QPalette::Button,          QColor("#313244"));
    p.setColor(QPalette::ButtonText,      QColor("#cdd6f4"));
    p.setColor(QPalette::Highlight,       QColor("#89b4fa"));
    p.setColor(QPalette::HighlightedText, QColor("#1e1e2e"));
    setPalette(p);
}

// ── UI Setup ──────────────────────────────────────────────────────────────

void MainWindow::setupUI() {
    // Code editor
    m_editor = new CodeEditor(this);
    m_editor->setStyleSheet(
        "QPlainTextEdit { background: #1e1e2e; color: #cdd6f4; "
        "selection-background-color: #45475a; border: none; }");
    new SyntaxHighlighter(m_editor->document());

    // Console output
    m_console = new QTextEdit(this);
    m_console->setReadOnly(true);
    m_console->setFont(QFont("Courier New", 9));
    m_console->setStyleSheet(
        "QTextEdit { background: #11111b; color: #a6e3a1; border: none; }");

    // Bytecode view
    m_bytecodeView = new QTextEdit(this);
    m_bytecodeView->setReadOnly(true);
    m_bytecodeView->setFont(QFont("Courier New", 9));
    m_bytecodeView->setStyleSheet(
        "QTextEdit { background: #11111b; color: #89dceb; border: none; }");

    // Bottom tabs
    m_bottomTabs = new QTabWidget(this);
    m_bottomTabs->addTab(m_console,      "Console");
    m_bottomTabs->addTab(m_bytecodeView, "Bytecode");
    m_bottomTabs->setStyleSheet(
        "QTabWidget::pane { border: none; } "
        "QTabBar::tab { background: #313244; color: #cdd6f4; padding: 4px 12px; } "
        "QTabBar::tab:selected { background: #45475a; }");
    m_bottomTabs->setMaximumHeight(220);

    // Splitter: editor top, tabs bottom
    auto *splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(m_editor);
    splitter->addWidget(m_bottomTabs);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    setCentralWidget(splitter);

    // ── Debug watch panel (right-side dock) ──────────────────────────────
    m_watchTable = new QTableWidget(0, 3, this);
    m_watchTable->setHorizontalHeaderLabels({"Name", "Value", "Scope"});
    m_watchTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_watchTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_watchTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_watchTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_watchTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_watchTable->setAlternatingRowColors(true);
    m_watchTable->setStyleSheet(
        "QTableWidget { background: #181825; color: #cdd6f4; "
        "gridline-color: #313244; border: none; } "
        "QTableWidget::item:alternate { background: #1e1e2e; } "
        "QHeaderView::section { background: #313244; color: #cdd6f4; "
        "padding: 3px; border: none; } ");
    m_watchTable->setFont(QFont("Courier New", 9));

    m_debugDock = new QDockWidget("Variables", this);
    m_debugDock->setWidget(m_watchTable);
    m_debugDock->setMinimumWidth(260);
    m_debugDock->setStyleSheet(
        "QDockWidget { color: #cdd6f4; } "
        "QDockWidget::title { background: #313244; padding: 4px; } ");
    addDockWidget(Qt::RightDockWidgetArea, m_debugDock);
    m_debugDock->hide(); // shown only when debugger is active

    // Status bar label
    m_statusLabel = new QLabel("Ready");
    statusBar()->addPermanentWidget(m_statusLabel);
    statusBar()->setStyleSheet(
        "QStatusBar { background: #313244; color: #cdd6f4; }");

    // Track modifications
    connect(m_editor->document(), &QTextDocument::modificationChanged,
            this, [this](bool mod) { m_modified = mod; });
}

void MainWindow::setupMenus() {
    // File menu
    QMenu *fileMenu = menuBar()->addMenu("&File");
    m_actNew    = fileMenu->addAction("&New",         this, &MainWindow::newFile,    QKeySequence::New);
    m_actOpen   = fileMenu->addAction("&Open...",     this, &MainWindow::openFile,   QKeySequence::Open);
    m_actSave   = fileMenu->addAction("&Save",        this, &MainWindow::saveFile,   QKeySequence::Save);
    m_actSaveAs = fileMenu->addAction("Save &As...",  this, &MainWindow::saveFileAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", qApp, &QApplication::quit, QKeySequence::Quit);

    // Build menu
    QMenu *buildMenu = menuBar()->addMenu("&Build");
    m_actBuild    = buildMenu->addAction("&Build",         this, &MainWindow::buildCode,   QKeySequence("F7"));
    m_actRun      = buildMenu->addAction("&Run",           this, &MainWindow::runCode,     QKeySequence("F5"));
    m_actBuildRun = buildMenu->addAction("Build && &Run",  this, &MainWindow::buildAndRun, QKeySequence("Ctrl+F5"));

    // Debug menu
    QMenu *debugMenu = menuBar()->addMenu("&Debug");
    m_actDebugRun = debugMenu->addAction("Debug && Run",   this, &MainWindow::debugRun,       QKeySequence("F6"));
    debugMenu->addSeparator();
    m_actContinue = debugMenu->addAction("&Continue",      this, &MainWindow::onDebugContinue, QKeySequence("F8"));
    m_actStep     = debugMenu->addAction("Step &Over",     this, &MainWindow::onDebugStep,     QKeySequence("F10"));
    m_actStop     = debugMenu->addAction("&Stop Debugger", this, &MainWindow::onDebugStop);

    m_actContinue->setEnabled(false);
    m_actStep->setEnabled(false);
    m_actStop->setEnabled(false);

    // Tools menu
    QMenu *toolsMenu = menuBar()->addMenu("&Tools");
    toolsMenu->addAction("&UI Designer", this, &MainWindow::openUIDesigner, QKeySequence("Ctrl+D"));

    menuBar()->setStyleSheet(
        "QMenuBar { background: #313244; color: #cdd6f4; } "
        "QMenuBar::item:selected { background: #45475a; } "
        "QMenu { background: #313244; color: #cdd6f4; } "
        "QMenu::item:selected { background: #45475a; }");
}

void MainWindow::setupToolbar() {
    QString tbStyle =
        "QToolBar { background: #313244; border: none; spacing: 4px; padding: 2px; } "
        "QToolButton { background: #45475a; color: #cdd6f4; padding: 4px 10px; "
        "border-radius: 3px; min-width: 50px; } "
        "QToolButton:hover { background: #585b70; } "
        "QToolButton:pressed { background: #6c7086; } "
        "QToolButton:disabled { color: #585b70; background: #313244; }";

    QToolBar *tb = addToolBar("Main");
    tb->setMovable(false);
    tb->setStyleSheet(tbStyle);

    tb->addAction(m_actNew);
    tb->addAction(m_actOpen);
    tb->addAction(m_actSave);
    tb->addSeparator();

    m_actBuild->setText("Build (F7)");
    m_actRun->setText("Run (F5)");
    m_actBuildRun->setText("Build & Run");
    tb->addAction(m_actBuild);
    tb->addAction(m_actRun);
    tb->addAction(m_actBuildRun);
    tb->addSeparator();

    m_actDebugRun->setText("Debug (F6)");
    tb->addAction(m_actDebugRun);
    tb->addSeparator();

    m_actContinue->setText("Continue (F8)");
    m_actStep->setText("Step (F10)");
    m_actStop->setText("Stop");
    tb->addAction(m_actContinue);
    tb->addAction(m_actStep);
    tb->addAction(m_actStop);
    tb->addSeparator();

    tb->addAction("UI Designer", this, &MainWindow::openUIDesigner);
}

// ── File operations ───────────────────────────────────────────────────────

void MainWindow::setCurrentFile(const QString &path) {
    m_currentFile = path;
    m_editor->document()->setModified(false);
    m_modified = false;
    QString title = path.isEmpty() ? "Untitled" : QFileInfo(path).fileName();
    setWindowTitle(title + " - CustomLanguage IDE");
}

bool MainWindow::maybeSave() {
    if (!m_modified) return true;
    auto ret = QMessageBox::warning(this, "Unsaved Changes",
        "The document has unsaved changes.\nDo you want to save?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)   return saveFile();
    if (ret == QMessageBox::Cancel) return false;
    return true;
}

void MainWindow::newFile() {
    if (!maybeSave()) return;
    m_editor->clear();
    setCurrentFile(QString());
}

void MainWindow::openFile() {
    if (!maybeSave()) return;
    QString path = QFileDialog::getOpenFileName(this,
        "Open Source File", QString(),
        "VBScript-like files (*.vbs *.bas *.txt);;All files (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot open file: " + path);
        return;
    }
    m_editor->setPlainText(QTextStream(&file).readAll());
    setCurrentFile(path);
}

bool MainWindow::saveFile() {
    if (m_currentFile.isEmpty()) return saveFileAs();
    QFile file(m_currentFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Cannot save file: " + m_currentFile);
        return false;
    }
    QTextStream(&file) << m_editor->toPlainText();
    setCurrentFile(m_currentFile);
    return true;
}

bool MainWindow::saveFileAs() {
    QString path = QFileDialog::getSaveFileName(this,
        "Save Source File", QString(),
        "VBScript-like files (*.vbs *.bas *.txt);;All files (*)");
    if (path.isEmpty()) return false;
    m_currentFile = path;
    return saveFile();
}

// ── Build ─────────────────────────────────────────────────────────────────

static OpcodeStream s_stream;

void MainWindow::buildCode() {
    QString source = m_editor->toPlainText();
    m_console->clear();
    m_bytecodeView->clear();
    m_bottomTabs->setCurrentIndex(1); // show bytecode tab

    m_statusLabel->setText("Building...");

    // Lex
    Lexer lexer(source);
    QVector<Token> tokens = lexer.tokenize();
    if (lexer.hasError()) {
        m_console->append("<span style='color:#f38ba8;'>Lexer error: " +
                          lexer.errorMessage() + "</span>");
        m_bytecodeView->setPlainText("Build failed.");
        m_statusLabel->setText("Build failed.");
        m_bottomTabs->setCurrentIndex(0);
        return;
    }

    // Parse
    Parser parser(tokens);
    Program *prog = parser.parse();
    if (parser.hasError()) {
        delete prog;
        m_console->append("<span style='color:#f38ba8;'>Parse error: " +
                          parser.errorMessage() + "</span>");
        m_bytecodeView->setPlainText("Build failed.");
        m_statusLabel->setText("Build failed.");
        m_bottomTabs->setCurrentIndex(0);
        return;
    }

    // Compile
    Compiler compiler;
    if (!compiler.compile(prog)) {
        delete prog;
        m_console->append("<span style='color:#f38ba8;'>Compile error: " +
                          compiler.errorMessage() + "</span>");
        m_bytecodeView->setPlainText("Build failed.");
        m_statusLabel->setText("Build failed.");
        m_bottomTabs->setCurrentIndex(0);
        return;
    }

    s_stream = compiler.stream();
    delete prog;

    m_bytecodeView->setPlainText(s_stream.disassemble());
    m_statusLabel->setText(QString("Build OK — %1 instructions").arg(s_stream.size()));
    m_console->append("<span style='color:#a6e3a1;'>Build successful.</span>");
}

// ── Run ───────────────────────────────────────────────────────────────────

void MainWindow::runCode() {
    if (s_stream.size() == 0) {
        m_console->append("<span style='color:#fab387;'>Nothing built. "
                          "Press Build (F7) first.</span>");
        m_bottomTabs->setCurrentIndex(0);
        return;
    }

    m_console->clear();
    m_bottomTabs->setCurrentIndex(0);

    m_actRun->setEnabled(false);
    m_actBuildRun->setEnabled(false);
    m_actBuild->setEnabled(false);
    m_actDebugRun->setEnabled(false);
    m_statusLabel->setText("Running...");

    m_formRuntime->clearAll();

    VM *vm = new VM();
    m_debugVM = nullptr; // not in debug mode

    QThread *thread = new QThread(this);
    vm->moveToThread(thread);
    vm->setStream(&s_stream);
    vm->setFormRuntime(m_formRuntime);
    vm->reset();

    connect(thread, &QThread::started,          vm,   &VM::run);
    connect(vm,     &VM::output,                this, &MainWindow::onVMOutput,   Qt::QueuedConnection);
    connect(vm,     &VM::errorOccurred,         this, &MainWindow::onVMError,    Qt::QueuedConnection);
    connect(vm,     &VM::executionFinished,     this, &MainWindow::onVMFinished, Qt::QueuedConnection);
    connect(vm,     &VM::executionFinished,     thread, &QThread::quit);
    connect(vm,     &VM::errorOccurred,         thread, &QThread::quit);
    connect(thread, &QThread::finished,         vm,   &VM::deleteLater);
    connect(thread, &QThread::finished,         thread, &QThread::deleteLater);

    thread->start();
}

void MainWindow::buildAndRun() {
    buildCode();
    if (m_statusLabel->text().startsWith("Build OK"))
        runCode();
}

// ── Debug run ─────────────────────────────────────────────────────────────

void MainWindow::debugRun() {
    buildCode();
    if (!m_statusLabel->text().startsWith("Build OK"))
        return;

    if (s_stream.size() == 0) return;

    m_console->clear();
    m_bottomTabs->setCurrentIndex(0);
    m_debugDock->show();
    m_watchTable->setRowCount(0);

    m_actRun->setEnabled(false);
    m_actBuildRun->setEnabled(false);
    m_actBuild->setEnabled(false);
    m_actDebugRun->setEnabled(false);
    m_actStop->setEnabled(true); // can always stop
    m_statusLabel->setText("Debug running...");

    m_formRuntime->clearAll();

    VM *vm = new VM();
    m_debugVM = vm; // remember for invokeMethod calls

    QThread *thread = new QThread(this);
    vm->moveToThread(thread);
    vm->setStream(&s_stream);
    vm->setFormRuntime(m_formRuntime);
    vm->setDebugMode(true);
    vm->setBreakpoints(m_editor->breakpoints());
    vm->reset();

    connect(thread, &QThread::started,      vm,   &VM::run);
    connect(vm, &VM::output,                this, &MainWindow::onVMOutput,   Qt::QueuedConnection);
    connect(vm, &VM::errorOccurred,         this, &MainWindow::onVMError,    Qt::QueuedConnection);
    connect(vm, &VM::executionFinished,     this, &MainWindow::onVMFinished, Qt::QueuedConnection);
    connect(vm, SIGNAL(pausedAt(int,QVariantMap,QVariantMap)),
            this, SLOT(onVMPausedAt(int,QVariantMap,QVariantMap)),
            Qt::QueuedConnection);
    connect(vm, &VM::executionFinished, thread, &QThread::quit);
    connect(vm, &VM::errorOccurred,     thread, &QThread::quit);
    connect(thread, &QThread::finished, vm,     &VM::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    // Clear m_debugVM when VM is destroyed so we don't call a dangling pointer
    connect(vm, &QObject::destroyed, this, [this]() { m_debugVM = nullptr; });

    thread->start();
}

// ── VM signal handlers ────────────────────────────────────────────────────

void MainWindow::onVMOutput(const QString &text) {
    m_console->append(text);
}

void MainWindow::onVMError(const QString &msg) {
    m_console->append("<span style='color:#f38ba8;'>" + msg + "</span>");
    m_statusLabel->setText("Runtime error.");
    clearDebugState();
}

void MainWindow::onVMFinished() {
    m_statusLabel->setText("Execution finished.");
    clearDebugState();
}

void MainWindow::clearDebugState() {
    m_actRun->setEnabled(true);
    m_actBuildRun->setEnabled(true);
    m_actBuild->setEnabled(true);
    m_actDebugRun->setEnabled(true);
    m_actContinue->setEnabled(false);
    m_actStep->setEnabled(false);
    m_actStop->setEnabled(false);
    m_editor->setDebugLine(0);
    m_debugVM = nullptr;
}

// ── Debugger pause/control ────────────────────────────────────────────────

void MainWindow::onVMPausedAt(int line, QVariantMap locals, QVariantMap globals) {
    // Scroll editor to the paused line and show yellow arrow
    m_editor->setDebugLine(line);
    m_statusLabel->setText(QString("Paused at line %1").arg(line));

    // Populate watch table: locals first, then globals
    m_watchTable->setRowCount(0);

    auto addRow = [this](const QString &name, const QVariant &val, const QString &scope) {
        int row = m_watchTable->rowCount();
        m_watchTable->insertRow(row);
        m_watchTable->setItem(row, 0, new QTableWidgetItem(name));
        m_watchTable->setItem(row, 1, new QTableWidgetItem(val.toString()));
        QTableWidgetItem *scopeItem = new QTableWidgetItem(scope);
        scopeItem->setForeground(scope == "Local"
            ? QColor("#a6e3a1")   // green for locals
            : QColor("#89b4fa")); // blue for globals
        m_watchTable->setItem(row, 2, scopeItem);
    };

    for (auto it = locals.constBegin();  it != locals.constEnd();  ++it)
        addRow(it.key(), it.value(), "Local");
    for (auto it = globals.constBegin(); it != globals.constEnd(); ++it)
        addRow(it.key(), it.value(), "Global");

    // Enable debugger controls
    m_actContinue->setEnabled(true);
    m_actStep->setEnabled(true);
    m_actStop->setEnabled(true);

    m_bottomTabs->setCurrentIndex(0); // keep console visible
}

void MainWindow::onDebugContinue() {
    if (!m_debugVM) return;
    m_actContinue->setEnabled(false);
    m_actStep->setEnabled(false);
    m_editor->setDebugLine(0);
    m_statusLabel->setText("Debug running...");
    QMetaObject::invokeMethod(m_debugVM, "debugContinue", Qt::QueuedConnection);
}

void MainWindow::onDebugStep() {
    if (!m_debugVM) return;
    m_actContinue->setEnabled(false);
    m_actStep->setEnabled(false);
    m_editor->setDebugLine(0);
    m_statusLabel->setText("Stepping...");
    QMetaObject::invokeMethod(m_debugVM, "debugStep", Qt::QueuedConnection);
}

void MainWindow::onDebugStop() {
    if (!m_debugVM) {
        clearDebugState();
        return;
    }
    m_statusLabel->setText("Stopping...");
    QMetaObject::invokeMethod(m_debugVM, "debugStop", Qt::QueuedConnection);
    // clearDebugState() will be called when VM emits executionFinished/errorOccurred
}

// ── UI Designer ───────────────────────────────────────────────────────────

void MainWindow::openUIDesigner() {
    auto *designer = new UIDesigner(nullptr); // top-level window
    designer->setAttribute(Qt::WA_DeleteOnClose);
    designer->show();
}
