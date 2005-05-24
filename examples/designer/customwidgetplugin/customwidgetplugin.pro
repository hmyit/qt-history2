CONFIG      += designer plugin
TEMPLATE    = lib
DESTDIR     = $$QT_BUILD_TREE/plugins/designer

HEADERS     = analogclock.h \
              customwidgetplugin.h
SOURCES     = analogclock.cpp \
              customwidgetplugin.cpp

# install
target.path = $$[QT_INSTALL_PLUGINS]/designer
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/designer/customwidgetplugin
INSTALLS += target sources
