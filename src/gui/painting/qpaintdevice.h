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

#ifndef QPAINTDEVICE_H
#define QPAINTDEVICE_H

#include <QtGui/qwindowdefs.h>
#include <QtCore/qrect.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#if defined(Q_WS_QWS)
class QWSDisplay;
#endif

class QPaintEngine;

class Q_GUI_EXPORT QPaintDevice                                // device for QPainter
{
public:
    enum PaintDeviceMetric {
        PdmWidth = 1,
        PdmHeight,
        PdmWidthMM,
        PdmHeightMM,
        PdmNumColors,
        PdmDepth,
        PdmDpiX,
        PdmDpiY,
        PdmPhysicalDpiX,
        PdmPhysicalDpiY
    };

    virtual ~QPaintDevice();

    virtual int devType() const;
    bool paintingActive() const;
    virtual QPaintEngine *paintEngine() const = 0;

#if defined(Q_WS_QWS)
    static QWSDisplay *qwsDisplay();
#endif

#ifdef Q_WS_WIN
    virtual HDC getDC() const;
    virtual void releaseDC(HDC hdc) const;
#endif

    int width() const { return metric(PdmWidth); }
    int height() const { return metric(PdmHeight); }
    int widthMM() const { return metric(PdmWidthMM); }
    int heightMM() const { return metric(PdmHeightMM); }
    int logicalDpiX() const { return metric(PdmDpiX); }
    int logicalDpiY() const { return metric(PdmDpiY); }
    int physicalDpiX() const { return metric(PdmPhysicalDpiX); }
    int physicalDpiY() const { return metric(PdmPhysicalDpiY); }
    int numColors() const { return metric(PdmNumColors); }
    int depth() const { return metric(PdmDepth); }

protected:
    QPaintDevice();
    virtual int metric(PaintDeviceMetric metric) const;

    ushort        painters;                        // refcount

private:
    Q_DISABLE_COPY(QPaintDevice)

#if defined(Q_WS_X11) && defined(QT3_SUPPORT)
public:
    QT3_SUPPORT Display *x11Display() const;
    QT3_SUPPORT int x11Screen() const;
    QT3_SUPPORT int x11Depth() const;
    QT3_SUPPORT int x11Cells() const;
    QT3_SUPPORT Qt::HANDLE x11Colormap() const;
    QT3_SUPPORT bool x11DefaultColormap() const;
    QT3_SUPPORT void *x11Visual() const;
    QT3_SUPPORT bool x11DefaultVisual() const;

    static QT3_SUPPORT Display *x11AppDisplay();
    static QT3_SUPPORT int x11AppScreen();
    static QT3_SUPPORT int x11AppDepth(int screen = -1);
    static QT3_SUPPORT int x11AppCells(int screen = -1);
    static QT3_SUPPORT Qt::HANDLE x11AppRootWindow(int screen = -1);
    static QT3_SUPPORT Qt::HANDLE x11AppColormap(int screen = -1);
    static QT3_SUPPORT void *x11AppVisual(int screen = -1);
    static QT3_SUPPORT bool x11AppDefaultColormap(int screen =-1);
    static QT3_SUPPORT bool x11AppDefaultVisual(int screen =-1);
    static QT3_SUPPORT int x11AppDpiX(int screen = -1);
    static QT3_SUPPORT int x11AppDpiY(int screen = -1);
    static QT3_SUPPORT void x11SetAppDpiX(int, int);
    static QT3_SUPPORT void x11SetAppDpiY(int, int);
#endif

    friend class QPainter;
    friend class QFontEngineMac;
    friend class QX11PaintEngine;
};

#ifdef QT3_SUPPORT
QT3_SUPPORT Q_GUI_EXPORT
void bitBlt(QPaintDevice *dst, int dx, int dy,
             const QPaintDevice *src, int sx=0, int sy=0, int sw=-1, int sh=-1,
             bool ignoreMask=false);

QT3_SUPPORT Q_GUI_EXPORT
void bitBlt(QPaintDevice *dst, int dx, int dy,
             const QImage *src, int sx=0, int sy=0, int sw=-1, int sh=-1,
             int conversion_flags=0);

QT3_SUPPORT Q_GUI_EXPORT
void bitBlt(QPaintDevice *dst, const QPoint &dp,
            const QPaintDevice *src, const QRect &sr=QRect(0,0,-1,-1),
            bool ignoreMask=false);
#endif

/*****************************************************************************
  Inline functions
 *****************************************************************************/

inline int QPaintDevice::devType() const
{ return QInternal::UnknownDevice; }

inline bool QPaintDevice::paintingActive() const
{ return painters != 0; }

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPAINTDEVICE_H
