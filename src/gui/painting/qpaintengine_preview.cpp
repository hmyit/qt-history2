/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <private/qpaintengine_preview_p.h>
#include <private/qpainter_p.h>
#include <private/qpaintengine_p.h>
#include <private/qpicture_p.h>

#include <QtGui/qprintengine.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpicture.h>

class QPreviewPaintEnginePrivate : public QPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(QPreviewPaintEngine)
public:
    QPreviewPaintEnginePrivate() {}
    ~QPreviewPaintEnginePrivate() {}

    QList<const QPicture *> pages;
    QPaintEngine *engine;
    QPainter *painter;

    QPaintEngine *proxy_paint_engine;
    QPrintEngine *proxy_print_engine;
};


QPreviewPaintEngine::QPreviewPaintEngine()
    : QPaintEngine(*(new QPreviewPaintEnginePrivate), AllFeatures)
{
    Q_D(QPreviewPaintEngine);
    d->proxy_print_engine = 0;
    d->proxy_paint_engine = 0;
}

QPreviewPaintEngine::~QPreviewPaintEngine()
{
    Q_D(QPreviewPaintEngine);

    qDeleteAll(d->pages);
}

bool QPreviewPaintEngine::begin(QPaintDevice *)
{
    Q_D(QPreviewPaintEngine);

    qDeleteAll(d->pages);
    d->pages.clear();

    QPicture *page = new QPicture;
    page->d_func()->in_memory_only = true;
    d->painter = new QPainter(page);
    d->engine = d->painter->paintEngine();
    d->pages.append(page);
    return true;
}

bool QPreviewPaintEngine::end()
{
    Q_D(QPreviewPaintEngine);

    delete d->painter;
    d->painter = 0;
    d->engine = 0;
    return true;
}

void QPreviewPaintEngine::updateState(const QPaintEngineState &state)
{
    Q_D(QPreviewPaintEngine);
    d->engine->updateState(state);
}

void QPreviewPaintEngine::drawPath(const QPainterPath &path)
{
    Q_D(QPreviewPaintEngine);
    d->engine->drawPath(path);
}

void QPreviewPaintEngine::drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
    Q_D(QPreviewPaintEngine);
    d->engine->drawPolygon(points, pointCount, mode);
}

void QPreviewPaintEngine::drawTextItem(const QPointF &p, const QTextItem &textItem)
{
    Q_D(QPreviewPaintEngine);
    d->engine->drawTextItem(p, textItem);
}

void QPreviewPaintEngine::drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr)
{
    Q_D(QPreviewPaintEngine);
    d->engine->drawPixmap(r, pm, sr);
}

void QPreviewPaintEngine::drawTiledPixmap(const QRectF &r, const QPixmap &pm, const QPointF &p)
{
    Q_D(QPreviewPaintEngine);
    d->engine->drawTiledPixmap(r, pm, p);
}

bool QPreviewPaintEngine::newPage()
{
    Q_D(QPreviewPaintEngine);

    QPicture *page = new QPicture;
    page->d_func()->in_memory_only = true;
    QPainter *tmp_painter = new QPainter(page);
    QPaintEngine *tmp_engine = tmp_painter->paintEngine();

    // copy the painter state from the original painter
    Q_ASSERT(painter()->d_func()->state && tmp_painter->d_func()->state);
    *tmp_painter->d_func()->state = *painter()->d_func()->state;

    // composition modes aren't supported on a QPrinter and yields a
    // warning, so ignore it for now
    tmp_engine->setDirty(DirtyFlags(AllDirty & ~DirtyCompositionMode));
    tmp_engine->syncState();

    delete d->painter;
    d->painter = tmp_painter;
    d->pages.append(page);
    d->engine = tmp_engine;
    return true;
}

bool QPreviewPaintEngine::abort()
{
    Q_D(QPreviewPaintEngine);
    end();
    qDeleteAll(d->pages);

    return true;
}

QList<const QPicture *> QPreviewPaintEngine::pages()
{
    Q_D(QPreviewPaintEngine);
    return d->pages;
}

void QPreviewPaintEngine::setProxyEngines(QPrintEngine *printEngine, QPaintEngine *paintEngine)
{
    Q_D(QPreviewPaintEngine);
    d->proxy_print_engine = printEngine;
    d->proxy_paint_engine = paintEngine;
}

void QPreviewPaintEngine::setProperty(PrintEnginePropertyKey key, const QVariant &value)
{
    Q_D(QPreviewPaintEngine);
    d->proxy_print_engine->setProperty(key, value);
}

QVariant QPreviewPaintEngine::property(PrintEnginePropertyKey key) const
{
    Q_D(const QPreviewPaintEngine);
    return d->proxy_print_engine->property(key);
}

int QPreviewPaintEngine::metric(QPaintDevice::PaintDeviceMetric id) const
{
    Q_D(const QPreviewPaintEngine);
    return d->proxy_print_engine->metric(id);
}

QPrinter::PrinterState QPreviewPaintEngine::printerState() const
{
    Q_D(const QPreviewPaintEngine);
    return d->proxy_print_engine->printerState();
}
