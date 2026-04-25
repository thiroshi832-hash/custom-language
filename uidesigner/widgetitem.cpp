#include "widgetitem.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>
#include <QApplication>
#include <QtMath>

// ── Construction ───────────────────────────────────────────────────────────

static QMap<QString,QString> defaultProps(const QString &type, const QString &name) {
    QMap<QString,QString> p;
    p["enabled"] = "true";
    p["visible"] = "true";

    if (type == "Label")        p["text"] = name;
    else if (type == "PushButton") p["text"] = name;
    else if (type == "CheckBox")   p["text"] = "CheckBox";
    else if (type == "RadioButton")p["text"] = "RadioButton";
    else if (type == "GroupBox")   p["text"] = "GroupBox";
    else if (type == "LineEdit")   { p["text"] = ""; p["placeholderText"] = ""; }
    else if (type == "TextEdit")   p["text"] = "";
    else if (type == "ComboBox")   p["items"] = "Item 1\nItem 2\nItem 3";
    else if (type == "SpinBox")    { p["minimum"] = "0"; p["maximum"] = "99"; p["value"] = "0"; }
    else if (type == "ProgressBar"){ p["minimum"] = "0"; p["maximum"] = "100"; p["value"] = "50"; }
    else if (type == "Slider")     { p["minimum"] = "0"; p["maximum"] = "100"; p["value"] = "50"; }
    else if (type == "Frame")      p["frameShape"] = "Box";
    else if (type == "ListWidget") p["items"] = "Item 1\nItem 2\nItem 3";
    return p;
}

WidgetItem::WidgetItem(const QString &widgetType, const QString &name,
                       qreal x, qreal y, qreal w, qreal h,
                       QGraphicsItem *parent)
    : QGraphicsObject(parent), m_type(widgetType), m_name(name), m_w(w), m_h(h)
{
    setPos(x, y);
    setFlags(ItemIsSelectable | ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    m_props = defaultProps(widgetType, name);
}

// ── Properties ────────────────────────────────────────────────────────────

void WidgetItem::setObjectName(const QString &n) { m_name = n; update(); }

QString WidgetItem::prop(const QString &key) const {
    return m_props.value(key);
}

void WidgetItem::setProp(const QString &key, const QString &val) {
    m_props[key] = val;
    update();
}

void WidgetItem::setItemSize(qreal w, qreal h) {
    prepareGeometryChange();
    m_w = qMax(20.0, w);
    m_h = qMax(12.0, h);
    update();
}

// ── Bounding rect ──────────────────────────────────────────────────────────

QRectF WidgetItem::boundingRect() const {
    // Extra margin for selection handles
    return QRectF(-HS, -HS, m_w + 2*HS, m_h + 2*HS);
}

// ── Handle geometry ────────────────────────────────────────────────────────

QRectF WidgetItem::handleRect(Handle h) const {
    qreal mx = m_w / 2, my = m_h / 2;
    QPointF c;
    switch (h) {
    case H_TL: c = QPointF(0,    0   ); break;
    case H_T:  c = QPointF(mx,   0   ); break;
    case H_TR: c = QPointF(m_w,  0   ); break;
    case H_R:  c = QPointF(m_w,  my  ); break;
    case H_BR: c = QPointF(m_w,  m_h ); break;
    case H_B:  c = QPointF(mx,   m_h ); break;
    case H_BL: c = QPointF(0,    m_h ); break;
    case H_L:  c = QPointF(0,    my  ); break;
    default:   return QRectF();
    }
    return QRectF(c.x()-HS/2, c.y()-HS/2, HS, HS);
}

WidgetItem::Handle WidgetItem::handleAt(const QPointF &lp) const {
    if (!isSelected()) return H_None;
    for (int h = H_TL; h <= H_L; ++h)
        if (handleRect((Handle)h).contains(lp)) return (Handle)h;
    return H_None;
}

// ── Cursor based on handle ────────────────────────────────────────────────

static Qt::CursorShape cursorForHandle(WidgetItem::Handle h) {
    switch (h) {
    case WidgetItem::H_TL: case WidgetItem::H_BR: return Qt::SizeFDiagCursor;
    case WidgetItem::H_TR: case WidgetItem::H_BL: return Qt::SizeBDiagCursor;
    case WidgetItem::H_T:  case WidgetItem::H_B:  return Qt::SizeVerCursor;
    case WidgetItem::H_L:  case WidgetItem::H_R:  return Qt::SizeHorCursor;
    default: return Qt::SizeAllCursor;
    }
}

// ── Drawing ───────────────────────────────────────────────────────────────

void WidgetItem::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *) {
    p->save();

    // Draw widget body
    p->setRenderHint(QPainter::Antialiasing, true);
    drawContent(p);

    // Selection outline + handles
    if (isSelected()) {
        p->setRenderHint(QPainter::Antialiasing, false);
        QPen selPen(QColor("#3584e4"), 1.5, Qt::SolidLine);
        p->setPen(selPen);
        p->setBrush(Qt::NoBrush);
        p->drawRect(QRectF(0, 0, m_w, m_h));

        // Draw 8 resize handles
        p->setBrush(QColor("#3584e4"));
        p->setPen(QPen(Qt::white, 1));
        for (int h = H_TL; h <= H_L; ++h)
            p->drawRect(handleRect((Handle)h));
    }

    p->restore();
}

void WidgetItem::drawContent(QPainter *p) const {
    const QRectF r(0, 0, m_w, m_h);
    const QString text = m_props.value("text", m_name);

    p->setFont(QApplication::font());

    if (m_type == "Label") {
        p->setPen(QColor("#1a1a2e"));
        p->setBrush(Qt::NoBrush);
        // Dotted selection border when not selected
        if (!isSelected()) {
            p->setPen(QPen(QColor("#aaaacc"), 1, Qt::DashLine));
            p->drawRect(r.adjusted(0,0,-1,-1));
        }
        p->setPen(QColor("#1a1a2e"));
        p->drawText(r.adjusted(4,0,-4,0), Qt::AlignVCenter | Qt::AlignLeft, text);

    } else if (m_type == "PushButton") {
        QLinearGradient g(0, 0, 0, m_h);
        g.setColorAt(0, QColor("#5b9cf6"));
        g.setColorAt(1, QColor("#3584e4"));
        p->setBrush(g);
        p->setPen(QPen(QColor("#2563ab"), 1));
        p->drawRoundedRect(r.adjusted(0,0,-1,-1), 4, 4);
        p->setPen(Qt::white);
        QFont f = p->font(); f.setWeight(QFont::Medium); p->setFont(f);
        p->drawText(r, Qt::AlignCenter, text);

    } else if (m_type == "LineEdit") {
        p->setBrush(QColor("#ffffff"));
        p->setPen(QPen(QColor("#9e9e9e"), 1));
        p->drawRoundedRect(r.adjusted(0,0,-1,-1), 3, 3);
        p->setPen(QColor("#9e9e9e"));
        p->drawText(r.adjusted(6,0,-6,0), Qt::AlignVCenter, text.isEmpty() ? m_props.value("placeholderText") : text);

    } else if (m_type == "TextEdit") {
        p->setBrush(QColor("#ffffff"));
        p->setPen(QPen(QColor("#9e9e9e"), 1));
        p->drawRect(r.adjusted(0,0,-1,-1));
        if (!text.isEmpty()) {
            p->setPen(QColor("#333333"));
            p->drawText(r.adjusted(4,4,-4,-4), Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap, text);
        }

    } else if (m_type == "CheckBox") {
        qreal s = qMin(14.0, m_h - 4.0);
        QRectF box(3, (m_h - s)/2, s, s);
        p->setBrush(QColor("#ffffff"));
        p->setPen(QPen(QColor("#7c7c9e"), 1.5));
        p->drawRoundedRect(box, 2, 2);
        p->setPen(QColor("#1a1a2e"));
        p->drawText(QRectF(s + 8, 0, m_w - s - 10, m_h), Qt::AlignVCenter, text);

    } else if (m_type == "RadioButton") {
        qreal s = qMin(14.0, m_h - 4.0);
        QRectF circle(3, (m_h - s)/2, s, s);
        p->setBrush(QColor("#ffffff"));
        p->setPen(QPen(QColor("#7c7c9e"), 1.5));
        p->drawEllipse(circle);
        p->setPen(QColor("#1a1a2e"));
        p->drawText(QRectF(s + 8, 0, m_w - s - 10, m_h), Qt::AlignVCenter, text);

    } else if (m_type == "ComboBox") {
        p->setBrush(QColor("#f5f5f5"));
        p->setPen(QPen(QColor("#9e9e9e"), 1));
        p->drawRoundedRect(r.adjusted(0,0,-1,-1), 3, 3);
        // Arrow area
        qreal aw = 20;
        QRectF arrowArea(m_w - aw, 0, aw, m_h);
        p->setBrush(QColor("#e0e0e0"));
        p->drawRect(arrowArea);
        // Triangle
        QPolygonF tri;
        qreal cx = m_w - aw/2, cy = m_h/2;
        tri << QPointF(cx-4, cy-2) << QPointF(cx+4, cy-2) << QPointF(cx, cy+3);
        p->setBrush(QColor("#555"));
        p->setPen(Qt::NoPen);
        p->drawPolygon(tri);
        // First item text
        p->setPen(QColor("#333333"));
        QString firstItem = m_props.value("items").section('\n', 0, 0);
        p->drawText(r.adjusted(6, 0, -aw-2, 0), Qt::AlignVCenter, firstItem);

    } else if (m_type == "SpinBox") {
        p->setBrush(QColor("#ffffff"));
        p->setPen(QPen(QColor("#9e9e9e"), 1));
        p->drawRoundedRect(r.adjusted(0,0,-1,-1), 3, 3);
        qreal bw = 18;
        // Up/down buttons
        p->setBrush(QColor("#f0f0f0"));
        p->drawRect(QRectF(m_w-bw, 0, bw, m_h/2));
        p->drawRect(QRectF(m_w-bw, m_h/2, bw, m_h/2));
        p->setBrush(QColor("#555")); p->setPen(Qt::NoPen);
        qreal cx = m_w - bw/2;
        QPolygonF up, dn;
        up << QPointF(cx-3, m_h/2-2) << QPointF(cx+3, m_h/2-2) << QPointF(cx, m_h/4);
        dn << QPointF(cx-3, m_h/2+2) << QPointF(cx+3, m_h/2+2) << QPointF(cx, m_h*3/4);
        p->drawPolygon(up); p->drawPolygon(dn);
        p->setPen(QColor("#333333"));
        p->drawText(r.adjusted(4, 0, -bw-2, 0), Qt::AlignVCenter,
                    m_props.value("value", "0"));

    } else if (m_type == "ProgressBar") {
        p->setBrush(QColor("#e0e0e0"));
        p->setPen(QPen(QColor("#bdbdbd"), 1));
        p->drawRoundedRect(r.adjusted(0,0,-1,-1), 3, 3);
        int val = m_props.value("value", "50").toInt();
        int minV = m_props.value("minimum", "0").toInt();
        int maxV = m_props.value("maximum", "100").toInt();
        if (maxV > minV) {
            qreal ratio = qreal(val - minV) / (maxV - minV);
            p->setBrush(QColor("#3584e4"));
            p->setPen(Qt::NoPen);
            p->drawRoundedRect(QRectF(1, 1, (m_w-2)*ratio, m_h-2), 2, 2);
        }
        p->setPen(QColor("#333333"));
        p->drawText(r, Qt::AlignCenter, QString("%1%").arg(val));

    } else if (m_type == "Slider") {
        qreal trackH = 4, trackY = (m_h - trackH) / 2;
        p->setBrush(QColor("#bdbdbd"));
        p->setPen(Qt::NoPen);
        p->drawRoundedRect(QRectF(8, trackY, m_w-16, trackH), 2, 2);
        int val = m_props.value("value", "50").toInt();
        int minV = m_props.value("minimum", "0").toInt();
        int maxV = m_props.value("maximum", "100").toInt();
        qreal ratio = maxV > minV ? qreal(val - minV) / (maxV - minV) : 0;
        qreal handleX = 8 + ratio * (m_w - 16);
        p->setBrush(QColor("#3584e4"));
        p->drawEllipse(QPointF(handleX, m_h/2), 7, 7);

    } else if (m_type == "GroupBox") {
        QPen pen(QColor("#aaaacc"), 1);
        p->setPen(pen);
        p->setBrush(Qt::NoBrush);
        p->drawRoundedRect(r.adjusted(0, 8, -1, -1), 3, 3);
        // Title background
        QFontMetrics fm(p->font());
        int tw = fm.horizontalAdvance(text) + 8;
        p->setBrush(QColor("#f0f0f8"));
        p->setPen(Qt::NoPen);
        p->drawRect(QRectF(10, 2, tw, 14));
        p->setPen(QColor("#444466"));
        QFont f = p->font(); f.setWeight(QFont::Medium); p->setFont(f);
        p->drawText(QRectF(12, 2, tw, 14), Qt::AlignVCenter, text);

    } else if (m_type == "Frame") {
        p->setBrush(Qt::NoBrush);
        p->setPen(QPen(QColor("#9e9e9e"), 1));
        p->drawRect(r.adjusted(0,0,-1,-1));

    } else if (m_type == "ListWidget") {
        p->setBrush(QColor("#ffffff"));
        p->setPen(QPen(QColor("#9e9e9e"), 1));
        p->drawRect(r.adjusted(0,0,-1,-1));
        QStringList items = m_props.value("items").split('\n');
        qreal rowH = 20;
        p->setPen(QColor("#333333"));
        for (int i = 0; i < items.size() && (i+1)*rowH <= m_h - 4; ++i) {
            if (i == 0) { p->setBrush(QColor("#3584e4")); p->setPen(Qt::NoPen);
                          p->drawRect(QRectF(1, 1+i*rowH, m_w-2, rowH-1));
                          p->setPen(Qt::white); }
            else { p->setPen(QColor("#333333")); }
            p->drawText(QRectF(6, 2+i*rowH, m_w-8, rowH-2), Qt::AlignVCenter, items[i]);
        }

    } else {
        // Fallback: gray box
        p->setBrush(QColor("#e8e8f0"));
        p->setPen(QPen(QColor("#9e9e9e"), 1));
        p->drawRect(r.adjusted(0,0,-1,-1));
        p->setPen(QColor("#555"));
        p->drawText(r, Qt::AlignCenter, m_type);
    }
}

// ── Mouse events ──────────────────────────────────────────────────────────

void WidgetItem::mousePressEvent(QGraphicsSceneMouseEvent *e) {
    if (e->button() != Qt::LeftButton) { QGraphicsObject::mousePressEvent(e); return; }

    m_pressScene = e->scenePos();
    m_origPos    = pos();
    m_origW      = m_w;
    m_origH      = m_h;
    m_activeHandle = handleAt(e->pos());

    setSelected(true);
    emit itemSelected(this);
    e->accept();
}

void WidgetItem::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
    QPointF delta = e->scenePos() - m_pressScene;

    if (m_activeHandle == H_None) {
        // Move
        setPos(m_origPos + delta);
    } else {
        // Resize
        qreal nx = m_origPos.x(), ny = m_origPos.y();
        qreal nw = m_origW,       nh = m_origH;
        qreal dx = delta.x(),     dy = delta.y();

        switch (m_activeHandle) {
        case H_TL: nx += dx; ny += dy; nw -= dx; nh -= dy; break;
        case H_T:              ny += dy;           nh -= dy; break;
        case H_TR:             ny += dy; nw += dx; nh -= dy; break;
        case H_R:                        nw += dx;           break;
        case H_BR:                       nw += dx; nh += dy; break;
        case H_B:                                  nh += dy; break;
        case H_BL: nx += dx;           nw -= dx; nh += dy; break;
        case H_L:  nx += dx;           nw -= dx;           break;
        default: break;
        }

        nw = qMax(20.0, nw); nh = qMax(12.0, nh);
        prepareGeometryChange();
        setPos(nx, ny);
        m_w = nw; m_h = nh;
    }

    emit geometryChanged(this);
    e->accept();
}

void WidgetItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
    m_activeHandle = H_None;
    e->accept();
}

void WidgetItem::hoverMoveEvent(QGraphicsSceneHoverEvent *e) {
    Handle h = handleAt(e->pos());
    setCursor(h == H_None ? Qt::SizeAllCursor : cursorForHandle(h));
}

void WidgetItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
    unsetCursor();
}

QVariant WidgetItem::itemChange(GraphicsItemChange change, const QVariant &val) {
    if (change == ItemSelectedHasChanged && val.toBool())
        emit itemSelected(this);
    return QGraphicsObject::itemChange(change, val);
}
