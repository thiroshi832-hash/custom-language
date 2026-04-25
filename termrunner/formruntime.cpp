#include "formruntime.h"

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QSpinBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QListWidget>
#include <QGroupBox>
#include <QFrame>
#include <QSlider>
#include <QCloseEvent>

// ── FormWindow ────────────────────────────────────────────────────────────
// Internal QWidget subclass that signals when the user closes it.

class FormWindow : public QWidget {
    Q_OBJECT
public:
    explicit FormWindow(QWidget *parent = nullptr)
        : QWidget(parent, Qt::Window) {}
signals:
    void windowClosed();
protected:
    void closeEvent(QCloseEvent *e) override {
        emit windowClosed();
        QWidget::closeEvent(e);
    }
};

// ── FormRuntime ────────────────────────────────────────────────────────────

FormRuntime::FormRuntime(QObject *parent)
    : QObject(parent), m_form(nullptr), m_nextId(1), m_formVisible(false)
{}

FormRuntime::~FormRuntime() {
    // deleteLater on m_form would be unsafe here; just delete directly.
    delete m_form;
    m_form = nullptr;
}

void FormRuntime::ensureForm(const QString &title, int w, int h) {
    if (!m_form) {
        auto *fw = new FormWindow(nullptr);
        fw->setWindowTitle(title);
        fw->resize(w, h);
        m_form = fw;
        connect(fw, &FormWindow::windowClosed,
                this, &FormRuntime::formClosed);
    }
}

// ── Slots (called from VM thread via BlockingQueuedConnection) ────────────

void FormRuntime::initForm(const QString &title, int w, int h) {
    // Create form if needed; update title & size without destroying controls.
    ensureForm(title, w, h);
    m_form->setWindowTitle(title);
    m_form->resize(w, h);
}

int FormRuntime::createControl(const QString &type, const QString &name,
                               int x, int y, int w, int h)
{
    ensureForm(); // use defaults if not yet initialised

    QWidget *widget = nullptr;
    QString t = type.toLower();

    if (t == "pushbutton" || t == "button") {
        auto *btn = new QPushButton(name, m_form);
        connect(btn, &QPushButton::clicked, this, [this, name]() {
            emit eventFired(name + "_Click");
        });
        widget = btn;
    }
    else if (t == "label") {
        widget = new QLabel(name, m_form);
    }
    else if (t == "lineedit" || t == "textbox") {
        widget = new QLineEdit(m_form);
    }
    else if (t == "checkbox") {
        auto *cb = new QCheckBox(name, m_form);
        connect(cb, &QCheckBox::stateChanged, this, [this, name](int) {
            emit eventFired(name + "_Changed");
        });
        widget = cb;
    }
    else if (t == "radiobutton") {
        auto *rb = new QRadioButton(name, m_form);
        connect(rb, &QRadioButton::toggled, this, [this, name](bool) {
            emit eventFired(name + "_Changed");
        });
        widget = rb;
    }
    else if (t == "combobox") {
        widget = new QComboBox(m_form);
    }
    else if (t == "spinbox") {
        widget = new QSpinBox(m_form);
    }
    else if (t == "progressbar") {
        widget = new QProgressBar(m_form);
    }
    else if (t == "textedit" || t == "memo") {
        widget = new QTextEdit(m_form);
    }
    else if (t == "listwidget" || t == "listbox") {
        widget = new QListWidget(m_form);
    }
    else if (t == "groupbox") {
        widget = new QGroupBox(name, m_form);
    }
    else if (t == "frame") {
        auto *fr = new QFrame(m_form);
        fr->setFrameStyle(QFrame::Box | QFrame::Plain);
        widget = fr;
    }
    else if (t == "slider") {
        widget = new QSlider(Qt::Horizontal, m_form);
    }
    else {
        widget = new QLabel(type + ": " + name, m_form);
    }

    if (widget) {
        widget->move(x, y);
        widget->resize(w, h);
        int id = m_nextId++;
        m_widgets[id] = widget;
        m_names[id]   = name;
        return id;
    }
    return -1;
}

void FormRuntime::setProperty(int id, const QString &key, const QString &value) {
    QWidget *w = m_widgets.value(id, nullptr);
    if (!w) return;

    QString k = key.toLower();

    if (k == "text") {
        if      (auto *l  = qobject_cast<QLabel*>(w))       l->setText(value);
        else if (auto *b  = qobject_cast<QPushButton*>(w))  b->setText(value);
        else if (auto *c  = qobject_cast<QCheckBox*>(w))    c->setText(value);
        else if (auto *r  = qobject_cast<QRadioButton*>(w)) r->setText(value);
        else if (auto *e  = qobject_cast<QLineEdit*>(w))    e->setText(value);
        else if (auto *t  = qobject_cast<QTextEdit*>(w))    t->setPlainText(value);
        else if (auto *g  = qobject_cast<QGroupBox*>(w))    g->setTitle(value);
    }
    else if (k == "placeholdertext") {
        if (auto *e = qobject_cast<QLineEdit*>(w)) e->setPlaceholderText(value);
    }
    else if (k == "enabled") {
        w->setEnabled(value.toLower() != "false" && value != "0");
    }
    else if (k == "visible") {
        w->setVisible(value.toLower() != "false" && value != "0");
    }
    else if (k == "checked") {
        bool on = (value.toLower() == "true" || value == "1");
        if      (auto *c = qobject_cast<QCheckBox*>(w))    c->setChecked(on);
        else if (auto *r = qobject_cast<QRadioButton*>(w)) r->setChecked(on);
    }
    else if (k == "value") {
        int v = value.toInt();
        if      (auto *s  = qobject_cast<QSpinBox*>(w))     s->setValue(v);
        else if (auto *p  = qobject_cast<QProgressBar*>(w)) p->setValue(v);
        else if (auto *sl = qobject_cast<QSlider*>(w))      sl->setValue(v);
    }
    else if (k == "minimum") {
        int v = value.toInt();
        if      (auto *s  = qobject_cast<QSpinBox*>(w))     s->setMinimum(v);
        else if (auto *p  = qobject_cast<QProgressBar*>(w)) p->setMinimum(v);
        else if (auto *sl = qobject_cast<QSlider*>(w))      sl->setMinimum(v);
    }
    else if (k == "maximum") {
        int v = value.toInt();
        if      (auto *s  = qobject_cast<QSpinBox*>(w))     s->setMaximum(v);
        else if (auto *p  = qobject_cast<QProgressBar*>(w)) p->setMaximum(v);
        else if (auto *sl = qobject_cast<QSlider*>(w))      sl->setMaximum(v);
    }
    else if (k == "readonly") {
        bool ro = (value.toLower() == "true" || value == "1");
        if      (auto *e = qobject_cast<QLineEdit*>(w))  e->setReadOnly(ro);
        else if (auto *t = qobject_cast<QTextEdit*>(w))  t->setReadOnly(ro);
    }
}

void FormRuntime::addItem(int id, const QString &value) {
    QWidget *w = m_widgets.value(id, nullptr);
    if (!w) return;
    if      (auto *c = qobject_cast<QComboBox*>(w))   c->addItem(value);
    else if (auto *l = qobject_cast<QListWidget*>(w)) l->addItem(value);
}

void FormRuntime::showForm() {
    ensureForm();
    m_formVisible = true;
    m_form->show();
    m_form->raise();
    m_form->activateWindow();
}

void FormRuntime::clearAll() {
    if (m_form) {
        m_form->hide();
        delete m_form;
        m_form = nullptr;
    }
    m_widgets.clear();
    m_names.clear();
    m_nextId      = 1;
    m_formVisible = false;
}

// Required so moc processes the FormWindow class defined above.
#include "formruntime.moc"
