/****************************************************************************
** $Id: //depot/qt/main/examples/qfileiconview/main.cpp#10 $
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#include "nntp.h"
#include <qurlinfo.h>
#include <stdlib.h>
#include <qurloperator.h>
#include <qstringlist.h>
#include <qregexp.h>

Nntp::Nntp()
    : QNetworkProtocol(), connectionReady( FALSE ),
      readGroups( FALSE ), readHead( FALSE ), readBody( FALSE )
{
    // create the command socket and connect to its signals
    commandSocket = new QSocket( this );
    connect( commandSocket, SIGNAL( hostFound() ),
	     this, SLOT( hostFound() ) );
    connect( commandSocket, SIGNAL( connected() ),
	     this, SLOT( connected() ) );
    connect( commandSocket, SIGNAL( closed() ),
	     this, SLOT( closed() ) );
    connect( commandSocket, SIGNAL( readyRead() ),
	     this, SLOT( readyRead() ) );
    connect( commandSocket, SIGNAL( error() ),
	     this, SLOT( error() ) );
    commandSocket->setMode( QSocket::Ascii );
}

Nntp::~Nntp()
{
    close();
    delete commandSocket;
}

void Nntp::operationListChildren( QNetworkOperation * )
{
    // for listing dirs let's switch to ASCII mode, as we only want to get lines
    commandSocket->setMode( QSocket::Ascii );

    // create a command
    QString path = url()->path(), cmd;
    if ( path.isEmpty() || path == "/" ) {
	// if the path is empty or we are in the root dir,
	// we want to read the list of available newsgroups
	cmd = "list newsgroups\r\n";
    } else if ( url()->isDir() ) {
	// if the path is a directory (in our case a news group)
	// we want to list the articles of this group
	path = path.replace( QRegExp( "/" ), "" );
	cmd = "listgroup " + path + "\r\n";
    } else
	return;

    // write the command to the socket
    commandSocket->writeBlock( cmd.latin1(), cmd.length() );
    readGroups = TRUE;
}

void Nntp::operationGet( QNetworkOperation *op )
{
    // for  reading and article let's also switch to ASCII mode,
    // as we want to read it line after line.
    commandSocket->setMode( QSocket::Ascii );

    // get the dirPath of the URL (this is our news group)
    // and the filename (which is the article we want to read)
    QUrl u( op->arg1() );
    QString dirPath = u.dirPath(), file = u.fileName();
    dirPath = dirPath.replace( QRegExp( "/" ), "" );

    // go to the group in which the article is
    QString cmd;
    cmd = "group " + dirPath + "\r\n";
    commandSocket->writeBlock( cmd.latin1(), cmd.length() );

    // read the head of the article
    cmd = "head " + file + "\r\n";
    commandSocket->writeBlock( cmd.latin1(), cmd.length() );
    readHead = TRUE;
}

bool Nntp::checkConnection( QNetworkOperation * )
{
    // we are connected, return TRUE
    if ( commandSocket->isOpen() && connectionReady )
	return TRUE;

    // seems that there is no chance to connect
    if ( commandSocket->isOpen() )
	return FALSE;

    // start connecting
    connectionReady = FALSE;
    commandSocket->connectToHost( url()->host(),
				  url()->port() != -1 ? url()->port() : 119 );
    return FALSE;
}

void Nntp::close()
{
    // close the command socket
    if ( commandSocket->isOpen() ) {
 	commandSocket->writeBlock( "quit\r\n", strlen( "quit\r\n" ) );
 	commandSocket->close();
    }
}

int Nntp::supportedOperations() const
{
    // we only support listing children and getting data
    return OpListChildren | OpGet;
}

void Nntp::hostFound()
{
    if ( url() )
	emit connectionStateChanged( ConHostFound, tr( "Host %1 found" ).arg( url()->host() ) );
    else
	emit connectionStateChanged( ConHostFound, tr( "Host found" ) );
}

void Nntp::connected()
{
    if ( url() )
	emit connectionStateChanged( ConConnected, tr( "Connected to host %1" ).arg( url()->host() ) );
    else
	emit connectionStateChanged( ConConnected, tr( "Connected to host" ) );
}

void Nntp::closed()
{
    if ( url() )
	emit connectionStateChanged( ConClosed, tr( "Connection to %1 closed" ).arg( url()->host() ) );
    else
	emit connectionStateChanged( ConClosed, tr( "Connection closed" ) );
}

void Nntp::readyRead()
{
    // new data arrived on the command socket

    // of we should read the list of available groups, let's do so
    if ( readGroups ) {
	parseGroups();
	return;
    }

    // of we should read an article, let's do so
    if ( readHead || readBody ) {
	readArticle();
	return;
    }

    // read the new data from the socket
    QCString s;
    s.resize( commandSocket->bytesAvailable() );
    commandSocket->readBlock( s.data(), commandSocket->bytesAvailable() );

    if ( !url() )
	return;

    // of the code of the server response was 200, we know that the
    // server is ready to get commands from us now
    if ( s.left( 3 ) == "200" )
	connectionReady = TRUE;
}

void Nntp::parseGroups()
{
    if ( !commandSocket->canReadLine() )
	return;

    // read one line after the other
    while ( commandSocket->canReadLine() ) {
	QString s = commandSocket->readLine();

	// if the  line starts with a dot, all groups or articles have been listed,
	// so we finished processing the listChildren() command
	if ( s[ 0 ] == '.' ) {
	    readGroups = FALSE;
	    operationInProgress()->setState( StDone );
	    emit finished( operationInProgress() );
	    return;
	}
	
	// if the code of the server response is 215 or 211
	// the next line will be the first group or article (depending on what we read).
	// So let others know that we start reading now...
	if ( s.left( 3 ) == "215" || s.left( 3 ) == "211" ) {
	    operationInProgress()->setState( StInProgress );
	    emit start( operationInProgress() );
	    continue;
	}
	
	// parse the line and create a QUrlInfo object
	// which describes the child (group or article)
	bool tab = s.find( '\t' ) != -1;
	QString group = s.mid( 0, s.find( tab ? '\t' : ' ' ) );
	QUrlInfo inf;
	inf.setName( group );
	QString path = url()->path();
	inf.setDir( path.isEmpty() || path == "/" );
	inf.setSymLink( FALSE );
	inf.setFile( !inf.isDir() );
	inf.setWritable( FALSE );
	inf.setReadable( TRUE );
	
	// let others know about our new child
	emit newChild( inf, operationInProgress() );
    }
	
}

void Nntp::readArticle()
{
    if ( !commandSocket->canReadLine() )
	return;

    // read an article one line after the other
    while ( commandSocket->canReadLine() ) {
	QString s = commandSocket->readLine();

	// if the  line starts with a dot, we finished reading something
	if ( s[ 0 ] == '.' ) {
	    // if we read the head, now let the server know, that we want to read
	    // the body of the article
	    if ( readHead ) {
		readHead = FALSE;
		readBody =TRUE;
		QString cmd = "body " + QUrl( operationInProgress()->arg1() ).fileName() + "\r\n";
		commandSocket->writeBlock( cmd.latin1(), cmd.length() );
	    } else {
		// we read the body, so we finised processing this operation
		// let others know about that
		readBody = FALSE;
		operationInProgress()->setState( StDone );
		emit finished( operationInProgress() );
	    }
	    return;
	}

	if ( s.right( 1 ) == "\n" )
	    s.remove( s.length() - 1, 1 );
	
	// emit the new data of the article which we read
	emit data( QCString( s.ascii() ), operationInProgress() );
    }
}

void Nntp::error()
{
    // this signal is called if connecting to the server failed
    if ( operationInProgress() ) {
	QString msg = tr( "Host not found or couldn't connect to: \n" + url()->host() );
	operationInProgress()->setState( StFailed );
	operationInProgress()->setProtocolDetail( msg );
	operationInProgress()->setErrorCode( (int)ErrHostNotFound );
	clearOperationQueue();
	emit finished( operationInProgress() );
    }
}
