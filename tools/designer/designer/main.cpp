/**********************************************************************
** Copyright (C) 2000 Trolltech AS.  All rights reserved.
**
** This file is part of Qt Designer.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "formwindow.h"
#include "mainwindow.h"
#include "config.h"
#include "splashloader.h"

#if defined(HAVE_KDE)
#include <kapp.h>
#else
#include <qapplication.h>
#endif

#include <qlabel.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qobjectlist.h>

#include <stdlib.h>
#include <signal.h>

#if defined(_WS_X11_)
extern void qt_wait_for_window_manager( QWidget* );
#endif

#if defined(_OS_UNIX_)
#include <sys/types.h>
#include <unistd.h>

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static void signalHandler( int )
{
    QFile f( QDir::homeDirPath() + "/.designerargs" );
    f.open( IO_ReadOnly );
    QString args;
    f.readLine( args, f.size() );
    QStringList lst = QStringList::split( " ", args );
    for ( QStringList::Iterator it = lst.begin(); it != lst.end(); ++it ) {
	QString arg = (*it).stripWhiteSpace();
	if ( arg[0] != '-' ) {
	    QObjectList* l = MainWindow::self->queryList( "FormWindow" );
	    FormWindow* fw = (FormWindow*) l->first();
	    FormWindow* totop;
	    bool haveit = FALSE;
	    while ( fw ) {
		haveit = haveit || fw->fileName() == arg;
		if ( haveit )
		    totop = fw;
		
		fw = (FormWindow*) l->next();
	    }
	    if ( !haveit )
		MainWindow::self->openFile( arg );
	}
    }
    MainWindow::self->raise();
    MainWindow::self->setActiveWindow();
}

#if defined(Q_C_CALLBACKS)
}
#endif

#if defined(HAVE_KDE)
#define QDesignerApplication KApplication
#else
#define QDesignerApplication QApplication
#endif

#endif

#if defined(_OS_WIN32_)
#include <qt_windows.h>
#include <process.h>

class QDesignerApplication : public QApplication
{
public:
    QDesignerApplication( int argc, char** argv )
	: QApplication( argc, argv )
    {
	DESIGNER_OPENFILE = RegisterWindowMessage("QT_DESIGNER_OPEN_FILE");
    }

protected:
    bool winEventFilter( MSG *msg );
    QDateTime lastMod;

    uint DESIGNER_OPENFILE;
};

bool QDesignerApplication::winEventFilter( MSG *msg )
{
    if ( msg->message == DESIGNER_OPENFILE ) {
	QFile f( QDir::homeDirPath() + "/.designerargs" );
	QFileInfo fi(f);
	if ( fi.lastModified() == lastMod )
	    return QApplication::winEventFilter( msg );
	lastMod = fi.lastModified();
	f.open( IO_ReadOnly );
	QString args;
	f.readLine( args, f.size() );
	QStringList lst = QStringList::split( " ", args );

	for ( QStringList::Iterator it = lst.begin(); it != lst.end(); ++it ) {
	    QString arg = (*it).stripWhiteSpace();
	    if ( arg[0] != '-' ) {
		QObjectList* l = MainWindow::self->queryList( "FormWindow" );
		FormWindow* fw = (FormWindow*) l->first();
		FormWindow* totop;
		bool haveit = FALSE;
		while ( fw ) {
		    haveit = haveit || fw->fileName() == arg;
		    if ( haveit )
			totop = fw;

		    fw = (FormWindow*) l->next();
		}
		
		if ( !haveit ) {
		    FlashWindow( MainWindow::self->winId(), TRUE );
		    MainWindow::self->openFile( arg );
		} else {
		    totop->setFocus();
		}
		delete l;
	    }
	}
	return TRUE;
    }
    return QApplication::winEventFilter( msg );
}

#endif

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static void exitHandler( int )
{
    QDir d( QDir::homeDirPath() );
    d.remove( ".designerpid" );
    exit( -1 );
}

static void crashHandler( int )
{
    MainWindow::self->saveAllTemp();
    ::exit( -1 ); 
}

#if defined(Q_C_CALLBACKS)
}
#endif

#include <qmessagebox.h>

int main( int argc, char *argv[] )
{
    signal( SIGSEGV, crashHandler );

    QApplication::setColorSpec( QApplication::ManyColor );
	
#if defined(HAVE_KDE)
    QDesignerApplication a( argc, argv, "Qt Designer" );
#else
    QDesignerApplication a( argc, argv );
#endif

    QDesignerApplication::setOverrideCursor( WaitCursor );
    bool showSplash = TRUE;

    bool creatPid = FALSE;
    if ( a.argc() > 1 ) {
	QString arg1 = a.argv()[ 1 ];
	if ( arg1 == "-client" ) {
	    QFile pf( QDir::homeDirPath() + "/.designerpid" );
	    if ( pf.exists() && pf.open( IO_ReadOnly ) ) {
		QString pidStr;
		pf.readLine( pidStr, pf.size() );
		QFile f( QDir::homeDirPath() + "/.designerargs" );
		f.open( IO_WriteOnly );
		QTextStream ts( &f );
		for ( int i = 1; i < a.argc(); ++i )
		    ts << a.argv()[ i ] << " ";
		ts << endl;
		f.close();
#if defined(_OS_UNIX_)
		if ( kill( pidStr.toInt(), SIGUSR1 ) == -1 )
		    creatPid = TRUE;
		else
		    return 0;
#elif defined(_OS_WIN32_)
		if ( !GetProcessVersion( pidStr.toUInt() ) ) {		
		    creatPid = TRUE;
		} else {
		    SendMessage( HWND_BROADCAST, RegisterWindowMessage("QT_DESIGNER_OPEN_FILE"), 0, 0 );
		    return 0;
		}
#endif
	    } else {
		creatPid = TRUE;
	    }
	}
    }

    if ( creatPid ) {
	QFile pf( QDir::homeDirPath() + "/.designerpid" );
	pf.open( IO_WriteOnly );
	QTextStream ts( &pf );
#if defined(_OS_UNIX_)
	signal( SIGUSR1, signalHandler );
#endif
	ts << getpid() << endl;

	pf.close();
	signal( SIGABRT, exitHandler );
	signal( SIGFPE, exitHandler );
	signal( SIGILL, exitHandler );
	signal( SIGINT, exitHandler );
	signal( SIGSEGV, exitHandler );
	signal( SIGTERM, exitHandler );
    }

    QString fn = QDir::homeDirPath() + "/.designerrc";
    if ( QFile::exists( fn ) ) {
	Config config( fn );
	config.setGroup( "General" );
	showSplash = config.readBoolEntry( "SplashScreen", TRUE );
    }

    QLabel *splash = 0;
    if ( showSplash ) {
	splash = new QLabel( 0, "splash", Qt::WDestructiveClose | Qt::WStyle_Customize | Qt::WStyle_NoBorder );
	splash->setFrameStyle( QFrame::WinPanel | QFrame::Raised );
	splash->setPixmap( splashScreen() );
	splash->adjustSize();
	QRect r = QApplication::desktop()->geometry();
	splash->move( r.center() - splash->rect().center() );
	splash->show();
	splash->repaint( FALSE );
	QApplication::flushX();
    }

    MainWindow *mw = new MainWindow( creatPid );
    mw->setCaption( "Qt Designer by Trolltech" );

    mw->show();
#if defined(_WS_X11_)
    qt_wait_for_window_manager( mw );
#endif
    delete splash;

    QApplication::restoreOverrideCursor();
    for ( int i = 1; i < a.argc(); ++i ) {
	QString arg = a.argv()[ i ];
	if ( arg[0] != '-' )
	    mw->openFile( arg );
    }

    a.connect( &a, SIGNAL( lastWindowClosed() ),
	       &a, SLOT( quit() ) );

    return a.exec();
}
