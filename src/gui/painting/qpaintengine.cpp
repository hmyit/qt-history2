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

#include "qpaintengine.h"
#include "qpaintengine_p.h"
#include "qpainter_p.h"

#include <qdebug.h>

#include <private/qfontengine_p.h>

/*! \class QPaintEngine qpaintengine.h
  \brief The QPaintEngine class provides an abstract definition of how
  QPainter draws to a given device on a given platform.

  Qt 4.0 provides several premade implementations of QPaintEngine for
  the different painter backends we support. We provide one paint
  engine for each window system and painting framework we
  support. This includes X11 on Unix/Linux, GDI/GDI+ on Windows and
  QuickDraw/CoreGraphics on Mac OS X. In addition we provide
  QPaintEngine implementations for OpenGL (accessible through
  QGLWidget) and PostScript (accessible through QPSPrinter on X11).

  If one wants to use QPainter to draw to a different backend, such as
  PDF, one must subclass QPaintEngine and reimplement all its virtual
  functions. The QPaintEngine implementation is then made available by
  subclassing QPaintDevice and reimplementing the virtual function \c
  QPaintDevice::paintEngine().

  QPaintEngine is created and owned by the QPaintDevice that created it.

  The big advantage of the QPaintEngine approach opposed to the
  previous QPainter/QPaintDevice::cmd() approach is that it is now
  possible to adapt to multiple technologies on each platform and take
  advantage of the to the fullest.

  \sa QPainter, QPaintDevice::paintEngine()
*/

/*!
  \enum QPaintEngine::PaintEngineFeature

  This enum is used to describe the features or capabilities that the
  paint engine has. If a feature is not supported by the engine,
  QPainter will do a best effort to emulate that feature through other
  means. The features that are currently emulated are: \c
  CoordTransform, \c PixmapTransform, \c LinearGradients, \c
  PixmapScale, \c SolidAlphaFill and \c ClipTransform.

  \value CoordTransform The engine can transform the points in a
  drawing operation.

  \value PenWidthTransform The engine has support for transforming pen
  widths.

  \value PatternTransform The engine has support for transforming brush
  patterns.

  \value PixmapTransform The engine can transform pixmaps, including
  rotation and shearing.

  \value LinearGradients The engine can fill with linear gradients

  \value DrawRects The engine can draw rectangles.

  \value PixmapScale The engine can scale pixmaps.

  \value SolidAlphaFill The engine can fill and outline with alpha colors

  \value PainterPaths The engine has path support.

  \value ClipTransform The engine is capable of transforming clip regions.

    \value PaintOutsidePaintEvent The engine is capable of painting
    outside of paint events.

  \omitvalue UsesFontEngine
*/

/*!
  \enum QPaintEngine::DirtyFlags

  \internal

  This enum is used by QPainter to trigger lazy updates of the various states
  in the QPaintEngine
*/

/*!
    \enum QPaintEngine::PolygonDrawMode

    \value OddEvenMode
    \value WindingMode
    \value ConvexMode
    \value UnconnectedMode
*/

/*!
    \fn void QPaintEngine::drawPolygon(const QPointArray &pa, PolygonDrawMode mode)

    Reimplement this pure virtual function to draw a polygon based on
    the points in \a pa using the given drawing \a mode.
*/


#define d d_func()
#define q q_func()

/*!
    \enum QPaintEngine::Type

    \value X11
    \value Windows
    \value Gdiplus (same as Windows)
    \value QuickDraw (same as CoreGraphics)
    \value MacPrinter
    \value CoreGraphics Mac OS X
    \value QWindowSystem Qt/Embedded
    \value PostScript
    \value OpenGL
    \value Picture QPicture format
    \value SVG Scalable Vector Graphics XML format
    \value User First user type ID
    \value MaxUser Last user type ID
*/

/*!
    \fn bool QPaintEngine::isActive() const

    Returns true if the paint engine is actively drawing; otherwise
    returns false.

    \sa setActive()
*/

/*!
    \fn void QPaintEngine::setActive(bool state)

    Sets the active state of the paint engine to \a state.

    \sa isActive()
*/

/*!
    \fn bool QPaintEngine::begin(QPaintDevice *pdev)

    Reimplement this function to initialise your paint engine when
    painting is to start on the paint device \a pdev. Return true if
    the initialization was successful; otherwise return false.

    \sa end() isActive()
*/

/*!
    \fn bool QPaintEngine::end()

    Reimplement this function to finish painting on the current paint
    device. Return true if painting was finished successfully;
    otherwise return false.

    \sa begin() isActive()
*/

/*!
    \fn void QPaintEngine::drawLine(const QPoint &p1, const QPoint &p2)

    Reimplement this function to draw a line from point \a p1 to point \a p2.
    The default implementation calls drawPolygon.
*/

void QPaintEngine::drawLine(const QPoint &p1, const QPoint &p2)
{
    QPointArray pa;
    pa << p1 << p2;
    drawPolygon(pa, UnconnectedMode);
}

/*!
    \fn void QPaintEngine::drawRect(const QRect &rectangle)

    Reimplement this function to draw the given \a rectangle.
*/

void QPaintEngine::drawRect(const QRect &r)
{
    drawPolygon(r, ConvexMode);
}

/*!
    \fn void QPaintEngine::drawPoint(const QPoint &point)

    Reimplement this function to draw the given \a point.
*/

/*!
    Calls drawPoint() to draw every point in the point array \a pa.
*/

void QPaintEngine::drawPoints(const QPointArray &pa)
{
    for (int i=0; i<pa.size(); ++i)
        drawPoint(pa.at(i));
}

/*!
    \fn void QPaintEngine::drawEllipse(const QRect &rectangle)

    Reimplement this function to draw the largest ellipse that can be
    contained within the given \a rectangle.

    The default implementation calls drawPolygon

    \sa drawPolygon
*/

void QPaintEngine::drawEllipse(const QRect &r)
{
    QPointArray a;
    a.makeEllipse(r.x(), r.y(), r.width(), r.height());
    drawPolygon(a, ConvexMode);
}

/*!
    Calls drawLine() for every pair of points in the point array \a
    pa.
*/

void QPaintEngine::drawLineSegments(const QPointArray &pa)
{
    for (int i=0; i+1<pa.size(); i+=2)
        drawLine(pa.at(i), pa.at(i+1));
}

/*!
    \fn void QPaintEngine::drawPixmap(const QRect &rectangle, const QPixmap
    &pixmap, const QRect &sr, Qt::PixmapDrawingMode mode)

    Reimplement this function to draw the part of the \a pixmap
    specified by the \a sr rectangle in the given \a rectangle using
    the given drawing \a mode.
*/

/*!
    \fn void QPaintEngine::drawTiledPixmap(const QRect &rectangle, const
    QPixmap &pixmap, const QPoint &point, Qt::PixmapDrawingMode mode)

    Reimplement this function to draw the \a pixmap in the given \a
    rectangle, starting at the given \a point. The pixmap will be
    drawn repeatedly until the \a rectangle is filled using the given
    \a mode.
*/

/*!
    \fn Type QPaintEngine::type() const

    Reimplement this function to return the paint engine \l{Type}.
*/

/*!
    \fn bool QPaintEngine::testf(uint b) const

    \internal
*/

/*!
    \fn void QPaintEngine::setf(uint b)

    \internal
*/

/*!
    \fn void QPaintEngine::clearf(uint b)

    \internal
*/

/*!
    \fn void QPaintEngine::assignf(uint b)

    \internal
*/

/*!
    \fn void QPaintEngine::fix_neg_rect(int *x, int *y, int *w, int *h);

    \internal
*/

/*!
    \fn bool QPaintEngine::hasClipping() const

    \internal
*/

/*!
    \fn bool QPaintEngine::testDirty(DirtyFlags df)

    \internal
*/

/*!
    \fn void QPaintEngine::setDirty(DirtyFlags df)

    \internal
*/

/*!
    \fn void QPaintEngine::clearDirty(DirtyFlags df)

    \internal
*/

/*!
    \fn bool QPaintEngine::hasFeature(PaintEngineFeatures feature) const

    Returns true if the paint engine supports the specified \a
    feature; otherwise returns false.
*/

/*!
    \fn void QPaintEngine::updateState(QPainterState *state, bool updateGC)

    \internal
*/


/*!
  Creates a paint engine with the featureset specified by \a caps.
*/

QPaintEngine::QPaintEngine(PaintEngineFeatures caps)
    : dirtyFlag(0),
      active(0),
      selfDestruct(false),
      state(0),
      gccaps(caps),
      d_ptr(new QPaintEnginePrivate)
{
    d_ptr->q_ptr = this;
}

/*!
  \internal
*/

QPaintEngine::QPaintEngine(QPaintEnginePrivate &dptr, PaintEngineFeatures caps)
    : dirtyFlag(0),
      active(0),
      selfDestruct(false),
      state(0),
      gccaps(caps),
      d_ptr(&dptr)
{
    d_ptr->q_ptr = this;
}

/*!
    Destroys the paint engine.
*/
QPaintEngine::~QPaintEngine()
{
    delete d_ptr;
}

/*!
    Returns the paint engine's painter.
*/
QPainter *QPaintEngine::painter() const
{
    return state->painter;
}

/*!
  \internal

  This function is responsible for calling the reimplemented updateXXX functions in
  the QPaintEngine based on what is currently marked as dirty. If \a updateGC is
  false we don't call the update functions.
*/

void QPaintEngine::updateInternal(QPainterState *s, bool updateGC)
{
    Q_ASSERT(state);
    if (!s || !updateGC) {
        state = s;
        return;
    }

    // assign state after we start checking so the if works, but before update
    // calls since we need it in some cases..
    if (s->painter != state->painter) {
        setDirty(AllDirty);
    } else if (s != state) {
        dirtyFlag = state->changeFlags;
    } else {
        state->changeFlags |= dirtyFlag;
    }
    state = s;

    if (testDirty(DirtyPen)) {
        updatePen(s->pen);
        clearDirty(DirtyPen);
    }
    if (testDirty(DirtyBrush)) {
        updateBrush(s->brush, s->bgOrigin);
        clearDirty(DirtyBrush);
    }
    if (testDirty(DirtyFont)) {
        updateFont(s->font);
        clearDirty(DirtyFont);
    }
    if (testDirty(DirtyBackground)) {
        updateBackground(s->bgMode, s->bgBrush);
        clearDirty(DirtyBackground);
    }
    bool joinWithPath = false;
    if (testDirty(DirtyClipPath)) {
        // Assume for now that painterpaths implies native clip xform.
        if (hasFeature(PainterPaths)) {
            updateMatrix(s->clipPathMatrix);
            updateClipPath(s->clipPath, s->clipEnabled);
            setDirty(DirtyTransform);
        } else {
            joinWithPath = true;
            setDirty(DirtyClip);
        }
        clearDirty(DirtyClipPath);
    }
    if (testDirty(DirtyClip)) {
        if (hasFeature(ClipTransform)) {
            if (joinWithPath) {
                qWarning("QPaintEngine::updateInternal(), feature combination ClipTransform and"
                         " not PainterPaths is not supported");
            }
            updateMatrix(s->clipRegionMatrix);
            updateClipRegion(s->clipRegion, s->clipEnabled);
            setDirty(DirtyTransform);
        } else {
            QRegion region = s->txop > QPainter::TxNone
                             ? (s->clipRegion * s->clipRegionMatrix)
                             : s->clipRegion;
            if (joinWithPath && !s->clipPathRegion.isEmpty())
                region &= s->clipPathRegion * s->clipPathMatrix;
            updateClipRegion(region, s->clipEnabled);
        }
        clearDirty(DirtyClip);
    }
    if (testDirty(DirtyTransform)) {
        updateMatrix(s->matrix);
        clearDirty(DirtyTransform);
    }
    if (testDirty(DirtyHints)) {
        updateRenderHints(d->renderhints);
        clearDirty(DirtyHints);
    }

    // It might be the case that a update call flags a previously
    // updated state to dirty. For this case we need to call
    // updateInternal() again to update these states. This is to be
    // sure that all states are in sync when the function returns.
    if (dirtyFlag)
        updateInternal(state);
}

/*!
    The default implementation ignores the \a path and does nothing.
*/

void QPaintEngine::drawPath(const QPainterPath &)
{

}

/*!
    The default implementation splits \a line into two points and
    calls the integer version of drawLine().
*/
void QPaintEngine::drawLine(const QLineF &line)
{
    drawLine(line.start().toPoint(), line.end().toPoint());
}

/*!
    \overload

    The default implementation converts \a rf to an integer rectangle
    and calls the integer version of drawRect().
*/
void QPaintEngine::drawRect(const QRectF &rf)
{
    drawRect(rf.toRect());
}

/*!
    \overload

    The default implementation converts \a pf to an integer point and
    calls the integer version of drawPoint().
*/
void QPaintEngine::drawPoint(const QPointF &pf)
{
    drawPoint(pf.toPoint());
}


/*!
    This function draws the text item \a ti at position \a p in
    accordance with the given \a textFlags. The default implementation
    of this function renders the text to a pixmap and draws the
    resultant pixmap.
*/

void QPaintEngine::drawTextItem(const QPoint &p, const QTextItem &ti, int textFlags)
{
#ifndef Q_WS_X11
    bool useFontEngine = false;
    if (hasFeature(QPaintEngine::UsesFontEngine)) {
	useFontEngine = true;
        if (state->txop > QPainter::TxTranslate) {
            useFontEngine = false;
            QFontEngine *fe = ti.fontEngine;
            QFontEngine::FECaps fecaps = fe->capabilites();
            if (state->txop == QPainter::TxRotShear) {
                useFontEngine = (fecaps == QFontEngine::FullTransformations);
                if (!useFontEngine
                    && state->matrix.m11() == state->matrix.m22()
                    && state->matrix.m12() == -state->matrix.m21())
                    useFontEngine = (fecaps & QFontEngine::RotScale) == QFontEngine::RotScale;
            } else if (state->txop == QPainter::TxScale) {
                useFontEngine = (fecaps & QFontEngine::Scale);
            }
        }
        if (useFontEngine) {
            ti.fontEngine->draw(this, p.x(),  p.y(), ti, textFlags);
        }
    }
#else
    const bool useFontEngine = false;
#endif

    if (!useFontEngine) {
        // Fallback: rasterize into a pixmap and draw the pixmap
        QPixmap pm(ti.width, ti.ascent + ti.descent);
        pm.fill(Qt::white);

        QPainter painter;
        painter.begin(&pm);
        painter.setPen(Qt::black);
        painter.drawTextItem(0, ti.ascent, ti, textFlags);
        painter.end();

        QImage img = pm.toImage();
        if (img.depth() != 32)
            img = img.convertDepth(32);
        img.setAlphaBuffer(true);
        int i = 0;
        while (i < img.height()) {
            uint *p = (uint *) img.scanLine(i);
            uint *end = p + img.width();
            while (p < end) {
                *p = ((0xff - qGray(*p)) << 24) | (state->pen.color().rgb() & 0x00ffffff);
                ++p;
            }
            ++i;
        }

        pm = img;
        state->painter->drawPixmap(p.x(), p.y() - ti.ascent, pm);
    }
}

/*!
  Draws the rectangles in the list \a rects.
*/
void QPaintEngine::drawRects(const QList<QRect> &rects)
{
    for (int i=0; i<rects.size(); ++i)
        drawRect(rects.at(i));
}

/*!
  Returns the set of supported renderhints.
 */
QPainter::RenderHints QPaintEngine::supportedRenderHints() const
{
    return 0;
}

/*!
  Returns the currently set renderhints.
*/
QPainter::RenderHints QPaintEngine::renderHints() const
{
    return d->renderhints;
}

/*!
  Sets the the render hints specified by \a hints in addition to the
  currently set render hints.

  \sa clearRenderHints()
*/
void QPaintEngine::setRenderHints(QPainter::RenderHints hints)
{
    if (QPainter::RenderHints(d->renderhints & hints) != hints) {
        d->renderhints |= hints;
        setDirty(DirtyHints);
    }
}

/*!
  Clears the render hints specified by \a hints.
*/
void QPaintEngine::clearRenderHints(QPainter::RenderHints hints)
{
    if (QPainter::RenderHints(d->renderhints & hints) != 0) {
        d->renderhints &= ~hints;
        setDirty(DirtyHints);
    }
}

/*!
  This function is caleed when the engine needs to be updated with
  the new set of renderhints specified by \a hints.
*/

void QPaintEngine::updateRenderHints(QPainter::RenderHints /*hints*/)
{
}


/*!
    This function is called when the engine needs to be updated with
    the new clip \a path. The value of \a enabled signifies whether or
    not the clippath should be enabled.
*/
void QPaintEngine::updateClipPath(const QPainterPath &path, bool enabled)
{
    Q_UNUSED(path);
    Q_UNUSED(enabled);
}

/*!
  \fn QPaintEngine::updatePen(const QPen &pen)

  This function is called when the engine needs to be updated with the
  a new pen, specified by \a pen.
*/


/*!
  \fn QPaintEngine::updateBrush(const QBrush &brush, const QPoint &origin)

  This function is called when the engine needs to be updated with
  a new brush, specified with \a brush. \a origin describes the brush origin.
*/

/*!
  \fn QPaintEngine::updateFont(const QFont &f)

  This function is called when the engine needs to be updated with
  a new font, specified by \a f
*/

/*!
  \fn QPaintEngine::updateBackground(Qt::BGMode bgmode, const QBrush &brush)

  This function is called when the engine needs to be updated with
  new background settings. \a bgmode describes the background mode and
  \a brush describes the background brush.
*/

/*!
  \fn QPaintEngine::updateMatrix(const QMatrix &matrix)

  This function is called when the engine needs to be updated with
  new transformation settings, specified with \a matrix.
*/

/*!
  \fn QPaintEngine::updateClipRegion(const QRegion &region, bool enabled)

  This function is called when the clip region changes, specified by \a region or
  when clipping is enabled or disabled, specified by \a enabled.
*/

