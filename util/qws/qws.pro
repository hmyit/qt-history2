TEMPLATE	= app
CONFIG		= qt warn_on release
DEFINES += QWS
LIBS		= -lqnetwork
INCLUDEPATH	= $(QTDIR)/extensions/network/src
HEADERS	= qws.h qwscommand.h qwsproperty.h qws_gui.h qwsaccel.h \
	  qwsmach64.h qwsmach64_defs.h
SOURCES	= qws.cpp qwsproperty.cpp qws_gui.cpp qws_linuxfb.cpp main.cpp \
	  qwsaccel.cpp qwsmach64.cpp
TARGET		= qws
TMAKE_CXXFLAGS += -fno-exceptions -fno-rtti
