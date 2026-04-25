#pragma once
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QString>

class WidgetItem;

// ── FormScene ─────────────────────────────────────────────────────────────
// The QGraphicsScene backing the design canvas.
// Draws grid + form boundary.

class FormScene : public QGraphicsScene {
    Q_OBJECT
public:
    explicit FormScene(int formW, int formH, QObject *parent = nullptr);

    void setFormSize(int w, int h);
    int  formWidth()  const { return m_formW; }
    int  formHeight() const { return m_formH; }

    bool  gridVisible() const    { return m_showGrid; }
    void  setGridVisible(bool v) { m_showGrid = v; update(); }
    int   gridSpacing()          const { return m_gridSpacing; }
    void  setGridSpacing(int s)  { m_gridSpacing = s; update(); }

signals:
    void selectionChanged(WidgetItem *item);  // nullptr = deselected

protected:
    void drawBackground(QPainter *p, const QRectF &rect) override;

private:
    int  m_formW, m_formH;
    bool m_showGrid { true };
    int  m_gridSpacing { 10 };
};

// ── DesignCanvas ──────────────────────────────────────────────────────────
// The QGraphicsView. Manages the current placement tool.

class DesignCanvas : public QGraphicsView {
    Q_OBJECT
public:
    explicit DesignCanvas(QWidget *parent = nullptr);

    FormScene *formScene() const { return m_scene; }

    // Set which widget type the next click will place ("" = select only)
    void setActiveTool(const QString &widgetType);
    QString activeTool() const { return m_activeTool; }

    // Generate a unique name for a widget type
    QString uniqueName(const QString &type) const;

    // Remove the currently selected item
    void deleteSelected();

    // All items on canvas
    QList<WidgetItem*> widgetItems() const;

signals:
    void itemSelected(WidgetItem *item);
    void itemPlaced(WidgetItem *item);
    void mouseMoved(int x, int y);

protected:
    void mousePressEvent  (QMouseEvent *e) override;
    void mouseMoveEvent   (QMouseEvent *e) override;
    void keyPressEvent    (QKeyEvent   *e) override;

private:
    void placeWidget(const QString &type, QPointF scenePos);
    int  snapToGrid(qreal v) const;

    FormScene *m_scene;
    QString    m_activeTool;

    // Default sizes per widget type
    static QSizeF defaultSize(const QString &type);
};
