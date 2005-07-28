TEMPLATE = lib
TARGET = QtDesignerComponents
CONFIG += qt debug_and_release depend_prl
DESTDIR = $$QT_BUILD_TREE/lib
DLLDESTDIR = $$QT_BUILD_TREE/bin

VERSION = 4.1.0
QMAKE_TARGET_COMPANY = Trolltech AS
QMAKE_TARGET_PRODUCT = Designer
QMAKE_TARGET_DESCRIPTION = Graphical user interface designer.
QMAKE_TARGET_COPYRIGHT = Copyright (c) 2003-2005 Trolltech

SOURCES += qdesigner_components.cpp

!contains(CONFIG, static) {
    DEFINES += QDESIGNER_COMPONENTS_LIBRARY
    CONFIG += dll
    LIBS += -lQtDesigner
} else {
    DEFINES += QT_DESIGNER_STATIC
}

INCLUDEPATH += . .. \
    $$QT_SOURCE_TREE/tools/designer/src/lib/components \
    $$QT_SOURCE_TREE/tools/designer/src/lib/sdk \
    $$QT_SOURCE_TREE/tools/designer/src/lib/extension \
    $$QT_SOURCE_TREE/tools/designer/src/lib/uilib \
    $$QT_SOURCE_TREE/tools/designer/src/lib/shared


win32:dll {
   dlltarget.path=$$[QT_INSTALL_BINS]
   INSTALLS        += dlltarget
}
target.path=$$[QT_INSTALL_LIBS]
INSTALLS        += target

include(../propertyeditor/propertyeditor.pri)
include(../objectinspector/objectinspector.pri)
include(../signalsloteditor/signalsloteditor.pri)
include(../formeditor/formeditor.pri)
include(../widgetbox/widgetbox.pri)
include(../buddyeditor/buddyeditor.pri)
include(../taskmenu/taskmenu.pri)
include(../tabordereditor/tabordereditor.pri)
include(../resourceeditor/resourceeditor.pri)

PRECOMPILED_HEADER= lib_pch.h

include(../../sharedcomponents.pri)
include(../component.pri)
