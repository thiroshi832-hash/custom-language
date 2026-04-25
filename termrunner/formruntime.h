#pragma once
#include <QObject>
#include <QMap>
#include <QString>

class QWidget;

// ── FormRuntime ───────────────────────────────────────────────────────────
// Lives on the MAIN thread.
// The VM (worker thread) calls its slots via BlockingQueuedConnection.

class FormRuntime : public QObject {
    Q_OBJECT
public:
    explicit FormRuntime(QObject *parent = nullptr);
    ~FormRuntime();

    bool isFormVisible() const { return m_formVisible; }

public slots:
    // --- called from VM thread via BlockingQueuedConnection ---
    void initForm   (const QString &title, int w, int h);
    int  createControl(const QString &type, const QString &name,
                       int x, int y, int w, int h);
    void setProperty(int id, const QString &key, const QString &value);
    void addItem    (int id, const QString &value);
    void showForm   ();
    void clearAll   ();

signals:
    void eventFired(const QString &handlerName); // e.g. "btn1_Click"
    void formClosed();

private:
    QWidget            *m_form   { nullptr };
    QMap<int, QWidget*> m_widgets;
    QMap<int, QString>  m_names;
    int                 m_nextId { 1 };
    bool                m_formVisible { false };

    void ensureForm(const QString &title = "Form", int w = 640, int h = 480);
};
