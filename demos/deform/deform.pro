SOURCES += main.cpp pathdeform.cpp
HEADERS += pathdeform.h

SHARED_FOLDER = ../shared

include($$SHARED_FOLDER/shared.pri)

RESOURCES += deform.qrc

# install
target.path = $$[QT_INSTALL_DEMOS]/deform
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.html
sources.path = $$[QT_INSTALL_DEMOS]/deform
INSTALLS += target sources
