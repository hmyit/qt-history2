#include "app.h"
#include <qsqlcursor.h>
#include <qregexp.h>
#include <qfile.h>

ImportApp::ImportApp( int argc, char** argv ) : QApplication( argc, argv )
{
    internDB = QSqlDatabase::addDatabase( "QMYSQL3", "intern" );
    internDB->setUserName( QString::null );
    internDB->setDatabaseName( "troll" );
    internDB->setPassword( QString::null );
    internDB->setHostName( "lupinella.troll.no" );
    axaptaDB = QSqlDatabase::addDatabase( "QOCI8", "axapta" );
    axaptaDB->setUserName( "bmssa" );
    axaptaDB->setDatabaseName( "axdb.troll.no" );
    axaptaDB->setPassword( "bmssa_pwd" );
    axaptaDB->setHostName( "minitrue.troll.no" );
}

void ImportApp::doImport()
{
    QStringList skippedIds;

    if( internDB->open() ) {
	if( axaptaDB->open() ) {
	    QSqlCursor internCursor( "tempCustomer", true, internDB );

	    internCursor.select();
	    internCursor.first();
	    while( internCursor.isValid() ) {
		QString customerID = internCursor.value( "customerid" ).toString();
		QString customerName = internCursor.value( "customername" ).toString();
		QString customerAddress = internCursor.value( "address" ).toString();

		customerAddress.replace( QRegExp( "[\\r,\\n]" ), " " );
		QSqlCursor axaptaCursor( "CustTable", true, axaptaDB );

		axaptaCursor.select( "Name = '" + customerName + "' and replace( replace( Address, chr( 13 ), ' '), chr( 10 ), ' ' ) = '" + customerAddress + "' and dataareaid = 'ts3'" );
		axaptaCursor.first();
		if( axaptaCursor.isValid() ) {
		    // We found a record, hooray :)
		    QSqlRecord* buffer = axaptaCursor.primeUpdate();
		    QSqlQuery q( QString::null, axaptaDB );
		    QString tmp = "UPDATE CUSTTABLE SET INTERNID = '" + customerID + "' WHERE DATAAREAID = 'ts3' AND ACCOUNTNUM = '" + buffer->value( "ACCOUNTNUM" ).toString() + "'";
		    bool b = q.exec( tmp );
		}
		else {
		    skippedIds += customerID;
		}

		internCursor.next();
	    }
	}
    }
    QFile logFile( "skipped.log" );

    if( logFile.open( IO_WriteOnly | IO_Translate ) ) {
	QTextStream log( &logFile );
	log << skippedIds.join( "\n" );
	log << endl;
	logFile.close();
    }
}
