/****************************************************************************
** $Id: //depot/qt/main/examples/process/process.cpp#6 $
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#include <qobject.h>
#include <qprocess.h>
#include <qvbox.h>
#include <qtextview.h>
#include <qpushbutton.h>
#include <qapplication.h>
#include <qmessagebox.h>

#if defined(Q_OS_WIN32)
#include <process.h>
#endif

class UicManager : public QVBox
{
    Q_OBJECT

public:
    UicManager();
    ~UicManager() {}

public slots:
    void readFromStdout();
    void scrollToTop();

private:
    QProcess *proc;
    QTextView *output;
    QPushButton *quitButton;
};

UicManager::UicManager()
{
    // Layout
    output = new QTextView( this );
    quitButton = new QPushButton( tr("Quit"), this );
    connect( quitButton, SIGNAL(clicked()),
	    qApp, SLOT(quit()) );
    resize( 500, 500 );

    // QProcess related code
    proc = new QProcess( this );

    // Set up the command and arguments.
    // On the command line you would do:
    //   uic -tr i18n "small dialog.ui"
    proc->addArgument( "uic" );
    proc->addArgument( "-tr" );
    proc->addArgument( "i18n" );
    proc->addArgument( "small dialog.ui" );

    connect( proc, SIGNAL(readyReadStdout()),
	    this, SLOT(readFromStdout()) );
    connect( proc, SIGNAL(processExited()),
	    this, SLOT(scrollToTop()) );

    if ( !proc->start() ) {
	// error handling
	QMessageBox::critical( 0,
		tr("Fatal error"),
		tr("Could not start the uic command."),
		tr("Quit") );
	exit( -1 );
    }
}

void UicManager::readFromStdout()
{
    // Read and process the data.
    // Keep in mind that the data might be reported in chunks.
    output->append( proc->readStdout() );
}

void UicManager::scrollToTop()
{
    output->setContentsPos( 0, 0 );
}

int main( int argc, char **argv )
{
    QApplication a( argc, argv );
    UicManager manager;
    a.setMainWidget( &manager );
    manager.show();
    return a.exec();
}

#include "process.moc"
