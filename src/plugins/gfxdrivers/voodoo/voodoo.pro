TEMPLATE = lib
TARGET	 = qgfxvoodoo

CONFIG  += qt warn_off plugin
DESTDIR	 = ../../../gfxdrivers

DEFINES	-= QT_NO_QWS_VOODOO3
unix:OBJECTS_DIR	= .obj

HEADERS		= ../../../../include/Qt/qgfxvoodoo_qws.h \
		  ../../../../include/Qt/qgfxvoodoodefs_qws.h
SOURCES		= main.cpp \
		  ../../../../src/gui/embedded/qgfxvoodoo_qws.cpp


target.path += $$plugins.path/gfxdrivers
INSTALLS 	+= target
