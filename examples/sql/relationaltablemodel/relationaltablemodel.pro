HEADERS       = ../connection.h
SOURCES       = relationaltablemodel.cpp
QT           += sql

# install
target.path = $$[QT_INSTALL_DATA]/examples/sql/relationaltablemodel
sources.files = $$SOURCES *.h $$RESOURCES $$FORMS relationaltablemodel.pro
sources.path = $$[QT_INSTALL_DATA]/examples/sql/relationaltablemodel
INSTALLS += target sources
