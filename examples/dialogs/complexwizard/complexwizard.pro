HEADERS       = complexwizard.h \
                licensewizard.h
SOURCES       = complexwizard.cpp \
                licensewizard.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/dialogs/complexwizard
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/dialogs/complexwizard
INSTALLS += target sources
