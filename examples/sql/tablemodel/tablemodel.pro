HEADERS       = ../connection.h
SOURCES       = tablemodel.cpp
QT           += sql

# install
target.path = $$[QT_INSTALL_DATA]/examples/sql/tablemodel
sources.files = $$SOURCES *.h $$RESOURCES $$FORMS tablemodel.pro
sources.path = $$[QT_INSTALL_DATA]/examples/sql/tablemodel
INSTALLS += target sources
