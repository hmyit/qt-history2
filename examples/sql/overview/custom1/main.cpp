/****************************************************************************
** $Id$
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#include "main.h"


CustomEdit::CustomEdit( QWidget *parent, const char *name ) : 
    QLineEdit( parent, name )
{
    connect( this, SIGNAL(textChanged(const QString &)), 
	     this, SLOT(changed(const QString &)) );
}


void CustomEdit::changed( const QString &line )
{
    setUpperLine( line );
}


void CustomEdit::setUpperLine( const QString &line )
{
    upperLineText = line.upper();
    setText( upperLineText );
}


QString CustomEdit::upperLine() const 
{
    return upperLineText;
}


FormDialog::FormDialog()
{
    QLabel	*forenameLabel	= new QLabel( "Forename:", this );
    CustomEdit	*forenameEdit	= new CustomEdit( this );
    QLabel	*surnameLabel   = new QLabel( "Surname:", this );
    CustomEdit	*surnameEdit	= new CustomEdit( this );
    QLabel	*salaryLabel	= new QLabel( "Salary:", this );
    QLineEdit	*salaryEdit	= new QLineEdit( this );
    salaryEdit->setAlignment( Qt::AlignRight );
    QPushButton *saveButton	= new QPushButton( "&Save", this );
    connect( saveButton, SIGNAL(clicked()), this, SLOT(save()) );

    QGridLayout *grid = new QGridLayout( this );
    grid->addWidget( forenameLabel, 0, 0 );
    grid->addWidget( forenameEdit,  0, 1 );
    grid->addWidget( surnameLabel,  1, 0 );
    grid->addWidget( surnameEdit,   1, 1 );
    grid->addWidget( salaryLabel,   2, 0 );
    grid->addWidget( salaryEdit,    2, 1 );
    grid->addWidget( saveButton,    3, 0 );
    grid->activate();

    staffCursor = new QSqlCursor( "staff" );
    staffCursor->setTrimmed( "forename", TRUE );
    staffCursor->setTrimmed( "surname",  TRUE );
    idIndex = staffCursor->index( "id" );
    staffCursor->select( idIndex );
    staffCursor->first();

    propMap = new QSqlPropertyMap;
    propMap->insert( forenameEdit->className(), "upperLine" );

    sqlForm = new QSqlForm( this );
    sqlForm->setRecord( staffCursor->primeUpdate() );
    sqlForm->installPropertyMap( propMap );
    sqlForm->insert( forenameEdit, "forename" );
    sqlForm->insert( surnameEdit, "surname" );
    sqlForm->insert( salaryEdit, "salary" );
    sqlForm->readFields();
}


FormDialog::~FormDialog()
{
    delete staffCursor;
}


void FormDialog::save()
{
    sqlForm->writeFields();
    staffCursor->update();
    staffCursor->select( idIndex );
    staffCursor->first();
}


int main( int argc, char *argv[] )
{
    QApplication app( argc, argv );

    if ( ! createConnections() ) 
	return 1;

    FormDialog *formDialog = new FormDialog();
    formDialog->show();
    app.setMainWidget( formDialog );

    return app.exec();
}


bool createConnections()
{
    // create the default database connection
    QSqlDatabase *defaultDB = QSqlDatabase::addDatabase( DB_SALES_DRIVER );
    if ( ! defaultDB ) {
	qWarning( "Failed to connect to driver" );
	return FALSE;
    }
    defaultDB->setDatabaseName( DB_SALES_DBNAME );
    defaultDB->setUserName( DB_SALES_USER );
    defaultDB->setPassword( DB_SALES_PASSWD );
    defaultDB->setHostName( DB_SALES_HOST );
    if ( ! defaultDB->open() ) { 
	qWarning( "Failed to open sales database: " + 
		  defaultDB->lastError().driverText() );
	qWarning( defaultDB->lastError().databaseText() );
	return FALSE;
    }

    // create a named connection to oracle
    QSqlDatabase *oracle = QSqlDatabase::addDatabase( DB_ORDERS_DRIVER, "ORACLE" );
    if ( ! oracle ) {
	qWarning( "Failed to connect to oracle driver" );
	return FALSE;
    }
    oracle->setDatabaseName( DB_ORDERS_DBNAME );
    oracle->setUserName( DB_ORDERS_USER );
    oracle->setPassword( DB_ORDERS_PASSWD );
    oracle->setHostName( DB_ORDERS_HOST );
    if ( ! oracle->open() ) {
	qWarning( "Failed to open orders database: " + 
		  oracle->lastError().driverText() );
	qWarning( oracle->lastError().databaseText() );
	return FALSE;
    }

    return TRUE;
}


