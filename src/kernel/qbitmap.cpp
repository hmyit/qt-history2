/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qbitmap.cpp#5 $
**
** Implementation of QBitmap class
**
** Author  : Haavard Nord
** Created : 941020
**
** Copyright (C) 1994,1995 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#include "qbitmap.h"

#if defined(DEBUG)
static char ident[] = "$Id: //depot/qt/main/src/kernel/qbitmap.cpp#5 $";
#endif


/*!
\class QBitmap qbitmap.h
\brief The QBitmap class provides monochrome (1 bit depth) pixmaps.

The QBitmap class is normally used for creating a custom mouse cursor
(QCursor) or creating a special brush (QBrush).
*/

/*!
Constructs a null bitmap.
*/

QBitmap::QBitmap()
{
    data->bitmap = TRUE;
}

/*!
Constructs an uninitialized \e w x \e h bitmap.
*/

QBitmap::QBitmap( int w, int h )
    : QPixmap( w, h, 1 )
{
    data->bitmap = TRUE;
}

/*!
Constructs a \e w x \e h bitmap and sets the contents to \e bits.

The \e isXbitmap should be TRUE if \e bits was generated by the
X-Windows bitmap program.  The X bitmap bit order is little endian.
The QImage documentation discusses bit order of monochrome images.
*/

QBitmap::QBitmap( int w, int h, const char *bits, bool isXbitmap )
    : QPixmap( w, h, bits, isXbitmap )
{
    data->bitmap = TRUE;
}

/*!
Constructs a bitmap which is a copy of \e bm. 
*/

QBitmap::QBitmap( const QBitmap &bm )
    : QPixmap( bm )
{
}


/*!
\fn QBitmap &QBitmap::operator=( const QBitmap &bm )
Assigns the bitmap \e bm to this bitmap.
*/

/*!
Converts the image \e im to a bitmap that is assigned to this bitmap.
Returns a reference to the bitmap.

Dithering will be performed if the image has a depth > 1.
*/

QBitmap &QBitmap::operator=( const QImage &im )
{
    QBitmap bm;
    bm.convertFromImage( &im );
    *this = bm;
    return *this;
}


/*!
Returns a deep copy of the bitmap.  All pixels are copied using bitBlt.
*/

QBitmap QBitmap::copy() const
{
    QBitmap tmp( data->w, data->h );
    bitBlt( &tmp, 0,0, this, 0,0, data->w, data->h );
    return tmp;    
}
