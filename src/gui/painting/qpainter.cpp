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

// QtGui
#include "qbitmap.h"
#include "qimage.h"
#include "qpaintdevice.h"
#include "qpaintdevicemetrics.h"
#include "qpaintengine.h"
#include "qpainter.h"
#include "qpainter_p.h"
#include "qpainterpath.h"
#include "qpicture.h"
#include "qpolygon.h"
#include "qtextlayout.h"
#include "qwidget.h"

#include <private/qfontengine_p.h>
#include <private/qpaintengine_p.h>
#include <private/qpainterpath_p.h>
#include <private/qtextengine_p.h>
#include <private/qwidget_p.h>

// QtCore
#include <qdebug.h>

// Other
#include <math.h>

#define d d_func()
#define q q_func()

// #define QT_DEBUG_DRAW
#ifdef QT_DEBUG_DRAW
bool qt_show_painter_debug_output = true;
#endif

extern QPixmap qt_pixmapForBrush(int style, bool invert);

void qt_format_text(const QFont &font,
                    const QRectF &_r, int tf, const QString& str, int len, QRectF *brect,
                    int tabstops, int* tabarray, int tabarraylen,
                    QPainter *painter);


void qt_fill_linear_gradient(const QRect &r, QPainter *pixmap, const QBrush &brush);

QPolygon QPainterPrivate::draw_helper_xpolygon(const void *data, ShapeType shape)
{
    switch (shape) {
    case EllipseShape: {
        QPainterPath path;
        path.addEllipse(*reinterpret_cast<const QRectF*>(data));
        return path.toSubpathPolygons().first() * state->matrix;
    }
    case RectangleShape:
        return QPolygon(*reinterpret_cast<const QRectF*>(data)) * state->matrix;
    case PathShape:
        Q_ASSERT("should not happend...");
        return QPolygon();
    default:
        return (*reinterpret_cast<const QPolygon*>(data)) * state->matrix;
    }
}


/*
  Sets the shape as a clipregion on the painter
*/
QRect QPainterPrivate::draw_helper_setclip(const void *data, Qt::FillRule fillRule, ShapeType shape)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf(" -> setclip()\n");
#endif
    QRegion clip;
    switch (shape) {
    case EllipseShape:
        clip = QRegion(reinterpret_cast<const QRectF*>(data)->toRect(), QRegion::Ellipse);
        break;
    case PathShape:
        clip = QRegion(reinterpret_cast<const QPainterPath*>(data)->toFillPolygon().toPointArray(),
                       reinterpret_cast<const QPainterPath*>(data)->fillRule());
        break;
    case RectangleShape:
        clip = QRegion(reinterpret_cast<const QRectF*>(data)->toRect());
        break;
    default:
        clip = QRegion(reinterpret_cast<const QPolygon*>(data)->toPointArray(), fillRule);
        break;
    }
    q->setClipRegion(clip, Qt::IntersectClip);
    return clip.boundingRect();
}


/*
  Fallback implementation of fill linear gradient. Sets a clipregion matching the shape to
  fill and calls qt_fill_linear_gradient (lots of drawlines) for the region in question
*/
void QPainterPrivate::draw_helper_fill_lineargradient(const void *data, Qt::FillRule fillRule,
                                                      ShapeType type)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf(" -> fill_lineargradient()\n");
#endif
    q->save();
    QRect bounds = draw_helper_setclip(data, fillRule, type);
    qt_fill_linear_gradient(bounds, q, state->brush);
    q->restore();
}


/*
  Fallback implementation of fill alpha. Creates an alpha pixmap and sets a clipmask matching
  the primitive type to fill (no clip for rects though), then tiles the pixmap across the
  area.
*/
void QPainterPrivate::draw_helper_fill_alpha(const void *data, Qt::FillRule fillRule, ShapeType type)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf(" -> fill_alpha()\n");
#endif
    const int BUFFERSIZE = 16;
    QImage image(BUFFERSIZE, BUFFERSIZE, 32);
    image.fill(state->brush.color().rgba());
    image.setAlphaBuffer(true);
    QPixmap pm(image);

    QRect bounds;

    if (type != RectangleShape) {
        q->save();
        bounds = draw_helper_setclip(data, fillRule, type);
    } else {
        bounds = reinterpret_cast<const QRectF *>(data)->toRect();
    }

    q->drawTiledPixmap(bounds, pm);

    if (type != RectangleShape)
        q->restore();
}


/*
  Fallback implementation of fill patterns.
*/
void QPainterPrivate::draw_helper_fill_pattern(const void *data, Qt::FillRule fillRule, ShapeType type)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf(" -> fill_pattern()\n");
#endif
    q->save();
    q->setPen(QPen(state->brush.color(), 1));

    QRect bounds;
    if (type != RectangleShape) {
        bounds = draw_helper_setclip(data, fillRule, type);
    } else {
        bounds = reinterpret_cast<const QRectF *>(data)->toRect();
    }

    switch (state->brush.style()) {
    case Qt::HorPattern:
        if (state->bgMode == Qt::OpaqueMode)
            q->fillRect(bounds, state->bgBrush);
        for (int y=bounds.top(); y<bounds.bottom(); y+=8)
            q->drawLine(bounds.left(), y, bounds.right(), y);
        break;
    case Qt::VerPattern:
        if (state->bgMode == Qt::OpaqueMode)
            q->fillRect(bounds, state->bgBrush);
        for (int x=bounds.left(); x<bounds.right(); x+=8)
            q->drawLine(x, bounds.top(), x, bounds.bottom());
        break;
    case Qt::CrossPattern:
        if (state->bgMode == Qt::OpaqueMode)
            q->fillRect(bounds, state->bgBrush);
        for (int x=bounds.left(); x<bounds.right(); x+=8)
            q->drawLine(x, bounds.top(), x, bounds.bottom());
        for (int y=bounds.top(); y<bounds.bottom(); y+=8)
            q->drawLine(bounds.left(), y, bounds.right(), y);
        break;
    default: {
        QPixmap pattern;
        if (state->brush.style() == Qt::CustomPattern)
            pattern = *state->brush.pixmap();
        else
            pattern = qt_pixmapForBrush(state->brush.style(), true);

        if (state->bgMode == Qt::TransparentMode && pattern.isQBitmap())
            pattern.setMask(*static_cast<QBitmap *>(&pattern));

        if (state->bgMode == Qt::OpaqueMode
            && state->brush.style() != Qt::CustomPattern
            && state->brush.style() != Qt::LinearGradientPattern) {
            q->fillRect(bounds, state->bgBrush);
        }

        q->drawTiledPixmap(bounds, pattern);
    }
    }

    q->restore();
}


/*!
  Fallback implementation for stroking. Will transform coordinates if necesary.
*/
void QPainterPrivate::draw_helper_stroke_normal(const void *data, ShapeType shape, uint emulate)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf(" -> stroke normal()\n");
#endif
    QBrush originalBrush = state->brush;
    q->setBrush(Qt::NoBrush);
    engine->updateState(state);
    if (shape == PathShape) {
        Q_ASSERT_X(engine->hasFeature(QPaintEngine::PainterPaths), "draw_helper",
                   "PathShape is only used when engine supports painterpaths.");
        engine->drawPath(emulate & QPaintEngine::CoordTransform
                         ? *reinterpret_cast<const QPainterPath *>(data) * state->matrix
                         : *reinterpret_cast<const QPainterPath *>(data));
    } else {
        if (emulate & QPaintEngine::CoordTransform) {
            // Engine cannot transform
            QPolygon xformed = draw_helper_xpolygon(data, shape);
            engine->drawPolygon(xformed, QPaintEngine::PolylineMode);
        } else {
            // Engine does transformation
            if (shape == EllipseShape)
                engine->drawEllipse(*reinterpret_cast<const QRectF *>(data));
            else if (shape == RectangleShape)
                engine->drawRect(*reinterpret_cast<const QRectF *>(data));
            else
                engine->drawPolygon(*reinterpret_cast<const QPolygon *>(data),
                                    QPaintEngine::PolylineMode);
        }
    }
    q->setBrush(originalBrush);
}


/*!
  Fallback implementation for path based stroking. This function is used to emulate
  pen width transformation.
*/
void QPainterPrivate::draw_helper_stroke_pathbased(const void *data, ShapeType shape)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf(" -> stroke pathbased()\n");
#endif
    QPainterPath path;
    switch (shape) {
    case EllipseShape:
        path.addEllipse(*reinterpret_cast<const QRectF*>(data));
        break;
    case RectangleShape:
        path.addRect(*reinterpret_cast<const QRectF*>(data));
        break;
    case PathShape:
        path = *reinterpret_cast<const QPainterPath*>(data);
        break;
    default:
        path.addPolygon(*reinterpret_cast<const QPolygon*>(data));
        break;
    }

    bool doRestore = false;
    int width = state->pen.width();
    if (state->pen.width() == 0) {
        if (state->txop != TxNone && state->pen.width() == 0) {
            path = path * state->worldMatrix;
            q->save();
            q->resetMatrix();
            doRestore = true;
        }
        width = 1;
    }

    QPainterPathStroker stroker;
    stroker.setWidth(width);
    stroker.setStyle(state->pen.style());
    stroker.setCapStyle(state->pen.capStyle());
    stroker.setJoinStyle(state->pen.joinStyle());

    q->fillPath(stroker.createStroke(path), QBrush(state->pen.color()));

    if (doRestore)
        q->restore();
}


// ### make this inline when the X11 define has been removed
void QPainterPrivate::draw_helper(const void *data, Qt::FillRule fillRule, ShapeType type,
                                  DrawOperation operation)
{
    draw_helper(data, fillRule, type, operation, engine->emulationSpecifier);
}

void QPainterPrivate::draw_helper(const void *data, Qt::FillRule fillRule, ShapeType shape,
                                  DrawOperation op, uint emulationSpecifier)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output) {
        printf("QPainter::drawHelper: fillRule=%d, shape=%d, op=%d, emulation=0x%x ( ",
               fillRule, shape, op, emulationSpecifier);
        static struct { uint value; const char *text; } emuMap[] = {
            {    0x0001, "CoordTransform "},
            {    0x0002, "PenWidthTransform "},
            {    0x0004, "PatternTransform "},
            {    0x0008, "PatternBrush "},
            {    0x0010, "PixmapTransform "},
            {    0x0020, "LinearGradients "},
            {    0x0040, "LinearGradientFillPolygon "},
            {    0x0080, "PixmapScale "},
            {    0x0100, "AlphaFill "},
            {    0x0200, "AlphaFillPolygon "},
            {    0x0400, "AlphaStroke "},
            {    0x0800, "AlphaPixmap "},
            {    0x1000, "PainterPaths "},
            {    0x2000, "ClipTransform "},
            {0x10000000, "UsesFontEngine "},
            {0x20000000, "PaintOutsidePaintEvent "},
            {       0x0, 0x0},
        };
        for (int i = 0; emuMap[i].text; ++i)
            if (emulationSpecifier & emuMap[i].value)
                printf(emuMap[i].text);
        printf(")\n");
    }
#endif
    enum { Normal, PathBased, None } outlineMode = Normal;
    if (state->pen.style() == Qt::NoPen)
        op = DrawOperation(op&~StrokeDraw);
    if (state->brush.style() == Qt::NoBrush)
        op = DrawOperation(op&~FillDraw);

    // Creates an outline path to handle the stroking.
    if ((op & StrokeDraw)
        && (emulationSpecifier & QPaintEngine::PenWidthTransform
            || emulationSpecifier & QPaintEngine::AlphaStroke
            || emulationSpecifier & QPaintEngine::LineAntialiasing))
        outlineMode = PathBased;

    if (op & FillDraw) {

        // Custom fill, gradients, alpha and patterns
        if ((emulationSpecifier & QPaintEngine::LinearGradients)
            || (emulationSpecifier & QPaintEngine::AlphaFill)
            || (emulationSpecifier & QPaintEngine::PatternTransform))
        {
            if (((emulationSpecifier & QPaintEngine::LinearGradients)
                 && engine->hasFeature(QPaintEngine::LinearGradientFillPolygon))
                || ((emulationSpecifier & QPaintEngine::AlphaFill)
                    && engine->hasFeature(QPaintEngine::AlphaFillPolygon)))
            {
                if (shape == PathShape) {
                    q->drawPath(*reinterpret_cast<const QPainterPath *>(data));
                } else {
                    QPolygon pg = draw_helper_xpolygon(data, shape);
                    QPen old_pen = state->pen;
                    if (old_pen.style() != Qt::NoPen) {
                        state->pen = Qt::NoPen;
                        engine->setDirty(QPaintEngine::DirtyPen);
                        engine->updateState(state);
                    }
                    engine->drawPolygon(pg, QPaintEngine::PolygonDrawMode(fillRule));
                    if (old_pen.style() != Qt::NoPen) {
                        state->pen = old_pen;
                        engine->setDirty(QPaintEngine::DirtyPen);
                    }
                }
            } else {
                if (emulationSpecifier & QPaintEngine::LinearGradients)
                    draw_helper_fill_lineargradient(data, fillRule, shape);
                else if (emulationSpecifier & QPaintEngine::AlphaFill)
                    draw_helper_fill_alpha(data, fillRule, shape);
                else if (emulationSpecifier & QPaintEngine::PatternTransform)
                    draw_helper_fill_pattern(data, fillRule, shape);
            }

            // XFormed fills
        } else if (emulationSpecifier & QPaintEngine::CoordTransform) {
            q->save();
            if (outlineMode == PathBased)
                q->setPen(Qt::NoPen);
            else
                outlineMode = None;
            if (shape == PathShape) {
                Q_ASSERT_X(engine->hasFeature(QPaintEngine::PainterPaths), "draw_helper",
                           "PathShape is only used when engine supports painterpaths.");
                QPainterPath pathCopy = *reinterpret_cast<const QPainterPath *>(data);
                if (emulationSpecifier & QPaintEngine::CoordTransform)
                    pathCopy = pathCopy * state->matrix;
                q->resetMatrix();
                engine->updateState(state);
                engine->drawPath(pathCopy);
            } else {
                QPolygon xformed = draw_helper_xpolygon(data, shape);
                q->resetMatrix();
                engine->updateState(state);
                engine->drawPolygon(xformed, QPaintEngine::PolygonDrawMode(fillRule));
            }
            q->restore();

            // Normal fills, custom outlining only, which is done later.
        } else {
            QPen oldPen = state->pen;
            q->setPen(Qt::NoPen);
            engine->updateState(state);
            switch (shape) {
            case EllipseShape:
                engine->drawEllipse(*reinterpret_cast<const QRectF*>(data));
                break;
            case RectangleShape:
                engine->drawRect(*reinterpret_cast<const QRectF*>(data));
                break;
            case PathShape: {
                const QPainterPath *path = reinterpret_cast<const QPainterPath*>(data);
                if (engine->hasFeature(QPaintEngine::PainterPaths)) {
                    engine->drawPath(*path);
                } else {
                    QList<QPolygon> polys = path->toFillPolygons();
                    for (int i=0; i<polys.size(); ++i)
                        engine->drawPolygon(polys.at(i), QPaintEngine::PolygonDrawMode(fillRule));
                }
                break;
            }
            default:
                engine->drawPolygon(*reinterpret_cast<const QPolygon*>(data),
                                    QPaintEngine::PolygonDrawMode(fillRule));
                break;
            }
        }
    } // end of filling

    // Do the outlining...
    if (op & StrokeDraw) {
        if (outlineMode == Normal && state->pen.style() != Qt::NoPen) {
            draw_helper_stroke_normal(data, shape, emulationSpecifier);
        } else if (outlineMode == PathBased) {
            draw_helper_stroke_pathbased(data, shape);
        }
    } // end of stroking
}


void QPainterPrivate::init()
{
    state->painter = q;
}

void QPainterPrivate::updateMatrix()
{
    QMatrix m;
    if (state->VxF) {
        double scaleW = (double)state->vw/(double)state->ww;
        double scaleH = (double)state->vh/(double)state->wh;
        m.setMatrix(scaleW, 0, 0, scaleH, state->vx - state->wx*scaleW, state->vy - state->wy*scaleH);
    }
    if (state->WxF) {
        if (state->VxF)
            m = state->worldMatrix * m;
        else
            m = state->worldMatrix;
    }
    state->matrix = m;

    txinv = false;                                // no inverted matrix
    state->txop  = TxNone;
    if (state->matrix.m12()==0.0 && state->matrix.m21()==0.0
        && state->matrix.m11() >= 0.0 && state->matrix.m22() >= 0.0) {
        if (state->matrix.m11()==1.0 && state->matrix.m22()==1.0) {
            if (state->matrix.dx()!=0.0 || state->matrix.dy()!=0.0)
                state->txop = TxTranslate;
        } else {
            state->txop = TxScale;
        }
    } else {
        state->txop = TxRotShear;
    }
    if (!redirection_offset.isNull()) {
        state->txop |= TxTranslate;
        state->WxF = true;
        // We want to translate in dev space so we do the adding of the redirection
        // offset manually.
        state->matrix = QMatrix(state->matrix.m11(), state->matrix.m12(),
                              state->matrix.m21(), state->matrix.m22(),
                              state->matrix.dx()-redirection_offset.x(),
                              state->matrix.dy()-redirection_offset.y());
    }
    engine->setDirty(QPaintEngine::DirtyTransform);
//     printf("VxF=%d, WxF=%d\n", state->VxF, state->WxF);
//     printf("Using matrix: %f, %f, %f, %f, %f, %f\n",
//            state->matrix.m11(),
//            state->matrix.m12(),
//            state->matrix.m21(),
//            state->matrix.m22(),
//            state->matrix.dx(),
//            state->matrix.dy());
}

/*! \internal */
void QPainterPrivate::updateInvMatrix()
{
    Q_ASSERT(txinv == false);
    txinv = true;                                // creating inverted matrix
    bool invertible;
    QMatrix m;
    if (state->VxF) {
        m.translate(state->vx, state->vy);
        m.scale(1.0*state->vw/state->ww, 1.0*state->vh/state->wh);
        m.translate(-state->wx, -state->wy);
    }
    if (state->WxF) {
        if (state->VxF)
            m = state->worldMatrix * m;
        else
            m = state->worldMatrix;
    }
    invMatrix = m.invert(&invertible);                // invert matrix
}

/*!
    \enum Qt::PixmapDrawingMode

    \value ComposePixmap This mode will merge the source with the
    destination, including the alpha channels.

    \value CopyPixmap Copies the source to the destination, including
    the mask. If the destination is not a pixmap, this operation is
    undefined.

    \value CopyPixmapNoMask Draws the source onto the destination, ignoring
    the source mask.
*/

/*!
    \enum Qt::FillRule

    Specifies which method should be used to fill the paths and polygons.

    \value OddEvenFill Specifies that the region is filled using the
    odd even fill rule. With this rule, we determine whether a point
    is inside the shape by using the following method.
    Draw a horizontal line from the point to a location outside the shape,
    and count the number of intersections. If the number of intersections
    is an odd number, the point is inside the shape. This mode is the
    default.

    \value WindingFill Specifies that the region is filled using the
    non zero winding rule. With this rule, we determine whether a
    point is inside the shape by using the following method.
    Draw a horizontal line from the point to a location outside the shape.
    Determine whether the direction of the line at each intersection point
    is up or down. The winding number is determined by summing the
    direction of each intersection. If the number is non zero, the point
    is inside the shape. This fill mode can also in most cases be considered
    as the intersection of closed shapes.
*/

/*!
    \class QPainter qpainter.h
    \brief The QPainter class does low-level painting e.g. on widgets.

    \ingroup multimedia
    \mainclass

    The painter provides highly optimized functions to do most of the
    drawing GUI programs require. QPainter can draw everything from
    simple lines to complex shapes like pies and chords. It can also
    draw aligned text and pixmaps. Normally, it draws in a "natural"
    coordinate system, but it can also do view and world
    transformation.

    The typical use of a painter is:

    \list
    \i Construct a painter.
    \i Set a pen, a brush etc.
    \i Draw.
    \i Destroy the painter.
    \endlist

    Mostly, all this is done inside a paint event. (In fact, 99% of
    all QPainter use is in a reimplementation of
    QWidget::paintEvent(), and the painter is heavily optimized for
    such use.) Here's one very simple example:

    \code
    void SimpleExampleWidget::paintEvent()
    {
        QPainter paint(this);
        paint.setPen(Qt::blue);
        paint.drawText(rect(), Qt::AlignCenter, "The Text");
    }
    \endcode

    If you need to draw a complex shape, especially if you need to do
    so repeatedly, consider creating a QPainterPath and drawing it
    using drawPath().

    Usage is simple, and there are many settings you can use:

    \list

    \i font() is the currently set font. If you set a font that isn't
    available, Qt finds a close match. In fact font() returns what
    you set using setFont() and fontInfo() returns the font actually
    being used (which may be the same).

    \i brush() is the currently set brush; the color or pattern that's
    used for filling e.g. circles.

    \i pen() is the currently set pen; the color or stipple that's
    used for drawing lines or boundaries.

    \i backgroundMode() is \c Opaque or \c Transparent, i.e. whether
    backgroundColor() is used or not.

    \i backgroundColor() only applies when backgroundMode() is Opaque
    and pen() is a stipple. In that case, it describes the color of
    the background pixels in the stipple.

    \i brushOrigin() is the origin of the tiled brushes, normally the
    origin of the window.

    \i viewport(), window(), matrix() and many more make up the
    painter's coordinate transformation system. See \link
    coordsys.html The Coordinate System \endlink for an explanation of
    this, or see below for a very brief overview of the functions.

    \i hasClipping() is whether the painter clips at all. (The paint
    device clips, too.) If the painter clips, it clips to clipRegion().

    \i pos() is the current position, set by moveTo() and used by
    lineTo().

    \endlist

    Note that some of these settings mirror settings in some paint
    devices, e.g. QWidget::font(). QPainter::begin() (or the QPainter
    constructor) copies these attributes from the paint device.
    Calling, for example, QWidget::setFont() doesn't take effect until
    the next time a painter begins painting on it.

    save() saves all of these settings on an internal stack, restore()
    pops them back.

    The core functionality of QPainter is drawing, and there are
    functions to draw most primitives: drawPoint(), drawPoints(),
    drawLine(), drawRect(), drawRoundRect(),
    drawEllipse(), drawArc(), drawPie(), drawChord(),
    drawLine:Segments(), drawPolyline(), drawPolygon(),
    drawConvexPolygon() and drawCubicBezier(). All of these functions
    take integer coordinates; there are no floating-point versions
    since we want drawing to be as fast as possible.

    \table
    \row \i
    \inlineimage qpainter-angles.png
    \i The functions that draw curved primitives accept angles measured
    in 1/16s of a degree. Angles are measured in an counter-clockwise
    direction from the rightmost edge of the primitive being drawn.
    \endtable

    There are functions to draw pixmaps/images, namely drawPixmap(),
    drawImage() and drawTiledPixmap(). drawPixmap() and drawImage()
    produce the same result, except that drawPixmap() is faster
    on-screen while drawImage() may be faster on QPrinter and other
    devices.

    Text drawing is done using drawText(), and when you need
    fine-grained positioning, boundingRect() tells you where a given
    drawText() command would draw.

    There is a drawPicture() function that draws the contents of an
    entire QPicture using this painter. drawPicture() is the only
    function that disregards all the painter's settings: the QPicture
    has its own settings.

    Normally, the QPainter operates on the device's own coordinate
    system (usually pixels), but QPainter has good support for
    coordinate transformation. See \link coordsys.html The Coordinate
    System \endlink for a more general overview and a simple example.

    The most common functions used are scale(), rotate(), translate()
    and shear(), all of which operate on the matrix().
    setMatrix() can replace or add to the currently set
    matrix().

    setViewport() sets the rectangle on which QPainter operates. The
    default is the entire device, which is usually fine, except on
    printers. setWindow() sets the coordinate system, that is, the
    rectangle that maps to viewport(). What's drawn inside the
    window() ends up being inside the viewport(). The window's
    default is the same as the viewport, and if you don't use the
    transformations, they are optimized away, gaining another little
    bit of speed.

    After all the coordinate transformation is done, QPainter can clip
    the drawing to an arbitrary rectangle or region. hasClipping() is
    true if QPainter clips, and clipRegion() returns the clip region.
    You can set it using either setClipRegion() or setClipRect().
    Note that the clipping can be slow. It's all system-dependent,
    but as a rule of thumb, you can assume that drawing speed is
    inversely proportional to the number of rectangles in the clip
    region.

    After QPainter's clipping, the paint device may also clip. For
    example, most widgets clip away the pixels used by child widgets,
    and most printers clip away an area near the edges of the paper.
    This additional clipping is not reflected by the return value of
    clipRegion() or hasClipping().

    QPainter also includes some less-used functions that are very
    useful on those occasions when they're needed.

    isActive() indicates whether the painter is active. begin() (and
    the most usual constructor) makes it active. end() (and the
    destructor) deactivates it. If the painter is active, device()
    returns the paint device on which the painter paints.

    Sometimes it is desirable to make someone else paint on an unusual
    QPaintDevice. QPainter supports a static function to do this,
    redirect(). We recommend not using it, but for some hacks it's
    perfect.

    setTabStops() and setTabArray() can change where the tab stops
    are, but these are very seldomly used.

    \warning A QPainter can only be used inside a paintEvent() or a
    function called by a paintEvent().

    \warning Note that QPainter does not attempt to work around
    coordinate limitations in the underlying window system. Some
    platforms may behave incorrectly with coordinates outside +/-4000.

    \headerfile qdrawutil.h

    \sa QPaintDevice QWidget QPixmap QPrinter QPicture
        \link simple-application.html Application Walkthrough \endlink
        \link coordsys.html Coordinate System Overview \endlink
*/

/*!
    \enum QPainter::RenderHint

    \internal

    \value LineAntialiasing
*/

/*!
    \enum QPainter::TextDirection

    \value Auto
    \value RTL right to left
    \value LTR left to right

    \sa drawText()
*/

/*!
    \fn QPaintEngine *QPaintDevice::paintEngine() const

    \internal
*/

/*!
    Constructs a painter.

    Notice that all painter settings (setPen, setBrush etc.) are reset
    to default values when begin() is called.

    \sa begin(), end()
*/

QPainter::QPainter()
{
    d_ptr = new QPainterPrivate(this);
    d->init();
}

/*!
    Constructs a painter that begins painting the paint device \a pd
    immediately.

    This constructor is convenient for short-lived painters, e.g. in a
    \link QWidget::paintEvent() paint event\endlink and should be used
    only once. The constructor calls begin() for you and the QPainter
    destructor automatically calls end().

    Here's an example using begin() and end():
    \code
        void MyWidget::paintEvent(QPaintEvent *)
        {
            QPainter p;
            p.begin(this);
            p.drawLine(...);        // drawing code
            p.end();
        }
    \endcode

    The same example using this constructor:
    \code
        void MyWidget::paintEvent(QPaintEvent *)
        {
            QPainter p(this);
            p.drawLine(...);        // drawing code
        }
    \endcode

    Since the constructor cannot provide feedback when the initialization
    of the painter failed you should rather use begin() and end() to paint
    on external devices, e.g. printers.

    \sa begin(), end()
*/

QPainter::QPainter(QPaintDevice *pd)
{
    d_ptr = new QPainterPrivate(this);
    d->init();
    Q_ASSERT(pd != 0);
    begin(pd);
}

/*!
    Destroys the painter.
*/

QPainter::~QPainter()
{
    if (isActive())
        end();
    delete d;
}

/*!
    Returns the paint device on which this painter is currently
    painting, or 0 if the painter is not active.

    \sa QPaintDevice::paintingActive()
*/

QPaintDevice *QPainter::device() const
{
    return d->device;
}

/*!
    Returns true if the painter is active painting, i.e. begin() has
    been called and end() has not yet been called; otherwise returns
    false.

    \sa QPaintDevice::paintingActive()
*/

bool QPainter::isActive() const
{
    if (d->engine) {
        return d->engine->isActive();
    }
    return false;
}

/*!
  Initializes the painters pen, background and font to the same as \a widget.
*/
void QPainter::initFrom(const QWidget *widget)
{
    Q_ASSERT_X(widget, "QPainter::initFrom(const QWidget *widget)", "Widget cannot be 0");
    QPalette pal = widget->palette();
    d->state->pen = pal.color(QPalette::Foreground);
    d->state->bgBrush = pal.background();
    d->state->font = widget->font();
    if (d->engine) {
        d->engine->setDirty(QPaintEngine::DirtyPen);
        d->engine->setDirty(QPaintEngine::DirtyBrush);
        d->engine->setDirty(QPaintEngine::DirtyFont);
    }
}


/*!
    Saves the current painter state (pushes the state onto a stack). A
    save() must be followed by a corresponding restore(). end()
    unwinds the stack.

    \sa restore()
*/

void QPainter::save()
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::save()\n");
#endif
    if (!isActive()) {
        qWarning("QPainter::save(), painter not active");
        return;
    }

    d->engine->syncState();

    d->state = new QPainterState(d->states.back());
    // We need top propagate pending changes to the restore later..
    d->state->changeFlags = d->engine->dirtyFlag;
    d->states.push_back(d->state);
    d->engine->updateState(d->state, false);
}

/*!
    Restores the current painter state (pops a saved state off the
    stack).

    \sa save()
*/

void QPainter::restore()
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::restore()\n");
#endif
    if (d->states.size()==0) {
        qWarning("QPainter::restore(), unbalanced save/restore");
        return;
    } else if (!isActive()) {
        qWarning("QPainter::restore(), painter not active");
        return;
    }

    QPainterState *tmp = d->state;
    d->states.pop_back();
    d->state = d->states.back();
    d->txinv = false;

    // trigger clip update if the clip path/region has changed since
    // last save
    if (!d->state->clipInfo.isEmpty()
        && (tmp->changeFlags & (QPaintEngine::DirtyClip | QPaintEngine::DirtyClipPath))) {
        d->state->tmpClipRegion = clipRegion();
        d->state->tmpClipOp = Qt::ReplaceClip;
        d->engine->setDirty(QPaintEngine::DirtyClip);
    }

    d->engine->updateState(d->state);
    delete tmp;
}


/*!
    Begins painting the paint device \a pd and returns true if
    successful; otherwise returns false.

    The errors that can occur are serious problems, such as these:

    \code
        p->begin(0); // impossible - paint device cannot be 0

        QPixmap pm(0, 0);
        p->begin(pm); // impossible - pm.isNull();

        p->begin(myWidget);
        p2->begin(myWidget); // impossible - only one painter at a time
    \endcode

    Note that most of the time, you can use one of the constructors
    instead of begin(), and that end() is automatically done at
    destruction.

    \warning A paint device can only be painted by one painter at a
    time.

    \sa end()
*/

bool QPainter::begin(QPaintDevice *pd)
{
    Q_ASSERT(pd);

    if (d->engine) {
        qWarning("QPainter::begin(): Painter is already active."
                 "\n\tYou must end() the painter before a second begin()");
        return false;
    }

    // Ensure fresh painter state
    d->state->init(d->state->painter);

    QPaintDevice *rpd = redirected(pd, &d->redirection_offset);

    if (rpd) {
        pd = rpd;
    }

#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::begin(), device=%p, type=%d\n", pd, pd->devType());
#endif


    d->state->bgOrigin -= d->redirection_offset; // This will accumulate!! ############

    d->device = pd;
    d->engine = pd->paintEngine();

    if (!d->engine) {
        qWarning("QPainter::begin(), paintdevice returned engine == 0, type: %d\n", pd->devType());
        return true;
    }

    switch (pd->devType()) {
        case QInternal::Widget:
        {
            const QWidget *widget = static_cast<const QWidget *>(pd);
            Q_ASSERT(widget);

            if(!d->engine->hasFeature(QPaintEngine::PaintOutsidePaintEvent)
               && !widget->testAttribute(Qt::WA_PaintOutsidePaintEvent)
               && !widget->testWState(Qt::WState_InPaintEvent)) {
                qWarning("QPainter::begin: Widget painting can only begin as a "
                         "result of a paintEvent");
                return false;
            }
            d->state->deviceFont = d->state->font = widget->font();
            d->state->pen = widget->palette().color(widget->foregroundRole());
            d->state->bgBrush = widget->palette().brush(widget->backgroundRole());
            d->state->ww = d->state->vw = widget->width();
            d->state->wh = d->state->vh = widget->height();
            break;
        }
        case QInternal::Pixmap:
        {
            const QPixmap *pm = static_cast<const QPixmap *>(pd);
            Q_ASSERT(pm);
            d->state->ww = d->state->vw = pm->width();
            d->state->wh = d->state->vh = pm->height();
            if (pm->depth() == 1) {
                d->state->pen = QPen(Qt::color1);
                d->state->brush = QBrush(Qt::color0);
            }
            break;
        }
        case QInternal::ExternalDevice:
        {
            d->state->ww = d->state->vw = pd->metric(QPaintDeviceMetrics::PdmWidth);
            d->state->wh = d->state->vh = pd->metric(QPaintDeviceMetrics::PdmHeight);
        }
    }

    if (d->state->ww == 0) // For compat with 3.x painter defaults
        d->state->ww = d->state->wh = d->state->vw = d->state->vh = 1024;

    // Slip a painter state into the engine before we do any other operations
    d->engine->state = d->state;

    if (!d->engine->begin(pd)) {
        qWarning("QPainter::begin(), QPaintEngine::begin() returned false\n");
        return false;
    }

    Q_ASSERT(d->engine->isActive());

    if (!d->redirection_offset.isNull())
        d->updateMatrix();

    Q_ASSERT(d->engine->isActive());
    d->engine->setRenderHint(QPainter::LineAntialiasing, false);
    d->engine->setRenderHint(QPainter::TextAntialiasing, true);
    ++d->device->painters;

    return true;
}

/*!
    Ends painting. Any resources used while painting are released. You
    don't normally need to call this since it is called by the
    destructor.

    \sa begin(), isActive()
*/

bool QPainter::end()
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::end()\n");
#endif

    if (!isActive()) {
        qWarning("QPainter::end: Painter is not active, aborted");
        return false;
    }

    if (d->states.size()>1) {
        qWarning("QPainter::end(), painter ended with %d saved states",
                 d->states.size());
    }

    bool ended = d->engine->end();
    d->engine->updateState(0);

    if (d->engine->autoDestruct()) {
        delete d->engine;
    }

    if (ended)
        d->engine = 0;

    --d->device->painters;
    return ended;
}


/*!
    Returns the paint engine that the painter is currently operating
    on, if the painter is active; otherwise 0.
*/
QPaintEngine *QPainter::paintEngine() const
{
    return d->engine;
}


/*!
    Returns the font metrics for the painter, if the painter is
    active. It is not possible to obtain metrics for an inactive
    painter, so the return value is undefined if the painter is not
    active.

    \sa fontInfo(), isActive()
*/

QFontMetrics QPainter::fontMetrics() const
{
    if (d->engine)
        d->engine->updateState(d->state);
    return QFontMetrics(d->state->pfont ? *d->state->pfont : d->state->font);
}


/*!
    Returns the font info for the painter, if the painter is active.
    It is not possible to obtain font information for an inactive
    painter, so the return value is undefined if the painter is not
    active.

    \sa fontMetrics(), isActive()
*/

QFontInfo QPainter::fontInfo() const
{
    if (d->engine)
        d->engine->updateState(d->state);
    return QFontInfo(d->state->pfont ? *d->state->pfont : d->state->font);
}

/*!
    Returns the brush origin currently set.

    \sa setBrushOrigin()
*/

QPoint QPainter::brushOrigin() const
{
    return QPointF(d->state->bgOrigin + d->redirection_offset).toPoint();
}

/*!
    \fn void QPainter::setBrushOrigin(int x, int y)

    \overload

    Sets the brush's origin to point (\a x, \a y).
*/

/*!
    Sets the brush origin to \a p.

    The brush origin specifies the (0, 0) coordinate of the painter's
    brush. This setting only applies to pattern brushes and pixmap
    brushes.

    \sa brushOrigin()
*/

void QPainter::setBrushOrigin(const QPointF &p)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::setBrushOrigin(), (%.2f,%.2f)\n", p.x(), p.y());
#endif
    d->state->bgOrigin = p - d->redirection_offset;
    if (d->engine)
        d->engine->setDirty(QPaintEngine::DirtyBrush);
}

/*!
    Returns the current background brush.

    \sa setBackground() QBrush
*/

const QBrush &QPainter::background() const
{
    return d->state->bgBrush;
}


/*!
    Returns true if clipping has been set; otherwise returns false.

    \sa setClipping()
*/

bool QPainter::hasClipping() const
{
    return d->state->tmpClipOp != Qt::NoClip;
}


/*!
    Enables clipping if \a enable is true, or disables clipping if \a
    enable is false.

    \sa hasClipping(), setClipRect(), setClipRegion()
*/

void QPainter::setClipping(bool enable)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::setClipping(), enable=%s, was=%s\n",
               enable ? "on" : "off",
               hasClipping() ? "on" : "off");
#endif
    if (!isActive()) {
        qWarning("QPainter::setClipping(), painter not active, state will be reset by begin");
        return;
    }

    if (hasClipping() == enable)
        return;

    if (enable) {
        d->state->tmpClipRegion = clipRegion();
        d->state->tmpClipOp = Qt::ReplaceClip;
    } else {
        d->state->tmpClipRegion = QRegion();
        d->state->tmpClipOp = Qt::NoClip;
    }
    d->engine->setDirty(QPaintEngine::DirtyClip);
    d->engine->updateState(d->state);
}


/*!
    Returns the currently set clip region. Note that the clip region
    is given in logical coordinates and \e subject to
    \link coordsys.html coordinate transformation \endlink.

    \sa setClipRegion(), setClipRect(), setClipping()
*/

QRegion QPainter::clipRegion() const
{
    if (!isActive()) {
        qWarning("QPainter::clipRegion(), painter not active");
        return QRegion();
    }

    QRegion region;
    bool lastWasNothing = true;

    if (!d->txinv)
        const_cast<QPainter *>(this)->d->updateInvMatrix();

    for (int i=0; i<d->state->clipInfo.size(); ++i) {
        const QPainterClipInfo &info = d->state->clipInfo.at(i);
        QRegion other;
        switch (info.clipType) {

        case QPainterClipInfo::RegionClip: {
            QMatrix matrix = (info.matrix * d->invMatrix);
            if (lastWasNothing) {
                region = info.region * matrix;
                lastWasNothing = false;
                continue;
            }
            if (info.operation == Qt::IntersectClip)
                region &= info.region * matrix;
            else if (info.operation == Qt::UniteClip)
                region |= info.region * matrix;
            else if (info.operation == Qt::NoClip) {
                lastWasNothing = true;
                region = QRegion();
            } else
                region = info.region * matrix;
            break;
        }

        case QPainterClipInfo::PathClip: {
            QMatrix matrix = (info.matrix * d->invMatrix);
            if (lastWasNothing) {
                region = QRegion((info.path * matrix).toFillPolygon().toPointArray(),
                                 info.path.fillRule());
                lastWasNothing = false;
                continue;
            }
            if (info.operation == Qt::IntersectClip) {
                region &= QRegion((info.path * matrix).toFillPolygon().toPointArray(),
                                  info.path.fillRule());
            } else if (info.operation == Qt::UniteClip) {
                region |= QRegion((info.path * matrix).toFillPolygon().toPointArray(),
                                  info.path.fillRule());
            } else if (info.operation == Qt::NoClip) {
                lastWasNothing = true;
                region = QRegion();
            } else {
                region = QRegion((info.path * matrix).toFillPolygon().toPointArray(),
                                 info.path.fillRule());
            }
            break;
        }
        }
    }

    return region;
}

/*!
    Returns the currently clip as a path. Note that the clip path is
    given in logical coordinates and \e subject to \link coordsys.html
    coordinate transformation \endlink
*/
QPainterPath QPainter::clipPath() const
{
    // ### Since we do not support path intersections and path unions yet,
    // we just use clipRegion() here...
    if (!isActive()) {
        qWarning("QPainter::clipPath(), painter not active");
        return QPainterPath();
    }

    // No clip, return empty
    if (d->state->clipInfo.size() == 0) {
        return QPainterPath();
    } else {

        // Update inverse matrix, used below.
        if (!d->txinv)
            const_cast<QPainter *>(this)->d->updateInvMatrix();

        // For the simple case avoid conversion.
        if (d->state->clipInfo.size() == 1
            && d->state->clipInfo.at(0).clipType == QPainterClipInfo::PathClip) {
            QMatrix matrix = (d->state->clipInfo.at(0).matrix * d->invMatrix);
            return d->state->clipInfo.at(0).path * matrix;

        // Fallback to clipRegion() for now, since we don't have isect/unite for paths
        } else {
            QPainterPath path;
            path.addRegion(clipRegion());
            return path;
        }
    }
}

/*!
    \fn void QPainter::setClipRect(int x, int y, int w, int h)

    Sets the clip region to the rectangle \a x, \a y, \a w, \a h and
    enables clipping.

    \sa setClipRegion(), clipRegion(), setClipping()
*/

/*!
    \overload

    Sets the clip region of the rectange \a rect.
*/
void QPainter::setClipRect(const QRectF &rect, Qt::ClipOperation op)
{
    QPainterPath path;
    path.addRect(rect);
    setClipPath(path, op);
}

/*!
    Sets the clip region to \a r and enables clipping.

    Note that the clip region is given in logical coordinates
    and \e subject to \link coordsys.html coordinate
    transformation.\endlink

    \sa setClipRect(), clipRegion(), setClipping()
*/

void QPainter::setClipRegion(const QRegion &r, Qt::ClipOperation op)
{
#ifdef QT_DEBUG_DRAW
    QRect rect = r.boundingRect();
    if (qt_show_painter_debug_output)
        printf("QPainter::setClipRegion(), size=%d, [%d,%d,%d,%d]\n",
           r.rects().size(), rect.x(), rect.y(), rect.width(), rect.height());
#endif
    Q_ASSERT(op != Qt::NoClip);
    if (!isActive()) {
        qWarning("QPainter::setClipRegion(); painter not active");
        return;
    }

    d->state->tmpClipRegion = r;
    d->state->tmpClipOp = op;
    if (op == Qt::NoClip || op == Qt::ReplaceClip)
        d->state->clipInfo.clear();
    d->state->clipInfo << QPainterClipInfo(r, op, d->state->worldMatrix);
    d->engine->setDirty(QPaintEngine::DirtyClip);
    d->engine->updateState(d->state);
}

/*!
    Sets the transformation matrix to \a matrix and enables transformations.

    If \a combine is true, then \a matrix is combined with the current
    transformation matrix; otherwise \a matrix replaces the current
    transformation matrix.

    If \a matrix is the identity matrix and \a combine is false, this
    function calls setMatrixEnabled(false). (The identity matrix is the
    matrix where QMatrix::m11() and QMatrix::m22() are 1.0 and the
    rest are 0.0.)

    World transformations are applied after the view transformations
    (i.e. \link setWindow() window\endlink and \link setViewport()
    viewport\endlink).

    The following functions can transform the coordinate system without using
    a QMatrix:
    \list
    \i translate()
    \i scale()
    \i shear()
    \i rotate()
    \endlist

    They operate on the painter's worldMatrix() and are implemented like this:

    \code
        void QPainter::rotate(double a)
        {
            QMatrix m;
            m.rotate(a);
            setMatrix(m, true);
        }
    \endcode

    Note that you should always use \a combine when you are drawing
    into a QPicture. Otherwise it may not be possible to replay the
    picture with additional transformations. Using translate(),
    scale(), etc., is safe.

    For a brief overview of coordinate transformation, see the \link
    coordsys.html Coordinate System Overview. \endlink

    \sa matrix(), setMatrixEnabled(), QMatrix
*/

void QPainter::setMatrix(const QMatrix &matrix, bool combine)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::setMatrix(), combine=%d\n", combine);
#endif

   if (!isActive()) {
        qWarning("QPainter::setMatrix(), painter not active ");
        return;
    }

    if (combine)
        d->state->worldMatrix = matrix * d->state->worldMatrix;                        // combines
    else
        d->state->worldMatrix = matrix;                                // set new matrix
//     bool identity = d->state->worldMatrix.isIdentity();
//     if (identity && pdev->devType() != QInternal::Picture)
//         setWorldXForm(false);
//     else
    if (!d->state->WxF)
        setMatrixEnabled(true);
    else
        d->updateMatrix();
}

/*!
    Returns the world transformation matrix.

    \sa setMatrix()
*/

const QMatrix &QPainter::matrix() const
{
    return d->state->worldMatrix;
}

/*!
    Resets any transformations that were made using translate(), scale(),
    shear(), rotate(), setMatrix(), setViewport() and
    setWindow().

    \sa matrix(), setMatrix()
*/
void QPainter::resetMatrix()
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::resetMatrix()\n");
#endif
    if (!isActive()) {
        qWarning("QPainter::resetMatrix(), painter not active");
        return;
    }

    d->state->wx = d->state->wy = d->state->vx = d->state->vy = 0;                        // default view origins
    d->state->ww = d->state->vw = d->device->metric(QPaintDeviceMetrics::PdmWidth);
    d->state->wh = d->state->vh = d->device->metric(QPaintDeviceMetrics::PdmHeight);
    d->state->worldMatrix = QMatrix();
    setMatrixEnabled(false);
    setViewXForm(false);
    if (d->engine)
        d->engine->setDirty(QPaintEngine::DirtyTransform);
}


/*!
    Enables transformations if \a enable is true, or disables
    world transformations if \a enable is false. The world
    transformation matrix is not changed.

    \sa setMatrix(), matrix()
*/

void QPainter::setMatrixEnabled(bool enable)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::setMatrixEnabled(), enable=%d\n", enable);
#endif

    if (!isActive()) {
        qWarning("QPainter::setMatrixEnabled(), painter not active");
        return;
    }
    if (enable == d->state->WxF)
        return;

    d->state->WxF = enable;
    d->updateMatrix();
}

/*!
    Returns true if world transformation is enabled; otherwise returns
    false.

    \sa setMatrixEnabled(), setMatrix()
*/

bool QPainter::matrixEnabled() const
{
#ifndef QT_NO_TRANSFORMATIONS
    return d->state->WxF;
#else
    return d->state->xlatex || d->state->xlatey;
#endif
}

#ifndef QT_NO_TRANSFORMATIONS
/*!
    Scales the coordinate system by (\a{sx}, \a{sy}).

    \sa translate(), shear(), rotate(), resetXForm(), setMatrix(),
    xForm()
*/

void QPainter::scale(double sx, double sy)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::scale(), sx=%f, sy=%f\n", sx, sy);
#endif

    QMatrix m;
    m.scale(sx, sy);
    setMatrix(m, true);
}

/*!
    Shears the coordinate system by (\a{sh}, \a{sv}).

    \sa translate(), scale(), rotate(), resetXForm(), setMatrix(),
    xForm()
*/

void QPainter::shear(double sh, double sv)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::shear(), sh=%f, sv=%f\n", sh, sv);
#endif
    QMatrix m;
    m.shear(sv, sh);
    setMatrix(m, true);
}

/*!
    Rotates the coordinate system \a a degrees clockwise.

    \sa translate(), scale(), shear(), resetXForm(), setMatrix(),
    xForm()
*/

void QPainter::rotate(double a)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::rotate(), angle=%f\n", a);
#endif
    QMatrix m;
    m.rotate(a);
    setMatrix(m, true);
}
#endif


/*!
    \fn void QPainter::translate(const QPoint &offset)

    \overload

    Translates the coordinate system by the given \a offset.
*/

/*!
    Translates the coordinate system by (\a{dx}, \a{dy}). After this call,
    (\a{dx}, \a{dy}) is added to points.

    For example, the following code draws the same point twice:
    \code
        void MyWidget::paintEvent()
        {
            QPainter paint(this);

            paint.drawPoint(0, 0);

            paint.translate(100.0, 40.0);
            paint.drawPoint(-100, -40);
        }
    \endcode

    \sa scale(), shear(), rotate(), resetXForm(), setMatrix(), xForm()
*/

void QPainter::translate(double dx, double dy)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::translate(), dx=%f, dy=%f\n", dx, dy);
#endif

#ifndef QT_NO_TRANSFORMATIONS
    QMatrix m;
    m.translate(dx, dy);
    setMatrix(m, true);
#else
    xlatex += (int)dx;
    xlatey += (int)dy;
    d->state->VxF = (bool)xlatex || xlatey;
#endif
}

/*!
    Sets the clippath for the painter to \a path.

    The clip path is specified in logical (painter) coordinates.

    \warning This function is not yet implemented.
*/
void QPainter::setClipPath(const QPainterPath &path, Qt::ClipOperation op)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output) {
        QRectF b = path.boundingRect();
        printf("QPainter::setClipPath(), size=%d, op=%d, bounds=[%.2f,%.2f,%.2f,%.2f]\n",
               path.elementCount(), op, b.x(), b.y(), b.width(), b.height());
    }
#endif
    Q_ASSERT(op != Qt::NoClip);

    if (!isActive()
        || (!hasClipping() && path.isEmpty()))
        return;

    d->state->tmpClipPath = path;
    d->state->tmpClipOp = op;
    if (op == Qt::NoClip || op == Qt::ReplaceClip)
        d->state->clipInfo.clear();
    d->state->clipInfo << QPainterClipInfo(path, op, d->state->worldMatrix);
    d->engine->setDirty(QPaintEngine::DirtyClipPath);
    d->engine->updateState(d->state);
}

/*!
    Draws the outline (strokes) the path \a path with the pen specified
    by \a pen
*/
void QPainter::strokePath(const QPainterPath &path, const QPen &pen)
{
    QBrush oldBrush = d->state->brush;
    QPen oldPen = d->state->pen;

    d->state->pen = pen;
    d->state->brush = Qt::NoBrush;
    d->engine->setDirty(QPaintEngine::DirtyFlags(QPaintEngine::DirtyPen | QPaintEngine::DirtyBrush));

    drawPath(path);

    // Reset old state
    d->state->pen = oldPen;
    d->state->brush = oldBrush;
    d->engine->setDirty(QPaintEngine::DirtyFlags(QPaintEngine::DirtyPen | QPaintEngine::DirtyBrush));
}

/*!
    Fills the path \a path using the given \a brush. The outline
    is not drawn.
*/
void QPainter::fillPath(const QPainterPath &path, const QBrush &brush)
{
    QBrush oldBrush = d->state->brush;
    QPen oldPen = d->state->pen;

    d->state->pen = Qt::NoPen;
    d->state->brush = brush;
    d->engine->setDirty(QPaintEngine::DirtyFlags(QPaintEngine::DirtyPen | QPaintEngine::DirtyBrush));

    drawPath(path);

    // Reset old state
    d->state->pen = oldPen;
    d->state->brush = oldBrush;
    d->engine->setDirty(QPaintEngine::DirtyFlags(QPaintEngine::DirtyPen | QPaintEngine::DirtyBrush));
}
/*!
    Draws the painter path specified by \a path using the current pen
    for outline and the current brush for filling.
*/
void QPainter::drawPath(const QPainterPath &path)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawPath(), size=%d\n", path.elementCount());
#endif

    if (!isActive())
	return;

    d->engine->updateState(d->state);

    if (d->engine->hasFeature(QPaintEngine::PainterPaths)) {
        if (d->engine->emulationSpecifier) {
            d->draw_helper(&path, path.fillRule(), QPainterPrivate::PathShape);
            return;
        }
        d->engine->drawPath(path);
        return;
    }

    QList<QPolygon> polygons = path.toSubpathPolygons();
    if (polygons.isEmpty())
        return;

    QMatrix worldMatrix = d->state->matrix;

    save();

    uint emulationSpecifier = d->engine->emulationSpecifier;
    if ((emulationSpecifier & QPaintEngine::LinearGradients)
        && d->engine->hasFeature(QPaintEngine::LinearGradientFillPolygon))
        emulationSpecifier &= ~QPaintEngine::LinearGradients;

    if ((emulationSpecifier & QPaintEngine::AlphaFill)
        && d->engine->hasFeature(QPaintEngine::AlphaFillPolygon))
        emulationSpecifier &= ~QPaintEngine::AlphaFill;

    // Fill the path...
    if (d->state->brush.style() != Qt::NoBrush) {
        QList<QPolygon> fills = path.toFillPolygons();
        QPen oldPen = d->state->pen;
        setPen(Qt::NoPen);
	d->engine->updateState(d->state);
        for (int i=0; i<fills.size(); ++i) {
            if (emulationSpecifier)
                d->draw_helper(&fills.at(i), path.fillRule(), QPainterPrivate::PolygonShape,
                               QPainterPrivate::StrokeAndFillDraw, emulationSpecifier);
            else
                d->engine->drawPolygon(fills.at(i), QPaintEngine::PolygonDrawMode(path.fillRule()));
        }
        setPen(oldPen);
    }

    // Draw the outline of the path...
    if (d->state->pen.style() != Qt::NoPen) {
	for (int i=0; i<polygons.size(); ++i) {
	    d->engine->updateState(d->state);
            if (d->engine->emulationSpecifier) {
                QPolygon polygon = polygons.at(i);
                d->draw_helper(&polygon, path.fillRule(), QPainterPrivate::LineShape,
                               QPainterPrivate::StrokeDraw);
            } else {
                d->engine->drawPolygon(polygons.at(i), QPaintEngine::PolylineMode);
            }
	}
    }

    restore();
}


/*!
    \fn void QPainter::drawLine(int x1, int y1, int x2, int y2)
    \overload

    Draws a line from (\a x1, \a y1) to (\a x2, \a y2) and sets the
    current pen position to (\a x2, \a y2).
*/

/*!
    Draws a line from point \a p1 to point \a p2.

    \sa pen()
*/

void QPainter::drawLine(const QLineF &l)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawLine(), p1=(%.2f,%.2f), p2=(%.2f,%.2f)\n",
               l.startX(), l.startY(), l.endX(), l.endY());
#endif

    if (!isActive())
        return;

    d->engine->updateState(d->state);

    uint lineEmulation = d->engine->emulationSpecifier
                         & (QPaintEngine::CoordTransform
                            | QPaintEngine::PenWidthTransform
                            | QPaintEngine::AlphaStroke);
    QLineF line(l);
    if (lineEmulation) {
        if (lineEmulation == QPaintEngine::CoordTransform
            && d->state->txop == QPainterPrivate::TxTranslate) {
            line += QPointF(d->state->matrix.dx(), d->state->matrix.dy());
        } else {
            QPolygon polyline;
            polyline << line.start() << line.end();
            d->draw_helper(&polyline,
                           Qt::OddEvenFill, // Not used
                           QPainterPrivate::LineShape, QPainterPrivate::StrokeDraw);
            return;
        }
    }
    d->engine->drawLine(line);
}

/*!
    \fn void QPainter::drawRect(int x, int y, int w, int h)

    \overload

    Draws a rectangle with upper left corner at (\a{x}, \a{y}) and with
    width \a w and height \a h.
*/

/*!
  Draws the rectangle \r with the current pen and brush.

  A filled rectangle has a size of r.size(). A stroked rectangle
  has a size of r.size() plus the pen width.
*/
void QPainter::drawRect(const QRectF &r)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawRect(), [%.2f,%.2f,%.2f,%.2f]\n", r.x(), r.y(), r.width(), r.height());
#endif
    QRectF rect = r.normalize();

    if (!isActive() || rect.isEmpty())
        return;
    d->engine->updateState(d->state);

    uint emulationSpecifier = d->engine->emulationSpecifier;

    if ((emulationSpecifier & QPaintEngine::AlphaFill)
        && d->engine->hasFeature(QPaintEngine::AlphaFillPolygon))
        emulationSpecifier &= ~QPaintEngine::AlphaFill;

    if (emulationSpecifier) {
        if (emulationSpecifier == QPaintEngine::CoordTransform
            && d->state->txop == QPainterPrivate::TxTranslate) {
            rect.translate(QPointF(d->state->matrix.dx(), d->state->matrix.dy()));
        } else {
            d->draw_helper(&rect,
                           Qt::OddEvenFill, // Not used.
                           QPainterPrivate::RectangleShape,
                           QPainterPrivate::StrokeAndFillDraw,
                           emulationSpecifier);
            return;
        }
    }
    d->engine->drawRect(rect);
}


/*!
    Draws all the rectangles in the \a rects list using the current
    pen and brush.

    \sa drawRect()
*/
void QPainter::drawRects(const QList<QRectF> &rects)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawRects(), count=%d\n", rects.size());
#endif
    if (!isActive())
        return;

    d->engine->updateState(d->state);

    QList<QRectF> rectangles = rects;
    if (d->state->txop == QPainterPrivate::TxTranslate
        && !d->engine->hasFeature(QPaintEngine::CoordTransform)) {
        for (int i=0; i<rects.size(); ++i)
            rectangles[i].translate(QPointF(d->state->matrix.dx(), d->state->matrix.dy()));
    } else if (d->engine->emulationSpecifier) {
        for (int i=0; i<rects.size(); ++i)
            drawRect(rects.at(i));
        return;
    }

    d->engine->drawRects(rectangles);
}

/*!
    Draws a single point at position \a p using the current pen's color.

    \sa QPen
*/
void QPainter::drawPoint(const QPointF &p)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawPoint(), p=(%.2f,%.2f)\n", p.x(), p.y());
#endif
    if (!isActive())
        return;
    d->engine->updateState(d->state);

    QPointF pt(p);
    if (d->engine->emulationSpecifier) {
        if (d->engine->emulationSpecifier == QPaintEngine::CoordTransform
            && d->state->txop == QPainterPrivate::TxTranslate) {
            pt += QPointF(d->state->matrix.dx(), d->state->matrix.dy());
        } else {
            QRectF rect(pt.x(), pt.y(), 1, 1);
            d->draw_helper(&rect,
                           Qt::OddEvenFill,
                           QPainterPrivate::RectangleShape);
            return;
        }
    }
    d->engine->drawPoint(pt);
}

/*! \fn void QPainter::drawPoint(int x, int y)

    \overload

    Draws a single point at position (\a x, \a y).
*/

/*!
    Draws the array of points \a pa using the current pen's color.

    \sa QPen
*/
void QPainter::drawPoints(const QPointArray &pa)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawPoints(), count=%d\n", pa.size());
#endif
    if (!isActive())
        return;
    d->engine->updateState(d->state);

    QPolygon a = QPolygon::fromPointArray(pa);
    if (d->engine->emulationSpecifier) {
        if (d->engine->emulationSpecifier == QPaintEngine::CoordTransform
            && d->state->txop == QPainterPrivate::TxTranslate) {
            a.translate(d->state->matrix.dx(), d->state->matrix.dy());
        } else {
            for (int i=0; i<a.size(); ++i) {
                QRectF rect(a.at(i).x(), a.at(i).y(), 1, 1);
                d->draw_helper(&rect,
                               Qt::OddEvenFill,
                               QPainterPrivate::RectangleShape);
            }
            return;
        }
    }

    d->engine->drawPoints(a);
}


/*!
    Sets the background mode of the painter to \a mode, which must be
    either \c Qt::TransparentMode (the default) or \c Qt::OpaqueMode.

    Transparent mode draws stippled lines and text without setting the
    background pixels. Opaque mode fills these space with the current
    background color.

    Note that in order to draw a bitmap or pixmap transparently, you
    must use QPixmap::setMask().

    \sa backgroundMode(), setBackground()
*/

void QPainter::setBackgroundMode(Qt::BGMode mode)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::setBackgroundMode(), mode=%d\n", mode);
#endif

    if (mode != Qt::TransparentMode && mode != Qt::OpaqueMode) {
        qWarning("QPainter::setBackgroundMode: Invalid mode");
        return;
    }
    d->state->bgMode = mode;
    if (d->engine)
        d->engine->setDirty(QPaintEngine::DirtyBackground);
}

/*!
    Returns the current background mode.

    \sa setBackgroundMode() Qt::BGMode
*/
Qt::BGMode QPainter::backgroundMode() const
{
    return d->state->bgMode;
}


/*!
    \overload

    Sets the painter's pen to have style \c Qt::SolidLine, width 0 and the
    specified \a color.

    \sa pen(), QPen
*/

void QPainter::setPen(const QColor &color)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::setPen(), color=%04x\n", color.rgb());
#endif
    QPen newPen = QPen(color.isValid() ? color : Qt::black, 0, Qt::SolidLine);
    if (newPen == d->state->pen)
        return;
    d->state->pen = newPen;

    if (d->engine)
        d->engine->setDirty(QPaintEngine::DirtyPen);
}

/*!
    Sets a new painter pen.

    The \a pen defines how to draw lines and outlines, and it also
    defines the text color.

    \sa pen()
*/

void QPainter::setPen(const QPen &pen)
{

#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::setPen(), color=%04x, style=%d, cap=%d, join=%d\n",
           pen.color().rgb(), pen.style(), pen.capStyle(), pen.joinStyle());
#endif

    if (d->state->pen == pen)
        return;
    d->state->pen = pen;
    if (!pen.color().isValid())
        d->state->pen.setColor(Qt::black);
    if (d->engine)
        d->engine->setDirty(QPaintEngine::DirtyPen);
}

/*!
    \overload

    Sets the painter's pen to have style \a style, width 0 and black
    color.

    \sa pen(), QPen
*/

void QPainter::setPen(Qt::PenStyle style)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::setPen(), style=%d\n", style);
#endif

    if (d->state->pen.style() == style
        && d->state->pen.color() == Qt::black
        && d->state->pen.width() == 0)
        return;
    d->state->pen.setStyle(style);
    if (d->engine)
        d->engine->setDirty(QPaintEngine::DirtyPen);
}

/*!
    Returns the painter's current pen.

    \sa setPen()
*/

const QPen &QPainter::pen() const
{
    return d->state->pen;
}


/*!
    \overload

    Sets the painter's brush to \a brush.

    The \a brush defines how shapes are filled.

    \sa brush()
*/

void QPainter::setBrush(const QBrush &brush)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::setBrush(), color=%04x, style=%d\n", brush.color().rgb(), brush.style());
#endif

    if (d->state->brush == brush)
        return;
    d->state->brush = brush;
    if (!brush.color().isValid())
        d->state->brush.setColor(Qt::black);
    if (d->engine)
        d->engine->setDirty(QPaintEngine::DirtyBrush);
}


/*!
    Sets the painter's brush to black color and the specified \a
    style.

    \sa brush(), QBrush
*/

void QPainter::setBrush(Qt::BrushStyle style)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::setBrush(), style=%d\n", style);
#endif

    if (d->state->brush.style() == style &&
        d->state->brush.color() == Qt::black)
        return;
    d->state->brush = QBrush(Qt::black, style);
    if (d->engine)
        d->engine->setDirty(QPaintEngine::DirtyBrush);
}

/*!
    Returns the painter's current brush.

    \sa QPainter::setBrush()
*/

const QBrush &QPainter::brush() const
{
    return d->state->brush;
}

/*!
    Sets the background brush of the painter to \a bg.

    The background brush is the brush that is filled in when drawing
    opaque text, stippled lines and bitmaps. The background brush has
    no effect in transparent background mode (which is the default).

    \sa background() setBackgroundMode() Qt::BGMode
*/

void QPainter::setBackground(const QBrush &bg)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::setBackground(), color=%04x, style=%d\n", bg.color().rgb(), bg.style());
#endif

    d->state->bgBrush = bg;
    if (d->engine && d->engine->isActive())
        d->engine->setDirty(QPaintEngine::DirtyBackground);
}

/*!
    Sets the painter's font to \a font.

    This font is used by subsequent drawText() functions. The text
    color is the same as the pen color.

    \sa font(), drawText()
*/

void QPainter::setFont(const QFont &font)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::setFont(), family=%s, pointSize=%d\n", font.family().latin1(), font.pointSize());
#endif

    d->state->font = font.resolve(d->state->deviceFont);
    if (d->engine)
        d->engine->setDirty(QPaintEngine::DirtyFont);
}

/*!
    Returns the currently set painter font.

    \sa setFont(), QFont
*/

const QFont &QPainter::font() const
{
    if (d->engine)
        d->engine->updateState(d->state);
    return d->state->pfont ? *d->state->pfont : d->state->font;
}

/*!
    \fn QPainter::drawRoundRect(int x, int y, int w, int h, int
    xround, int yround)

    \overload
*/


/*!
    Draws a rectangle \a r with rounded corners.

    The \a xRnd and \a yRnd arguments specify how rounded the corners
    should be. 0 is angled corners, 99 is maximum roundedness.

    A filled rectangle has a size of r.size(). A stroked rectangle
    has a size of r.size() plus the pen width.

    \sa drawRect(), QPen
*/
void QPainter::drawRoundRect(const QRectF &r, int xRnd, int yRnd)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawRoundRectangle(), [%.2f,%.2f,%.2f,%.2f]\n", r.x(), r.y(), r.width(), r.height());
#endif

    if (!isActive())
        return;

    if(xRnd >= 100)                          // fix ranges
        xRnd = 99;
    if(yRnd >= 100)
        yRnd = 99;
    if(xRnd <= 0 || yRnd <= 0) {             // draw normal rectangle
        drawRect(r);
        return;
    }

    QRectF rect = r.normalize();

    QPainterPath path;

    float x = rect.x();
    float y = rect.y();
    float w = rect.width();
    float h = rect.height();
    float rxx = w*xRnd/200;
    float ryy = h*yRnd/200;
    // were there overflows?
    if (rxx < 0)
        rxx = w/200*xRnd;
    if (ryy < 0)
        ryy = h/200*yRnd;
    float rxx2 = 2*rxx;
    float ryy2 = 2*ryy;

    QPointF startPoint;
    qt_find_ellipse_coords(QRectF(x, y, rxx2, ryy2), 90, 90, &startPoint, 0);
    path.moveTo(startPoint);
    path.arcTo(x, y, rxx2, ryy2, 90, 90);
    path.arcTo(x, y+h-ryy2, rxx2, ryy2, 2*90, 90);
    path.arcTo(x+w-rxx2, y+h-ryy2, rxx2, ryy2, 3*90, 90);
    path.arcTo(x+w-rxx2, y, rxx2, ryy2, 0, 90);
    path.closeSubpath();

    drawPath(path);
}

/*!
    \fn QPainter::drawEllipse(int x, int y, int w, int h)

    \overload

    Draws an ellipse that fits inside the rectangle \a r.
*/

/*!
    Draws the ellipse that fits inside the rectangle \a r.

    A filled ellipse has a size of r.size(). An stroked ellipse
    has a size of r.size() plus the pen width.
*/
void QPainter::drawEllipse(const QRectF &r)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawEllipse(), [%.2f,%.2f,%.2f,%.2f]\n", r.x(), r.y(), r.width(), r.height());
#endif

    if (!isActive())
        return;
    d->engine->updateState(d->state);

    QRectF rect(r.normalize());

    if (d->engine->emulationSpecifier) {
        if (d->engine->emulationSpecifier == QPaintEngine::CoordTransform
            && d->state->txop == QPainterPrivate::TxTranslate) {
            rect.translate(QPointF(d->state->matrix.dx(), d->state->matrix.dy()));
        } else {
            d->draw_helper(&rect, Qt::OddEvenFill, QPainterPrivate::EllipseShape,
                           QPainterPrivate::StrokeAndFillDraw);
            return;
        }
    }

    d->engine->drawEllipse(rect);
}

/*!
    \fn void QPainter::drawArc(int x, int y, int w, int h, int
    startAngle, int spanAngle)

    \overload

    Draws the arc that fits inside the rectangle (\a{x}, \a{y}, \a{w},
    \a{h}), with the given \a startAngle and \a spanAngle.
*/

/*!
    Draws an arc defined by the rectangle \a r, the start angle \a a
    and the arc length \a alen.

    The angles \a a and \a alen are 1/16th of a degree, i.e. a full
    circle equals 5760 (16*360). Positive values of \a a and \a alen
    mean counter-clockwise while negative values mean the clockwise
    direction. Zero degrees is at the 3 o'clock position.

    Example:
    \code
        QPainter p(myWidget);
        p.drawArc(QRect(10,10, 70,100), 100*16, 160*16); // draws a "(" arc
    \endcode

    \sa drawPie(), drawChord()
*/

void QPainter::drawArc(const QRectF &r, int a, int alen)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawArc(), [%.2f,%.2f,%.2f,%.2f], angle=%d, sweep=%d\n",
           r.x(), r.y(), r.width(), r.height(), a/16, alen/16);
#endif

    if (!isActive())
        return;
    d->engine->updateState(d->state);

    QRectF rect = r.normalize();

    QPointF startPoint;
    qt_find_ellipse_coords(r, a/16.0, alen/16.0, &startPoint, 0);

    QPainterPath path;
    path.moveTo(startPoint);
    path.arcTo(rect, a/16.0, alen/16.0);
    strokePath(path, d->state->pen);
}


/*!
    \fn void QPainter::drawPie(int x, int y, int w, int h, int
    startAngle, int spanAngle)

    \overload

    Draws a pie segment that fits inside the rectangle (\a{x}, \a{y},
    \a{w}, \a{h}) with the given \a startAngle and \a spanAngle.
*/

/*!
    Draws a pie defined by the rectangle \a r, the start angle \a a
    and the arc length \a alen.

    The pie is filled with the current brush().

    The angles \a a and \a alen are 1/16th of a degree, i.e. a full
    circle equals 5760 (16*360). Positive values of \a a and \a alen
    mean counter-clockwise while negative values mean the clockwise
    direction. Zero degrees is at the 3 o'clock position.

    \sa drawArc(), drawChord()
*/
void QPainter::drawPie(const QRectF &r, int a, int alen)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawPie(), [%.2f,%.2f,%.2f,%.2f], angle=%d, sweep=%d\n",
           r.x(), r.y(), r.width(), r.height(), a/16, alen/16);
#endif

    if (!isActive())
        return;
    d->engine->updateState(d->state);

    if (a > (360*16)) {
        a = a % (360*16);
    } else if (a < 0) {
        a = a % (360*16);
        if (a < 0) a += (360*16);
    }

    QRectF rect = r.normalize();

    QPainterPath path;
    path.moveTo(rect.center());
    path.arcTo(rect.x(), rect.y(), rect.width(), rect.height(), a/16.0, alen/16.0);
    path.closeSubpath();
    drawPath(path);

}

/*!
    \fn void QPainter::drawChord(int x, int y, int w, int h, int
    startAngle, int spanAngle)

    \overload

    Draws a chord that fits inside the rectangle (\a{x}, \a{y}, \a{w},
    \a{h}) with the given \a startAngle and \a spanAngle.
*/


/*!
    Draws a chord defined by the rectangle \a r, the start angle \a a
    and the arc length \a alen.

    The chord is filled with the current brush().

    The angles \a a and \a alen are 1/16th of a degree, i.e. a full
    circle equals 5760 (16*360). Positive values of \a a and \a alen
    mean counter-clockwise while negative values mean the clockwise
    direction. Zero degrees is at the 3 o'clock position.

    \sa drawArc(), drawPie()
*/

void QPainter::drawChord(const QRectF &r, int a, int alen)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawChord(), [%.2f,%.2f,%.2f,%.2f], angle=%d, sweep=%d\n",
           r.x(), r.y(), r.width(), r.height(), a/16, alen/16);
#endif

    if (!isActive())
        return;
    d->engine->updateState(d->state);

    QRectF rect = r.normalize();

    QPointF startPoint;
    qt_find_ellipse_coords(r, a/16.0, alen/16.0, &startPoint, 0);

    QPainterPath path;
    path.moveTo(startPoint);
    path.arcTo(rect.x(), rect.y(), rect.width(), rect.height(), a/16.0, alen/16.0);
    path.closeSubpath();
    drawPath(path);
}

/*!
    Draws \a nlines separate lines from points defined in \a a,
    starting at \a{a}\e{[index]} (\a index defaults to 0). If \a nlines is
    -1 (the default) all points until the end of the array are used
    (i.e. (a.size()-index)/2 lines are drawn).

    Draws the 1st line from \a{a}\e{[index]} to \a{a}\e{[index + 1]}. Draws the
    2nd line from \a{a}\e{[index + 2]} to \a{a}\e{[index + 3]} etc.

    \sa drawPolyline(), drawPolygon(), QPen
*/

void QPainter::drawLineSegments(const QPointArray &a, int index, int nlines)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawLineSegments(), count=%d\n", a.size()/2);
#endif

    if (!isActive())
        return;
    d->engine->updateState(d->state);

    if (nlines < 0)
        nlines = a.size()/2 - index/2;
    if (index + nlines*2 > (int)a.size())
        nlines = (a.size() - index)/2;
    if (!isActive() || nlines < 1 || index < 0)
        return;

    QList<QLineF> lines;
    if (d->engine->emulationSpecifier) {
        if (d->engine->emulationSpecifier == QPaintEngine::CoordTransform
            && d->state->txop == QPainterPrivate::TxTranslate) {
            QPointF offset((float) d->state->matrix.dx(), (float) d->state->matrix.dy());
            for (int i=index; i<index + nlines*2; i+=2)
                lines << QLineF(a.at(i) + offset, a.at(i+1) + offset);
        } else {
            for (int i=index; i<index + nlines*2; i+=2) {
                QPolygon p;
                p << a.at(i) << a.at(i+1);
                d->draw_helper(&p, Qt::OddEvenFill, QPainterPrivate::LineShape,
                               QPainterPrivate::StrokeDraw);
            }
            return;
        }
    } else {
        for (int i=index; i<index + nlines*2; i+=2)
            lines << QLineF(a.at(i), a.at(i+1));
    }

    d->engine->drawLines(lines);
}

/*!
    Draws the polyline defined by the \a npoints points in \a a
    starting at \a{a}\e{[index]}. (\a index defaults to 0.)

    If \a npoints is -1 (the default) all points until the end of the
    array are used (i.e. a.size()-index-1 line segments are drawn).

    \sa drawLineSegments(), drawPolygon(), QPen
*/

void QPainter::drawPolyline(const QPolygon &a, int index, int npoints)
{
#ifdef QT_DEBUG_DRAW
    QRectF rect = a.boundingRect();
    if (qt_show_painter_debug_output)
        printf("QPainter::drawPolyline(), count=%d, [%d,%d,%d,%d]\n",
           a.size(), rect.x(), rect.y(), rect.width(), rect.height());
#endif

    if (!isActive())
        return;
    d->engine->updateState(d->state);

    if (a.isEmpty())
	return;

    if (npoints < 0)
        npoints = a.size() - index;
    if (index + npoints > (int)a.size())
        npoints = a.size() - index;
    if (!isActive() || npoints < 2 || index < 0)
        return;

    QPolygon pa = a.mid(index, npoints);

    uint lineEmulation = d->engine->emulationSpecifier
                         & (QPaintEngine::CoordTransform
                            | QPaintEngine::PenWidthTransform
                            | QPaintEngine::AlphaStroke);

    if (lineEmulation) {
        if (lineEmulation == QPaintEngine::CoordTransform
            && d->state->txop == QPainterPrivate::TxTranslate) {
            pa.translate(QPointF(d->state->matrix.dx(), d->state->matrix.dy()));
        } else {
            d->draw_helper(&pa, Qt::OddEvenFill, QPainterPrivate::LineShape,
                           QPainterPrivate::StrokeDraw);
            return;
        }
    }

    d->engine->drawPolygon(pa, QPaintEngine::PolylineMode);
}


void QPainter::drawPolyline(const QPointArray &a, int index, int npoints)
{
    // ###
    drawPolyline(QPolygon::fromPointArray(a), index, npoints);
}

/*!
    Draws the polygon defined by the \a npoints points in \a a
    starting at \a{a}\e{[index]}. (\a index defaults to 0.)

    If \a npoints is -1 (the default) all points until the end of the
    array are used (i.e. a.size()-index line segments define the
    polygon).

    The first point is always connected to the last point.

    The polygon is filled with the current brush(). If \a fillRule is
    \c Qt::WindingFill, the polygon is filled using the winding fill algorithm.
    If \a fillRule is \c Qt::OddEvenFill, the polygon is filled using the
    odd-even fill algorithm. See \l{Qt::FillRule} for a more detailed
    description of these fill rules.

    \sa drawLineSegments() drawPolyline() QPen
*/

void QPainter::drawPolygon(const QPolygon &polygon, Qt::FillRule fillRule, int index, int npoints)
{
#ifdef QT_DEBUG_DRAW
    QRect rect = polygon.boundingRect().toRect();
    if (qt_show_painter_debug_output)
        printf("QPainter::drawPolygon(), count=%d, [%d,%d,%d,%d]\n",
           polygon.size(), rect.x(), rect.y(), rect.width(), rect.height());
#endif

    if (!isActive())
        return;
    d->engine->updateState(d->state);

    if (polygon.isEmpty())
	return;

    if (npoints < 0)
        npoints = polygon.size() - index;
    if (index + npoints > (int)polygon.size())
        npoints = polygon.size() - index;
    if (!isActive() || npoints < 2 || index < 0)
        return;

    QPolygon pa = (npoints - index != polygon.size() ?
                   QPolygon(polygon.mid(index, npoints)) : polygon);

    // Connect if polygon if we have a pen.
    if (d->state->pen.style() != Qt::NoPen && pa.first() != pa.last())
        pa << QPointF(pa.first());

    uint emulationSpecifier = d->engine->emulationSpecifier;
    if ((emulationSpecifier & QPaintEngine::LinearGradients)
        && d->engine->hasFeature(QPaintEngine::LinearGradientFillPolygon))
        emulationSpecifier &= ~QPaintEngine::LinearGradients;

    if ((emulationSpecifier & QPaintEngine::AlphaFill)
        && d->engine->hasFeature(QPaintEngine::AlphaFillPolygon))
        emulationSpecifier &= ~QPaintEngine::AlphaFill;

    if (emulationSpecifier) {
        if (emulationSpecifier == QPaintEngine::CoordTransform
            && d->state->txop == QPainterPrivate::TxTranslate) {
            pa.translate(QPointF(d->state->matrix.dx(), d->state->matrix.dy()));
        } else {
            d->draw_helper(&pa, fillRule, QPainterPrivate::PolygonShape,
                           QPainterPrivate::StrokeAndFillDraw,
                           emulationSpecifier);
            return;
        }
    }

    d->engine->drawPolygon(pa, QPaintEngine::PolygonDrawMode(fillRule));
}

void QPainter::drawPolygon(const QPointArray &pa, Qt::FillRule fillRule, int index, int npoints)
{
    drawPolygon(QPolygon::fromPointArray(pa), fillRule, index, npoints);
}


/*!
    Draws the convex polygon defined by the \a npoints points in \a a
    starting at \a{a}\e{[index]} (\a index defaults to 0).

    If the supplied polygon is not convex, the results are undefined.

    On some platforms (e.g. X Window), this is faster than
    drawPolygon().
*/

void QPainter::drawConvexPolygon(const QPointArray &a, int index, int npoints)
{
    // Fix when QPainter::drawPolygon(QPointArray, PolyDrawMode) is in place
    drawPolygon(a, Qt::WindingFill, index, npoints);
}

void QPainter::drawConvexPolygon(const QPolygon &p, int index, int npoints)
{
    // Fix when QPainter::drawPolygon(QPointArray, PolyDrawMode) is in place
    drawPolygon(p, Qt::WindingFill, index, npoints);
}
/*!
    \fn void QPainter::drawPixmap(int x, int y, const QPixmap &pixmap, Qt::PixmapDrawingMode mode)

    \overload

    Draws the given \a pixmap at position (\a{x}, \a{y}) using the
    specified drawing \a mode.
*/

/*!
    \fn void QPainter::drawPixmap(int x, int y, int width, int height,
    const QPixmap &pixmap, Qt::PixmapDrawingMode mode)

    \overload

    Draws the \a pixmap in the rectangle at position (\a{x}, \a{y})
    and of the given \a width and \a height.

*/

/*!
    \fn void QPainter::drawPixmap(int x, int y, int w, int h, const QPixmap &pm,
                                  int sx, int sy, int sw, int sh, Qt::PixmapDrawingMode mode)

    \overload

    Draws the rectangular portion with the origin (\a{sx}, \a{sy}),
    width \a sw and height \a sh, of the pixmap \a pm, at the point
    (\a{x}, \a{y}), with a width of \a w and a height of \a h. If \a
    mode is QPainter::CopyPixmap \a pm will not be masked to
    QPixmap::mask()
*/

/*!
    \fn void QPainter::drawPixmap(int x, int y, const QPixmap &pixmap, int sx, int sy, int sw, int sh,
                                  Qt::PixmapDrawingMode mode)

    \overload

    Draws a pixmap at (\a{x}, \a{y}) by copying a part of \a pixmap into
    the paint device.

    (\a{x}, \a{y}) specifies the top-left point in the paint device that is
    to be drawn onto. (\a{sx}, \a{sy}) specifies the top-left point in \a
    pixmap that is to be drawn. The default is (0, 0).

    (\a{sw}, \a{sh}) specifies the size of the pixmap that is to be drawn.
    The default, (-1, -1), means all the way to the bottom-right of
    the pixmap.

    \sa QPixmap::setMask()
*/

/*!
    \fn void QPainter::drawPixmap(const QPoint &p, const QPixmap &pm, const QRect &sr,
                                  Qt::PixmapDrawingMode mode)
    \overload

    Draws the rectangle \a sr of pixmap \a pm with its origin at point
    \a p.
*/

/*!
    \fn void QPainter::drawPixmap(const QPoint &p, const QPixmap &pm, Qt::PixmapDrawingMode mode)
    \overload

    Draws the pixmap \a pm with its origin at point \a p.
*/

/*!
    \fn void QPainter::drawPixmap(const QRect &r, const QPixmap &pm, Qt::PixmapDrawingMode mode)
    \overload

    Draws the pixmap \a pm into the rectangle \a r.
*/

/*!
    Draws the rectanglular portion \a sr, of pixmap \a pm, into rectangle
    \a r in the paint device. The blend mode \a mode decides how the
    pixmap is merged with the target paint device.

    \sa Qt::PixmapDrawingMode
*/
void QPainter::drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr,
                          Qt::PixmapDrawingMode mode)
{
#if defined QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawPixmap(), target=[%.2f,%.2f,%.2f,%.2f], pix=[%d,%d], source=[%.2f,%.2f,%.2f,%.2f], mode=%d\n",
           r.x(), r.y(), r.width(), r.height(),
           pm.width(), pm.height(),
           sr.x(), sr.y(), sr.width(), sr.height(),
           mode);
#endif

    if (!isActive() || pm.isNull()
	|| (mode == Qt::CopyPixmap && d->device->devType() != QInternal::Pixmap))
        return;
    d->engine->updateState(d->state);

    float x = r.x();
    float y = r.y();
    float w = r.width();
    float h = r.height();
    float sx = sr.x();
    float sy = sr.y();
    float sw = sr.width();
    float sh = sr.height();

    // Sanity-check clipping
    if (sw <= 0 || sw + sx > pm.width())
        sw = pm.width() - sx;

    if (sh <= 0 || sh + sy > pm.height())
        sh = pm.height() - sy;

    if (sx < 0) {
        x -= sx;
        sw += sx;
        sx = 0;
    }

    if (sy < 0) {
        y -= sy;
        sh += sy;
        sy = 0;
    }

    if (w < 0)
        w = sw;
    if (h < 0)
        h = sh;

    if (sw <= 0 || sh <= 0)
        return;

    if ((d->state->txop > QPainterPrivate::TxTranslate
         && !d->engine->hasFeature(QPaintEngine::PixmapTransform)) ||
        ((w != sw || h != sh) && !d->engine->hasFeature(QPaintEngine::PixmapScale))) {
        QPixmap source;
        if(sx || sy || sw != pm.width() || sh != pm.height()) {
            source = QPixmap(qRound(sw), qRound(sh), pm.depth());
            QPainter p(&source);
            p.drawPixmap(QRectF(0, 0, sw, sh), pm, QRectF(sx, sy, sw, sh), Qt::CopyPixmap);
            p.end();
        } else {
            source = pm;
        }

        QMatrix mat(d->state->matrix);
        double scalex = w / (double)sw;
        double scaley = h / (double)sh;
        mat = QMatrix(scalex, 0, 0, scaley, 0, 0) * mat;
        mat = QPixmap::trueMatrix(mat, qRound(sw), qRound(sh));
        QPixmap pmx = source.transform(mat);
        if (pmx.isNull())                        // xformed into nothing
            return;
        if (pmx.depth() == 1 && !pmx.mask())
            pmx.setMask(*(static_cast<QBitmap *>(&pmx)));

        if (!pmx.mask() && d->state->txop == QPainterPrivate::TxRotShear) {
            QBitmap bm_clip(qRound(sw), qRound(sh), 1);        // make full mask, xform it
            bm_clip.fill(Qt::color1);
            pmx.setMask(bm_clip.transform(mat));
        }
        d->state->matrix.map(x, y, &x, &y);        // compute position of pixmap
        float dx, dy;
        mat.map(0, 0, &dx, &dy);
        d->engine->drawPixmap(QRectF(x-dx, y-dy, pmx.width(), pmx.height()), pmx,
                              QRectF(0, 0, pmx.width(), pmx.height()), mode);
    } else {
        if (!d->engine->hasFeature(QPaintEngine::CoordTransform)) {
            x += qRound(d->state->matrix.dx());
            y += qRound(d->state->matrix.dy());
        }
        d->engine->drawPixmap(QRectF(x, y, w, h), pm, QRectF(sx, sy, sw, sh), mode);
    }

    // If we have CopyPixmap we copy the mask from the source to the target device if it
    // is a pixmap also...
    if (mode == Qt::CopyPixmap
        && d->device->devType() == QInternal::Pixmap
        && pm.mask()) {
        QPixmap *p = static_cast<QPixmap *>(d->device);
        QBitmap bitmap(p->width(), p->height());
        bitmap.fill(Qt::color0);
        QPainter pt(&bitmap);
	pt.setPen(Qt::color1);
        const QBitmap *mask = pm.mask();
	Q_ASSERT(!mask->mask());
  	pt.drawPixmap(r, *mask, sr);
        pt.end();
        p->setMask(bitmap);
    }
}

/*!
    Draws the rectanglular portion \a sourceRect, of image \a image, into rectangle
    \a targetRect in the paint device.

    If the image needs to be modified to fit in a lower-resolution
    result (e.g. converting from 32-bit to 8-bit), use the \a
    conversionFlags to specify how you'd prefer this to happen.

    \sa Qt::ImageConversionFlags
*/
void QPainter::drawImage(const QRectF &targetRect, const QImage &image, const QRectF &sourceRect,
                         Qt::ImageConversionFlags flags)
{
    if (!isActive() || image.isNull())
        return;

    d->engine->updateState(d->state);

    float x = targetRect.x();
    float y = targetRect.y();
    float w = targetRect.width();
    float h = targetRect.height();
    float sx = sourceRect.x();
    float sy = sourceRect.y();
    float sw = sourceRect.width();
    float sh = sourceRect.height();

    // Sanity-check clipping
    if (sw <= 0 || sw + sx > image.width())
        sw = image.width() - sx;

    if (sh <= 0 || sh + sy > image.height())
        sh = image.height() - sy;

    if (sx < 0) {
        x -= sx;
        sw += sx;
        sx = 0;
    }

    if (sy < 0) {
        y -= sy;
        sh += sy;
        sy = 0;
    }

    if (w < 0)
        w = sw;
    if (h < 0)
        h = sh;

    if (sw <= 0 || sh <= 0)
        return;

    if ((d->state->txop > QPainterPrivate::TxTranslate
         && !d->engine->hasFeature(QPaintEngine::PixmapTransform)) ||
        ((w != sw || h != sh) && !d->engine->hasFeature(QPaintEngine::PixmapScale))) {
        QPixmap pm;
        pm.fromImage(image, flags);
        drawPixmap(targetRect, pm, sourceRect, Qt::ComposePixmap);
        return;
    }

    if (d->state->txop == QPainterPrivate::TxTranslate
        && !d->engine->hasFeature(QPaintEngine::PixmapTransform)) {
        x += qRound(d->state->matrix.dx());
        y += qRound(d->state->matrix.dy());
    }

    d->engine->drawImage(QRectF(x, y, w, h), image, QRectF(sx, sy, sw, sh), flags);
}

/*!
    \fn void QPainter::drawText(int x, int y, const QString &text, TextDirection dir)

    \overload

    Draws the given \a text at position (\a{x}, \a{y}), in text
    direction \a dir.
*/

/*!
    \fn void QPainter::drawText(int x, int y, int w, int h, int flags,
                                const QString &str, int len, QRect *br)

    \overload

    Draws the string \a str within the rectangle with origin (\a{x},
    \a{y}), width \a w and height \a h. If \a len is -1 (the default)
    all the text is drawn, otherwise only the first \a len characters
    are drawn. The flags that are given in the \a flags parameter are
    \l{Qt::AlignmentFlag}s and \l{Qt::TextFlag}s OR'd together. \a br
    (if not null) is set to the actual bounding rectangle of the
    output.
*/

/*!
    Draws the string \a str at position \a p. The text's direction is
    given by \a dir.

    \sa QPainter::TextDirection
*/

void QPainter::drawText(const QPointF &p, const QString &str, TextDirection dir)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawText(), pos=[%.2f,%.2f], str='%s'\n", p.x(), p.y(), str.latin1());
#endif

    if (!isActive())
        return;
    d->engine->updateState(d->state);

    if (str.isEmpty())
        return;

    QTextLayout layout(str, d->state->pfont ? *d->state->pfont : d->state->font);
    QTextEngine *engine = layout.engine();

    engine->itemize(QTextEngine::SingleLine);

    if (dir != Auto) {
        int level = dir == RTL ? 1 : 0;
        for (int i = engine->items.size()-1; i >= 0; i--)
            engine->items[i].analysis.bidiLevel = level;
    }

    QTextLine line = layout.createLine();
    line.layout(0x01000000);
    const QScriptLine &sl = engine->lines[0];
    line.draw(this, qRound(p.x()), qRound(p.y() - sl.ascent));
}

/*!
    \overload

    Draws the string \a str within the rectangle \a r. If \a len is -1
    (the default) all the text is drawn, otherwise only the first \a
    len characters are drawn. The flags that are given in the \a flags
    parameter are \l{Qt::AlignmentFlag}s and \l{Qt::TextFlag}s OR'd
    together. \a br (if not null) is set to the actual bounding
    rectangle of the output.
*/
void QPainter::drawText(const QRect &r, int flags, const QString &str, int len, QRect *br)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawText(), r=[%d,%d,%d,%d], flags=%d, str='%s'\n",
           r.x(), r.y(), r.width(), r.height(), flags, str.latin1());
#endif

    if (!isActive())
        return;
    d->engine->updateState(d->state);

    if (len < 0)
        len = str.length();
    if (len == 0)                                // empty string
        return;

    QRectF bounds;
    qt_format_text(font(), r, flags, str, len, &bounds, 0, 0, 0, this);
    if (br)
        *br = bounds.toRect();
}

void QPainter::drawText(const QRectF &r, int flags, const QString &str, int len, QRectF *br)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawText(), r=[%d,%d,%d,%d], flags=%d, str='%s'\n",
           r.x(), r.y(), r.width(), r.height(), flags, str.latin1());
#endif

    if (!isActive())
        return;
    d->engine->updateState(d->state);

    if (len < 0)
        len = str.length();
    if (len == 0)                                // empty string
        return;

    qt_format_text(font(), r, flags, str, len, br, 0, 0, 0, this);
}

/*!
    \fn void QPainter::drawTextItem(int x, int y, const QTextItem &ti,
                                    int textflags)

    \internal
    \overload
*/

/*! \internal
    Draws the text item \a ti at position \a p.

    This method ignores the painters background mode and
    color. drawText and qt_format_text have to do it themselves, as
    only they know the extents of the complete string.

    It ignores the font set on the painter as the text item has one of its own.

    The underline and strikeout parameters of the text items font are
    ignored aswell. You'll need to pass in the correct flags to get
    underlining and strikeout.
*/

void QPainter::drawTextItem(const QPointF &p, const QTextItem &ti, int textFlags)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawTextItem(), pos=[%.f,%.f], flags=%d, str='%s'\n",
           p.x(), p.y(), textFlags, QString(ti.chars, ti.num_chars).latin1());
#endif
    if (!isActive())
        return;
    d->engine->updateState(d->state);
    d->engine->drawTextItem(p, ti, textFlags);
}

/*!
    \fn QRect QPainter::boundingRect(const QRect &r, int flags,
                                     const QString &str, int len)

    \overload

    Returns the bounding rectangle constrained by rectangle \a r.
*/

/*!
    \fn QRect QPainter::boundingRect(int x, int y, int w, int h, int flags,
                                     const QString &str, int len = -1);

    Returns the bounding rectangle of the aligned text that would be
    printed with the corresponding drawText() function using the first
    \a len characters of the string \a str, if \a len is > -1, or the
    whole of the string if \a len is -1. The drawing, and hence the
    bounding rectangle, is constrained to the rectangle that begins at
    point (\a{x}, \a{y}) with width \a w and height \a h, or to the
    rectangle required to draw the text, whichever is the larger.

    The \a flags argument is
    the bitwise OR of the following flags:
    \table
    \header \i Flag \i Meaning
    \row \i \c Qt::AlignAuto \i aligns according to the language, usually left.
    \row \i \c Qt::AlignLeft \i aligns to the left border.
    \row \i \c Qt::AlignRight \i aligns to the right border.
    \row \i \c Qt::AlignHCenter \i aligns horizontally centered.
    \row \i \c Qt::AlignTop \i aligns to the top border.
    \row \i \c Qt::AlignBottom \i aligns to the bottom border.
    \row \i \c Qt::AlignVCenter \i aligns vertically centered.
    \row \i \c Qt::AlignCenter \i (== \c Qt::AlignHCenter | \c Qt::AlignVCenter).
    \row \i \c Qt::TextSingleLine \i ignores newline characters in the text.
    \row \i \c Qt::TextExpandTabs \i expands tabs.
    \row \i \c Qt::TextShowMnemonic \i interprets "&x" as "<u>x</u>".
    \row \i \c Qt::TextWordBreak \i breaks the text to fit the rectangle.
    \endtable

    Qt::Horizontal alignment defaults to \c Qt::AlignLeft and vertical
    alignment defaults to \c Qt::AlignTop.

    If several of the horizontal or several of the vertical alignment flags
    are set, the resulting alignment is undefined.

    \sa Qt::TextFlags
*/

QRect QPainter::boundingRect(const QRect &bounds, int flags, const QString &str, int len)
{
    QRect brect;
    if (str.isEmpty())
        brect.setRect(bounds.x(),bounds.y(), 0,0);
    else
        drawText(bounds, flags | Qt::TextDontPrint, str, len, &brect);
    return brect;
}

/*!
  \overload
*/
QRectF QPainter::boundingRect(const QRectF &bounds, int flags, const QString &str, int len)
{
    QRectF brect;
    if (str.isEmpty())
        brect.setRect(bounds.x(),bounds.y(), 0,0);
    else
        drawText(bounds, flags | Qt::TextDontPrint, str, len, &brect);
    return brect;
}

/*!
  \fn void QPainter::drawTiledPixmap(int x, int y, int w, int h, const
  QPixmap &pixmap, int sx, int sy, Qt::PixmapDrawingMode mode);

    Draws a tiled \a pixmap in the specified rectangle.

    (\a{x}, \a{y}) specifies the top-left point in the paint device
    that is to be drawn onto; with the width and height given by \a w
    and \a h. (\a{sx}, \a{sy}) specifies the top-left point in the \a
    pixmap that is to be drawn; this defaults to (0, 0). The pixmap is
    drawn using the given drawing \a mode.

    Calling drawTiledPixmap() is similar to calling drawPixmap()
    several times to fill (tile) an area with a pixmap, but is
    potentially much more efficient depending on the underlying window
    system.

    \sa drawPixmap()
*/

/*!
    \overload

    Draws a tiled \a pixmap, inside rectangle \a r with its origin
    at point \a sp, using the given drawing \a mode.
*/
void QPainter::drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &sp,
                               Qt::PixmapDrawingMode mode)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::drawTiledPixmap(), target=[%d,%d,%d,%d], pix=[%d,%d], offset=[%d,%d], mode=%d\n",
           r.x(), r.y(), r.width(), r.height(),
           pixmap.width(), pixmap.height(),
           sp.x(), sp.y(), mode);
#endif

    if (!isActive())
        return;
    d->engine->updateState(d->state);

    float sw = pixmap.width();
    float sh = pixmap.height();
    if (!sw || !sh)
        return;
    float sx = sp.x();
    float sy = sp.y();
    if (sx < 0)
        sx = qRound(sw) - qRound(-sx) % qRound(sw);
    else
        sx = qRound(sx) % qRound(sw);
    if (sy < 0)
        sy = qRound(sh) - -qRound(sy) % qRound(sh);
    else
        sy = qRound(sy) % qRound(sh);

    if (d->state->txop > QPainterPrivate::TxTranslate
        && !d->engine->hasFeature(QPaintEngine::PixmapTransform)) {
        QPixmap pm;
        if (pixmap.hasAlphaChannel()) {
            // Needed to preserve the alpha channel in the pixmap
            // While setPixel() is needed to switch on alpha buffer on Embedded
            QImage img(qRound(r.width()), qRound(r.height()), 32);
            img.setPixel(0, 0, QColor(255, 0, 0, 127).rgba());
            img.setAlphaBuffer(true);
            pm = img;
        } else {
            pm = QPixmap(qRound(r.width()), qRound(r.height()));
        }
        QPainter p(&pm);
        // Recursive call ok, since the pixmap is not transformed...
        p.setPen(pen());
        p.setBackground(background());
        p.setBackgroundMode(backgroundMode());
        p.drawTiledPixmap(QRectF(0, 0, r.width(), r.height()), pixmap, QPointF(sx, sy), Qt::CopyPixmap);
        p.end();
        if (pixmap.depth() == 1) {
            QBitmap mask(pm.width(), pm.height(), true);
            p.begin(&mask);
            p.drawTiledPixmap(QRectF(0, 0, r.width(), r.height()), pixmap, QPointF(sx, sy));
            p.end();
            pm.setMask(mask);
        }
        drawPixmap(qRound(r.x()), qRound(r.y()), pm);
        return;
    }

    float x = r.x();
    float y = r.y();
    if (d->state->txop == QPainterPrivate::TxTranslate
        && !d->engine->hasFeature(QPaintEngine::PixmapTransform)) {
        x += qRound(d->state->matrix.dx());
        y += qRound(d->state->matrix.dy());
    }

    d->engine->drawTiledPixmap(QRectF(x, y, r.width(), r.height()), pixmap, QPointF(sx, sy), mode);
}


/*!
    \fn void QPainter::drawPicture(int x, int y, const QPicture &picture)
    \overload

    Draws picture \a picture at point (\a x, \a y).
*/

/*!
    Replays the picture \a picture at point \a p.

    This function does exactly the same as QPicture::play() when
    called with \a p = QPoint(0, 0).
*/

void QPainter::drawPicture(const QPoint &p, const QPicture &picture)
{
    if (!isActive())
        return;
    d->engine->updateState(d->state);
    save();
    translate(p);
    const_cast<QPicture *>(&picture)->play(this);
    restore();
}

/*!
    Erases the area inside the rectangle \a r. Equivalent to
    \c{fillRect(r, backgroundColor())}.
*/
void QPainter::eraseRect(const QRectF &r)
{
    if (!isActive())
        return;
    d->engine->updateState(d->state);

    if (d->state->bgBrush.pixmap())
        drawTiledPixmap(r, *d->state->bgBrush.pixmap(), -d->state->bgOrigin);
    else
        fillRect(r, d->state->bgBrush);
}

/*!
  \fn void QPainter::eraseRect(int x, int y, int w, int h)
  \overload

    Erases the area inside \a x, \a y, \a w, \a h. Equivalent to
    \c{fillRect(x, y, w, h, backgroundColor())}.
*/


/*!
    Fills the rectangle \a r with the \a brush.

    You can specify a QColor as \a brush, since there is a QBrush
    constructor that takes a QColor argument and creates a solid
    pattern brush.

    \sa drawRect()
*/
void QPainter::fillRect(const QRectF &r, const QBrush &brush)
{
    QPen oldPen   = pen();
    bool swap = oldPen.style() != Qt::NoPen;
    if (swap)
        setPen(Qt::NoPen);
    QBrush oldBrush = this->brush();
    setBrush(brush);
    drawRect(r);
    setBrush(oldBrush);
    if (swap)
        setPen(oldPen);
}

/*!
  \fn void QPainter::fillRect(int x, int y, int w, int h, const QBrush &brush)

    \overload

    Fills the rectangle (\a{x}, \a{y}, \a{w}, \a{h}) with the \a brush.

    You can specify a QColor as \a brush, since there is a QBrush
    constructor that takes a QColor argument and creates a solid
    pattern brush.

    \sa drawRect()
*/


/*!
  Sets the render hint \a hint on this painter if \a on is true;
  otherwise clears the render hint.
*/
void QPainter::setRenderHint(RenderHint hint, bool on)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::setRenderHint(), hint=%x, %s\n", hint, on ? "on" : "off");
#endif

    if (!isActive()) {
        qWarning("Painter must be active to set rendering hints");
        return;
    }

    d->engine->setRenderHint(hint, on);
}

/*!
    Returns a flag that specifies the rendering hints that this
    painter supports.
*/
QPainter::RenderHints QPainter::supportedRenderHints() const
{
    if (!isActive()) {
        qWarning("Painter must be active to set rendering hints");
        return 0;
    }
    return d->engine->supportedRenderHints();
}

/*!
    Returns a flag that specifies the rendering hints that are set for
    this painter.
*/
QPainter::RenderHints QPainter::renderHints() const
{
    if (!isActive()) {
        qWarning("Painter must be active to set rendering hints");
        return 0;
    }
    return d->engine->renderHints();
}

#ifdef QT_COMPAT

/*!
    Returns true if view transformation is enabled; otherwise returns
    false.

    \sa setViewXForm(), xForm()
*/

bool QPainter::hasViewXForm() const
{
#ifndef QT_NO_TRANSFORMATIONS
    return d->state->VxF;
#else
    return d->state->xlatex || d->state->xlatey;
#endif
}

/*!
    \fn void QPainter::setWindow(const QRect &r)

    \overload

    Sets the painter's window to the rectangle \a r.
*/

/*!
  \fn void QPainter::setWindow(int x, int y, int w, int h)

    Sets the window rectangle view transformation for the painter and
    enables view transformation.

    The window rectangle is part of the view transformation. The
    window specifies the logical coordinate system and is specified by
    the \a x, \a y, \a w width and \a h height parameters. Its sister,
    the viewport(), specifies the device coordinate system.

    The default window rectangle is the same as the device's
    rectangle. See the \link coordsys.html Coordinate System Overview
    \endlink for an overview of coordinate transformation.

    \sa window(), setViewport(), setViewXForm(), setMatrix(),
    setMatrixEnabled()
*/

void QPainter::setWindow(const QRect &r)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::setWindow(), [%d,%d,%d,%d]\n", r.x(), r.y(), r.width(), r.height());
#endif

    if (!isActive()) {
        qWarning("QPainter::setWindow(), painter not active");
        return;
    }

    // Must update clip before changing matrix.
    if (d->engine->hasFeature(QPaintEngine::ClipTransform)
        && d->engine->testDirty(QPaintEngine::DirtyClip))
        d->engine->updateState(d->state);

    d->state->wx = r.x();
    d->state->wy = r.y();
    d->state->ww = r.width();
    d->state->wh = r.height();
    if (d->state->VxF)
        d->updateMatrix();
    else
        setViewXForm(true);
}

/*!
    Returns the window rectangle.

    \sa setWindow(), setViewXForm()
*/

QRect QPainter::window() const
{
    return QRect(d->state->wx, d->state->wy, d->state->ww, d->state->wh);
}

/*!
    \fn void QPainter::setViewport(const QRect &r)

    \overload

    Sets the painter's viewport rectangle to \a r.
*/

/*!
  \fn void QPainter::setViewport(int x, int y, int w, int h)

    Sets the viewport rectangle view transformation for the painter
    and enables view transformation.

    The viewport rectangle is part of the view transformation. The
    viewport specifies the device coordinate system and is specified
    by the \a x, \a y, \a w width and \a h height parameters. Its
    sister, the window(), specifies the logical coordinate system.

    The default viewport rectangle is the same as the device's
    rectangle. See the \link coordsys.html Coordinate System Overview
    \endlink for an overview of coordinate transformation.

    \sa viewport(), setWindow(), setViewXForm(), setMatrix(),
    setMatrixEnabled()
*/

void QPainter::setViewport(const QRect &r)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::setViewport(), [%d,%d,%d,%d]\n", r.x(), r.y(), r.width(), r.height());
#endif

    if (!isActive()) {
        qWarning("QPainter::setViewport(), painter not active");
        return;
    }

    // Must update clip before changing matrix.
    if (d->engine->hasFeature(QPaintEngine::ClipTransform)
        && d->engine->testDirty(QPaintEngine::DirtyClip))
        d->engine->updateState(d->state);

    d->state->vx = r.x();
    d->state->vy = r.y();
    d->state->vw = r.width();
    d->state->vh = r.height();
    if (d->state->VxF)
        d->updateMatrix();
    else
        setViewXForm(true);
}

/*!
    Returns the viewport rectangle.

    \sa setViewport(), setViewXForm()
*/

QRect QPainter::viewport() const
{
    return QRect(d->state->vx, d->state->vy, d->state->vw, d->state->vh);
}

/*!
    Enables view transformations if \a enable is true, or disables
    view transformations if \a enable is false.

    \sa hasViewXForm(), setWindow(), setViewport(), setMatrix(),
    setMatrixEnabled()
*/

void QPainter::setViewXForm(bool enable)
{
#ifdef QT_DEBUG_DRAW
    if (qt_show_painter_debug_output)
        printf("QPainter::setViewXForm(), enable=%d\n", enable);
#endif

    if (!isActive()) {
        qWarning("QPainter::setViewXForm(), painter not active");
        return;
    }
    if (enable == d->state->VxF)
        return;

    // Must update clip before changing matrix.
    if (d->engine->hasFeature(QPaintEngine::ClipTransform)
        && d->engine->testDirty(QPaintEngine::DirtyClip))
        d->engine->updateState(d->state);

    d->state->VxF = enable;
    d->updateMatrix();
}


double QPainter::translationX() const
{
#ifndef QT_NO_TRANSFORMATIONS
    return d->state->worldMatrix.dx();
#else
    return d->state->xlatex;
#endif
}

double QPainter::translationY() const
{
#ifndef QT_NO_TRANSFORMATIONS
    return d->state->worldMatrix.dy();
#else
    return d->state->xlatey;
#endif
}

/*!
    \fn void QPainter::map(int x, int y, int *rx, int *ry) const

    \internal

    Sets (\a{rx}, \a{ry}) to the point that results from applying the
    painter's current transformation on the point (\a{x}, \a{y}).
*/
void QPainter::map(int x, int y, int *rx, int *ry) const
{
    QPoint p(x, y);
    p = p * d->state->matrix;
    *rx = p.x();
    *ry = p.y();
}

/*!
    Returns the point \a p transformed from model coordinates to
    device coordinates.

    \sa xFormDev(), QMatrix::map()
*/

QPoint QPainter::xForm(const QPoint &p) const
{
#ifndef QT_NO_TRANSFORMATIONS
    if (d->state->txop == QPainterPrivate::TxNone)
        return p;
    return p * d->state->matrix;
#else
    return QPoint(p.x() + d->state->xlatex, p.y() + d->state->xlatey);
#endif
}


/*!
    \overload

    Returns the rectangle \a r transformed from model coordinates to
    device coordinates.

    If world transformation is enabled and rotation or shearing has
    been specified, then the bounding rectangle is returned.

    \sa xFormDev(), QMatrix::map()
*/

QRect QPainter::xForm(const QRect &r)        const
{
#ifndef QT_NO_TRANSFORMATIONS
    if (d->state->txop == QPainterPrivate::TxNone)
        return r;
    return d->state->matrix.mapRect(r);
#else
    return QRect(r.x()+d->state->xlatex, r.y()+d->state->xlatey, r.width(), r.height());
#endif
}

/*!
    \overload

    Returns the point array \a a transformed from model coordinates
    to device coordinates.

    \sa xFormDev(), QMatrix::map()
*/

QPointArray QPainter::xForm(const QPointArray &a) const
{
#ifndef QT_NO_TRANSFORMATIONS
    if (d->state->txop == QPainterPrivate::TxNone)
        return a;
    return a * d->state->matrix;
#else
    QPointArray p(a);
    p.translate(d->state->xlatex, d->state->xlatey);
    return p;
#endif
}

/*!
    \overload

    Returns the point array \a av transformed from model coordinates
    to device coordinates. The \a index is the first point in the
    array and \a npoints denotes the number of points to be
    transformed. If \a npoints is negative, all points from
    \a{av}\e{[index]} until the last point in the array are transformed.

    The returned point array consists of the number of points that
    were transformed.

    Example:
    \code
        QPointArray a(10);
        QPointArray b;
        b = painter.xForm(a, 2, 4);  // b.size() == 4
        b = painter.xForm(a, 2, -1); // b.size() == 8
    \endcode

    \sa xFormDev(), QMatrix::map()
*/

QPointArray QPainter::xForm(const QPointArray &av, int index, int npoints) const
{
    int lastPoint = npoints < 0 ? av.size() : index+npoints;
    QPointArray a(lastPoint-index);
    memcpy(a.data(), av.data()+index, (lastPoint-index)*sizeof(QPoint));
#ifndef QT_NO_TRANSFORMATIONS
    return a * d->state->matrix;
#else
    a.translate(d->state->xlatex, d->state->xlatey);
    return a;
#endif
}

/*!
    \overload

    Returns the point \a p transformed from device coordinates to
    model coordinates.

    \sa xForm(), QMatrix::map()
*/

QPoint QPainter::xFormDev(const QPoint &p) const
{
#ifndef QT_NO_TRANSFORMATIONS
    if(d->state->txop == QPainterPrivate::TxNone)
        return p;
    if (!d->txinv) {
        QPainter *that = (QPainter*)this;        // mutable
        that->d_ptr->updateInvMatrix();
    }
    return p * d->invMatrix;
#else
    return QPoint(p.x() - xlatex, p.y() - xlatey);
#endif
}

/*!
    Returns the rectangle \a r transformed from device coordinates to
    model coordinates.

    If world transformation is enabled and rotation or shearing is
    used, then the bounding rectangle is returned.

    \sa xForm(), QMatrix::map()
*/

QRect QPainter::xFormDev(const QRect &r)  const
{
#ifndef QT_NO_TRANSFORMATIONS
    if (d->state->txop == QPainterPrivate::TxNone)
        return r;
    if (!d->txinv) {
        QPainter *that = (QPainter*)this;        // mutable
        that->d_ptr->updateInvMatrix();
    }
    return d->invMatrix.mapRect(r);
#else
    return QRect(r.x()-d->state->xlatex, r.y()-d->state->xlatey, r.width(), r.height());
#endif
}

/*!
    \overload

    Returns the point array \a a transformed from device coordinates
    to model coordinates.

    \sa xForm(), QMatrix::map()
*/

QPointArray QPainter::xFormDev(const QPointArray &a) const
{
#ifndef QT_NO_TRANSFORMATIONS
    if (d->state->txop == QPainterPrivate::TxNone)
        return a;
    if (!d->txinv) {
        QPainter *that = (QPainter*)this;        // mutable
        that->d_ptr->updateInvMatrix();
    }
    return a * d->invMatrix;
#else
    QPointArray p(a);
    p.translate(-d->state->xlatex, -d->state->xlatey);
    return p;
#endif

}

/*!
    \overload

    Returns the point array \a ad transformed from device coordinates
    to model coordinates. The \a index is the first point in the array
    and \a npoints denotes the number of points to be transformed. If
    \a npoints is negative, all points from \a{ad}\e{[index]} until the
    last point in the array are transformed.

    The returned point array consists of the number of points that
    were transformed.

    Example:
    \code
        QPointArray a(10);
        QPointArray b;
        b = painter.xFormDev(a, 1, 3);  // b.size() == 3
        b = painter.xFormDev(a, 1, -1); // b.size() == 9
    \endcode

    \sa xForm(), QMatrix::map()
*/

QPointArray QPainter::xFormDev(const QPointArray &ad, int index, int npoints) const
{
    int lastPoint = npoints < 0 ? ad.size() : index+npoints;
    QPointArray a(lastPoint-index);
    memcpy(a.data(), ad.data()+index, (lastPoint-index)*sizeof(QPoint));
#ifndef QT_NO_TRANSFORMATIONS
    if (d->state->txop == QPainterPrivate::TxNone)
        return a;
    if (!d->txinv) {
        QPainter *that = (QPainter*)this;        // mutable
        that->d_ptr->updateInvMatrix();
    }
    return a * d->invMatrix;
#else
    a.translate(-d->state->xlatex, -d->state->xlatey);
    return a;
#endif
}

/*!
  \fn void QPainter::drawPoints(const QPointArray &pa, int index, int npoints)

  \overload
  \obsolete

    Draws the array of points \a pa using the current pen's color.

    If \a index is non-zero (the default is zero), only points
    starting from \a index are drawn. If \a npoints is negative (the
    default) the rest of the points from \a index are drawn. If \a
    npoints is zero or greater, \a npoints points are drawn.

    \sa drawPoints
*/
void QPainter::drawPoints(const QPointArray &pa, int index, int npoints)
{
    if (npoints < 0)
        npoints = pa.size() - index;
    if (index + npoints > (int)pa.size())
        npoints = pa.size() - index;
    if (!isActive() || npoints < 1 || index < 0)
        return;

    QPointArray a = pa.mid(index, npoints);
    drawPoints(a);
}

/*!
    Draws a cubic Bezier curve defined by the control points in \a a,
    starting at \a{a}\e{[index]} (\a index defaults to 0).

    Control points after \a{a}\e{[index + 3]} are ignored. Nothing happens
    if there aren't enough control points.
*/
void QPainter::drawCubicBezier(const QPointArray &a, int index)
{
    if (!isActive())
        return;
    d->engine->updateState(d->state);

    if ((int)a.size() - index < 4) {
        qWarning("QPainter::drawCubicBezier: Cubic Bezier needs 4 control "
                  "points");
        return;
    }

    QPainterPath path;
    path.moveTo(a.at(index));
    path.curveTo(a.at(index+1), a.at(index+2), a.at(index+3));
    strokePath(path, d->state->pen);
}
#endif

struct QPaintDeviceRedirection
{
    QPaintDeviceRedirection() : device(0), replacement(0) {}
    QPaintDeviceRedirection(const QPaintDevice *device, QPaintDevice *replacement,
                            const QPoint& offset)
        : device(device), replacement(replacement), offset(offset) { }
    const QPaintDevice *device;
    QPaintDevice *replacement;
    QPoint offset;
    bool operator==(const QPaintDevice *pdev) const { return device == pdev; }
    Q_DUMMY_COMPARISON_OPERATOR(QPaintDeviceRedirection)
};

typedef QList<QPaintDeviceRedirection> QPaintDeviceRedirectionList;
Q_GLOBAL_STATIC(QPaintDeviceRedirectionList, globalRedirections)

/*!
  \internal

    Redirects all paint commands for a paint device, \a device, to
    another paint device, \a replacement. The optional point \a offset
    defines an offset within the replaced device. After painting you
    must call restoreRedirected().

    In general, you'll probably find calling QPixmap::grabWidget() or
    QPixmap::grabWindow() is an easier solution.

    \sa redirected()
*/
void QPainter::setRedirected(const QPaintDevice *device,
                             QPaintDevice *replacement,
                             const QPoint &offset)
{
    Q_ASSERT(device != 0);
    QPaintDeviceRedirectionList *redirections = globalRedirections();
    Q_ASSERT(redirections != 0);
    *redirections += QPaintDeviceRedirection(device, replacement, offset);
}


/*!\internal

  Restores the previous redirection for \a device after a call to
  setRedirected().

  \sa redirected()
 */
void QPainter::restoreRedirected(const QPaintDevice *device)
{
    Q_ASSERT(device != 0);
    QPaintDeviceRedirectionList *redirections = globalRedirections();
    Q_ASSERT(redirections != 0);
    for (int i = redirections->size()-1; i >= 0; --i)
        if (redirections->at(i) == device) {
            redirections->removeAt(i);
            return;
        }
}


/*!
    \internal

    Returns the replacement for \a device. The optional out parameter
    \a offset returns return the offset within the replaced device.
*/
QPaintDevice *QPainter::redirected(const QPaintDevice *device, QPoint *offset)
{
    Q_ASSERT(device != 0);
    QPaintDeviceRedirectionList *redirections = globalRedirections();
    Q_ASSERT(redirections != 0);
    for (int i = redirections->size()-1; i >= 0; --i)
        if (redirections->at(i) == device) {
            if (offset)
                *offset = redirections->at(i).offset;
            return redirections->at(i).replacement;
        }
    if (offset)
        *offset = QPoint(0, 0);
    return 0;
}


void qt_format_text(const QFont &font, const QRectF &_r,
                    int tf, const QString& str, int len, QRectF *brect,
                    int tabstops, int *, int tabarraylen,
                    QPainter *painter)
{
    // we need to copy r here to protect against the case (&r == brect).
    QRectF r(_r);

    bool dontclip  = (tf & Qt::TextDontClip);
    bool wordwrap  = (tf & Qt::TextWordWrap);
    bool singleline = (tf & Qt::TextSingleLine);
    bool showmnemonic = (tf & Qt::TextShowMnemonic);
    bool hidemnmemonic = (tf & Qt::TextHideMnemonic);

    bool isRightToLeft = str.isRightToLeft();
    if ((tf & Qt::AlignHorizontal_Mask) == Qt::AlignAuto)
        tf |= isRightToLeft ? Qt::AlignRight : Qt::AlignLeft;

    bool expandtabs = ((tf & Qt::TextExpandTabs) &&
                        (((tf & Qt::AlignLeft) && !isRightToLeft) ||
                          ((tf & Qt::AlignRight) && isRightToLeft)));

    if (!painter)
        tf |= Qt::TextDontPrint;

    int maxUnderlines = 0;
    int numUnderlines = 0;
    int underlinePositionStack[32];
    int *underlinePositions = underlinePositionStack;

    QFont fnt(painter
              ? (painter->d->state->pfont
                 ? *painter->d->state->pfont
                 : painter->d->state->font)
              : font);
    QFontMetrics fm(fnt);

    QString text = str;
    // compatible behaviour to the old implementation. Replace
    // tabs by spaces
    QChar *chr = text.data();
    const QChar *end = chr + len;
    bool haveLineSep = false;
    while (chr != end) {
        if (*chr == '\r' || (singleline && *chr == '\n')) {
            *chr = ' ';
        } else if (*chr == '\n') {
            *chr = QChar::LineSeparator;
            haveLineSep = true;
        } else if (*chr == '&') {
            ++maxUnderlines;
        }
        ++chr;
    }
    if (!expandtabs) {
        chr = (QChar*)text.unicode();
        while (chr != end) {
            if (*chr == '\t')
                *chr = ' ';
            ++chr;
        }
    } else if (!tabarraylen && !tabstops) {
        tabstops = fm.width('x')*8;
    }

    if (hidemnmemonic || showmnemonic) {
        if (maxUnderlines > 32)
            underlinePositions = new int[maxUnderlines];
        QChar *cout = (QChar*)text.unicode();
        QChar *cin = cout;
        int l = len;
        while (l) {
            if (*cin == '&') {
                ++cin;
                --l;
                if (!l)
                    break;
                if (*cin != '&' && !hidemnmemonic)
                    underlinePositions[numUnderlines++] = cout - text.unicode();
            }
            *cout = *cin;
            ++cout;
            ++cin;
            --l;
        }
        int newlen = cout - text.unicode();
        if (newlen != text.length())
            text.resize(newlen);
    }

    // no need to do extra work for underlines if we don't paint
    if (tf & Qt::TextDontPrint)
        numUnderlines = 0;

    underlinePositions[numUnderlines] = -1;
    int height = 0;
    int width = 0;

    QTextLayout textLayout(text, fnt);
    textLayout.engine()->underlinePositions = underlinePositions;

    if (text.isEmpty()) {
        height = fm.height();
        width = 0;
        tf |= Qt::TextDontPrint;
    } else {
        float lineWidth = wordwrap ? qMax(0, r.width()) : 0x01000000;
        if(!wordwrap)
            tf |= Qt::TextIncludeTrailingSpaces;
        textLayout.beginLayout((tf & Qt::TextDontPrint) ? QTextLayout::NoBidi : QTextLayout::MultiLine);

        int leading = fm.leading();
        height = -leading;

        textLayout.clearLines();
        while (1) {
            QTextLine l = textLayout.createLine();
            if (!l.isValid())
                break;

            l.layout(qRound(lineWidth));
            height += leading;
            l.setPosition(QPoint(0, height));
            height += l.ascent() + l.descent();
            width = qMax(width, l.textWidth());
        }
    }

    float yoff = 0;
    float xoff = 0;
    if (tf & Qt::AlignBottom)
        yoff = r.height() - height;
    else if (tf & Qt::AlignVCenter)
        yoff = (r.height() - height)/2;
    if (tf & Qt::AlignRight)
        xoff = r.width() - width;
    else if (tf & Qt::AlignHCenter)
        xoff = (r.width() - width)/2;
    if (brect)
        *brect = QRectF(r.x() + xoff, r.y() + yoff, width, height);

    if (!(tf & Qt::TextDontPrint)) {
        bool restore = false;
        if (!dontclip) {
            restore = true;
            painter->save();
            painter->setClipRect(r, Qt::IntersectClip);
        }

        int _tf = 0;
        if (fnt.underline()) _tf |= Qt::TextUnderline;
        if (fnt.overline()) _tf |= Qt::TextOverline;
        if (fnt.strikeOut()) _tf |= Qt::TextStrikeOut;

        for (int i = 0; i < textLayout.numLines(); i++) {
            QTextLine line = textLayout.lineAt(i);

            line.draw(painter, qRound(r.x() + xoff + line.x()), qRound(r.y() + yoff));
        }

        if (restore) {
            painter->restore();
        }
    }

    if (underlinePositions != underlinePositionStack)
        delete [] underlinePositions;
}



// #define QT_GRAD_NO_POLY
// #define QT_GRAD_NO_LINE

#define qt_gradient_fill_color(c)  QColor(c.red(), c.green(), c.blue())

void qt_fill_linear_gradient(const QRect &rect, QPainter *p, const QBrush &brush)
{
    Q_ASSERT(brush.style() == Qt::LinearGradientPattern);

    p->translate(rect.topLeft());
    QPoint gstart = brush.gradientStart() - rect.topLeft();
    QPoint gstop  = brush.gradientStop() - rect.topLeft();

    QPen oldPen = p->pen();

    QColor gcol1 = brush.color();
    QColor gcol2 = brush.gradientColor();

    int dx = gstop.x() - gstart.x();
    int dy = gstop.y() - gstart.y();

    int rw = rect.width();
    int rh = rect.height();

    p->setRenderHint(QPainter::LineAntialiasing, false);

    if (QABS(dx) > QABS(dy)) { // Fill horizontally
        // Make sure we fill left to right.
        if (gstop.x() < gstart.x()) {
            qSwap(gcol1, gcol2);
            qSwap(gstart, gstop);
        }
        // Find the location where the lines covering the gradient intersect
        // the lines making up the top and bottom of the target rectangle.
        // Note: This might be outside the target rect, but that is ok.
        int xtop1, xtop2, xbot1, xbot2;
        if (dy == 0) {
            xtop1 = xbot1 = gstart.x();
            xtop2 = xbot2 = gstop.x();
        } else {
            double gamma = double(dx) / double(-dy);
            xtop1 = qRound((-gstart.y() + gamma * gstart.x() ) / gamma);
            xtop2 = qRound((-gstop.y()  + gamma * gstop.x()  ) / gamma);
            xbot1 = qRound((rh - gstart.y() + gamma * gstart.x() ) / gamma);
            xbot2 = qRound((rh - gstop.y()  + gamma * gstop.x()  ) / gamma);
            Q_ASSERT(xtop2 > xtop1);
        }

        p->setPen(Qt::NoPen);
#ifndef QT_GRAD_NO_POLY
        // Fill the area to the left of the gradient
        QPointArray leftFill;
	if (xtop1 > 0)
	    leftFill << QPoint(0, 0);
	leftFill << QPoint(xtop1+1, 0)
		 << QPoint(xbot1+1, rh);
        if (xbot1 > 0)
            leftFill << QPoint(0, rh);
        p->setBrush(qt_gradient_fill_color(gcol1));
        p->drawPolygon(leftFill);

        // Fill the area to the right of the gradient
        QPointArray rightFill;
	rightFill << QPoint(xtop2-1, 0);
	if (xtop2 < rw)
	    rightFill << QPoint(rw, 0);
	if (xbot2 < rw)
	    rightFill << QPoint(rw, rh);
	rightFill << QPoint(xbot2-1, rh);
        p->setBrush(qt_gradient_fill_color(gcol2));
        p->drawPolygon(rightFill);
#endif // QT_GRAD_NO_POLY

#ifndef QT_GRAD_NO_LINE
        // Fill the gradient.
        double r = gcol1.red();
        double g = gcol1.green();
        double b = gcol1.blue();
        int steps = (xtop2-xtop1);
        double rinc = (gcol2.red()-r) / steps;
        double ginc = (gcol2.green()-g) / steps;
        double binc = (gcol2.blue()-b) / steps;
        for (int x=0; x<=steps; ++x) {
            p->setPen(QColor(int(r), int(g), int(b)));
            p->drawLine(x+xtop1, 0, x+xbot1, rh);
            r += rinc;
            g += ginc;
            b += binc;
        }
#else
        p->setPen(Qt::black);
        p->drawLine(xtop1, 0, xbot1, rh);
        p->drawLine(xtop2, 0, xbot2, rh);
#endif // QT_GRAD_NO_LINE
    } else {
        // Fill Verticallty
        // Code below is a conceptually equal to the one above except that all
        // coords are swapped x <-> y.
        // Make sure we fill top to bottom...
        if (gstop.y() < gstart.y()) {
            qSwap(gstart, gstop);
            qSwap(gcol1, gcol2);
        }
        int yleft1, yleft2, yright1, yright2;
        if (dx == 0) {
            yleft1 = yright1 = gstart.y();
            yleft2 = yright2 = gstop.y();
        } else {
            double gamma = double(dy) / double(-dx);
            yleft1 = qRound((-gstart.x() + gamma * gstart.y()) / gamma);
            yleft2 = qRound((-gstop.x() + gamma * gstop.y()) / gamma);
            yright1 = qRound((rw - gstart.x() + gamma*gstart.y()) / gamma);
            yright2 = qRound((rw - gstop.x() + gamma*gstop.y()) / gamma);
            Q_ASSERT(yleft2 > yleft1);
        }

#ifndef QT_GRAD_NO_POLY
        p->setPen(Qt::NoPen);
        QPointArray topFill;
        topFill << QPoint(0, yleft1+1);
	if (yleft1 > 0)
	    topFill << QPoint(0, 0);
	if (yright1 > 0)
	    topFill << QPoint(rw, 0);
	topFill << QPoint(rw, yright1+1);
        p->setBrush(qt_gradient_fill_color(gcol1));
        p->drawPolygon(topFill);

        QPointArray bottomFill;
	bottomFill << QPoint(0, yleft2-1);
	if (yleft2 < rh)
	    bottomFill << QPoint(0, rh);
	if (yright2 < rh)
	    bottomFill << QPoint(rw, rh);
	bottomFill << QPoint(rw, yright2-1);
        p->setBrush(qt_gradient_fill_color(gcol2));
        p->drawPolygon(bottomFill);
#endif // QT_GRAD_NO_POLY

#ifndef QT_GRAD_NO_LINE
        double r = gcol1.red();
        double g = gcol1.green();
        double b = gcol1.blue();
        int steps = (yleft2-yleft1);
        double rinc = (gcol2.red()-r) / steps;
        double ginc = (gcol2.green()-g) / steps;
        double binc = (gcol2.blue()-b) / steps;
        for (int y=0; y<=steps; ++y) {
            p->setPen(QColor(int(r), int(g), int(b)));
            p->drawLine(0, y+yleft1, rw, y+yright1);
            r += rinc;
            g += ginc;
            b += binc;
        }
#else
        p->setPen(Qt::black);
        p->drawLine(0, yleft1, rw, yright1);
        p->drawLine(0, yleft2, rw, yright2);
#endif // QT_GRAD_NO_LINE
    }

    p->translate(-rect.topLeft());
    p->setPen(oldPen);
}

// Declared in qpaintdevice.h

#ifdef QT_COMPAT
static void bitBlt_helper(QPaintDevice *dst, const QPoint &dp,
                          const QPaintDevice *src, const QRect &sr, bool imask)
{
    Q_ASSERT(dst);
    Q_ASSERT(src);

    if (src->devType() == QInternal::Pixmap) {
        const QPixmap *pixmap = static_cast<const QPixmap *>(src);
        QPainter pt(dst);
        pt.drawPixmap(dp, *pixmap, sr, imask ? Qt::CopyPixmapNoMask : Qt::CopyPixmap);
    } else {
        qWarning("::bitBlt only works when source is of type pixmap");
    }
}

void bitBlt(QPaintDevice *dst, int dx, int dy,
             const QPaintDevice *src, int sx, int sy, int sw, int sh,
             bool ignoreMask )
{
    bitBlt_helper(dst, QPoint(dx, dy), src, QRect(sx, sy, sw, sh), ignoreMask);
}

void bitBlt(QPaintDevice *dst, const QPoint &dp, const QPaintDevice *src, const QRect &sr, bool ignoreMask)
{
    bitBlt_helper(dst, dp, src, sr, ignoreMask);
}

void bitBlt(QPaintDevice *dst, int dx, int dy,
            const QImage *src, int sx, int sy, int sw, int sh, Qt::ImageConversionFlags flags)
{
    QPixmap srcPixmap(src->width(), src->height());
    srcPixmap.fromImage(*src, flags);
    bitBlt_helper(dst, QPoint(dx, dy), &srcPixmap, QRect(sx, sy, sw, sh), false);
}

#endif // QT_COMPAT

/*!
    \enum Qt::PaintUnit

    \compat

    \value PixelUnit
    \value LoMetricUnit Obsolete
    \value HiMetricUnit Obsolete
    \value LoEnglishUnit Obsolete
    \value HiEnglishUnit Obsolete
    \value TwipsUnit Obsolete
*/

/*!
    \fn void QPainter::setBackgroundColor(const QColor &color)

    Use setBackground() instead.
*/

/*!
    \fn const QColor &QPainter::backgroundColor() const

    Use background().color() instead.
*/

/*!
    \fn void QPainter::drawText(int x, int y, const QString &text, int pos, int len, TextDirection dir)

    Use drawText(x, y, text.mid(pos, len), dir) instead.
*/

/*!
    \fn void QPainter::drawText(const QPoint &p, const QString &text, int pos, int len, TextDirection dir)

    Use drawText(p, text.mid(pos, len), dir) instead.
*/

/*!
    \fn void QPainter::drawText(int x, int y, const QString &text, int len, TextDirection dir)

    Use drawText(x, y, text.left(len), dir) instead.
*/

/*!
    \fn void QPainter::drawText(const QPoint &p, const QString &s, int len, TextDirection dir)

    Use drawText(p, text.left(len), dir) instead.
*/

/*!
    \fn bool QPainter::begin(QPaintDevice *pdev, const QWidget *init)
###
*/


/*!
    \fn void QPainter::drawImage(const QPoint &p, const QImage &image)

    Draws the image \a i at point \a p.
*/

/*!
    \fn void QPainter::drawImage(const QPointF &p, const QImage &image)

    \overload
*/

/*!
    \fn void QPainter::drawImage(const QPointF &p, const QImage &image, const QRectF &sr,
                                 Qt::ImageConversionFlags conversionFlags = 0)

    Draws the rectangle \a sr of image \a image with its origin at point \a p.

    If the image needs to be modified to fit in a lower-resolution
    result (e.g. converting from 32-bit to 8-bit), use the \a
    conversionFlags to specify how you'd prefer this to happen.

    \sa Qt::ImageConversionFlags
*/

/*!
    \fn void QPainter::drawImage(const QPoint &p, const QImage &image, const QRect &sr,
                                 Qt::ImageConversionFlags conversionFlags = 0)
    \overload
*/

/*!
    \fn void QPainter::drawImage(const QRectF &rectangle, const QImage &image)

    Draws \a image into \a rectangle.
*/

/*!
    \fn void QPainter::drawImage(const QRect &rectangle, const QImage &image)

    \overload
*/

/*!
    \fn void QPainter::drawImage(int x, int y, const QImage &image,
                                 int sx, int sy, int sw, int sh,
                                 Qt::ImageConversionFlags conversionFlags)
    \overload

    Draws an image at (\a{x}, \a{y}) by copying a part of \a image into
    the paint device.

    (\a{x}, \a{y}) specifies the top-left point in the paint device that is
    to be drawn onto. (\a{sx}, \a{sy}) specifies the top-left point in \a
    image that is to be drawn. The default is (0, 0).

    (\a{sw}, \a{sh}) specifies the size of the image that is to be drawn.
    The default, (-1, -1), means all the way to the bottom-right of
    the image.
*/

/*!
    \fn void QPainter::drawImage(const QRect &targetRect, const QImage &image, const QRect &sourceRect,
                                 Qt::ImageConversionFlags flags)
    \overload
*/

/*!
    \fn void QPainter::redirect(QPaintDevice *pdev, QPaintDevice *replacement)

    Use setRedirected() instead.
*/

/*!
    \fn QPaintDevice *QPainter::redirect(QPaintDevice *pdev)

    Use redirected() instead.
*/

