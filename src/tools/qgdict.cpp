/****************************************************************************
** $Id: //depot/qt/main/src/tools/qgdict.cpp#73 $
**
** Implementation of QGDict and QGDictIterator classes
**
** Created : 920529
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees with valid Qt Professional Edition licenses may distribute and
** use this file in accordance with the Qt Professional Edition License
** provided at sale or upon request.
**
** See http://www.troll.no/pricing.html or email sales@troll.no for
** information about the Professional Edition licensing, or see
** http://www.troll.no/qpl/ for QPL licensing information.
**
*****************************************************************************/

#include "qgdict.h"
#include "qlist.h"
#include "qstring.h"
#include "qdatastream.h"
#include <ctype.h>


/*!
  \class QGDict qgdict.h
  \brief The QGDict class is an internal class for implementing QDict and QIntDict.

  QGDict is a strictly internal class that acts as a base class for the
  \link collection.html collection classes\endlink QDict and QIntDict.

  QGDict has some virtual functions that can be reimplemented to customize
  the subclasses.
  <ul>
  <li> hashKey() implements the hashing function for the dictionary.
  <li> read() reads a collection/dictionary item from a QDataStream.
  <li> write() writes a collection/dictionary item to a QDataStream.
  </ul>
  Normally, you do not have to reimplement any of these functions.
*/

static const int op_find = 0;
static const int op_insert = 1;
static const int op_replace = 2;


class QGDItList : public QList<QGDictIterator>
{
public:
    QGDItList() : QList<QGDictIterator>() {}
    QGDItList( const QGDItList &list ) : QList<QGDictIterator>(list) {}
   ~QGDItList() { clear(); }
    QGDItList &operator=(const QGDItList &list)
	{ return (QGDItList&)QList<QGDictIterator>::operator=(list); }
};


/*****************************************************************************
  Default implementation of virtual functions
 *****************************************************************************/

/*!
  Returns the hash key for \e key, when key is a string.
*/

int QGDict::hashKey( const char *key )
{
#if defined(CHECK_NULL)
    if ( key == 0 )
	warning( "QGDict::hash: Invalid null key" );
#endif
    register const char *k = key;
    register uint h=0;
    uint g;
    if ( cases ) {				// case sensitive
	while ( *k ) {
	    h = (h<<4) + *k++;
	    if ( (g = h & 0xf0000000) )
		h ^= g >> 24;
	    h &= ~g;
	}
    } else {					// case insensitive
	while ( *k ) {
	    h = (h<<4) + tolower(*k);
	    if ( (g = h & 0xf0000000) )
		h ^= g >> 24;
	    h &= ~g;
	    k++;
	}
    }
    int index = h;
    if ( index < 0 )				// adjust index to table size
	index = -index;
    return index;
}


/*!
  Reads a collection/dictionary item from the stream \e s and returns a
  reference to the stream.

  The default implementation sets \e item to 0.

  \sa write()
*/

QDataStream& QGDict::read( QDataStream &s, Item &item )
{
    item = 0;
    return s;
}

/*!
  Writes a collection/dictionary item to the stream \e s and returns a
  reference to the stream.

  The default implementation does nothing.

  \sa read()
*/

QDataStream& QGDict::write( QDataStream &s, Item ) const
{
    return s;
}


/*****************************************************************************
  QGDict member functions
 *****************************************************************************/

/*!
  \internal
  Constructs a dictionary.
*/

QGDict::QGDict( uint len, bool cs, bool ck, bool th )
{
    init( len );
    cases = cs;
    copyk = ck;
    triv = th;
    if ( triv )					// copyk must be FALSE for
	copyk = FALSE;				//   int-hashed dicts
}

void QGDict::init( uint len )
{
    vec = new QBucketPrivate *[vlen = len];		// allocate hash table
    CHECK_PTR( vec );
    memset( (char*)vec, 0, vlen*sizeof(QBucketPrivate*) );
    numItems = 0;
    iterators = 0;

    // Be careful not to break resize() if you add something.
}

/*!
  \internal
  Constructs a copy of \e dict.
*/

QGDict::QGDict( const QGDict & dict )
    : QCollection( dict )
{
    init( dict.vlen );
    cases = dict.cases;
    copyk = dict.copyk;
    triv  = dict.triv;
    QGDictIterator it( dict );
    while ( it.get() ) {			// copy from other dict
	look( it.getKey(), it.get(), op_insert );
	++it;
    }
}

/*!
  \internal
  Removes all items from the dictionary and destroys it.
*/

QGDict::~QGDict()
{
    clear();					// delete everything
    delete [] vec;
    if ( !iterators )				// no iterators for this dict
	return;
    register QGDictIterator *i = iterators->first();
    while ( i ) {				// notify all iterators that
	i->dict = 0;				// this dict is deleted
	i = iterators->next();
    }
    delete iterators;
}


/*!
  \internal
  Assigns \e dict to this dictionary.
*/

QGDict &QGDict::operator=( const QGDict &dict )
{
    clear();
    QGDictIterator it( dict );
    while ( it.get() ) {			// copy from other dict
	look( it.getKey(), it.get(), op_insert );
	++it;
    }
    return *this;
}


/*!
  \fn uint QGDict::count() const
  \internal
  Returns the number of items in the dictionary.
*/

/*!
  \fn uint QGDict::size() const
  \internal
  Returns the size of the hash array.
*/


/*!
  \internal
  The do-it-all function; op is one of op_find, op_insert, op_replace
*/

QCollection::Item QGDict::look( const char *key, Item d, int op )
{
    register QBucketPrivate *n;
    int	 index;
    if ( triv ) {				// key is a long/ptr
	index = (int)((unsigned long)(key) % vlen);	// simple hash
	if ( op == op_find ) {			// find
	    for ( n=vec[index]; n; n=n->getNext() ) {
		if ( n->getKey() == key )
		    return n->getData();	// item found
	    }
	    return 0;				// not such item
	}
    } else {					// key is a string
	index = hashKey( key ) % vlen;
	if ( op == op_find ) {			// find
	    for ( n=vec[index]; n; n=n->getNext() ) {
		if ( (cases ? strcmp(n->getKey(),key)
			 : stricmp(n->getKey(),key)) == 0 )
		    return n->getData();	// item found
	    }
	    return 0;				// did not find the item
	}
    }
    if ( op == op_replace ) {			// replace
	if ( vec[index] != 0 )			// maybe something there
	    remove( key );
    }
    QBucketPrivate *node = new QBucketPrivate;		// insert new node
    CHECK_PTR( node );
    if ( !node )				// no memory
	return 0;
    node->setKey( (char *)(copyk ? qstrdup(key) : key) );
    node->setData( newItem(d) );
#if defined(CHECK_NULL)
    if ( node->getData() == 0 )
	warning( "QGDict::look: Attempt to insert null item" );
#endif
    node->setNext( vec[index] );		// link node into table
    vec[index] = node;
    numItems++;
    return node->getData();
}

/*!
  \internal
  Changes the size of the hashtable.
  The contents of the dictionary are preserved,
  but all iterators on the dictionary become invalid.
*/
void QGDict::resize( uint newsize )
{
    // Save old information
    QBucketPrivate   **old_vec = vec;
    uint	old_vlen = vlen;
    QGDItList  *old_iterators = iterators;
    bool	old_copyk = copyk;

    init( newsize );
    copyk = FALSE;

    // Reinsert every item from vec, deleting vec as we go
    for ( uint index = 0; index < old_vlen; index++ ) {
	QBucketPrivate *n=old_vec[index];
	while ( n ) {
	    look( n->getKey(), n->getData(), op_insert );
	    QBucketPrivate *t=n->getNext();
	    delete n;
	    n = t;
	}
    }
    delete [] old_vec;

    // Restore state
    iterators = old_iterators;
    copyk = old_copyk;

    // `Invalidate' all iterators, since order is lost
    if ( iterators ) {			// update iterators
	register QGDictIterator *i = iterators->first();
	while ( i ) {			// fix all iterators
	    i->toFirst();
	    i = iterators->next();
	}
    }
}

/*!
  \internal
  Unlinks the bucket with the specified key (and specifed
  data pointer, if it is set).
*/

QBucketPrivate *QGDict::unlink( const char *key, Item d )
{
    if ( numItems == 0 )			// nothing in dictionary
	return 0;
    register QBucketPrivate *n;
    QBucketPrivate *prev = 0;
    int index;
    if ( triv )
	index = (int)(long(key) % vlen);
    else
	index = hashKey( key ) % vlen;
    for ( n=vec[index]; n; n=n->getNext() ) {	// find item in list
	bool equal;
	if ( triv )
	    equal = n->getKey() == key;
	else
	    equal = (cases ? strcmp(n->getKey(),key)
			   : stricmp(n->getKey(),key)) == 0;
	if ( equal && d )
	    equal = (n->getData() == d);
	if ( equal ) {				// found node to be removed
	    if ( iterators ) {			// update iterators
		register QGDictIterator *i = iterators->first();
		while ( i ) {			// fix all iterators that
		    if ( i->curNode == n )	// refers to pending node
			i->operator++();
		    i = iterators->next();
		}
	    }
	    if ( prev )				// unlink node
		prev->setNext( n->getNext() );
	    else
		vec[index] = n->getNext();
	    numItems--;
	    return n;
	}
	prev = n;
    }
    return 0;
}

/*!
  \internal
  Removes the item with the specified key.
*/

bool QGDict::remove( const char *key )
{
    register QBucketPrivate *n = unlink( key );
    if ( n ) {
	if ( copyk )
	    delete [] n->getKey();
	deleteItem( n->getData() );
	delete n;				// delete bucket
    }
    return n != 0;
}

/*!
  \internal
  Removes the item with the specified key and with data \a item .
  Use to remove a specific item when several items have the same key.
*/

bool QGDict::removeItem( const char *key, Item item )
{
    register QBucketPrivate *n = unlink( key, item );
    if ( n ) {
	if ( copyk )
	    delete [] n->getKey();
	deleteItem( n->getData() );
	delete n;				// delete bucket
    }
    return n != 0;
}

/*!
  \internal
  Takes out the item with the specified key.
*/

QCollection::Item QGDict::take( const char *key )
{
    register QBucketPrivate *n = unlink( key );
    Item tmp;
    if ( n ) {
	tmp = n->getData();
	if ( copyk )
	    delete [] n->getKey();
	delete n;
    } else {
	tmp = 0;
    }
    return tmp;
}


/*!
  \internal
  Removes all items from the dictionary.
*/

void QGDict::clear()
{
    if ( !numItems )
	return;
    register QBucketPrivate *n;
    numItems = 0;				// disable remove() function
    for ( uint j=0; j<vlen; j++ ) {		// destroy hash table
	if ( vec[j] ) {
	    n = vec[j];
	    vec[j] = 0;				// detach list of buckets
	    while ( n ) {
		if ( copyk )
		    delete [] n->getKey();
		QBucketPrivate *next = n->getNext();
		deleteItem( n->getData() );
		delete n;
		n = next;
	    }
	}
    }
    if ( !iterators )				// no iterators for this dict
	return;
    register QGDictIterator *i = iterators->first();
    while ( i ) {				// notify all iterators that
	i->curNode = 0;				// this dict is empty
	i = iterators->next();
    }
}


/*!
  \internal
  Outputs debug statistics.
*/

void QGDict::statistics() const
{
#if defined(DEBUG)
    QString line;
    line.fill( '-', 60 );
    double real, ideal;
    debug( line );
    debug( "DICTIONARY STATISTICS:" );
    if ( count() == 0 ) {
	debug( "Empty!" );
	debug( line );
	return;
    }
    real = 0.0;
    ideal = (float)count()/(2.0*size())*(count()+2.0*size()-1);
    uint i = 0;
    while ( i<size() ) {
	QBucketPrivate *n = vec[i];
	int b = 0;
	while ( n ) {				// count number of buckets
	    b++;
	    n = n->getNext();
	}
	real = real + (double)b * ((double)b+1.0)/2.0;
	char buf[80], *pbuf;
	if ( b > 78 )
	    b = 78;
	pbuf = buf;
	while ( b-- )
	    *pbuf++ = '*';
	*pbuf = '\0';
	debug( buf );
	i++;
    }
    debug( "Array size = %d", size() );
    debug( "# items    = %d", count() );
    debug( "Real dist  = %g", real );
    debug( "Rand dist  = %g", ideal );
    debug( "Real/Rand  = %g", real/ideal );
    debug( line );
#endif // DEBUG
}


/*****************************************************************************
  QGDict stream functions
 *****************************************************************************/

QDataStream &operator>>( QDataStream &s, QGDict &dict )
{
    return dict.read( s );
}

QDataStream &operator<<( QDataStream &s, const QGDict &dict )
{
    return dict.write( s );
}

#if defined(_CC_DEC_) && defined(__alpha) && (__DECCXX_VER >= 50190001)
#pragma message disable narrowptr
#endif

/*!
  \internal
  Reads a dictionary from the stream \e s.
*/

QDataStream &QGDict::read( QDataStream &s )
{
    uint num;
    s >> num;					// read number of items
    clear();					// clear dict
    while ( num-- ) {				// read all items
	Item d;
	char *k;
	if ( triv ) {
	    Q_UINT32 k_triv;
	    s >> k_triv;			// key is 32-bit int
	    k = (char *)k_triv;
	} else {
	    s >> k;				// key is string
	}
	read( s, d );				// read data
	look( k, d, op_insert );
    }
    return s;
}

/*!
  \internal
  Writes the dictionary to the stream \e s.
*/

QDataStream& QGDict::write( QDataStream &s ) const
{
    s << count();				// write number of items
    uint i = 0;
    while ( i<size() ) {
	QBucketPrivate *n = vec[i];
	while ( n ) {				// write all buckets
	    if ( triv )
		s << (Q_UINT32)(long)n->getKey(); // write key as 32-bit int
	    else
		s << n->getKey();		// write key as string
	    write( s, n->getData() );		// write data
	    n = n->getNext();
	}
	i++;
    }
    return s;
}


/*****************************************************************************
  QGDictIterator member functions
 *****************************************************************************/

/*!
  \class QGDictIterator qgdict.h
  \brief An internal class for implementing QDictIterator and QIntDictIterator.

  QGDictIterator is a strictly internal class that does the heavy work for
  QDictIterator and QIntDictIterator.
*/

/*!
  \internal
  Constructs an iterator that operates on the dictionary \e d.
*/

QGDictIterator::QGDictIterator( const QGDict &d )
{
    dict = (QGDict *)&d;			// get reference to dict
    toFirst();					// set to first noe
    if ( !dict->iterators ) {
	dict->iterators = new QGDItList;	// create iterator list
	CHECK_PTR( dict->iterators );
    }
    dict->iterators->append( this );		// attach iterator to dict
}

/*!
  \internal
  Constructs a copy of the iterator \e it.
*/

QGDictIterator::QGDictIterator( const QGDictIterator &it )
{
    dict = it.dict;
    curNode = it.curNode;
    curIndex = it.curIndex;
    if ( dict )
	dict->iterators->append( this );	// attach iterator to dict
}

/*!
  \internal
  Assigns a copy of the iterator \e it and returns a reference to this
  iterator.
*/

QGDictIterator &QGDictIterator::operator=( const QGDictIterator &it )
{
    if ( dict ) {				// detach from old dict
	if ( dict->iterators->removeRef(this) ){
	    if ( dict->iterators->count() == 0 ) {
		delete dict->iterators;		// this was the last iterator
		dict->iterators = 0;
	    }
	}
    }
    dict = it.dict;
    curNode = it.curNode;
    curIndex = it.curIndex;
    if ( dict )
	dict->iterators->append( this );	// attach to new list
    return *this;
}

/*!
  \internal
  Destroys the iterator.
*/

QGDictIterator::~QGDictIterator()
{
    if ( dict ) {				// detach iterator from dict
#if defined(DEBUG)
	ASSERT( dict->iterators );
#endif
	if ( dict->iterators->removeRef(this) ) {
	    if ( dict->iterators->count() == 0 ) {
		delete dict->iterators;		// this was the last iterator
		dict->iterators = 0;
	    }
	}
    }
}


/*!
  \internal
  Sets the iterator to point to the first item in the dictionary.
*/

QCollection::Item QGDictIterator::toFirst()
{
    if ( !dict ) {
#if defined(CHECK_NULL)
	warning( "QGDictIterator::toFirst: Dictionary has been deleted" );
#endif
	return 0;
    }
    if ( dict->count() == 0 ) {			// empty dictionary
	curNode = 0;
	return 0;
    }
    register uint i = 0;
    register QBucketPrivate **v = dict->vec;
    while ( !(*v++) )
	i++;
    curNode = dict->vec[i];
    curIndex = i;
    return curNode->getData();
}


/*!
  \internal
  Moves to the next item (postfix).
*/

QCollection::Item QGDictIterator::operator()()
{
    if ( !dict ) {
#if defined(CHECK_NULL)
	warning( "QGDictIterator::operator(): Dictionary has been deleted" );
#endif
	return 0;
    }
    if ( !curNode )
	return 0;
    QCollection::Item d = curNode->getData();
    this->operator++();
    return d;
}

/*!
  \internal
  Moves to the next item (prefix).
*/

QCollection::Item QGDictIterator::operator++()
{
    if ( !dict ) {
#if defined(CHECK_NULL)
	warning( "QGDictIterator::operator++: Dictionary has been deleted" );
#endif
	return 0;
    }
    if ( !curNode )
	return 0;
    curNode = curNode->getNext();
    if ( !curNode ) {				// no next bucket
	register uint i = curIndex + 1;		// look from next vec element
	register QBucketPrivate **v = &dict->vec[i];
	while ( i < dict->size() && !(*v++) )
	    i++;
	if ( i == dict->size() ) {		// nothing found
	    curNode = 0;
	    return 0;
	}
	curNode = dict->vec[i];
	curIndex = i;
    }
    return curNode->getData();
}

/*!
  \internal
  Moves \e jumps positions forward.
*/

QCollection::Item QGDictIterator::operator+=( uint jumps )
{
    while ( curNode && jumps-- )
	operator++();
    return curNode ? curNode->getData() : 0;
}


/*!
  \internal
  QString version of remove.
*/
bool QGDict::remove( const QString& key )
{
    QCString kutf8 = key.utf8();
    return remove( kutf8.data() );
}

/*!
  \internal
  QString version of removeItem.
*/
bool QGDict::removeItem( const QString& key, Item item )
{
    QCString kutf8 = key.utf8();
    return removeItem( kutf8.data(), item );
}

/*!
  \internal
  QString version of take.
*/
QCollection::Item QGDict::take( const QString& key )
{
    QCString kutf8 = key.utf8();
    return take( kutf8.data() );
}

/*!
  \internal
  QString version of look.
*/
QCollection::Item QGDict::look( const QString& key, Item g, int op )
{
    QCString kutf8 = key.utf8();
    if ( op == op_find ) {
	return look( kutf8.data(), g, op );
    } else {
	if ( !copyk ) {
	    fatal("QGDict: attempted to insert QString without copying key");
	    return 0;
	} else {
	    return look( kutf8.data(), g, op );
	}
    }
}

/*!
  Returns the current key.
*/
QString QGDictIterator::getKey() const
{
    if ( curNode ) {
	return QString::fromUtf8(curNode->getKey());
    } else {
	return QString::null;
    }
}
