#-------------------------------------------------
#
# Project created by QtCreator 2019-02-06T10:24:01
#
#-------------------------------------------------

unix|win32: CONFIG   += c++1z
QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = quotagui
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    examples.cpp \
    n4d.cpp \
    datamodel.cpp

HEADERS += \
        mainwindow.h \
    examples.h \
    n4d.h \
    datamodel.h

FORMS += \
        mainwindow.ui

DISTFILES += \
    banner.png

RESOURCES += \
    resources.qrc

unix|win32: LIBS += -lxmlrpc++ -lxmlrpc -lxmlrpc_xmlparse -lxmlrpc_xmltok -lxmlrpc_util -lxmlrpc_client++
