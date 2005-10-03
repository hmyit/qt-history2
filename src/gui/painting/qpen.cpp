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

#include "qpen.h"
#include "qdatastream.h"
#include "qvariant.h"
#include "qbrush.h"

#include <qdebug.h>

/*!
    \class QPen qpen.h
    \brief The QPen class defines how a QPainter should draw lines and outlines
    of shapes.

    \ingroup multimedia
    \ingroup shared
    \mainclass

    A pen has a style, width, brush, cap style and join style.

    The pen style defines the line type. The default pen style is
    Qt::SolidLine. Setting the style to Qt::NoPen tells the painter to
    not draw lines or outlines.

    The pen brush defines the fill of lines and text. The default pen
    is a solid black brush. The QColor documentation lists predefined
    colors.

    The cap style defines how the end points of lines are drawn. The
    join style defines how the joins between two lines are drawn when
    multiple connected lines are drawn (QPainter::drawPolyline()
    etc.). The cap and join styles only apply to wide lines, i.e. when
    the width is 1 or greater.

    Use the QBrush class to specify fill styles.

    Example:
    \quotefromfile snippets/brush/brush.cpp
    \skipto PEN
    \skipto QPainter
    \printuntil end

    See the \l Qt::PenStyle enum type for a complete list of pen
    styles.

    Whether or not end points are drawn when the pen width is zero or one
    depends on the cap style. Using SquareCap (the default) or
    RoundCap they are drawn, using FlatCap they are not drawn.

    A pen's color(), brush(), width(), style(), capStyle() and
    joinStyle() can be set in the constructor or later with
    setColor(), setWidth(), setStyle(), setCapStyle() and
    setJoinStyle(). Pens may also be compared and streamed.

    \img pen-styles.png Pen styles

    \sa QPainter, QPainter::setPen()
*/

class QPenPrivate {
public:
    QPenPrivate(const QBrush &brush, qreal width, Qt::PenStyle, Qt::PenCapStyle, Qt::PenJoinStyle);

    QAtomic ref;
    qreal width;
    QBrush brush;
    Qt::PenStyle style;
    Qt::PenCapStyle capStyle;
    Qt::PenJoinStyle joinStyle;
};


/*!
  \internal
*/
inline QPenPrivate::QPenPrivate(const QBrush &_brush, qreal _width, Qt::PenStyle penStyle,
                                Qt::PenCapStyle _capStyle, Qt::PenJoinStyle _joinStyle)
    : ref(1), width(_width), brush(_brush), style(penStyle), capStyle(_capStyle), joinStyle(_joinStyle)
{
}

static const Qt::PenCapStyle qpen_default_cap = Qt::SquareCap;
static const Qt::PenJoinStyle qpen_default_join = Qt::BevelJoin;


class QPenStatic
{
public:
    QPenPrivate *pointer;
    bool destroyed;

    inline QPenStatic()
        : pointer(0), destroyed(false)
    { }

    inline ~QPenStatic()
    {
        if (!pointer->ref.deref())
            delete pointer;
        pointer = 0;
        destroyed = true;
    }
};


static QPenPrivate *defaultPenInstance()
{
    static QPenStatic defaultPen;
    if (!defaultPen.pointer && !defaultPen.destroyed) {
        QPenPrivate *x = new QPenPrivate(Qt::black, 0, Qt::SolidLine, qpen_default_cap, qpen_default_join);
        if (!q_atomic_test_and_set_ptr(&defaultPen.pointer, 0, x))
            delete x;
    }
    return defaultPen.pointer;
}

static QPenPrivate *nullPenInstance()
{
    static QPenStatic defaultPen;
    if (!defaultPen.pointer && !defaultPen.destroyed) {
        QPenPrivate *x = new QPenPrivate(Qt::black, 0, Qt::NoPen, qpen_default_cap, qpen_default_join);
        if (!q_atomic_test_and_set_ptr(&defaultPen.pointer, 0, x))
            delete x;
    }
    return defaultPen.pointer;
}

/*!
    Constructs a default black solid line pen with 0 width.
*/

QPen::QPen()
{
    d = defaultPenInstance();
    d->ref.ref();
}

/*!
    Constructs a black pen with 0 width and style \a style.

    \sa setStyle()
*/

QPen::QPen(Qt::PenStyle style)
{
    if (style == Qt::NoPen) {
        d = nullPenInstance();
        d->ref.ref();
    } else {
        d = new QPenPrivate(Qt::black, 0, style, qpen_default_cap, qpen_default_join);
    }
}


/*!
    Constructs a pen of color \a color with 0 width.

    \sa setBrush(), setColor()
*/

QPen::QPen(const QColor &color)
{
    d = new QPenPrivate(color, 0, Qt::SolidLine, qpen_default_cap, qpen_default_join);
}


/*!
    Constructs a pen with the specified brush \a brush and width \a
    width.  The pen style is set to \a s, the pen cap style to \a c
    and the pen join style to \a j.

    \sa setWidth(), setStyle(), setBrush()
*/

QPen::QPen(const QBrush &brush, qreal width, Qt::PenStyle s, Qt::PenCapStyle c, Qt::PenJoinStyle j)
{
    d = new QPenPrivate(brush, width, s, c, j);
}

/*!
    Constructs a pen that is a copy of \a p.
*/

QPen::QPen(const QPen &p)
{
    d = p.d;
    d->ref.ref();
}


/*!
    Destroys the pen.
*/

QPen::~QPen()
{
    if (!d->ref.deref())
        delete d;
}

/*!
    \fn void QPen::detach()
    Detaches from shared pen data to make sure that this pen is the
    only one referring the data.

    If multiple pens share common data, this pen dereferences the data
    and gets a copy of the data. Nothing is done if there is just a
    single reference.
*/

void QPen::detach()
{
    if (d->ref == 1)
        return;

    QPenPrivate *x = new QPenPrivate(d->brush, d->width, d->style, d->capStyle, d->joinStyle);
    x = qAtomicSetPtr(&d, x);
    if (!x->ref.deref())
        delete x;
}


/*!
    Assigns \a p to this pen and returns a reference to this pen.
*/

QPen &QPen::operator=(const QPen &p)
{
    qAtomicAssign(d, p.d);
    return *this;
}

/*!
   Returns the pen as a QVariant
*/
QPen::operator QVariant() const
{
    return QVariant(QVariant::Pen, this);
}

/*!
    \fn Qt::PenStyle QPen::style() const

    Returns the pen style.

    \sa setStyle()
*/

Qt::PenStyle QPen::style() const
{
    return d->style;
}

/*!
    Sets the pen style to \a s.

    See the \l Qt::PenStyle documentation for a list of all the
    styles.

    \sa style()
*/

void QPen::setStyle(Qt::PenStyle s)
{
    if (d->style == s)
        return;
    detach();
    d->style = s;
}


/*!
    \fn qreal QPen::width() const

    Returns the pen width with integer preceision.

    \sa setWidth()
*/

int QPen::width() const
{
    return qRound(d->width);
}

/*!
    \fn qreal QPen::widthF() const

    Returns the pen width with floating point precision.

    \sa setWidthF() width()
*/
qreal QPen::widthF() const
{
    return d->width;
}

/*!
    \fn QPen::setWidth(int width)

    Sets the pen width to \a width

    A line width of zero indicates cosmetic pen. This means that the
    pen width is always drawn one pixel wide, independent of the
    transformation set on the painter.

    Setting a pen width with a negative value is not supported.

    \sa setWidthF() width()
*/
void QPen::setWidth(int width)
{
    if (width < 0)
        qWarning("QPen::setWidth(): Setting a pen width with a negative value is not defined.");
    if ((qreal)width == d->width)
        return;
    detach();
    d->width = width;
}

/*!
    Sets the pen width to \a width.

    \overload

    \sa setWidth() widthF()
*/

void QPen::setWidthF(qreal width)
{
    if (width < 0.f)
        qWarning("QPen::setWidthF(): Setting a pen width with a negative value is not defined.");
    if (qAbs(d->width - width) < 0.00000001f)
        return;
    detach();
    d->width = width;
}


/*!
    Returns the pen's cap style.

    \sa setCapStyle()
*/
Qt::PenCapStyle QPen::capStyle() const
{
    return d->capStyle;
}

/*!
    Sets the pen's cap style to \a c.

    The default value is Qt::SquareCap.

    \img pen-cap-styles.png Pen Cap Styles

    \sa capStyle()
*/

void QPen::setCapStyle(Qt::PenCapStyle c)
{
    if (d->capStyle == c)
        return;
    detach();
    d->capStyle = c;
}

/*!
    Returns the pen's join style.

    \sa setJoinStyle()
*/
Qt::PenJoinStyle QPen::joinStyle() const
{
    return d->joinStyle;
}

/*!
    Sets the pen's join style to \a j.

    The default value is Qt::BevelJoin.

    \img pen-join-styles.png Pen Join Styles

    \sa joinStyle()
*/

void QPen::setJoinStyle(Qt::PenJoinStyle j)
{
    if (d->joinStyle == j)
        return;
    detach();
    d->joinStyle = j;
}

/*!
    \fn const QColor &QPen::color() const

    Returns the pen color.

    \sa setColor()
*/
QColor QPen::color() const
{
    return d->brush.color();
}

/*!
    Sets the pen color to \a c.

    \sa color()
*/

void QPen::setColor(const QColor &c)
{
    detach();
    d->brush = QBrush(c);
}


/*!
    Returns the brush used to fill strokes generated with this pen.
*/
QBrush QPen::brush() const
{
    return d->brush;
}


/*!
    Sets the brush used to fill strokes generated with this pen to the given
    \a brush.
*/
void QPen::setBrush(const QBrush &brush)
{
    detach();
    d->brush = brush;
}


/*!
    Returns true if the pen has a solid fill
*/
bool QPen::isSolid() const
{
    return d->brush.style() == Qt::SolidPattern;
}


/*!
    \fn bool QPen::operator!=(const QPen &p) const

    Returns true if the pen is different from \a p; otherwise returns
    false.

    Two pens are different if they have different styles, widths or
    colors.

    \sa operator==()
*/

/*!
    Returns true if the pen is equal to \a p; otherwise returns false.

    Two pens are equal if they have equal styles, widths and colors.

    \sa operator!=()
*/

bool QPen::operator==(const QPen &p) const
{
    return (p.d == d) || (p.d->style == d->style
                          && p.d->capStyle == d->capStyle
                          && p.d->joinStyle == d->joinStyle
                          && p.d->width == d->width
                          && p.d->brush == d->brush);
}


/*!
    \fn bool QPen::isDetached()

    \internal
*/

bool QPen::isDetached()
{
    return d->ref == 1;
}


/*****************************************************************************
  QPen stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
/*!
    \relates QPen

    Writes the pen \a p to the stream \a s and returns a reference to
    the stream.

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator<<(QDataStream &s, const QPen &p)
{
    if (s.version() < 3)
        s << (quint8)p.style();
    else
        s << (quint8)(p.style() | p.capStyle() | p.joinStyle());

    if (s.version() < 7) {
        s << (quint8)p.width();
        s << p.color();
    } else {
        s << p.widthF();
        s << p.brush();
    }
    return s;
}

/*!
    \relates QPen

    Reads a pen from the stream \a s into \a p and returns a reference
    to the stream.

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/

QDataStream &operator>>(QDataStream &s, QPen &p)
{
    quint8 style;
    quint8 width8 = 0;
    double width = 0;
    QColor color;
    QBrush brush;
    s >> style;
    if (s.version() < 7) {
        s >> width8;
        s >> color;
        brush = color;
        width = width8;
    } else {
        s >> width;
        s >> brush;
    }

    Qt::PenStyle penStyle = Qt::PenStyle(style & Qt::MPenStyle);
    Qt::PenCapStyle capStyle = Qt::PenCapStyle(style & Qt::MPenCapStyle);
    Qt::PenJoinStyle joinStyle = Qt::PenJoinStyle(style & Qt::MPenJoinStyle);

    p = QPen(brush, width, penStyle, capStyle, joinStyle);

    return s;
}
#endif //QT_NO_DATASTREAM
