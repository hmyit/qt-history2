/****************************************************************************
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of an example program for Qt.
** EDITIONS: NOLIMITS
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <qapplication.h>
#include <qtable.h>

// Table size

const int numRows = 1000000;
const int numCols = 1000000;

class MyTable : public QTable
{
public:
    MyTable( int r, int c ) : QTable( r, c ) {
	items.setAutoDelete( TRUE );
	widgets.setAutoDelete( TRUE );
	setWindowTitle( tr( "A 1 Million x 1 Million Cell Table" ) );
	setLeftMargin( fontMetrics().width( "W999999W" ) );
    }

    void resizeData( int ) {}
    QTableItem *item( int r, int c ) const { return items.find( indexOf( r, c ) ); }
    void setItem( int r, int c, QTableItem *i ) { items.replace( indexOf( r, c ), i ); }
    void clearCell( int r, int c ) { items.remove( indexOf( r, c ) ); }
    void takeItem( QTableItem *item )
    {
	items.setAutoDelete( FALSE );
	items.remove( indexOf( item->row(), item->col() ) );
	items.setAutoDelete( TRUE );
    }
    void insertWidget( int r, int c, QWidget *w ) { widgets.replace( indexOf( r, c ), w );  }
    QWidget *cellWidget( int r, int c ) const { return widgets.find( indexOf( r, c ) ); }
    void clearCellWidget( int r, int c )
    {
	QWidget *w = widgets.take( indexOf( r, c ) );
	if ( w )
	    w->deleteLater();
    }

private:
    QIntDict<QTableItem> items;
    QIntDict<QWidget> widgets;

};

// The program starts here.

int main( int argc, char **argv )
{
    QApplication app( argc, argv );

    MyTable table( numRows, numCols );
    app.setMainWidget( &table );
    table.show();
    return app.exec();
}
