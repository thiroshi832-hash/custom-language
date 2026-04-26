#pragma once
#include <QMainWindow>
#include <QVariantMap>
#include <QString>

class CodeEditor;
class QTextEdit;
class QTabWidget;
class QLabel;
class QAction;
class QTableWidget;
class QDockWidget;
class QToolButton;
class FormRuntime;
class VM;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void newFile();
    void openFile();
    bool saveFile();
    bool saveFileAs();
    void buildCode();
    void runCode();
    void buildAndRun();
    void debugRun();            // Build & run with debugger attached
    void openUIDesigner();

    // VM execution signals
    void onVMOutput(const QString &text);
    void onVMError(const QString &msg);
    void onVMFinished();

    // Debugger control (routed to the paused VM)
    void onVMPausedAt(int line, QVariantMap locals, QVariantMap globals);
    void onDebugContinue();
    void onDebugStep();
    void onDebugStop();

private:
    void setupUI();
    void setupMenus();
    void setupToolbar();
    void setCurrentFile(const QString &path);
    bool maybeSave();
    void clearDebugState();

    // ── Widgets ──────────────────────────────────────────────────────────
    CodeEditor   *m_editor        { nullptr };
    QTextEdit    *m_console       { nullptr };
    QTextEdit    *m_bytecodeView  { nullptr };
    QTabWidget   *m_bottomTabs    { nullptr };
    QLabel       *m_statusLabel   { nullptr };
    QLabel       *m_cursorLabel   { nullptr };  // line:col indicator

    // Left mode bar
    QToolButton  *m_btnModeEdit   { nullptr };
    QToolButton  *m_btnModeDebug  { nullptr };

    // Debug watch panel (right-side dock)
    QDockWidget  *m_debugDock     { nullptr };
    QTableWidget *m_watchTable    { nullptr };

    // ── State ─────────────────────────────────────────────────────────────
    QString      m_currentFile;
    bool         m_modified       { false };
    FormRuntime *m_formRuntime    { nullptr };
    VM          *m_debugVM        { nullptr }; // valid only while VM is paused

    // ── Actions ───────────────────────────────────────────────────────────
    QAction *m_actNew, *m_actOpen, *m_actSave, *m_actSaveAs;
    QAction *m_actBuild, *m_actRun, *m_actBuildRun;
    QAction *m_actDebugRun;                    // F6  – debug run
    QAction *m_actContinue;                    // F8  – continue
    QAction *m_actStep;                        // F10 – step over
    QAction *m_actStop;                        //      – stop debugger
};
