SOURCES = main.cpp
CONFIG += console
CONFIG -= qt app_bundle

DESTDIR = ./

# no install rule for application used by test
INSTALLS =


DEFINES += QT_USE_USING_NAMESPACE

