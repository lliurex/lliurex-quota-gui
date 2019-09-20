# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

#TARGET = gui

unix|win32: CONFIG += c++1z
QT += core gui

greaterThan(QT_MAJOR_VERSION,4): QT += widgets

TARGET = lliurex-quota-gui
TEMPLATE = app
TRANSLATIONS = quotagui_en.ts quotagui_ca_ES.ts quotagui_es_ES.ts

HEADERS += \
   $$PWD/mainwindow.h \
   $$PWD/n4d.h

SOURCES += \
   $$PWD/main.cpp \
   $$PWD/mainwindow.cpp \
   $$PWD/n4d.cpp

RESOURCES += \
   $$PWD/resources.qrc

FORMS += \
   $$PWD/mainwindow.ui

INCLUDEPATH += \
    $$PWD/.

#DISTFILES += \
#    banner.png \
#    configured.png

DEFINES += QT_DEPRECATED_WARNINGS

# N4D validator debug msgs
# DEFINES += _N4D_DEBUG_

# N4D result messages received into gui
# DEFINES += N4D_SHOW_RESULT

# INFO about running threads & deletion
# DEFINES += RUNNING_THREADS

# all qDebug() messages
# CONFIG += qt warn_off release
# DEFINES += QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT
# DEFINES += QT_NO_DEBUG

unix|win32: LIBS += -lxmlrpc++ -lxmlrpc -lxmlrpc_xmlparse -lxmlrpc_xmltok -lxmlrpc_util -lxmlrpc_client++
