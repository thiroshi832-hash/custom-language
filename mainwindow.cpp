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
#include <QToolButton>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMetaObject>

// ── Qt Creator colour palette ─────────────────────────────────────────────
// All colours are taken from Qt Creator's default dark theme.
namespace QC {
    static const char BG[]        = "#2b2b2b";   // main background
    static const char PANEL[]     = "#3c3f41";   // panels / sidebar
    static const char ACTIVE[]    = "#4e5254";   // hover / active tab
    static const char BORDER[]    = "#323232";   // borders / separators
    static const char TEXT[]      = "#a9b7c6";   // main text
    static const char DIM[]       = "#606366";   // dim text / line numbers
    static const char ACCENT[]    = "#3592c4";   // blue accent
    static const char EDITOR_BG[] = "#2b2b2b";   // editor background
    static const char SEL_BG[]    = "#214283";   // selection background
    static const char CURLINE[]   = "#323232";   // current-line highlight
    static const char CONSOLE[]   = "#1e1e1e";   // console/output background
    static const char CON_TEXT[]  = "#a9b7c6";   // console text
}

// ── Constructor ───────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_modified(false)
{
    m_formRuntime = new FormRuntime(this);

    // Application-wide palette (Qt Creator dark)
    QPalette p;
    p.setColor(QPalette::Window,          QColor(QC::PANEL));
    p.setColor(QPalette::WindowText,      QColor(QC::TEXT));
    p.setColor(QPalette::Base,            QColor(QC::BG));
    p.setColor(QPalette::AlternateBase,   QColor(QC::PANEL));
    p.setColor(QPalette::Text,            QColor(QC::TEXT));
    p.setColor(QPalette::Button,          QColor(QC::PANEL));
    p.setColor(QPalette::ButtonText,      QColor(QC::TEXT));
    p.setColor(QPalette::Highlight,       QColor(QC::ACCENT));
    p.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    p.setColor(QPalette::Mid,             QColor(QC::BORDER));
    p.setColor(QPalette::Dark,            QColor(QC::BORDER));
    p.setColor(QPalette::Shadow,          QColor("#1a1a1a"));
    p.setColor(QPalette::ToolTipBase,     QColor(QC::ACTIVE));
    p.setColor(QPalette::ToolTipText,     QColor(QC::TEXT));
    qApp->setPalette(p);

    setupUI();
    setupMenus();
    setupToolbar();
    setCurrentFile(QString());
    resize(1360, 820);
}

// ── UI Setup ──────────────────────────────────────────────────────────────

void MainWindow::setupUI() {
    // ── Code editor ───────────────────────────────────────────────────────
    m_editor = new CodeEditor(this);
    m_editor->setStyleSheet(
        QString("QPlainTextEdit { background:%1; color:%2; "
                "selection-background-color:%3; border:none; }")
        .arg(QC::EDITOR_BG).arg(QC::TEXT).arg(QC::SEL_BG));
    new SyntaxHighlighter(m_editor->document());

    // ── Console output ────────────────────────────────────────────────────
    m_console = new QTextEdit(this);
    m_console->setReadOnly(true);
    m_console->setFont(QFont("Consolas", 9));
    m_console->setStyleSheet(
        QString("QTextEdit { background:%1; color:%2; border:none; }")
        .arg(QC::CONSOLE).arg(QC::CON_TEXT));

    // ── Bytecode view ─────────────────────────────────────────────────────
    m_bytecodeView = new QTextEdit(this);
    m_bytecodeView->setReadOnly(true);
    m_bytecodeView->setFont(QFont("Consolas", 9));
    m_bytecodeView->setStyleSheet(
        QString("QTextEdit { background:%1; color:#5f9ea0; border:none; }")
        .arg(QC::CONSOLE));

    // ── Bottom output pane ────────────────────────────────────────────────
    m_bottomTabs = new QTabWidget(this);
    m_bottomTabs->addTab(m_console,      "  Application Output  ");
    m_bottomTabs->addTab(m_bytecodeView, "  Bytecode  ");
    m_bottomTabs->setStyleSheet(
        QString("QTabWidget::pane { border-top:1px solid %1; background:%2; } "
                "QTabBar::tab { background:%3; color:%4; padding:5px 14px; "
                "  border:none; border-top:2px solid transparent; } "
                "QTabBar::tab:selected { background:%2; color:#ffffff; "
                "  border-top:2px solid %5; } "
                "QTabBar::tab:hover:!selected { background:%6; } ")
        .arg(QC::BORDER).arg(QC::CONSOLE).arg(QC::PANEL)
        .arg(QC::DIM).arg(QC::ACCENT).arg(QC::ACTIVE));
    m_bottomTabs->setMaximumHeight(200);

    // ── Left mode bar (Qt Creator style) ─────────────────────────────────
    QWidget *modeBar = new QWidget();
    modeBar->setFixedWidth(54);
    modeBar->setStyleSheet(
        QString("QWidget { background:%1; border-right:1px solid %2; }")
        .arg(QC::PANEL).arg(QC::BORDER));

    QString modeBtnSS =
        QString("QToolButton {"
                "  background:transparent; color:%1;"
                "  border:none; border-left:3px solid transparent;"
                "  font-size:8pt; font-weight:bold;"
                "  padding:10px 0px; min-width:54px; min-height:54px; }"
                "QToolButton:checked {"
                "  background:%2; color:#ffffff;"
                "  border-left:3px solid %3; }"
                "QToolButton:hover:!checked {"
                "  background:%4; color:%5; }")
        .arg(QC::DIM).arg(QC::ACTIVE).arg(QC::ACCENT).arg(QC::ACTIVE).arg(QC::TEXT);

    m_btnModeEdit   = new QToolButton();
    m_btnModeDebug  = new QToolButton();
    m_btnModeDesign = new QToolButton();
    m_btnModeEdit  ->setStyleSheet(modeBtnSS);
    m_btnModeDebug ->setStyleSheet(modeBtnSS);
    m_btnModeDesign->setStyleSheet(modeBtnSS);
    m_btnModeEdit  ->setText("Edit");
    m_btnModeDebug ->setText("Debug");
    m_btnModeDesign->setText("Design");
    m_btnModeEdit  ->setCheckable(true);
    m_btnModeDebug ->setCheckable(true);
    m_btnModeDesign->setCheckable(true);
    m_btnModeEdit  ->setChecked(true);

    // Edit and Debug are mutually exclusive; Design is independent
    // (it opens a separate window, so we don't lock it to a single mode)
    auto *modeGroup = new QButtonGroup(modeBar);
    modeGroup->addButton(m_btnModeEdit);
    modeGroup->addButton(m_btnModeDebug);
    modeGroup->addButton(m_btnModeDesign);
    modeGroup->setExclusive(true);

    auto *modeLayout = new QVBoxLayout(modeBar);
    modeLayout->setContentsMargins(0, 6, 0, 6);
    modeLayout->setSpacing(2);
    modeLayout->addWidget(m_btnModeEdit);
    modeLayout->addWidget(m_btnModeDebug);
    modeLayout->addWidget(m_btnModeDesign);
    modeLayout->addStretch();

    // Toggle debug dock when mode changes
    connect(m_btnModeDebug, &QToolButton::toggled, this, [this](bool on) {
        if (on) m_debugDock->show(); else m_debugDock->hide();
    });
    connect(m_btnModeEdit, &QToolButton::toggled, this, [this](bool on) {
        if (on) m_debugDock->hide();
    });
    // Design button opens the UI Designer window and returns to Edit when closed
    connect(m_btnModeDesign, &QToolButton::clicked, this, [this]() {
        auto *designer = new UIDesigner(nullptr);
        designer->setAttribute(Qt::WA_DeleteOnClose);
        connect(designer, &QObject::destroyed, this, [this]() {
            m_btnModeEdit->setChecked(true); // back to Edit when window closes
        });
        designer->show();
    });

    // ── Main vertical splitter (editor + output) ──────────────────────────
    auto *vSplit = new QSplitter(Qt::Vertical);
    vSplit->setStyleSheet(
        QString("QSplitter::handle { background:%1; height:1px; }").arg(QC::BORDER));
    vSplit->addWidget(m_editor);
    vSplit->addWidget(m_bottomTabs);
    vSplit->setStretchFactor(0, 4);
    vSplit->setStretchFactor(1, 1);

    // ── Centre container = modeBar + splitter ─────────────────────────────
    QWidget *centre = new QWidget(this);
    auto *hLayout = new QHBoxLayout(centre);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(0);
    hLayout->addWidget(modeBar);
    hLayout->addWidget(vSplit);
    setCentralWidget(centre);

    // ── Debug variables dock (right side) ─────────────────────────────────
    m_watchTable = new QTableWidget(0, 3, this);
    m_watchTable->setHorizontalHeaderLabels({"Name", "Value", "Scope"});
    m_watchTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_watchTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_watchTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_watchTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_watchTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_watchTable->setAlternatingRowColors(true);
    m_watchTable->setFont(QFont("Consolas", 9));
    m_watchTable->setStyleSheet(
        QString("QTableWidget { background:%1; color:%2; "
                "  gridline-color:%3; border:none; }"
                "QTableWidget::item:alternate { background:%4; }"
                "QHeaderView::section { background:%5; color:%6; "
                "  padding:4px; border:none; border-bottom:1px solid %3; }")
        .arg(QC::BG).arg(QC::TEXT).arg(QC::BORDER)
        .arg(QC::PANEL).arg(QC::PANEL).arg(QC::TEXT));

    m_debugDock = new QDockWidget("Local && Global Variables", this);
    m_debugDock->setWidget(m_watchTable);
    m_debugDock->setMinimumWidth(270);
    m_debugDock->setStyleSheet(
        QString("QDockWidget { color:%1; font-weight:bold; }"
                "QDockWidget::title { background:%2; padding:6px; "
                "  border-bottom:1px solid %3; }")
        .arg(QC::TEXT).arg(QC::PANEL).arg(QC::BORDER));
    addDockWidget(Qt::RightDockWidgetArea, m_debugDock);
    m_debugDock->hide();

    // ── Status bar ────────────────────────────────────────────────────────
    statusBar()->setStyleSheet(
        QString("QStatusBar { background:%1; color:%2; border-top:1px solid %3; }"
                "QStatusBar::item { border:none; }")
        .arg(QC::PANEL).arg(QC::TEXT).arg(QC::BORDER));

    m_statusLabel = new QLabel("Ready");
    m_cursorLabel = new QLabel("Ln 1, Col 1");
    m_cursorLabel->setStyleSheet(QString("color:%1;").arg(QC::DIM));
    statusBar()->addWidget(m_statusLabel);
    statusBar()->addPermanentWidget(m_cursorLabel);

    // Update cursor position label
    connect(m_editor, &CodeEditor::cursorPositionChanged, this, [this]() {
        QTextCursor c = m_editor->textCursor();
        m_cursorLabel->setText(
            QString("Ln %1, Col %2")
            .arg(c.blockNumber() + 1)
            .arg(c.columnNumber() + 1));
    });

    // Track modifications
    connect(m_editor->document(), &QTextDocument::modificationChanged,
            this, [this](bool mod) { m_modified = mod; });
}

void MainWindow::setupMenus() {
    QString menuSS =
        QString("QMenuBar { background:%1; color:%2; padding:2px 0; spacing:0; }"
                "QMenuBar::item { padding:4px 10px; background:transparent; }"
                "QMenuBar::item:selected { background:%3; }"
                "QMenu { background:%1; color:%2; border:1px solid %4; }"
                "QMenu::item { padding:5px 24px 5px 12px; }"
                "QMenu::item:selected { background:%3; }"
                "QMenu::separator { height:1px; background:%4; margin:3px 0; }")
        .arg(QC::PANEL).arg(QC::TEXT).arg(QC::ACTIVE).arg(QC::BORDER);
    menuBar()->setStyleSheet(menuSS);

    // File
    QMenu *fileMenu = menuBar()->addMenu("&File");
    m_actNew    = fileMenu->addAction("&New",        this, &MainWindow::newFile,    QKeySequence::New);
    m_actOpen   = fileMenu->addAction("&Open...",    this, &MainWindow::openFile,   QKeySequence::Open);
    m_actSave   = fileMenu->addAction("&Save",       this, &MainWindow::saveFile,   QKeySequence::Save);
    m_actSaveAs = fileMenu->addAction("Save &As...", this, &MainWindow::saveFileAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", qApp, &QApplication::quit, QKeySequence::Quit);

    // Build
    QMenu *buildMenu = menuBar()->addMenu("&Build");
    m_actBuild    = buildMenu->addAction("&Build",        this, &MainWindow::buildCode,   QKeySequence("F7"));
    m_actRun      = buildMenu->addAction("&Run",          this, &MainWindow::runCode,     QKeySequence("F5"));
    m_actBuildRun = buildMenu->addAction("Build && &Run", this, &MainWindow::buildAndRun, QKeySequence("Ctrl+F5"));

    // Debug
    QMenu *debugMenu = menuBar()->addMenu("&Debug");
    m_actDebugRun = debugMenu->addAction("Debug && Run",   this, &MainWindow::debugRun,        QKeySequence("F6"));
    debugMenu->addSeparator();
    m_actContinue = debugMenu->addAction("&Continue",      this, &MainWindow::onDebugContinue, QKeySequence("F8"));
    m_actStep     = debugMenu->addAction("Step &Over",     this, &MainWindow::onDebugStep,     QKeySequence("F10"));
    m_actStop     = debugMenu->addAction("&Stop Debugger", this, &MainWindow::onDebugStop);
    m_actContinue->setEnabled(false);
    m_actStep    ->setEnabled(false);
    m_actStop    ->setEnabled(false);

    // Tools
    QMenu *toolsMenu = menuBar()->addMenu("&Tools");
    toolsMenu->addAction("&UI Designer", this, &MainWindow::openUIDesigner, QKeySequence("Ctrl+D"));
}

void MainWindow::setupToolbar() {
    // Qt Creator uses a flat, compact toolbar with no border-radius
    QString tbSS =
        QString("QToolBar { background:%1; border:none; border-bottom:1px solid %2;"
                "  spacing:1px; padding:2px 4px; }"
                "QToolButton { background:transparent; color:%3;"
                "  border:none; padding:4px 8px; font-size:9pt; }"
                "QToolButton:hover { background:%4; }"
                "QToolButton:pressed { background:%5; }"
                "QToolButton:disabled { color:%6; }"
                "QToolBar::separator { background:%2; width:1px; margin:4px 4px; }")
        .arg(QC::PANEL).arg(QC::BORDER).arg(QC::TEXT)
        .arg(QC::ACTIVE).arg(QC::BORDER).arg(QC::DIM);

    QToolBar *tb = addToolBar("Main");
    tb->setMovable(false);
    tb->setIconSize(QSize(16, 16));
    tb->setStyleSheet(tbSS);

    // File actions
    m_actNew ->setShortcut(QKeySequence::New);
    m_actOpen->setShortcut(QKeySequence::Open);
    m_actSave->setShortcut(QKeySequence::Save);
    tb->addAction(m_actNew);
    tb->addAction(m_actOpen);
    tb->addAction(m_actSave);
    tb->addSeparator();

    // Build / Run
    m_actBuild   ->setText("  Build  ");
    m_actRun     ->setText("  Run  ");
    m_actBuildRun->setText("  Build && Run  ");
    tb->addAction(m_actBuild);
    tb->addAction(m_actRun);
    tb->addAction(m_actBuildRun);
    tb->addSeparator();

    // Debug
    m_actDebugRun->setText("  Debug  ");
    tb->addAction(m_actDebugRun);
    tb->addSeparator();

    // Debug controls (enabled only when paused)
    m_actContinue->setText("  Continue  ");
    m_actStep    ->setText("  Step Over  ");
    m_actStop    ->setText("  Stop  ");
    tb->addAction(m_actContinue);
    tb->addAction(m_actStep);
    tb->addAction(m_actStop);
    tb->addSeparator();

    // (UI Designer is in the left mode bar; keep it in the Tools menu via Ctrl+D)

    // Colour the "Run" and "Debug" buttons with a subtle green/blue tint
    // to match Qt Creator's prominent build buttons
    if (QToolButton *btn = qobject_cast<QToolButton*>(tb->widgetForAction(m_actRun)))
        btn->setStyleSheet(
            QString("QToolButton { color:#57a64a; background:transparent;"
                    "  border:none; padding:4px 8px; }"
                    "QToolButton:hover { background:%1; }"
                    "QToolButton:disabled { color:%2; }").arg(QC::ACTIVE).arg(QC::DIM));
    if (QToolButton *btn = qobject_cast<QToolButton*>(tb->widgetForAction(m_actDebugRun)))
        btn->setStyleSheet(
            QString("QToolButton { color:%1; background:transparent;"
                    "  border:none; padding:4px 8px; }"
                    "QToolButton:hover { background:%2; }"
                    "QToolButton:disabled { color:%3; }").arg(QC::ACCENT).arg(QC::ACTIVE).arg(QC::DIM));
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
    m_btnModeDebug->setChecked(true); // switches to Debug mode, shows dock
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
    m_actRun     ->setEnabled(true);
    m_actBuildRun->setEnabled(true);
    m_actBuild   ->setEnabled(true);
    m_actDebugRun->setEnabled(true);
    m_actContinue->setEnabled(false);
    m_actStep    ->setEnabled(false);
    m_actStop    ->setEnabled(false);
    m_editor->setDebugLine(0);
    m_debugVM = nullptr;
    // Return mode bar to Edit
    m_btnModeEdit->setChecked(true);
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
