/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qnetworkprotocol.cpp#43 $
**
** Implementation of QNetworkProtocol class
**
** Created : 950429
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qnetworkprotocol.h"

#ifndef QT_NO_NETWORKPROTOCOL

#include "qlocalfs.h"
#include "qurloperator.h"
#include "qtimer.h"
#include "qmap.h"
#include "qptrqueue.h"

//#define QNETWORKPROTOCOL_DEBUG
#define NETWORK_OP_DELAY 1000

extern Q_EXPORT QNetworkProtocolDict *qNetworkProtocolRegister;

QNetworkProtocolDict *qNetworkProtocolRegister = 0;

class QNetworkProtocol::Private
{
public:
    QUrlOperator *url;
    QPtrQueue< QNetworkOperation > operationQueue;
    QNetworkOperation *opInProgress;
    QTimer *opStartTimer, *removeTimer;
    int removeInterval;
    bool autoDelete;
    QPtrList< QNetworkOperation > oldOps;
};

// NOT REVISED
/*!
  \class QNetworkProtocol qnetworkprotocol.h
  \brief The QNetworkProtocol class provides a common API for network protocols.

  \ingroup io

  This is a base class which should be used for implementations
  of network protocols that can then be used in Qt (e.g.,
  in the filedialog) together with the QUrlOperator.

  The easiest way to implement a new network protocol is to
  reimplement the operation[something]( QNetworkOperation * )
  methods. Of course only the ones that are supported should
  be reimplemented. To specify which operations are supported,
  also reimplement supportedOperations() and return an int there,
  that is OR'd together using the supported operations from
  the QNetworkProtocol::Operation enum.

  When you implement a network protocol this way, be careful
  always to emit the correct signals. Also, always emit
  the finished() signal when an operation is done (on failure or
  success!). The Qt Network Architecture relies on correctly emitted
  finished() signals.

  For a detailed description of the Qt Network Architecture and
  also how to implement and use network protocols in Qt, see the <a href="network.html">Qt Network Documentation</a>.
*/

/*!
  \fn void QNetworkProtocol::newChildren( const QValueList<QUrlInfo> &i, QNetworkOperation *op )

  This signal is emitted after listChildren() was called and
  new children (files) have been read from the list of files. \a i
  holds the information about the new children.
  \a op is the pointer to the operation object which contains all infos
  of the operation, including the state, etc.

  When a protocol emits this signal, QNetworkProtocol is smart enough
  to let the QUrlOperator, which is used by the network protocol, emit
  its corresponding signal.

  When implementing your own network protocol and reading children, you usually don't read one child at once, but rather a list of them. That's why this signal
  takes a list of QUrlInfo objects. But if you read only one child at once you can
  use the convenience signal newChild(), which takes only a single QUrlInfo object.
*/

/*!
  \fn void QNetworkProtocol::newChild( const QUrlInfo &i, QNetworkOperation *op )

  This signal is emitted if a new child has been read. QNetworkProtocol
  automatically connects it to a slot which creates a list of QUrlInfo objects
  (with just the one QUrlInfo \a i) and emits the newChildren() signal with this
  created list.

  This is just a convenience signal when implementing your own network protocol. In all
  other cases worry only about the newChildren() signal with the list of QUrlInfo objects.
*/

/*!
  \fn void QNetworkProtocol::finished( QNetworkOperation *op )

  This signal is emitted when an operation of some sort finishes.
  This signal is always emitted, whether success or failure.
  \a op is the pointer to the operation object which contains all infos
  of the operation that has finished, including the state, etc.
  Check the state and error code of the operation object to determine whether or not the operation was successful.

  When a protocol emits this signal, QNetworkProtocol is smart enough
  to let the QUrlOperator, which is used by the network protocol, emit
  its corresponding signal.
*/

/*!
  \fn void QNetworkProtocol::start( QNetworkOperation *op )

  Some operations (such as listChildren()) emit this signal
  when they start processing the operation.
  \a op is the pointer to the operation object which contains all infos
  of the operation, including the state, etc.

  When a protocol emits this signal, QNetworkProtocol is smart enough
  to let the QUrlOperator, which is used by the network protocol, emit
  its corresponding signal.
*/

/*!
  \fn void QNetworkProtocol::createdDirectory( const QUrlInfo &i, QNetworkOperation *op )

  This signal is emitted when mkdir() has been succesful
  and the directory has been created. \a i holds the information
  about the new directory.
  \a op is the pointer to the operation object which contains all infos
  of the operation, including the state, etc. Using op->arg( 0 ),
  you also get the file name of the new directory.

  When a protocol emits this signal, QNetworkProtocol is smart enough
  to let the QUrlOperator, which is used by the network protocol, emit
  its corresponding signal.
*/

/*!
  \fn void QNetworkProtocol::removed( QNetworkOperation *op )

  This signal is emitted when remove() has been succesful
  and the file has been removed. \a op holds the file name
  of the removed file in the first argument; you get it
  with op->arg( 0 ).

  \a op is the pointer to the operation object, which contains all infos
  of the operation, including the state and so on.

  When a protocol emits this signal, QNetworkProtocol is smart enough
  to let the QUrlOperator, which is used by the network protocol, emit
  its corresponding signal.
*/

/*!
  \fn void QNetworkProtocol::itemChanged( QNetworkOperation *op )

  This signal is emitted whenever a file which is a child of this URL
  has been changed, e.g., by successfully calling rename(). \a op holds
  the original and the new file names in the first and second arguments.
  You get them with op->arg( 0 ) and op->arg( 1 ).

  \a op is the pointer to the operation object, which contains all infos
  of the operation, including the state and so on.

  When a protocol emits this signal, QNetworkProtocol is smart enough
  to let the QUrlOperator, which is used by the network protocol, emit
  its corresponding signal.
*/

/*!
  \fn void QNetworkProtocol::data( const QByteArray &data, QNetworkOperation *op )

  This signal is emitted when new \a data has been received after calling
  get() or put(). \a op holds the name of the file in which data is
  retrieved its first argument, and the (raw) data in its second argument.
  You can fetch them with op->arg( 0 ) and op->rawArg( 1 ).

  Note that \a op contains all information about the operation, including
  the state.

  When a protocol emits this signal, QNetworkProtocol is smart enough to
  let the QUrlOperator (which is used by the network protocol) emit its
  corresponding signal.
*/

/*!
  \fn void QNetworkProtocol::dataTransferProgress( int bytesDone, int bytesTotal, QNetworkOperation *op )

  This signal is emitted during the progress when transferring data (using put() or get()).
  \a bytesDone tells how many bytes of \a bytesTotal are transferred. More information
  about the operation is stored in the \a op, the pointer to the network operation
  which is processed. \a bytesTotal may be -1, which means that the number of total
  bytes is not known.

  When a protocol emits this signal, QNetworkProtocol is smart enough
  to let the QUrlOperator, which is used by the network protocol, emit
  its corresponding signal.
*/

/*!
  \fn void QNetworkProtocol::connectionStateChanged( int state, const QString &data )

  This signal is emitted whenever the state of the connection of
  the network protocol is changed. \a state describes the new state,
  which is one of
	ConHostFound,
	ConConnected and
	ConClosed.
  \a data is a message text.
*/

/*!
  \enum QNetworkProtocol::State

  This enum contains the state that a QNetworkOperation
  can have:

  \value StWaiting  The operation is in the queue of the
  QNetworkProtocol and is waiting for being prcessed.

  \value StInProgress  The operation is being processed.

  \value StDone  The operation has been processed succesfully.

  \value StFailed  The operation has been processed but an error occurred.

  \value StStopped  The operation has been processed but has been
  stopped before it finished, and is waiting for being prcessed.

*/

/*!
  \enum QNetworkProtocol::Operation

  This enum lists all possible operations that a network protocol
  can support. supportedOperations() returns an int that is OR'd
  together. Also, the type() or a QNetworkOperation
  is always one of these values.

  \value OpListChildren  List the children of a URL, e.g., of a directory.
  \value OpMkdir  Create a directory.
  \value OpRemove  Remove a child (e.g., file).
  \value OpRename  Rename a child (e.g., file).
  \value OpGet  Get data from a location.
  \value OpPut  Put data to a location.
*/

/*!
  \enum QNetworkProtocol::ConnectionState

  When the connection state of a network protocol changes it emits
  the signal connectionStateChanged(). The first argument is one
  of the following values:

  \value ConHostFound  Host has been found.
  \value ConConnected  Connection to the host has been established.
  \value ConClosed  Connection has been closed.
*/

/*!
  \enum QNetworkProtocol::Error

  When an operation fails (finishes without success), the QNetworkOperation
  of the operation returns an error code which is one of the following values:

  \value NoError  No error occurred.

  \value ErrValid  The URL you are operating on is not valid.

  \value ErrUnknownProtocol  There is no protocol implementation
  available for the protocol of the URL you are operating on (e.g., if the
  protocol is http and no http implementation has been registered).

  \value ErrUnsupported  The operation is not supported by the protocol.

  \value ErrParse  Parse error of the URL.

  \value ErrLoginIncorrect  You needed to login but the username and/or
  password are wrong.

  \value ErrHostNotFound  The specified host (in the URL) couldn't be
  found.

  \value ErrListChildren  An error occurred while listing the children.

  \value ErrMkdir  An error occurred when creating a directory.

  \value ErrRemove  An error occurred while removing a child.

  \value ErrRename   An error occurred while renaming a child.

  \value ErrGet  An error occurred while getting (retrieving) data.

  \value ErrPut  An error occurred while putting (uploading) data.

  \value ErrFileNotExisting  A file which is needed by the operation
  doesn't exist.

  \value ErrPermissionDenied  The permission for doing the operation has
  been denied.

  You should also use these values of error codes when implementing custom
  network protocols. If this is not possible, you can define your own ones
  by using an integer value that doesn't conflict with one of these
  values.
*/

/*!
  Constructor of the network protocol base class. Does some initialization
  and connecting of signals and slots.
*/

QNetworkProtocol::QNetworkProtocol()
    : QObject()
{

    d = new Private;
    d->url = 0;
    d->opInProgress = 0;
    d->opStartTimer = new QTimer( this );
    d->removeTimer = new QTimer( this );
    d->operationQueue.setAutoDelete( FALSE );
    d->autoDelete = FALSE;
    d->removeInterval = 10000;
    d->oldOps.setAutoDelete( FALSE );
    connect( d->opStartTimer, SIGNAL( timeout() ),
	     this, SLOT( startOps() ) );
    connect( d->removeTimer, SIGNAL( timeout() ),
	     this, SLOT( removeMe() ) );

    if ( url() ) {
	connect( this, SIGNAL( data( const QByteArray &, QNetworkOperation * ) ),
		 url(), SIGNAL( data( const QByteArray &, QNetworkOperation * ) ) );
	connect( this, SIGNAL( finished( QNetworkOperation * ) ),
		 url(), SIGNAL( finished( QNetworkOperation * ) ) );
	connect( this, SIGNAL( start( QNetworkOperation * ) ),
		 url(), SIGNAL( start( QNetworkOperation * ) ) );
	connect( this, SIGNAL( newChildren( const QValueList<QUrlInfo> &, QNetworkOperation * ) ),
		 url(), SIGNAL( newChildren( const QValueList<QUrlInfo> &, QNetworkOperation * ) ) );
	connect( this, SIGNAL( newChildren( const QValueList<QUrlInfo> &, QNetworkOperation * ) ),
		 url(), SLOT( addEntry( const QValueList<QUrlInfo> & ) ) );
	connect( this, SIGNAL( createdDirectory( const QUrlInfo &, QNetworkOperation * ) ),
		 url(), SIGNAL( createdDirectory( const QUrlInfo &, QNetworkOperation * ) ) );
	connect( this, SIGNAL( removed( QNetworkOperation * ) ),
		 url(), SIGNAL( removed( QNetworkOperation * ) ) );
	connect( this, SIGNAL( itemChanged( QNetworkOperation * ) ),
		 url(), SIGNAL( itemChanged( QNetworkOperation * ) ) );
	connect( this, SIGNAL( dataTransferProgress( int, int, QNetworkOperation * ) ),
		 url(), SIGNAL( dataTransferProgress( int, int, QNetworkOperation * ) ) );
	connect( this, SIGNAL( connectionStateChanged( int, const QString & ) ),
		 url(), SIGNAL( connectionStateChanged( int, const QString & ) ) );
    }

    connect( this, SIGNAL( finished( QNetworkOperation * ) ),
	     this, SLOT( processNextOperation( QNetworkOperation * ) ) );
    connect( this, SIGNAL( newChild( const QUrlInfo &, QNetworkOperation * ) ),
	     this, SLOT( emitNewChildren( const QUrlInfo &, QNetworkOperation * ) ) );

}

/*!
  Destructor.
*/

QNetworkProtocol::~QNetworkProtocol()
{
    if ( !d )
	return;
    d->removeTimer->stop();
    if ( d->opInProgress == d->operationQueue.head() )
	d->operationQueue.dequeue();
    if ( d->opInProgress )
	d->opInProgress->free();
    d->opInProgress = 0;
    while ( d->operationQueue.head() ) {
	d->operationQueue.head()->free();
	d->operationQueue.dequeue();
    }
    while ( d->oldOps.first() ) {
	d->oldOps.first()->free();
	d->oldOps.removeFirst();
    }
    delete d->opStartTimer;
    d->opStartTimer = 0;
    delete d;
    d = 0;
}

/*!
  Sets the QUrlOperator, on which the protocol works.

  \sa QUrlOperator
*/

void QNetworkProtocol::setUrl( QUrlOperator *u )
{
    if ( url() ) {
	disconnect( this, SIGNAL( data( const QByteArray &, QNetworkOperation * ) ),
		    url(), SIGNAL( data( const QByteArray &, QNetworkOperation * ) ) );
	disconnect( this, SIGNAL( finished( QNetworkOperation * ) ),
		    url(), SIGNAL( finished( QNetworkOperation * ) ) );
	disconnect( this, SIGNAL( start( QNetworkOperation * ) ),
		    url(), SIGNAL( start( QNetworkOperation * ) ) );
	disconnect( this, SIGNAL( newChildren( const QValueList<QUrlInfo> &, QNetworkOperation * ) ),
		    url(), SIGNAL( newChildren( const QValueList<QUrlInfo> &, QNetworkOperation * ) ) );
	disconnect( this, SIGNAL( newChildren( const QValueList<QUrlInfo> &, QNetworkOperation * ) ),
		    url(), SLOT( addEntry( const QValueList<QUrlInfo> & ) ) );
	disconnect( this, SIGNAL( createdDirectory( const QUrlInfo &, QNetworkOperation * ) ),
		    url(), SIGNAL( createdDirectory( const QUrlInfo &, QNetworkOperation * ) ) );
	disconnect( this, SIGNAL( removed( QNetworkOperation * ) ),
		    url(), SIGNAL( removed( QNetworkOperation * ) ) );
	disconnect( this, SIGNAL( itemChanged( QNetworkOperation * ) ),
		    url(), SIGNAL( itemChanged( QNetworkOperation * ) ) );
	disconnect( this, SIGNAL( dataTransferProgress( int, int, QNetworkOperation * ) ),
		    url(), SIGNAL( dataTransferProgress( int, int, QNetworkOperation * ) ) );
	disconnect( this, SIGNAL( connectionStateChanged( int, const QString & ) ),
		    url(), SIGNAL( connectionStateChanged( int, const QString & ) ) );
    }

    d->url = u;

    if ( url() ) {
	connect( this, SIGNAL( data( const QByteArray &, QNetworkOperation * ) ),
		 url(), SIGNAL( data( const QByteArray &, QNetworkOperation * ) ) );
	connect( this, SIGNAL( finished( QNetworkOperation * ) ),
		 url(), SIGNAL( finished( QNetworkOperation * ) ) );
	connect( this, SIGNAL( start( QNetworkOperation * ) ),
		 url(), SIGNAL( start( QNetworkOperation * ) ) );
	connect( this, SIGNAL( newChildren( const QValueList<QUrlInfo> &, QNetworkOperation * ) ),
		 url(), SIGNAL( newChildren( const QValueList<QUrlInfo> &, QNetworkOperation * ) ) );
	connect( this, SIGNAL( newChildren( const QValueList<QUrlInfo> &, QNetworkOperation * ) ),
		 url(), SLOT( addEntry( const QValueList<QUrlInfo> & ) ) );
	connect( this, SIGNAL( createdDirectory( const QUrlInfo &, QNetworkOperation * ) ),
		 url(), SIGNAL( createdDirectory( const QUrlInfo &, QNetworkOperation * ) ) );
	connect( this, SIGNAL( removed( QNetworkOperation * ) ),
		 url(), SIGNAL( removed( QNetworkOperation * ) ) );
	connect( this, SIGNAL( itemChanged( QNetworkOperation * ) ),
		 url(), SIGNAL( itemChanged( QNetworkOperation * ) ) );
	connect( this, SIGNAL( dataTransferProgress( int, int, QNetworkOperation * ) ),
		 url(), SIGNAL( dataTransferProgress( int, int, QNetworkOperation * ) ) );
	connect( this, SIGNAL( connectionStateChanged( int, const QString & ) ),
		 url(), SIGNAL( connectionStateChanged( int, const QString & ) ) );
    }

    if ( !d->opInProgress && !d->operationQueue.isEmpty() )
	d->opStartTimer->start( 1, TRUE );
}

/*!
  For processing operations the network protocol base class calls this
  method quite often. This should be reimplemented by new
  network protocols. It should return TRUE if the connection
  is OK (open), otherwise FALSE. If the connection is not open the protocol
  should open it.

  If the connection can't be opened (e.g., because you already tried it
  but the host couldn't be found), set the state
  of \a op to QNetworkProtocol::StFailed and emit the finished() signal with
  this QNetworkOperation as argument.

  \a op is the operation that needs an open connection.
*/

bool QNetworkProtocol::checkConnection( QNetworkOperation * )
{
    return TRUE;
}

/*!
  Returns an int that is OR'd together using the enum values
  of \c QNetworkProtocol::Operation, which describes which operations
  are supported by the network protocol. Should be reimplemented by new
  network protocols.
*/

int QNetworkProtocol::supportedOperations() const
{
    return 0;
}

/*!
  Adds the operation \a op to the operation queue. The operation
  will be processed as soon as possible. This method returns
  immediately.
*/

void QNetworkProtocol::addOperation( QNetworkOperation *op )
{
#ifdef QNETWORKPROTOCOL_DEBUG
    qDebug( "QNetworkOperation: addOperation: %p %d", op, op->operation() );
#endif
    d->operationQueue.enqueue( op );
    if ( !d->opInProgress )
	d->opStartTimer->start( 1, TRUE );
}

/*!
  Static method to register a network protocol for Qt. For example, if you have
  an implementation of NNTP (called Nntp) which is derived from
  QNetworkProtocol, call

  QNetworkProtocol::registerNetworkProtocol( "nntp", new QNetworkProtocolFactory<Nntp> );

  This implementation is thereafter registered for nntp operations.
*/

void QNetworkProtocol::registerNetworkProtocol( const QString &protocol,
						QNetworkProtocolFactoryBase *protocolFactory )
{
    if ( !qNetworkProtocolRegister ) {
	qNetworkProtocolRegister = new QNetworkProtocolDict;
	QNetworkProtocol::registerNetworkProtocol( "file", new QNetworkProtocolFactory< QLocalFs > );
    }

    qNetworkProtocolRegister->insert( protocol, protocolFactory );
}

/*!
  Static method to get a new instance of a network protocol. For example, if
  you need to do some FTP operations, do the following:

  QFtp *ftp = QNetworkProtocol::getNetworkProtocol( "ftp" );

  This returns null if no protocol for ftp was registered
  or a pointer to a new instance of an FTP implementation. The ownership
  of the pointer is transferred to you, so you have to delete it if you
  don't need it anymore.

  Normally you should not work directly with network protocols, so
  you will not need to call this method yourself. Instead, use the
  QUrlOperator, which makes working with network protocols
  much more convenient.

  \sa QUrlOperator
*/

QNetworkProtocol *QNetworkProtocol::getNetworkProtocol( const QString &protocol )
{
    if ( !qNetworkProtocolRegister ) {
	qNetworkProtocolRegister = new QNetworkProtocolDict;
	QNetworkProtocol::registerNetworkProtocol( "file", new QNetworkProtocolFactory< QLocalFs > );
    }

    if ( protocol.isNull() )
	return 0;

    QNetworkProtocolFactoryBase *factory = qNetworkProtocolRegister->find( protocol );
    if ( factory )
	return factory->createObject();

    return 0;
}

/*!
  Returns TRUE if only a protocol for working on the local filesystem is
  registered, or FALSE if other network protocols are also registered.
*/

bool QNetworkProtocol::hasOnlyLocalFileSystem()
{
    if ( !qNetworkProtocolRegister )
	return FALSE;

    QDictIterator< QNetworkProtocolFactoryBase > it( *qNetworkProtocolRegister );
    for ( ; it.current(); ++it )
	if ( it.currentKey() != "file" )
	    return FALSE;
    return TRUE;
}

/*!
  \internal
  Starts processing network operations.
*/

void QNetworkProtocol::startOps()
{
#ifdef QNETWORKPROTOCOL_DEBUG
    qDebug( "QNetworkOperation: start processing operations" );
#endif
    processNextOperation( 0 );
}

/*!
  \internal
  Processes the operation \a op. It calls the
  corresponding operation[something]( QNetworkOperation * )
  methods.
*/

void QNetworkProtocol::processOperation( QNetworkOperation *op )
{
    if ( !op )
	return;

    switch ( op->operation() ) {
    case OpListChildren:
	operationListChildren( op );
	break;
    case OpMkdir:
	operationMkDir( op );
	break;
    case OpRemove:
	operationRemove( op );
	break;
    case OpRename:
	operationRename( op );
	break;
    case OpGet:
	operationGet( op );
	break;
    case OpPut:
	operationPut( op );
	break;
    }
}

/*!
  When implementing a new network protocol, this method should
  be reimplemented if the protocol supports listing children; this method should then process this QNetworkOperation.

  When you reimplement this method it's very important that
  you emit the correct signals at the correct time (especially the
  finished() signal after processing an operation). Take
  a look at the <a href="network.html">Qt Network Documentation</a>.
  It describes in detail how to reimplement this method.
  You may also look at the example implementation of
  qt/extenstions/network/examples/networkprotocol/nntp.cpp.
*/

void QNetworkProtocol::operationListChildren( QNetworkOperation * )
{
}

/*!
  When implementing a new network protocol, this method should
  be reimplemented if the protocol supports making directories;
  this method should then process this QNetworkOperation.

  When you reimplement this method it's very important that
  you emit the correct signals at the correct time (especially the
  finished() signal after processing an operation). Take
  a look at the <a href="network.html">Qt Network Documentation</a>.
  It describes in detail how to reimplement this method.
  You may also look at the example implementation of
  qt/extenstions/network/examples/networkprotocol/nntp.cpp.
*/

void QNetworkProtocol::operationMkDir( QNetworkOperation * )
{
}

/*!
  When implementing a new network protocol, this method should
  be reimplemented if the protocol supports removing children; this method should then process this QNetworkOperation.

  When you reimplement this method it's very important that
  you emit the correct signals at the correct time (especially the
  finished() signal after processing an operation). Take
  a look at the <a href="network.html">Qt Network Documentation</a>.
  It is describes in detail how to reimplement this method.
  You may also look at the example implementation of
  qt/extenstions/network/examples/networkprotocol/nntp.cpp.
*/

void QNetworkProtocol::operationRemove( QNetworkOperation * )
{
}

/*!
  When implementing a new newtork protocol, this method should
  be reimplemented if the protocol supports renaming children; this method should then process this QNetworkOperation.

  When you reimplement this method it's very important that
  you emit the correct signals at the correct time (especially the
  finished() signal after processing an operation). Take
  a look at the <a href="network.html">Qt Network Documentation</a>.
  It describes in detail how to reimplement this method.
  You may also look at the example implementation of
  qt/extenstions/network/examples/networkprotocol/nntp.cpp.
*/

void QNetworkProtocol::operationRename( QNetworkOperation * )
{
}

/*!
  When implementing a new network protocol, this method should
  be reimplemented if the protocol supports getting data; this method should then
  process the QNetworkOperation.

  When you reimplement this method it's very important that
  you emit the correct signals at the correct time (especially the
  finished() signal after processing an operation). Take
  a look at the <a href="network.html">Qt Network Documentation</a>.
  It describes in detail how to reimplement this method.
  You may also look at the example implementation of
  qt/extenstions/network/examples/networkprotocol/nntp.cpp.
*/

void QNetworkProtocol::operationGet( QNetworkOperation * )
{
}

/*!
  When implementing a new network protocol, this method should
  be reimplemented if the protocol supports putting data; this method should then process the QNetworkOperation.

  When you reimplement this method it's very important that
  you emit the correct signals at the correct time (especially the
  finished() signal after processing an operation). Take
  a look at the <a href="network.html">Qt Network Documentation</a>.
  It describes in detail how to reimplement this method.
  You may also look at the example implementation of
  qt/extenstions/network/examples/networkprotocol/nntp.cpp.
*/

void QNetworkProtocol::operationPut( QNetworkOperation * )
{
}

/*!
  \internal
  Handles operations. Deletes the previous operation object and
  tries to process the next operation. It also checks the connection state
  and only processes the next operation, if the connection of the protocol
  is open. Else it waits until the protocol opens the connection.
*/

void QNetworkProtocol::processNextOperation( QNetworkOperation *old )
{
#ifdef QNETWORKPROTOCOL_DEBUG
    qDebug( "QNetworkOperation: process next operation, old: %p", old );
#endif
    d->removeTimer->stop();

    if ( old )
	d->oldOps.append( old );

    if ( d->operationQueue.isEmpty() ) {
	d->opInProgress = 0;
	if ( d->autoDelete )
	    d->removeTimer->start( d->removeInterval, TRUE );
	return;
    }

    QNetworkOperation *op = d->operationQueue.head();

    d->opInProgress = 0;

    if ( !checkConnection( op ) ) {
	if ( op->state() != QNetworkProtocol::StFailed ) {
	    d->opStartTimer->start( 1, TRUE );
	    d->opInProgress = op;
	} else {
	    d->opInProgress = op;
	    d->operationQueue.dequeue();
	    clearOperationQueue();
	    emit finished( op );
	}

	return;
    }

    d->opInProgress = op;
    d->operationQueue.dequeue();
    processOperation( op );
}

/*!
  Returns the QUrlOperator on which the protocol works.
*/

QUrlOperator *QNetworkProtocol::url() const
{
    return d->url;
}

/*!
  Returns the operation, which is just processed, or NULL
  of none is processed at the moment.
*/

QNetworkOperation *QNetworkProtocol::operationInProgress() const
{
    return d->opInProgress;
}

/*!
  Clears the operation queue.
*/

void QNetworkProtocol::clearOperationQueue()
{
    d->operationQueue.dequeue();
    d->operationQueue.setAutoDelete( TRUE );
    d->operationQueue.clear();
}

/*!
  Stops the current operation that was just processed and clears
  all waiting operations.
*/

void QNetworkProtocol::stop()
{
    QNetworkOperation *op = d->opInProgress;
    clearOperationQueue();
    if ( op ) {
	op->setState( StStopped );
	op->setProtocolDetail( tr( "Operation stopped by the user" ) );
	emit finished( op );
	setUrl( 0 );
	op->free();
    }
}

/*!  Because it's sometimes hard to take care of removing network protocol
  instances, QNetworkProtocol provides an auto-delete mechanism. If you
  set \a b to TRUE, the network protocol instance is removed after it has
  been \a i milliseconds inactive (\a i ms after the last operation has
  been processed).  If you set \a b to FALSE the auto-delete mechanism is
  switched off.

  Note that if you switch on auto-delete, the QNetworkProtocol also
  deletes its QUrlOperator!
*/

void QNetworkProtocol::setAutoDelete( bool b, int i )
{
    d->autoDelete = b;
    d->removeInterval = i;
}

/*!
  Returns TRUE if auto-deleting is enabled, otherwise FALSE.

  \sa QNetworkProtocol::setAutoDelete()
*/

bool QNetworkProtocol::autoDelete() const
{
    return d->autoDelete;
}

/*!
  \internal
*/

void QNetworkProtocol::removeMe()
{
    if ( d->autoDelete ) {
#ifdef QNETWORKPROTOCOL_DEBUG
	qDebug( "QNetworkOperation:  autodelete of QNetworkProtocol %p", this );
#endif
	delete d->url; // destructor deletes the network protocol
    }
}

void QNetworkProtocol::emitNewChildren( const QUrlInfo &i, QNetworkOperation *op )
{
    QValueList<QUrlInfo> lst;
    lst << i;
    emit newChildren( lst, op );
}

class QNetworkOperation::Private
{
public:
    QNetworkProtocol::Operation operation;
    QNetworkProtocol::State state;
    QMap<int, QString> args;
    QMap<int, QByteArray> rawArgs;
    QString protocolDetail;
    int errorCode;
    QTimer *deleteTimer;
};

/*!
  \class QNetworkOperation qnetworkprotocol.h

  \brief The QNetworkOperation class provides common operations for network protocols.

  \ingroup io

  An object is created to describe the operation and the current
  state for each operation that a network protocol should process.

  For a detailed description of the Qt Network Architecture and
  how to implement and use network protocols in Qt, see
  the <a href="network.html">Qt Network Documentation</a>.

  \sa QNetworkProtocol
*/

/*!
  Constructs a network operation object. \a operation is the type
  of the operation, and \a arg0, \a arg1 and  \a arg2 are the
  first three arguments of the operation.
  The state is initialized to QNetworkProtocol::StWaiting.
*/

QNetworkOperation::QNetworkOperation( QNetworkProtocol::Operation operation,
				      const QString &arg0, const QString &arg1,
				      const QString &arg2 )
{
    d = new Private;
    d->deleteTimer = new QTimer( this );
    connect( d->deleteTimer, SIGNAL( timeout() ),
	     this, SLOT( deleteMe() ) );
    d->operation = operation;
    d->state = QNetworkProtocol::StWaiting;
    d->args[ 0 ] = arg0;
    d->args[ 1 ] = arg1;
    d->args[ 2 ] = arg2;
    d->rawArgs[ 0 ] = 0;
    d->rawArgs[ 1 ] = 0;
    d->rawArgs[ 2 ] = 0;
    d->protocolDetail = QString::null;
    d->errorCode = (int)QNetworkProtocol::NoError;
}

/*!
  Constructs a network operation object. \a operation is the type
  of the operation, and \a arg0, \a arg1 and  \a arg2 are the first three
  raw data arguments of the operation.
  The state is initialized to QNetworkProtocol::StWaiting.
*/

QNetworkOperation::QNetworkOperation( QNetworkProtocol::Operation operation,
				      const QByteArray &arg0, const QByteArray &arg1,
				      const QByteArray &arg2 )
{
    d = new Private;
    d->deleteTimer = new QTimer( this );
    connect( d->deleteTimer, SIGNAL( timeout() ),
	     this, SLOT( deleteMe() ) );
    d->operation = operation;
    d->state = QNetworkProtocol::StWaiting;
    d->args[ 0 ] = QString::null;
    d->args[ 1 ] = QString::null;
    d->args[ 2 ] = QString::null;
    d->rawArgs[ 0 ] = arg0;
    d->rawArgs[ 1 ] = arg1;
    d->rawArgs[ 2 ] = arg2;
    d->protocolDetail = QString::null;
    d->errorCode = (int)QNetworkProtocol::NoError;
}

/*!
  Destructor.
*/

QNetworkOperation::~QNetworkOperation()
{
    delete d;
    d = 0;
}

/*!
  Sets the \a state of the operation object. This should be done
  by the network protocol during processing; at the end
  it should be set to QNetworkProtocol::StDone or QNetworkProtocol::StFailed,
  depending on success or failure.
*/

void QNetworkOperation::setState( QNetworkProtocol::State state )
{
    if ( d->deleteTimer->isActive() ) {
	d->deleteTimer->stop();
	d->deleteTimer->start( NETWORK_OP_DELAY );
    }
    d->state = state;
}

/*!
  If the operation failed, the error message can be specified as \a detail.
*/

void QNetworkOperation::setProtocolDetail( const QString &detail )
{
    if ( d->deleteTimer->isActive() ) {
	d->deleteTimer->stop();
	d->deleteTimer->start( NETWORK_OP_DELAY );
    }
    d->protocolDetail = detail;
}

/*!
  If the operation failed, the protocol should set an error code
  to describe the error in more detail. If possible, one of the
  error codes defined in QNetworkProtocol should be used.

  \sa setProtocolDetail()
*/

void QNetworkOperation::setErrorCode( int ec )
{
    if ( d->deleteTimer->isActive() ) {
	d->deleteTimer->stop();
	d->deleteTimer->start( NETWORK_OP_DELAY );
    }
    d->errorCode = ec;
}

/*!
  Sets the argument \a num of the network operation to \a arg.
*/

void QNetworkOperation::setArg( int num, const QString &arg )
{
    if ( d->deleteTimer->isActive() ) {
	d->deleteTimer->stop();
	d->deleteTimer->start( NETWORK_OP_DELAY );
    }
    d->args[ num ] = arg;
}

/*!
  Sets the raw data argument \a num of the network operation to \a arg.
*/

void QNetworkOperation::setRawArg( int num, const QByteArray &arg )
{
    if ( d->deleteTimer->isActive() ) {
	d->deleteTimer->stop();
	d->deleteTimer->start( NETWORK_OP_DELAY );
    }
    d->rawArgs[ num ] = arg;
}

/*!
  Returns the type of the operation.
*/

QNetworkProtocol::Operation QNetworkOperation::operation() const
{
    if ( d->deleteTimer->isActive() ) {
	d->deleteTimer->stop();
	d->deleteTimer->start( NETWORK_OP_DELAY );
    }
    return d->operation;
}

/*!
  Returns the state of the operation. You can determine whether an operation is still waiting to be processed,
  is in process, has been done successfully, or it failed.
*/

QNetworkProtocol::State QNetworkOperation::state() const
{
    if ( d->deleteTimer->isActive() ) {
	d->deleteTimer->stop();
	d->deleteTimer->start( NETWORK_OP_DELAY );
    }
    return d->state;
}

/*!
  Returns the argument \a num of the operation. If this argument was
  not set already, an empty string is returned.
*/

QString QNetworkOperation::arg( int num ) const
{
    if ( d->deleteTimer->isActive() ) {
	d->deleteTimer->stop();
	d->deleteTimer->start( NETWORK_OP_DELAY );
    }
    return d->args[ num ];
}

/*!
  Returns the raw data argument \a num of the operation. If this argument was
  not set already, an empty bytearray is returned.
*/

QByteArray QNetworkOperation::rawArg( int num ) const
{
    if ( d->deleteTimer->isActive() ) {
	d->deleteTimer->stop();
	d->deleteTimer->start( NETWORK_OP_DELAY );
    }
    return d->rawArgs[ num ];
}

/*!  Returns a detailed error message for the last error. This must
  have been set using setProtocolDetail().
*/

QString QNetworkOperation::protocolDetail() const
{
    if ( d->deleteTimer->isActive() ) {
	d->deleteTimer->stop();
	d->deleteTimer->start( NETWORK_OP_DELAY );
    }
    return d->protocolDetail;
}

/*! Returns the error code for the last error that occurred.
*/

int QNetworkOperation::errorCode() const
{
    if ( d->deleteTimer->isActive() ) {
	d->deleteTimer->stop();
	d->deleteTimer->start( NETWORK_OP_DELAY );
    }
    return d->errorCode;
}

/*!
  \internal
*/

QByteArray& QNetworkOperation::raw( int num ) const
{
    if ( d->deleteTimer->isActive() ) {
	d->deleteTimer->stop();
	d->deleteTimer->start( NETWORK_OP_DELAY );
    }
    return d->rawArgs[ num ];
}

/*!
  Sets this object to delete itself when it hasn't been used for one second.

  Because QNetworkOperation pointers are passed around a lot the
  QNetworkProtocol generally does not have enough knowledge to delete
  these at the correct time. If a QNetworkProtocol doesn't need an
  operation anymore it will call this function instead.

  You should never need to call the method yourself!
*/

void QNetworkOperation::free()
{
    d->deleteTimer->start( NETWORK_OP_DELAY );
}

/*!
  \internal
  Internal slot for auto-deletion.
*/

void QNetworkOperation::deleteMe()
{
    delete this;
}

#endif
