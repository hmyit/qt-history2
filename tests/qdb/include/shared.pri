CONFIG		+= qt warn_on debug console
DEFINES		+= QT_NO_CODECS QT_LITE_UNICODE QT_NO_COMPONENT 
INCLUDEPATH	+= ../include
SOURCES	        += ../src/sqlinterpreter.cpp \
		../src/environment.cpp \
		../src/filedriver_xbase.cpp \
		../src/op.cpp \
		../src/parser.cpp \
		../src/qdb.cpp 
xbase {
	XBASE_ROOT	= ../xdb-1.2.0
	XBASE_PATH	= $$XBASE_ROOT/xdb
	DEFINES		+= HAVE_CONFIG_H XBASE_DEBUG
	INCLUDEPATH	+= $$XBASE_PATH $$XBASE_ROOT
}

