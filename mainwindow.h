#pragma once
#include <QMainWindow>
#include <QString>

class CodeEditor;
class QTextEdit;
class QTabWidget;
class QLabel;
class QAction;
class FormRuntime;

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
    void openUIDesigner();
    void onVMOutput(const QString &text);
    void onVMError(const QString &msg);
    void onVMFinished();

private:
    void setupUI();
    void setupMenus();
    void setupToolbar();
    void setCurrentFile(const QString &path);
    bool maybeSave();

    // Widgets
    CodeEditor *m_editor;
    QTextEdit  *m_console;
    QTextEdit  *m_bytecodeView;
    QTabWidget *m_bottomTabs;
    QLabel     *m_statusLabel;

    // State
    QString      m_currentFile;
    bool         m_modified;
    FormRuntime *m_formRuntime { nullptr };

    // Actions
    QAction *m_actNew, *m_actOpen, *m_actSave, *m_actSaveAs;
    QAction *m_actBuild, *m_actRun, *m_actBuildRun;
};
