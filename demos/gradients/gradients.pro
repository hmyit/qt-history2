SOURCES += main.cpp gradients.cpp
HEADERS += gradients.h

SHARED_FOLDER = ../shared

include($$SHARED_FOLDER/shared.pri)

RESOURCES += gradients.qrc

# install
target.path = $$[QT_INSTALL_DEMOS]/gradients
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.html
sources.path = $$[QT_INSTALL_DEMOS]/gradients
INSTALLS += target sources
