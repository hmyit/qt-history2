/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QTRANSPORTAUTH_QWS_H
#define QTRANSPORTAUTH_QWS_H

#include <QtCore/qobject.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>
#include <QtCore/qbuffer.h>
#include <QtCore/qpointer.h>

#ifndef QT_NO_QWS_MULTIPROCESS

QT_MODULE(Gui)

#define KEY_TEMPLATE "XOXOXOauthOXOXOX99"
#define APP_KEY const char *_key = KEY_TEMPLATE;
#define QL_APP_KEY const char *_ql_key = KEY_TEMPLATE;



class QAuthDevice;
class QWSClient;
class QIODevice;
class QTransportAuthPrivate;

class Q_GUI_EXPORT QTransportAuth : public QObject
{
    Q_OBJECT
public:
    static QTransportAuth *getInstance();

    enum Result {
        // Error codes
        Pending = 0x00,
        TooSmall = 0x01,
        CacheMiss = 0x02,
        NoMagic = 0x03,
        NoSuchKey = 0x04,
        FailMatch = 0x05,
        // reserved for expansion
        Success = 0x1e,
        ErrMask = 0x1f,

        // Verification codes
        Allow = 0x20,
        Deny = 0x40,
        Ask = 0x60,
        // reserved
        StatusMask = 0xe0
    };

    enum Properties {
        Trusted = 0x01,
        Connection = 0x02,
        UnixStreamSock = 0x04,
        SharedMemory = 0x08,
        MessageQueue = 0x10,
        UDP = 0x20,
        TCP = 0x40,
        UserDefined = 0x80,
        TransportType = 0xfc
    };

    struct Data
    {
        Data() {}
        Data( unsigned char p, int d )
            : properties( p )
            , descriptor( d )
        {
            if (( properties & TransportType ) == TCP ||
                ( properties & TransportType ) == UnixStreamSock )
                properties |= Connection;
        }

        unsigned char properties;
        unsigned char progId;
        unsigned char status;
        unsigned int descriptor;   // socket fd or shmget key

        bool trusted() const;
        void setTrusted( bool );
        bool connection() const;
        void setConnection( bool );
    };

    static const char *errorString( const Data & );

    Data *connectTransport( unsigned char, int );

    QAuthDevice *authBuf( Data *, QIODevice * );
    QAuthDevice *recvBuf( Data *, QIODevice * );
    QIODevice *passThroughByClient( QWSClient * ) const;

    void setKeyFilePath( const QString & );
    QString keyFilePath() const;
    void setLogFilePath( const QString & );
    QString logFilePath() const;
    bool isDiscoveryMode() const;
    void setProcessKey( const char * );
    void registerPolicyReceiver( QObject * );

private:
    // users should never construct their own
    QTransportAuth();
    ~QTransportAuth();

    friend class QAuthDevice;
    Q_DECLARE_PRIVATE(QTransportAuth)
};

#endif // QT_NO_QWS_MULTIPROCESS
#endif // QTRANSPORTAUTH_QWS_H
