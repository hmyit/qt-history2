/****************************************************************************
** $Id: //depot/qt/main/src/tools/qgvector.h#20 $
**
** Definition of QGVector class
**
** Created : 930907
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

#ifndef QGVECTOR_H
#define QGVECTOR_H

#ifndef QT_H
#include "qcollection.h"
#endif // QT_H


class Q_EXPORT QGVector : public QCollection	// generic vector
{
friend class QGList;				// needed by QGList::toVector
public:
    QDataStream &read( QDataStream & );		// read vector from stream
    QDataStream &write( QDataStream & ) const;	// write vector to stream

    virtual int compareItems( GCI, GCI );

protected:
    QGVector();					// create empty vector
    QGVector( uint size );			// create vector with nullptrs
    QGVector( const QGVector &v );		// make copy of other vector
   ~QGVector();

    QGVector &operator=( const QGVector &v );	// assign from other vector

    GCI	 *data()    const	{ return vec; }
    uint  size()    const	{ return len; }
    uint  count()   const	{ return numItems; }

    bool  insert( uint index, GCI );		// insert item at index
    bool  remove( uint index );			// remove item
    GCI	  take( uint index );			// take out item

    void  clear();				// clear vector
    bool  resize( uint newsize );		// resize vector

    bool  fill( GCI, int flen );		// resize and fill vector

    void  sort();				// sort vector
    int	  bsearch( GCI ) const;			// binary search (when sorted)

    int	  findRef( GCI, uint index ) const;	// find exact item in vector
    int	  find( GCI, uint index ) const;	// find equal item in vector
    uint  containsRef( GCI ) const;		// get number of exact matches
    uint  contains( GCI ) const;		// get number of equal matches

    GCI	  at( uint index ) const		// return indexed item
    {
#if defined(CHECK_RANGE)
	if ( index >= len )
	    warningIndexRange( index );
#endif
	return vec[index];
    }

    bool insertExpand( uint index, GCI );	// insert, expand if necessary

    void toList( QGList * ) const;		// put items in list

    virtual QDataStream &read( QDataStream &, GCI & );
    virtual QDataStream &write( QDataStream &, GCI ) const;

private:
    GCI	 *vec;
    uint  len;
    uint  numItems;

    static void warningIndexRange( uint );
};


/*****************************************************************************
  QGVector stream functions
 *****************************************************************************/

Q_EXPORT QDataStream &operator>>( QDataStream &, QGVector & );
Q_EXPORT QDataStream &operator<<( QDataStream &, const QGVector & );


#endif // QGVECTOR_H
