/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qlinefloat.h"

#include <math.h>

/*!
    \class QLineF

    \brief The QLineF class provides a two-dimensional vector that
    uses floating point coordinates for accuracy.

    A QLineF describes a finite length line on a two-dimensional surface.
    The start and end points of the line are specified using floating point
    coordinates for accuracy.

    Convenience functions are provided for finding the lines's length(),
    the unitVector() along the line, and whether two lines intersect().
    The line can be translated along the length of another line with the
    moveBy() function.

    \sa QPointF QSizeF QRectF
*/

/*!
    \enum QLineF::IntersectMode

    \value Unbounded In this mode, checks for intersection ignore the start and
                     end points of each line. This means that non-parallel
                     lines will always have an intersection.

    \img qlinefloat-unbounded.png

    \value Bounded   In this mode, checks for intersection take into account
                     the start and end points of each line. This means that
                     non-parallel lines will only intersect if the intersection
                     occurs between the start and end points of each line.

    \img qlinefloat-bounded.png

*/

/*!
    \fn QLineF::QLineF()

    Constructs a null line.
*/

/*!
    \fn QLineF::QLineF(const QPointF &pt1, const QPointF &pt2)

    Constructs a line object that represents the line between \a pt1 and
    \a pt2.
*/

/*!
    \fn QLineF::QLineF(float x1, float y1, float x2, float y2)

    Constructs a line object that represents the line between (\a x1, \a y1) and
    (\a x2, \a y2).
*/

/*!
    \fn bool QLineF::isNull() const

    Returns true if the line is not set up with valid start and end point;
    otherwise returns false.
*/

/*!
    \fn QPointF QLineF::start() const

    Returns the line's start point.
*/

/*!
    \fn QPointF QLineF::end() const

    Returns the line's end point.
*/

/*!
    \fn float QLineF::startX() const

    Returns the x-coordinate of the line's start point.
*/

/*!
    \fn float QLineF::startY() const

    Returns the y-coordinate of the line's start point.
*/

/*!
    \fn float QLineF::endX() const

    Returns the x-coordinate of the line's end point.
*/

/*!
    \fn float QLineF::endY() const

    Returns the y-coordinate of the line's end point.
*/

/*!
    \fn float QLineF::vx() const

    Returns the horizontal component of the line's vector.
*/

/*!
    \fn float QLineF::vy() const

    Returns the vertical component of the line's vector.
*/

/*!
    \fn QLineF::setLength(float len)
*/

/*!
    \fn QLineF QLineF::normalVector() const

    Returns a line that is perpendicular to this line with the same starting
    point and length.

    \img qlinefloat-normalvector.png
*/

/*!
    \fn bool QLineF::intersects(const QLineF &l, IntersectMode mode = Unbounded)

    Returns true if this line intersects the line specified by \a l for the
    given intersection \a mode; otherwise returns false.
*/

/*!
    \fn void QLineF::operator+=(const QPointF &other)

    Performs a vector addition of this line with the \a other line given.
*/


/*!
    Returns the length of the line.
*/
float QLineF::length() const
{
    float x = p2.x() - p1.x();
    float y = p2.y() - p1.y();
    return sqrt(x*x + y*y);
}

/*!
    Returns a normalized version of this line, starting at the same
    point as this line. A normalized line is a line of unit length
    (length() is equal to 1.0).
*/
QLineF QLineF::unitVector() const
{
    float x = p2.x() - p1.x();
    float y = p2.y() - p1.y();

    float len = sqrt(x*x + y*y);
    QLineF f(start(), QPointF(p1.x() + x/len, p1.y() + y/len));
    Q_ASSERT(QABS(f.length() - 1) < 0.001);
    return f;
}

/*!
    Returns the intersection point between the this line and the line
    \a l. The mode \a mode specifies if the intersection is bounded or
    not and \a intersected is set to true if the lines did intersect.
    The return value is undefined if the lines do not intersect.
*/
QPointF QLineF::intersect(const QLineF &l, IntersectMode mode, bool *intersected) const
{
    Q_ASSERT(!isNull());
    Q_ASSERT(!l.isNull());
    // Parallell lines
    if (vx() == l.vx() && vy() == l.vy()) {
        if (intersected)
            *intersected = false;
        return QPointF();
    }

    // For special case where one of the lines are vertical
    if (vx() == 0) {
        float la = l.vy() / l.vx();
        QPointF isect(p1.x(), la * p1.x() + l.startY() - la*l.startX());
        return isect;
    } else if (l.vx() == 0) {
        float ta = vy() / vx();
        QPointF isect(l.startX(), ta * l.startX() + startY() - ta*startX());
        return isect;
    }

    float ta = vy()/vx();
    float la = l.vy()/l.vx();

    float x = ( - l.startY() + la * l.startX() + p1.y() - ta * p1.x() ) / (la - ta);

    QPointF isect = QPointF(x, ta*(x - p1.x()) + p1.y());

    if (mode == Unbounded) {
        if (intersected)
            *intersected = true;
    } else {
        if (intersected)
            *intersected = (x >= p1.x() && x <= p2.x() && x >= l.p1.x() && x <= l.p2.x());
    }
    return isect;
}


/*!
    \fn void QLineF::moveBy(const QLineF &l)

    Translates this line by the vector specified by the line \a l.

*/
