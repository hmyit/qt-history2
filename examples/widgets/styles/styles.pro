HEADERS       = norwegianwoodstyle.h \
                widgetgallery.h
SOURCES       = main.cpp \
                norwegianwoodstyle.cpp \
                widgetgallery.cpp
RESOURCES     = styles.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/styles
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS styles.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/styles
INSTALLS += target sources
