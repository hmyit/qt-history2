TEMPLATE	= app
TARGET		= bigtable

CONFIG		+= qt warn_on release
DEPENDPATH 	= ../../include

QTDIR_build:REQUIRES 	= table full-config

HEADERS		=
SOURCES		= main.cpp
