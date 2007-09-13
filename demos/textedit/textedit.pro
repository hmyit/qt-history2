TEMPLATE        = app
TARGET          = textedit

CONFIG          += qt warn_on

HEADERS         = textedit.h printpreview.h
SOURCES         = textedit.cpp \
                  printpreview.cpp \
                  main.cpp

RESOURCES += textedit.qrc
build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

# install
target.path = $$[QT_INSTALL_DEMOS]/textedit
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.html *.doc images
sources.path = $$[QT_INSTALL_DEMOS]/textedit
INSTALLS += target sources

DEFINES += QT_USE_USING_NAMESPACE
