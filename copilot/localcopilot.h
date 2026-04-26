#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

// ── LocalCopilot ──────────────────────────────────────────────────────────
// Fully offline, zero-dependency code generator for CustomLanguage.
// Matches the user's plain-English description against a library of
// 20+ complete code templates using keyword scoring, then returns the
// best-matching code instantly (no network, no external programs).
//
// Usage:
//   LocalCopilot *lc = new LocalCopilot(this);
//   connect(lc, &LocalCopilot::codeReady, panel, ...);
//   lc->generate("create a counter form with buttons");

struct CodeTemplate {
    QString     id;
    QString     title;
    QString     description;  // shown to the user
    QStringList keywords;     // scored against the user's input
    QString     code;
};

class LocalCopilot : public QObject {
    Q_OBJECT
public:
    explicit LocalCopilot(QObject *parent = nullptr);

    // Find the best-matching template and emit codeReady().
    // If nothing scores above threshold, emit noMatch() with a help list.
    void generate(const QString &userDescription);

    // Return all template titles+descriptions for the help panel
    QVector<CodeTemplate> allTemplates() const { return m_templates; }

signals:
    void codeReady(const QString &code,
                   const QString &templateTitle,
                   const QString &templateDescription);
    void noMatch(const QString &helpText);

private:
    void buildTemplates();
    int  score(const CodeTemplate &tpl, const QStringList &words) const;

    QVector<CodeTemplate> m_templates;
};
