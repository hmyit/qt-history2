#############################################################################
# Makefile for building qt
# Generated by tmake at 01:55, 1997/07/04
#     Project: qtwin.pro
#    Template: e:\tmake\lib\win32-msvc\lib.t
#############################################################################

####### Compiler, tools and options

CC	=	cl
CFLAGS	=	-nologo -W3 -O1
INCPATH	=	-I$(QTDIR)\include
LIB	=	lib
MOC	=	moc

####### Files

HEADERS =	..\include\qfiledlg.h \
		..\include\qmsgbox.h \
		..\include\qprogdlg.h \
		..\include\qtabdlg.h \
		..\include\qaccel.h \
		..\include\qapp.h \
		..\include\qasyncimageio.h \
		..\include\qasyncio.h \
		..\include\qbitmap.h \
		..\include\qbrush.h \
		..\include\qclipbrd.h \
		..\include\qcolor.h \
		..\include\qconnect.h \
		..\include\qcursor.h \
		..\include\qdialog.h \
		..\include\qdrawutl.h \
		..\include\qevent.h \
		..\include\qfont.h \
		..\include\qfontdta.h \
		..\include\qfontinf.h \
		..\include\qfontmet.h \
		..\include\qgmanagr.h \
		..\include\qimage.h \
		..\include\qkeycode.h \
		..\include\qlayout.h \
		..\include\qmetaobj.h \
		..\include\qmovie.h \
		..\include\qobjcoll.h \
		..\include\qobjdefs.h \
		..\include\qobject.h \
		..\include\qpaintd.h \
		..\include\qpaintdc.h \
		..\include\qpainter.h \
		..\include\qpalette.h \
		..\include\qpdevmet.h \
		..\include\qpen.h \
		..\include\qpicture.h \
		..\include\qpixmap.h \
		..\include\qpmcache.h \
		..\include\qpntarry.h \
		..\include\qpoint.h \
		..\include\qprinter.h \
		..\include\qrect.h \
		..\include\qregion.h \
		..\include\qsemimodal.h \
		..\include\qsignal.h \
		..\include\qsize.h \
		..\include\qsocknot.h \
		..\include\qtimer.h \
		..\include\qwidcoll.h \
		..\include\qwidget.h \
		..\include\qwindefs.h \
		..\include\qwindow.h \
		..\include\qwmatrix.h \
		..\include\qarray.h \
		..\include\qbitarry.h \
		..\include\qbuffer.h \
		..\include\qcache.h \
		..\include\qcollect.h \
		..\include\qdatetm.h \
		..\include\qdict.h \
		..\include\qdir.h \
		..\include\qdstream.h \
		..\include\qfile.h \
		..\include\qfiledef.h \
		..\include\qfileinf.h \
		..\include\qgarray.h \
		..\include\qgcache.h \
		..\include\qgdict.h \
		..\include\qgeneric.h \
		..\include\qglist.h \
		..\include\qglobal.h \
		..\include\qgvector.h \
		..\include\qintcach.h \
		..\include\qintdict.h \
		..\include\qiodev.h \
		..\include\qlist.h \
		..\include\qptrdict.h \
		..\include\qqueue.h \
		..\include\qregexp.h \
		..\include\qshared.h \
		..\include\qstack.h \
		..\include\qstring.h \
		..\include\qstrlist.h \
		..\include\qstrvec.h \
		..\include\qtstream.h \
		..\include\qvector.h \
		..\include\qbttngrp.h \
		..\include\qbutton.h \
		..\include\qchkbox.h \
		..\include\qcombo.h \
		..\include\qframe.h \
		..\include\qgrpbox.h \
		..\include\qlabel.h \
		..\include\qlcdnum.h \
		..\include\qlined.h \
		..\include\qlistbox.h \
		..\include\qmenubar.h \
		..\include\qmenudta.h \
		..\include\qmlined.h \
		..\include\qpopmenu.h \
		..\include\qprogbar.h \
		..\include\qpushbt.h \
		..\include\qradiobt.h \
		..\include\qrangect.h \
		..\include\qscrbar.h \
		..\include\qslider.h \
		..\include\qtabbar.h \
		..\include\qtablevw.h \
		..\include\qtooltip.h
SOURCES =	dialogs\qfiledlg.cpp \
		dialogs\qmsgbox.cpp \
		dialogs\qprogdlg.cpp \
		dialogs\qtabdlg.cpp \
		kernel\qaccel.cpp \
		kernel\qapp.cpp \
		kernel\qapp_win.cpp \
		kernel\qasyncimageio.cpp \
		kernel\qasyncio.cpp \
		kernel\qbitmap.cpp \
		kernel\qclb_win.cpp \
		kernel\qclipbrd.cpp \
		kernel\qcol_win.cpp \
		kernel\qcolor.cpp \
		kernel\qconnect.cpp \
		kernel\qcur_win.cpp \
		kernel\qcursor.cpp \
		kernel\qdialog.cpp \
		kernel\qdrawutl.cpp \
		kernel\qevent.cpp \
		kernel\qfnt_win.cpp \
		kernel\qfont.cpp \
		kernel\qgmanagr.cpp \
		kernel\qimage.cpp \
		kernel\qlayout.cpp \
		kernel\qmetaobj.cpp \
		kernel\qmovie.cpp \
		kernel\qobject.cpp \
		kernel\qpainter.cpp \
		kernel\qpalette.cpp \
		kernel\qpdevmet.cpp \
		kernel\qpic_win.cpp \
		kernel\qpicture.cpp \
		kernel\qpixmap.cpp \
		kernel\qpm_win.cpp \
		kernel\qpmcache.cpp \
		kernel\qpntarry.cpp \
		kernel\qpoint.cpp \
		kernel\qprinter.cpp \
		kernel\qprn_win.cpp \
		kernel\qptd_win.cpp \
		kernel\qptr_win.cpp \
		kernel\qrect.cpp \
		kernel\qregion.cpp \
		kernel\qrgn_win.cpp \
		kernel\qsemimodal.cpp \
		kernel\qsignal.cpp \
		kernel\qsize.cpp \
		kernel\qsocknot.cpp \
		kernel\qtimer.cpp \
		kernel\qwid_win.cpp \
		kernel\qwidget.cpp \
		kernel\qwindow.cpp \
		kernel\qwmatrix.cpp \
		tools\qbitarry.cpp \
		tools\qbuffer.cpp \
		tools\qcollect.cpp \
		tools\qdatetm.cpp \
		tools\qdir.cpp \
		tools\qdstream.cpp \
		tools\qfile.cpp \
		tools\qfileinf.cpp \
		tools\qgarray.cpp \
		tools\qgcache.cpp \
		tools\qgdict.cpp \
		tools\qglist.cpp \
		tools\qglobal.cpp \
		tools\qgvector.cpp \
		tools\qiodev.cpp \
		tools\qregexp.cpp \
		tools\qstring.cpp \
		tools\qtstream.cpp \
		widgets\qbttngrp.cpp \
		widgets\qbutton.cpp \
		widgets\qchkbox.cpp \
		widgets\qcombo.cpp \
		widgets\qframe.cpp \
		widgets\qgrpbox.cpp \
		widgets\qlabel.cpp \
		widgets\qlcdnum.cpp \
		widgets\qlined.cpp \
		widgets\qlistbox.cpp \
		widgets\qmenubar.cpp \
		widgets\qmenudta.cpp \
		widgets\qmlined.cpp \
		widgets\qpopmenu.cpp \
		widgets\qprogbar.cpp \
		widgets\qpushbt.cpp \
		widgets\qradiobt.cpp \
		widgets\qrangect.cpp \
		widgets\qscrbar.cpp \
		widgets\qslider.cpp \
		widgets\qtabbar.cpp \
		widgets\qtablevw.cpp \
		widgets\qtooltip.cpp
OBJECTS =	dialogs\qfiledlg.obj \
		dialogs\qmsgbox.obj \
		dialogs\qprogdlg.obj \
		dialogs\qtabdlg.obj \
		kernel\qaccel.obj \
		kernel\qapp.obj \
		kernel\qapp_win.obj \
		kernel\qasyncimageio.obj \
		kernel\qasyncio.obj \
		kernel\qbitmap.obj \
		kernel\qclb_win.obj \
		kernel\qclipbrd.obj \
		kernel\qcol_win.obj \
		kernel\qcolor.obj \
		kernel\qconnect.obj \
		kernel\qcur_win.obj \
		kernel\qcursor.obj \
		kernel\qdialog.obj \
		kernel\qdrawutl.obj \
		kernel\qevent.obj \
		kernel\qfnt_win.obj \
		kernel\qfont.obj \
		kernel\qgmanagr.obj \
		kernel\qimage.obj \
		kernel\qlayout.obj \
		kernel\qmetaobj.obj \
		kernel\qmovie.obj \
		kernel\qobject.obj \
		kernel\qpainter.obj \
		kernel\qpalette.obj \
		kernel\qpdevmet.obj \
		kernel\qpic_win.obj \
		kernel\qpicture.obj \
		kernel\qpixmap.obj \
		kernel\qpm_win.obj \
		kernel\qpmcache.obj \
		kernel\qpntarry.obj \
		kernel\qpoint.obj \
		kernel\qprinter.obj \
		kernel\qprn_win.obj \
		kernel\qptd_win.obj \
		kernel\qptr_win.obj \
		kernel\qrect.obj \
		kernel\qregion.obj \
		kernel\qrgn_win.obj \
		kernel\qsemimodal.obj \
		kernel\qsignal.obj \
		kernel\qsize.obj \
		kernel\qsocknot.obj \
		kernel\qtimer.obj \
		kernel\qwid_win.obj \
		kernel\qwidget.obj \
		kernel\qwindow.obj \
		kernel\qwmatrix.obj \
		tools\qbitarry.obj \
		tools\qbuffer.obj \
		tools\qcollect.obj \
		tools\qdatetm.obj \
		tools\qdir.obj \
		tools\qdstream.obj \
		tools\qfile.obj \
		tools\qfileinf.obj \
		tools\qgarray.obj \
		tools\qgcache.obj \
		tools\qgdict.obj \
		tools\qglist.obj \
		tools\qglobal.obj \
		tools\qgvector.obj \
		tools\qiodev.obj \
		tools\qregexp.obj \
		tools\qstring.obj \
		tools\qtstream.obj \
		widgets\qbttngrp.obj \
		widgets\qbutton.obj \
		widgets\qchkbox.obj \
		widgets\qcombo.obj \
		widgets\qframe.obj \
		widgets\qgrpbox.obj \
		widgets\qlabel.obj \
		widgets\qlcdnum.obj \
		widgets\qlined.obj \
		widgets\qlistbox.obj \
		widgets\qmenubar.obj \
		widgets\qmenudta.obj \
		widgets\qmlined.obj \
		widgets\qpopmenu.obj \
		widgets\qprogbar.obj \
		widgets\qpushbt.obj \
		widgets\qradiobt.obj \
		widgets\qrangect.obj \
		widgets\qscrbar.obj \
		widgets\qslider.obj \
		widgets\qtabbar.obj \
		widgets\qtablevw.obj \
		widgets\qtooltip.obj
SRCMOC	=	tmp\moc_qfiledlg.cpp \
		tmp\moc_qmsgbox.cpp \
		tmp\moc_qprogdlg.cpp \
		tmp\moc_qtabdlg.cpp \
		tmp\moc_qaccel.cpp \
		tmp\moc_qapp.cpp \
		tmp\moc_qasyncio.cpp \
		tmp\moc_qclipbrd.cpp \
		tmp\moc_qdialog.cpp \
		tmp\moc_qgmanagr.cpp \
		tmp\moc_qsemimodal.cpp \
		tmp\moc_qsocknot.cpp \
		tmp\moc_qtimer.cpp \
		tmp\moc_qwidget.cpp \
		tmp\moc_qwindow.cpp \
		tmp\moc_qbttngrp.cpp \
		tmp\moc_qbutton.cpp \
		tmp\moc_qchkbox.cpp \
		tmp\moc_qcombo.cpp \
		tmp\moc_qframe.cpp \
		tmp\moc_qgrpbox.cpp \
		tmp\moc_qlabel.cpp \
		tmp\moc_qlcdnum.cpp \
		tmp\moc_qlined.cpp \
		tmp\moc_qlistbox.cpp \
		tmp\moc_qmenubar.cpp \
		tmp\moc_qmlined.cpp \
		tmp\moc_qpopmenu.cpp \
		tmp\moc_qprogbar.cpp \
		tmp\moc_qpushbt.cpp \
		tmp\moc_qradiobt.cpp \
		tmp\moc_qscrbar.cpp \
		tmp\moc_qslider.cpp \
		tmp\moc_qtabbar.cpp \
		tmp\moc_qtablevw.cpp \
		tmp\moc_qtooltip.cpp
OBJMOC	=	tmp\moc_qfiledlg.obj \
		tmp\moc_qmsgbox.obj \
		tmp\moc_qprogdlg.obj \
		tmp\moc_qtabdlg.obj \
		tmp\moc_qaccel.obj \
		tmp\moc_qapp.obj \
		tmp\moc_qasyncio.obj \
		tmp\moc_qclipbrd.obj \
		tmp\moc_qdialog.obj \
		tmp\moc_qgmanagr.obj \
		tmp\moc_qsemimodal.obj \
		tmp\moc_qsocknot.obj \
		tmp\moc_qtimer.obj \
		tmp\moc_qwidget.obj \
		tmp\moc_qwindow.obj \
		tmp\moc_qbttngrp.obj \
		tmp\moc_qbutton.obj \
		tmp\moc_qchkbox.obj \
		tmp\moc_qcombo.obj \
		tmp\moc_qframe.obj \
		tmp\moc_qgrpbox.obj \
		tmp\moc_qlabel.obj \
		tmp\moc_qlcdnum.obj \
		tmp\moc_qlined.obj \
		tmp\moc_qlistbox.obj \
		tmp\moc_qmenubar.obj \
		tmp\moc_qmlined.obj \
		tmp\moc_qpopmenu.obj \
		tmp\moc_qprogbar.obj \
		tmp\moc_qpushbt.obj \
		tmp\moc_qradiobt.obj \
		tmp\moc_qscrbar.obj \
		tmp\moc_qslider.obj \
		tmp\moc_qtabbar.obj \
		tmp\moc_qtablevw.obj \
		tmp\moc_qtooltip.obj
TARGET	=	qt.lib

####### Implicit rules

.SUFFIXES: .cpp .c

.cpp.obj:
	$(CC) -c $(CFLAGS) $(INCPATH) -Fo$@ $<

.c.obj:
	$(CC) -c $(CFLAGS) $(INCPATH) -Fo$@ $<

####### Build rules

all: $(TARGET) 

$(TARGET): $(OBJECTS) $(OBJMOC)
	$(LIB) $(LFLAGS) /OUT:$(TARGET) @<<
	    $(OBJECTS) $(OBJMOC)
<<
	-copy $(TARGET) ..\lib

moc: $(SRCMOC)

clean:
	-del dialogs\qfiledlg.obj
	-del dialogs\qmsgbox.obj
	-del dialogs\qprogdlg.obj
	-del dialogs\qtabdlg.obj
	-del kernel\qaccel.obj
	-del kernel\qapp.obj
	-del kernel\qapp_win.obj
	-del kernel\qasyncimageio.obj
	-del kernel\qasyncio.obj
	-del kernel\qbitmap.obj
	-del kernel\qclb_win.obj
	-del kernel\qclipbrd.obj
	-del kernel\qcol_win.obj
	-del kernel\qcolor.obj
	-del kernel\qconnect.obj
	-del kernel\qcur_win.obj
	-del kernel\qcursor.obj
	-del kernel\qdialog.obj
	-del kernel\qdrawutl.obj
	-del kernel\qevent.obj
	-del kernel\qfnt_win.obj
	-del kernel\qfont.obj
	-del kernel\qgmanagr.obj
	-del kernel\qimage.obj
	-del kernel\qlayout.obj
	-del kernel\qmetaobj.obj
	-del kernel\qmovie.obj
	-del kernel\qobject.obj
	-del kernel\qpainter.obj
	-del kernel\qpalette.obj
	-del kernel\qpdevmet.obj
	-del kernel\qpic_win.obj
	-del kernel\qpicture.obj
	-del kernel\qpixmap.obj
	-del kernel\qpm_win.obj
	-del kernel\qpmcache.obj
	-del kernel\qpntarry.obj
	-del kernel\qpoint.obj
	-del kernel\qprinter.obj
	-del kernel\qprn_win.obj
	-del kernel\qptd_win.obj
	-del kernel\qptr_win.obj
	-del kernel\qrect.obj
	-del kernel\qregion.obj
	-del kernel\qrgn_win.obj
	-del kernel\qsemimodal.obj
	-del kernel\qsignal.obj
	-del kernel\qsize.obj
	-del kernel\qsocknot.obj
	-del kernel\qtimer.obj
	-del kernel\qwid_win.obj
	-del kernel\qwidget.obj
	-del kernel\qwindow.obj
	-del kernel\qwmatrix.obj
	-del tools\qbitarry.obj
	-del tools\qbuffer.obj
	-del tools\qcollect.obj
	-del tools\qdatetm.obj
	-del tools\qdir.obj
	-del tools\qdstream.obj
	-del tools\qfile.obj
	-del tools\qfileinf.obj
	-del tools\qgarray.obj
	-del tools\qgcache.obj
	-del tools\qgdict.obj
	-del tools\qglist.obj
	-del tools\qglobal.obj
	-del tools\qgvector.obj
	-del tools\qiodev.obj
	-del tools\qregexp.obj
	-del tools\qstring.obj
	-del tools\qtstream.obj
	-del widgets\qbttngrp.obj
	-del widgets\qbutton.obj
	-del widgets\qchkbox.obj
	-del widgets\qcombo.obj
	-del widgets\qframe.obj
	-del widgets\qgrpbox.obj
	-del widgets\qlabel.obj
	-del widgets\qlcdnum.obj
	-del widgets\qlined.obj
	-del widgets\qlistbox.obj
	-del widgets\qmenubar.obj
	-del widgets\qmenudta.obj
	-del widgets\qmlined.obj
	-del widgets\qpopmenu.obj
	-del widgets\qprogbar.obj
	-del widgets\qpushbt.obj
	-del widgets\qradiobt.obj
	-del widgets\qrangect.obj
	-del widgets\qscrbar.obj
	-del widgets\qslider.obj
	-del widgets\qtabbar.obj
	-del widgets\qtablevw.obj
	-del widgets\qtooltip.obj
	-del tmp\moc_qfiledlg.cpp
	-del tmp\moc_qmsgbox.cpp
	-del tmp\moc_qprogdlg.cpp
	-del tmp\moc_qtabdlg.cpp
	-del tmp\moc_qaccel.cpp
	-del tmp\moc_qapp.cpp
	-del tmp\moc_qasyncio.cpp
	-del tmp\moc_qclipbrd.cpp
	-del tmp\moc_qdialog.cpp
	-del tmp\moc_qgmanagr.cpp
	-del tmp\moc_qsemimodal.cpp
	-del tmp\moc_qsocknot.cpp
	-del tmp\moc_qtimer.cpp
	-del tmp\moc_qwidget.cpp
	-del tmp\moc_qwindow.cpp
	-del tmp\moc_qbttngrp.cpp
	-del tmp\moc_qbutton.cpp
	-del tmp\moc_qchkbox.cpp
	-del tmp\moc_qcombo.cpp
	-del tmp\moc_qframe.cpp
	-del tmp\moc_qgrpbox.cpp
	-del tmp\moc_qlabel.cpp
	-del tmp\moc_qlcdnum.cpp
	-del tmp\moc_qlined.cpp
	-del tmp\moc_qlistbox.cpp
	-del tmp\moc_qmenubar.cpp
	-del tmp\moc_qmlined.cpp
	-del tmp\moc_qpopmenu.cpp
	-del tmp\moc_qprogbar.cpp
	-del tmp\moc_qpushbt.cpp
	-del tmp\moc_qradiobt.cpp
	-del tmp\moc_qscrbar.cpp
	-del tmp\moc_qslider.cpp
	-del tmp\moc_qtabbar.cpp
	-del tmp\moc_qtablevw.cpp
	-del tmp\moc_qtooltip.cpp
	-del tmp\moc_qfiledlg.obj
	-del tmp\moc_qmsgbox.obj
	-del tmp\moc_qprogdlg.obj
	-del tmp\moc_qtabdlg.obj
	-del tmp\moc_qaccel.obj
	-del tmp\moc_qapp.obj
	-del tmp\moc_qasyncio.obj
	-del tmp\moc_qclipbrd.obj
	-del tmp\moc_qdialog.obj
	-del tmp\moc_qgmanagr.obj
	-del tmp\moc_qsemimodal.obj
	-del tmp\moc_qsocknot.obj
	-del tmp\moc_qtimer.obj
	-del tmp\moc_qwidget.obj
	-del tmp\moc_qwindow.obj
	-del tmp\moc_qbttngrp.obj
	-del tmp\moc_qbutton.obj
	-del tmp\moc_qchkbox.obj
	-del tmp\moc_qcombo.obj
	-del tmp\moc_qframe.obj
	-del tmp\moc_qgrpbox.obj
	-del tmp\moc_qlabel.obj
	-del tmp\moc_qlcdnum.obj
	-del tmp\moc_qlined.obj
	-del tmp\moc_qlistbox.obj
	-del tmp\moc_qmenubar.obj
	-del tmp\moc_qmlined.obj
	-del tmp\moc_qpopmenu.obj
	-del tmp\moc_qprogbar.obj
	-del tmp\moc_qpushbt.obj
	-del tmp\moc_qradiobt.obj
	-del tmp\moc_qscrbar.obj
	-del tmp\moc_qslider.obj
	-del tmp\moc_qtabbar.obj
	-del tmp\moc_qtablevw.obj
	-del tmp\moc_qtooltip.obj
	-del $(TARGET)

####### Compile

dialogs\qfiledlg.obj: dialogs\qfiledlg.cpp

dialogs\qmsgbox.obj: dialogs\qmsgbox.cpp

dialogs\qprogdlg.obj: dialogs\qprogdlg.cpp

dialogs\qtabdlg.obj: dialogs\qtabdlg.cpp

kernel\qaccel.obj: kernel\qaccel.cpp

kernel\qapp.obj: kernel\qapp.cpp

kernel\qapp_win.obj: kernel\qapp_win.cpp

kernel\qasyncimageio.obj: kernel\qasyncimageio.cpp

kernel\qasyncio.obj: kernel\qasyncio.cpp

kernel\qbitmap.obj: kernel\qbitmap.cpp

kernel\qclb_win.obj: kernel\qclb_win.cpp

kernel\qclipbrd.obj: kernel\qclipbrd.cpp

kernel\qcol_win.obj: kernel\qcol_win.cpp

kernel\qcolor.obj: kernel\qcolor.cpp

kernel\qconnect.obj: kernel\qconnect.cpp

kernel\qcur_win.obj: kernel\qcur_win.cpp

kernel\qcursor.obj: kernel\qcursor.cpp

kernel\qdialog.obj: kernel\qdialog.cpp

kernel\qdrawutl.obj: kernel\qdrawutl.cpp

kernel\qevent.obj: kernel\qevent.cpp

kernel\qfnt_win.obj: kernel\qfnt_win.cpp

kernel\qfont.obj: kernel\qfont.cpp

kernel\qgmanagr.obj: kernel\qgmanagr.cpp

kernel\qimage.obj: kernel\qimage.cpp

kernel\qlayout.obj: kernel\qlayout.cpp

kernel\qmetaobj.obj: kernel\qmetaobj.cpp

kernel\qmovie.obj: kernel\qmovie.cpp

kernel\qobject.obj: kernel\qobject.cpp

kernel\qpainter.obj: kernel\qpainter.cpp

kernel\qpalette.obj: kernel\qpalette.cpp

kernel\qpdevmet.obj: kernel\qpdevmet.cpp

kernel\qpic_win.obj: kernel\qpic_win.cpp

kernel\qpicture.obj: kernel\qpicture.cpp

kernel\qpixmap.obj: kernel\qpixmap.cpp

kernel\qpm_win.obj: kernel\qpm_win.cpp

kernel\qpmcache.obj: kernel\qpmcache.cpp

kernel\qpntarry.obj: kernel\qpntarry.cpp

kernel\qpoint.obj: kernel\qpoint.cpp

kernel\qprinter.obj: kernel\qprinter.cpp

kernel\qprn_win.obj: kernel\qprn_win.cpp

kernel\qptd_win.obj: kernel\qptd_win.cpp

kernel\qptr_win.obj: kernel\qptr_win.cpp

kernel\qrect.obj: kernel\qrect.cpp

kernel\qregion.obj: kernel\qregion.cpp

kernel\qrgn_win.obj: kernel\qrgn_win.cpp

kernel\qsemimodal.obj: kernel\qsemimodal.cpp

kernel\qsignal.obj: kernel\qsignal.cpp

kernel\qsize.obj: kernel\qsize.cpp

kernel\qsocknot.obj: kernel\qsocknot.cpp

kernel\qtimer.obj: kernel\qtimer.cpp

kernel\qwid_win.obj: kernel\qwid_win.cpp

kernel\qwidget.obj: kernel\qwidget.cpp

kernel\qwindow.obj: kernel\qwindow.cpp

kernel\qwmatrix.obj: kernel\qwmatrix.cpp

tools\qbitarry.obj: tools\qbitarry.cpp

tools\qbuffer.obj: tools\qbuffer.cpp

tools\qcollect.obj: tools\qcollect.cpp

tools\qdatetm.obj: tools\qdatetm.cpp

tools\qdir.obj: tools\qdir.cpp

tools\qdstream.obj: tools\qdstream.cpp

tools\qfile.obj: tools\qfile.cpp

tools\qfileinf.obj: tools\qfileinf.cpp

tools\qgarray.obj: tools\qgarray.cpp

tools\qgcache.obj: tools\qgcache.cpp

tools\qgdict.obj: tools\qgdict.cpp

tools\qglist.obj: tools\qglist.cpp

tools\qglobal.obj: tools\qglobal.cpp

tools\qgvector.obj: tools\qgvector.cpp

tools\qiodev.obj: tools\qiodev.cpp

tools\qregexp.obj: tools\qregexp.cpp

tools\qstring.obj: tools\qstring.cpp

tools\qtstream.obj: tools\qtstream.cpp

widgets\qbttngrp.obj: widgets\qbttngrp.cpp

widgets\qbutton.obj: widgets\qbutton.cpp

widgets\qchkbox.obj: widgets\qchkbox.cpp

widgets\qcombo.obj: widgets\qcombo.cpp

widgets\qframe.obj: widgets\qframe.cpp

widgets\qgrpbox.obj: widgets\qgrpbox.cpp

widgets\qlabel.obj: widgets\qlabel.cpp

widgets\qlcdnum.obj: widgets\qlcdnum.cpp

widgets\qlined.obj: widgets\qlined.cpp

widgets\qlistbox.obj: widgets\qlistbox.cpp

widgets\qmenubar.obj: widgets\qmenubar.cpp

widgets\qmenudta.obj: widgets\qmenudta.cpp

widgets\qmlined.obj: widgets\qmlined.cpp

widgets\qpopmenu.obj: widgets\qpopmenu.cpp

widgets\qprogbar.obj: widgets\qprogbar.cpp

widgets\qpushbt.obj: widgets\qpushbt.cpp

widgets\qradiobt.obj: widgets\qradiobt.cpp

widgets\qrangect.obj: widgets\qrangect.cpp

widgets\qscrbar.obj: widgets\qscrbar.cpp

widgets\qslider.obj: widgets\qslider.cpp

widgets\qtabbar.obj: widgets\qtabbar.cpp

widgets\qtablevw.obj: widgets\qtablevw.cpp

widgets\qtooltip.obj: widgets\qtooltip.cpp

tmp\moc_qfiledlg.obj: tmp\moc_qfiledlg.cpp \
		..\include\qfiledlg.h

tmp\moc_qmsgbox.obj: tmp\moc_qmsgbox.cpp \
		..\include\qmsgbox.h

tmp\moc_qprogdlg.obj: tmp\moc_qprogdlg.cpp \
		..\include\qprogdlg.h

tmp\moc_qtabdlg.obj: tmp\moc_qtabdlg.cpp \
		..\include\qtabdlg.h

tmp\moc_qaccel.obj: tmp\moc_qaccel.cpp \
		..\include\qaccel.h

tmp\moc_qapp.obj: tmp\moc_qapp.cpp \
		..\include\qapp.h

tmp\moc_qasyncio.obj: tmp\moc_qasyncio.cpp \
		..\include\qasyncio.h

tmp\moc_qclipbrd.obj: tmp\moc_qclipbrd.cpp \
		..\include\qclipbrd.h

tmp\moc_qdialog.obj: tmp\moc_qdialog.cpp \
		..\include\qdialog.h

tmp\moc_qgmanagr.obj: tmp\moc_qgmanagr.cpp \
		..\include\qgmanagr.h

tmp\moc_qsemimodal.obj: tmp\moc_qsemimodal.cpp \
		..\include\qsemimodal.h

tmp\moc_qsocknot.obj: tmp\moc_qsocknot.cpp \
		..\include\qsocknot.h

tmp\moc_qtimer.obj: tmp\moc_qtimer.cpp \
		..\include\qtimer.h

tmp\moc_qwidget.obj: tmp\moc_qwidget.cpp \
		..\include\qwidget.h

tmp\moc_qwindow.obj: tmp\moc_qwindow.cpp \
		..\include\qwindow.h

tmp\moc_qbttngrp.obj: tmp\moc_qbttngrp.cpp \
		..\include\qbttngrp.h

tmp\moc_qbutton.obj: tmp\moc_qbutton.cpp \
		..\include\qbutton.h

tmp\moc_qchkbox.obj: tmp\moc_qchkbox.cpp \
		..\include\qchkbox.h

tmp\moc_qcombo.obj: tmp\moc_qcombo.cpp \
		..\include\qcombo.h

tmp\moc_qframe.obj: tmp\moc_qframe.cpp \
		..\include\qframe.h

tmp\moc_qgrpbox.obj: tmp\moc_qgrpbox.cpp \
		..\include\qgrpbox.h

tmp\moc_qlabel.obj: tmp\moc_qlabel.cpp \
		..\include\qlabel.h

tmp\moc_qlcdnum.obj: tmp\moc_qlcdnum.cpp \
		..\include\qlcdnum.h

tmp\moc_qlined.obj: tmp\moc_qlined.cpp \
		..\include\qlined.h

tmp\moc_qlistbox.obj: tmp\moc_qlistbox.cpp \
		..\include\qlistbox.h

tmp\moc_qmenubar.obj: tmp\moc_qmenubar.cpp \
		..\include\qmenubar.h

tmp\moc_qmlined.obj: tmp\moc_qmlined.cpp \
		..\include\qmlined.h

tmp\moc_qpopmenu.obj: tmp\moc_qpopmenu.cpp \
		..\include\qpopmenu.h

tmp\moc_qprogbar.obj: tmp\moc_qprogbar.cpp \
		..\include\qprogbar.h

tmp\moc_qpushbt.obj: tmp\moc_qpushbt.cpp \
		..\include\qpushbt.h

tmp\moc_qradiobt.obj: tmp\moc_qradiobt.cpp \
		..\include\qradiobt.h

tmp\moc_qscrbar.obj: tmp\moc_qscrbar.cpp \
		..\include\qscrbar.h

tmp\moc_qslider.obj: tmp\moc_qslider.cpp \
		..\include\qslider.h

tmp\moc_qtabbar.obj: tmp\moc_qtabbar.cpp \
		..\include\qtabbar.h

tmp\moc_qtablevw.obj: tmp\moc_qtablevw.cpp \
		..\include\qtablevw.h

tmp\moc_qtooltip.obj: tmp\moc_qtooltip.cpp \
		..\include\qtooltip.h

tmp\moc_qfiledlg.cpp: ..\include\qfiledlg.h
	$(MOC) ..\include\qfiledlg.h -o tmp\moc_qfiledlg.cpp

tmp\moc_qmsgbox.cpp: ..\include\qmsgbox.h
	$(MOC) ..\include\qmsgbox.h -o tmp\moc_qmsgbox.cpp

tmp\moc_qprogdlg.cpp: ..\include\qprogdlg.h
	$(MOC) ..\include\qprogdlg.h -o tmp\moc_qprogdlg.cpp

tmp\moc_qtabdlg.cpp: ..\include\qtabdlg.h
	$(MOC) ..\include\qtabdlg.h -o tmp\moc_qtabdlg.cpp

tmp\moc_qaccel.cpp: ..\include\qaccel.h
	$(MOC) ..\include\qaccel.h -o tmp\moc_qaccel.cpp

tmp\moc_qapp.cpp: ..\include\qapp.h
	$(MOC) ..\include\qapp.h -o tmp\moc_qapp.cpp

tmp\moc_qasyncio.cpp: ..\include\qasyncio.h
	$(MOC) ..\include\qasyncio.h -o tmp\moc_qasyncio.cpp

tmp\moc_qclipbrd.cpp: ..\include\qclipbrd.h
	$(MOC) ..\include\qclipbrd.h -o tmp\moc_qclipbrd.cpp

tmp\moc_qdialog.cpp: ..\include\qdialog.h
	$(MOC) ..\include\qdialog.h -o tmp\moc_qdialog.cpp

tmp\moc_qgmanagr.cpp: ..\include\qgmanagr.h
	$(MOC) ..\include\qgmanagr.h -o tmp\moc_qgmanagr.cpp

tmp\moc_qsemimodal.cpp: ..\include\qsemimodal.h
	$(MOC) ..\include\qsemimodal.h -o tmp\moc_qsemimodal.cpp

tmp\moc_qsocknot.cpp: ..\include\qsocknot.h
	$(MOC) ..\include\qsocknot.h -o tmp\moc_qsocknot.cpp

tmp\moc_qtimer.cpp: ..\include\qtimer.h
	$(MOC) ..\include\qtimer.h -o tmp\moc_qtimer.cpp

tmp\moc_qwidget.cpp: ..\include\qwidget.h
	$(MOC) ..\include\qwidget.h -o tmp\moc_qwidget.cpp

tmp\moc_qwindow.cpp: ..\include\qwindow.h
	$(MOC) ..\include\qwindow.h -o tmp\moc_qwindow.cpp

tmp\moc_qbttngrp.cpp: ..\include\qbttngrp.h
	$(MOC) ..\include\qbttngrp.h -o tmp\moc_qbttngrp.cpp

tmp\moc_qbutton.cpp: ..\include\qbutton.h
	$(MOC) ..\include\qbutton.h -o tmp\moc_qbutton.cpp

tmp\moc_qchkbox.cpp: ..\include\qchkbox.h
	$(MOC) ..\include\qchkbox.h -o tmp\moc_qchkbox.cpp

tmp\moc_qcombo.cpp: ..\include\qcombo.h
	$(MOC) ..\include\qcombo.h -o tmp\moc_qcombo.cpp

tmp\moc_qframe.cpp: ..\include\qframe.h
	$(MOC) ..\include\qframe.h -o tmp\moc_qframe.cpp

tmp\moc_qgrpbox.cpp: ..\include\qgrpbox.h
	$(MOC) ..\include\qgrpbox.h -o tmp\moc_qgrpbox.cpp

tmp\moc_qlabel.cpp: ..\include\qlabel.h
	$(MOC) ..\include\qlabel.h -o tmp\moc_qlabel.cpp

tmp\moc_qlcdnum.cpp: ..\include\qlcdnum.h
	$(MOC) ..\include\qlcdnum.h -o tmp\moc_qlcdnum.cpp

tmp\moc_qlined.cpp: ..\include\qlined.h
	$(MOC) ..\include\qlined.h -o tmp\moc_qlined.cpp

tmp\moc_qlistbox.cpp: ..\include\qlistbox.h
	$(MOC) ..\include\qlistbox.h -o tmp\moc_qlistbox.cpp

tmp\moc_qmenubar.cpp: ..\include\qmenubar.h
	$(MOC) ..\include\qmenubar.h -o tmp\moc_qmenubar.cpp

tmp\moc_qmlined.cpp: ..\include\qmlined.h
	$(MOC) ..\include\qmlined.h -o tmp\moc_qmlined.cpp

tmp\moc_qpopmenu.cpp: ..\include\qpopmenu.h
	$(MOC) ..\include\qpopmenu.h -o tmp\moc_qpopmenu.cpp

tmp\moc_qprogbar.cpp: ..\include\qprogbar.h
	$(MOC) ..\include\qprogbar.h -o tmp\moc_qprogbar.cpp

tmp\moc_qpushbt.cpp: ..\include\qpushbt.h
	$(MOC) ..\include\qpushbt.h -o tmp\moc_qpushbt.cpp

tmp\moc_qradiobt.cpp: ..\include\qradiobt.h
	$(MOC) ..\include\qradiobt.h -o tmp\moc_qradiobt.cpp

tmp\moc_qscrbar.cpp: ..\include\qscrbar.h
	$(MOC) ..\include\qscrbar.h -o tmp\moc_qscrbar.cpp

tmp\moc_qslider.cpp: ..\include\qslider.h
	$(MOC) ..\include\qslider.h -o tmp\moc_qslider.cpp

tmp\moc_qtabbar.cpp: ..\include\qtabbar.h
	$(MOC) ..\include\qtabbar.h -o tmp\moc_qtabbar.cpp

tmp\moc_qtablevw.cpp: ..\include\qtablevw.h
	$(MOC) ..\include\qtablevw.h -o tmp\moc_qtablevw.cpp

tmp\moc_qtooltip.cpp: ..\include\qtooltip.h
	$(MOC) ..\include\qtooltip.h -o tmp\moc_qtooltip.cpp

