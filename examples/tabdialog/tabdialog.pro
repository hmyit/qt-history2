TEMPLATE	= app
TARGET		= tabdialog

CONFIG		+= qt warn_on release
DEPENDPATH	= ../../include

QTDIR_build:REQUIRES	= large-config

HEADERS		= tabdialog.h
SOURCES		= main.cpp \
		  tabdialog.cpp
