TEMPLATE    =	lib
CONFIG      =	qt x11inc warn_on release
HEADERS     =	dialogs/qfiledlg.h \
		dialogs/qmsgbox.h \
		dialogs/qprndlg.h \
		dialogs/qprogdlg.h \
		dialogs/qtabdlg.h \
		kernel/qaccel.h \
		kernel/qapp.h \
		kernel/qasyncimageio.h \
		kernel/qasyncio.h \
		kernel/qbitmap.h \
		kernel/qbrush.h \
		kernel/qclipbrd.h \
		kernel/qcolor.h \
		kernel/qconnect.h \
		kernel/qcursor.h \
		kernel/qdialog.h \
		kernel/qdrawutl.h \
		kernel/qevent.h \
		kernel/qfont.h \
		kernel/qfontdta.h \
		kernel/qfontinf.h \
		kernel/qfontmet.h \
		kernel/qgmanagr.h \
		kernel/qimage.h \
		kernel/qkeycode.h \
		kernel/qlayout.h \
		kernel/qmetaobj.h \
		kernel/qmovie.h \
		kernel/qobjcoll.h \
		kernel/qobjdefs.h \
		kernel/qobject.h \
		kernel/qpaintd.h \
		kernel/qpaintdc.h \
		kernel/qpainter.h \
		kernel/qpalette.h \
		kernel/qpdevmet.h \
		kernel/qpen.h \
		kernel/qpicture.h \
		kernel/qpixmap.h \
		kernel/qpmcache.h \
		kernel/qpntarry.h \
		kernel/qpoint.h \
		kernel/qprinter.h \
		kernel/qpsprn.h \
		kernel/qrect.h \
		kernel/qregion.h \
		kernel/qsemimodal.h \
		kernel/qsignal.h \
		kernel/qsize.h \
		kernel/qsocknot.h \
		kernel/qtimer.h \
		kernel/qwidcoll.h \
		kernel/qwidget.h \
		kernel/qwindefs.h \
		kernel/qwindow.h \
		kernel/qwmatrix.h \
		tools/qarray.h \
		tools/qbitarry.h \
		tools/qbuffer.h \
		tools/qcache.h \
		tools/qcollect.h \
		tools/qdatetm.h \
		tools/qdict.h \
		tools/qdir.h \
		tools/qdstream.h \
		tools/qfile.h \
		tools/qfiledef.h \
		tools/qfileinf.h \
		tools/qgarray.h \
		tools/qgcache.h \
		tools/qgdict.h \
		tools/qgeneric.h \
		tools/qglist.h \
		tools/qglobal.h \
		tools/qgvector.h \
		tools/qintcach.h \
		tools/qintdict.h \
		tools/qiodev.h \
		tools/qlist.h \
		tools/qptrdict.h \
		tools/qqueue.h \
		tools/qregexp.h \
		tools/qshared.h \
		tools/qstack.h \
		tools/qstring.h \
		tools/qstrlist.h \
		tools/qstrvec.h \
		tools/qtstream.h \
		tools/qvector.h \
		widgets/qbttngrp.h \
		widgets/qbutton.h \
		widgets/qchkbox.h \
		widgets/qcombo.h \
		widgets/qframe.h \
		widgets/qgrpbox.h \
		widgets/qlabel.h \
		widgets/qlcdnum.h \
		widgets/qlined.h \
		widgets/qlistbox.h \
		widgets/qmenubar.h \
		widgets/qmenudta.h \
		widgets/qmlined.h \
		widgets/qpopmenu.h \
		widgets/qprogbar.h \
		widgets/qpushbt.h \
		widgets/qradiobt.h \
		widgets/qrangect.h \
		widgets/qscrbar.h \
		widgets/qslider.h \
		widgets/qtabbar.h \
		widgets/qtablevw.h \
		widgets/qtooltip.h \
		widgets/qvalidator.h
SOURCES     =	dialogs/qfiledlg.cpp \
		dialogs/qmsgbox.cpp \
		dialogs/qprndlg.cpp \
		dialogs/qprogdlg.cpp \
		dialogs/qtabdlg.cpp \
		kernel/qaccel.cpp \
		kernel/qapp.cpp \
		kernel/qapp_x11.cpp \
		kernel/qasyncimageio.cpp \
		kernel/qasyncio.cpp \
		kernel/qbitmap.cpp \
		kernel/qclb_x11.cpp \
		kernel/qclipbrd.cpp \
		kernel/qcol_x11.cpp \
		kernel/qcolor.cpp \
		kernel/qconnect.cpp \
		kernel/qcur_x11.cpp \
		kernel/qcursor.cpp \
		kernel/qdialog.cpp \
		kernel/qdrawutl.cpp \
		kernel/qevent.cpp \
		kernel/qfnt_x11.cpp \
		kernel/qfont.cpp \
		kernel/qgmanagr.cpp \
		kernel/qimage.cpp \
		kernel/qlayout.cpp \
		kernel/qmetaobj.cpp \
		kernel/qmovie.cpp \
		kernel/qnpsupport.cpp \
		kernel/qobject.cpp \
		kernel/qpainter.cpp \
		kernel/qpalette.cpp \
		kernel/qpdevmet.cpp \
		kernel/qpic_x11.cpp \
		kernel/qpicture.cpp \
		kernel/qpixmap.cpp \
		kernel/qpm_x11.cpp \
		kernel/qpmcache.cpp \
		kernel/qpntarry.cpp \
		kernel/qpoint.cpp \
		kernel/qprinter.cpp \
		kernel/qprn_x11.cpp \
		kernel/qpsprn.cpp \
		kernel/qptd_x11.cpp \
		kernel/qptr_x11.cpp \
		kernel/qrect.cpp \
		kernel/qregion.cpp \
		kernel/qrgn_x11.cpp \
		kernel/qsemimodal.cpp \
		kernel/qsignal.cpp \
		kernel/qsize.cpp \
		kernel/qsocknot.cpp \
		kernel/qt_x11.cpp \
		kernel/qtimer.cpp \
		kernel/qwid_x11.cpp \
		kernel/qwidget.cpp \
		kernel/qwindow.cpp \
		kernel/qwmatrix.cpp \
		tools/qbitarry.cpp \
		tools/qbuffer.cpp \
		tools/qcollect.cpp \
		tools/qdatetm.cpp \
		tools/qdir.cpp \
		tools/qdstream.cpp \
		tools/qfile.cpp \
		tools/qfileinf.cpp \
		tools/qgarray.cpp \
		tools/qgcache.cpp \
		tools/qgdict.cpp \
		tools/qglist.cpp \
		tools/qglobal.cpp \
		tools/qgvector.cpp \
		tools/qiodev.cpp \
		tools/qregexp.cpp \
		tools/qstring.cpp \
		tools/qtstream.cpp \
		widgets/qbttngrp.cpp \
		widgets/qbutton.cpp \
		widgets/qchkbox.cpp \
		widgets/qcombo.cpp \
		widgets/qframe.cpp \
		widgets/qgrpbox.cpp \
		widgets/qlabel.cpp \
		widgets/qlcdnum.cpp \
		widgets/qlined.cpp \
		widgets/qlistbox.cpp \
		widgets/qmenubar.cpp \
		widgets/qmenudta.cpp \
		widgets/qmlined.cpp \
		widgets/qpopmenu.cpp \
		widgets/qprogbar.cpp \
		widgets/qpushbt.cpp \
		widgets/qradiobt.cpp \
		widgets/qrangect.cpp \
		widgets/qscrbar.cpp \
		widgets/qslider.cpp \
		widgets/qtabbar.cpp \
		widgets/qtablevw.cpp \
		widgets/qtooltip.cpp \
		widgets/qvalidator.cpp
TARGET      =	qt
VERSION     =	1.30
DESTDIR     =	../lib
