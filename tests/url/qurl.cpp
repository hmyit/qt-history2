/****************************************************************************
** $Id: //depot/qt/main/tests/url/qurl.cpp#31 $
**
** Implementation of QFileDialog class
**
** Created : 950429
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.troll.no/pricing.html or email sales@troll.no for
** information about the Professional Edition licensing, or see
** http://www.troll.no/qpl/ for QPL licensing information.
**
*****************************************************************************/

#include "qurl.h"
#include "qurlinfo.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <qapplication.h>
#include <qmap.h>

struct QUrlPrivate
{
    QString protocol;
    QString user;
    QString pass;
    QString host;
    QString path;
    QString refEncoded;
    QString queryEncoded;
    bool isMalformed;
    int port;
    QString nameFilter;
    QDir dir;
    QMap<QString, QUrlInfo> entryMap;
};

/*!
  Mention that URL has some restrictions regarding the path
  encoding. URL works intern with the decoded path and
  and encoded query. For example in
  <pre>
  http://localhost/cgi-bin/test%20me.pl?cmd=Hello%20you
  </pre>
  would result in a decoded path "/cgi-bin/test me.pl"
  and in the encoded query "cmd=Hello%20you".
  Since path is internally always encoded you may NOT use
  "%00" in the path while this is ok for the query.
*/

/*!
  \a url is considered to be encoded. You can pass strings like
  "/home/weis", in this case the protocol "file" is assumed.
  This is dangerous since even this simple path is assumed to be
  encoded. For example "/home/Torben%20Weis" will be decoded to
  "/home/Torben Weis". This means: If you have a usual UNIX like
  path, you have to use @ref encode first before you pass it to URL.
*/

QUrl::QUrl()
{
    d = new QUrlPrivate;
    d->isMalformed = TRUE;
    d->nameFilter = "*";
}

QUrl::QUrl( const QString& url )
{
    d = new QUrlPrivate;
    // Assume the default protocol
    d->protocol = "file";
    d->port = -1;
    d->nameFilter = "*";
    QString tmp = url.stripWhiteSpace();
    parse( tmp );
}

QUrl::QUrl( const QUrl& url )
    : QObject()
{
    d = new QUrlPrivate;
    *d = *url.d;
}

QUrl::QUrl( const QUrl& url, const QString& relUrl_ )
{
    d = new QUrlPrivate;
    QString relUrl = relUrl_.stripWhiteSpace();
    // relUrl starts in the root ?
    if ( relUrl[0] == '/' ) {
	*this = url;
 	if ( QFileInfo( d->path ).isFile() )
 	    d->path = QFileInfo( d->path ).dirPath();
	int pos = relUrl.find( '#' );
	QString tmp;
	if ( pos == -1 ) {
	    tmp = relUrl;
	    setRef( "" );
	} else {
	    setRef( relUrl.mid( pos + 1 ) );
	    tmp = relUrl.left( pos );
	}
	setEncodedPathAndQuery( tmp );
    } else if ( relUrl[0] == '#' ) {
	// relUrl just affects the reference ?
	*this = url;
 	if ( QFileInfo( d->path ).isFile() )
 	    d->path = QFileInfo( d->path ).dirPath();
	setRef( relUrl.mid(1) );
    } else if ( relUrl.find( ":/" ) != -1 ) {
	// relUrl is a complete QUrl ?
	*this = relUrl;
    } else {	
	*this = url;
 	if ( QFileInfo( d->path ).isFile() )
 	    d->path = QFileInfo( d->path ).dirPath();
	int pos = relUrl.find( '#' );
	QString tmp;
	if ( pos == -1 ) {
	    tmp = relUrl;
	    setRef( "" );
	} else {
	    setRef( relUrl.mid( pos + 1 ) );
	    tmp = relUrl.left( pos );
	}
	decode( tmp );
	setFileName( tmp );
    }
    d->nameFilter = "*";
}

QUrl::~QUrl()
{
    delete d;
}

QString QUrl::protocol() const
{
    return d->protocol;
}

void QUrl::setProtocol( const QString& protocol )
{
    d->protocol = protocol;
}

QString QUrl::user() const
{
    return  d->user;
}

void QUrl::setUser( const QString& user )
{
    d->user = user;
}

bool QUrl::hasUser() const
{
    return !d->user.isEmpty();
}

QString QUrl::pass() const
{
    return d->pass;
}

void QUrl::setPass( const QString& pass )
{
    d->pass = pass;
}

bool QUrl::hasPass() const
{
    return !d->pass.isEmpty();
}

QString QUrl::host() const
{
    return d->host;
}

void QUrl::setHost( const QString& host )
{
    d->host = host;
}

bool QUrl::hasHost() const
{
    return !d->host.isEmpty();
}

int QUrl::port() const
{
    return d->port;
}

void QUrl::setPort( int port )
{
    d->port = port;
}

void QUrl::setPath( const QString& path )
{
    d->path = path;
}

bool QUrl::hasPath() const
{
    return !d->path.isEmpty();
}

void QUrl::setQuery( const QString& txt )
{
    d->queryEncoded = txt;
}

QString QUrl::query() const
{ 	
    return d->queryEncoded;
}

QString QUrl::ref() const
{
    return d->refEncoded;
}

void QUrl::setRef( const QString& txt )
{
    d->refEncoded = txt;
}

bool QUrl::hasRef() const
{
    return !d->refEncoded.isEmpty();
}

bool QUrl::isMalformed() const
{
    return d->isMalformed;
}

void QUrl::reset()
{
    // Assume the default protocol.
    d->protocol = "file";
    d->user = "";
    d->pass = "";
    d->host = "";
    d->path = "";
    d->queryEncoded = "";
    d->refEncoded = "";
    d->isMalformed = FALSE;
    d->port = -1;
}

void QUrl::parse( const QString& url )
{
    if ( url.isEmpty() ) {
	d->isMalformed = true;
	return;
    }

    d->isMalformed = false;

    QString port;
    int start = 0;
    uint len = url.length();
    QChar* buf = new QChar[ len + 1 ];
    QChar* orig = buf;
    memcpy( buf, url.unicode(), len * sizeof( QChar ) );

    uint pos = 0;

    // Node 1: Accept alpha or slash
    QChar x = buf[pos++];
    if ( x == '/' )
	goto Node9;
    if ( !isalpha( (int)x ) )
	goto NodeErr;

  // Node 2: Accept any amount of alphas
  // Proceed with :// :/ or :
    while( isalpha( buf[pos] ) && pos < len ) pos++;
    if ( pos == len )
	goto NodeErr;
    if (buf[pos] == ':' && buf[pos+1] == '/' && buf[pos+2] == '/' ) {
	d->protocol = QString( orig, pos ).lower();
	pos += 3;
    } else if (buf[pos] == ':' && buf[pos+1] == '/' ) {
	d->protocol = QString( orig, pos ).lower();
	pos++;
	start = pos;
	goto Node9;
    } else if ( buf[pos] == ':' ) {
	d->protocol = QString( orig, pos ).lower();
	pos++;
	goto Node11;
    } else
	goto NodeErr;

  //Node 3: We need at least one character here
    if ( pos == len )
	goto NodeOk;
    //    goto NodeErr;
#if 0
    start = pos++;
#else
    start = pos;
#endif

    // Node 4: Accept any amount of characters.
    // Terminate or / or @
    while( buf[pos] != ':' && buf[pos] != '@' && buf[pos] != '/' && pos < len ) pos++;
    if ( pos == len ) {
	d->host = QString( buf + start, pos - start );
	goto NodeOk;
    }

    x = buf[pos];
    if ( x == '@' ) {
	d->user = QString( buf + start, pos - start );
	pos++;
	goto Node7;
    } else if ( x == '/' ) {
	d->host = QString( buf + start, pos - start );
	start = pos++;
	goto Node9;
    } else if ( x != ':' )
	goto NodeErr;
    d->user = QString( buf + start, pos - start );
    pos++;

    // Node 5: We need at least one character
    if ( pos == len )
	goto NodeErr;
    start = pos++;

    // Node 6: Read everything until @
    while( buf[pos] != '@' && pos < len ) pos++;
    if ( pos == len ) {
	// Ok the : was used to separate host and port
	d->host = d->user;
	d->user = "";
	QString tmp( buf + start, pos - start );
	char *endptr;
	d->port = (unsigned short int)strtol(tmp.ascii(), &endptr, 10);
	if ((pos == len) && (strlen(endptr) == 0))
	    goto NodeOk;
	// there is more after the digits
	pos -= strlen(endptr);
	start = pos++;
	goto Node9;
    }
    d->pass = QString( buf + start, pos - start);
    pos++;

    // Node 7: We need at least one character
Node7:
    if ( pos == len )
	goto NodeErr;
    start = pos++;

    // Node 8: Read everything until / : or terminate
    while( buf[pos] != '/' && buf[pos] != ':' && pos < len ) pos++;
    if ( pos == len ) {
	d->host = QString( buf + start, pos - start );
	goto NodeOk;
    }

    x = buf[pos];
    d->host = QString( buf + start, pos - start );
    if ( x == '/' ) {
	start = pos++;
	goto Node9;
    } else if ( x != ':' )
	goto NodeErr;
    pos++;

    // Node 8a: Accept at least one digit
    if ( pos == len )
	goto NodeErr;
    start = pos;
    if ( !isdigit( buf[pos++] ) )
	goto NodeErr;

    // Node 8b: Accept any amount of digits
    while( isdigit( buf[pos] ) && pos < len ) pos++;
    port = QString( buf + start, pos - start );
    d->port = port.toUShort();
    if ( pos == len )
	goto NodeOk;
    start = pos++;

    // Node 9: Accept any character and # or terminate
Node9:
    while( buf[pos] != '#' && pos < len ) pos++;
    if ( pos == len ) {
	QString tmp( buf + start, len - start );
	setEncodedPathAndQuery( tmp );
	// setEncodedPathAndQuery( QString( buf + start, pos - start ) );
	goto NodeOk;
    }
    else if ( buf[pos] != '#' )
	goto NodeErr;
    setEncodedPathAndQuery( QString( buf + start, pos - start ) );
    pos++;

    // Node 10: Accept all the rest
    d->refEncoded = QString( buf + pos, len - pos );
    goto NodeOk;

    // Node 11 We need at least one character
Node11:
    start = pos;
    if ( pos++ == len )
	goto NodeOk;
    //    goto NodeErr;

    // Node 12: Accept the res
    setEncodedPathAndQuery( QString( buf + start, len - start ) );
    goto NodeOk;

NodeOk:
    if ( d->path.isEmpty() )
	d->path = "/";
    delete []orig;
    return;

NodeErr:
    if ( d->path.isEmpty() )
	d->path = "/";
    qDebug( "Error in parsing \"%s\"", url.ascii() );
    delete []orig;
    d->isMalformed = true;

}

QUrl& QUrl::operator=( const QString& url )
{
    reset();

//     d->protocol = "file";
//     d->port = -1;
//     d->nameFilter = "*";

//     QString tmp = url.stripWhiteSpace();
//     parse( tmp );
//     d->url = tmp;

    parse( url );

    return *this;
}

QUrl& QUrl::operator=( const QUrl& url )
{
    *d = *url.d;
    return *this;
}

bool QUrl::operator==( const QUrl& url ) const
{
    if ( isMalformed() || url.isMalformed() )
	return FALSE;

    if ( d->protocol == url.d->protocol &&
	 d->user == url.d->user &&
	 d->pass == url.d->pass &&
	 d->host == url.d->host &&
	 d->path == url.d->path &&
	 d->queryEncoded == url.d->queryEncoded &&
	 d->refEncoded == url.d->refEncoded &&
	 d->isMalformed == url.d->isMalformed &&
	 d->port == url.d->port )
	return TRUE;

    return FALSE;
}

bool QUrl::operator==( const QString& url ) const
{
    QUrl u( url );
    return ( *this == u );
}

bool QUrl::cmp( QUrl &url, bool ignoreTrailing )
{
    if ( ignoreTrailing ) {
	QString path1 = path(1);
	QString path2 = url.path(1);
	if ( path1 != path2 )
	    return FALSE;

	if ( d->protocol == url.d->protocol &&
	     d->user == url.d->user &&
	     d->pass == url.d->pass &&
	     d->host == url.d->host &&
	     d->path == url.d->path &&
	     d->queryEncoded == url.d->queryEncoded &&
	     d->refEncoded == url.d->refEncoded &&
	     d->isMalformed == url.d->isMalformed &&
	     d->port == url.d->port )
	    return TRUE;
	
	return FALSE;
    }

    return ( *this == url );
}

void QUrl::setFileName( const QString& name )
{
    // Remove '/' in the front
    int start = 0;
    while( name[start] == '/' )
	start++;

    // Empty path ?
    int len = d->path.length();
    if ( len == 0 ) {
	d->path = "/";
	d->path += name.mid( start );
	return;
    }

    // The current path is a directory ?
    if ( d->path[ len - 1 ] == '/' ) {
	// See wether there are multiple '/' characters
	while ( len >= 1 && d->path[ len - 1 ] == '/' )
	    len--;

	// Does the path only consist of '/' characters ?
	if ( len == 0 && d->path[ 1 ] == '/' ) {
	    d->path = "/";
	    d->path += name.mid( start );
	    return;
	}

	// Just append the filename
	d->path += name.mid( start );
	return;
    }

    // Find the rightmost '/'
    int i = d->path.findRev( '/', len - 1 );
    // If ( i == -1 ) => The first character is not a '/' ???
    // This looks strange ...
    if ( i == -1 ) {
	d->path = "/";
	d->path += name.mid( start );
	return;
    }

    // #### these two lines are not correct!
    //d->path.truncate( i + 1 );
    d->path += "/" + name.mid( start );
}

QString QUrl::encodedPathAndQuery( int trailing, bool noEmptyPath )
{
    QString tmp = path( trailing );
    if ( noEmptyPath && tmp.isEmpty() )
	tmp = "/";

    encode( tmp );

    if ( !d->queryEncoded.isEmpty() ) {
	tmp += "?";
	tmp += d->queryEncoded;
    }

    return tmp;
}

void QUrl::setEncodedPathAndQuery( const QString& path )
{
    int pos = path.find( '?' );
    if ( pos == -1 ) {
	d->path = path;
	d->queryEncoded = "";
    } else {
	d->path = path.left( pos );
	d->queryEncoded = path.mid( pos + 1 );
    }

    decode( d->path );
}

QString QUrl::path() const
{
    return QDir::cleanDirPath( d->path );
}

QString QUrl::path( int trailing ) const
{
    QString result = path();

    // No modifications to the trailing stuff ?
    if ( trailing == 0 )
	return result;
    else if ( trailing == 1 ) {
	int len = result.length();
	// Path is empty ? => It should still be empty
	if ( len == 0 )
	    result = "";
	// Add a trailing '/'
	else if ( result[ len - 1 ] != '/' )
	    result += "/";
	return result;
    } else if ( trailing == -1 ) {
	// Dont change anything if path is just the slash
	if ( result == "/" )
	    return result;
	int len = result.length();
	// Strip trailing slash
	if ( len > 0 && result[ len - 1 ] == '/' )
	    result.truncate( len - 1 );
	return result;
    } else
	assert( 0 );

    return QString::null;
}

bool QUrl::isLocalFile() const
{
    return d->protocol == "file";
}

QString QUrl::url()
{
    QString result = url( 0 );
    return result;
}

QString QUrl::url( int trailing, bool stripRef )
{
    QString u = d->protocol;
    if ( hasHost() ) {
	u += "://";
	if ( hasUser() ) {
	    u += d->user;
	    if ( hasPass() ) {
		u += ":";
		u += d->pass;
	    }
	    u += "@";
	}
	u += d->host;
	if ( d->port != -1 ) {
	    QString buffer = QString( ":%1" ).arg( d->port );
	    u += buffer;
	}
    } else
	u += ":";

    QString tmp;
    if ( trailing == 0 )
	tmp = d->path;
    else
	tmp = path( trailing );
    encode( tmp );
    u += tmp;

    if ( !d->queryEncoded.isEmpty() ) {
	u += "?";
	u += d->queryEncoded;
    }

    if ( hasRef() && !stripRef ) {
	u += "#";
	u += d->refEncoded;
    }

    return u;
}

QString QUrl::filename( bool stripTrailingSlash )
{
    QString fname;

    int len = d->path.length();
    if ( len == 0 )
	return fname;

    if ( stripTrailingSlash ) {
	while ( len >= 1 && d->path[ len - 1 ] == '/' )
	    len--;
    } else if ( d->path[ len - 1 ] == '/' ) {
	// Path is a directory => empty filename
	return fname;
    }

    // Does the path only consist of '/' characters ?
    if ( len == 1 && d->path[ 1 ] == '/' )
	return fname;

    int i = d->path.findRev( '/', len - 1 );
    // If ( i == -1 ) => The first character is not a '/' ???
    // This looks like an error to me.
    if ( i == -1 )
	return fname;

    fname = d->path.mid( i + 1 );
    return fname;
}

void QUrl::addPath( const QString& txt )
{
    if ( txt.isEmpty() )
	return;

    int len = d->path.length();
    // Add the trailing '/' if it is missing
    if ( txt[ 0 ] != '/' && ( len == 0 || d->path[ len - 1 ] != '/' ) )
	d->path += "/";

    // No double '/' characters
    int start = 0;
    if ( len != 0 && d->path[ len - 1 ] == '/' )
	while( txt[ start ] == '/' )
	    start++;

    d->path += txt.mid( start );
}

QString QUrl::directory( bool stripTrailingSlashFromResult,
			bool ignoreTrailingSlashInPath )
{
    QString result;
    // Have to deal with the traling slash before
    // we parse the string ?
    if ( ignoreTrailingSlashInPath )
	result = path( -1 );
    else
	result = d->path;

    // Check trivial cases
    if ( result.isEmpty() || result == "/" )
	return result;

    // Look out for the rightmost slash
    int i = result.findRev( "/" );
    // No slash => empty directory
    if ( i == -1 )
	return result;

    // Another trivial case
    if ( i == 0 ) {
	result = "/";
	return result;
    }

    // Strip th trailing slash from the result
    if ( stripTrailingSlashFromResult )
	result = d->path.left( i );
    else
	result = d->path.left( i + 1 );

    return result;
}

void QUrl::encode( QString& url )
{
    int old_length = url.length();

    if ( !old_length )
	return;

    QString new_url;
    //char *new_url = new char[ old_length * 3 + 1 ];
    int new_length = 0;

    for ( int i = 0; i < old_length; i++ ) {
	// 'unsave' and 'reserved' characters
	// according to RFC 1738,
	// 2.2. QUrl Character Encoding Issues (pp. 3-4)
	// Torben: Added the space characters
	if ( strchr("<>#@\"&%$:,;?={}|^~[]\'`\\ \n\t\r", url[ i ].unicode() ) ) {
	    new_url[ new_length++ ] = '%';

	    char c = url[ i ].unicode() / 16;
	    c += (c > 9) ? ('A' - 10) : '0';
	    new_url[ new_length++ ] = c;

	    c = url[ i ].unicode() % 16;
	    c += (c > 9) ? ('A' - 10) : '0';
	    new_url[ new_length++ ] = c;
	
	}
	else
	    new_url[ new_length++ ] = url[ i ];
    }

    //new_url[ new_length ] = 0;
    url = new_url;
}

char QUrl::hex2int( char c )
{
    if ( c >= 'A' && c <='F')
	return c - 'A' + 10;
    if ( c >= 'a' && c <='f')
	return c - 'a' + 10;
    if ( c >= '0' && c <='9')
	return c - '0';
    return 0;
}

void QUrl::decode( QString& url )
{
    int old_length = url.length();
    if ( !old_length )
	return;

    int new_length = 0;

    // make a copy of the old one
    //char *new_url = new char[ old_length + 1];
    QString new_url;

    int i = 0;
    while( i < old_length ) {
	char character = url[ i++ ].unicode();
	if ( character == '%' ) {
	    character = hex2int( url[ i ].unicode() ) * 16 + hex2int( url[ i+1].unicode() );
	    i += 2;
	}
	new_url [ new_length++ ] = character;
    }
    //new_url [ new_length ] = 0;
    url = new_url;
}

void QUrl::listEntries( int filterSpec = QDir::DefaultFilter,
			int sortSpec   = QDir::DefaultSort )
{
    listEntries( d->nameFilter, filterSpec, sortSpec );
}

void QUrl::listEntries( const QString &nameFilter, int filterSpec = QDir::DefaultFilter,
			int sortSpec   = QDir::DefaultSort )
{
    clearEntries();
    if ( isLocalFile() ) {
	d->dir = QDir( d->path );
	d->dir.setNameFilter( nameFilter );
	d->dir.setMatchAllDirs( TRUE );
	
	const QFileInfoList *filist = d->dir.entryInfoList( filterSpec, sortSpec );
	if ( !filist ) {
	    QString msg = QUrl::tr( "Could not read directory\n" + d->path );
	    emit error( ReadDir, msg );
	    return;
	}
	
	emit start();
	QFileInfoListIterator it( *filist );
	QFileInfo *fi;
	while ( ( fi = it.current()) != 0 ) {
	    ++it;
	    QUrlInfo inf( fi->fileName(), 0/*permissions*/, fi->owner(), fi->group(),
			  fi->size(), fi->lastModified(), fi->lastRead(), fi->isDir(), fi->isFile(),
			  fi->isSymLink(), fi->isWritable(), fi->isReadable(), fi->isExecutable() );
	    emit entry( inf );
	    addEntry( inf );
	}
	emit finished();
    }
}

void QUrl::mkdir( const QString &dirname )
{
    if ( isLocalFile() ) {
	d->dir = QDir( d->path );
	if ( d->dir.mkdir( dirname ) ) {
	    QFileInfo fi( d->dir, dirname );
	    QUrlInfo inf( fi.fileName(), 0/*permissions*/, fi.owner(), fi.group(),
			  fi.size(), fi.lastModified(), fi.lastRead(), fi.isDir(), fi.isFile(),
			  fi.isSymLink(), fi.isWritable(), fi.isReadable(), fi.isExecutable() );
	    emit entry( inf );
	    emit createdDirectory( inf );
	} else {
	    QString msg = QUrl::tr( "Could not create directory\n" + dirname );
	    emit error( CreateDir, msg );
	}
    }
}

void QUrl::remove( const QString &filename )
{
    QDir dir( d->path );
    if ( dir.remove( filename ) )
	emit removed( filename );
    else {
	QString msg = QUrl::tr( "Could not delete file\n" + filename );
	emit error( DeleteFile, msg );
    }
}

void QUrl::rename( const QString &oldname, const QString &newname )
{
    QDir dir( d->path );
    if ( dir.rename( oldname, newname ) )
	emit itemChanged( oldname, newname );
}

void QUrl::copy( const QString &from, const QString &to )
{
    QFile f( from );
    if ( !f.open( IO_ReadOnly ) )
	return;

    QFileInfo fi( to );
    if ( FALSE /*fi.exists()*/ ) { // #### todo
	if ( !fi.isFile() ) {
	    f.close();
	    qWarning( QString( "Couldn't write file\n%1" ).arg( to ) );
	    return;
	}
    }

    QFile f2( to );
    if ( !f2.open( IO_WriteOnly ) ) {
	f.close();
	return;
    }

    char *block = new char[ 1024 ];
    bool error = FALSE;
    while ( !f.atEnd() ) {
	int len = f.readBlock( block, 1024 );
	if ( len == -1 )
	    error = TRUE;
	    break;
	f2.writeBlock( block, len );
	qApp->processEvents();
    }

    delete[] block;

    f.close();
    f2.close();

    return;
}

void QUrl::copy( const QStringList &files, const QString &dest, bool move )
{
    QString de = dest;
    if ( de.left( QString( "file:" ).length() ) == "file:" )
	de.remove( 0, QString( "file:" ).length() );
    QStringList::ConstIterator it = files.begin();
    for ( ; it != files.end(); ++it ) {
	if ( TRUE /*isFile*/) {
	    copy( *it, de + "/" + QFileInfo( *it ).fileName() );
	    if ( move )
		QFile::remove( *it );
	}
    }

    return;
}

void QUrl::setNameFilter( const QString &nameFilter )
{
    d->nameFilter = nameFilter;
}

QString QUrl::nameFilter() const
{
    return d->nameFilter;
}

QUrlInfo QUrl::makeInfo() const
{
    if ( d->entryMap.contains( "." ) )
	return d->entryMap[ "." ];

    if ( isLocalFile() ) {
	QFileInfo inf( d->path );
	return QUrlInfo( inf.fileName(), 0/*permissions*/, inf.owner(), inf.group(),
			 inf.size(), inf.lastModified(), inf.lastRead(), inf.isDir(), inf.isFile(),
			 inf.isSymLink(), inf.isWritable(), inf.isReadable(), inf.isExecutable() );
    }

    return QUrlInfo();
}

QString QUrl::toString() const
{
    if ( isLocalFile() )
	return d->protocol + ":" + QDir::cleanDirPath( d->path );

    return QString::null;
}

QUrl::operator QString() const
{
    return toString();
}

bool QUrl::cdUp()
{
//    addPath( ".." );
    d->path += "/..";
    return TRUE;
}

void QUrl::sendNewEntry( const QUrlInfo &i )
{
    emit entry( i );
    addEntry( i );
}

void QUrl::clearEntries()
{
    d->entryMap.clear();
}

void QUrl::addEntry( const QUrlInfo &i )
{
    d->entryMap[ i.name().stripWhiteSpace() ] = i;
}

QUrlInfo QUrl::info( const QString &entry ) const
{
    return d->entryMap[ entry ];
}

void QUrl::listFinished()
{
    emit finished();
}
