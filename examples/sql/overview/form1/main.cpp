/****************************************************************************
** $Id$
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#include <qapplication.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qsqldatabase.h>
#include <qsqlcursor.h>
#include <qsqlform.h>

bool create_connections();


class FormDialog : public QDialog
{
    public:
	FormDialog();
};


FormDialog::FormDialog()
{
    QLabel *forenameLabel   = new QLabel( "Forename:", this );
    QLabel *forenameDisplay = new QLabel( this );
    QLabel *surnameLabel    = new QLabel( "Surname:", this );
    QLabel *surnameDisplay  = new QLabel( this );
    QLabel *salaryLabel	    = new QLabel( "Salary:", this );
    QLineEdit *salaryEdit   = new QLineEdit( this );

    QGridLayout *grid = new QGridLayout( this );
    grid->addWidget( forenameLabel,	0, 0 );
    grid->addWidget( forenameDisplay,	0, 1 );
    grid->addWidget( surnameLabel,	1, 0 );
    grid->addWidget( surnameDisplay,	1, 1 );
    grid->addWidget( salaryLabel,	2, 0 );
    grid->addWidget( salaryEdit,	2, 1 );
    grid->activate();

    QSqlCursor staffCursor( "staff" );
    staffCursor.select();
    staffCursor.next();

    QSqlForm sqlForm( this );
    sqlForm.insert( forenameDisplay, "forename" );
    sqlForm.insert( surnameDisplay, "surname" );
    sqlForm.insert( salaryEdit, "salary" );
    sqlForm.setRecord( staffCursor.primeUpdate() );
    sqlForm.readFields();
}


int main( int argc, char *argv[] )
{
    QApplication app( argc, argv );

    if ( ! create_connections() ) return 1;

    FormDialog *formDialog = new FormDialog();
    formDialog->show();
    app.setMainWidget( formDialog );

    return app.exec();
}


bool create_connections()
{
    // create the default database connection
    QSqlDatabase *defaultDB = QSqlDatabase::addDatabase( "QODBC" );
    defaultDB->setDatabaseName( "sales" );
    defaultDB->setUserName( "salesuser" );
    defaultDB->setPassword( "salespw" );
    defaultDB->setHostName( "saleshost" );
    if ( ! defaultDB->open() ) { 
	qWarning( "Failed to open sales database: " + 
		  defaultDB->lastError().driverText() );
	qWarning( defaultDB->lastError().databaseText() );
	return false;
    }

    // create a named connection to oracle
    QSqlDatabase *oracle = QSqlDatabase::addDatabase( "QOCI", "ORACLE" );
    oracle->setDatabaseName( "orders" );
    oracle->setUserName( "ordersuser" );
    oracle->setPassword( "orderspw" );
    oracle->setHostName( "ordershost" );
    if ( ! oracle->open() ) {
	qWarning( "Failed to open orders database: " + 
		  oracle->lastError().driverText() );
	qWarning( oracle->lastError().databaseText() );
	return false;
    }

    return true;
}


