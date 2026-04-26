QT += core gui widgets
CONFIG += c++11
TARGET = CustomLanguage
TEMPLATE = app

INCLUDEPATH += . lang termrunner uidesigner copilot

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    codeeditor.cpp \
    syntaxhighlighter.cpp \
    lang/lexer.cpp \
    lang/parser.cpp \
    lang/compiler.cpp \
    termrunner/opcodestream.cpp \
    termrunner/formruntime.cpp \
    termrunner/vm.cpp \
    uidesigner/widgetitem.cpp \
    uidesigner/designcanvas.cpp \
    uidesigner/uidesigner.cpp \
    copilot/localcopilot.cpp \
    copilot/copilotpanel.cpp

HEADERS += \
    mainwindow.h \
    codeeditor.h \
    syntaxhighlighter.h \
    lang/token.h \
    lang/lexer.h \
    lang/ast.h \
    lang/parser.h \
    lang/opcodes.h \
    lang/compiler.h \
    termrunner/opcodestream.h \
    termrunner/formruntime.h \
    termrunner/vm.h \
    uidesigner/widgetitem.h \
    uidesigner/designcanvas.h \
    uidesigner/uidesigner.h \
    copilot/localcopilot.h \
    copilot/copilotpanel.h
