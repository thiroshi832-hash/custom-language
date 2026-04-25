#include "designcanvas.h"
#include "widgetitem.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QScrollBar>

// ── FormScene ─────────────────────────────────────────────────────────────

FormScene::FormScene(int formW, int formH, QObject *parent)
    : QGraphicsScene(parent), m_formW(formW), m_formH(formH)
{
    setSceneRect(-40, -40, formW + 80, formH + 80);
}

void FormScene::setFormSize(int w, int h) {
    m_formW = w; m_formH = h;
    setSceneRect(-40, -40, w + 80, h + 80);
    update();
}

void FormScene::drawBackground(QPainter *p, const QRectF &rect) {
    // Outer area
    p->fillRect(rect, QColor("#4a4a5a"));

    // Form surface
    QRectF form(0, 0, m_formW, m_formH);
    p->fillRect(form, QColor("#f0f0f8"));

    // Grid
    if (m_showGrid) {
        p->setPen(QPen(QColor("#d0d0e0"), 0.5));
        int gs = m_gridSpacing;
        for (int x = 0; x <= m_formW; x += gs)
            p->drawLine(QLineF(x, 0, x, m_formH));
        for (int y = 0; y <= m_formH; y += gs)
            p->drawLine(QLineF(0, y, m_formW, y));
    }

    // Form border
    p->setPen(QPen(QColor("#7878aa"), 1.5));
    p->setBrush(Qt::NoBrush);
    p->drawRect(form);
}

// ── DesignCanvas ──────────────────────────────────────────────────────────

DesignCanvas::DesignCanvas(QWidget *parent)
    : QGraphicsView(parent)
{
    m_scene = new FormScene(640, 480, this);
    setScene(m_scene);

    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::RubberBandDrag);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setTransformationAnchor(AnchorUnderMouse);
    setResizeAnchor(AnchorViewCenter);

    connect(m_scene, &QGraphicsScene::selectionChanged, this, [this](){
        auto items = m_scene->selectedItems();
        WidgetItem *sel = nullptr;
        for (auto *it : items)
            if (it->type() == WidgetItem::Type)
                sel = static_cast<WidgetItem*>(it);
        emit itemSelected(sel);
    });
}

void DesignCanvas::setActiveTool(const QString &widgetType) {
    m_activeTool = widgetType;
    if (widgetType.isEmpty()) {
        setDragMode(QGraphicsView::RubberBandDrag);
        setCursor(Qt::ArrowCursor);
    } else {
        setDragMode(QGraphicsView::NoDrag);
        setCursor(Qt::CrossCursor);
    }
}

int DesignCanvas::snapToGrid(qreal v) const {
    int gs = m_scene->gridSpacing();
    return (int(v) / gs) * gs;
}

QSizeF DesignCanvas::defaultSize(const QString &type) {
    if (type == "Label")       return { 120, 22 };
    if (type == "PushButton")  return { 90,  28 };
    if (type == "LineEdit")    return { 150, 26 };
    if (type == "TextEdit")    return { 200, 80 };
    if (type == "CheckBox")    return { 110, 22 };
    if (type == "RadioButton") return { 110, 22 };
    if (type == "ComboBox")    return { 130, 26 };
    if (type == "SpinBox")     return { 80,  26 };
    if (type == "ProgressBar") return { 180, 22 };
    if (type == "Slider")      return { 160, 22 };
    if (type == "GroupBox")    return { 200, 120 };
    if (type == "Frame")       return { 160, 100 };
    if (type == "ListWidget")  return { 150, 120 };
    return { 120, 30 };
}

QString DesignCanvas::uniqueName(const QString &type) const {
    // Abbreviation prefix
    QString prefix;
    if (type == "Label")       prefix = "lbl";
    else if (type == "PushButton")  prefix = "btn";
    else if (type == "LineEdit")    prefix = "txt";
    else if (type == "TextEdit")    prefix = "te";
    else if (type == "CheckBox")    prefix = "chk";
    else if (type == "RadioButton") prefix = "rdo";
    else if (type == "ComboBox")    prefix = "cbo";
    else if (type == "SpinBox")     prefix = "spn";
    else if (type == "ProgressBar") prefix = "prg";
    else if (type == "Slider")      prefix = "sld";
    else if (type == "GroupBox")    prefix = "grp";
    else if (type == "Frame")       prefix = "frm";
    else if (type == "ListWidget")  prefix = "lst";
    else prefix = type.toLower().left(3);

    QSet<QString> existing;
    for (auto *it : m_scene->items())
        if (it->type() == WidgetItem::Type)
            existing.insert(static_cast<WidgetItem*>(it)->objectName());

    int n = 1;
    while (existing.contains(prefix + QString::number(n))) ++n;
    return prefix + QString::number(n);
}

void DesignCanvas::placeWidget(const QString &type, QPointF scenePos) {
    QSizeF sz = defaultSize(type);
    int x = snapToGrid(scenePos.x());
    int y = snapToGrid(scenePos.y());

    // Keep inside form
    x = qBound(0, x, m_scene->formWidth()  - (int)sz.width());
    y = qBound(0, y, m_scene->formHeight() - (int)sz.height());

    QString name = uniqueName(type);
    auto *item = new WidgetItem(type, name, x, y, sz.width(), sz.height());

    connect(item, &WidgetItem::itemSelected,    this, &DesignCanvas::itemSelected);
    connect(item, &WidgetItem::geometryChanged, this, [this](WidgetItem *w){
        emit itemSelected(w); // refresh property panel
    });

    m_scene->addItem(item);
    m_scene->clearSelection();
    item->setSelected(true);
    emit itemPlaced(item);
}

void DesignCanvas::deleteSelected() {
    for (auto *it : m_scene->selectedItems()) {
        m_scene->removeItem(it);
        delete it;
    }
    emit itemSelected(nullptr);
}

QList<WidgetItem*> DesignCanvas::widgetItems() const {
    QList<WidgetItem*> out;
    for (auto *it : m_scene->items())
        if (it->type() == WidgetItem::Type)
            out.append(static_cast<WidgetItem*>(it));
    return out;
}

void DesignCanvas::mousePressEvent(QMouseEvent *e) {
    if (!m_activeTool.isEmpty() && e->button() == Qt::LeftButton) {
        QPointF sp = mapToScene(e->pos());
        if (QRectF(0,0,m_scene->formWidth(),m_scene->formHeight()).contains(sp))
            placeWidget(m_activeTool, sp);
        // Stay in current tool mode (Qt Designer behaviour)
        return;
    }
    QGraphicsView::mousePressEvent(e);
}

void DesignCanvas::mouseMoveEvent(QMouseEvent *e) {
    QPointF sp = mapToScene(e->pos());
    emit mouseMoved((int)sp.x(), (int)sp.y());
    QGraphicsView::mouseMoveEvent(e);
}

void DesignCanvas::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace) {
        deleteSelected();
        return;
    }
    // Arrow key nudge
    QPointF delta;
    int step = (e->modifiers() & Qt::ShiftModifier) ? 10 : 1;
    if (e->key() == Qt::Key_Left)  delta = {-step, 0};
    if (e->key() == Qt::Key_Right) delta = { step, 0};
    if (e->key() == Qt::Key_Up)    delta = {0, -step};
    if (e->key() == Qt::Key_Down)  delta = {0,  step};
    if (!delta.isNull()) {
        for (auto *it : m_scene->selectedItems())
            it->moveBy(delta.x(), delta.y());
        return;
    }
    QGraphicsView::keyPressEvent(e);
}
