#pragma once
#include <QGraphicsObject>
#include <QMap>
#include <QString>

class WidgetItem : public QGraphicsObject {
    Q_OBJECT
public:
    enum { Type = QGraphicsItem::UserType + 1 };
    int type() const override { return Type; }

    // Resize handle positions (public so helper functions can use them)
    enum Handle { H_None=-1, H_TL, H_T, H_TR, H_R, H_BR, H_B, H_BL, H_L };

    WidgetItem(const QString &widgetType, const QString &name,
               qreal x, qreal y, qreal w, qreal h,
               QGraphicsItem *parent = nullptr);

    QString widgetType() const { return m_type; }
    QString objectName() const { return m_name; }
    void    setObjectName(const QString &n);

    qreal  itemW() const { return m_w; }
    qreal  itemH() const { return m_h; }
    void   setItemSize(qreal w, qreal h);

    QString prop(const QString &key) const;
    void    setProp(const QString &key, const QString &val);
    const QMap<QString,QString> &props() const { return m_props; }

    QRectF boundingRect() const override;
    void   paint(QPainter *p, const QStyleOptionGraphicsItem *opt,
                 QWidget *w) override;

signals:
    void itemSelected(WidgetItem *);
    void geometryChanged(WidgetItem *);

protected:
    void mousePressEvent  (QGraphicsSceneMouseEvent *e) override;
    void mouseMoveEvent   (QGraphicsSceneMouseEvent *e) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *e) override;
    void hoverMoveEvent   (QGraphicsSceneHoverEvent *e) override;
    void hoverLeaveEvent  (QGraphicsSceneHoverEvent *e) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &val) override;

private:
    void   drawContent(QPainter *p) const;
    QRectF handleRect(Handle h)            const;
    Handle handleAt  (const QPointF &lp)  const;

    static const int HS = 7;

    QString  m_type, m_name;
    qreal    m_w, m_h;
    QMap<QString,QString> m_props;

    Handle  m_activeHandle { H_None };
    QPointF m_pressScene;
    QPointF m_origPos;
    qreal   m_origW { 0 }, m_origH { 0 };
};
