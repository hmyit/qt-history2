TEMPLATE	= app
TARGET		= rot13

CONFIG		+= qt warn_on release
DEPENDPATH	= ../../include

QTDIR_build:REQUIRES	= large-config

HEADERS		= rot13.h
SOURCES		= rot13.cpp

