/****************************************************************************
**
** Implementation of QSocketDevice class.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the network module of the Qt GUI Toolkit.
** EDITIONS: ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qkernelapplication.h"
#include "qsocketdevice.h"
#include "qdatetime.h"

#include <string.h>

#if !defined (QT_NO_IPV6)
#include <winsock2.h>
#include <ws2tcpip.h>
#if !defined(SOCKADDR_STORAGE) && !defined(_SS_ALIGNSIZE)
// Compiling with old SDK, which does not contain these
// structs and defines
# define _SS_MAXSIZE 128
# define _SS_ALIGNSIZE (sizeof(__int64))
# define _SS_PAD1SIZE (_SS_ALIGNSIZE - sizeof (short))
# define _SS_PAD2SIZE (_SS_MAXSIZE - (sizeof (short) + _SS_PAD1SIZE \
                                                     + _SS_ALIGNSIZE))
  typedef struct {
      short ss_family;
      char __ss_pad1[_SS_PAD1SIZE];
      __int64 __ss_align;
      char __ss_pad2[_SS_PAD2SIZE];
  } SOCKADDR_STORAGE;
#endif

// sockaddr_in6 size changed between old and new SDK
// Only the new version is the correct one, so always
// use this structure.
struct alt_in6_addr {
    u_char alt_s6_addr[16];
};
struct alt_sockaddr_in6 {
    short   sin6_family;            /* AF_INET6 */
    u_short sin6_port;              /* Transport level port number */
    u_long  sin6_flowinfo;          /* IPv6 flow information */
    struct alt_in6_addr sin6_addr;  /* IPv6 address */
    u_long sin6_scope_id;           /* set of interfaces for a scope */
};

#else
#include <windows.h>
#include <winsock.h>
#endif

#ifndef NO_ERRNO_H
#include <errno.h>
#endif


#if defined(SOCKLEN_T)
#undef SOCKLEN_T
#endif

#define SOCKLEN_T int // #### Winsock 1.1

static int initialized = 0x00; // Holds the Winsock version

static void cleanupWinSock() // post-routine
{
    WSACleanup();
    initialized = FALSE;
}


void QSocketDevice::init()
{
    if ( !initialized ) {
	WSAData wsadata;
#if !defined(QT_NO_IPV6)
	// IPv6 requires Winsock v2.0 or better.
	if ( WSAStartup( MAKEWORD(2,0), &wsadata ) != 0 ) {

#if defined(QSOCKETDEVICE_DEBUG)
	    qDebug( "QSocketDevice: WinSock v2.0 initialization failed, disabling IPv6 support." );
#endif
	    return;
	} else {
	    initialized = 0x20;
	}

	if (!initialized) {
#endif
	    if ( WSAStartup( MAKEWORD(1,1), &wsadata ) != 0 ) {
#if defined(QT_CHECK_NULL)
	        qWarning( "QSocketDevice: WinSock initialization failed" );
#endif
#if defined(QSOCKETDEVICE_DEBUG)
	        qDebug( "QSocketDevice: WinSock initialization failed"  );
#endif
	        return;
	    }

            initialized = 0x11;
	}

	qAddPostRoutine( cleanupWinSock );
#if defined(QSOCKETDEVICE_DEBUG)
	qDebug( "QSocketDevice: WinSock initialization OK" );
#endif
    }
}

void QSocketDevice::initFd( Family *family )
{
    if (fd != -1) {
#if !defined (QT_NO_IPV6)
	SOCKADDR_STORAGE ss;
	SOCKLEN_T sslen = sizeof( ss );
	if ( getsockname(fd, (struct sockaddr *)&ss, &sslen) == 0 ) {
	    switch ( ss.ss_family ) {
		case AF_INET:
		    *family = Ipv4;
		    break;
		case AF_INET6:
		    *family = Ipv6;
		    break;
		default:
		    qWarning("Unable to initialize invalid socket");
		    break;
	    }
	} else {
	    qWarning("Unable to initialize invalid socket");
	}
#else
	*family = Ipv4;
#endif
    }

}

int QSocketDevice::createNewSocket( )
{
#if !defined(QT_NO_IPV6)
    int s;
    // Support IPv6 for Winsock v2.0++
    if ( initialized >= 0x20 ) {
	s = ::socket( family() == Ipv6 ? AF_INET6 : AF_INET,
		      t == Datagram ? SOCK_DGRAM : SOCK_STREAM, 0 );
    } else {
	s = ::socket( AF_INET, t==Datagram?SOCK_DGRAM:SOCK_STREAM, 0 );
    }
#else
    int s = ::socket( AF_INET, t==Datagram?SOCK_DGRAM:SOCK_STREAM, 0 );
#endif
    if ( s == INVALID_SOCKET ) {
	switch( WSAGetLastError() ) {
	    case WSANOTINITIALISED:
		e = Impossible;
		break;
	    case WSAENETDOWN:
		// ### what to use here?
		e = NetworkFailure;
		//e = Inaccessible;
		break;
	    case WSAEMFILE:
		e = NoFiles; // special case for this
		break;
	    case WSAEINPROGRESS:
	    case WSAENOBUFS:
		e = NoResources;
		break;
	    case WSAEAFNOSUPPORT:
	    case WSAEPROTOTYPE:
	    case WSAEPROTONOSUPPORT:
	    case WSAESOCKTNOSUPPORT:
		e = InternalError;
		break;
	    default:
		e = UnknownError;
		break;
	}
    } else {
	return s;
    }
    return -1;
}


void QSocketDevice::close()
{
    if ( fd == -1 || !isOpen() )		// already closed
	return;
    setFlags( IO_Sequential );
    resetStatus();
    setState( 0 );
    ::closesocket( fd );
#if defined(QSOCKETDEVICE_DEBUG)
    qDebug( "QSocketDevice::close: Closed socket %x", fd );
#endif
    fd = -1;
    fetchConnectionParameters();
}


bool QSocketDevice::blocking() const
{
    if ( !isValid() )
	return TRUE;
    return TRUE;
}


void QSocketDevice::setBlocking( bool enable )
{
#if defined(QSOCKETDEVICE_DEBUG)
    qDebug( "QSocketDevice::setBlocking( %d )", enable );
#endif
    if ( !isValid() )
	return;

    unsigned long dummy = enable ? 0 : 1;
    ioctlsocket( socket(), FIONBIO, &dummy );
}


int QSocketDevice::option( Option opt ) const
{
    if ( !isValid() )
	return -1;
    int n = -1;
    int v = -1;
    switch ( opt ) {
	case Broadcast:
	    n = SO_BROADCAST;
	    break;
	case ReceiveBuffer:
	    n = SO_RCVBUF;
	    break;
	case ReuseAddress:
	    n = SO_REUSEADDR;
	    break;
	case SendBuffer:
	    n = SO_SNDBUF;
	    break;
    }
    if ( n != -1 ) {
	SOCKLEN_T len = sizeof(v);
	int r = ::getsockopt( socket(), SOL_SOCKET, n, (char*)&v, &len );
	if ( r != SOCKET_ERROR )
	    return v;
	if ( !e ) {
            QSocketDevice *that = (QSocketDevice*)this; // mutable function
	    switch( WSAGetLastError() ) {
		case WSANOTINITIALISED:
		    that->e = Impossible;
		    break;
		case WSAENETDOWN:
		    that->e = NetworkFailure;
		    break;
		case WSAEFAULT:
		case WSAEINVAL:
		case WSAENOPROTOOPT:
		    that->e = InternalError;
		    break;
		case WSAEINPROGRESS:
		    that->e = NoResources;
		    break;
		case WSAENOTSOCK:
		    that->e = Impossible;
		    break;
		default:
		    that->e = UnknownError;
		    break;
	    }
	}
	return -1;
    }
    return v;
}


void QSocketDevice::setOption( Option opt, int v )
{
    if ( !isValid() )
	return;
    int n = -1; // for really, really bad compilers
    switch ( opt ) {
	case Broadcast:
	    n = SO_BROADCAST;
	    break;
	case ReceiveBuffer:
	    n = SO_RCVBUF;
	    break;
	case ReuseAddress:
	    n = SO_REUSEADDR;
	    break;
	case SendBuffer:
	    n = SO_SNDBUF;
	    break;
	default:
	    return;
    }
    int r = ::setsockopt( socket(), SOL_SOCKET, n, (char*)&v, sizeof(v) );
    if ( r == SOCKET_ERROR && e == NoError ) {
	switch( WSAGetLastError() ) {
	    case WSANOTINITIALISED:
		e = Impossible;
		break;
	    case WSAENETDOWN:
		e = NetworkFailure;
		break;
	    case WSAEFAULT:
	    case WSAEINVAL:
	    case WSAENOPROTOOPT:
		e = InternalError;
		break;
	    case WSAEINPROGRESS:
		e = NoResources;
		break;
	    case WSAENETRESET:
	    case WSAENOTCONN:
		e =  Impossible; // ### ?
		break;
	    case WSAENOTSOCK:
		e = Impossible;
		break;
	    default:
		e = UnknownError;
		break;
	}
    }
}


bool QSocketDevice::connect( const QHostAddress &addr, Q_UINT16 port )
{
    if ( addr.isIp6Addr() )
	setFamily( Ipv6 );
    else if ( addr.isIp4Addr() )
	setFamily( Ipv4 );

    struct sockaddr_in a4;
    struct sockaddr *aa;
    SOCKLEN_T aalen;

#if !defined(QT_NO_IPV6)
    struct alt_sockaddr_in6 a6;

    if ( initialized >= 0x20 && addr.isIp6Addr() ) {
        memset(&a6, 0, sizeof(a6));
	a6.sin6_family = AF_INET6;
	a6.sin6_port = htons( port );
	Q_IPV6ADDR tmp = addr.ip6Addr();
	memcpy( &a6.sin6_addr.alt_s6_addr, &tmp, sizeof(tmp) );

    	setFamily( Ipv6 );
	aalen = sizeof( a6 );
	aa = (struct sockaddr *)&a6;
    } else
#endif
    {
	if ( !addr.isIp4Addr() ) {
	    qWarning( "QSocketDevice: IPv6 is not supported by this version" );
	    e = Impossible;
	    return FALSE;
	}

	memset(&a4, 0, sizeof(a4));
	a4.sin_family = AF_INET;
	a4.sin_port = htons(port);
	a4.sin_addr.s_addr = htonl(addr.ip4Addr());

	setFamily(Ipv4);
	aalen = sizeof(a4);
	aa = (struct sockaddr *)&a4;
    }

    int r = ::connect( socket(), aa, aalen );

    if ( r == SOCKET_ERROR )
    {
	switch( WSAGetLastError() ) {
	    case WSANOTINITIALISED:
		e = Impossible;
		break;
	    case WSAENETDOWN:
		e = NetworkFailure;
		break;
	    case WSAEADDRINUSE:
	    case WSAEINPROGRESS:
	    case WSAENOBUFS:
		e = NoResources;
		break;
	    case WSAEINTR:
		e = UnknownError; // ### ?
		break;
	    case WSAEALREADY:
		// ### ?
		break;
	    case WSAEADDRNOTAVAIL:
		e = ConnectionRefused; // ### ?
		break;
	    case WSAEAFNOSUPPORT:
	    case WSAEFAULT:
		e = InternalError;
		break;
	    case WSAEINVAL:
		break;
	    case WSAECONNREFUSED:
		e = ConnectionRefused;
		break;
	    case WSAEISCONN:
		goto successful;
	    case WSAENETUNREACH:
	    case WSAETIMEDOUT:
		e = NetworkFailure;
		break;
	    case WSAENOTSOCK:
		e = Impossible;
		break;
	    case WSAEWOULDBLOCK:
		break;
	    case WSAEACCES:
		e = Inaccessible;
		break;
	    case 10107:
		// Workaround for a problem with the WinSock Proxy Server. See
		// also support/arc-12/25557 for details on the problem.
		goto successful;
	    default:
		e = UnknownError;
		break;
	}
	return FALSE;
    }
successful:
    fetchConnectionParameters();
    return TRUE;
}


bool QSocketDevice::bind( const QHostAddress &address, Q_UINT16 port )
{
    if ( address.isIp6Addr() )
	setFamily( Ipv6 );
    else if ( address.isIp4Addr() )
	setFamily( Ipv4 );

    int r;
    struct sockaddr_in a4;
#if !defined(QT_NO_IPV6)
    struct alt_sockaddr_in6 a6;

    if ( initialized >= 0x20 && address.isIp6Addr() ) {
	memset( &a6, 0, sizeof(a6) );
	a6.sin6_family = AF_INET6;
	a6.sin6_port = htons( port );

	Q_IPV6ADDR tmp = address.ip6Addr();

	memcpy( &a6.sin6_addr.alt_s6_addr, &tmp, sizeof(tmp) );
	setFamily( Ipv6 );

	r = ::bind( socket(), (struct sockaddr *)&a6, sizeof(SOCKADDR_STORAGE) );
    } else
#endif
    {
	if ( !address.isIp4Addr() ) {
	    e = Impossible;
	    return FALSE;
	}
	memset( &a4, 0, sizeof(a4) );
	a4.sin_family = AF_INET;
	a4.sin_port = htons( port );
	a4.sin_addr.s_addr = htonl( address.ip4Addr() );

	setFamily( Ipv4 );

	r = ::bind( socket(), (struct sockaddr*)&a4, sizeof(struct sockaddr_in) );
    }

    if ( r == SOCKET_ERROR ) {
	switch( WSAGetLastError() ) {
	    case WSANOTINITIALISED:
		e = Impossible;
		break;
	    case WSAENETDOWN:
		e = NetworkFailure;
		break;
	    case WSAEACCES:
		e = Inaccessible;
		break;
	    case WSAEADDRNOTAVAIL:
		e = Inaccessible;
		break;
	    case WSAEFAULT:
		e = InternalError;
		break;
	    case WSAEINPROGRESS:
	    case WSAENOBUFS:
		e = NoResources;
		break;
	    case WSAEADDRINUSE:
	    case WSAEINVAL:
		e = AlreadyBound;
		break;
	    case WSAENOTSOCK:
		e = Impossible;
		break;
	    default:
		e = UnknownError;
		break;
	}
	return FALSE;
    }
    fetchConnectionParameters();
    return TRUE;
}


bool QSocketDevice::listen( int backlog )
{
    if ( !isValid() )
	return FALSE;
    if ( ::listen( socket(), backlog ) >= 0 )
	return TRUE;
    if ( !e )
	e = Impossible;
    return FALSE;
}


int QSocketDevice::accept()
{
    if ( !isValid() )
	return -1;
#if !defined(QT_NO_IPV6)
    SOCKADDR_STORAGE a;
#else
    struct sockaddr a;
#endif
    SOCKLEN_T l = sizeof(struct sockaddr);
    bool done;
    int s;
    do {
        s = ::accept( fd, (struct sockaddr*)&a, &l );
        // we'll blithely throw away the stuff accept() wrote to a
        done = TRUE;
        if ( s == INVALID_SOCKET && e == NoError ) {
	    switch( WSAGetLastError() ) {
                case WSAEINTR:
                    done = FALSE;
                    break;
	        case WSANOTINITIALISED:
		    e = Impossible;
		    break;
	        case WSAENETDOWN:
	        case WSAEOPNOTSUPP:
		    // in all these cases, an error happened during connection
		    // setup.  we're not interested in what happened, so we
		    // just treat it like the client-closed-quickly case.
		    break;
	        case WSAEFAULT:
		    e = InternalError;
		    break;
	        case WSAEINTR:
		    // ### ?
		    break;
	        case WSAEMFILE:
	        case WSAEINPROGRESS:
	        case WSAENOBUFS:
		    e = NoResources;
		    break;
	        case WSAEINVAL:
	        case WSAENOTSOCK:
		    e = Impossible;
		    break;
	        case WSAEWOULDBLOCK:
		    break;
	        default:
		    e = UnknownError;
		    break;
            }
	}
    } while (!done);
    return s;
}


Q_LONG QSocketDevice::bytesAvailable() const
{
    if ( !isValid() )
	return -1;
    u_long nbytes = 0;
    if ( ::ioctlsocket( socket(), FIONREAD, &nbytes) < 0 )
	return -1;
    return nbytes;
}


Q_LONG QSocketDevice::waitForMore( int msecs, bool *timeout ) const
{
    if ( !isValid() )
	return -1;

    fd_set fds;
    struct timeval tv;

    FD_ZERO( &fds );
    FD_SET( socket(), &fds );

    tv.tv_sec = msecs / 1000;
    tv.tv_usec = (msecs % 1000) * 1000;

    int rv = select( fd + 1, &fds, 0, 0, msecs < 0 ? 0 : &tv );

    if ( rv < 0 )
	return -1;

    if ( timeout ) {
	if ( rv == 0 )
	    *timeout = TRUE;
	else
	    *timeout = FALSE;
    }

    return bytesAvailable();
}


Q_LONG QSocketDevice::readBlock( char *data, Q_ULONG maxlen )
{
    if ( data == 0 && maxlen != 0 ) {
	qWarning( "QSocketDevice::readBlock: Null pointer error" );
    }
    if ( !isValid() ) {
	qWarning( "QSocketDevice::readBlock: Invalid socket" );
	return -1;
    }
    if ( !isOpen() ) {
	qWarning( "QSocketDevice::readBlock: Device is not open" );
	return -1;
    }
    if ( !isReadable() ) {
	qWarning( "QSocketDevice::readBlock: Read operation not permitted" );
	return -1;
    }
    Q_LONG r = 0;
    if ( t == Datagram ) {
#if !defined(QT_NO_IPV6)
	// With IPv6 support, we must be prepared to receive both IPv4
	// and IPv6 packets. The generic SOCKADDR_STORAGE (struct
	// sockaddr_storage on unix) replaces struct sockaddr.
	SOCKADDR_STORAGE a;
#else
	struct sockaddr_in a;
#endif
	memset( &a, 0, sizeof(a) );
	SOCKLEN_T sz;
	sz = sizeof( a );
	r = ::recvfrom( socket(), data, maxlen, 0, (struct sockaddr *)&a, &sz );
#if !defined(QT_NO_IPV6)
	// Check the family type, and translate the remote address
	// accordingly.
	struct sockaddr *ap = (struct sockaddr *)&a;
	if (ap->sa_family == AF_INET6) {
	    struct alt_sockaddr_in6 *a6 = (struct alt_sockaddr_in6 *)&a;
	    Q_IPV6ADDR tmp;
	    memcpy( &tmp, &a6->sin6_addr.alt_s6_addr, sizeof(tmp) );

	    pp = ntohs(a6->sin6_port);
	    pa = QHostAddress(tmp);
	} else
#endif
	{
	    struct sockaddr_in *a4 = (struct sockaddr_in *)&a;
	    pp = ntohs( a4->sin_port );
	    pa = QHostAddress( ntohl( a4->sin_addr.s_addr ) );
	}
    } else {
	r = ::recv( socket(), data, maxlen, 0 );
    }
    if ( r == SOCKET_ERROR && e == NoError ) {
	switch( WSAGetLastError() ) {
	    case WSANOTINITIALISED:
		e = Impossible;
		break;
	    case WSAECONNABORTED:
	    case WSAETIMEDOUT:
	    case WSAECONNRESET:
		// connection closed
		close();
		r = 0;
		break;
	    case WSAENETDOWN:
	    case WSAENETRESET:
		e = NetworkFailure;
		break;
	    case WSAEFAULT:
	    case WSAENOTCONN:
	    case WSAESHUTDOWN:
	    case WSAEINVAL:
		e = Impossible;
		break;
	    case WSAEINTR:
		// ### ?
		break;
	    case WSAEINPROGRESS:
		e = NoResources;
		break;
	    case WSAENOTSOCK:
		e = Impossible;
		break;
	    case WSAEOPNOTSUPP:
		e = InternalError; // ### ?
		break;
	    case WSAEWOULDBLOCK:
		break;
	    case WSAEMSGSIZE:
		e = NoResources; // ### ?
		break;
	    case WSAEISCONN:
		// ### ?
		break;
	    default:
		e = UnknownError;
		break;
	}
    }
    return r;
}


Q_LONG QSocketDevice::writeBlock( const char *data, Q_ULONG len )
{
    if ( data == 0 && len != 0 ) {
	qWarning( "QSocketDevice::writeBlock: Null pointer error" );
	return -1;
    }
    if ( !isValid() ) {
	qWarning( "QSocketDevice::writeBlock: Invalid socket" );
	return -1;
    }
    if ( !isOpen() ) {
	qWarning( "QSocketDevice::writeBlock: Device is not open" );
	return -1;
    }
    if ( !isWritable() ) {
	qWarning( "QSocketDevice::writeBlock: Write operation not permitted" );
	return -1;
    }
    bool done = FALSE;
    Q_LONG r = 0;
    while ( !done ) {
	// Don't write more than 64K (see Knowledge Base Q201213).
	r = ::send( socket(), data, ( len>64*1024 ? 64*1024 : len ), 0 );
	done = TRUE;
	if ( r == SOCKET_ERROR && e == NoError ) {//&& errno != WSAEAGAIN ) {
	    switch( WSAGetLastError() ) {
		case WSANOTINITIALISED:
		    e = Impossible;
		    break;
		case WSAENETDOWN:
		case WSAEACCES:
		case WSAENETRESET:
		case WSAESHUTDOWN:
		case WSAEHOSTUNREACH:
		    e = NetworkFailure;
		    break;
		case WSAECONNABORTED:
		case WSAECONNRESET:
		    // connection closed
		    close();
		    break;
		case WSAEINTR:
		    done = FALSE;
		    break;
		case WSAEINPROGRESS:
		    e = NoResources;
		    // ### perhaps try it later?
		    break;
		case WSAEFAULT:
		case WSAEOPNOTSUPP:
		    e = InternalError;
		    break;
		case WSAENOBUFS:
		    // ### try later?
		    break;
		case WSAEMSGSIZE:
		    e = NoResources;
		    break;
		case WSAENOTCONN:
		case WSAENOTSOCK:
		case WSAEINVAL:
		    e = Impossible;
		    break;
		case WSAEWOULDBLOCK:
		    break;
		default:
		    e = UnknownError;
		    break;
	    }
	}
    }
    return r;
}


Q_LONG QSocketDevice::writeBlock( const char * data, Q_ULONG len,
			       const QHostAddress & host, Q_UINT16 port )
{
    if ( t != Datagram ) {
	qWarning( "QSocketDevice::sendBlock: Not datagram" );
	return -1; // for now - later we can do t/tcp
    }

    if ( data == 0 && len != 0 ) {
	qWarning( "QSocketDevice::sendBlock: Null pointer error" );
	return -1;
    }
    if ( !isValid() ) {
	qWarning( "QSocketDevice::sendBlock: Invalid socket" );
	return -1;
    }
    if ( !isOpen() ) {
	qWarning( "QSocketDevice::sendBlock: Device is not open" );
	return -1;
    }
    if ( !isWritable() ) {
	qWarning( "QSocketDevice::sendBlock: Write operation not permitted" );
	return -1;
    }
#if !defined(QT_NO_IPV6)
    struct alt_sockaddr_in6 a6;
    struct sockaddr_in a4;
    struct sockaddr *aa;
    SOCKLEN_T slen;

    if ( initialized >= 0x20 && host.isIp6Addr() ) {
	memset( &a6, 0, sizeof(a6) );
	a6.sin6_family = AF_INET6;
	a6.sin6_port = htons( port );
	Q_IPV6ADDR tmp = host.ip6Addr();
	for ( int i = 0; i < 16; ++i )
	    a6.sin6_addr.alt_s6_addr[i] = tmp.c[i];

	slen = sizeof( a6 );
	aa = (struct sockaddr *)&a6;

	setFamily( Ipv6 );
    } else
#endif
    {
	if ( !host.isIp4Addr() ) {
	    qWarning( "QSocketDevice: IPv6 is not supported by this version" );
	    e = Impossible;
	    return -1;
	}

	memset( &a4, 0, sizeof(a4) );
	a4.sin_family = AF_INET;
	a4.sin_port = htons( port );
	a4.sin_addr.s_addr = htonl( host.ip4Addr() );

	slen = sizeof(a4);
	aa = (struct sockaddr *)&a4;

	setFamily( Ipv4 );
    }

    // we'd use MSG_DONTWAIT + MSG_NOSIGNAL if Stevens were right.
    // but apparently Stevens and most implementors disagree
    bool done = FALSE;
    Q_LONG r = 0;
    while ( !done ) {
	r = ::sendto( socket(), data, len, 0, aa, slen );
	done = TRUE;
	if ( r == SOCKET_ERROR && e == NoError ) {//&& e != EAGAIN ) {
	    switch( WSAGetLastError() ) {
		case WSANOTINITIALISED:
		    e = Impossible;
		    break;
		case WSAENETDOWN:
		case WSAEACCES:
		case WSAENETRESET:
		case WSAESHUTDOWN:
		case WSAEHOSTUNREACH:
		case WSAECONNABORTED:
		case WSAECONNRESET:
		case WSAEADDRNOTAVAIL:
		case WSAENETUNREACH:
		case WSAETIMEDOUT:
		    e = NetworkFailure;
		    break;
		case WSAEINTR:
		    done = FALSE;
		    break;
		case WSAEINPROGRESS:
		    e = NoResources;
		    // ### perhaps try it later?
		    break;
		case WSAEFAULT:
		case WSAEOPNOTSUPP:
		case WSAEAFNOSUPPORT:
		    e = InternalError;
		    break;
		case WSAENOBUFS:
		case WSAEMSGSIZE:
		    e = NoResources;
		    break;
		case WSAENOTCONN:
		case WSAENOTSOCK:
		case WSAEINVAL:
		case WSAEDESTADDRREQ:
		    e = Impossible;
		    break;
		case WSAEWOULDBLOCK:
		    break;
		default:
		    e = UnknownError;
		    break;
	    }
	}
    }
    return r;
}


void QSocketDevice::fetchConnectionParameters()
{
    if ( !isValid() ) {
	p = 0;
	a = QHostAddress();
	pp = 0;
	pa = QHostAddress();
	return;
    }
#if !defined (QT_NO_IPV6)
    SOCKADDR_STORAGE sa;
#else
    struct sockaddr_in sa;
#endif
    memset( &sa, 0, sizeof(sa) );
    SOCKLEN_T sz;
    sz = sizeof( sa );
    if ( !::getsockname( socket(), (struct sockaddr *)(&sa), &sz ) ) {
#if !defined (QT_NO_IPV6)
	struct sockaddr *sap = (struct sockaddr *)&sa;
	if (sap->sa_family == AF_INET6) {
	    struct alt_sockaddr_in6 *sa6 = (struct alt_sockaddr_in6 *)&sa;
	    p = ntohs( sa6->sin6_port );
	    Q_IPV6ADDR tmp;
	    for ( int i = 0; i < 16; ++i )
		tmp.c[i] = sa6->sin6_addr.alt_s6_addr[i];

	    a = QHostAddress(tmp);
	} else
#endif
	{
	    struct sockaddr_in *sa4 = (struct sockaddr_in *)&sa;
	    p = ntohs( sa4->sin_port );
	    a = QHostAddress( ntohl( sa4->sin_addr.s_addr ) );
	}
    }
    pp = 0;
    pa = QHostAddress();
}


void QSocketDevice::fetchPeerConnectionParameters()
{
    // do the getpeername() lazy on Windows (sales/arc-18/37759 claims that
    // there will be problems otherwise if you use MS Proxy server)
#if !defined (QT_NO_IPV6)
    SOCKADDR_STORAGE sa;
#else
    struct sockaddr_in sa;
#endif
    memset( &sa, 0, sizeof(sa) );
    SOCKLEN_T sz;
    sz = sizeof( sa );
    if ( !::getpeername( socket(), (struct sockaddr *)(&sa), &sz ) ) {
#if !defined (QT_NO_IPV6)
	struct sockaddr *sa4 = (struct sockaddr *)&sa;
	if (sa4->sa_family == AF_INET6) {
	    struct alt_sockaddr_in6 *sa6 = (struct alt_sockaddr_in6 *)&sa;
	    p = ntohs( sa6->sin6_port );
	    Q_IPV6ADDR tmp;
	    for ( int i = 0; i < 16; ++i )
		tmp.c[i] = sa6->sin6_addr.alt_s6_addr[i];

	    a = QHostAddress(tmp);
	} else
#endif
	{
	    struct sockaddr_in *sa4 = (struct sockaddr_in *)&sa;
	    p = ntohs( sa4->sin_port );
	    a = QHostAddress( ntohl( sa4->sin_addr.s_addr ) );
	}
    }
}

Q_UINT16 QSocketDevice::peerPort() const
{
    if ( pp==0 && isValid() ) {
	QSocketDevice *that = (QSocketDevice*)this; // mutable
	that->fetchPeerConnectionParameters();
    }
    return pp;
}


QHostAddress QSocketDevice::peerAddress() const
{
    if ( pp==0 && isValid() ) {
	QSocketDevice *that = (QSocketDevice*)this; // mutable
	that->fetchPeerConnectionParameters();
    }
    return pa;
}
