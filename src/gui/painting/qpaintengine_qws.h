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

#ifndef QPAINTENGINE_QWS_H
#define QPAINTENGINE_QWS_H

#include "qatomic.h"
#include "qpaintengine.h"

class QGfx;
struct QWSPaintEngineData;
class QWSPaintEnginePrivate;
class QPainterState;
class QApplicationPrivate;

class QWSPaintEngine : public QPaintEngine
{
    Q_DECLARE_PRIVATE(QWSPaintEngine)

public:
    QWSPaintEngine();
    ~QWSPaintEngine();

    bool begin(QPaintDevice *pdev);
    bool end();

    void updatePen(const QPen &pen);
    void updateBrush(const QBrush &brush, const QPoint &pt);
    void updateFont(const QFont &font);
    void updateBackground(Qt::BGMode bgmode, const QBrush &bgBrush);
    void updateXForm(const QMatrix &matrix);
    void updateClipRegion(const QRegion &region, bool clipEnabled);

    void drawLine(const QPoint &p1, const QPoint &p2);
    void drawRect(const QRect &r);
    void drawPoint(const QPoint &p);
    void drawPoints(const QPointArray &pa);
    void drawEllipse(const QRect &r);
    void drawPolygon(const QPointArray &pa, PolygonDrawMode mode);

    void drawPixmap(const QRect &r, const QPixmap &pm, const QRect &sr, Qt::PixmapDrawingMode mode);
    void drawTiledPixmap(const QRect &r, const QPixmap &pixmap, const QPoint &s, Qt::PixmapDrawingMode mode);

    virtual Qt::HANDLE handle() const;
    inline Type type() const { return QPaintEngine::QWindowSystem; }

    static void initialize();
    static void cleanup();


    QGfx *gfx();

protected:
    QWSPaintEngine(QPaintEnginePrivate &dptr);

    void drawPolyInternal(const QPointArray &a, bool close=true);

    void copyQWSData(const QWSPaintEngine *);
    void cloneQWSData(const QWSPaintEngine *);
    //virtual void setQWSData(const QWSPaintEngineData *);
    QWSPaintEngineData* getQWSData(bool def = false) const;

    friend void qt_init(QApplicationPrivate *, int);
    friend void qt_cleanup();
    friend void qt_draw_transformed_rect(QPainter *pp,  int x, int y, int w,  int h, bool fill);
    friend void qt_draw_background(QPainter *pp, int x, int y, int w,  int h);
    friend class QWidget;
    friend class QPixmap;
    friend class QFontEngineBox;
    friend class QFontEngineXft;
    friend class QFontEngineXLFD;

private:
//    friend class QWidget;
//    friend class QPixmap;
    friend class QFontEngine;

   //QWSPaintEngineData *qwsData;

private:
#if defined(Q_DISABLE_COPY)
    QWSPaintEngine(const QWSPaintEngine &);
    QWSPaintEngine &operator=(const QWSPaintEngine &);
#endif
};

/* I don't know where you use this, but I removed the QShared and gave you a QAtomic --tws */
struct QWSPaintEngineData {
    QAtomic ref;
    /*Display*/ void *x_display;
    int x_screen;
    int x_depth;
    int x_cells;
    Qt::HANDLE x_colormap;
    bool x_defcolormap;
    void *x_visual;
    bool x_defvisual;
};

#endif
