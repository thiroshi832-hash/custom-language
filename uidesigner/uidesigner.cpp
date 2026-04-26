#include "uidesigner.h"
#include "designcanvas.h"
#include "widgetitem.h"

#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QToolBar>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QAction>
#include <QMessageBox>
#include <QTextEdit>
#include <QApplication>
#include <QSplitter>
#include <QClipboard>
#include <QMainWindow>     // only for the generate-code viewer child window

// ── Qt Creator colour constants (match mainwindow.cpp) ────────────────────
namespace QCD {
    static const char BG[]      = "#2b2b2b";
    static const char PANEL[]   = "#3c3f41";
    static const char ACTIVE[]  = "#4e5254";
    static const char BORDER[]  = "#323232";
    static const char TEXT[]    = "#a9b7c6";
    static const char DIM[]     = "#606366";
    static const char ACCENT[]  = "#3592c4";
    static const char CONSOLE[] = "#1e1e1e";
}

// ── Widget palette entries ────────────────────────────────────────────────

struct PaletteEntry { QString type, label, category; };
static const QVector<PaletteEntry> s_palette = {
    // Buttons
    { "PushButton",  "Push Button",   "Buttons" },
    { "CheckBox",    "Check Box",     "Buttons" },
    { "RadioButton", "Radio Button",  "Buttons" },
    // Input
    { "LineEdit",    "Line Edit",     "Input Widgets" },
    { "TextEdit",    "Text Edit",     "Input Widgets" },
    { "SpinBox",     "Spin Box",      "Input Widgets" },
    { "ComboBox",    "Combo Box",     "Input Widgets" },
    { "Slider",      "Slider",        "Input Widgets" },
    // Display
    { "Label",       "Label",         "Display Widgets" },
    { "ProgressBar", "Progress Bar",  "Display Widgets" },
    { "ListWidget",  "List Widget",   "Display Widgets" },
    // Containers
    { "GroupBox",    "Group Box",     "Containers" },
    { "Frame",       "Frame",         "Containers" },
};

// ── Property definitions per widget type ──────────────────────────────────

static QStringList propsFor(const QString &type) {
    QStringList common = { "objectName", "geometry", "enabled", "visible" };
    QStringList text   = { "text" };

    if (type == "Label")        return common + text + QStringList{"alignment"};
    if (type == "PushButton")   return common + text;
    if (type == "CheckBox")     return common + text + QStringList{"checked"};
    if (type == "RadioButton")  return common + text + QStringList{"checked"};
    if (type == "LineEdit")     return common + text + QStringList{"placeholderText", "readOnly"};
    if (type == "TextEdit")     return common + text + QStringList{"readOnly"};
    if (type == "ComboBox")     return common + QStringList{"items"};
    if (type == "SpinBox")      return common + QStringList{"minimum", "maximum", "value"};
    if (type == "ProgressBar")  return common + QStringList{"minimum", "maximum", "value"};
    if (type == "Slider")       return common + QStringList{"minimum", "maximum", "value"};
    if (type == "GroupBox")     return common + text;
    if (type == "ListWidget")   return common + QStringList{"items"};
    return common;
}

static QString propValue(WidgetItem *item, const QString &key) {
    if (key == "objectName") return item->objectName();
    if (key == "geometry")
        return QString("%1, %2, %3, %4")
               .arg((int)item->x()).arg((int)item->y())
               .arg((int)item->itemW()).arg((int)item->itemH());
    return item->prop(key);
}

// ── UIDesigner ────────────────────────────────────────────────────────────

UIDesigner::UIDesigner(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void UIDesigner::setupUI() {
    // ── Overall layout: toolbar / splitter / status-bar ───────────────────
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── Toolbar ───────────────────────────────────────────────────────────
    auto *toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->setStyleSheet(
        QString("QToolBar { background:%1; border:none; border-bottom:1px solid %2;"
                "  spacing:1px; padding:2px 4px; }"
                "QToolButton { background:transparent; color:%3;"
                "  border:none; padding:4px 8px; font-size:9pt; }"
                "QToolButton:hover { background:%4; }"
                "QToolButton:checked { background:%5; color:#ffffff; }"
                "QToolBar::separator { background:%2; width:1px; margin:4px 3px; }")
        .arg(QCD::PANEL).arg(QCD::BORDER).arg(QCD::TEXT)
        .arg(QCD::ACTIVE).arg(QCD::ACCENT));

    m_actSelect = toolbar->addAction("Arrow");
    m_actSelect->setCheckable(true);
    m_actSelect->setChecked(true);
    m_actSelect->setToolTip("Select / Move  (Esc)");
    connect(m_actSelect, &QAction::triggered, this, &UIDesigner::selectTool);

    toolbar->addSeparator();
    toolbar->addAction("Delete", this, [this]{ m_canvas->deleteSelected(); })->setToolTip("Delete selected (Del)");
    toolbar->addSeparator();

    auto *gridAct = toolbar->addAction("Grid");
    gridAct->setCheckable(true);
    gridAct->setChecked(true);
    connect(gridAct, &QAction::toggled, this, [this](bool on){
        m_canvas->formScene()->setGridVisible(on);
    });

    toolbar->addSeparator();
    toolbar->addAction("Clear All", this, [this]{
        m_canvas->formScene()->clear();
        onItemSelected(nullptr);
        setStatus("Canvas cleared.");
    });
    toolbar->addSeparator();
    toolbar->addAction("New Form",      this, &UIDesigner::newForm);
    toolbar->addAction("Generate Code", this, &UIDesigner::generateCode);

    mainLayout->addWidget(toolbar);

    // ── Horizontal splitter: palette | canvas | properties ────────────────
    auto *hSplit = new QSplitter(Qt::Horizontal, this);
    hSplit->setStyleSheet(
        QString("QSplitter::handle { background:%1; width:1px; }").arg(QCD::BORDER));

    // Left panel — Widget palette
    auto *palettePanel = new QWidget;
    palettePanel->setMinimumWidth(150);
    palettePanel->setMaximumWidth(200);
    palettePanel->setStyleSheet(
        QString("QWidget { background:%1; }").arg(QCD::PANEL));

    auto *palLayout = new QVBoxLayout(palettePanel);
    palLayout->setContentsMargins(0, 0, 0, 0);
    palLayout->setSpacing(0);

    auto *palTitle = new QLabel("  Widget Box");
    palTitle->setFixedHeight(24);
    palTitle->setStyleSheet(
        QString("QLabel { background:%1; color:%2; font-weight:bold;"
                "  border-bottom:1px solid %3; padding:2px; }")
        .arg(QCD::BORDER).arg(QCD::TEXT).arg(QCD::BORDER));
    palLayout->addWidget(palTitle);

    m_palette = new QListWidget;
    m_palette->setIconSize(QSize(18, 18));
    m_palette->setSpacing(1);
    m_palette->setStyleSheet(
        QString("QListWidget { background:%1; color:%2; border:none; }"
                "QListWidget::item:selected { background:%3; }"
                "QListWidget::item:hover    { background:%4; }")
        .arg(QCD::BG).arg(QCD::TEXT).arg(QCD::ACTIVE).arg(QCD::BORDER));

    QString lastCat;
    for (const auto &e : s_palette) {
        if (e.category != lastCat) {
            auto *catItem = new QListWidgetItem(e.category);
            catItem->setFlags(Qt::NoItemFlags);
            QFont f = catItem->font(); f.setBold(true);
            catItem->setFont(f);
            catItem->setBackground(QColor(QCD::BORDER));
            catItem->setForeground(QColor(QCD::ACCENT));
            m_palette->addItem(catItem);
            lastCat = e.category;
        }
        auto *item = new QListWidgetItem("  " + e.label);
        item->setData(Qt::UserRole, e.type);
        m_palette->addItem(item);
    }

    connect(m_palette, &QListWidget::itemClicked, this, [this](QListWidgetItem *item){
        QString type = item->data(Qt::UserRole).toString();
        if (!type.isEmpty()) onToolSelected(type);
    });

    palLayout->addWidget(m_palette);
    hSplit->addWidget(palettePanel);

    // Centre — Design canvas
    m_canvas = new DesignCanvas(this);
    hSplit->addWidget(m_canvas);

    connect(m_canvas, &DesignCanvas::itemSelected, this, &UIDesigner::onItemSelected);
    connect(m_canvas, &DesignCanvas::mouseMoved, this, [this](int x, int y){
        m_statusPos->setText(QString("  x: %1  y: %2  ").arg(x).arg(y));
    });

    // Right panel — Property editor
    auto *propPanel = new QWidget;
    propPanel->setMinimumWidth(220);
    propPanel->setMaximumWidth(300);
    auto *propLayout = new QVBoxLayout(propPanel);
    propLayout->setContentsMargins(0, 0, 0, 0);
    propLayout->setSpacing(0);

    auto *propTitle = new QLabel("  Property Editor");
    propTitle->setFixedHeight(24);
    propTitle->setStyleSheet(
        QString("QLabel { background:%1; color:%2; font-weight:bold;"
                "  border-bottom:1px solid %3; padding:2px; }")
        .arg(QCD::BORDER).arg(QCD::TEXT).arg(QCD::BORDER));
    propLayout->addWidget(propTitle);

    // Form size controls
    auto *formBox = new QGroupBox("Form Size");
    formBox->setStyleSheet(
        QString("QGroupBox { color:%1; border:1px solid %2; margin-top:10px;"
                "  border-radius:3px; }"
                "QGroupBox::title { subcontrol-origin:margin; left:8px; color:%3; }"
                "QLineEdit { background:%4; color:%1; border:1px solid %2;"
                "  padding:2px; border-radius:2px; }"
                "QPushButton { background:%2; color:%1; border:none;"
                "  padding:3px 8px; border-radius:2px; }"
                "QPushButton:hover { background:%5; }")
        .arg(QCD::TEXT).arg(QCD::BORDER).arg(QCD::ACCENT).arg(QCD::BG).arg(QCD::ACTIVE));
    auto *formLay = new QHBoxLayout(formBox);
    formLay->setContentsMargins(6, 4, 6, 4);
    formLay->addWidget(new QLabel("W:"));
    m_formWEdit = new QLineEdit("640"); m_formWEdit->setMaximumWidth(50);
    formLay->addWidget(m_formWEdit);
    formLay->addWidget(new QLabel("H:"));
    m_formHEdit = new QLineEdit("480"); m_formHEdit->setMaximumWidth(50);
    formLay->addWidget(m_formHEdit);
    auto *applyBtn = new QPushButton("Apply");
    applyBtn->setMaximumWidth(45);
    connect(applyBtn, &QPushButton::clicked, this, &UIDesigner::applyFormSize);
    formLay->addWidget(applyBtn);

    auto *formBoxWrapper = new QWidget;
    auto *fbwl = new QVBoxLayout(formBoxWrapper);
    fbwl->setContentsMargins(4, 4, 4, 0);
    fbwl->addWidget(formBox);
    propLayout->addWidget(formBoxWrapper);

    // Property table
    m_propTable = new QTableWidget(0, 2);
    m_propTable->setHorizontalHeaderLabels({"Property", "Value"});
    m_propTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_propTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_propTable->verticalHeader()->setVisible(false);
    m_propTable->verticalHeader()->setDefaultSectionSize(22);
    m_propTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_propTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    m_propTable->setStyleSheet(
        QString("QTableWidget { background:%1; color:%2; gridline-color:%3; border:none; }"
                "QTableWidget::item:selected { background:%4; }"
                "QHeaderView::section { background:%3; color:%2; padding:3px; border:none; }")
        .arg(QCD::BG).arg(QCD::TEXT).arg(QCD::BORDER).arg(QCD::ACTIVE));
    connect(m_propTable, &QTableWidget::cellChanged, this, &UIDesigner::onPropChanged);
    propLayout->addWidget(m_propTable, 1);

    // Generate Code button
    auto *genBtn = new QPushButton("Generate Code");
    genBtn->setStyleSheet(
        QString("QPushButton { background:%1; color:#ffffff; padding:6px;"
                "  border:none; border-radius:3px; font-weight:bold; }"
                "QPushButton:hover { background:%2; }")
        .arg(QCD::ACCENT).arg(QCD::ACTIVE));
    connect(genBtn, &QPushButton::clicked, this, &UIDesigner::generateCode);
    auto *genWrapper = new QWidget;
    auto *gwl = new QVBoxLayout(genWrapper);
    gwl->setContentsMargins(4, 4, 4, 4);
    gwl->addWidget(genBtn);
    propLayout->addWidget(genWrapper);

    hSplit->addWidget(propPanel);

    // Splitter proportions: palette narrow, canvas stretchy, props narrow
    hSplit->setStretchFactor(0, 0);
    hSplit->setStretchFactor(1, 1);
    hSplit->setStretchFactor(2, 0);

    mainLayout->addWidget(hSplit, 1);

    // ── Status bar at the bottom ──────────────────────────────────────────
    auto *statusWidget = new QWidget;
    statusWidget->setFixedHeight(22);
    statusWidget->setStyleSheet(
        QString("QWidget { background:%1; border-top:1px solid %2; }")
        .arg(QCD::PANEL).arg(QCD::BORDER));
    auto *statusLay = new QHBoxLayout(statusWidget);
    statusLay->setContentsMargins(4, 0, 4, 0);
    statusLay->setSpacing(0);

    m_statusLabel = new QLabel("Select a widget from the palette and click on the canvas to place it.");
    m_statusLabel->setStyleSheet(QString("color:%1; background:transparent;").arg(QCD::TEXT));
    statusLay->addWidget(m_statusLabel, 1);

    m_statusPos = new QLabel("  x: 0  y: 0  ");
    m_statusPos->setStyleSheet(QString("color:%1; background:transparent;").arg(QCD::DIM));
    statusLay->addWidget(m_statusPos);

    mainLayout->addWidget(statusWidget);
}

void UIDesigner::setStatus(const QString &msg) {
    if (m_statusLabel) m_statusLabel->setText(msg);
}

// ── Tool selection ────────────────────────────────────────────────────────

void UIDesigner::selectTool() {
    m_canvas->setActiveTool(QString());
    m_actSelect->setChecked(true);
    m_palette->clearSelection();
    setStatus("Select mode. Click a widget to select it.");
}

void UIDesigner::onToolSelected(const QString &type) {
    m_canvas->setActiveTool(type);
    m_actSelect->setChecked(false);
    setStatus(QString("Click on the canvas to place a %1. Press Esc to return to select mode.").arg(type));
}

// ── Property panel ────────────────────────────────────────────────────────

void UIDesigner::onItemSelected(WidgetItem *item) {
    m_selectedItem = item;
    populateProps(item);
    if (item)
        setStatus(QString("%1 \"%2\"  |  x:%3  y:%4  w:%5  h:%6")
            .arg(item->widgetType()).arg(item->objectName())
            .arg((int)item->x()).arg((int)item->y())
            .arg((int)item->itemW()).arg((int)item->itemH()));
}

void UIDesigner::populateProps(WidgetItem *item) {
    m_updatingProps = true;
    m_propTable->setRowCount(0);

    if (!item) { m_updatingProps = false; return; }

    QStringList keys = propsFor(item->widgetType());
    m_propTable->setRowCount(keys.size());

    for (int i = 0; i < keys.size(); ++i) {
        const QString &key = keys[i];
        auto *keyItem = new QTableWidgetItem(key);
        keyItem->setFlags(Qt::ItemIsEnabled);
        keyItem->setForeground(QColor(QCD::ACCENT));
        m_propTable->setItem(i, 0, keyItem);

        auto *valItem = new QTableWidgetItem(propValue(item, key));
        m_propTable->setItem(i, 1, valItem);
    }

    m_updatingProps = false;
}

void UIDesigner::onPropChanged(int row, int col) {
    if (m_updatingProps || col != 1 || !m_selectedItem) return;

    auto *keyItem = m_propTable->item(row, 0);
    auto *valItem = m_propTable->item(row, 1);
    if (!keyItem || !valItem) return;

    const QString key = keyItem->text();
    const QString val = valItem->text();

    if (key == "objectName") {
        m_selectedItem->setObjectName(val);
    } else if (key == "geometry") {
        QStringList parts = val.split(',');
        if (parts.size() == 4) {
            m_selectedItem->setPos(parts[0].trimmed().toDouble(),
                                   parts[1].trimmed().toDouble());
            m_selectedItem->setItemSize(parts[2].trimmed().toDouble(),
                                        parts[3].trimmed().toDouble());
        }
    } else {
        m_selectedItem->setProp(key, val);
    }

    // Refresh the geometry row display
    m_updatingProps = true;
    for (int i = 0; i < m_propTable->rowCount(); ++i) {
        if (m_propTable->item(i, 0) && m_propTable->item(i, 0)->text() == "geometry")
            if (m_propTable->item(i, 1))
                m_propTable->item(i, 1)->setText(propValue(m_selectedItem, "geometry"));
    }
    m_updatingProps = false;
}

// ── Form size ─────────────────────────────────────────────────────────────

void UIDesigner::applyFormSize() {
    int w = m_formWEdit->text().toInt();
    int h = m_formHEdit->text().toInt();
    if (w < 100) w = 100;
    if (h < 100) h = 100;
    m_canvas->formScene()->setFormSize(w, h);
}

// ── New form ──────────────────────────────────────────────────────────────

void UIDesigner::newForm() {
    auto ret = QMessageBox::question(this, "New Form",
        "Clear the canvas and start a new form?",
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        m_canvas->formScene()->clear();
        onItemSelected(nullptr);
        setStatus("New form created.");
    }
}

// ── Code generator ────────────────────────────────────────────────────────

void UIDesigner::generateCode() {
    auto items = m_canvas->widgetItems();
    int fw = m_canvas->formScene()->formWidth();
    int fh = m_canvas->formScene()->formHeight();

    QString code;
    code += "' ============================================================\n";
    code += "' Generated by UI Designer\n";
    code += "' Paste into the IDE editor, then press Build & Run (Ctrl+F5)\n";
    code += "' ============================================================\n\n";

    if (!items.isEmpty()) {
        code += "' --- Variable declarations ---\n";
        for (auto *w : items)
            code += QString("Dim %1\n").arg(w->objectName());
        code += "\n";
    }

    if (!items.isEmpty()) {
        code += "' --- Create controls ---\n";
        for (auto *w : items) {
            int x  = (int)w->x(),    y  = (int)w->y();
            int wd = (int)w->itemW(), ht = (int)w->itemH();
            const QString &name = w->objectName();
            const QString &type = w->widgetType();

            code += QString("' %1 \"%2\"\n").arg(type).arg(name);
            code += QString("%1 = CreateControl(\"%2\", \"%3\", %4, %5, %6, %7)\n")
                    .arg(name).arg(type).arg(name).arg(x).arg(y).arg(wd).arg(ht);

            if (type == "Label" || type == "PushButton" || type == "CheckBox" ||
                type == "RadioButton" || type == "GroupBox") {
                QString txt = w->prop("text");
                if (txt.isEmpty()) txt = name;
                code += QString("SetProperty %1, \"text\", \"%2\"\n").arg(name).arg(txt);
            }
            if (type == "LineEdit") {
                QString ph = w->prop("placeholderText");
                if (!ph.isEmpty())
                    code += QString("SetProperty %1, \"placeholderText\", \"%2\"\n")
                            .arg(name).arg(ph);
            }
            if (type == "SpinBox" || type == "ProgressBar" || type == "Slider") {
                QString mn = w->prop("minimum"), mx = w->prop("maximum"), vl = w->prop("value");
                if (!mn.isEmpty()) code += QString("SetProperty %1, \"minimum\", \"%2\"\n").arg(name).arg(mn);
                if (!mx.isEmpty()) code += QString("SetProperty %1, \"maximum\", \"%2\"\n").arg(name).arg(mx);
                if (!vl.isEmpty()) code += QString("SetProperty %1, \"value\",   \"%2\"\n").arg(name).arg(vl);
            }
            if (type == "ComboBox" || type == "ListWidget") {
                QStringList its = w->prop("items").split('\n');
                for (const QString &it : its)
                    if (!it.trimmed().isEmpty())
                        code += QString("AddItem %1, \"%2\"\n").arg(name).arg(it.trimmed());
            }
            if (w->prop("enabled").toLower() == "false")
                code += QString("SetProperty %1, \"enabled\", \"false\"\n").arg(name);

            code += "\n";
        }
    }

    code += "' --- Show form (blocks until closed) ---\n";
    code += QString("ShowForm \"My Form\", %1, %2\n\n").arg(fw).arg(fh);

    bool hasStubs = false;
    for (auto *w : items) {
        if (w->widgetType() == "PushButton") {
            if (!hasStubs) { code += "' --- Event handlers ---\n\n"; hasStubs = true; }
            code += QString("Sub %1_Click()\n    ' TODO\nEnd Sub\n\n").arg(w->objectName());
        }
        if (w->widgetType() == "CheckBox" || w->widgetType() == "RadioButton") {
            if (!hasStubs) { code += "' --- Event handlers ---\n\n"; hasStubs = true; }
            code += QString("Sub %1_Changed()\n    ' TODO\nEnd Sub\n\n").arg(w->objectName());
        }
    }

    // Show code in a floating viewer window
    auto *dlg = new QMainWindow(this);
    dlg->setWindowTitle("Generated Code");
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->resize(680, 560);

    auto *te = new QTextEdit(dlg);
    te->setFont(QFont("Consolas", 9));
    te->setPlainText(code);
    te->setStyleSheet(
        QString("QTextEdit { background:%1; color:#a9b7c6; }").arg(QCD::CONSOLE));
    dlg->setCentralWidget(te);

    auto *tb = dlg->addToolBar("Actions");
    tb->setStyleSheet(
        QString("QToolBar { background:%1; border:none; } "
                "QToolButton { background:%2; color:#a9b7c6; padding:4px 10px; border-radius:3px; }"
                "QToolButton:hover { background:%3; }")
        .arg(QCD::PANEL).arg(QCD::BORDER).arg(QCD::ACTIVE));
    tb->addAction("Copy All", dlg, [te]{
        QApplication::clipboard()->setText(te->toPlainText());
    });
    tb->addAction("Close", dlg, &QWidget::close);

    dlg->show();
    setStatus("Code generated — copy it into the editor.");
}
