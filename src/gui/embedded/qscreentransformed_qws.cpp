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

#include "qgfxtransformed_qws.h"

#ifndef QT_NO_QWS_TRANSFORMED

#include "qmemorymanager_qws.h"
#include "qwsdisplay_qws.h"
#include "qgfxraster_qws.h"

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

static QTransformedScreen *qt_trans_screen = 0;

void qws_setScreenTransformation(int t)
{
    if (qt_trans_screen)
        qt_trans_screen->setTransformation((QTransformedScreen::Transformation)t);
}

// Unaccelerated screen/driver setup. Can be overridden by accelerated
// drivers

QTransformedScreen::QTransformedScreen(int display_id)
    : QT_TRANS_SCREEN_BASE(display_id)
{
    qt_trans_screen = this;
    trans = None;
}

QTransformedScreen::~QTransformedScreen()
{
}

void QTransformedScreen::setTransformation(Transformation t)
{
    trans = t;

    QSize s = mapFromDevice(QSize(dw,dh));
    w = s.width();
    h = s.height();
}

bool QTransformedScreen::connect(const QString &displaySpec)
{
    if (displaySpec.contains(":Rot270"))
        trans = Rot270;
    else if (displaySpec.contains(":Rot180"))
        trans = Rot180;
    else if (displaySpec.contains(":Rot90"))
        trans = Rot90;

    bool result = QT_TRANS_SCREEN_BASE::connect(displaySpec);
    if (result) {
        QSize s = mapFromDevice(QSize(w, h));
        w = s.width();
        h = s.height();
    }

    return result;
}

bool QTransformedScreen::isTransformed() const
{
    return trans != None;
}

QSize QTransformedScreen::mapToDevice(const QSize &s) const
{
    if (trans == Rot90 || trans == Rot270) {
        return QSize(s.height(), s.width());
    }

    return s;
}

QSize QTransformedScreen::mapFromDevice(const QSize &s) const
{
    if (trans == Rot90 || trans == Rot270) {
        return QSize(s.height(), s.width());
    }

    return s;
}

QPoint QTransformedScreen::mapToDevice(const QPoint &p, const QSize &s) const
{
    QPoint rp(p);

    switch (trans) {
        case Rot90:
            rp.setX(p.y());
            rp.setY(s.width() - p.x() - 1);
            break;
        case Rot180:
            rp.setX(s.width() - p.x() - 1);
            rp.setY(s.height() - p.y() - 1);
            break;
        case Rot270:
            rp.setX(s.height() - p.y() - 1);
            rp.setY(p.x());
            break;
        default:
            break;
    }

    return rp;
}

QPoint QTransformedScreen::mapFromDevice(const QPoint &p, const QSize &s) const
{
    QPoint rp(p);

    switch (trans) {
        case Rot90:
            rp.setX(s.height() - p.y() - 1);
            rp.setY(p.x());
            break;
        case Rot180:
            rp.setX(s.width() - p.x() - 1);
            rp.setY(s.height() - p.y() - 1);
            break;
        case Rot270:
            rp.setX(p.y());
            rp.setY(s.width() - p.x() - 1);
            break;
        default:
            break;
    }

    return rp;
}

QRect QTransformedScreen::mapToDevice(const QRect &r, const QSize &s) const
{
    QRect tr;
    switch (trans) {
        case Rot90:
            tr.setCoords(r.y(), s.width() - r.x() - 1,
                          r.bottom(), s.width() - r.right() - 1);
            break;
        case Rot180:
            tr.setCoords(s.width() - r.x() - 1, s.height() - r.y() - 1,
                          s.width() - r.right() - 1, s.height() - r.bottom() - 1);
            break;
        case Rot270:
            tr.setCoords(s.height() - r.y() - 1, r.x(),
                          s.height() - r.bottom() - 1, r.right());
            break;
        default:
            tr = r;
            break;
    }

    return tr.normalized();
}

QRect QTransformedScreen::mapFromDevice(const QRect &r, const QSize &s) const
{
    QRect tr;
    switch (trans) {
        case Rot90:
            tr.setCoords(s.height() - r.y() - 1, r.x(),
                          s.height() - r.bottom() - 1, r.right());
            break;
        case Rot180:
            tr.setCoords(s.width() - r.x() - 1, s.height() - r.y() - 1,
                          s.width() - r.right() - 1, s.height() - r.bottom() - 1);
            break;
        case Rot270:
            tr.setCoords(r.y(), s.width() - r.x() - 1,
                          r.bottom(), s.width() - r.right() - 1);
            break;
        default:
            tr = r;
            break;
    }

    return tr.normalized();
}

template<class T>
static inline void rotateLoopTemplate(
        const uchar *src, int srcBytesPerLine,
        uchar *dst, int dstBytesPerLine,
        int width, int height,
        QTransformedScreen::Transformation trans, bool mapToDevice)
{
    int dstXAdd = 0;
    int dstYAdd = 0;
    int dstXOfs = 0;
    int dstYOfs = 0;
    int srcYAdd = srcBytesPerLine - width * sizeof(T);

    //static int times = 0;
    //printf("called %i times\n", times++);

    if (!mapToDevice) {
        if (trans == QTransformedScreen::Rot90)
            trans = QTransformedScreen::Rot270;
        else if (trans == QTransformedScreen::Rot270)
            trans = QTransformedScreen::Rot90;
    }

    switch (trans) {
        case QTransformedScreen::Rot90:
            dstXOfs = 0;
            dstYOfs = width - 1;
            dstXAdd = -dstBytesPerLine;
            dstYAdd = 1 * sizeof(T) + width * dstBytesPerLine;
            break;
        case QTransformedScreen::Rot270:
            dstXOfs = height - 1;
            dstYOfs = 0;
            dstXAdd = dstBytesPerLine;
            dstYAdd = -1 * sizeof(T) - width * dstBytesPerLine;
            break;
        default:
            dstXOfs = width - 1;
            dstYOfs = height - 1;
            dstXAdd = -1 * sizeof(T);
            dstYAdd = -dstBytesPerLine + width * sizeof(T);
            break;
    };

    T *dstPtr = (T *)(dst + dstYOfs * dstBytesPerLine) + dstXOfs;
    T *srcPtr = (T *)src;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            *dstPtr = *srcPtr++;
            dstPtr = (T *)((uchar*)dstPtr + dstXAdd); // add dstXAdd number of bytes
        }
        srcPtr = (T *)((uchar*)srcPtr + srcYAdd); // add srcYAdd number of bytes
        dstPtr = (T *)((uchar*)dstPtr + dstYAdd); // add dstYAdd number of bytes
    }
}

QImage QTransformedScreen::mapToDevice(const QImage &img) const
{
    if (img.isNull() || trans == None)
        return img;

    int iw = img.width();
    int ih = img.height();
    int w = iw;
    int h = ih;
    if (trans == Rot90 || trans == Rot270) {
        w = ih;
        h = iw;
    }

    QImage rimg(w, h, img.depth(), img.numColors(), img.bitOrder());

    for (int i = 0; i < img.numColors(); i++) {
        rimg.colorTable()[i] = img.colorTable()[i];
    }

    // Optimized image rotation code for nice bit depths
    int d = img.depth();
    if (d == 8 || d == 16 || d == 32) {
        int srcBytesPerLine = img.bytesPerLine();
        int dstBytesPerLine = rimg.bytesPerLine();
        const uchar *srcBits = img.bits();
        uchar *dstBits = rimg.bits();
        switch (d) {
            case 8:
                rotateLoopTemplate<uchar>(srcBits, srcBytesPerLine, dstBits, dstBytesPerLine, iw, ih, trans, true);
                break;
            case 16:
                rotateLoopTemplate<ushort>(srcBits, srcBytesPerLine, dstBits, dstBytesPerLine, iw, ih, trans, true);
                break;
            case 32:
                rotateLoopTemplate<uint>(srcBits, srcBytesPerLine, dstBits, dstBytesPerLine, iw, ih, trans, true);
                break;
        }
        rimg.setAlphaBuffer(img.hasAlphaBuffer());
        rimg.setOffset(img.offset());
        return rimg;
    }

    // Slower fall back code for image rotation for 1-bit and other depths
    #define ROTATE_LOOP(X, Y, VAL) \
                    for (int y = 0; y < ih; y++) { \
                        for (int x = 0; x < iw; x++) { \
                            rimg.setPixel(X, Y, VAL); \
                        } \
                    } \
                    break;

    if (img.depth() > 8) {
        switch (trans) {
            case Rot90:
                ROTATE_LOOP(y, iw - x - 1, img.pixel(x, y))
            case Rot270:
                ROTATE_LOOP(ih - y - 1, x, img.pixel(x, y));
            default:
                ROTATE_LOOP(iw - x - 1, ih - y - 1, img.pixel(x, y));
        }
    } else {
        switch (trans) {
            case Rot90:
                ROTATE_LOOP(y, iw - x - 1, img.pixelIndex(x, y));
            case Rot270:
                ROTATE_LOOP(ih - y - 1, x, img.pixelIndex(x, y));
            default:
                ROTATE_LOOP(iw - x - 1, ih - y - 1, img.pixelIndex(x, y));
        }
    }

    #undef ROTATE_LOOP

    rimg.setAlphaBuffer(img.hasAlphaBuffer());
    rimg.setOffset(img.offset());

    return rimg;
}

QImage QTransformedScreen::mapFromDevice(const QImage &img) const
{
    if (img.isNull() || trans == None)
        return img;

    int iw = img.width();
    int ih = img.height();
    int w = iw;
    int h = ih;
    if (trans == Rot90 || trans == Rot270) {
        w = ih;
        h = iw;
    }

    QImage rimg(w, h, img.depth(), img.numColors(), img.bitOrder());

    for (int i = 0; i < img.numColors(); i++) {
        rimg.colorTable()[i] = img.colorTable()[i];
    }

    // Optimized image rotation code for nice bit depths
    int d = img.depth();
    if (d == 8 || d == 16 || d == 32) {
        int srcBytesPerLine = img.bytesPerLine();
        int dstBytesPerLine = rimg.bytesPerLine();
        const uchar *srcBits = img.bits();
        uchar *dstBits = rimg.bits();
        switch (d) {
            case 8:
                rotateLoopTemplate<uchar>(srcBits, srcBytesPerLine, dstBits, dstBytesPerLine, iw, ih, trans, false);
                break;
            case 16:
                rotateLoopTemplate<ushort>(srcBits, srcBytesPerLine, dstBits, dstBytesPerLine, iw, ih, trans, false);
                break;
            case 32:
                rotateLoopTemplate<uint>(srcBits, srcBytesPerLine, dstBits, dstBytesPerLine, iw, ih, trans, false);
                break;
        }
        rimg.setAlphaBuffer(img.hasAlphaBuffer());
        rimg.setOffset(img.offset());
        return rimg;
    }

    // Slower fall back code for image rotation for 1-bit and other depths
    #define ROTATE_LOOP(X, Y, VAL) \
                    for (int y = 0; y < ih; y++) { \
                        for (int x = 0; x < iw; x++) { \
                            rimg.setPixel(X, Y, VAL); \
                        } \
                    } \
                    break;

    if (img.depth() > 8) {
        switch (trans) {
            case Rot90:
                ROTATE_LOOP(ih - y - 1, x, img.pixel(x, y));
            case Rot270:
                ROTATE_LOOP(y, iw - x - 1, img.pixel(x, y))
            default:
                ROTATE_LOOP(iw - x - 1, ih - y - 1, img.pixel(x, y));
        }
    } else {
        switch (trans) {
            case Rot90:
                ROTATE_LOOP(ih - y - 1, x, img.pixelIndex(x, y));
            case Rot270:
                ROTATE_LOOP(y, iw - x - 1, img.pixelIndex(x, y));
            default:
                ROTATE_LOOP(iw - x - 1, ih - y - 1, img.pixelIndex(x, y));
        }
    }

    #undef ROTATE_LOOP

    rimg.setAlphaBuffer(img.hasAlphaBuffer());
    rimg.setOffset(img.offset());

    return rimg;
}

QRegion QTransformedScreen::mapToDevice(const QRegion &rgn, const QSize &s) const
{
    if (trans == None)
        return rgn;

    QRegion trgn;
    QVector<QRect> a = rgn.rects();
    QRect tr;
    const QRect *r = a.data();

    int w = s.width();
    int h = s.height();
    int size = a.size();

    switch (trans) {
        case Rot270:
            for (int i = 0; i < size; i++, r++) {
                tr.setCoords(h - r->y() - 1, r->x(),
                             h - r->bottom() - 1, r->right());
                trgn |= tr.normalized();
            }
            break;
        case Rot90:
            for (int i = 0; i < size; i++, r++) {
                tr.setCoords(r->y(), w - r->x() - 1,
                             r->bottom(), w - r->right() - 1);
                trgn |= tr.normalized();
            }
            break;
        case Rot180:
            for (int i = 0; i < size; i++, r++) {
                tr.setCoords(w - r->x() - 1, h - r->y() - 1,
                             w - r->right() - 1, h - r->bottom() - 1);
                trgn |= tr.normalized();
            }
            break;
        default:
            break;
    }

    return trgn;
}

QRegion QTransformedScreen::mapFromDevice(const QRegion &rgn, const QSize &s) const
{
    if (trans == None)
        return rgn;

    QRegion trgn;
    QVector<QRect> a = rgn.rects();
    const QRect *r = a.data();
    QRect tr;

    int w = s.width();
    int h = s.height();
    int size = a.size();

    switch (trans) {
        case Rot270:
            for (int i = 0; i < size; i++, r++) {
                tr.setCoords(r->y(), w - r->x() - 1,
                             r->bottom(), w - r->right() - 1);
                trgn |= tr.normalized();
            }
            break;
        case Rot90:
            for (int i = 0; i < size; i++, r++) {
                tr.setCoords(h - r->y() - 1, r->x(),
                             h - r->bottom() - 1, r->right());
                trgn |= tr.normalized();
            }
            break;
        case Rot180:
            for (int i = 0; i < size; i++, r++) {
                tr.setCoords(w - r->x() - 1, h - r->y() - 1,
                             w - r->right() - 1, h - r->bottom() - 1);
                trgn |= tr.normalized();
            }
            break;
        default:
            break;
    }

    return trgn;
}

/*!
    \table
    \header \i Transformation \i Rotation \i Mirrored
    \row \i 0 \i 0         \i No
    \row \i 1 \i 90        \i No
    \row \i 2 \i 180       \i No
    \row \i 3 \i 270       \i No
    \row \i 4 \i 0         \i Yes
    \row \i 5 \i 90        \i Yes
    \row \i 6 \i 180       \i Yes
    \row \i 7 \i 270       \i Yes
    \endtable
*/
int QTransformedScreen::transformOrientation() const
{
    return (int)trans;
}

//===========================================================================

#ifndef QT_NO_QWS_CURSOR

class QTransformedScreenCursor : public QT_TRANS_CURSOR_BASE
{
public:
    QTransformedScreenCursor() : QT_TRANS_CURSOR_BASE() {}
    virtual void init(SWCursorData *da, bool init = false);
    virtual void set(const QImage &image, int hotx, int hoty);
};

void QTransformedScreenCursor::init(SWCursorData *da, bool init)
{
    QT_TRANS_CURSOR_BASE::init(da, init);
}

void QTransformedScreenCursor::set(const QImage &image, int hotx, int hoty)
{
    QImage rimg = qt_trans_screen->mapToDevice(image);
    QPoint tp = qt_trans_screen->mapToDevice(QPoint(hotx, hoty), image.size());
    QT_TRANS_CURSOR_BASE::set(rimg, tp.x(), tp.y());
}

#endif

//===========================================================================

template <const int depth, const int type>
class QGfxTransformedRaster : public QT_TRANS_GFX_BASE<depth,type>
{
public:
    QGfxTransformedRaster(unsigned char *,int w,int h);
    virtual ~QGfxTransformedRaster();

    virtual void setSource(const QImage * i);
    virtual void drawPoint(int,int);
    virtual void drawPoints(const QPolygon &,int,int);
    virtual void drawLine(int,int,int,int);
    virtual void fillRect(int,int,int,int);
    virtual void drawPolygon(const QPolygon &,bool,int,int);
    virtual void drawPolyline(const QPolygon &,int,int);
    virtual void blt(int,int,int,int,int,int);
    virtual void stretchBlt(int rx,int ry,int w,int h, int sw,int sh);
    virtual void tiledBlt(int rx,int ry,int w,int h);

    inline int tx(int x, int y) {
        switch (qt_trans_screen->transformation()) {
            case QTransformedScreen::Rot90:
                return y - this->xoffs + this->yoffs;
            case QTransformedScreen::Rot180:
                return (this->width - x - 1) - this->xoffs - this->xoffs;
            case QTransformedScreen::Rot270:
                return (this->height - y - 1) - this->xoffs - this->yoffs;
            default:
                return x;
        }
    }
    inline int ty(int x, int y) {
        switch (qt_trans_screen->transformation()) {
            case QTransformedScreen::Rot90:
                return (this->width - x - 1) - this->yoffs - this->xoffs;
            case QTransformedScreen::Rot180:
                return (this->height - y - 1) - this->yoffs - this->yoffs;
            case QTransformedScreen::Rot270:
                return x - this->yoffs + this->xoffs;
            default:
                return y;
        }
    }

protected:
    virtual void setSourceWidgetOffset(int x, int y);
    void processSpans(int n, QPoint* point, int* width);


    bool inDraw;
};

template <const int depth, const int type>
QGfxTransformedRaster<depth,type>::QGfxTransformedRaster(unsigned char *b,int w,int h)
: QT_TRANS_GFX_BASE<depth,type>(b, w, h), inDraw(false)
{
}

template <const int depth, const int type>
QGfxTransformedRaster<depth,type>::~QGfxTransformedRaster()
{
}

template <const int depth, const int type>
void QGfxTransformedRaster<depth,type>::setSourceWidgetOffset(int x, int y)
{
    if (this->srcbits == this->buffer) {
        switch (qt_trans_screen->transformation()) {
            case QTransformedScreen::Rot90:
                this->srcwidgetoffs = QPoint(y, this->width - x - this->srcwidth);
                break;
            case QTransformedScreen::Rot180:
                this->srcwidgetoffs = QPoint(this->width - x - this->srcwidth, this->height - y - this->srcheight);
                break;
            case QTransformedScreen::Rot270:
                this->srcwidgetoffs = QPoint(this->height - y - this->srcheight, x);
                break;
            default:
                this->srcwidgetoffs = QPoint(x, y);
                break;
        }
    } else
        this->srcwidgetoffs = QPoint(x, y);
}

template <const int depth, const int type>
void QGfxTransformedRaster<depth,type>::setSource(const QImage * i)
{
    QT_TRANS_GFX_BASE<depth,type>::setSource(i);
    QSize s = qt_screen->mapToDevice(QSize(i->width(), i->height()));
    this->srcwidth = s.width();
    this->srcheight = s.height();
}

template <const int depth, const int type>
void QGfxTransformedRaster<depth,type>::drawPoint(int x, int y)
{
    QT_TRANS_GFX_BASE<depth,type>::drawPoint(tx(x,y), ty(x,y));
}

template <const int depth, const int type>
void QGfxTransformedRaster<depth,type>::drawPoints(const QPolygon &a, int idx, int num)
{
    QPolygon na(num);

    for (int i = 0; i < num; i++) {
        int x, y;
        a.point(i+idx, &x, &y);
        na.setPoint(i, tx(x,y), ty(x,y));
    }

    QT_TRANS_GFX_BASE<depth,type>::drawPoints(na, 0, num);
}

template <const int depth, const int type>
void QGfxTransformedRaster<depth,type>::drawLine(int x1, int y1, int x2, int y2)
{
    if (inDraw) {
        QT_TRANS_GFX_BASE<depth,type>::drawLine(x1, y1, x2, y2);
    } else {
        inDraw = true;
        QT_TRANS_GFX_BASE<depth,type>::drawLine(tx(x1,y1), ty(x1,y1),
                                          tx(x2,y2), ty(x2,y2));
        inDraw = false;
    }
}

template <const int depth, const int type>
void QGfxTransformedRaster<depth,type>::fillRect(int x, int y, int w, int h)
{
    if (w == 0 || h == 0)
        return;
    QRect r(x, y, w, h);
    if (this->cbrush.style() == Qt::SolidPattern) {
        r.setCoords(tx(x,y), ty(x,y), tx(x+w-1,y+h-1), ty(x+w-1,y+h-1));
        r = r.normalized();
    }
    QT_TRANS_GFX_BASE<depth,type>::fillRect(r.x(), r.y(), r.width(), r.height());
}

template <const int depth, const int type>
void QGfxTransformedRaster<depth,type>::drawPolygon(const QPolygon &a, bool w, int idx, int num)
{
    // Because of stitchedges, we cannot transform first. However, if
    // we draw an outline, edges do not matter and we can do a fastpath
    // solution. The brush offset logic is complicated enough, so we don't
    // fastpath patternedbrush.

    if (inDraw  || this->cpen.style()==Qt::NoPen || this->patternedbrush) {
        //slowpath
        QT_TRANS_GFX_BASE<depth,type>::drawPolygon(a, w, idx, num);
    } else {
        inDraw = true;
        QPolygon na(num);

        for (int i = 0; i < num; i++) {
            int x, y;
            a.point(i+idx, &x, &y);
            na.setPoint(i, tx(x,y), ty(x,y));
        }

        QT_TRANS_GFX_BASE<depth,type>::drawPolygon(na, w, 0, num);
        inDraw = false;
    }
}


template <const int depth, const int type>
void QGfxTransformedRaster<depth,type>::processSpans(int n, QPoint* point, int* width)
{
    if (inDraw || this->patternedbrush && this->srcwidth != 0 && this->srcheight != 0) {
        //in the patternedbrush case, we let blt do the transformation
        // so we leave inDraw false.
        QT_TRANS_GFX_BASE<depth,type>::processSpans(n, point, width);
    } else {
        inDraw = true;
        while (n--) {
            if (*width > 0) {
                int x=tx(point->x(),point->y())+this->xoffs;
                int y=ty(point->x(),point->y())+this->yoffs;

                switch(qt_trans_screen->transformation()) {
                case QTransformedScreen::Rot90:
                    this->vline(x, y-(*width-1), y);
                    break;
                case QTransformedScreen::Rot180:
                    this->hline(x - (*width-1), x, y);
                    break;
                case QTransformedScreen::Rot270:
                    this->vline(x, y, y+*width-1);
                    break;
                default:
                    this->hline(x, x+*width-1, y);
                    break;
                }
            }
            point++;
            width++;
        }
        inDraw = false;
    }

}






template <const int depth, const int type>
void QGfxTransformedRaster<depth,type>::drawPolyline(const QPolygon &a, int idx, int num)
{
    if (inDraw) {
        QT_TRANS_GFX_BASE<depth,type>::drawPolyline(a, idx, num);
    } else {
        inDraw = true;
        QPolygon na(num);

        for (int i = 0; i < num; i++) {
            int x, y;
            a.point(i+idx, &x, &y);
            na.setPoint(i, tx(x,y), ty(x,y));
        }

        QT_TRANS_GFX_BASE<depth,type>::drawPolyline(na, 0, num);
        inDraw = false;
    }
}

template <const int depth, const int type>
void QGfxTransformedRaster<depth,type>::blt(int x, int y, int w, int h, int sx, int sy)
{
    if (w == 0 || h == 0)
        return;
    QRect r;
    int rsx;
    int rsy;
    if (inDraw) {
        r = QRect(x, y, w, h);
        rsx = sx;
        rsy = sy;
    } else {
        r.setCoords(tx(x,y), ty(x,y), tx(x+w-1,y+h-1), ty(x+w-1,y+h-1));
        r = r.normalized();
        switch (qt_trans_screen->transformation()) {
        case QTransformedScreen::Rot90:
            rsx = sy;
            rsy = this->srcwidth - sx - w;
            break;
        case QTransformedScreen::Rot180:
            rsx = this->srcwidth - sx - w;
            rsy = this->srcheight - sy - h;
            break;
        case QTransformedScreen::Rot270:
            rsx = this->srcheight - sy - h;
            rsy = sx;
            break;
        default:
            rsx = sx;
            rsy = sy;
            break;
        }
    }
    QT_TRANS_GFX_BASE<depth,type>::blt(r.x(), r.y(), r.width(), r.height(), rsx, rsy);
}

template <const int depth, const int type>
void QGfxTransformedRaster<depth,type>::stretchBlt(int x, int y, int w, int h,
                                                    int sw, int sh)
{
    if (w == 0 || h == 0)
        return;
    QRect r;
    r.setCoords(tx(x,y), ty(x,y), tx(x+w-1,y+h-1), ty(x+w-1,y+h-1));
    r = r.normalized();
    QSize rs = qt_trans_screen->mapToDevice(QSize(sw, sh));
    QT_TRANS_GFX_BASE<depth,type>::stretchBlt(r.x(), r.y(), r.width(), r.height(),
                                        rs.width(), rs.height());
}

template <const int depth, const int type>
void QGfxTransformedRaster<depth,type>::tiledBlt(int rx,int ry,int w,int h)
{
    if (w <= 0 || h <= 0)
        return;
    inDraw = true;
    QRect r;
    r.setCoords(tx(rx,ry), ty(rx,ry), tx(rx+w-1,ry+h-1), ty(rx+w-1,ry+h-1));
    r = r.normalized();

    QPoint oldBrushOrig = this->brushorig;
    this->brushorig = qt_screen->mapToDevice(this->brushorig, QSize(qt_screen->width(),qt_screen->height()));
    int oldsw = this->srcwidth;
    int oldsh = this->srcheight;
    QSize s = qt_screen->mapToDevice(QSize(this->srcwidth,this->srcheight));
    this->srcwidth = s.width();
    this->srcheight = s.height();

    QT_TRANS_GFX_BASE<depth,type>::tiledBlt(r.x(), r.y(), r.width(), r.height());

    this->srcwidth = oldsw;
    this->srcheight = oldsh;
    this->brushorig = oldBrushOrig;
    inDraw = false;
}

int QTransformedScreen::initCursor(void* e, bool init)
{
#ifndef QT_NO_QWS_CURSOR
    qt_sw_cursor=true;
    // ### until QLumpManager works Ok with multiple connected clients,
    // we steal a chunk of shared memory
    SWCursorData *data = (SWCursorData *)e - 1;
    qt_screencursor=new QTransformedScreenCursor();
    qt_screencursor->init(data, init);
    return sizeof(SWCursorData);
#else
    return 0;
#endif
}

QGfx *QTransformedScreen::createGfx(unsigned char * bytes,int w,int h,int d, int linestep)
{
    QGfx* ret = 0;
    if(d==1) {
        ret = new QGfxTransformedRaster<1,0>(bytes,w,h);
#ifndef QT_NO_QWS_DEPTH_16
    } else if(d==16) {
        ret = new QGfxTransformedRaster<16,0>(bytes,w,h);
#endif
#ifndef QT_NO_QWS_DEPTH_8
    } else if (d==8) {
        ret = new QGfxTransformedRaster<8,0>(bytes,w,h);
#endif
#ifndef QT_NO_QWS_DEPTH_4
    } else if (d==4) {
        ret = new QGfxTransformedRaster<4,0>(bytes,w,h);
#endif
#ifndef QT_NO_QWS_DEPTH_24
    } else if (d==24) {
        ret = new QGfxTransformedRaster<24,0>(bytes,w,h);
#endif
#ifndef QT_NO_QWS_DEPTH_32
    } else if (d==32) {
        ret = new QGfxTransformedRaster<32,0>(bytes,w,h);
#endif
    } else {
        qFatal("Can't drive depth %d",d);
    }
    ret->setLineStep(linestep);
    return ret;
}


#endif // QT_NO_QWS_TRANSFORMED
