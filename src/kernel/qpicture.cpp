/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qpicture.cpp#69 $
**
** Implementation of QPicture class
**
** Created : 940802
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

#include "qpicture.h"

#ifndef QT_NO_PICTURE

#include "qpainter.h"
#include "qpixmap.h"
#include "qimage.h"
#include "qfile.h"
#include "qdatastream.h"
#include "qpaintdevicemetrics.h"


/*!
  \class QPicture qpicture.h
  \brief The QPicture class is a paint device that records and replays QPainter
  commands.

  \ingroup drawing

  A picture serializes painter commands to an IO device in a
  platform-independent format.	A picture created under Windows
  can be read on a Sun SPARC.

  Pictures are called meta-files on some platforms.

  Qt pictures use a proprietary binary format.	Unlike native picture
  (meta-file) formats on many window systems, Qt pictures have no
  limitations regarding the contents.  Everything that can be painted can
  also be stored in a picture (fonts, pixmaps, regions, transformed
  graphics, etc.).

  QPicture is an \link shclass.html implicitely shared\endlink class.

  Example of how to record a picture:
  \code
    QPicture  pic;
    QPainter  p;
    p.begin( &pic );				// paint in picture
    p.drawEllipse( 10,20, 80,70 );		// draw an ellipse
    p.end();					// painting done
    pic.save( "drawing.pic" );			// save picture
  \endcode

  Example of how to replay a picture:
  \code
    QPicture  pic;
    pic.load( "drawing.pic" );			// load picture
    QPainter  p;
    p.begin( &myWidget );			// paint in myWidget
    p.drawPicture( pic );			// draw the picture
    p.end();					// painting done
  \endcode

*/


static const char  *mfhdr_tag = "QPIC";		// header tag
static const Q_UINT16 mfhdr_maj = 4;		// major version #
static const Q_UINT16 mfhdr_min = 0;		// minor version #


/*!
  Constructs an empty picture.

  The \a formatVersion parameter may be used to create a QPicture that can
  be read by applications that are compiled with earlier versions of Qt.
  Currently supported is \a formatVersion == 1, which will make a QPicture
  stored in a format that is binary compatible with Qt 1.x, \a
  formatVersion == 2 for a format compatible with Qt 2.0.x programs and \a
  formatVersion == 3 for Qt 2.1.x and later.

  Reading of pictures generated by earlier versions of Qt is supported and
  needs no special coding; the format is automatically detected.
*/

QPicture::QPicture( int formatVersion )
    : QPaintDevice( QInternal::Picture | QInternal::ExternalDevice )
    // set device type
{
    d = new QPicturePrivate;

    if ( formatVersion != (int)mfhdr_maj ) {
	d->formatMajor = formatVersion;
	d->formatMinor = 0;
	d->formatOk = FALSE;
    }
    else {
	d->resetFormat();
    }
}

/*!
  Constructs a
  \link shclass.html shallow copy\endlink of \a pic.
*/

QPicture::QPicture( const QPicture &pic )
    : QPaintDevice( QInternal::Picture | QInternal::ExternalDevice )
{
    d = pic.d;
    d->ref();
}

/*!
  Destructs the picture.
*/
QPicture::~QPicture()
{
    if ( d->deref() )
	delete d;
}


/*!
  \fn bool QPicture::isNull() const
  Returns TRUE if the picture contains no data, otherwise FALSE.
*/

/*!
  \fn uint QPicture::size() const
  Returns the size of the picture data.
  \sa data()
*/

/*!
  \fn const char* QPicture::data() const
  Returns a pointer to the picture data. It is only valid until the next
  non const function is called on this picture. The returned pointer is null
  if the picture contains no data.
  \sa size(), isNull()
*/

/*!
  \fn QRect QPicture::boundingRect() const
  Returns the bounding rectangle of the picture or an invalid rectangle if the
  picture contains no data.
*/

/*!
  Sets the picture data directly from \a data and \a size. This function
  copies the input data.
  \sa data(), size()
*/

void QPicture::setData( const char* data, uint size )
{
    detach();
    QByteArray a( size );
    memcpy( a.data(), data, size );
    d->pictb.setBuffer( a );			// set byte array in buffer
    d->resetFormat();				// we'll have to check
}


/*!
  Loads a picture from the file specified by \e fileName and returns TRUE
  if successful, otherwise FALSE.

  \sa save()
*/

bool QPicture::load( const QString &fileName )
{
    detach();
    QByteArray a;
    QFile f( fileName );
    if ( !f.open(IO_ReadOnly) )
	return FALSE;
    a.resize( (uint)f.size() );
    f.readBlock( a.data(), (uint)f.size() );	// read file into byte array
    f.close();
    d->pictb.setBuffer( a );			// set byte array in buffer
    return d->checkFormat();
}

/*!
  Saves a picture to the file specified by \e fileName and returns TRUE
  if successful, otherwise FALSE.

  \sa load()
*/

bool QPicture::save( const QString &fileName )
{
    QFile f( fileName );
    if ( !f.open( IO_WriteOnly ) )
	return FALSE;
    f.writeBlock( d->pictb.buffer().data(), d->pictb.buffer().size() );
    f.close();
    return TRUE;
}


/*!
  Replays the picture using \e painter, and returns TRUE if successful or
  FALSE if the internal picture data is inconsistent.

  This function does exactly the same as QPainter::drawPicture() with
  (\a x, \a y) = (0, 0).
*/

bool QPicture::play( QPainter *painter )
{
    if ( d->pictb.size() == 0 )			// nothing recorded
	return TRUE;

    if ( !d->formatOk && !d->checkFormat() )
	return FALSE;

    d->pictb.open( IO_ReadOnly );		// open buffer device
    QDataStream s;
    s.setDevice( &d->pictb );			// attach data stream to buffer
    s.device()->at( 10 );			// go directly to the data
    s.setVersion( d->formatMajor );

    Q_UINT8  c, clen;
    Q_UINT32 nrecords;
    s >> c >> clen;
    Q_ASSERT( c == PdcBegin );
    if ( !( d->formatMajor >= 1 && d->formatMajor <= 3 )) {
	Q_INT32 dummy;
	s >> dummy >> dummy >> dummy >> dummy;
    }
    s >> nrecords;
    if ( !exec( painter, s, nrecords ) ) {
#if defined(QT_CHECK_RANGE)
	qWarning( "QPicture::play: Format error" );
#endif
	d->pictb.close();
	return FALSE;
    }
    d->pictb.close();
    return TRUE;				// no end-command
}


/*!
  \internal
  Iterates over the internal picture data and draws the picture using
  \e painter.
*/

bool QPicture::exec( QPainter *painter, QDataStream &s, int nrecords )
{
#if defined(QT_DEBUG)
    int		strm_pos;
#endif
    Q_UINT8	c;				// command id
    Q_UINT8	tiny_len;			// 8-bit length descriptor
    Q_INT32	len;				// 32-bit length descriptor
    Q_INT16	i_16, i1_16, i2_16;		// parameters...
    Q_INT8	i_8;
    Q_UINT32	ul;
    QCString	str1;
    QString	str;
    QPoint	p, p1, p2;
    QRect	r;
    QPointArray a;
    QColor	color;
    QFont	font;
    QPen	pen;
    QBrush	brush;
    QRegion	rgn;
#ifndef QT_NO_TRANSFORMATIONS
    QWMatrix	matrix;
#endif

    while ( nrecords-- && !s.eof() ) {
	s >> c;					// read cmd
	s >> tiny_len;				// read param length
	if ( tiny_len == 255 )			// longer than 254 bytes
	    s >> len;
	else
	    len = tiny_len;
#if defined(QT_DEBUG)
	strm_pos = s.device()->at();
#endif
	switch ( c ) {				// exec cmd
	    case PdcNOP:
		break;
	    case PdcDrawPoint:
		s >> p;
		painter->drawPoint( p );
		break;
	    case PdcMoveTo:
		s >> p;
		painter->moveTo( p );
		break;
	    case PdcLineTo:
		s >> p;
		painter->lineTo( p );
		break;
	    case PdcDrawLine:
		s >> p1 >> p2;
		painter->drawLine( p1, p2 );
		break;
	    case PdcDrawRect:
		s >> r;
		painter->drawRect( r );
		break;
	    case PdcDrawRoundRect:
		s >> r >> i1_16 >> i2_16;
		painter->drawRoundRect( r, i1_16, i2_16 );
		break;
	    case PdcDrawEllipse:
		s >> r;
		painter->drawEllipse( r );
		break;
	    case PdcDrawArc:
		s >> r >> i1_16 >> i2_16;
		painter->drawArc( r, i1_16, i2_16 );
		break;
	    case PdcDrawPie:
		s >> r >> i1_16 >> i2_16;
		painter->drawPie( r, i1_16, i2_16 );
		break;
	    case PdcDrawChord:
		s >> r >> i1_16 >> i2_16;
		painter->drawChord( r, i1_16, i2_16 );
		break;
	    case PdcDrawLineSegments:
		s >> a;
		painter->drawLineSegments( a );
		break;
	    case PdcDrawPolyline:
		s >> a;
		painter->drawPolyline( a );
		break;
	    case PdcDrawPolygon:
		s >> a >> i_8;
		painter->drawPolygon( a, i_8 );
		break;
	    case PdcDrawCubicBezier:
		s >> a;
		painter->drawCubicBezier( a );
		break;
	    case PdcDrawText:
		s >> p >> str1;
		painter->drawText( p, str1 );
		break;
	    case PdcDrawTextFormatted:
		s >> r >> i_16 >> str1;
		painter->drawText( r, i_16, str1 );
		break;
	    case PdcDrawText2:
		s >> p >> str;
		painter->drawText( p, str );
		break;
	    case PdcDrawText2Formatted:
		s >> r >> i_16 >> str;
		painter->drawText( r, i_16, str );
		break;
	    case PdcDrawPixmap: {
		QPixmap pixmap;
		s >> p >> pixmap;
		painter->drawPixmap( p, pixmap );
		}
		break;
	    case PdcDrawImage: {
		QImage image;
		s >> p >> image;
		painter->drawImage( p, image );
		}
		break;
	    case PdcBegin:
		s >> ul;			// number of records
		if ( !exec( painter, s, ul ) )
		    return FALSE;
		break;
	    case PdcEnd:
		if ( nrecords == 0 )
		    return TRUE;
		break;
	    case PdcSave:
		painter->save();
		break;
	    case PdcRestore:
		painter->restore();
		break;
	    case PdcSetBkColor:
		s >> color;
		painter->setBackgroundColor( color );
		break;
	    case PdcSetBkMode:
		s >> i_8;
		painter->setBackgroundMode( (Qt::BGMode)i_8 );
		break;
	    case PdcSetROP:
		s >> i_8;
		painter->setRasterOp( (Qt::RasterOp)i_8 );
		break;
	    case PdcSetBrushOrigin:
		s >> p;
		painter->setBrushOrigin( p );
		break;
	    case PdcSetFont:
		s >> font;
		painter->setFont( font );
		break;
	    case PdcSetPen:
		s >> pen;
		painter->setPen( pen );
		break;
	    case PdcSetBrush:
		s >> brush;
		painter->setBrush( brush );
		break;
	    case PdcSetTabStops:
		s >> i_16;
		painter->setTabStops( i_16 );
		break;
	    case PdcSetTabArray:
		s >> i_16;
		if ( i_16 == 0 ) {
		    painter->setTabArray( 0 );
		} else {
		    int *ta = new int[i_16];
		    Q_CHECK_PTR( ta );
		    for ( int i=0; i<i_16; i++ ) {
			s >> i1_16;
			ta[i] = i1_16;
		    }
		    painter->setTabArray( ta );
		    delete [] ta;
		}
		break;
	    case PdcSetVXform:
		s >> i_8;
#ifndef QT_NO_TRANSFORMATIONS
		painter->setViewXForm( i_8 );
#endif
		break;
	    case PdcSetWindow:
		s >> r;
#ifndef QT_NO_TRANSFORMATIONS
		painter->setWindow( r );
#endif
		break;
	    case PdcSetViewport:
		s >> r;
#ifndef QT_NO_TRANSFORMATIONS
		painter->setViewport( r );
#endif
		break;
	    case PdcSetWXform:
		s >> i_8;
#ifndef QT_NO_TRANSFORMATIONS
		painter->setWorldXForm( i_8 );
#endif
		break;
	    case PdcSetWMatrix:
#ifndef QT_NO_TRANSFORMATIONS	// #### fix me!
		s >> matrix >> i_8;
		painter->setWorldMatrix( matrix, i_8 );
#endif
		break;
#ifndef QT_NO_TRANSFORMATIONS
	    case PdcSaveWMatrix:
		painter->saveWorldMatrix();
		break;
	    case PdcRestoreWMatrix:
		painter->restoreWorldMatrix();
		break;
#endif
	    case PdcSetClip:
		s >> i_8;
		painter->setClipping( i_8 );
		break;
	    case PdcSetClipRegion:
		s >> rgn;
		painter->setClipRegion( rgn );
		break;
	    default:
#if defined(QT_CHECK_RANGE)
		qWarning( "QPicture::play: Invalid command %d", c );
#endif
		if ( len )			// skip unknown command
		    s.device()->at( s.device()->at()+len );
	}
#if defined(QT_DEBUG)
	//qDebug( "device->at(): %i, strm_pos: %i len: %i", s.device()->at(), strm_pos, len );
	Q_ASSERT( Q_INT32(s.device()->at() - strm_pos) == len );
#endif
    }
    return FALSE;
}


/*!
  \internal
  Records painter commands and stores them in the pictb buffer.
*/

bool QPicture::cmd( int c, QPainter *pt, QPDevCmdParam *p )
{
    detach();
    return d->cmd( c, pt, p );
}

/*!
  \internal
  Implementation of the function forwarded above to the internal data struct.
*/

bool QPicture::QPicturePrivate::cmd( int c, QPainter *pt, QPDevCmdParam *p )
{
    QDataStream s;
    s.setDevice( &pictb );
    s.setVersion( formatMajor );
    if ( c ==  PdcBegin ) {			// begin; write header
	QByteArray empty( 0 );
	pictb.setBuffer( empty );		// reset byte array in buffer
	pictb.open( IO_WriteOnly );
	s.writeRawBytes( mfhdr_tag, 4 );
	s << (Q_UINT16)0 << (Q_UINT16)formatMajor << (Q_UINT16)formatMinor;
	s << (Q_UINT8)c << (Q_UINT8)sizeof(Q_INT32);
	brect = QRect();
	if ( !( formatMajor >= 1 && formatMajor <= 3 )) {
	    s << (Q_INT32)brect.left() << (Q_INT32)brect.top()
	      << (Q_INT32)brect.width() << (Q_INT32)brect.height();
	}
	trecs = 0;
	s << (Q_UINT32)trecs;			// total number of records
	formatOk = FALSE;
	return TRUE;
    } else if ( c == PdcEnd ) {		// end; calc checksum and close
	trecs++;
	s << (Q_UINT8)c << (Q_UINT8)0;
	QByteArray buf = pictb.buffer();
	int cs_start = sizeof(Q_UINT32);		// pos of checksum word
	int data_start = cs_start + sizeof(Q_UINT16);
	int brect_start = data_start + 2*sizeof(Q_INT16) + 2*sizeof(Q_UINT8);
	int pos = pictb.at();
	pictb.at( brect_start );
	if ( !( formatMajor >= 1 && formatMajor <= 3 )) { // bounding rectangle
	    s << (Q_INT32)brect.left() << (Q_INT32)brect.top()
	      << (Q_INT32)brect.width() << (Q_INT32)brect.height();
	}
	s << (Q_UINT32)trecs;			// write number of records
	pictb.at( cs_start );
	Q_UINT16 cs = (Q_UINT16)qChecksum( buf.data()+data_start, pos-data_start );
	s << cs;				// write checksum
	pictb.close();
	return TRUE;
    }
    trecs++;
    s << (Q_UINT8)c;				// write cmd to stream
    s << (Q_UINT8)0;				// write dummy length info
    int pos = (int)pictb.at();			// save position
    QRect br;					// bounding rect addition
    bool corr = FALSE;				// correction for pen width

    switch ( c ) {
	case PdcDrawPoint:
	case PdcMoveTo:
	case PdcLineTo:
	case PdcSetBrushOrigin:
	    s << *p[0].point;
	    br = QRect( *p[0].point, QSize( 1, 1 ) );
	    corr = TRUE;
	    break;
	case PdcDrawLine:
	    s << *p[0].point << *p[1].point;
	    br = QRect( *p[0].point, *p[1].point ).normalize();
	    corr = TRUE;
	    break;
	case PdcDrawRect:
	case PdcDrawEllipse:
	    s << *p[0].rect;
	    br = *p[0].rect;
	    corr = TRUE;
	    break;
	case PdcDrawRoundRect:
	case PdcDrawArc:
	case PdcDrawPie:
	case PdcDrawChord:
	    s << *p[0].rect << (Q_INT16)p[1].ival << (Q_INT16)p[2].ival;
	    br = *p[0].rect;
	    corr = TRUE;
	    break;
	case PdcDrawLineSegments:
	case PdcDrawPolyline:
	    s << *p[0].ptarr;
	    br = p[0].ptarr->boundingRect();
	    corr = TRUE;
	    break;
	case PdcDrawCubicBezier:
	    s << *p[0].ptarr;
	    br = p[0].ptarr->cubicBezier().boundingRect();
	    corr = TRUE;
	    break;
	case PdcDrawPolygon:
	    s << *p[0].ptarr << (Q_INT8)p[1].ival;
	    br = p[0].ptarr->boundingRect();
	    corr = TRUE;
	    break;
	case PdcDrawText2:
	    if ( formatMajor == 1 ) {
		pictb.at( pos - 2 );
		s << (Q_UINT8)PdcDrawText << (Q_UINT8)0;
		QCString str1( (*p[1].str).latin1() );
		s << *p[0].point << str1;
	    }
	    else {
		s << *p[0].point << *p[1].str;
	    }
	    br = pt->fontMetrics().boundingRect( *p[1].str );
	    br.moveBy( p[0].point->x(), p[0].point->y() );
	    break;
	case PdcDrawText2Formatted:
	    if ( formatMajor == 1 ) {
		pictb.at( pos - 2 );
		s << (Q_UINT8)PdcDrawTextFormatted << (Q_UINT8)0;
		QCString str1( (*p[2].str).latin1() );
		s << *p[0].rect << (Q_INT16)p[1].ival << str1;
	    }
	    else {
		s << *p[0].rect << (Q_INT16)p[1].ival << *p[2].str;
	    }
	    br = *p[0].rect;
	    break;
	case PdcDrawPixmap:
	    s << *p[0].point;
	    s << *p[1].pixmap;
	    br = QRect( *p[0].point, p[1].pixmap->size() );
	    break;
	case PdcDrawImage:
	    s << *p[0].point;
	    s << *p[1].image;
	    br = QRect( *p[0].point, p[1].image->size() );
	    break;
	case PdcSave:
	case PdcRestore:
	    break;
	case PdcSetBkColor:
	    s << *p[0].color;
	    break;
	case PdcSetBkMode:
	case PdcSetROP:
	    s << (Q_INT8)p[0].ival;
	    break;
	case PdcSetFont:
	    s << *p[0].font;
	    break;
	case PdcSetPen:
	    s << *p[0].pen;
	    break;
	case PdcSetBrush:
	    s << *p[0].brush;
	    break;
	case PdcSetTabStops:
	    s << (Q_INT16)p[0].ival;
	    break;
	case PdcSetTabArray:
	    s << (Q_INT16)p[0].ival;
	    if ( p[0].ival ) {
		int *ta = p[1].ivec;
		for ( int i=0; i<p[0].ival; i++ )
		    s << (Q_INT16)ta[i];
	    }
	    break;
	case PdcSetUnit:
	case PdcSetVXform:
	case PdcSetWXform:
	case PdcSetClip:
	    s << (Q_INT8)p[0].ival;
	    break;
#ifndef QT_NO_TRANSFORMATIONS
	case PdcSetWindow:
	case PdcSetViewport:
	    s << *p[0].rect;
	    break;
	case PdcSetWMatrix:
	    s << *p[0].matrix << (Q_INT8)p[1].ival;
	    break;
#endif
	case PdcSetClipRegion:
	    s << *p[0].rgn;
	    break;
#if defined(QT_CHECK_RANGE)
	default:
	    qWarning( "QPicture::cmd: Command %d not recognized", c );
#endif
    }
    int newpos = (int)pictb.at();		// new position
    int length = newpos - pos;
    if ( length < 255 ) {			// write 8-bit length
	pictb.at(pos - 1);			// position to right index
	s << (Q_UINT8)length;
    } else {					// write 32-bit length
	s << (Q_UINT32)0;				// extend the buffer
	pictb.at(pos - 1);			// position to right index
	s << (Q_UINT8)255;			// indicate 32-bit length
	char *p = pictb.buffer().data();
	memmove( p+pos+4, p+pos, length );	// make room for 4 byte
	s << (Q_UINT32)length;
	newpos += 4;
    }
    pictb.at( newpos );				// set to new position

    if ( br.isValid() ) {
	if ( corr ) {				// widen bounding rect
	    int w2 = pt->pen().width() / 2;
	    br.setCoords( br.left() - w2, br.top() - w2,
			  br.right() + w2, br.bottom() + w2 );
	}
	br = pt->worldMatrix().map( br );
	if ( pt->hasClipping() ) {
	    QRect cr = pt->clipRegion().boundingRect();
	    br &= cr;
	}
	if ( br.isValid() )
	    brect |= br;		     	// merge with existing rect
    }

    return TRUE;
}


/*!
  Internal implementation of the virtual QPaintDevice::metric() function.

  Use the QPaintDeviceMetrics class instead.

  A picture has the following hard-coded values: dpi = 72,
  numcolors=16777216 and depth=24.
*/

int QPicture::metric( int m ) const
{
    int val;
    switch ( m ) {
	// ### hard coded dpi and color depth values !
	case QPaintDeviceMetrics::PdmWidth:
	    val = d->brect.width();
	    break;
	case QPaintDeviceMetrics::PdmHeight:
	    val = d->brect.height();
	    break;
	case QPaintDeviceMetrics::PdmWidthMM:
	    val = int(25.4/72.0*d->brect.width());
	    break;
	case QPaintDeviceMetrics::PdmHeightMM:
	    val = int(25.4/72.0*d->brect.height());
	    break;
	case QPaintDeviceMetrics::PdmDpiX:
	case QPaintDeviceMetrics::PdmPhysicalDpiX:
	    val = 72;
	    break;
	case QPaintDeviceMetrics::PdmDpiY:
	case QPaintDeviceMetrics::PdmPhysicalDpiY:
	    val = 72;
	    break;
	case QPaintDeviceMetrics::PdmNumColors:
	    val = 16777216;
	    break;
	case QPaintDeviceMetrics::PdmDepth:
	    val = 24;
	    break;
	default:
	    val = 0;
#if defined(QT_CHECK_RANGE)
	    qWarning( "QPicture::metric: Invalid metric command" );
#endif
    }
    return val;
}

/*!
  Detaches from shared picture data and makes sure that this picture is the
  only one referring the data.

  If multiple pictures share common data, this picture makes a copy of the
  data and detaches itself from the sharing mechanism.	Nothing is
  done if there is just a single reference.
*/

void QPicture::detach()
{
    if ( d->count != 1 )
	*this = copy();
}

/*!
  Returns a
  \link shclass.html deep copy\endlink of the picture.
*/

QPicture QPicture::copy() const
{
    QPicture p;
    QByteArray a( size() );
    memcpy( a.data(), data(), size() );
    p.d->pictb.setBuffer( a );			// set byte array in buffer
    if ( d->pictb.isOpen() ) {			// copy buffer state
	p.d->pictb.open( d->pictb.mode() );
	p.d->pictb.at( d->pictb.at() );
    }
    p.d->trecs = d->trecs;
    p.d->formatOk = d->formatOk;
    p.d->formatMinor = d->formatMajor;
    p.d->brect = boundingRect();
    return p;
}

/*****************************************************************************
  QPainter member functions
 *****************************************************************************/

/*!
  Replays the picture \a pic translated by ( \a x, \a y ).

  This function does exactly the same as QPicture::play() when called with
  (\a x, \a y) = (0, 0).
*/

void QPainter::drawPicture( int x, int y, const QPicture &pic )
{
    save();
    translate( x, y );
    ((QPicture*)&pic)->play( (QPainter*)this );
    restore();
}

/*!
  \overload void QPainter::drawPicture( const QPoint &p, const QPicture &pic )
*/

void QPainter::drawPicture( const QPoint &p, const QPicture &pic )
{
    drawPicture( p.x(), p.y(), pic );
}

/*!
  \obsolete

  Use one of the other QPainter::drawPicture() functions with a (0, 0)
  offset instead.
*/

void QPainter::drawPicture( const QPicture &pic )
{
    drawPicture( 0, 0, pic );
}

/*!
  Assigns a
  \link shclass.html shallow copy\endlink
  of \e p to this picture and returns a reference to this picture.
*/

QPicture& QPicture::operator= (const QPicture& p)
{
    p.d->ref();				// avoid 'x = x'
    if ( d->deref() )
	delete d;
    d = p.d;
    return *this;
}


/*!
  \internal

  Sets formatOk to FALSE and resets the format version numbers to default
*/

void QPicture::QPicturePrivate::resetFormat()
{
    formatOk = FALSE;
    formatMajor = mfhdr_maj;
    formatMinor = mfhdr_min;
}

/*!
  \internal

  Checks data integrity and format version number. Set formatOk to TRUE
  on success, to FALSE otherwise. Returns the resulting formatOk value.
*/

bool QPicture::QPicturePrivate::checkFormat()
{
    resetFormat();

    pictb.open( IO_ReadOnly );			// open buffer device
    QDataStream s;
    s.setDevice( &pictb );			// attach data stream to buffer

    char mf_id[4];				// picture header tag
    s.readRawBytes( mf_id, 4 );			// read actual tag
    if ( memcmp(mf_id, mfhdr_tag, 4) != 0 ) { 	// wrong header id
#if defined(QT_CHECK_RANGE)
	qWarning( "QPicture::checkFormat: Incorrect header" );
#endif
	pictb.close();
	return FALSE;
    }

    int cs_start = sizeof(Q_UINT32);		// pos of checksum word
    int data_start = cs_start + sizeof(Q_UINT16);
    Q_UINT16 cs,ccs;
    QByteArray buf = pictb.buffer();	// pointer to data
    s >> cs;				// read checksum
    ccs = qChecksum( buf.data() + data_start, buf.size() - data_start );
    if ( ccs != cs ) {
#if defined(QT_CHECK_STATE)
	qWarning( "QPicture::checkFormat: Invalid checksum %x, %x expected",
		  ccs, cs );
#endif
	pictb.close();
	return FALSE;
    }

    Q_UINT16 major, minor;
    s >> major >> minor;			// read version number
    if ( major > mfhdr_maj ) {		// new, incompatible version
#if defined(QT_CHECK_RANGE)
	qWarning( "QPicture::checkFormat: Incompatible version %d.%d",
		  major, minor);
#endif
	pictb.close();
	return FALSE;
    }
    s.setVersion( major );

    Q_UINT8  c, clen;
    s >> c >> clen;
    if ( c == PdcBegin ) {
	if ( !( major >= 1 && major <= 3 )) {
	    Q_INT32 l, t, w, h;
	    s >> l >> t >> w >> h;
	    brect = QRect( l, t, w, h );
	}
    } else {
#if defined(QT_CHECK_RANGE)
	qWarning( "QPicture::checkFormat: Format error" );
#endif
	pictb.close();
	return FALSE;
    }
    pictb.close();

    formatOk = TRUE;			// picture seems to be ok
    formatMajor = major;
    formatMinor = minor;
    return TRUE;
}

/*****************************************************************************
  QPicture stream functions
 *****************************************************************************/

/*!
  \relates QPicture

  Writes a QPicture to the stream and returns a reference to the stream.
*/

QDataStream &operator<<( QDataStream &s, const QPicture &r )
{
    s << (Q_UINT32)r.d->pictb.buffer().size();
    // just write the whole buffer to the stream
    return s.writeRawBytes ( r.d->pictb.buffer().data(),
			     r.d->pictb.buffer().size() );
}

/*!
  \relates QPicture

  Reads a QPicture from the stream and returns a reference to the stream.
*/

QDataStream &operator>>( QDataStream &s, QPicture &r )
{
    QDataStream sr;

    // "init"; this code is similar to the beginning of QPicture::cmd()
    sr.setDevice( &r.d->pictb );
    sr.setVersion( r.d->formatMajor );
    QByteArray empty( 0 );
    Q_UINT32 len;
    s >> len;
    QByteArray data( len );
    s.readRawBytes( data.data(), len );

    r.d->pictb.setBuffer( data );
    r.d->resetFormat();

    return s;
}

#endif // QT_NO_PICTURE

