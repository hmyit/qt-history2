TEMPLATE	= lib
CONFIG		= qt warn_on release
WIN32:CONFIG   += dll
HEADERS	= p4.h
SOURCES	= main.cpp p4.cpp
INTERFACES = diffdialog.ui
DESTDIR		= $(QTDIR)/plugins
TARGET		= p4plugin