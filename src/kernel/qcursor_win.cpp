/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qcursor_win.cpp#44 $
**
** Implementation of QCursor class for Win32
**
** Created : 940219
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit Professional Edition.
**
** Licensees with valid Qt Professional Edition licenses may distribute and
** use this file in accordance with the Qt Professional Edition License
** provided at sale or upon request.
**
** See http://www.troll.no/pricing.html or email sales@troll.no for
** information about the Professional Edition licensing.
**
*****************************************************************************/

#include "qcursor.h"
#include "qbitmap.h"
#include "qapplication.h"
#include "qimage.h"
#include "qdatastream.h"
#include "qt_windows.h"


static const int cursors = 14;
static QCursor cursorTable[cursors];

QT_STATIC_CONST_IMPL QCursor & Qt::arrowCursor = cursorTable[0];
QT_STATIC_CONST_IMPL QCursor & Qt::upArrowCursor = cursorTable[1];
QT_STATIC_CONST_IMPL QCursor & Qt::crossCursor = cursorTable[2];
QT_STATIC_CONST_IMPL QCursor & Qt::waitCursor = cursorTable[3];
QT_STATIC_CONST_IMPL QCursor & Qt::ibeamCursor = cursorTable[4];
QT_STATIC_CONST_IMPL QCursor & Qt::sizeVerCursor = cursorTable[5];
QT_STATIC_CONST_IMPL QCursor & Qt::sizeHorCursor = cursorTable[6];
QT_STATIC_CONST_IMPL QCursor & Qt::sizeBDiagCursor = cursorTable[7];
QT_STATIC_CONST_IMPL QCursor & Qt::sizeFDiagCursor = cursorTable[8];
QT_STATIC_CONST_IMPL QCursor & Qt::sizeAllCursor = cursorTable[9];
QT_STATIC_CONST_IMPL QCursor & Qt::blankCursor = cursorTable[10];
QT_STATIC_CONST_IMPL QCursor & Qt::splitHCursor = cursorTable[11];
QT_STATIC_CONST_IMPL QCursor & Qt::splitVCursor = cursorTable[12];
QT_STATIC_CONST_IMPL QCursor & Qt::pointingHandCursor = cursorTable[13];


/*****************************************************************************
  Internal QCursorData class
 *****************************************************************************/

struct QCursorData : public QShared {
    QCursorData( int s = 0 );
   ~QCursorData();
    int	      cshape;
    QBitmap  *bm, *bmm;
    short     hx, hy;
    HCURSOR   hcurs;
};

QCursorData::QCursorData( int s )
{
    cshape = s;
    hcurs = 0;
    bm = bmm = 0;
    hx = hy  = 0;
}

QCursorData::~QCursorData()
{
    if ( bm || bmm ) {
	delete bm;
	delete bmm;
	if ( hcurs )
	    DestroyCursor( hcurs );
    }
}


/*****************************************************************************
  QCursor member functions
 *****************************************************************************/

QCursor *QCursor::find_cur( int shape )	// find predefined cursor
{
    return (uint)shape <= LastCursor ? &cursorTable[shape] : 0;
}


bool initialized = FALSE;

void QCursor::initialize()
{
    int shape;
    for( shape = 0; shape < cursors; shape++ )
	cursorTable[shape].data = new QCursorData( shape );
    initialized = TRUE;
    qAddPostRoutine( cleanup );
}

void QCursor::cleanup()
{
    int shape;
    for( shape = 0; shape < cursors; shape++ ) {
	delete cursorTable[shape].data;
	cursorTable[shape].data = 0;
    }
    initialized = FALSE;
}

QCursor::QCursor()
{
    if ( !initialized ) {
	if ( qApp->startingUp() ) {
	    data = 0;
	    return;
	}
	initialize();
    }
    QCursor* c = (QCursor *)&Qt::arrowCursor;
    c->data->ref();
    data = c->data;
}

QCursor::QCursor( int shape )			// cursor with shape
{
    if ( !initialized )
	initialize();
    QCursor *c = find_cur( shape );
    if ( !c )					// not found
	c = (QCursor *)&Qt::arrowCursor;	//   then use arrowCursor
    c->data->ref();
    data = c->data;
}

void QCursor::setBitmap( const QBitmap &bitmap, const QBitmap &mask,
			 int hotX, int hotY )
{
    if ( !initialized )
	initialize();
    if ( bitmap.depth() != 1 || mask.depth() != 1 ||
	 bitmap.size() != mask.size() ) {
#if defined(CHECK_NULL)
	warning( "QCursor: Cannot create bitmap cursor; invalid bitmap(s)" );
#endif
	QCursor *c = (QCursor *)&Qt::arrowCursor;
	c->data->ref();
	data = c->data;
	return;
    }
    data = new QCursorData;
    CHECK_PTR( data );
    data->bm  = new QBitmap( bitmap );
    data->bmm = new QBitmap( mask );
    data->hcurs = 0;
    data->cshape = BitmapCursor;
    data->hx = hotX >= 0 ? hotX : bitmap.width()/2;
    data->hy = hotY >= 0 ? hotY : bitmap.height()/2;
}

QCursor::QCursor( const QCursor &c )
{
    if ( !initialized )
	initialize();
    data = c.data;
    data->ref();
}

QCursor::~QCursor()
{
    if ( data && data->deref() )
	delete data;
}


QCursor &QCursor::operator=( const QCursor &c )
{
    if ( !initialized )
	initialize();
    if ( !initialized )
	initialize();
    c.data->ref();				// avoid c = c
    if ( data && data->deref() )
	delete data;
    data = c.data;
    return *this;
}


int QCursor::shape() const			// get cursor shape
{
    if ( !initialized )
	initialize();
    return data->cshape;
}

void QCursor::setShape( int shape )		// set cursor shape
{
    if ( !initialized )
	initialize();
    QCursor *c = find_cur( shape );		// find one of the global ones
    if ( !c )					// not found
	c = (QCursor *)&Qt::arrowCursor;	//   then use arrowCursor
    c->data->ref();
    if ( data->deref() )			// make shallow copy
	delete data;
    data = c->data;
}


const QBitmap *QCursor::bitmap() const
{
    if ( !initialized )
	initialize();
    return data->bm;
}

const QBitmap *QCursor::mask() const
{
    if ( !initialized )
	initialize();
    return data->bmm;
}

QPoint QCursor::hotSpot() const
{
    if ( !initialized )
	initialize();
    return QPoint( data->hx, data->hy );
}


HCURSOR QCursor::handle() const
{
    if ( !initialized )
	initialize();
    if ( !data->hcurs )
	update();
    return data->hcurs;
}


QPoint QCursor::pos()
{
    POINT p;
    GetCursorPos( &p );
    return QPoint( p.x, p.y );
}

void QCursor::setPos( int x, int y )
{
    SetCursorPos( x, y );
}


void QCursor::update() const
{
    if ( !initialized )
	initialize();
    if ( data->hcurs )				// already loaded
	return;

    // Non-standard Windows cursors are created from bitmaps

    static uchar vsplit_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
	0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
	0x00, 0x41, 0x82, 0x00, 0x80, 0x41, 0x82, 0x01, 0xc0, 0x7f, 0xfe, 0x03,
	0x80, 0x41, 0x82, 0x01, 0x00, 0x41, 0x82, 0x00, 0x00, 0x40, 0x02, 0x00,
	0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
	0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static uchar vsplitm_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00,
	0x00, 0xe0, 0x07, 0x00, 0x00, 0xe2, 0x47, 0x00, 0x00, 0xe3, 0xc7, 0x00,
	0x80, 0xe3, 0xc7, 0x01, 0xc0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x07,
	0xc0, 0xff, 0xff, 0x03, 0x80, 0xe3, 0xc7, 0x01, 0x00, 0xe3, 0xc7, 0x00,
	0x00, 0xe2, 0x47, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00,
	0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static uchar hsplit_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x80, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00,
	0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
	0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xff, 0x7f, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x7f, 0x00,
	0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
	0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x00,
	0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static uchar hsplitm_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
	0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00, 0x00, 0xf0, 0x07, 0x00,
	0x00, 0xf8, 0x0f, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
	0x00, 0xc0, 0x01, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00,
	0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00,
	0x80, 0xff, 0xff, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
	0x00, 0xc0, 0x01, 0x00, 0x00, 0xf8, 0x0f, 0x00, 0x00, 0xf0, 0x07, 0x00,
	0x00, 0xe0, 0x03, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    static uchar phand_bits[] = {
	0x00,0x00,0x00,0x00, 0xfe,0x01,0x00,0x00, 0x01,0x02,0x00,0x00,
	0x7e,0x04,0x00,0x00, 0x08,0x08,0x00,0x00, 0x70,0x08,0x00,0x00,
	0x08,0x08,0x00,0x00, 0x70,0x14,0x00,0x00, 0x08,0x22,0x00,0x00,
	0x30,0x41,0x00,0x00, 0xc0,0x20,0x00,0x00, 0x40,0x12,0x00,0x00,
	0x80,0x08,0x00,0x00, 0x00,0x05,0x00,0x00, 0x00,0x02,0x00,0x00,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00
    };
    
    static uchar phandm_bits[] = {
	0xfe,0x01,0x00,0x00, 0xff,0x03,0x00,0x00, 0xff,0x07,0x00,0x00,
	0xff,0x0f,0x00,0x00, 0xfe,0x1f,0x00,0x00, 0xf8,0x1f,0x00,0x00,
	0xfc,0x1f,0x00,0x00, 0xf8,0x3f,0x00,0x00, 0xfc,0x7f,0x00,0x00,
	0xf8,0xff,0x00,0x00, 0xf0,0x7f,0x00,0x00, 0xe0,0x3f,0x00,0x00,
	0xc0,0x1f,0x00,0x00, 0x80,0x0f,0x00,0x00, 0x00,0x07,0x00,0x00,
	0x00,0x02,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00
    };

    static uchar *cursor_bits32[] = {
	vsplit_bits, vsplitm_bits, hsplit_bits, hsplitm_bits,
	phand_bits, phandm_bits
    };


    TCHAR *sh;
    switch ( data->cshape ) {			// map to windows cursor
	case ArrowCursor:
	    sh = IDC_ARROW;
	    break;
	case UpArrowCursor:
	    sh = IDC_UPARROW;
	    break;
	case CrossCursor:
	    sh = IDC_CROSS;
	    break;
	case WaitCursor:
	    sh = IDC_WAIT;
	    break;
	case IbeamCursor:
	    sh = IDC_IBEAM;
	    break;
	case SizeVerCursor:
	    sh = IDC_SIZENS;
	    break;
	case SizeHorCursor:
	    sh = IDC_SIZEWE;
	    break;
	case SizeBDiagCursor:
	    sh = IDC_SIZENESW;
	    break;
	case SizeFDiagCursor:
	    sh = IDC_SIZENWSE;
	    break;
	case SizeAllCursor:
	    sh = IDC_SIZEALL;
	    break;
	case BlankCursor:
	case SplitVCursor:
	case SplitHCursor:
	case PointingHandCursor:
	case BitmapCursor: {
	    QImage bbits, mbits;
	    bool invb, invm;
	    if ( data->cshape == BlankCursor ) {
		bbits.create( 32, 32, 1, 2, QImage::BigEndian );
		bbits.fill( 0 );		// ignore color table
		mbits = bbits.copy();
		data->hx = data->hy = 16;
		invb = invm = FALSE;
	    } else if ( data->cshape != BitmapCursor ) {
		int i = data->cshape - SplitVCursor;
		QBitmap cb( 32, 32, cursor_bits32[i*2], TRUE );
		QBitmap cm( 32, 32, cursor_bits32[i*2+1], TRUE );
		bbits = cb;
		mbits = cb;
		if ( data->cshape == PointingHandCursor )
		    data->hx = data->hy = 0;
		else
		    data->hx = data->hy = 16;
		invb = invm = FALSE;
	    } else {
		bbits = *data->bm;
		mbits = *data->bmm;
		invb = bbits.numColors() > 1 &&
		    qGray(bbits.color(0)) < qGray(bbits.color(1));
		invm = mbits.numColors() > 1 &&
		    qGray(mbits.color(0)) < qGray(mbits.color(1));
	    }
	    int i, n = bbits.numBytes();
	    uchar *bits = bbits.scanLine( 0 );
	    uchar *mask = mbits.scanLine( 0 );
	    for ( i=0; i<n; i++ ) {
		uchar b = bits[i];
		uchar m = mask[i];
		if ( invb )
		    b ^= 0xff;
		if ( invm )
		    m ^= 0xff;
		bits[i] = ~m;
		mask[i] = b ^ m;
	    }
	    data->hcurs = CreateCursor( qWinAppInst(), data->hx, data->hy,
					bbits.width(), bbits.height(),
					bits, mask );
	    return;
	}
	default:
#if defined(CHECK_RANGE)
	    warning( "QCursor::update: Invalid cursor shape %d", data->cshape);
#endif
	    return;
    }
    data->hcurs = LoadCursorA( 0, sh );
}
