/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qrect.cpp#14 $
**
** Implementation of QRect class
**
** Author  : Haavard Nord
** Created : 931028
**
** Copyright (C) 1993-1995 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#define QRECT_C
#include "qrect.h"
#include "qdstream.h"

#if defined(DEBUG)
static char ident[] = "$Id: //depot/qt/main/src/kernel/qrect.cpp#14 $";
#endif


/*!
\class QRect qrect.h
\brief The QRect class defined a rectangle in the plane.

A rectangle is internally represented as an upper left corner and a bottom
right corner, but it is normally expressed as an upper left corner and a
size.

The coordinate type is QCOORD (defined as <code>short</code>). The minimum
value of QCOORD is -32768 and the maximum value is 32767.

Notice that the size (width and height) of a rectange might be different
from what you are used to. If the top left corner and
the bottom right corner are the same, then the height and the width of the
rectange will both be one.

Generally, <em>width = right - left + 1</em> and <em>height = bottom - top
+ 1</em>. We designed it this way to make it correspond to rectangular
spaces used by drawing functions, where the width and height denote a
number of pixels. For example, drawing a rectangle with width and height 1
draws a single pixel.

The default coordinate system is assumed to have its origo (0,0) in the top
left corner, the positive direction of the y axis is downwards and the positive
x axis is from the left to the right.

\sa QPoint and QSize.
*/


// --------------------------------------------------------------------------
// QRect member functions
//

/*!
\fn QRect::QRect()
Constructs a rectangle with undefined position and size.
*/

/*!
Constructs a rectangle with \e topleft as the top left corner and
\e bottomright as the bottom right corner.
*/

QRect::QRect( const QPoint &topleft, const QPoint &bottomright )
{
    x1 = topleft.x();
    y1 = topleft.y();
    x2 = bottomright.x();
    y2 = bottomright.y();
}

/*!
Constructs a rectangle with \e topleft as the top left corner and
\e size as the rectangle size.
*/

QRect::QRect( const QPoint &topleft, const QSize &size )
{
    x1 = topleft.x();
    y1 = topleft.y();
    x2 = x1+size.width()-1;
    y2 = y1+size.height()-1;
}

/*!
Constructs a rectangle with the a top left corner and a width and a
height.
*/

QRect::QRect( QCOORD left, QCOORD top, QCOORD width, QCOORD height )
{
    x1 = left;
    y1 = top;
    x2 = left+width-1;
    y2 = top+height-1;
}

/*!
Returns TRUE if the rectangle is a null rectangle.

A null rectangle has both the width and the height set to 0, that is
right() == left() - 1 and bottom() == top() - 1.

Remember that if right() == left() and bottom() == top(), then we have
a rectangle with width 1 and height 1.

A null rectangle is also empty.

A null rectangle is not valid.

\sa isEmpty() and isValid().
*/

bool QRect::isNull() const
{
    return x2 == x1-1 && y2 == y1-1;
}

/*!
Returns TRUE if the rectangle is empty, or FALSE if it is not empty.

An empty rectangle has a left() > right() or top() > bottom().

An empty rectangle is not valid.

\sa isValid().
*/

bool QRect::isEmpty() const
{
    return x1 > x2 || y1 > y2;
}

/*!
Returns if the rectangle is valid, or FALSE if it is invalid.

A valid rectangle has a left() \<= right() and top() \<= bottom().
*/

bool QRect::isValid() const
{
    return x1 <= x2 && y1 <= y2;
}

/*!
Repairs an invalid rectangle.

It swaps left and right if left() > right(), and swaps top and bottom if
top() > bottom().

\sa isValid().
*/

void QRect::fixup()
{
    QCOORD t;
    if ( x2 < x1 ) {				// swap bad x values
	t = x1;
	x1 = x2;
	x2 = t;
    }
    if ( y2 < y1 ) {				// swap bad y values
	t = y1;
	y1 = y2;
	y2 = t;
    }
}


/*!
\fn QCOORD QRect::left() const
Returns the left coordinate of the rectangle.
*/

/*!
\fn QCOORD QRect::top() const
Returns the top coordinate of the rectangle.
*/

/*!
\fn QCOORD QRect::right() const
Returns the right coordinate of the rectangle.
*/

/*!
\fn QCOORD QRect::bottom() const
Returns the bottom coordinate of the rectangle.
*/

/*!
\fn QCOORD QRect::x() const
Returns the left coordinate of the rectangle.

Synonymous to left().
*/

/*!
\fn QCOORD QRect::y() const
Returns the top coordinate of the rectangle.

Synonymous to top().
*/

/*!
Sets the left position of the rectangle, possibly changing the width.
*/

void QRect::setLeft( QCOORD pos )
{
    x1=pos;
}

/*!
Sets the top position of the rectangle, possibly changing the height.
*/

void QRect::setTop( QCOORD pos )
{
    y1=pos;
}

/*!
Sets the x (left) position of the rectangle, possibly changing the width.

Synonymous to setLeft().
*/

void QRect::setX( QCOORD x )
{
    x1 = x;
}

/*!
Sets the y (top) position of the rectangle, possibly changing the height.

Synonymous to setTop().
*/

void QRect::setY( QCOORD y )
{
    y1 = y;
}

/*!
Sets the right position of the rectangle, possibly changing the width.

\sa right().
*/

void QRect::setRight( QCOORD pos )
{
    x2=pos;
}

/*!
Sets the bottom position of the rectangle, possibly changing the height.

\sa bottom().
*/

void QRect::setBottom( QCOORD pos )
{
    y2=pos;
}

/*!
Returns the top left position of the rectangle.
*/

QPoint QRect::topLeft() const
{
    return QPoint( x1, y1 );
}

/*!
Returns the bottom right position of the rectangle.
*/

QPoint QRect::bottomRight() const
{
    return QPoint( x2, y2 );
}

/*!
Returns the top right position of the rectangle.
*/

QPoint QRect::topRight() const
{
    return QPoint( x2, y1 );
}

/*!
Returns the bottom left position of the rectangle.
*/

QPoint QRect::bottomLeft() const
{
    return QPoint( x1, y2 );
}

/*!
Returns the center point of the rectangle.
*/

QPoint QRect::center() const
{
    return QPoint( (x1+x2)/2, (y1+y2)/2 );
}

/*!
Extracts the rectangle parameters as the position and the size.
*/

void QRect::rect( int *x, int *y, int *w, int *h ) const
{
    *x = x1;
    *y = y1;
    *w = x2-x1+1;
    *h = y2-y1+1;
}

/*!
Extracts the rectangle parameters as the top left point and the bottom right
point.
*/

void QRect::coords( int *xp1, int *yp1, int *xp2, int *yp2 ) const
{
    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2;
}

/*!
Sets the top left position of the rectangle to \e p, leaving the size
unchanged.
*/

void QRect::setTopLeft( const QPoint &p )
{
    x2 += (p.x() - x1);
    y2 += (p.y() - y1);
    x1 = p.x();
    y1 = p.y();
}

/*!
Sets the bottom right position of the rectangle to \e p, leaving the size
unchanged.
*/

void QRect::setBottomRight( const QPoint &p )
{
    x1 += (p.x() - x2);
    y1 += (p.y() - y2);
    x2 = p.x();
    y2 = p.y();
}

/*!
Sets the top right position of the rectangle to \e p, leaving the size
unchanged.
*/

void QRect::setTopRight( const QPoint &p )
{
    x1 += (p.x() - x2);
    y2 += (p.y() - y1);
    x2 = p.x();
    y1 = p.y();
}

/*!
Sets the bottom left position of the rectangle to \e p, leaving the size
unchanged.
*/

void QRect::setBottomLeft( const QPoint &p )
{
    x2 += (p.x() - x1);
    y1 += (p.y() - y2);
    x1 = p.x();
    y2 = p.y();
}

/*!
Sets the center point of the rectangle to \e p, leaving the size
unchanged.
*/

void QRect::setCenter( const QPoint &p )
{
    QCOORD w = x2 - x1;
    QCOORD h = y2 - y1;
    x1 = p.x() - w/2;
    y1 = p.y() - h/2;
    x2 = x1 + w;
    y2 = y1 + h;
}

/*!
Moves the rectangle \e dx along the X axis and \e dy along the Y axis.
*/

void QRect::move( int dx, int dy )
{
    x1 += dx;
    y1 += dy;
    x2 += dx;
    y2 += dy;
}

/*!
Sets the rectangle to a top left position and a size.
*/

void QRect::setRect( int x, int y, int w, int h )
{
    x1 = x;
    y1 = y;
    x2 = x+w-1;
    y2 = y+h-1;
}

/*!
Sets the rectangle to a top left position and bottom right position.
*/

void QRect::setCoords( int xp1, int yp1, int xp2, int yp2 )
{
    x1 = xp1;
    y1 = yp1;
    x2 = xp2;
    y2 = yp2;
}

/*!
Returns the size of the rectangle.
*/

QSize QRect::size() const
{
    return QSize( x2-x1+1, y2-y1+1 );
}

/*!
Returns the width of the rectangle.

width = right - left + 1.
*/

QCOORD QRect::width() const
{
    return x2-x1+1;
}

/*!
Returns the height of the rectangle.

height = bottom - top + 1.
*/

QCOORD QRect::height() const
{
    return y2-y1+1;
}

/*!
Sets the size of the rectangle to \e s.
*/

void QRect::setSize( const QSize &s )
{
    x2 = x1+s.width()-1;
    y2 = y1+s.height()-1;
}

/*!
Returns TRUE if the point \e p is inside or on the edge of the rectangle.

If \e proper is TRUE, this function returns TRUE only if \e p is
inside (not on the edge).
*/

bool QRect::contains( const QPoint &p, bool proper ) const
{
    if ( proper )
	return p.x() > x1 && p.x() < x2 &&
	       p.y() > y1 && p.y() < y2;
    else
	return p.x() >= x1 && p.x() <= x2 &&
	       p.y() >= y1 && p.y() <= y2;
}

/*!
Returns TRUE if the rectangle \e r is inside or if it touches an edge of this
rectangle.

If \e proper is TRUE, this function returns TRUE only if \e r is inside
(not on the edge).
*/

bool QRect::contains( const QRect &r, bool proper ) const
{
    if ( proper )
	return r.x1 > x1 && r.x2 < x2 && r.y1 > y1 && r.y2 < y2;
    else
	return r.x1 >= x1 && r.x2 <= x2 && r.y1 >= y1 && r.y2 <= y2;
}

/*!
Returns the union rectangle of this rectangle and \e r.
*/

QRect QRect::unite( const QRect &r ) const
{
    QRect tmp;
    tmp.setLeft( QMIN( x1, r.x1 ) );
    tmp.setRight( QMAX( x2, r.x2 ) );
    tmp.setTop( QMIN( y1, r.y1 ) );
    tmp.setBottom( QMAX( y2, r.y2 ) );
    return tmp;
}

/*!
Returns the intersection rectangle of this rectangle and \e r.

The returned rectangle will be empty if there is no intersection.

\sa isEmpty().
*/

QRect QRect::intersect( const QRect &r ) const
{
    QRect tmp;
    tmp.x1 = QMAX( x1, r.x1 );
    tmp.x2 = QMIN( x2, r.x2 );
    tmp.y1 = QMAX( y1, r.y1 );
    tmp.y2 = QMIN( y2, r.y2 );
    return tmp;
}

/*!
Returns TRUE if this rectangle intersects with \e r.
*/

bool QRect::intersects(const QRect &r ) const
{
    return ( QMAX( x1, r.x1 ) <= QMIN( x2, r.x2 ) && 
             QMAX( y1, r.y1 ) <= QMIN( y2, r.y2 ) );
}


/*!
\relates QRect
Returns TRUE if \e r1 and \e r2 are equal, or FALSE if they are different.
*/

bool operator==( const QRect &r1, const QRect &r2 )
{
    return r1.x1==r2.x1 && r1.x2==r2.x2 && r1.y1==r2.y1 && r1.y2==r2.y2;
}

/*!
\relates QRect
Returns TRUE if \e r1 and \e r2 are different, or FALSE if they are equal.
*/

bool operator!=( const QRect &r1, const QRect &r2 )
{
    return r1.x1!=r2.x1 || r1.x2!=r2.x2 || r1.y1!=r2.y1 || r1.y2!=r2.y2;
}


// --------------------------------------------------------------------------
// QRect stream functions
//

/*!
\relates QRect
Writes a QRect to the stream.

The output format is four INT16 (order: left, top, right, bottom).
*/

QDataStream &operator<<( QDataStream &s, const QRect &r )
{
    return s << (INT16)r.left() << (INT16)r.top()
	     << (INT16)r.right() << (INT16)r.bottom();
}

/*!
\relates QRect
Reads a QRect from the stream.
*/

QDataStream &operator>>( QDataStream &s, QRect &r )
{
    INT16 x1, y1, x2, y2;
    s >> x1; s >> y1; s >> x2; s >> y2;
    r.setCoords( x1, y1, x2, y2 );
    return s;
}
