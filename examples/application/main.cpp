/****************************************************************************
** $Id: //depot/qt/main/examples/application/main.cpp#2 $
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#include <qapplication.h>
#include "application.h"

int main( int argc, char ** argv ) 
{
    QApplication a( argc, argv );

    ApplicationWindow * mw = new ApplicationWindow();
    mw->resize( 640, 480 );
    mw->setCaption( "Document 1" );
    a.setMainWidget( mw );
    mw->show();

    a.connect( &a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()) );
    return a.exec();
}
