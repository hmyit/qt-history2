TEMPLATE = lib
QT += xml
CONFIG += qt
CONFIG += staticlib

INCLUDEPATH += \
    ../../lib/sdk \
    ../../lib/extension \
    ../../lib/shared \

TARGET = taskmenu

DESTDIR = ../../../lib

DEFINES += QT_TASKMENU_LIBRARY

PRECOMPILED_HEADER=taskmenu_pch.h

FORMS += listwidgeteditor.ui \
    treewidgeteditor.ui

HEADERS += button_taskmenu.h \
  groupbox_taskmenu.h \
  label_taskmenu.h \
  lineedit_taskmenu.h \
  listwidget_taskmenu.h \
  inplace_editor.h \
  taskmenu_component.h

SOURCES += button_taskmenu.cpp \
  groupbox_taskmenu.cpp \
  label_taskmenu.cpp \
  lineedit_taskmenu.cpp \
  listwidget_taskmenu.cpp \
  inplace_editor.cpp \
  taskmenu_component.cpp

include(../component.pri)
