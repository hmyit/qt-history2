######################################################################
# Automatically generated by qmake (1.04a) Tue Nov 12 15:27:53 2002
######################################################################

TEMPLATE = app

# Input
HEADERS += qfont.h \
           qfontdata_p.h \
	   qfontengine_p.h \
           qpainter.h \
           qtextdata.h \
           qtextengine.h \
           qtextlayout.h \
           scriptengine.h \
	   opentype.h \
	opentype/fterrcompat.h  opentype/ftxgpos.h  opentype/ftxopen.h \
	opentype/ftxgdef.h      opentype/ftxgsub.h  opentype/ftxopenf.h \
	editwidget.h

SOURCES += qfont.cpp \
	   qfont_x11.cpp \
	   ../../src/kernel/qfontdatabase.cpp \
	   qfontengine.cpp \
           qpainter_x11.cpp \
           qtextdata.cpp \
           qtextengine.cpp \
           qtextlayout.cpp \
	   scriptengine.cpp \
           scriptenginearabic.cpp \
           scriptenginedevanagari.cpp \
           scriptenginebengali.cpp \
           scriptenginetamil.cpp \
	   opentype.cpp \
	opentype/ftxgdef.c  opentype/ftxgpos.c  opentype/ftxgsub.c  opentype/ftxopen.c \
           test.cpp editwidget.cpp
CONFIG += qt warn_on debug  thread create_prl link_prl
OBJECTS_DIR=.obj/debug-shared-mt
MOC_DIR=.moc/debug-shared-mt

DEFINES += NEW_FONT
