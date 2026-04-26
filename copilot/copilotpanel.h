#pragma once
#include <QWidget>
#include <QString>

class LocalCopilot;
class CodeEditor;
class QTextEdit;
class QPushButton;
class QLabel;

// ── CopilotPanel ──────────────────────────────────────────────────────────
// Chat-style dock panel.  The user describes what they want in plain English;
// the built-in LocalCopilot matches the request against 20+ ready-made
// CustomLanguage templates and returns the code instantly — no network,
// no server, no API key required.

class CopilotPanel : public QWidget {
    Q_OBJECT
public:
    explicit CopilotPanel(QWidget *parent = nullptr);

    // Called by MainWindow so Insert can place code at the cursor
    void setEditor(CodeEditor *editor);

    // Use the shared LocalCopilot owned by MainWindow
    void setLocalCopilot(LocalCopilot *copilot);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onGenerate();
    void onInsertToEditor();
    void onClearHistory();
    void onCodeReady(const QString &code,
                     const QString &title,
                     const QString &description);
    void onNoMatch(const QString &helpText);

private:
    void    setupUI();
    void    appendUserBubble(const QString &text);
    void    appendAssistantBubble(const QString &title,
                                  const QString &description,
                                  const QString &code);
    void    appendInfoBubble(const QString &text);

    LocalCopilot *m_localCopilot { nullptr };
    CodeEditor   *m_editor       { nullptr };

    // ── Widgets ───────────────────────────────────────────────────────────
    QTextEdit   *m_history     { nullptr };
    QTextEdit   *m_input       { nullptr };
    QPushButton *m_genBtn      { nullptr };
    QPushButton *m_insertBtn   { nullptr };
    QPushButton *m_clearBtn    { nullptr };
    QLabel      *m_statusLabel { nullptr };

    // ── State ─────────────────────────────────────────────────────────────
    QString m_lastCode;
};
