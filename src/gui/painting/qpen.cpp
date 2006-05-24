/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
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
    \class QPen
    \ingroup multimedia
    \ingroup shared
    \mainclass

    \brief The QPen class defines how a QPainter should draw lines and outlines
    of shapes.

    A pen has a style(), width(), brush(), capStyle() and joinStyle().

    The pen style defines the line type. The brush is used to fill
    strokes generated with the pen. Use the QBrush class to specify
    fill styles.  The cap style determines the line end caps that can
    be drawn using QPainter, while the join style describes how joins
    between two lines are drawn. The pen width can be specified in
    both integer (width()) and floating point (widthF()) precision. A
    line width of zero indicates a cosmetic pen.  This means that the
    pen width is always drawn one pixel wide, independent of the \l
    {QPainter#Coordinate Transformations}{transformation} set on the
    painter.

    The various settings can easily be modified using the
    corresponding setStyle(), setWidth(), setBrush(), setCapStyle()
    and setJoinStyle() functions (note that the painter's pen must be
    reset when altering the pen's properties).

    For example:

    \code
        QPainter painter(this);
        QPen pen(Qt::green, 3, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);
    \endcode

    which is equivalent to

    \code
        QPainter painter(this);
        QPen pen();  // creates a default pen

        pen.setStyle(Qt::DashDotLine);
        pen.setWidth(3);
        pen.setBrush(Qt::green);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);

        painter.setPen(pen);
    \endcode

    The default pen is a solid black brush with 0 width, square
    cap style (Qt::SquareCap), and  bevel join style (Qt::BevelJoin).

    In addition QPen provides the color() and setColor()
    convenience functions to extract and set the color of the pen's
    brush, respectively. Pens may also be compared and streamed.

    For more information about painting in general, see {The Paint
    System} documentation.

    \tableofcontents

    \section1 Pen Style

    Qt provides several built-in styles represented by the
    Qt::PenStyle enum:

    \table
    \row
    \o \inlineimage qpen-solid.png
    \o \inlineimage qpen-dash.png
    \o \inlineimage qpen-dot.png
    \row
    \o Qt::SolidLine
    \o Qt::DashLine
    \o Qt::DotLine
    \row
    \o \inlineimage qpen-dashdot.png
    \o \inlineimage qpen-dashdotdot.png
    \o \inlineimage qpen-custom.png
    \row
    \o Qt::DashDotLine
    \o Qt::DashDotDotLine
    \o Qt::CustomDashLine
    \endtable

    Simply use the setStyle() function to convert the pen style to
    either of the built-in styles, except the Qt::CustomDashLine style
    which we will come back to shortly. Setting the style to Qt::NoPen
    tells the painter to not draw lines or outlines. The default pen
    style is Qt::SolidLine.

    Since Qt 4.1 it is also possible to specify a custom dash pattern
    using the setDashPattern() function which implicitly converts the
    style of the pen to Qt::CustomDashLine. The pattern argument, a
    QVector, must be specified as an even number of \l qreal entries
    where the entries 1, 3, 5... are the dashes and 2, 4, 6... are the
    spaces. For example, the custom pattern shown above is created
    using the following code:

    \code
    QPen pen;
    QVector<qreal> dashes;
    qreal space = 4;

    dashes << 1 << space << 3 << space << 9 << space
               << 27 << space << 9;

    pen.setDashPattern(dashes);
    \endcode

    Note that the dash pattern is specified in units of the pens
    width, e.g. a dash of length 5 in width 10 is 50 pixels long.

    The currently set dash pattern can be retrieved using the
    dashPattern() function. Use the isSolid() function to determine
    whether the pen has a solid fill, or not.

    \section1 Cap Style

    The cap style defines how the end points of lines are drawn using
    QPainter.  The cap style only apply to wide lines, i.e. when the
    width is 1 or greater. The Qt::PenCapStyle enum provides the
    following styles:

    \table
    \row
    \o \inlineimage qpen-square.png
    \o \inlineimage qpen-flat.png
    \o \inlineimage qpen-roundcap.png
    \row
    \o Qt::SquareCap
    \o Qt::FlatCap
    \o Qt::RoundCap
    \endtable

    The Qt::SquareCap style is a square line end that covers the end
    point and extends beyond it by half the line width. The
    Qt::FlatCap style is a square line end that does not cover the end
    point of the line. And the Qt::RoundCap style is a rounded line
    end covering the end point.

    The default is Qt::SquareCap.

    Whether or not end points are drawn when the pen width is 0 or 1
    depends on the cap style. Using Qt::SquareCap or Qt::RoundCap they
    are drawn, using Qt::FlatCap they are not drawn.

    \section1 Join Style

    The join style defines how joins between two connected lines can
    be drawn using QPainter. The join style only apply to wide lines,
    i.e. when the width is 1 or greater. The Qt::PenJoinStyle enum
    provides the following styles:

    \table
    \row
    \o \inlineimage qpen-bevel.png
    \o \inlineimage qpen-miter.png
    \o \inlineimage qpen-roundjoin.png
    \row
    \o Qt::BevelJoin
    \o Qt::MiterJoin
    \o Qt::RoundJoin
    \endtable

    The Qt::BevelJoin style fills the triangular notch between the two
    lines. The Qt::MiterJoin style extends the lines to meet at an
    angle. And the Qt::RoundJoin style fills a circular arc between
    the two lines.

    The default is Qt::BevelJoin.

    \image qpen-miterlimit.png

    When the Qt::MiterJoin style is applied, it is possible to use the
    setMiterLimit() function to specify how far the miter join can
    extend from the join point. The miterLimit() is used to reduce
    artifacts between line joins where the lines are close to
    parallel.

    The miterLimit() must be specified in units of the pens width,
    e.g. a miter limit of 5 in width 10 is 50 pixels long. The
    default miter limit is 2, i.e. twice the pen width in pixels.

    \table 100%
    \row
    \o \inlineimage qpen-demo.png
    \o \bold {\l {demos/pathstroke}{The Path Stroking Demo}}

    The Path Stroking demo shows Qt's built-in dash patterns and shows
    how custom patterns can be used to extend the range of available
    patterns.
    \endtable

    \sa QPainter, QBrush, {demos/pathstroke}{The Path Stroking Demo}
*/

class QPenPrivate {
public:
    QPenPrivate(const QBrush &brush, qreal width, Qt::PenStyle, Qt::PenCapStyle,
                Qt::PenJoinStyle _joinStyle);

    QAtomic ref;
    qreal width;
    QBrush brush;
    Qt::PenStyle style;
    Qt::PenCapStyle capStyle;
    Qt::PenJoinStyle joinStyle;
    mutable QVector<qreal> dashPattern;
    qreal miterLimit;
};


/*!
  \internal
*/
inline QPenPrivate::QPenPrivate(const QBrush &_brush, qreal _width, Qt::PenStyle penStyle,
                                Qt::PenCapStyle _capStyle, Qt::PenJoinStyle _joinStyle)
    : ref(1), width(_width), brush(_brush), style(penStyle), capStyle(_capStyle),
      joinStyle(_joinStyle), miterLimit(2)
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
        QPenPrivate *x = new QPenPrivate(Qt::black, 0, Qt::SolidLine,
                                         qpen_default_cap, qpen_default_join);
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
    Constructs a black pen with 0 width and the given \a style.

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
    Constructs a pen with 0 width and the given \a color.

    \sa setBrush(), setColor()
*/

QPen::QPen(const QColor &color)
{
    d = new QPenPrivate(color, 0, Qt::SolidLine, qpen_default_cap, qpen_default_join);
}


/*!
    \fn QPen::QPen(const QBrush &brush, qreal width, Qt::PenStyle style, Qt::PenCapStyle cap, Qt::PenJoinStyle join)

    Constructs a pen with the specified \a brush, \a width, pen \a style,
    \a cap style and \a join style.

    \sa setBrush(), setWidth(), setStyle(),  setCapStyle(), setJoinStyle()
*/

QPen::QPen(const QBrush &brush, qreal width, Qt::PenStyle s, Qt::PenCapStyle c, Qt::PenJoinStyle j)
{
    d = new QPenPrivate(brush, width, s, c, j);
}

/*!
    \fn QPen::QPen(const QPen &pen)

    Constructs a pen that is a copy of the given \a pen.
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

    QPenPrivate *x = new QPenPrivate(d->brush, d->width, d->style, d->capStyle,
                                     d->joinStyle);
    x->miterLimit = d->miterLimit;
    x->dashPattern = d->dashPattern;
    x = qAtomicSetPtr(&d, x);
    if (!x->ref.deref())
        delete x;
}


/*!
    \fn QPen &QPen::operator=(const QPen &pen)

    Assigns the given \a pen to this pen and returns a reference to
    this pen.
*/

QPen &QPen::operator=(const QPen &p)
{
    qAtomicAssign(d, p.d);
    return *this;
}

/*!
   Returns the pen as a QVariant.
*/
QPen::operator QVariant() const
{
    return QVariant(QVariant::Pen, this);
}

/*!
    \fn Qt::PenStyle QPen::style() const

    Returns the pen style.

    \sa setStyle(), {QPen#Pen Style}{Pen Style}
*/

Qt::PenStyle QPen::style() const
{
    return d->style;
}

/*!
    \fn void QPen::setStyle(Qt::PenStyle style)

    Sets the pen style to the given \a style.

    See the \l Qt::PenStyle documentation for a list of the available
    styles. Since Qt 4.1 it is also possible to specify a custom dash
    pattern using the setDashPattern() function which implicitly
    converts the style of the pen to Qt::CustomDashLine.

    \sa style(), {QPen#Pen Style}{Pen Style}
*/

void QPen::setStyle(Qt::PenStyle s)
{
    if (d->style == s)
        return;
    detach();
    d->style = s;
    d->dashPattern.clear();
}

/*!
    Returns the dash pattern of this pen.

    \sa style(), isSolid()
 */
QVector<qreal> QPen::dashPattern() const
{
    if (d->style == Qt::SolidLine || d->style == Qt::NoPen) {
        return QVector<qreal>();
    } else if (d->dashPattern.isEmpty()) {
        const qreal space = 2;
        const qreal dot = 1;
        const qreal dash = 4;

        switch (d->style) {
        case Qt::DashLine:
            d->dashPattern << dash << space;
            break;
        case Qt::DotLine:
            d->dashPattern << dot << space;
            break;
        case Qt::DashDotLine:
            d->dashPattern << dash << space << dot << space;
            break;
        case Qt::DashDotDotLine:
            d->dashPattern << dash << space << dot << space << dot << space;
            break;
        default:
            break;
        }
    }
    return d->dashPattern;
}

/*!
    Sets the dash pattern for this pen to the given \a pattern. This
    implicitly converts the style of the pen to Qt::CustomDashLine.

    The pattern must be specified as an even number of entries where
    the entries 1, 3, 5... are the dashes and 2, 4, 6... are the
    spaces. For example:

    \table 100%
    \row
    \o \inlineimage qpen-custom.png
    \o
    \code
    QPen pen;
    QVector<qreal> dashes;
    qreal space = 4;
    dashes << 1 << space << 3 << space << 9 << space
               << 27 << space << 9;
    pen.setDashPattern(dashes);
    \endcode
    \endtable

    The dash pattern is specified in units of the pens width, e.g. a
    dash of length 5 in width 10 is 50 pixels long. Each dash is also
    subject to cap styles so a dash of 1 with square cap set will
    extend 0.5 pixels out in each direction resulting in a total width
    of 2.

    \sa setStyle(), dashPattern()
 */
void QPen::setDashPattern(const QVector<qreal> &pattern)
{
    if (pattern.isEmpty())
        return;
    detach();
    d->dashPattern = pattern;
    d->style = Qt::CustomDashLine;

    if ((d->dashPattern.size() % 2) == 1) {
        qWarning("QPen::setDashPattern: Pattern not of even length");
        d->dashPattern << 1;
    }
}

/*!
    Returns the miter limit of the pen. The miter limit is only
    relevant when the join style is set to Qt::MiterJoin.

    \sa setMiterLimit(),  {QPen#Join Style}{Join Style}
*/
qreal QPen::miterLimit() const
{
    return d->miterLimit;
}

/*!
    Sets the miter limit of this pen to the given \a limit.

    \image qpen-miterlimit.png

    The miter limit describes how far a miter join can extend from the
    join point. This is used to reduce artifacts between line joins
    where the lines are close to parallel.

    This value does only have effect when the pen style is set to
    Qt::MiterJoin. The value is specified in units of the pen's width,
    e.g. a miter limit of 5 in width 10 is 50 pixels long. The default
    miter limit is 2, i.e. twice the pen width in pixels.

    \sa miterLimit(), setJoinStyle(), {QPen#Join Style}{Join Style}
*/
void QPen::setMiterLimit(qreal limit)
{
    detach();
    d->miterLimit = limit;
}


/*!
    \fn qreal QPen::width() const

    Returns the pen width with integer precision.

    \sa setWidth(), widthF()
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

    Sets the pen width to the given  \a width with integer point precision.

    A line width of zero indicates a cosmetic pen. This means that the
    pen width is always drawn one pixel wide, independent of the \l
    {QPainter#Coordinate Transformations}{transformation} set on the
    painter.

    Setting a pen width with a negative value is not supported.

    \sa setWidthF(), width()
*/
void QPen::setWidth(int width)
{
    if (width < 0)
        qWarning("QPen::setWidth: Setting a pen width with a negative value is not defined");
    if ((qreal)width == d->width)
        return;
    detach();
    d->width = width;
}

/*!
    Sets the pen width to the given \a width with floating point precision.

    A line width of zero indicates a cosmetic pen. This means that the
    pen width is always drawn one pixel wide, independent of the \l
    {QPainter#Coordinate Transformations}{transformation} on the
    painter.

    Setting a pen width with a negative value is not supported.

    \sa setWidth() widthF()
*/

void QPen::setWidthF(qreal width)
{
    if (width < 0.f)
        qWarning("QPen::setWidthF: Setting a pen width with a negative value is not defined");
    if (qAbs(d->width - width) < 0.00000001f)
        return;
    detach();
    d->width = width;
}


/*!
    Returns the pen's cap style.

    \sa setCapStyle(), {QPen#Cap Style}{Cap Style}
*/
Qt::PenCapStyle QPen::capStyle() const
{
    return d->capStyle;
}

/*!
    \fn void QPen::setCapStyle(Qt::PenCapStyle style)

    Sets the pen's cap style to the given \a style. The default value
    is Qt::SquareCap.

    \sa capStyle(), {QPen#Cap Style}{Cap Style}
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

    \sa setJoinStyle(),  {QPen#Join Style}{Join Style}
*/
Qt::PenJoinStyle QPen::joinStyle() const
{
    return d->joinStyle;
}

/*!
    \fn void QPen::setJoinStyle(Qt::PenJoinStyle style)

    Sets the pen's join style to the given \a style. The default value
    is Qt::BevelJoin.

    \sa joinStyle(), {QPen#Join Style}{Join Style}
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

    Returns the color of this pen's brush.

    \sa brush(), setColor()
*/
QColor QPen::color() const
{
    return d->brush.color();
}

/*!
    \fn void QPen::setColor(const QColor &color)

    Sets the color of this pen's brush to the given \a color.

    \sa setBrush(), color()
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

    \sa brush(), setColor()
*/
void QPen::setBrush(const QBrush &brush)
{
    detach();
    d->brush = brush;
}


/*!
    Returns true if the pen has a solid fill, otherwise false.

    \sa style(), dashPattern()
*/
bool QPen::isSolid() const
{
    return d->brush.style() == Qt::SolidPattern;
}


/*!
    \fn bool QPen::operator!=(const QPen &pen) const

    Returns true if the pen is different from the given \a pen;
    otherwise false. Two pens are different if they have different
    styles, widths or colors.

    \sa operator==()
*/

/*!
    \fn bool QPen::operator==(const QPen &pen) const

    Returns true if the pen is equal to the given \a pen; otherwise
    false. Two pens are equal if they have equal styles, widths and
    colors.

    \sa operator!=()
*/

bool QPen::operator==(const QPen &p) const
{
    return (p.d == d) || (p.d->style == d->style
                          && p.d->capStyle == d->capStyle
                          && p.d->joinStyle == d->joinStyle
                          && p.d->width == d->width
                          && p.d->miterLimit == d->miterLimit
                          && (d->style != Qt::CustomDashLine
                              || p.dashPattern() == dashPattern())
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
    \fn QDataStream &operator<<(QDataStream &stream, const QPen &pen)
    \relates QPen

    Writes the given \a pen to the given \a stream and returns a reference to
    the \a stream.

    \sa {Format of the QDataStream Operators}
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
        s << p.miterLimit();
        s << p.dashPattern();
    }
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QPen &pen)
    \relates QPen

    Reads a pen from the given \a stream into the given \a pen and
    returns a reference to the \a stream.

    \sa {Format of the QDataStream Operators}
*/

QDataStream &operator>>(QDataStream &s, QPen &p)
{
    quint8 style;
    quint8 width8 = 0;
    double width = 0;
    QColor color;
    QBrush brush;
    double miterLimit = 2;
    QVector<qreal> dashPattern;
    s >> style;
    if (s.version() < 7) {
        s >> width8;
        s >> color;
        brush = color;
        width = width8;
    } else {
        s >> width;
        s >> brush;
        s >> miterLimit;
        s >> dashPattern;
    }

    p.detach();
    p.d->width = width;
    p.d->brush = brush;
    p.d->style = Qt::PenStyle(style & Qt::MPenStyle);
    p.d->capStyle = Qt::PenCapStyle(style & Qt::MPenCapStyle);
    p.d->joinStyle = Qt::PenJoinStyle(style & Qt::MPenJoinStyle);
    p.d->dashPattern = dashPattern;
    p.d->miterLimit = miterLimit;

    return s;
}
#endif //QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QPen &p)
{
#ifndef Q_BROKEN_DEBUG_STREAM
    dbg.nospace() << "QPen(" << p.width() << ',' << p.brush()
                  << ',' << int(p.style()) << ',' << int(p.capStyle())
                  << ',' << int(p.joinStyle()) << ',' << p.dashPattern()
                  << ',' << p.miterLimit() << ')';
    return dbg.space();
#else
    qWarning("This compiler doesn't support streaming QPen to QDebug");
    return dbg;
    Q_UNUSED(p);
#endif
}
#endif

