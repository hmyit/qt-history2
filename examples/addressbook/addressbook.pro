TEMPLATE	= app
TARGET		= addressbook

CONFIG		+= qt warn_on release
DEPENDPATH	= ../../include

QTDIR_build:REQUIRES	= full-config

HEADERS		= centralwidget.h \
		  mainwindow.h
SOURCES		= centralwidget.cpp \
		  main.cpp \
		  mainwindow.cpp
