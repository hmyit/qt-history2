# Qt tools module

tools {
	TOOLS_P		= tools
	HEADERS +=  $$TOOLS_H/qmemarray.h \
		  $$TOOLS_H/qasciicache.h \
		  $$TOOLS_H/qasciidict.h \
		  $$TOOLS_H/qbitarray.h \
		  $$TOOLS_H/qbuffer.h \
		  $$TOOLS_H/qcache.h \
		  $$TOOLS_H/qptrcollection.h \
		  $$TOOLS_H/qcstring.h \
		  $$TOOLS_H/qdatastream.h \
		  $$TOOLS_H/qdatetime.h \
		  $$TOOLS_H/qdict.h \
		  $$TOOLS_H/qdir.h \
		  $$TOOLS_P/qdir_p.h \
		  $$TOOLS_H/qfile.h \
		  $$TOOLS_P/qfiledefs_p.h \
		  $$TOOLS_H/qfileinfo.h \
		  $$TOOLS_H/qgarray.h \
		  $$TOOLS_H/qgcache.h \
		  $$TOOLS_H/qgdict.h \
		  $$TOOLS_H/qgeneric.h \
		  $$TOOLS_H/qglist.h \
		  $$TOOLS_H/qglobal.h \
		  $$TOOLS_H/qgvector.h \
		  $$TOOLS_H/qintcache.h \
		  $$TOOLS_H/qintdict.h \
		  $$TOOLS_H/qiodevice.h \
		  $$TOOLS_H/qlibrary.h \
		  $$TOOLS_H/qptrlist.h \
		  $$TOOLS_H/qmap.h \
		  $$TOOLS_H/qpluginmanager.h \
		  $$TOOLS_H/qptrdict.h \
		  $$TOOLS_H/qptrqueue.h \
		  $$TOOLS_H/qregexp.h \
		  $$TOOLS_H/qsettings.h \
		  $$TOOLS_H/qshared.h \
		  $$TOOLS_H/qsortedlist.h \
		  $$TOOLS_H/qptrstack.h \
		  $$TOOLS_H/qstring.h \
		  $$TOOLS_H/qstringlist.h \
		  $$TOOLS_H/qptrstrlist.h \
		  $$TOOLS_H/qstrvec.h \
		  $$TOOLS_H/qtextstream.h \
		  $$TOOLS_H/qptrvector.h \
	          $$TOOLS_H/qvaluelist.h \
		  $$TOOLS_H/ucom.h \
		  $$TOOLS_H/qcom.h \
		  $$TOOLS_H/quuid.h


	win32:SOURCES += $$TOOLS_CPP/qdir_win.cpp \
	 	  $$TOOLS_CPP/qfile_win.cpp \
		  $$TOOLS_CPP/qfileinfo_win.cpp \
		  $$TOOLS_CPP/qsettings_win.cpp

        offmac:SOURCES += $$TOOLS_CPP/qdir_mac.cpp \
		  $$TOOLS_CPP/qfile_mac.cpp \
		  $$TOOLS_CPP/qfileinfo_mac.cpp \
		  $$TOOLS_CPP/qsettings_unix.cpp

	!offmac:unix:SOURCES += $$TOOLS_CPP/qdir_unix.cpp \
		  $$TOOLS_CPP/qfile_unix.cpp \
		  $$TOOLS_CPP/qfileinfo_unix.cpp \
		  $$TOOLS_CPP/qsettings_unix.cpp

	SOURCES += $$TOOLS_CPP/qbitarray.cpp \
		  $$TOOLS_CPP/qbuffer.cpp \
		  $$TOOLS_CPP/qptrcollection.cpp \
		  $$TOOLS_CPP/qcstring.cpp \
		  $$TOOLS_CPP/qdatastream.cpp \
		  $$TOOLS_CPP/qdatetime.cpp \
		  $$TOOLS_CPP/qdir.cpp \
		  $$TOOLS_CPP/qfile.cpp \
		  $$TOOLS_CPP/qfileinfo.cpp \
		  $$TOOLS_CPP/qgarray.cpp \
		  $$TOOLS_CPP/qgcache.cpp \
		  $$TOOLS_CPP/qgdict.cpp \
		  $$TOOLS_CPP/qglist.cpp \
		  $$TOOLS_CPP/qglobal.cpp \
		  $$TOOLS_CPP/qgvector.cpp \
		  $$TOOLS_CPP/qiodevice.cpp \
		  $$TOOLS_CPP/qlibrary.cpp \
		  $$TOOLS_CPP/qmap.cpp \
		  $$TOOLS_CPP/qregexp.cpp \
		  $$TOOLS_CPP/qstring.cpp \
		  $$TOOLS_CPP/qstringlist.cpp \
		  $$TOOLS_CPP/qtextstream.cpp \
		  $$TOOLS_CPP/quuid.cpp
}
