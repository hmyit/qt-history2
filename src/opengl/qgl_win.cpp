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


#include <qgl.h>
#include <qlist.h>
#include <qmap.h>
#include <qpixmap.h>
#include <qevent.h>
#include <private/qgl_p.h>
#include <qcolormap.h>
#include <qvarlengtharray.h>
#include <qdebug.h>

#include <windows.h>

typedef bool (APIENTRY *PFNWGLGETPIXELFORMATATTRIBIVARB)(HDC hdc,
							 int iPixelFormat,
							 int iLayerPlane,
							 uint nAttributes,
							 const int *piAttributes,
							 int *piValues);
typedef bool (APIENTRY *PFNWGLCHOOSEPIXELFORMATARB)(HDC hdc,
						    const int *piAttribList,
						    const float *pfAttribFList,
						    uint nMaxFormats,
						    int *piFormats,
						    UINT *nNumFormats);
static PFNWGLCHOOSEPIXELFORMATARB wglChoosePixelFormatARB = 0;
static PFNWGLGETPIXELFORMATATTRIBIVARB wglGetPixelFormatAttribivARB = 0;

#ifndef WGL_ARB_multisample
#define WGL_SAMPLE_BUFFERS_ARB		     0x2041
#define WGL_SAMPLES_ARB			     0x2042
#endif

#ifndef WGL_ARB_pixel_format
#define WGL_NUMBER_PIXEL_FORMATS_ARB   0x2000
#define WGL_DRAW_TO_WINDOW_ARB         0x2001
#define WGL_DRAW_TO_BITMAP_ARB         0x2002
#define WGL_ACCELERATION_ARB           0x2003
#define WGL_NEED_PALETTE_ARB           0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB    0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB     0x2006
#define WGL_SWAP_METHOD_ARB            0x2007
#define WGL_NUMBER_OVERLAYS_ARB        0x2008
#define WGL_NUMBER_UNDERLAYS_ARB       0x2009
#define WGL_TRANSPARENT_ARB            0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB  0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB 0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB 0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB 0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB 0x203B
#define WGL_SHARE_DEPTH_ARB            0x200C
#define WGL_SHARE_STENCIL_ARB          0x200D
#define WGL_SHARE_ACCUM_ARB            0x200E
#define WGL_SUPPORT_GDI_ARB            0x200F
#define WGL_SUPPORT_OPENGL_ARB         0x2010
#define WGL_DOUBLE_BUFFER_ARB          0x2011
#define WGL_STEREO_ARB                 0x2012
#define WGL_PIXEL_TYPE_ARB             0x2013
#define WGL_COLOR_BITS_ARB             0x2014
#define WGL_RED_BITS_ARB               0x2015
#define WGL_RED_SHIFT_ARB              0x2016
#define WGL_GREEN_BITS_ARB             0x2017
#define WGL_GREEN_SHIFT_ARB            0x2018
#define WGL_BLUE_BITS_ARB              0x2019
#define WGL_BLUE_SHIFT_ARB             0x201A
#define WGL_ALPHA_BITS_ARB             0x201B
#define WGL_ALPHA_SHIFT_ARB            0x201C
#define WGL_ACCUM_BITS_ARB             0x201D
#define WGL_ACCUM_RED_BITS_ARB         0x201E
#define WGL_ACCUM_GREEN_BITS_ARB       0x201F
#define WGL_ACCUM_BLUE_BITS_ARB        0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB       0x2021
#define WGL_DEPTH_BITS_ARB             0x2022
#define WGL_STENCIL_BITS_ARB           0x2023
#define WGL_AUX_BUFFERS_ARB            0x2024
#define WGL_NO_ACCELERATION_ARB        0x2025
#define WGL_GENERIC_ACCELERATION_ARB   0x2026
#define WGL_FULL_ACCELERATION_ARB      0x2027
#define WGL_SWAP_EXCHANGE_ARB          0x2028
#define WGL_SWAP_COPY_ARB              0x2029
#define WGL_SWAP_UNDEFINED_ARB         0x202A
#define WGL_TYPE_RGBA_ARB              0x202B
#define WGL_TYPE_COLORINDEX_ARB        0x202C
#endif

class QGLCmapPrivate
{
public:
    QGLCmapPrivate() : count(1) { }
    void ref()                { ++count; }
    bool deref()        { return !--count; }
    uint count;

    enum AllocState{ UnAllocated = 0, Allocated = 0x01, Reserved = 0x02 };

    int maxSize;
    QVector<uint> colorArray;
    QVector<quint8> allocArray;
    QVector<quint8> contextArray;
    QMap<uint,int> colorMap;
};

/*****************************************************************************
  QColorMap class - temporarily here, until it is ready for prime time
 *****************************************************************************/

/****************************************************************************
**
** Definition of QColorMap class
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
** information about the Professional Edition licensing, or see
** http://www.trolltech.com/qpl/ for QPL licensing information.
**
*****************************************************************************/

#ifndef QGLCMAP_H
#define QGLCMAP_H

#include <qcolor.h>

class QGLCmapPrivate;

class /*Q_EXPORT*/ QGLCmap
{
public:
    enum Flags { Reserved = 0x01 };

    QGLCmap(int maxSize = 256);
    QGLCmap(const QGLCmap& map);
    ~QGLCmap();

    QGLCmap& operator=(const QGLCmap& map);

    // isEmpty and/or isNull ?
    int size() const;
    int maxSize() const;

    void resize(int newSize);

    int find(QRgb color) const;
    int findNearest(QRgb color) const;
    int allocate(QRgb color, uint flags = 0, quint8 context = 0);

    void setEntry(int idx, QRgb color, uint flags = 0, quint8 context = 0);

    const QRgb* colors() const;

private:
    void detach();
    QGLCmapPrivate* d;
};

#endif


QGLCmap::QGLCmap(int maxSize) // add a bool prealloc?
{
    d = new QGLCmapPrivate;
    d->maxSize = maxSize;
}


QGLCmap::QGLCmap(const QGLCmap& map)
{
    d = map.d;
    d->ref();
}


QGLCmap::~QGLCmap()
{
    if (d && d->deref())
        delete d;
    d = 0;
}


QGLCmap& QGLCmap::operator=(const QGLCmap& map)
{
    map.d->ref();
    if (d->deref())
        delete d;
    d = map.d;
    return *this;
}


int QGLCmap::size() const
{
    return d->colorArray.size();
}


int QGLCmap::maxSize() const
{
    return d->maxSize;
}


void QGLCmap::detach()
{
    if (d->count != 1) {
        d->deref();
        QGLCmapPrivate* newd = new QGLCmapPrivate;
        newd->maxSize = d->maxSize;
        newd->colorArray = d->colorArray;
        newd->allocArray = d->allocArray;
        newd->contextArray = d->contextArray;
        newd->colorArray.detach();
        newd->allocArray.detach();
        newd->contextArray.detach();
        newd->colorMap = d->colorMap;
        d = newd;
    }
}


void QGLCmap::resize(int newSize)
{
    if (newSize < 0 || newSize > d->maxSize) {
        qWarning("QGLCmap::resize(): size out of range");
        return;
    }
    int oldSize = size();
    detach();
    //if shrinking; remove the lost elems from colorMap
    d->colorArray.resize(newSize);
    d->allocArray.resize(newSize);
    d->contextArray.resize(newSize);
    if (newSize > oldSize) {
        memset(d->allocArray.data() + oldSize, 0, newSize - oldSize);
        memset(d->contextArray.data() + oldSize, 0, newSize - oldSize);
    }
}


int QGLCmap::find(QRgb color) const
{
    QMap<uint,int>::ConstIterator it = d->colorMap.find(color);
    if (it != d->colorMap.end())
        return *it;
    return -1;
}


int QGLCmap::findNearest(QRgb color) const
{
    int idx = find(color);
    if (idx >= 0)
        return idx;
    int mapSize = size();
    int mindist = 200000;
    int r = qRed(color);
    int g = qGreen(color);
    int b = qBlue(color);
    int rx, gx, bx, dist;
    for (int i=0; i < mapSize; i++) {
        if (!(d->allocArray[i] & QGLCmapPrivate::Allocated))
            continue;
        QRgb ci = d->colorArray[i];
        rx = r - qRed(ci);
        gx = g - qGreen(ci);
        bx = b - qBlue(ci);
        dist = rx*rx + gx*gx + bx*bx;                // calculate distance
        if (dist < mindist) {                        // minimal?
            mindist = dist;
            idx = i;
        }
    }
    return idx;
}




// Does not always allocate; returns existing c idx if found

int QGLCmap::allocate(QRgb color, uint flags, quint8 context)
{
    int idx = find(color);
    if (idx >= 0)
        return idx;

    int mapSize = d->colorArray.size();
    int newIdx = d->allocArray.indexOf(QGLCmapPrivate::UnAllocated);

    if (newIdx < 0) {                        // Must allocate more room
        if (mapSize < d->maxSize) {
            newIdx = mapSize;
            mapSize++;
            resize(mapSize);
        }
        else {
            //# add a bool param that says what to do in case no more room -
            // fail (-1) or return nearest?
            return -1;
        }
    }

    d->colorArray[newIdx] = color;
    if (flags & QGLCmap::Reserved) {
        d->allocArray[newIdx] = QGLCmapPrivate::Reserved;
    }
    else {
        d->allocArray[newIdx] = QGLCmapPrivate::Allocated;
        d->colorMap.insert(color, newIdx);
    }
    d->contextArray[newIdx] = context;
    return newIdx;
}


void QGLCmap::setEntry(int idx, QRgb color, uint flags, quint8 context)
{
    if (idx < 0 || idx >= d->maxSize) {
        qWarning("QGLCmap::set(): Index out of range");
        return;
    }
    detach();
    int mapSize = size();
    if (idx >= mapSize) {
        mapSize = idx + 1;
        resize(mapSize);
    }
    d->colorArray[idx] = color;
    if (flags & QGLCmap::Reserved) {
        d->allocArray[idx] = QGLCmapPrivate::Reserved;
    }
    else {
        d->allocArray[idx] = QGLCmapPrivate::Allocated;
        d->colorMap.insert(color, idx);
    }
    d->contextArray[idx] = context;
}


const QRgb* QGLCmap::colors() const
{
    return d->colorArray.data();
}



/*****************************************************************************
  QGLFormat Win32/WGL-specific code
 *****************************************************************************/


void qwglError(const char* method, const char* func)
{
#ifndef QT_NO_DEBUG
    char* lpMsgBuf;
    FormatMessageA(
                  FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  0, GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (char*) &lpMsgBuf, 0, 0);
    qWarning("%s : %s failed: %s", method, func, lpMsgBuf);
    LocalFree(lpMsgBuf);
#else
    Q_UNUSED(method);
    Q_UNUSED(func);
#endif
}



bool QGLFormat::hasOpenGL()
{
    return true;
}

static bool opengl32dll = false;

bool QGLFormat::hasOpenGLOverlays()
{
    // workaround for matrox driver:
    // make a cheap call to opengl to force loading of DLL
    if (!opengl32dll) {
        GLint params;
        glGetIntegerv(GL_DEPTH_BITS, &params);
        opengl32dll = true;
    }

    static bool checkDone = false;
    static bool hasOl = false;

    if (!checkDone) {
        checkDone = true;
        HDC dc = qt_win_display_dc();
        int pfiMax = DescribePixelFormat(dc, 0, 0, NULL);
        PIXELFORMATDESCRIPTOR pfd;
        for (int pfi = 1; pfi <= pfiMax; pfi++) {
            DescribePixelFormat(dc, pfi, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
            if ((pfd.bReserved & 0x0f) && (pfd.dwFlags & PFD_SUPPORT_OPENGL)) {
                // This format has overlays/underlays
                LAYERPLANEDESCRIPTOR lpd;
                wglDescribeLayerPlane(dc, pfi, 1,
                                       sizeof(LAYERPLANEDESCRIPTOR), &lpd);
                if (lpd.dwFlags & LPD_SUPPORT_OPENGL) {
                    hasOl = true;
                    break;
                }
            }
        }
    }
    return hasOl;
}


/*****************************************************************************
  QGLContext Win32/WGL-specific code
 *****************************************************************************/

static uchar qgl_rgb_palette_comp(int idx, uint nbits, uint shift)
{
    const uchar map_3_to_8[8] = {
        0, 0111>>1, 0222>>1, 0333>>1, 0444>>1, 0555>>1, 0666>>1, 0377
    };
    const uchar map_2_to_8[4] = {
        0, 0x55, 0xaa, 0xff
    };
    const uchar map_1_to_8[2] = {
        0, 255
    };

    uchar val = (uchar) (idx >> shift);
    uchar res = 0;
    switch (nbits) {
    case 1:
        val &= 0x1;
        res =  map_1_to_8[val];
        break;
    case 2:
        val &= 0x3;
        res = map_2_to_8[val];
        break;
    case 3:
        val &= 0x7;
        res = map_3_to_8[val];
        break;
    default:
        res = 0;
    }
    return res;
}


static QRgb* qgl_create_rgb_palette(const PIXELFORMATDESCRIPTOR* pfd)
{
    if ((pfd->iPixelType != PFD_TYPE_RGBA) ||
         !(pfd->dwFlags & PFD_NEED_PALETTE) ||
         (pfd->cColorBits != 8))
        return 0;
    int numEntries = 1 << pfd->cColorBits;
    QRgb* pal = new QRgb[numEntries];
    for (int i = 0; i < numEntries; i++) {
        int r = qgl_rgb_palette_comp(i, pfd->cRedBits, pfd->cRedShift);
        int g = qgl_rgb_palette_comp(i, pfd->cGreenBits, pfd->cGreenShift);
        int b = qgl_rgb_palette_comp(i, pfd->cBlueBits, pfd->cBlueShift);
        pal[i] = qRgb(r, g, b);
    }

    const int syscol_indices[12] = {
        3, 24, 27, 64, 67, 88, 173, 181, 236, 247, 164, 91
    };

    const uint syscols[20] = {
        0x000000, 0x800000, 0x008000, 0x808000, 0x000080, 0x800080,
        0x008080, 0xc0c0c0, 0xc0dcc0, 0xa6caf0, 0xfffbf0, 0xa0a0a4,
        0x808080, 0xff0000, 0x00ff00, 0xffff00, 0x0000ff, 0xff00ff,
        0x00ffff, 0xffffff
    };        // colors #1 - #12 are not present in pal; gets added below

    if ((pfd->cColorBits == 8)                                &&
         (pfd->cRedBits   == 3) && (pfd->cRedShift   == 0)        &&
         (pfd->cGreenBits == 3) && (pfd->cGreenShift == 3)        &&
         (pfd->cBlueBits  == 2) && (pfd->cBlueShift  == 6)) {
        for (int j = 0 ; j < 12 ; j++)
            pal[syscol_indices[j]] = QRgb(syscols[j+1]);
    }

    return pal;
}

static QGLFormat pfdToQGLFormat(const PIXELFORMATDESCRIPTOR* pfd)
{
    QGLFormat fmt;
    fmt.setDoubleBuffer(pfd->dwFlags & PFD_DOUBLEBUFFER);
    fmt.setDepth(pfd->cDepthBits);
    if (fmt.depth())
	fmt.setDepthBufferSize(pfd->cDepthBits);
    fmt.setRgba(pfd->iPixelType == PFD_TYPE_RGBA);
    fmt.setAlpha(pfd->cAlphaBits);
    if (fmt.alpha())
	fmt.setAlphaBufferSize(pfd->cAlphaBits);
    fmt.setAccum(pfd->cAccumBits);
    if (fmt.accum())
	fmt.setAccumBufferSize(pfd->cAccumRedBits);
    fmt.setStencil(pfd->cStencilBits);
    if (fmt.stencil())
	fmt.setStencilBufferSize(pfd->cStencilBits);
    fmt.setStereo(pfd->dwFlags & PFD_STEREO);
    fmt.setDirectRendering((pfd->dwFlags & PFD_GENERIC_ACCELERATED) ||
                            !(pfd->dwFlags & PFD_GENERIC_FORMAT));
    fmt.setOverlay((pfd->bReserved & 0x0f) != 0);
    return fmt;
}

static QGLFormat pfiToQGLFormat(HDC hdc, int pfi)
{
    QGLFormat fmt;
    QVarLengthArray<int> iAttributes(40);
    QVarLengthArray<int> iValues(40);
    int i = 0;
    iAttributes[i++] = WGL_DOUBLE_BUFFER_ARB; // 0
    iAttributes[i++] = WGL_DEPTH_BITS_ARB; // 1
    iAttributes[i++] = WGL_PIXEL_TYPE_ARB; // 2
    iAttributes[i++] = WGL_ALPHA_BITS_ARB; // 3
    iAttributes[i++] = WGL_ACCUM_BITS_ARB; // 4
    iAttributes[i++] = WGL_STENCIL_BITS_ARB; // 5
    iAttributes[i++] = WGL_STEREO_ARB; // 6
    iAttributes[i++] = WGL_ACCELERATION_ARB; // 7
    iAttributes[i++] = WGL_SAMPLE_BUFFERS_ARB; // 8
    iAttributes[i++] = WGL_SAMPLES_ARB; // 9

    if (wglGetPixelFormatAttribivARB(hdc, pfi, 0, i,
				     iAttributes.constData(),
				     iValues.data()))
    {
	fmt.setDoubleBuffer(iValues[0]);
	fmt.setDepth(iValues[1]);
	if (fmt.depth())
	    fmt.setDepthBufferSize(iValues[1]);
	fmt.setRgba(iValues[2] == WGL_TYPE_RGBA_ARB);
	fmt.setAlpha(iValues[3]);
	if (fmt.alpha())
	    fmt.setAlphaBufferSize(iValues[3]);
	fmt.setAccum(iValues[4]);
	if (fmt.accum())
	    fmt.setAccumBufferSize(iValues[4]);
	fmt.setStencil(iValues[5]);
	if (fmt.stencil())
	    fmt.setStencilBufferSize(iValues[5]);
	fmt.setStereo(iValues[6]);
	fmt.setDirectRendering(iValues[7]);
	fmt.setSampleBuffers(iValues[8]);
	if (fmt.sampleBuffers())
	    fmt.setSamples(iValues[9]);
    }
#if 0
    qDebug() << "values for pfi:" << pfi;
    qDebug() << "0:" << fmt.doubleBuffer();
    qDebug() << "1:" << fmt.depthBufferSize();
    qDebug() << "2:" << fmt.rgba();
    qDebug() << "3:" << fmt.alphaBufferSize();
    qDebug() << "4:" << fmt.accumBufferSize();
    qDebug() << "5:" << fmt.stencilBufferSize();
    qDebug() << "6:" << fmt.stereo();
    qDebug() << "7:" << fmt.directRendering();
    qDebug() << "8:" << fmt.sampleBuffers();
    qDebug() << "9:" << fmt.samples();
#endif
    return fmt;
}

#define d d_func()
#define q q_func()

bool QGLContext::chooseContext(const QGLContext* shareContext)
{
    // workaround for matrox driver:
    // make a cheap call to opengl to force loading of DLL
    if (!opengl32dll) {
        GLint params;
        glGetIntegerv(GL_DEPTH_BITS, &params);
        opengl32dll = true;
    }

    bool result = true;
    HDC myDc;

    if (deviceIsPixmap()) {
        if (d->glFormat.plane())
            return false;                // Pixmaps can't have overlay
        d->win = 0;
        myDc = d->paintDevice->getDC();
    }
    else {
        d->win = ((QWidget*)d->paintDevice)->winId();
        myDc = GetDC(d->win);
    }

    if (!myDc) {
        qWarning("QGLContext::chooseContext(): Paint device cannot be null");
        result = false;
        goto end;
    }

    if (d->glFormat.plane()) {
        d->pixelFormatId = ((QGLWidget*)d->paintDevice)->context()->d->pixelFormatId;
        if (!d->pixelFormatId) {                // I.e. the glwidget is invalid
            qWarning("QGLContext::chooseContext(): Cannot create overlay context for invalid widget");
            result = false;
            goto end;
        }

        d->rc = wglCreateLayerContext(myDc, d->glFormat.plane());
        if (!d->rc) {
            qwglError("QGLContext::chooseContext()", "CreateLayerContext");
            result = false;
            goto end;
        }

        LAYERPLANEDESCRIPTOR lpfd;
        wglDescribeLayerPlane(myDc, d->pixelFormatId, d->glFormat.plane(), sizeof(LAYERPLANEDESCRIPTOR), &lpfd);
        d->glFormat.setDoubleBuffer(lpfd.dwFlags & LPD_DOUBLEBUFFER);
        d->glFormat.setDepth(lpfd.cDepthBits);
        d->glFormat.setRgba(lpfd.iPixelType == PFD_TYPE_RGBA);
        d->glFormat.setAlpha(lpfd.cAlphaBits);
        d->glFormat.setAccum(lpfd.cAccumBits);
        d->glFormat.setStencil(lpfd.cStencilBits);
        d->glFormat.setStereo(lpfd.dwFlags & LPD_STEREO);
        d->glFormat.setDirectRendering(false);
	if (d->glFormat.depth())
	    d->glFormat.setDepthBufferSize(lpfd.cDepthBits);
	if (d->glFormat.alpha())
	    d->glFormat.setAlphaBufferSize(lpfd.cAlphaBits);
	if (d->glFormat.accum())
	    d->glFormat.setAccumBufferSize(lpfd.cAccumRedBits);
	if (d->glFormat.stencil())
	    d->glFormat.setStencilBufferSize(lpfd.cStencilBits);

        if (d->glFormat.rgba()) {
            if (lpfd.dwFlags & LPD_TRANSPARENT)
                d->transpColor = QColor(lpfd.crTransparent & 0xff,
                                        (lpfd.crTransparent >> 8) & 0xff,
                                        (lpfd.crTransparent >> 16) & 0xff);
            else
                d->transpColor = QColor(0, 0, 0);
        }
        else {
            if (lpfd.dwFlags & LPD_TRANSPARENT)
                d->transpColor = QColor(qRgb(1, 2, 3));//, lpfd.crTransparent);
            else
                d->transpColor = QColor(qRgb(1, 2, 3));//, 0);

            d->cmap = new QGLCmap(1 << lpfd.cColorBits);
            d->cmap->setEntry(lpfd.crTransparent, qRgb(1, 2, 3));//, QGLCmap::Reserved);
        }

        if (shareContext && shareContext->isValid()) {
            d->sharing = (wglShareLists(shareContext->d->rc, d->rc) != 0);
            const_cast<QGLContext *>(shareContext)->d->sharing = d->sharing;
        }

        goto end;
    }
    {
        PIXELFORMATDESCRIPTOR pfd;
        PIXELFORMATDESCRIPTOR realPfd;
        d->pixelFormatId = choosePixelFormat(&pfd, myDc);
        if (d->pixelFormatId == 0) {
            qwglError("QGLContext::chooseContext()", "ChoosePixelFormat");
            result = false;
            goto end;
        }

        bool overlayRequested = d->glFormat.hasOverlay();
	DescribePixelFormat(myDc, d->pixelFormatId, sizeof(PIXELFORMATDESCRIPTOR), &realPfd);
	if (wglGetPixelFormatAttribivARB)
	    d->glFormat = pfiToQGLFormat(myDc, d->pixelFormatId);
	else
	    d->glFormat = pfdToQGLFormat(&realPfd);

        d->glFormat.setOverlay(d->glFormat.hasOverlay() && overlayRequested);

        if (deviceIsPixmap() && !(realPfd.dwFlags & PFD_DRAW_TO_BITMAP)) {
            qWarning("QGLContext::chooseContext(): Failed to get pixmap rendering context.");
            result = false;
            goto end;
        }

        if (deviceIsPixmap() &&
             (((QPixmap*)d->paintDevice)->depth() != realPfd.cColorBits)) {
            qWarning("QGLContext::chooseContext(): Failed to get pixmap rendering context of suitable depth.");
            result = false;
            goto end;
        }

        if (!SetPixelFormat(myDc, d->pixelFormatId, &realPfd)) {
            qwglError("QGLContext::chooseContext()", "SetPixelFormat");
            result = false;
            goto end;
        }

        if (!(d->rc = wglCreateLayerContext(myDc, 0))) {
            qwglError("QGLContext::chooseContext()", "wglCreateContext");
            result = false;
            goto end;
        }

        if (shareContext && shareContext->isValid()) {
            d->sharing = (wglShareLists(shareContext->d->rc, d->rc) != 0);
            const_cast<QGLContext *>(shareContext)->d->sharing = d->sharing;
	}

        if(!deviceIsPixmap()) {
            QRgb* pal = qgl_create_rgb_palette(&realPfd);
            if (pal) {
                QGLColormap cmap;
                cmap.setEntries(256, pal);
                ((QGLWidget*)d->paintDevice)->setColormap(cmap);
                delete[] pal;
            }
        }
    }

end:
    if (d->win)
        ReleaseDC(d->win, myDc);
    else if (deviceIsPixmap())
        d->paintDevice->releaseDC(myDc);
    return result;
}



static bool qLogEq(bool a, bool b)
{
    return (((!a) && (!b)) || (a && b));
}

/*!
    <strong>Win32 only</strong> This virtual function chooses a pixel
    format that matches the OpenGL \link setFormat() format\endlink.
    Reimplement this function in a subclass if you need a custom
    context.

    \warning The \a dummyPfd pointer and \a pdc are used as a \c
    PIXELFORMATDESCRIPTOR*. We use \c void to avoid using
    Windows-specific types in our header files.

    \sa chooseContext()
*/

int QGLContext::choosePixelFormat(void* dummyPfd, HDC pdc)
{
    // workaround for matrox driver:
    // make a cheap call to opengl to force loading of DLL
    if (!opengl32dll) {
        GLint params;
        glGetIntegerv(GL_DEPTH_BITS, &params);
        opengl32dll = true;
    }

    int chosenPfi = 0;
    if (wglChoosePixelFormatARB) {
	bool valid;
	int pixelFormat = 0;
	uint numFormats = 0;
	float fAttributes[] = {0, 0};
	QVarLengthArray<int> iAttributes(40);
	int i = 0;
	iAttributes[i++] = WGL_ACCELERATION_ARB;
	iAttributes[i++] = WGL_FULL_ACCELERATION_ARB;
	iAttributes[i++] = WGL_SUPPORT_OPENGL_ARB;
	iAttributes[i++] = GL_TRUE;
	iAttributes[i++] = WGL_DRAW_TO_WINDOW_ARB;
	iAttributes[i++] = GL_TRUE;
	iAttributes[i++] = WGL_COLOR_BITS_ARB;
	iAttributes[i++] = 32;
	if (d->glFormat.doubleBuffer()) {
	    iAttributes[i++] = WGL_DOUBLE_BUFFER_ARB;
	    iAttributes[i++] = GL_TRUE;
	}
	if (d->glFormat.stereo()) {
	    iAttributes[i++] = WGL_STEREO_ARB;
	    iAttributes[i++] = GL_TRUE;
	}
	if (d->glFormat.depth()) {
	    iAttributes[i++] = WGL_DEPTH_BITS_ARB;
	    iAttributes[i++] = d->glFormat.depthBufferSize() == -1 ? 24 : d->glFormat.depthBufferSize();
	}
	iAttributes[i++] = WGL_PIXEL_TYPE_ARB;
	if (d->glFormat.rgba())
	    iAttributes[i++] = WGL_TYPE_RGBA_ARB;
	else
	    iAttributes[i++] = WGL_TYPE_COLORINDEX_ARB;
	if (d->glFormat.alpha()) {
	    iAttributes[i++] = WGL_ALPHA_BITS_ARB;
	    iAttributes[i++] = d->glFormat.alphaBufferSize() == -1 ? 8 : d->glFormat.alphaBufferSize();
	}
 	if (d->glFormat.accum()) {
 	    iAttributes[i++] = WGL_ACCUM_BITS_ARB;
 	    iAttributes[i++] = d->glFormat.accumBufferSize() == -1 ? 16 : d->glFormat.accumBufferSize();
 	}
 	if (d->glFormat.stencil()) {
	    iAttributes[i++] = WGL_STENCIL_BITS_ARB;
	    iAttributes[i++] = d->glFormat.stencilBufferSize() == -1 ? 8 : d->glFormat.stencilBufferSize();
	}
	int si = 0;
	if (d->glFormat.sampleBuffers()) {
	    iAttributes[i++] = WGL_SAMPLE_BUFFERS_ARB;
	    iAttributes[i++] = GL_TRUE;
	    iAttributes[i++] = WGL_SAMPLES_ARB;
	    si = i;
	    iAttributes[i++] = d->glFormat.samples() == -1 ? 16 : d->glFormat.samples();
	}
	iAttributes[i] = 0;

	do {
	    valid = wglChoosePixelFormatARB(pdc, iAttributes.constData(), fAttributes, 1,
					    &pixelFormat, &numFormats);
	    if ((!valid || numFormats < 1) && d->glFormat.sampleBuffers())
		iAttributes[si] /= 2; // try different no. samples - we aim for the best one
	    else
		break;
	} while ((!valid || numFormats < 1) && iAttributes[si] > 1);
	chosenPfi = pixelFormat;
    } else {
	int pmDepth = deviceIsPixmap() ? ((QPixmap*)d->paintDevice)->depth() : 0;
	PIXELFORMATDESCRIPTOR* p = (PIXELFORMATDESCRIPTOR*)dummyPfd;
	memset(p, 0, sizeof(PIXELFORMATDESCRIPTOR));
	p->nSize = sizeof(PIXELFORMATDESCRIPTOR);
	p->nVersion = 1;
	p->dwFlags  = PFD_SUPPORT_OPENGL;
	if (deviceIsPixmap())
	    p->dwFlags |= PFD_DRAW_TO_BITMAP;
	else
	    p->dwFlags |= PFD_DRAW_TO_WINDOW;
	if (d->glFormat.doubleBuffer() && !deviceIsPixmap())
	    p->dwFlags |= PFD_DOUBLEBUFFER;
	if (d->glFormat.stereo())
	    p->dwFlags |= PFD_STEREO;
	if (d->glFormat.depth() && !deviceIsPixmap())
	    p->cDepthBits = d->glFormat.depthBufferSize() == -1 ? 32 : d->glFormat.depthBufferSize();
	else
	    p->dwFlags |= PFD_DEPTH_DONTCARE;
	if (d->glFormat.rgba()) {
	    p->iPixelType = PFD_TYPE_RGBA;
	    if (deviceIsPixmap())
		p->cColorBits = pmDepth;
	    else
		p->cColorBits = 32;
	} else {
	    p->iPixelType = PFD_TYPE_COLORINDEX;
	    p->cColorBits = 8;
	}
	if (d->glFormat.alpha())
	    p->cAlphaBits = d->glFormat.alphaBufferSize() == -1 ? 8 : d->glFormat.alphaBufferSize();
	if (d->glFormat.accum()) {
	    p->cAccumRedBits = p->cAccumGreenBits = p->cAccumBlueBits = p->cAccumAlphaBits =
			       d->glFormat.accumBufferSize() == -1 ? 16 : d->glFormat.accumBufferSize();
	}
	if (d->glFormat.stencil())
	    p->cStencilBits = d->glFormat.stencilBufferSize() == -1 ? 8 : d->glFormat.stencilBufferSize();
	p->iLayerType = PFD_MAIN_PLANE;
	chosenPfi = ChoosePixelFormat(pdc, p);

	if (!chosenPfi)
	    qErrnoWarning("QGLContext: ChoosePixelFormat failed");

	// Since the GDI function ChoosePixelFormat() does not handle
	// overlay and direct-rendering requests, we must roll our own here

	bool doSearch = chosenPfi <= 0;
	PIXELFORMATDESCRIPTOR pfd;
	QGLFormat fmt;
	if (!doSearch) {
	    DescribePixelFormat(pdc, chosenPfi, sizeof(PIXELFORMATDESCRIPTOR),
				&pfd);
	    fmt = pfdToQGLFormat(&pfd);
	    if (d->glFormat.hasOverlay() && !fmt.hasOverlay())
		doSearch = true;
	    else if (!qLogEq(d->glFormat.directRendering(), fmt.directRendering()))
		doSearch = true;
	    else if (deviceIsPixmap() && (!(pfd.dwFlags & PFD_DRAW_TO_BITMAP) ||
					  pfd.cColorBits != pmDepth))
		doSearch = true;
	    else if (!deviceIsPixmap() && !(pfd.dwFlags & PFD_DRAW_TO_WINDOW))
		doSearch = true;
	    else if (!qLogEq(d->glFormat.rgba(), fmt.rgba()))
		doSearch = true;
	}

	if (doSearch) {
	    int pfiMax = DescribePixelFormat(pdc, 0, 0, NULL);
	    int bestScore = -1;
	    int bestPfi = -1;
	    for (int pfi = 1; pfi <= pfiMax; pfi++) {
		DescribePixelFormat(pdc, pfi, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
		if (!(pfd.dwFlags & PFD_SUPPORT_OPENGL))
		    continue;
		if (deviceIsPixmap() && (!(pfd.dwFlags & PFD_DRAW_TO_BITMAP) ||
					 pfd.cColorBits != pmDepth))
		    continue;
		if (!deviceIsPixmap() && !(pfd.dwFlags & PFD_DRAW_TO_WINDOW))
		    continue;

		fmt = pfdToQGLFormat(&pfd);
		if (d->glFormat.hasOverlay() && !fmt.hasOverlay())
		    continue;

		int score = pfd.cColorBits;
		if (qLogEq(d->glFormat.depth(), fmt.depth()))
		    score += pfd.cDepthBits;
		if (qLogEq(d->glFormat.alpha(), fmt.alpha()))
		    score += pfd.cAlphaBits;
		if (qLogEq(d->glFormat.accum(), fmt.accum()))
		    score += pfd.cAccumBits;
		if (qLogEq(d->glFormat.stencil(), fmt.stencil()))
		    score += pfd.cStencilBits;
		if (qLogEq(d->glFormat.doubleBuffer(), fmt.doubleBuffer()))
		    score += 1000;
		if (qLogEq(d->glFormat.stereo(), fmt.stereo()))
		    score += 2000;
		if (qLogEq(d->glFormat.directRendering(), fmt.directRendering()))
		    score += 4000;
		if (qLogEq(d->glFormat.rgba(), fmt.rgba()))
		    score += 8000;
		if (score > bestScore) {
		    bestScore = score;
		    bestPfi = pfi;
		}
	    }

	    if (bestPfi > 0)
		chosenPfi = bestPfi;
	}
    }
    return chosenPfi;
}



void QGLContext::reset()
{
    // workaround for matrox driver:
    // make a cheap call to opengl to force loading of DLL
    if (!opengl32dll) {
        GLint params;
        glGetIntegerv(GL_DEPTH_BITS, &params);
        opengl32dll = true;
    }

    if (!d->valid)
        return;
    doneCurrent();
    if (d->rc)
        wglDeleteContext(d->rc);
    d->rc  = 0;
    if (d->win && d->dc)
        ReleaseDC(d->win, d->dc);
    d->dc  = 0;
    d->win = 0;
    d->pixelFormatId = 0;
    d->sharing = false;
    d->valid = false;
    d->transpColor = QColor();
    delete d->cmap;
    d->cmap = 0;
    d->initDone = false;
}



//
// NOTE: In a multi-threaded environment, each thread has a current
// context. If we want to make this code thread-safe, we probably
// have to use TLS (thread local storage) for keeping current contexts.
//

void QGLContext::makeCurrent()
{
    if (d->rc == wglGetCurrentContext() || !d->valid)        // already current
        return;
    if (d->win)
        d->dc = GetDC(d->win);
    else
        d->dc = d->paintDevice->getDC();
    HPALETTE hpal = QColormap::hPal();
    if (hpal) {
        SelectPalette(d->dc, hpal, false);
        RealizePalette(d->dc);
    }
    if (d->glFormat.plane()) {
        wglRealizeLayerPalette(d->dc, d->glFormat.plane(), true);
    }

    if (!wglMakeCurrent(d->dc, d->rc))
        qwglError("QGLContext::makeCurrent()", "wglMakeCurrent");
    currentCtx = this;
}


void QGLContext::doneCurrent()
{
    if (currentCtx != this)
        return;
    currentCtx = 0;
    wglMakeCurrent(0, 0);
    if (d->win && d->dc)
        ReleaseDC(d->win, d->dc);
    else if (d->dc)
        d->paintDevice->releaseDC(d->dc);
    d->dc = 0;
}

void QGLContext::swapBuffers() const
{
    if (d->dc && d->glFormat.doubleBuffer() && !deviceIsPixmap()) {
        if (d->glFormat.plane())
            wglSwapLayerBuffers(d->dc, WGL_SWAP_OVERLAY1);
        else {
            if (d->glFormat.hasOverlay())
                wglSwapLayerBuffers(d->dc, WGL_SWAP_MAIN_PLANE);
            else
                SwapBuffers(d->dc);
        }
    }
}


QColor QGLContext::overlayTransparentColor() const
{
    return d->transpColor;
}


uint QGLContext::colorIndex(const QColor& c) const
{
    if (!isValid())
        return 0;
    if (d->cmap) {
        int idx = d->cmap->find(c.rgb());
        if (idx >= 0)
            return idx;
        if (d->dc && d->glFormat.plane()) {
            idx = d->cmap->allocate(c.rgb());
            if (idx >= 0) {
                COLORREF r = RGB(qRed(c.rgb()),qGreen(c.rgb()),qBlue(c.rgb()));
                wglSetLayerPaletteEntries(d->dc, d->glFormat.plane(), idx, 1, &r);
                wglRealizeLayerPalette(d->dc, d->glFormat.plane(), true);
                return idx;
            }
        }
        return d->cmap->findNearest(c.rgb());
    }
    QColormap cmap = QColormap::instance();
    return cmap.pixel(c) & 0x00ffffff; // Assumes standard palette
}

void QGLContext::generateFontDisplayLists(const QFont & fnt, int listBase)
{
    WId winId;
    HDC glHdc;
    if (!isValid())
        return;
    if (deviceIsPixmap()) {
        winId = 0;
        glHdc = d->paintDevice->getDC();
    } else {
        winId = ((QWidget*)d->paintDevice)->winId();
        glHdc = GetDC(winId);
    }
    SelectObject(glHdc, fnt.handle());
    if (!wglUseFontBitmaps(glHdc, 0, 256, listBase))
        qWarning("QGLContext::generateFontDisplayLists: Could not generate display lists for font '%s'", fnt.family().toLatin1().data());
    if (winId)
        ReleaseDC(winId, glHdc);
    else if (deviceIsPixmap())
        d->paintDevice->releaseDC(glHdc);
}

void *QGLContext::getProcAddress(const QString &proc) const
{
    return (void *)wglGetProcAddress(proc.toLatin1());
}

/*****************************************************************************
  QGLWidget Win32/WGL-specific code
 *****************************************************************************/

void QGLWidgetPrivate::init(QGLContext *ctx, const QGLWidget* shareWidget)
{
    QGLExtensions::init();
    glcx = 0;
    autoSwap = true;

    if (!ctx->device())
        ctx->setDevice(q);

    if (shareWidget) {
        q->setContext(ctx, shareWidget->context());
    } else {
        q->setContext(ctx);
    }
    q->setAttribute(Qt::WA_NoSystemBackground);

    if (q->isValid() && q->context()->format().hasOverlay()) {
        olcx = new QGLContext(QGLFormat::defaultOverlayFormat(), q);
        if (!olcx->create(shareWidget ? shareWidget->overlayContext() : 0)) {
            delete olcx;
            olcx = 0;
            glcx->d->glFormat.setOverlay(false);
        }
    } else {
        olcx = 0;
    }
}


bool QGLWidget::event(QEvent *e)
{
    if (e->type() == QEvent::ParentChange)
        setContext(new QGLContext(d->glcx->requestedFormat(), this));
    return QWidget::event(e);
}


void QGLWidget::setMouseTracking(bool enable)
{
    QWidget::setMouseTracking(enable);
}


void QGLWidget::resizeEvent(QResizeEvent *)
{
    if (!isValid())
        return;
    makeCurrent();
    if (!d->glcx->initialized())
        glInit();
    resizeGL(width(), height());
    if (d->olcx) {
        makeOverlayCurrent();
        resizeOverlayGL(width(), height());
    }
}


const QGLContext* QGLWidget::overlayContext() const
{
    return d->olcx;
}


void QGLWidget::makeOverlayCurrent()
{
    if (d->olcx) {
        d->olcx->makeCurrent();
        if (!d->olcx->initialized()) {
            initializeOverlayGL();
            d->olcx->setInitialized(true);
        }
    }
}


void QGLWidget::updateOverlayGL()
{
    if (d->olcx) {
        makeOverlayCurrent();
        paintOverlayGL();
        if (d->olcx->format().doubleBuffer()) {
            if (d->autoSwap)
                d->olcx->swapBuffers();
        }
        else {
            glFlush();
        }
    }
}


void QGLWidget::setContext(QGLContext *context,
                            const QGLContext* shareContext,
                            bool deleteOldContext)
{
    if (context == 0) {
        qWarning("QGLWidget::setContext: Cannot set null context");
        return;
    }
    if (!context->deviceIsPixmap() && context->device() != this) {
        qWarning("QGLWidget::setContext: Context must refer to this widget");
        return;
    }

    if (d->glcx)
        d->glcx->doneCurrent();
    QGLContext* oldcx = d->glcx;
    d->glcx = context;

    bool doShow = false;
    if (oldcx && oldcx->d->win == winId() && !d->glcx->deviceIsPixmap()) {
        // We already have a context and must therefore create a new
        // window since Windows does not permit setting a new OpenGL
        // context for a window that already has one set.
        doShow = isVisible();
        QWidget *pW = static_cast<QWidget *>(parent());
        QPoint pos = geometry().topLeft();
        setParent(pW, windowFlags());
        move(pos);
    }

    if (!d->glcx->isValid())
        d->glcx->create(shareContext ? shareContext : oldcx);

    if (deleteOldContext)
        delete oldcx;

    if (doShow)
        show();
}


bool QGLWidgetPrivate::renderCxPm(QPixmap*)
{
    return false;
}

void QGLWidgetPrivate::cleanupColormaps()
{
    if (cmap.handle()) {
        HDC hdc = GetDC(q->winId());
        SelectPalette(hdc, (HPALETTE) GetStockObject(DEFAULT_PALETTE), false);
        DeleteObject((HPALETTE) cmap.handle());
        ReleaseDC(q->winId(), hdc);
        cmap.setHandle(0);
    }
    return;
}

const QGLColormap & QGLWidget::colormap() const
{
    return d->cmap;
}

/*\internal
  Store color values in the given colormap.
*/
static void qStoreColors(HPALETTE cmap, const QGLColormap & cols)
{
    QRgb color;
    PALETTEENTRY pe;

    for (int i = 0; i < cols.size(); i++) {
        color = cols.entryRgb(i);
        pe.peRed   = qRed(color);
        pe.peGreen = qGreen(color);
        pe.peBlue  = qBlue(color);
        pe.peFlags = 0;

        SetPaletteEntries(cmap, i, 1, &pe);
    }
}

void QGLWidget::setColormap(const QGLColormap & c)
{
    d->cmap = c;
    if (!d->cmap.handle())
        return;

    if (d->cmap.handle()) { // already have an allocated cmap
        HDC hdc = GetDC(winId());
        SelectPalette(hdc, (HPALETTE) d->cmap.handle(), false);
        qStoreColors((HPALETTE) d->cmap.handle(), c);
        RealizePalette(hdc);
        ReleaseDC(winId(), hdc);
    } else {
        LOGPALETTE * lpal = (LOGPALETTE *) malloc(sizeof(LOGPALETTE)
                            + c.size() * sizeof(PALETTEENTRY));

        lpal->palVersion    = 0x300;
        lpal->palNumEntries = c.size();
        d->cmap.setHandle(CreatePalette(lpal));

        if (d->cmap.handle()) {
            HDC hdc = GetDC(winId());
            SelectPalette(hdc, (HPALETTE) d->cmap.handle(), false);
            qStoreColors((HPALETTE) d->cmap.handle(), c);
            RealizePalette(hdc);
            ReleaseDC(winId(), hdc);
        }
        free(lpal);
    }
}

void QGLExtensions::init()
{
    static bool init_done = false;

    if (init_done)
	return;
    init_done = true;

    // we need a current GL context in order to obtain the API
    // entries and GL extension strings
    QWidget dmy(0);
    HDC dmy_pdc = GetDC(dmy.winId());
    PIXELFORMATDESCRIPTOR dmy_pfd = {
	sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_SUPPORT_OPENGL,
	PFD_TYPE_RGBA, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
    };
    int dmy_pf = ChoosePixelFormat(dmy_pdc, &dmy_pfd);
    SetPixelFormat(dmy_pdc, dmy_pf, &dmy_pfd);
    HGLRC dmy_rc = wglCreateContext(dmy_pdc);
    wglMakeCurrent(dmy_pdc, dmy_rc);

    wglChoosePixelFormatARB =
	(PFNWGLCHOOSEPIXELFORMATARB) wglGetProcAddress("wglChoosePixelFormatARB");
    wglGetPixelFormatAttribivARB =
	(PFNWGLGETPIXELFORMATATTRIBIVARB) wglGetProcAddress("wglGetPixelFormatAttribivARB");

    init_extensions();

    wglMakeCurrent(dmy_pdc, 0);
    wglDeleteContext(dmy_rc);
    ReleaseDC(dmy.winId(), dmy_pdc);
}
