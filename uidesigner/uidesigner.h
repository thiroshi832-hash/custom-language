#pragma once
#include <QMainWindow>
#include <QString>

class DesignCanvas;
class WidgetItem;
class QListWidget;
class QTableWidget;
class QLabel;
class QLineEdit;
class QSpinBox;
class QAction;

class UIDesigner : public QMainWindow {
    Q_OBJECT
public:
    explicit UIDesigner(QWidget *parent = nullptr);

private slots:
    void onItemSelected(WidgetItem *item);
    void onPropChanged(int row, int col);
    void generateCode();
    void newForm();
    void onToolSelected(const QString &type);
    void selectTool();

private:
    void setupUI();
    void setupToolbar();
    void setupPalette();
    void setupProperties();
    void populateProps(WidgetItem *item);
    void applyFormSize();

    DesignCanvas *m_canvas;
    QListWidget  *m_palette;
    QTableWidget *m_propTable;
    QLabel       *m_statusPos;
    QLineEdit    *m_formWEdit;
    QLineEdit    *m_formHEdit;
    QLabel       *m_formTitleEdit;

    WidgetItem   *m_selectedItem { nullptr };
    bool          m_updatingProps { false };

    QAction *m_actSelect;
};
