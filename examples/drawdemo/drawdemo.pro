TEMPLATE	= app
TARGET		= drawdemo

CONFIG		+= qt warn_on release
DEPENDPATH	= ../../include

QTDIR_build:REQUIRES	= full-config

HEADERS		=
SOURCES		= drawdemo.cpp
