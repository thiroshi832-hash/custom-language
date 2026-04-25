#include "uidesigner.h"
#include "designcanvas.h"
#include "widgetitem.h"

#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QDockWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextEdit>
#include <QApplication>
#include <QScrollArea>
#include <QSplitter>
#include <QClipboard>

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
    : QMainWindow(parent)
{
    setWindowTitle("UI Designer");
    resize(1200, 800);
    setupUI();
    setupToolbar();
}

void UIDesigner::setupUI() {
    // ── Central canvas ───────────────────────────────────────────────────
    m_canvas = new DesignCanvas(this);
    setCentralWidget(m_canvas);

    connect(m_canvas, &DesignCanvas::itemSelected, this, &UIDesigner::onItemSelected);
    connect(m_canvas, &DesignCanvas::mouseMoved, this, [this](int x, int y){
        m_statusPos->setText(QString("  x: %1  y: %2").arg(x).arg(y));
    });

    // ── Left dock — Palette ───────────────────────────────────────────────
    auto *paletteDock = new QDockWidget("Widget Box", this);
    paletteDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    paletteDock->setMinimumWidth(160);
    paletteDock->setMaximumWidth(200);

    m_palette = new QListWidget;
    m_palette->setIconSize(QSize(18, 18));
    m_palette->setSpacing(1);

    // Populate palette with categories
    QString lastCat;
    for (const auto &e : s_palette) {
        if (e.category != lastCat) {
            auto *catItem = new QListWidgetItem(e.category);
            catItem->setFlags(Qt::NoItemFlags);
            QFont f = catItem->font(); f.setBold(true);
            catItem->setFont(f);
            catItem->setBackground(QColor("#d0d0e8"));
            catItem->setForeground(QColor("#333366"));
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

    paletteDock->setWidget(m_palette);
    addDockWidget(Qt::LeftDockWidgetArea, paletteDock);

    // ── Right dock — Property Editor ──────────────────────────────────────
    auto *propDock = new QDockWidget("Property Editor", this);
    propDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    propDock->setMinimumWidth(230);
    propDock->setMaximumWidth(300);

    auto *propWidget = new QWidget;
    auto *propLayout = new QVBoxLayout(propWidget);
    propLayout->setContentsMargins(4, 4, 4, 4);
    propLayout->setSpacing(4);

    // Form size controls at top
    auto *formBox = new QGroupBox("Form");
    auto *formLay = new QHBoxLayout(formBox);
    formLay->setContentsMargins(4, 4, 4, 4);
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
    propLayout->addWidget(formBox);

    // Property table
    m_propTable = new QTableWidget(0, 2);
    m_propTable->setHorizontalHeaderLabels({"Property", "Value"});
    m_propTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_propTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_propTable->verticalHeader()->setVisible(false);
    m_propTable->verticalHeader()->setDefaultSectionSize(22);
    m_propTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_propTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
    propLayout->addWidget(m_propTable);

    connect(m_propTable, &QTableWidget::cellChanged, this, &UIDesigner::onPropChanged);

    // Generate Code button
    auto *genBtn = new QPushButton("Generate Code");
    genBtn->setStyleSheet("QPushButton { background: #3584e4; color: white; "
                          "padding: 5px; border-radius: 3px; font-weight: bold; }");
    connect(genBtn, &QPushButton::clicked, this, &UIDesigner::generateCode);
    propLayout->addWidget(genBtn);

    propDock->setWidget(propWidget);
    addDockWidget(Qt::RightDockWidgetArea, propDock);

    // ── Status bar ────────────────────────────────────────────────────────
    m_statusPos = new QLabel("  x: 0  y: 0");
    statusBar()->addPermanentWidget(m_statusPos);
    statusBar()->showMessage("Select a widget from the palette and click on the canvas to place it.");

    // ── Dark style ────────────────────────────────────────────────────────
    setStyleSheet(
        "QMainWindow, QDockWidget { background: #2a2a3e; }"
        "QDockWidget::title { background: #313244; color: #cdd6f4; padding: 4px; }"
        "QListWidget { background: #1e1e2e; color: #cdd6f4; border: none; }"
        "QListWidget::item:selected { background: #45475a; }"
        "QListWidget::item:hover { background: #313244; }"
        "QTableWidget { background: #1e1e2e; color: #cdd6f4; gridline-color: #313244; }"
        "QTableWidget::item:selected { background: #45475a; }"
        "QHeaderView::section { background: #313244; color: #cdd6f4; padding: 3px; border: none; }"
        "QGroupBox { color: #cdd6f4; border: 1px solid #45475a; margin-top: 8px; "
        "            border-radius: 3px; padding-top: 4px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; color: #89b4fa; }"
        "QLineEdit { background: #313244; color: #cdd6f4; border: 1px solid #45475a; "
        "            padding: 2px; border-radius: 2px; }"
        "QPushButton { background: #45475a; color: #cdd6f4; border: none; "
        "              padding: 4px 8px; border-radius: 3px; }"
        "QPushButton:hover { background: #585b70; }"
        "QStatusBar { background: #313244; color: #cdd6f4; }"
        "QToolBar { background: #313244; border: none; spacing: 3px; padding: 2px; }"
        "QToolButton { background: #45475a; color: #cdd6f4; padding: 4px 8px; "
        "              border-radius: 3px; }"
        "QToolButton:hover    { background: #585b70; }"
        "QToolButton:checked  { background: #3584e4; color: white; }"
        "QMenuBar { background: #313244; color: #cdd6f4; }"
        "QMenuBar::item:selected { background: #45475a; }"
        "QMenu { background: #313244; color: #cdd6f4; }"
        "QMenu::item:selected { background: #45475a; }"
        "QLabel { color: #cdd6f4; }"
    );
}

void UIDesigner::setupToolbar() {
    QToolBar *tb = addToolBar("Tools");
    tb->setMovable(false);

    m_actSelect = tb->addAction("Arrow");
    m_actSelect->setCheckable(true);
    m_actSelect->setChecked(true);
    m_actSelect->setToolTip("Select / Move  (Esc)");
    connect(m_actSelect, &QAction::triggered, this, &UIDesigner::selectTool);

    tb->addSeparator();
    tb->addAction("Delete", this, [this]{ m_canvas->deleteSelected(); })->setToolTip("Delete selected (Del)");
    tb->addSeparator();

    auto *gridAct = tb->addAction("Grid");
    gridAct->setCheckable(true);
    gridAct->setChecked(true);
    connect(gridAct, &QAction::toggled, m_canvas->formScene(), &FormScene::setGridVisible);

    tb->addSeparator();
    tb->addAction("Clear All", this, [this]{
        m_canvas->formScene()->clear();
        onItemSelected(nullptr);
        statusBar()->showMessage("Canvas cleared.");
    });
    tb->addSeparator();
    tb->addAction("Generate Code", this, &UIDesigner::generateCode);

    // Menu
    auto *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("New Form", this, &UIDesigner::newForm, QKeySequence::New);
    fileMenu->addSeparator();
    fileMenu->addAction("Generate && Copy Code", this, &UIDesigner::generateCode);
    fileMenu->addSeparator();
    fileMenu->addAction("Close", this, &QWidget::close, QKeySequence::Close);

    auto *editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction("Delete Selected", this, [this]{ m_canvas->deleteSelected(); }, QKeySequence::Delete);
    editMenu->addAction("Select All", this, [this]{
        for (auto *it : m_canvas->formScene()->items()) it->setSelected(true);
    }, QKeySequence::SelectAll);
}

// ── Tool selection ────────────────────────────────────────────────────────

void UIDesigner::selectTool() {
    m_canvas->setActiveTool(QString());
    m_actSelect->setChecked(true);
    m_palette->clearSelection();
    statusBar()->showMessage("Select mode. Click a widget to select it.");
}

void UIDesigner::onToolSelected(const QString &type) {
    m_canvas->setActiveTool(type);
    m_actSelect->setChecked(false);
    statusBar()->showMessage(
        QString("Click on the canvas to place a %1. Press Esc to return to select mode.").arg(type));
}

// ── Property panel ────────────────────────────────────────────────────────

void UIDesigner::onItemSelected(WidgetItem *item) {
    m_selectedItem = item;
    populateProps(item);
    if (item)
        statusBar()->showMessage(
            QString("%1 \"%2\"  |  x:%3  y:%4  w:%5  h:%6")
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
        keyItem->setForeground(QColor("#89b4fa"));
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
        // Parse "x, y, w, h"
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

    // Refresh geometry row
    m_updatingProps = true;
    int geomRow = -1;
    for (int i = 0; i < m_propTable->rowCount(); ++i)
        if (m_propTable->item(i, 0) && m_propTable->item(i, 0)->text() == "geometry")
            geomRow = i;
    if (geomRow >= 0 && m_propTable->item(geomRow, 1))
        m_propTable->item(geomRow, 1)->setText(propValue(m_selectedItem, "geometry"));
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

    // ── 1. Variable declarations (module-level globals) ───────────────────
    if (!items.isEmpty()) {
        code += "' --- Variable declarations ---\n";
        for (auto *w : items)
            code += QString("Dim %1\n").arg(w->objectName());
        code += "\n";
    }

    // ── 2. Create & configure controls in the main body ───────────────────
    // Running at module scope means assignments go to globals, so event
    // handlers can read e.g. btn1 to get the widget ID.
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

            // Text / title
            if (type == "Label" || type == "PushButton" || type == "CheckBox" ||
                type == "RadioButton" || type == "GroupBox") {
                QString txt = w->prop("text");
                if (txt.isEmpty()) txt = name; // default caption = name
                code += QString("SetProperty %1, \"text\", \"%2\"\n").arg(name).arg(txt);
            }
            // Placeholder
            if (type == "LineEdit") {
                QString ph = w->prop("placeholderText");
                if (!ph.isEmpty())
                    code += QString("SetProperty %1, \"placeholderText\", \"%2\"\n")
                            .arg(name).arg(ph);
            }
            // Numeric range
            if (type == "SpinBox" || type == "ProgressBar" || type == "Slider") {
                QString mn = w->prop("minimum"), mx = w->prop("maximum"), vl = w->prop("value");
                if (!mn.isEmpty()) code += QString("SetProperty %1, \"minimum\", \"%2\"\n").arg(name).arg(mn);
                if (!mx.isEmpty()) code += QString("SetProperty %1, \"maximum\", \"%2\"\n").arg(name).arg(mx);
                if (!vl.isEmpty()) code += QString("SetProperty %1, \"value\",   \"%2\"\n").arg(name).arg(vl);
            }
            // List items (one per line in the "items" property)
            if (type == "ComboBox" || type == "ListWidget") {
                QStringList its = w->prop("items").split('\n');
                for (const QString &it : its)
                    if (!it.trimmed().isEmpty())
                        code += QString("AddItem %1, \"%2\"\n").arg(name).arg(it.trimmed());
            }
            // Disabled state
            if (w->prop("enabled").toLower() == "false")
                code += QString("SetProperty %1, \"enabled\", \"false\"\n").arg(name);

            code += "\n";
        }
    }

    // ── 3. Show the form — this blocks until the user closes the window ───
    code += "' --- Show form (blocks until closed) ---\n";
    code += QString("ShowForm \"My Form\", %1, %2\n\n").arg(fw).arg(fh);

    // ── 4. Event-handler stubs ─────────────────────────────────────────────
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

    // ── Show in viewer window ─────────────────────────────────────────────
    auto *dlg = new QMainWindow(this);
    dlg->setWindowTitle("Generated Code");
    dlg->resize(680, 560);
    auto *te = new QTextEdit(dlg);
    te->setFont(QFont("Courier New", 9));
    te->setPlainText(code);
    te->setStyleSheet("QTextEdit { background:#1e1e2e; color:#cdd6f4; }");
    dlg->setCentralWidget(te);

    auto *tb = dlg->addToolBar("Actions");
    tb->setStyleSheet("QToolBar { background:#313244; border:none; } "
                      "QToolButton { background:#45475a; color:#cdd6f4; "
                      "padding:4px 10px; border-radius:3px; }");
    tb->addAction("Copy All", dlg, [te]{
        QApplication::clipboard()->setText(te->toPlainText());
    });
    tb->addAction("Close", dlg, &QWidget::close);

    dlg->show();
}
