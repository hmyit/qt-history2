TEMPLATE	= app

QCONFIG		+= sql
CONFIG		+= qt warn_on release
DEPENDPATH	= ../../../include

REQUIRES	= full-config

HEADERS		=
SOURCES		= main.cpp ../connection.cpp
