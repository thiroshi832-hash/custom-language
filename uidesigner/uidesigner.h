#pragma once
#include <QWidget>
#include <QString>

class DesignCanvas;
class WidgetItem;
class QListWidget;
class QTableWidget;
class QLabel;
class QLineEdit;
class QToolBar;
class QAction;

class UIDesigner : public QWidget {
    Q_OBJECT
public:
    explicit UIDesigner(QWidget *parent = nullptr);

    // Called by MainWindow to wire "New Form" / "Generate Code" menu items
    void newForm();
    void generateCode();

private slots:
    void onItemSelected(WidgetItem *item);
    void onPropChanged(int row, int col);
    void onToolSelected(const QString &type);
    void selectTool();

private:
    void setupUI();
    void populateProps(WidgetItem *item);
    void applyFormSize();
    void setStatus(const QString &msg);

    DesignCanvas *m_canvas       { nullptr };
    QListWidget  *m_palette      { nullptr };
    QTableWidget *m_propTable    { nullptr };
    QLabel       *m_statusLabel  { nullptr };
    QLabel       *m_statusPos    { nullptr };
    QLineEdit    *m_formWEdit    { nullptr };
    QLineEdit    *m_formHEdit    { nullptr };

    WidgetItem   *m_selectedItem  { nullptr };
    bool          m_updatingProps { false };

    QAction      *m_actSelect    { nullptr };
};
