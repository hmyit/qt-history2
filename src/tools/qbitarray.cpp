/****************************************************************************
** $Id: //depot/qt/main/src/tools/qbitarray.cpp#10 $
**
** Implementation of QBitArray class
**
** Author  : Haavard Nord
** Created : 940118
**
** Copyright (C) 1994,1995 by Troll Tech AS.  All rights reserved.
**
** --------------------------------------------------------------------------
** The size of a bit array is stored in the beginning of the actual array,
** which complicates the implementation.  But it still works fine.
*****************************************************************************/

#include "qbitarry.h"
#include "qdstream.h"

#if defined(DEBUG)
static char ident[] = "$Id: //depot/qt/main/src/tools/qbitarray.cpp#10 $";
#endif


#define EXTRA(p) ((bitarr_data*)(p))


QBitArray::QBitArray() : QByteArray( 0, 0 )
{
    p = newData();
    CHECK_PTR( p );
    EXTRA(p)->nbits = 0;
}

QBitArray::QBitArray( uint size ) : QByteArray( 0, 0 )
{
    p = newData();
    CHECK_PTR( p );
    resize( size );
}


void QBitArray::pad0()				// pad last byte with 0-bits
{
    uint sz = size();
    if ( !sz )
	return;
    uchar mask = 1 << (sz%8);
    if ( mask )
	mask--;
    *(data()+sz/8) &= mask;
}


bool QBitArray::resize( uint sz )		// resize bit array
{
    uint s = size();
    if ( !QByteArray::resize( (sz+7)/8 ) )
	return FALSE;				// cannot resize
    EXTRA(p)->nbits = sz;
    if ( sz != 0 ) {				// not null array
	int ds = (int)(sz+7)/8 - (int)(s+7)/8;	// number of bytes difference
	if ( ds > 0 )				// expanding array
	    memset( data() + (s+7)/8, 0, ds );	//   reset new data
    }
    return TRUE;
}


bool QBitArray::fill( bool v, int sz )		// fill bit array with value
{
    if ( sz != -1 ) {				// resize first
	if ( !resize( sz ) )
	    return FALSE;			// cannot resize
    }
    else
	sz = size();
    memset( data(), v ? 0xff : 0, (sz+7)/8 );	// set many bytes, fast
    if ( v )
	pad0();
    return TRUE;
}


void QBitArray::detach() const			// detach bit array
{
    int nbits = EXTRA(p)->nbits;
    this->duplicate( *this );
    EXTRA(p) = nbits;
}

QBitArray QBitArray::copy() const		// get deep copy
{
    QBitArray tmp;
    tmp.duplicate( *this );
    EXTRA(tmp.p)->nbits = EXTRA(p)->nbits;	// copy extra data
    return tmp;
}


bool QBitArray::testBit( uint i ) const		// test if bit set
{
#if defined(CHECK_RANGE)
    if ( i >= size() ) {
	warning( "QBitArray::testBit: Index %d out of range", i );
	return FALSE;
    }
#endif
    return *(data()+(i>>3)) & (1 << (i & 7));
}

void QBitArray::setBit( uint i )		// set bit
{
#if defined(CHECK_RANGE)
    if ( i >= size() ) {
	warning( "QBitArray::setBit: Index %d out of range", i );
	return;
    }
#endif
    *(data()+(i>>3)) |= (1 << (i & 7));
}

void QBitArray::clearBit( uint i )		// clear bit
{
#if defined(CHECK_RANGE)
    if ( i >= size() ) {
	warning( "QBitArray::clearBit: Index %d out of range", i );
	return;
    }
#endif
    *(data()+(i>>3)) &= ~(1 << (i & 7));
}

bool QBitArray::toggleBit( uint i )		// toggle/invert bit
{
#if defined(CHECK_RANGE)
    if ( i >= size() ) {
	warning( "QBitArray::toggleBit: Index %d out of range", i );
	return FALSE;
    }
#endif
    register char *p = data() + (i>>3);
    uchar b = (1 << (i & 7));			// bit position
    uchar c = *p & b;				// read bit
    *p ^= b;					// toggle bit
    return c;
}


QBitArray &QBitArray::operator&=( const QBitArray &a )
{						// AND bits
    if ( size() == a.size() ) {			// must have same length
	register uchar *a1 = (uchar *)data();
	register uchar *a2 = (uchar *)a.data();
	int n = QByteArray::size();
	while ( --n >= 0 )
	    *a1++ &= *a2++;
    }
#if defined(CHECK_RANGE)
    else
	warning( "QBitArray::operator&=: Bit arrays have different size" );
#endif
    return *this;
}

QBitArray &QBitArray::operator|=( const QBitArray &a )
{						// OR bits
    if ( size() == a.size() ) {			// must have same length
	register uchar *a1 = (uchar *)data();
	register uchar *a2 = (uchar *)a.data();
	int n = QByteArray::size();
	while ( --n >= 0 )
	    *a1++ |= *a2++;
    }
#if defined(CHECK_RANGE)
    else
	warning( "QBitArray::operator|=: Bit arrays have different size" );
#endif
    return *this;
}

QBitArray &QBitArray::operator^=( const QBitArray &a )
{						// XOR bits
    if ( size() == a.size() ) {			// must have same length
	register uchar *a1 = (uchar *)data();
	register uchar *a2 = (uchar *)a.data();
	int n = QByteArray::size();
	while ( --n >= 0 )
	    *a1++ ^= *a2++;
    }
#if defined(CHECK_RANGE)
    else
	warning( "QBitArray::operator^=: Bit arrays have different size" );
#endif
    return *this;
}

QBitArray QBitArray::operator~() const		// NOT bits
{
    QBitArray a( size() );
    register uchar *a1 = (uchar *)data();
    register uchar *a2 = (uchar *)a.data();
    int n = QByteArray::size();
    while ( n-- )
	*a2++ = ~*a1++;
    a.pad0();
    return a;
}


QBitArray operator&( const QBitArray &a1, const QBitArray &a2 )
{
    QBitArray tmp = a1.copy();
    tmp &= a2;
    return tmp;
}

QBitArray operator|( const QBitArray &a1, const QBitArray &a2 )
{
    QBitArray tmp = a1.copy();
    tmp |= a2;
    return tmp;
}

QBitArray operator^( const QBitArray &a1, const QBitArray &a2 )
{
    QBitArray tmp = a1.copy();
    tmp ^= a2;
    return tmp;
}


// --------------------------------------------------------------------------
// QBitArray stream functions
//

QDataStream &operator<<( QDataStream &s, const QBitArray &a )
{
    UINT32 len = a.size();
    s << len;					// write size of array
    if ( len > 0 )				// write data
	s.writeRawBytes( a.data(), a.QByteArray::size() );
    return s;
}

QDataStream &operator>>( QDataStream &s, QBitArray &a )
{
    UINT32 len;
    s >> len;					// read size of array
    if ( !a.resize( (uint)len ) ) {		// resize array
#if defined(CHECK_NULL)
	warning( "QDataStream: Not enough memory to read QBitArray" );
#endif
	len = 0;
    }
    if ( len > 0 )				// read data
	s.readRawBytes( a.data(), a.QByteArray::size() );
    return s;
}
