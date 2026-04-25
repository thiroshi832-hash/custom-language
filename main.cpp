#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("CustomLanguage IDE");
    app.setOrganizationName("CustomLang");

    MainWindow w;
    w.show();
    return app.exec();
}
