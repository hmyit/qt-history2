######################################################################
# Automatically generated by qmake (2.01a) Tue Aug 29 12:28:05 2006
######################################################################

TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
CONFIG += qdbus uitools

# Input
# DBUS_INTERFACES += car.xml
FORMS += controller.ui
HEADERS += car_interface_p.h controller.h
SOURCES += main.cpp car_interface.cpp controller.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qdbus/remotecontrolledcar/controller
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro *.xml
sources.path = $$[QT_INSTALL_EXAMPLES]/qdbus/remotecontrolledcar/controller
INSTALLS += target sources
DEFINES += QT_USE_USING_NAMESPACE
