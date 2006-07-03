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

#ifndef QSCREEN_QWS_H
#define QSCREEN_QWS_H

#include <QtCore/qnamespace.h>
#include <QtCore/qpoint.h>
#include <QtGui/qrgb.h>
#include <QtCore/qrect.h>
#include <QtGui/qimage.h>
#include <QtGui/qregion.h>

QT_BEGIN_HEADER

QT_MODULE(Gui)

class QScreenCursor;
class QBrush;
class QWSWindow;
class QWSWindowSurface;

#ifdef QT_QWS_DEPTH_16
# ifndef QT_QWS_DEPTH16_RGB
#  define QT_QWS_DEPTH16_RGB 565
# endif
static const int qt_rbits = (QT_QWS_DEPTH16_RGB/100);
static const int qt_gbits = (QT_QWS_DEPTH16_RGB/10%10);
static const int qt_bbits = (QT_QWS_DEPTH16_RGB%10);
static const int qt_red_shift = qt_bbits+qt_gbits-(8-qt_rbits);
static const int qt_green_shift = qt_bbits-(8-qt_gbits);
static const int qt_neg_blue_shift = 8-qt_bbits;
static const int qt_blue_mask = (1<<qt_bbits)-1;
static const int qt_green_mask = (1<<(qt_gbits+qt_bbits))-(1<<qt_bbits);
static const int qt_red_mask = (1<<(qt_rbits+qt_gbits+qt_bbits))-(1<<(qt_gbits+qt_bbits));

static const int qt_red_rounding_shift = qt_red_shift + qt_rbits;
static const int qt_green_rounding_shift = qt_green_shift + qt_gbits;
static const int qt_blue_rounding_shift = qt_bbits - qt_neg_blue_shift;


inline ushort qt_convRgbTo16(const int r, const int g, const int b)
{
    const int tr = r << qt_red_shift;
    const int tg = g << qt_green_shift;
    const int tb = b >> qt_neg_blue_shift;

    return (tb & qt_blue_mask) | (tg & qt_green_mask) | (tr & qt_red_mask);
}

inline ushort qt_convRgbTo16(QRgb c)
{
    const int tr = qRed(c) << qt_red_shift;
    const int tg = qGreen(c) << qt_green_shift;
    const int tb = qBlue(c) >> qt_neg_blue_shift;

    return (tb & qt_blue_mask) | (tg & qt_green_mask) | (tr & qt_red_mask);
}

inline QRgb qt_conv16ToRgb(ushort c)
{
    const int r=(c & qt_red_mask);
    const int g=(c & qt_green_mask);
    const int b=(c & qt_blue_mask);
    const int tr = r >> qt_red_shift | r >> qt_red_rounding_shift;
    const int tg = g >> qt_green_shift | g >> qt_green_rounding_shift;
    const int tb = b << qt_neg_blue_shift | b >> qt_blue_rounding_shift;

    return qRgb(tr,tg,tb);
}

inline void qt_conv16ToRgb(ushort c, int& r, int& g, int& b)
{
    const int tr=(c & qt_red_mask);
    const int tg=(c & qt_green_mask);
    const int tb=(c & qt_blue_mask);
    r = tr >> qt_red_shift | tr >> qt_red_rounding_shift;
    g = tg >> qt_green_shift | tg >> qt_green_rounding_shift;
    b = tb << qt_neg_blue_shift | tb >> qt_blue_rounding_shift;
}
#endif // QT_QWS_DEPTH_16


const int SourceSolid=0;
const int SourcePixmap=1;

#ifndef QT_NO_QWS_CURSOR

class QScreenCursor;
extern QScreenCursor *qt_screencursor;
extern bool qt_sw_cursor;

class Q_GUI_EXPORT QScreenCursor
{
public:
    QScreenCursor();
    virtual ~QScreenCursor();

    virtual void set(const QImage &image, int hotx, int hoty);
    virtual void move(int x, int y);
    virtual void show();
    virtual void hide();

    bool supportsAlphaCursor() const { return supportsAlpha; }

    static bool enabled() { return qt_sw_cursor; }

    QRect boundingRect() const { return QRect(pos - hotspot, size); }
    QImage image() const { return cursor; }
    bool isVisible() const { return enable; }
    bool isAccelerated() const { return hwaccel; }

    static void initSoftwareCursor();
    static QScreenCursor* instance() { return qt_screencursor; }

protected:
    QImage cursor;

    QSize size;
    QPoint pos;
    QPoint hotspot;
    uint enable : 1;
    uint hwaccel : 1;
    uint supportsAlpha : 1;
};

#endif // QT_NO_QWS_CURSOR

struct fb_cmap;

// A (used) chunk of offscreen memory

class QPoolEntry
{
public:
    unsigned int start;
    unsigned int end;
    int clientId;
};

class QScreen;
extern QScreen *qt_screen;
typedef void(*ClearCacheFunc)(QScreen *obj, int);

class Q_GUI_EXPORT QScreen {

public:

    explicit QScreen(int display_id);
    virtual ~QScreen();
    static QScreen* instance() { return qt_screen; }
    virtual bool initDevice() = 0;
    virtual bool connect(const QString &displaySpec) = 0;
    virtual void disconnect() = 0;
    virtual void shutdownDevice();
    virtual void setMode(int,int,int) = 0;
    virtual bool supportsDepth(int) const;

    virtual void save();
    virtual void restore();
    virtual void blank(bool on);

    virtual int pixmapOffsetAlignment() { return 64; }
    virtual int pixmapLinestepAlignment() { return 64; }
    virtual int sharedRamSize(void *) { return 0; }

    virtual bool onCard(const unsigned char *) const;
    virtual bool onCard(const unsigned char *, ulong& out_offset) const;

    enum PixelType { NormalPixel, BGRPixel };

    // sets a single color in the colormap
    virtual void set(unsigned int,unsigned int,unsigned int,unsigned int);
    // allocates a color
    virtual int alloc(unsigned int,unsigned int,unsigned int);

    int width() const { return w; }
    int height() const { return h; }
    int depth() const { return d; }
    virtual int pixmapDepth() const;
    PixelType pixelType() const { return pixeltype; }
    int linestep() const { return lstep; }
    int deviceWidth() const { return dw; }
    int deviceHeight() const { return dh; }
    uchar * base() const { return data; }
    // Ask for memory from card cache with alignment
    virtual uchar * cache(int) { return 0; }
    virtual void uncache(uchar *) {}

    int screenSize() const { return size; }
    int totalSize() const { return mapsize; }

    QRgb * clut() { return screenclut; }
    int numCols() { return screencols; }

    virtual QSize mapToDevice(const QSize &) const;
    virtual QSize mapFromDevice(const QSize &) const;
    virtual QPoint mapToDevice(const QPoint &, const QSize &) const;
    virtual QPoint mapFromDevice(const QPoint &, const QSize &) const;
    virtual QRect mapToDevice(const QRect &, const QSize &) const;
    virtual QRect mapFromDevice(const QRect &, const QSize &) const;
    virtual QImage mapToDevice(const QImage &) const;
    virtual QImage mapFromDevice(const QImage &) const;
    virtual QRegion mapToDevice(const QRegion &, const QSize &) const;
    virtual QRegion mapFromDevice(const QRegion &, const QSize &) const;
    virtual int transformOrientation() const;
    virtual bool isTransformed() const;
    virtual bool isInterlaced() const;

    virtual void setDirty(const QRect&);

    virtual int memoryNeeded(const QString&);

    int * opType() { return screen_optype; }
    int * lastOp() { return screen_lastop; }

    virtual void haltUpdates();
    virtual void resumeUpdates();


    // composition manager methods
    virtual void exposeRegion(QRegion r, int changing);

    // these work directly on the screen
    virtual void blit(const QImage &img, const QPoint &topLeft, const QRegion &region);
    virtual void solidFill(const QColor &color, const QRegion &region);
    void blit(QWSWindow *bs, const QRegion &clip);

    virtual QWSWindowSurface* createSurface(QWidget *widget) const;
    virtual QWSWindowSurface* createSurface(const QString &key) const;

protected:

    // Only used without QT_NO_QWS_REPEATER, but included so that
    // it's binary compatible regardless.
    int * screen_optype;
    int * screen_lastop;

    QRgb screenclut[256];
    int screencols;

    bool initted;

    uchar * data;

    // Table of allocated lumps, kept in sorted highest-to-lowest order
    // The table itself is allocated at the bottom of offscreen memory
    // i.e. it's similar to having a stack (the table) and a heap
    // (the allocated blocks). Freed space is implicitly described
    // by the gaps between the allocated lumps (this saves entries and
    // means we don't need to worry about coalescing freed lumps)

    QPoolEntry * entries;
    int * entryp;
    unsigned int * lowest;

    int w;
    int lstep;
    int h;
    int d;
    PixelType pixeltype;
    bool grayscale;

    int dw;
    int dh;

    int hotx;
    int hoty;
    QImage cursor;

    int size;               // Screen size
    int mapsize;       // Total mapped memory

    int displayId;

    friend class QWSServer;
    friend class QWSServerPrivate;
    static ClearCacheFunc clearCacheFunc;

private:
    void compose(int level, const QRegion &exposed, QRegion &blend, QImage &blendbuffer, int changing_level);
    void paintBackground(const QRegion &);
};

// This lives in loadable modules

#ifndef QT_LOADABLE_MODULES
extern "C" QScreen * qt_get_screen(int display_id, const char* spec);
#endif

// This is in main lib, loads the right module, calls qt_get_screen
// In non-loadable cases just aliases to qt_get_screen

const unsigned char * qt_probe_bus();

QT_END_HEADER

#endif // QSCREEN_QWS_H
