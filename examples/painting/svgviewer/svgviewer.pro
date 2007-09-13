HEADERS       = mainwindow.h \
                svgview.h \
                svgwindow.h
RESOURCES     = svgviewer.qrc
SOURCES       = main.cpp \
                mainwindow.cpp \
                svgview.cpp \
                svgwindow.cpp
QT           += svg

contains(QT_CONFIG, opengl): QT += opengl

CONFIG += console

# install
target.path = $$[QT_INSTALL_EXAMPLES]/painting/svgviewer
sources.files = $$SOURCES $$HEADERS $$RESOURCES svgviewer.pro files
sources.path = $$[QT_INSTALL_EXAMPLES]/painting/svgviewer
INSTALLS += target sources
DEFINES += QT_USE_USING_NAMESPACE
