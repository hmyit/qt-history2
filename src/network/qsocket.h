/****************************************************************************
** $Id: //depot/qt/main/src/network/qsocket.h#11 $
**
** Definition of QSocket class.
**
** Created : 970521
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the network module of the Qt GUI Toolkit.
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
** Licensees holding valid Qt Enterprise Edition licenses may use this
** file in accordance with the Qt Commercial License Agreement provided
** with the Software.
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

#ifndef QSOCKET_H
#define QSOCKET_H

#ifndef QT_H
#include "qobject.h"
#include "qiodevice.h"
#include "qhostaddress.h" // int->QHostAddress conversion
#endif // QT_H

#ifndef QT_NO_NETWORK
class QSocketPrivate;
class QSocketDevice;


class Q_EXPORT QSocket : public QObject, public QIODevice
{
    Q_OBJECT
public:
    enum Error {
	ErrConnectionRefused,
	ErrHostNotFound,
	ErrSocketRead
    };

    QSocket( QObject *parent=0, const char *name=0 );
    virtual ~QSocket();

    enum State { Idle, HostLookup, Connecting,
		 Listening, Connected, Closing,
		 Connection=Connected };
    State	 state() const;

    int		 socket() const;
    virtual void setSocket( int );

    QSocketDevice *socketDevice();
    virtual void setSocketDevice( QSocketDevice * );

#ifndef QT_NO_DNS
    virtual void connectToHost( const QString &host, Q_UINT16 port );
#endif
    QString	 peerName() const;

    // Implementation of QIODevice abstract virtual functions
    bool	 open( int mode );
    void	 close();
    void	 flush();
    Q_ULONG	 size() const;
    Q_ULONG	 at() const;
    bool	 at( Q_ULONG );
    bool	 atEnd() const;

    int		 bytesAvailable() const;
    int		 waitForMore( int msecs ) const;
    int		 bytesToWrite() const;

    Q_LONG	 readBlock( char *data, Q_ULONG maxlen );
    Q_LONG	 writeBlock( const char *data, Q_ULONG len );
    Q_LONG	 readLine( char *data, Q_ULONG maxlen );

    int		 getch();
    int		 putch( int );
    int		 ungetch(int);

    bool	 canReadLine() const;
    virtual QString	 readLine();

    Q_UINT16	 port() const;
    Q_UINT16	 peerPort() const;
    QHostAddress address() const;
    QHostAddress peerAddress() const;

signals:
    void	 hostFound();
    void	 connected();
    void	 connectionClosed();
    void	 delayedCloseFinished();
    void	 readyRead();
    void	 bytesWritten( int nbytes );
    void	 error( int );

protected slots:
    virtual void sn_read();
    virtual void sn_write();

private slots:
    void	tryConnecting();

private:
    QSocketPrivate *d;

    bool	 consumeReadBuf( Q_ULONG nbytes, char * );
    bool	 consumeWriteBuf( Q_ULONG nbytes );
    bool	 scanNewline( QByteArray * = 0 );
    void	 tryConnection();
    void         setSocketIntern( int socket );

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QSocket( const QSocket & );
    QSocket &operator=( const QSocket & );
#endif
};

#endif //QT_NO_NETWORK
#endif // QSOCKET_H
