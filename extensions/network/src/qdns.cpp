/****************************************************************************
** $Id: main.cpp,v 1.2 1997/02/26 14:50:17 agulbra Exp $
**
** Implementation of something useful.
**
** Created : 979899
**
** Copyright (C) 1992-1998 Troll Tech AS.  All rights reserved.
**
** This file is part of Troll Tech's internal development tree for Qt.
**
** This header text will be replaced by an appropriate text by the
** mkdist script which generates external distributions.
**
** If you are using the Qt Professional Edition or the Qt Free Edition,
** please notify Troll Tech at <info@troll.no> if you see this text.
**
** To Troll Tech developers: This header was generated by the script
** fixcopyright-int. It has the same number of text lines as the free
** and professional editions to avoid line number inconsistency.
**
*****************************************************************************/

#include "qdns.h"

#include "qdatetime.h"
#include "qdict.h"
#include "qlist.h"
#include "qstring.h"
#include "qtimer.h"
#include "qapplication.h"
#include "qvector.h"
#include "qstrlist.h"
#include <qptrdict.h>


static QList<QHostAddress> * ns = 0;
static QStrList * domains = 0;

static void doResInit( void );

#define DEBUG_QDNS

class QDnsPrivate {
public:
    QDnsPrivate() {}
};


class QDnsRR;
class QDnsDomain;


// QDnsRR is the class used to store a single RR.  QDnsRR can store
// all of the supported RR types.  a QDnsRR is always cached.

// QDnsRR is mostly constructed from the outside.  a but hacky, but
// permissible since the entire class is internal.

class QDnsRR {
public:
    QDnsRR( const QString & label );
    ~QDnsRR();

public:
    QDnsDomain * domain;
    QDns::RecordType t;
    bool nxdomain;
    bool current;
    Q_UINT32 expireTime;
    Q_UINT32 deleteTime;
    // somewhat space-wasting per-type data
    // a / aaaa
    QHostAddress address;
    // cname / mx / srv / ptr
    QString target;
    // mx / srv
    Q_UINT32 priority;
    // srv
    Q_UINT32 weight;
    // txt
    QString text; // could be overloaded into target...
private:

};


class QDnsDomain {
public:
    QDnsDomain( const QString & label );
    ~QDnsDomain();

    static void add( const QString & label, QDnsRR * );
    static QList<QDnsRR> * cached( const QString &, QDns::RecordType );
    static void add( const QDns *, const QString & );
    static void remove( const QDns * );

    void take( QDnsRR * );

    void sweep();

    bool isEmpty() const { return rrs == 0 || rrs->isEmpty(); }

    QString name() const { return l; }

public:
    QString l;
    QList<QDnsRR> * rrs;
    QPtrDict<void> * dns;
};


class QDnsQuery: public QTimer { // this inheritance is a very evil hack
public:
    QDnsQuery(): id( 0 ), t( QDns::None ), step(0), started(0) {}
    Q_UINT16 id;
    QDns::RecordType t;
    QString l;

    uint step;
    Q_UINT32 started;
};



class QDnsAnswer {
public:
    QDnsAnswer( const QByteArray &, QDnsQuery * );
    ~QDnsAnswer();

    void parse();
    void notify();

private:
    QDnsQuery * q;

    bool ok;

    Q_UINT8 * answer;
    int size;
    int pp;

    QList<QDnsRR> * rrs;

    // convenience
    int next;
    int ttl;
    QString label;
    QDnsRR * rr;

    QString readString();
    void parseA();
    void parseAaaa();
    void parseMx();
    void parseSrv();
    void parseCname();
    void parsePtr();
    void parseTxt();
};


QDnsRR::QDnsRR( const QString & label )
    : domain( 0 ), t( QDns::None ),
      nxdomain( FALSE ), current( FALSE ),
      expireTime( 0 ), deleteTime( 0 ),
      priority( 0 ), weight( 0 )
{
    QDnsDomain::add( label, this );
}


// not supposed to be deleted except by QDnsDomain
QDnsRR::~QDnsRR()
{
    // nothing is necessary
}


QDnsAnswer::QDnsAnswer( const QByteArray& answer_,
			QDnsQuery * query_ )
{
    ok = TRUE;

    answer = (Q_UINT8 *)(answer_.data());
    size = answer_.size();
    q = query_;
    pp = 0;
    rrs = new QList<QDnsRR>;
    rrs->setAutoDelete( FALSE );
    next = size;
    ttl = 0;
    label = QString::null;
    rr = 0;
};


QDnsAnswer::~QDnsAnswer()
{
    if ( !ok && rrs ) {
	QListIterator<QDnsRR> it( *rrs );
	QDnsRR * rr;
	while( (rr=it.current()) != 0 ) {
	    ++it;
	    rr->t = QDns::None; // will be deleted very quickly
	}
    }
}


QString QDnsAnswer::readString()
{
    int p = pp;
    QString r = QString::null;
    Q_UINT8 b;
    while( TRUE ) {
	b = 128;
	if ( p >= 0 && p < size )
	    b = answer[p];

	switch( b >> 6 ) {
	case 0:
	    p++;
	    if ( b == 0 ) {
		if ( p > pp )
		    pp = p;
		return r.isNull() ? QString( "." ) : r;
	    }
	    if ( !r.isNull() )
		r += '.';
	    while( b-- > 0 )
		r += QChar( answer[p++] );
	    break;
	default:
	    ok = FALSE;
	    return QString::null;
	    break;
	case 3:
	    int q = ( (answer[p] & 0x3f) << 8 ) + answer[p+1];
	    if ( q >= pp || q >= p ) {
		ok = FALSE;
		return QString::null;
	    }
	    if ( p >= pp )
		pp = p + 2;
	    p = q;
	    break;
	}
    }
}



void QDnsAnswer::parseA()
{
    if ( next != pp + 4 ) {
#if defined(DEBUG_QDNS)
	qDebug( "QDns: saw %d bytes long IN A for %s",
		next - pp, label.ascii() );
#endif
	return;
    }

    rr = new QDnsRR( label );
    rr->t = QDns::A;
    rr->address = QHostAddress( ( answer[pp+0] << 24 ) +
				( answer[pp+1] << 16 ) +
				( answer[pp+2] <<  8 ) +
				( answer[pp+3] ) );
#if defined(DEBUG_QDNS)
    qDebug( "QDns: saw %s IN A %s (ttl %d)", label.ascii(),
	    rr->address.ip4AddrString().ascii(), ttl );
#endif

}


void QDnsAnswer::parseAaaa()
{
    // let's ignore it for now
}



void QDnsAnswer::parseMx()
{

}


void QDnsAnswer::parseSrv()
{
}


void QDnsAnswer::parseCname()
{
}


void QDnsAnswer::parsePtr()
{
}


void QDnsAnswer::parseTxt()
{
}


void QDnsAnswer::parse()
{
    // okay, do the work...
    if ( (answer[2] & 0x78) != 0 ) {
#if defined(DEBUG_QDNS)
	qDebug( "DNS Manager: asnwer to wrong query type (%d)", answer[1] );
#endif
	ok = FALSE;
	return;
    }

    // we skip aa

    if ( (answer[2] & 2) != 0 ) {
#if defined(DEBUG_QDNS)
	qDebug( "DNS Manager: truncated answer; pressing on" );
#endif
    }

    // we don't test RD
    // we don't test RA
    // we don't test the MBZ fields

    if ( (answer[3] & 0x0f) == 3 ) {
#if defined(DEBUG_QDNS)
	qDebug( "DNS Manager: saw NXDomain for %s", q->l.ascii() );
#endif
	// NXDomain.  cache that for... how long?
	rr = new QDnsRR( q->l );
	rr->t = q->t;
	rr->deleteTime = q->started + 300;
	rr->expireTime = q->started + 300;
	rr->nxdomain = TRUE;
	rr->current = TRUE;
	rrs->append( rr );
	return;
    }

    if ( (answer[3] & 0x0f) != 0 ) {
#if defined(DEBUG_QDNS)
	qDebug( "DNS Manager: error code %d", answer[3] & 0x0f );
#endif
	ok = FALSE;
	return;
    }

    int qdcount = ( answer[4] << 8 ) + answer[5];
    int ancount = ( answer[6] << 8 ) + answer[7];
    int nscount = ( answer[8] << 8 ) + answer[9];
    int adcount = (answer[10] << 8 ) +answer[11];

    int section = 0;
    pp = 12;

    // read query
    while( qdcount > 0 && pp < size ) {
	// should I compare the string against q->l?
	(void)readString();
	if ( !ok )
	    return;
	pp += 4;
	qdcount--;
    }

    // answers and stuff
    int rrno = 0;
    while( rrno < ancount + nscount + adcount && pp < size ) {
	label = readString();
	if ( !ok )
	    return;
	uint rdlength = 0;
	if ( pp + 10 <= size )
	    rdlength = ( answer[pp+8] << 8 ) + answer[pp+9];
	if ( pp + 10 + rdlength > size ) {
#if defined(DEBUG_QDNS)
	    qDebug( "DNS Manager: ran out of stuff to parse (%d+%d>%d (%d)",
		    pp, rdlength, size, rrno < ancount );
#endif
	    // if we're still in the AN section, we should go back and
	    // at least down the TTLs.  probably best to invalidate
	    // the results.
	    // the rrs list is good for this
	    ok = ( rrno < ancount );
	    return;
	}
	uint type, clas;
	type = ( answer[pp+0] << 8 ) + answer[pp+1];
	clas = ( answer[pp+2] << 8 ) + answer[pp+3];
	ttl = ( answer[pp+4] << 24 ) + ( answer[pp+5] << 16 ) +
	      ( answer[pp+6] <<  8 ) + answer[pp+7];
	pp = pp + 10;
#if defined(DEBUG_QDNS)
	debug( "class %d, type %d, label %s, ttl %d",
	       clas, type, label.isNull() ? "." : label.ascii(), ttl );
#endif
	if ( clas != 1 ) {
#if defined(DEBUG_QDNS)
	    qDebug( "DNS Manager: class $d (not internet) for %s",
		    clas, label.isNull() ? "." : label.ascii() );
#endif
	} else {
	    next = pp + rdlength;
	    rr = 0;
	    switch( type ) {
	    case 1:
		parseA();
		break;
	    case 28:
		parseAaaa();
		break;
	    case 15:
		parseMx();
		break;
	    case 33:
		parseSrv();
		break;
	    case 5:
		parseCname();
		break;
	    case 12:
		parsePtr();
		break;
	    case 16:
		parseTxt();
		break;
	    case 2:
		// we ignore NS records, but quietly
		break;
	    default:
		// something we don't know
#if defined(DEBUG_QDNS)
		qDebug( "DNS Manager: type $d for %s", type,
			label.isNull() ? "." : label.ascii() );
#endif
		break;
	    }
	    if ( rr ) {
		rr->expireTime = q->started + ttl;
		rr->deleteTime = ( rrno < ancount || ttl < 600)
				 ? q->started + ttl : 0;
		rr->current = TRUE;
		rrs->append( rr );
	    }
	}
	if ( !ok )
	    return;
	pp = next;
	next = size;
	rrno++;
    }

#if defined(DEBUG_QDNS)
    qDebug( "DNS Manager: ()" );
#endif
}


class QDnsUgleHack: public QDns {
public:
    void ugle() { emit statusChanged(); }
};


void QDnsAnswer::notify()
{
    if ( !rrs || !ok )
	return;

    QPtrDict<void> notified;
    notified.setAutoDelete( FALSE );

    QListIterator<QDnsRR> it( *rrs );
    QDnsRR * rr;
    it.toFirst();
    while ( (rr=it.current()) != 0 ) {
	++it;
	if ( rr->domain && notified.find( (void*)(rr->domain) ) == 0 &&
	     rr->domain->dns ) {
	    notified.insert( (void*)(rr->domain), (void*)42 );
	    QPtrDictIterator<void> it2( *rr->domain->dns );
	    QDns * dns;
	    it2.toFirst();
	    while( (dns=(QDns*)(it2.current())) != 0 ) {
		++it2;
		// this test should not hit... unless someone's been
		// playing a great deal with setLabel().  so we play
		// it safe and do test.
		if ( notified.find( (void*)dns ) == 0 ) {
		    notified.insert( (void*)dns, (void*)42 );
#if defined( DEBUG_QDNS )
		    qDebug( "DNS Manager: status change for %s (type %d)",
			    dns->label().ascii(), dns->recordType() );
#endif
		    ((QDnsUgleHack*)dns)->ugle();
		}
	    }
	}
    }
}


//
//
// QDnsManager 
//
//


class QDnsManager: public QDnsSocket {
private:
public: // just to silence the moronic g++.
    QDnsManager();
    ~QDnsManager();
public:
    static QDnsManager * manager();

    QDnsDomain * domain( const QString & );
    QDnsQuery * query( const QString &, QDns::RecordType );

    void transmitQuery( QDnsQuery * );
    void transmitQuery( int );

    // reimplementation of the slots
    void cleanCache();
    void retransmit();
    void answer();

public:
    QVector<QDnsQuery> queries;
    QDict<QDnsDomain> cache;
    QSocketDevice * socket;
};



static QDnsManager * globalManager;


QDnsManager * QDnsManager::manager()
{
    if ( !globalManager )
	new QDnsManager();
    return globalManager;
}


QDnsManager::QDnsManager()
    : QDnsSocket( qApp, "Internal DNS manager" ),
      queries( QVector<QDnsQuery>( 0 ) ),
      cache( QDict<QDnsDomain>( 83, FALSE ) ),
      socket( new QSocketDevice( QSocketDevice::Datagram ) )
{
    cache.setAutoDelete( TRUE );
    globalManager = this;

    QTimer * sweepTimer = new QTimer( this );
    sweepTimer->start( 1000 * 60 * 5 );
    connect( sweepTimer, SIGNAL(timeout()),
	     this, SLOT(cleanCache()) );

    QSocketNotifier * rn = new QSocketNotifier( socket->socket(),
						QSocketNotifier::Read,
						this, "dns socket watcher" );
    socket->setAddressReusable( FALSE );
    socket->setBlocking( FALSE );
    connect( rn, SIGNAL(activated(int)),
	     this, SLOT(answer()) );

    if ( !ns )
	doResInit();
}


QDnsManager::~QDnsManager()
{
    if ( globalManager )
	globalManager = 0;
}


void QDnsManager::cleanCache()
{
    bool again = FALSE;
    QDictIterator<QDnsDomain> it( cache );
    QDnsDomain * d;
    while( (d=it.current()) != 0 ) {
	++it;
	d->sweep(); // after this, d may be empty
	if ( !again )
	    again = !d->isEmpty();
    }
    if ( !again )
	delete this;
}


void QDnsManager::retransmit()
{
    const QObject * o = sender();
    if ( o == 0 || globalManager == 0 || this != globalManager )
	return;
    int q = 0;
    while( q < queries.size() && queries[q] != o )
	q++;
    if ( q < queries.size() )
	transmitQuery( q );
}


void QDnsManager::answer()
{
#if defined(DEBUG_QDNS)
    qDebug( "DNS Manager: answer arrived" );
#endif
    QByteArray a( 16383 ); // large enough for anything, one suspects
    int r = socket->readBlock( a.data(), a.size() );
    if ( r < 12 )
	return;
    a.resize( r );

    int id = (a[0] << 8) + a[1];
    int i = 0;
    while( i < queries.size() &&
	   !( queries[i] && queries[i]->id == id ) )
	i++;
    if ( i == queries.size() ) {
#if defined(DEBUG_QDNS)
	qDebug( "DNS Manager: bad id (%d)", id );
#endif
	return;
    }

    // at this point queries[i] is whatever we asked for.

    if ( (Q_UINT8)(a[2]) & 0x80 == 0 ) {
#if defined(DEBUG_QDNS)
	qDebug( "DNS Manager: received a query" );
#endif
	return;
    }

    QDnsAnswer answer( a, queries[i] );
    answer.parse();
    answer.notify();
};


void QDnsManager::transmitQuery( QDnsQuery * query )
{
    int i = queries.size();
    queries.resize( i+1 );
    queries.insert( i, query );
    transmitQuery( i );
}


void QDnsManager::transmitQuery( int i )
{
    if ( i < 0 || i >= queries.size() )
	return;
    QDnsQuery * q = queries[i];

    QByteArray p( 12 + q->l.length() + 2 + 4 );
    if ( p.size() > 500 )
	return; // way over the limit, so don't even try

    // header
    // id
    p[0] = q->id >> 8;
    p[1] = q->id & 0xff;
    p[2] = 1; // recursion desired, rest is 0
    p[3] = 0; // all is 0
    // one query
    p[4] = 0;
    p[5] = 1;
    // no answers, name servers or additional data
    p[6] = p[7] = p[8] = p[9] = p[10] = p[11] = 0;

    // the name is composed of several components.  each needs to be
    // written by itself... so we write...
    // oh, and we assume that there's no funky characters in there.
    int pp = 12;
    int lp = 0;
    while( lp < q->l.length() ) {
	int le = q->l.find( '.', lp );
	if ( le < 0 )
	    le = q->l.length();
	QString component = q->l.mid( lp, le-lp );
	p[pp++] = component.length();
	int cp;
	for( cp=0; cp < component.length(); cp++ )
	    p[pp++] = component[cp].latin1();
	lp = le + 1;
    }
    // final null
    p[pp++] = 0;
    // query type
    p[pp++] = 0;
    switch( q->t ) {
    case QDns::A:
	p[pp++] = 1;
	break;
    case QDns::Aaaa:
	p[pp++] = 28;
	break;
    case QDns::Mx:
	p[pp++] = 15;
	break;
    case QDns::Srv:
	p[pp++] = 33;
	break;
    case QDns::Cname:
	p[pp++] = 5;
	break;
    case QDns::Ptr:
	p[pp++] = 12;
	break;
    case QDns::Txt:
	p[pp++] = 16;
	break;
    default:
	p[pp++] = 255; // any
	break;
    }
    // query class (always internet)
    p[pp++] = 0;
    p[pp++] = 1;

    if ( !ns || ns->isEmpty() )
	return;

    debug( "here %s %d", q->l.ascii(), pp );
    socket->writeBlock( p.data(), pp, *ns->first(), 53 );
}


QDnsQuery * QDnsManager::query( const QString & label,
						  QDns::RecordType type )
{
    int q = 0;
    QDnsManager * m = manager();
    while ( q < m->queries.size() && ( m->queries[q]->t != type ||
				       m->queries[q]->l != label ) )
	q++;
    if ( q < m->queries.size() )
	return m->queries[q];

    return 0;
}


QDnsDomain * QDnsManager::domain( const QString & label )
{
    QDnsDomain * d = cache.find( label );
    if ( !d ) {
	d = new QDnsDomain( label );
	cache.insert( label, d );
    }
    return d;
}


//
//
// the QDnsDomain class looks after and coordinates queries for QDnsRRs for
// each domain, and the cached QDnsRRs.  (A domain, in DNS terminology, is
// a node in the DNS.  "no", "troll.no" and "lupinella.troll.no" are
// all domains.)
//
//


// this is ONLY to be called by QDnsManager::domain().  noone else.
QDnsDomain::QDnsDomain( const QString & label )
{
    l = label;
    rrs = 0;
    dns = 0;
}


QDnsDomain::~QDnsDomain()
{
    delete rrs;
    rrs = 0;
    delete dns;
    dns = 0;
}


void QDnsDomain::add( const QString & label, QDnsRR * rr )
{
    QDnsDomain * d = QDnsManager::manager()->domain( label );
    if ( !d->rrs ) {
	d->rrs = new QList<QDnsRR>;
	d->rrs->setAutoDelete( TRUE );
    }
    d->rrs->append( rr );
    rr->domain = d;
}


QList<QDnsRR> * QDnsDomain::cached( const QString & label,
				    QDns::RecordType t )
{
#if defined(DEBUG_QDNS)
    qDebug( "looking at cache for %s (t %d)", label.ascii(), t );
#endif
    QDnsDomain * d = QDnsManager::manager()->domain( label );
    QList<QDnsRR> * l = new QList<QDnsRR>;
    l->setAutoDelete( FALSE );
    if ( d->rrs ) {
	d->rrs->first();
	QDnsRR * rr;
	while( (rr=d->rrs->current()) != 0 ) {
	    if ( rr->t == t )
		l->append( rr );
	    d->rrs->next();
	}
    }
    return l;
}


void QDnsDomain::add( const QDns * dns, const QString & domain )
{
#if defined(DEBUG_QDNS)
    qDebug( "adding domain %s (t %d)",
	    domain.ascii(), dns->recordType() );
#endif
    QDnsDomain * d = QDnsManager::manager()->domain( domain );
    if ( !d->dns ) {
	d->dns = new QPtrDict<void>( 17 );
	d->dns->setAutoDelete( FALSE );
    }
    d->dns->replace( (void*)dns, (void*)dns );
}


void QDnsDomain::remove( const QDns * dns )
{
#if defined(DEBUG_QDNS)
    qDebug( "removing domain %s (t %d)",
	    dns->label().ascii(), dns->recordType() );
#endif
    QDnsDomain * d = QDnsManager::manager()->domain( dns->label() );
    if ( d->dns )
	d->dns->take( (void*)dns );
}


Q_UINT32 lastSweep;

void QDnsDomain::sweep()
{
    if ( !rrs )
	return;

    QDnsRR * rr;
    rrs->first();
    while( (rr=rrs->current()) != 0 ) {
	if ( !rr->deleteTime )
	    rr->deleteTime = lastSweep; // will hit next time around
	else if ( rr->current == FALSE ||
		  rr->t == QDns::None ||
		  rr->deleteTime < lastSweep ||
		  rr->expireTime < lastSweep )
	    rrs->remove();
	else
	    rrs->next();
    }

    if ( rrs->isEmpty() ) {
	delete rrs;
	rrs = 0;
    }
}




// the itsy-bitsy little socket class I don't really need except for
// so I can subclass and reimplement the slots.


QDnsSocket::QDnsSocket( QObject * parent, const char * name )
    : QObject( parent, name )
{
    // nothing
}


QDnsSocket::~QDnsSocket()
{
    // nothing
}


void QDnsSocket::cleanCache()
{
    // nothing
}


void QDnsSocket::retransmit()
{
    // nothing
}


void QDnsSocket::answer()
{
    // nothing
}




/*!  Constructs a DNS query object with invalid settings both for the
  label and the search type.
*/

QDns::QDns()
{
    d = 0;
    t = None;
}




/*!  Constructs a DNS query object that will search for \a rr
  information about \a label, and starts a query.

  \a rr defaults to \c A, IPv4 addresses.
*/

QDns::QDns( const QString & label, RecordType rr )
{
    d = 0;
    t = rr;
    setLabel( label );
}




/*! Destroys the query object and frees its allocated resources. */

QDns::~QDns()
{
    QDnsDomain::remove( this );
    delete d;
    d = 0;
}




/*!  Sets this query object to query for information about \a label.
  This does not change the recordType(), but its queryStatus() most
  likely changes as a result.
*/

void QDns::setLabel( const QString & label )
{
    l = label;
}


/*! \fn QString QDns::label() const

  Returns the domain name for which this object can query.

  \sa setLabel()
*/

/*! \enum QDns::RecordType

  This enum type defines the record types QDns can handle.  The DNS
  provides many more; these are the ones we've judged to be in current
  use, useful for GUI programs and important enough to support right
  away: <ul>

  <li> \c None - no information.  This exists only so that QDns can
  have a default.

  <li> \c A - IPv4 addresses.  By far the most common type.

  <li> \c Aaaa - Ipv6 addresses.  So far mostly unused.

  <li> \c Mx - Mail eXchanger names.  Used for mail delivery.

  <li> \c Srv - SeRVer names.  Generic record type for finding
  servers.  So far mostly unused.

  <li> \c Cname - canonical name.  Maps from nicknames to the true
  name (the canonical name) for a host.

  <li> \c Ptr - name PoinTeR.  Maps from IPv4 or IPv6 addresses to hostnames.

  <li> \c Txt - arbitrary text for domains.

  </ul>

  We expect that some support for the
  <a href="http://www.dns.net/dnsrd/rfc/rfc2535.html">RFC-2535</a>
  extensions will be added in future versions.
*/

/*!  Sets this object to query for \a rr records. */

void QDns::setRecordType( RecordType rr )
{
    t = rr;
}


/*! \fn QDns::RecordType QDns::recordType() const

  Returns the record type of this query object.

  \sa setRecordType() RecordType
*/


static QDateTime * originOfTime = 0;

static void cleanup()
{
    delete originOfTime;
    originOfTime = 0;
}

static Q_UINT32 now()
{
    if ( originOfTime )
	return originOfTime->secsTo( QDateTime::currentDateTime() );

    originOfTime == new QDateTime( QDateTime::currentDateTime() );
    qAddPostRoutine( cleanup );
    return 0;
}


static Q_UINT16 id; // ### start somewhere random

/*! Starts a DNS query for \a domain, unless QDns happens to have the
  result cached already.

*/

void QDns::sendQuery( const QString & domain ) const
{
#if defined(DEBUG_QDNS)
    qDebug( "QDns::sendQuery (%s)", domain.ascii() );
#endif
    if ( t == None || domain.isNull() )
	return;

    QDnsDomain::add( this, domain );

    QDnsQuery * q = QDnsManager::manager()->query( domain, t );
    if ( q ) {
	// if it exists, we restart the timer.  presumably the query
	// is new again, for some reason.
	q->step = 0;
	// but we don't restart the timeout origin
	return;
    }

    q = new QDnsQuery;
    q->id = ++::id;
    q->t = t;
    q->l = domain;
    q->started = now();

    QDnsManager::manager()->transmitQuery( q );
}


/*! Returns an indication of the query status for this query.  The
query status changes at unpredictable intervals (as may be expected
for a cache).
*/

QDns::Status QDns::queryStatus() const
{
#if defined(DEBUG_QDNS)
	qDebug( "QDns::queryStatus (%s)", l.ascii() );
#endif
    if ( t == None )
	return Passive;

    QDnsManager * m = QDnsManager::manager();
    QDnsDomain * d = m->domain( l );
    if ( d->rrs && d->rrs->first() ) {
	while( d->rrs->current() && d->rrs->current()->t != t )
	    d->rrs->next();
	if ( d->rrs->current() && d->rrs->current()->t == t )
	    return Done;
    }

    int q = 0;
    while ( q < m->queries.size() ) {
	if ( m->queries[q]->t == t && m->queries[q]->l == l )
	    return Active;
	q++;
    }
    sendQuery( l );
    return Passive;
}


/*!  Returns a list of the addresses for this name if this QDns object
  has a recordType() of \a QDns::A or \a QDns::Aaaa and the answer is
  available, or an empty list else.

  As a special case, if label() is a valid numeric IP address, this function
  returns that address.
*/

QValueList<QHostAddress> QDns::addresses() const
{
#if defined(DEBUG_QDNS)
    qDebug( "QDns::addresses (%s)", l.ascii() );
#endif
    QValueList<QHostAddress> result;
    if ( t != A && t != Aaaa )
	return result;

    if ( t == A ) {
	if ( l.lower() == "localhost" ) {
	    // undocumented hack:
	    result.append( QHostAddress( 0x7f000001 ) );
	    return result;
	}

	int maybeIP4 = 0;
	int bytes = 0;
	int byte = 0;
	QString left = l.simplifyWhiteSpace();
	while( left.length() && bytes < 4 ) {
	    QString byteString;
	    int i = bytes < 3 ? left.find( '.' ) : left.length();
	    if ( i < 0 ) {
		left = "";
	    } else {
		QString byteString = left.left( i ).simplifyWhiteSpace();
		left = left.mid( i+1 );
		bool ok = FALSE;
		uint byteValue = byteString.toUInt( &ok );
		if ( ok && byteValue < 256 ) {
		    maybeIP4 = maybeIP4 * 256 + byteValue;
		    bytes++;
		    if ( bytes == 4 && !left.length() ) {
			result.append( QHostAddress( maybeIP4 ) );
			return result;
		    }
		} else {
		    left = "";
		}
	    }
	}
    }

    
#if 0
    int i = l.length()-1;
    bool abbrev = FALSE;
    int maxDots = 2;
    if ( i > 0 && l[i] != '.' ) {
	int dots = 0;
	while( i-- > 0 ) {
	    if ( l[i] == '.' )
		dots++;
	}
	if ( dots < maxDots )
	    abbrev = TRUE;
    }
    if ( abbrev && domains && domains->count() ) {
	// search for each thing in the abbreviated blah first
	QStrListIterator it( *domains );
	const char * dom;
	QDns tmp;
	tmp.setRecordType( t );
	while( (dom=it.current()) != 0 ) {
	    ++it;
	    tmp.setLabel( l + "." + dom + "." );
	    result = tmp.addresses();
	    if ( result.isEmpty() ) {
		QDnsManager::domain( tmp.l )->add( this );
	    } else {
		return result;
	    }
	}
	setLabel( realLabel );
    }
#endif
    
    QList<QDnsRR> * cached = QDnsDomain::cached( l, t );

    (void)cached->first();
    QDnsRR * rr;
    bool noQuery = FALSE;
    while( (rr=cached->current()) != 0 ) {
	if ( rr->current && !rr->nxdomain )
	    result.append( rr->address );
	else if ( rr->nxdomain )
	    noQuery = TRUE;
	cached->next();
    }
    delete cached;
    if ( result.isEmpty() && !noQuery )
	sendQuery( l );
    return result;
}


#if defined(UNIX)

// include this stuff late, so any defines won't hurt.  funkily,
// struct __res_state is part of the api.  normally not used, it says.
// but we use it, to get various information.

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

// if various defines aren't there, we'll set them safely.

#if !defined(MAXNS)
#define MAXNS 1
#endif

static void doResInit( void )
{
    if ( ns )
	return;
    ns = new QList<QHostAddress>;
    ns->setAutoDelete( TRUE );
    domains = new QStrList( TRUE );
    domains->setAutoDelete( TRUE );

    res_init();
    int i;
    // find the name servers to use
    QHostAddress * h;
    for( i=0; i < MAXNS && i < _res.nscount; i++ ) {
	h = new QHostAddress( ntohl( _res.nsaddr_list[i].sin_addr.s_addr ) );
	ns->append( h );
#if defined(DEBUG_QDNS)
	qDebug( "using name server %s", h->ip4AddrString().latin1() );
#endif
    }
    bool hasDefDName = FALSE;
#if defined(MAXDFLSRCH)
    for( i=0; i < MAXDFLSRCH; i++ )
	if ( _res.dnsrch[i] && *(_res.dnsrch[i]) ) {
	    domains->append( QString::fromLatin1( _res.dnsrch[i] ) );
#if defined(DEBUG_QDNS)
	    qDebug( "searching domain %s", _res.dnsrch[i] );
#endif
	    if ( hasDefDName == FALSE &&
		 strcasecmp( _res.dnsrch[i], _res.defdname ) == 0 )
		hasDefDName = TRUE;
	}
#endif
    if ( !hasDefDName && *_res.defdname ) {
	domains->append( QString::fromLatin1( _res.defdname ) );
#if defined(DEBUG_QDNS)
	    qDebug( "searching domain %s (default)", _res.defdname );
#endif
    }
#if defined(SANE_OPERATING_SYSTEM) // not defined
    res_close();
#endif
}

#else

// ######### UGLEHACK!!!!!!! ######### !!!!!!!!!!!! ###############

static void doResInit( void )
{
    if ( ns )
	return;
    ns = new QList<QHostAddress>;
    ns->setAutoDelete( TRUE );
    domains = new QStrList( TRUE );
    domains->setAutoDelete( TRUE );
    h = new QHostAddress( 0xc300fe13 ); // lupinella.troll.no
    ns->append( h );
#if defined(DEBUG_QDNS)
    qDebug( "using name server %s", h->ip4AddrString().latin1() );
#endif
    // \HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters
    // contains three strings we need to read.

    // the ns list should really be set from ...\NameServer and the
    // domains list from ...\SeachList.  if ...\Domain is not in the
    // domains list, it should be appended.

    // but tonight I don't feel up to reading more m$ doc
}

#endif
