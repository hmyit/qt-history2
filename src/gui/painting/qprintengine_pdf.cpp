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

#include "qprintengine_pdf_p.h"

#ifndef QT_NO_PRINTER
#include <qiodevice.h>
#include <qpainter.h>
#include <qbitmap.h>
#include <qpainterpath.h>
#include <qpaintdevice.h>
#include <qfile.h>
#include <qdebug.h>

#include <time.h>
#include <limits.h>
#include <math.h>
#ifndef QT_NO_COMPRESS
#include <zlib.h>
#endif

// might be helpful for smooth transforms of images
static const bool interpolateImages = false;

#ifdef QT_NO_COMPRESS
static const bool do_compress = false;
#else
static const bool do_compress = false;
#endif

static const int resolution = 72;

/* also adds a space at the end of the number */
const char *qt_real_to_string(qreal val, char *buf) {
    const char *ret = buf;
    if (val < 0) {
        *(buf++) = '-';
        val = -val;
    }
    int ival = (int) val;
    qreal frac = val - (qreal)ival;

    int ifrac = (int)(frac * 1000000);
    if (ifrac == 1000000) {
        ++ival;
        ifrac = 0;
    }
    char output[256];
    int i = 0;
    while (ival) {
        output[i] = '0' + (ival % 10);
        ++i;
        ival /= 10;
    }
    if (i == 0) {
        *(buf++) = '0';
    } else {
        while (i)
            *(buf++) = output[--i];
    }

    if (ifrac) {
        *(buf++) =  '.';
        int fact = 100000;
        while (fact) {
            *(buf++) = '0' + ((ifrac/fact) % 10);
            fact /= 10;
        }
    }
    *(buf++) = ' ';
    *buf = 0;
    return ret;
}

const char *qt_int_to_string(int val, char *buf) {
    const char *ret = buf;
    if (val < 0) {
        *(buf++) = '-';
        val = -val;
    }
    char output[256];
    int i = 0;
    while (val) {
        output[i] = '0' + (val % 10);
        ++i;
        val /= 10;
    }
    if (i == 0) {
        *(buf++) = '0';
    } else {
        while (i)
            *(buf++) = output[--i];
    }
    *(buf++) = ' ';
    *buf = 0;
    return ret;
}

#define QT_PATH_ELEMENT(elm) 

QByteArray QPdf::generatePath(const QPainterPath &path, PathFlags flags)
{
    QByteArray result;
    if (flags == PathNone || !path.elementCount())
        return result;
    
    ByteStream s(&result);
    
    int start = -1;
    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element &elm = path.elementAt(i);
        switch (elm.type) {
        case QPainterPath::MoveToElement:
            if (start >= 0
                && path.elementAt(start).x == path.elementAt(i-1).x
                && path.elementAt(start).y == path.elementAt(i-1).y)
                s << "h\n";
            s << elm.x << elm.y << "m\n";
            start = i;
                break;
        case QPainterPath::LineToElement:
            s << elm.x << elm.y << "l\n";
            break;
        case QPainterPath::CurveToElement:
            Q_ASSERT(path.elementAt(i+1).type == QPainterPath::CurveToDataElement);
            Q_ASSERT(path.elementAt(i+2).type == QPainterPath::CurveToDataElement);
            s << elm.x << elm.y
              << path.elementAt(i+1).x << path.elementAt(i+1).y
              << path.elementAt(i+2).x << path.elementAt(i+2).y
              << "c\n";
            i += 2;
            break;
        default:
            qFatal("QPdf::generatePath(), unhandled type: %d", elm.type);
        }
    }
    if (start >= 0
        && path.elementAt(start).x == path.elementAt(path.elementCount()-1).x
        && path.elementAt(start).y == path.elementAt(path.elementCount()-1).y)
        s << "h\n";

    Qt::FillRule fillRule = path.fillRule();

    const char *op = 0;
    switch (flags) {
    case PathIsClip:
        op = (fillRule == Qt::WindingFill) ? "W n\n" : "W* n\n";
        break;
    case StrokeOnly:
        op = "S\n";
        break;
    case FillOnly:
        op = (fillRule == Qt::WindingFill) ? "f\n" : "f*\n";
        break;
    case StrokeAndFill:
        op = (fillRule == Qt::WindingFill) ? "B\n" : "B*\n";
    case PathNone:
        break;
    }
    s << op;
    return result;
}

QByteArray QPdf::generateMatrix(const QMatrix &matrix)
{
    QByteArray result;
    ByteStream s(&result);
    s << matrix.m11()
      << matrix.m12()
      << matrix.m21()
      << matrix.m22()
      << matrix.dx()
      << matrix.dy()
      << "cm\n";
    return result;
}

QByteArray QPdf::generateDashes(const QPen &pen)
{
    QByteArray result;
    ByteStream s(&result);
    s << "[";

    QVector<qreal> dasharray = pen.dashPattern();
    qreal w = pen.widthF();
    if (w < 0.001)
        w = 1;
    for (int i = 0; i < dasharray.size(); ++i)
        s << dasharray.at(i)*w;
    s << "]";
    //qDebug() << "dasharray: pen has" << dasharray;
    //qDebug() << "  => " << result;
    return result;
}



static const char* pattern_for_brush[] = {
    0, // NoBrush
    0, // SolidPattern
    "0 J\n"
    "6 w\n"
    "[] 0 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "0 4 m\n"
    "8 4 l\n"
    "S\n", // Dense1Pattern

    "0 J\n"
    "2 w\n"
    "[6 2] 1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n"
    "[] 0 d\n"
    "2 0 m\n"
    "2 8 l\n"
    "6 0 m\n"
    "6 8 l\n"
    "S\n"
    "[6 2] -3 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // Dense2Pattern

    "0 J\n"
    "2 w\n"
    "[6 2] 1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n"
    "[2 2] -1 d\n"
    "2 0 m\n"
    "2 8 l\n"
    "6 0 m\n"
    "6 8 l\n"
    "S\n"
    "[6 2] -3 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // Dense3Pattern

    "0 J\n"
    "2 w\n"
    "[2 2] 1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n"
    "[2 2] -1 d\n"
    "2 0 m\n"
    "2 8 l\n"
    "6 0 m\n"
    "6 8 l\n"
    "S\n"
    "[2 2] 1 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // Dense4Pattern

    "0 J\n"
    "2 w\n"
    "[2 6] -1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n"
    "[2 2] 1 d\n"
    "2 0 m\n"
    "2 8 l\n"
    "6 0 m\n"
    "6 8 l\n"
    "S\n"
    "[2 6] 3 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // Dense5Pattern

    "0 J\n"
    "2 w\n"
    "[2 6] -1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n"
    "[2 6] 3 d\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // Dense6Pattern

    "0 J\n"
    "2 w\n"
    "[2 6] -1 d\n"
    "0 0 m\n"
    "0 8 l\n"
    "8 0 m\n"
    "8 8 l\n"
    "S\n", // Dense7Pattern

    "1 w\n"
    "0 4 m\n"
    "8 4 l\n"
    "S\n", // HorPattern

    "1 w\n"
    "4 0 m\n"
    "4 8 l\n"
    "S\n", // VerPattern

    "1 w\n"
    "4 0 m\n"
    "4 8 l\n"
    "0 4 m\n"
    "8 4 l\n"
    "S\n", // CrossPattern

    "1 w\n"
    "-1 5 m\n"
    "5 -1 l\n"
    "3 9 m\n"
    "9 3 l\n"
    "S\n", // BDiagPattern

    "1 w\n"
    "-1 3 m\n"
    "5 9 l\n"
    "3 -1 m\n"
    "9 5 l\n"
    "S\n", // FDiagPattern

    "1 w\n"
    "-1 3 m\n"
    "5 9 l\n"
    "3 -1 m\n"
    "9 5 l\n"
    "-1 5 m\n"
    "5 -1 l\n"
    "3 9 m\n"
    "9 3 l\n"
    "S\n", // DiagCrossPattern
};

QByteArray QPdf::patternForBrush(const QBrush &b)
{
    int style = b.style();
    if (style > Qt::DiagCrossPattern)
        return QByteArray();
    return pattern_for_brush[style];
}

#undef MM
#define MM(n) int((n * 720 + 127) / 254)

#undef IN
#define IN(n) int(n * 72)

struct PaperSize {
    int width, height;
};

static const PaperSize paperSizes[QPrinter::NPageSize] = {
    {  MM(210), MM(297) },      // A4
    {  MM(176), MM(250) },      // B5
    {  IN(8.5), IN(11) },       // Letter
    {  IN(8.5), IN(14) },       // Legal
    {  IN(7.5), IN(10) },       // Executive
    {  MM(841), MM(1189) },     // A0
    {  MM(594), MM(841) },      // A1
    {  MM(420), MM(594) },      // A2
    {  MM(297), MM(420) },      // A3
    {  MM(148), MM(210) },      // A5
    {  MM(105), MM(148) },      // A6
    {  MM(74), MM(105)},        // A7
    {  MM(52), MM(74) },        // A8
    {  MM(37), MM(52) },        // A9
    {  MM(1000), MM(1414) },    // B0
    {  MM(707), MM(1000) },     // B1
    {  MM(31), MM(44) },        // B10
    {  MM(500), MM(707) },      // B2
    {  MM(353), MM(500) },      // B3
    {  MM(250), MM(353) },      // B4
    {  MM(125), MM(176) },      // B6
    {  MM(88), MM(125) },       // B7
    {  MM(62), MM(88) },        // B8
    {  MM(44), MM(62) },        // B9
    {  MM(162),    MM(229) },   // C5E
    {  IN(4.125),  IN(9.5) },   // Comm10E
    {  MM(110),    MM(220) },   // DLE
    {  IN(8.5),    IN(13) },    // Folio
    {  IN(17),     IN(11) },    // Ledger
    {  IN(11),     IN(17) }     // Tabloid
};


QPdfPage::QPdfPage()
    : QPdf::ByteStream(&data)
{
}

void QPdfPage::streamImage(int w, int h, int object)
{
    *this << "q " << w << "0 0 " << -h << "0 " << h << "cm /Im" << object << " Do Q\n";
    if (!images.contains(object))
        images.append(object);
}


inline QPaintEngine::PaintEngineFeatures qt_pdf_decide_features()
{
    QPaintEngine::PaintEngineFeatures f = QPaintEngine::AllFeatures;
    f &= ~(QPaintEngine::PorterDuff
           | QPaintEngine::LinearGradientFill
           | QPaintEngine::RadialGradientFill
           | QPaintEngine::ConicalGradientFill);
    return f;
}

QPdfEngine::QPdfEngine()
    : QPaintEngine(qt_pdf_decide_features()), outFile_(new QFile)
{
    device_ = 0;
    backgroundMode = Qt::TransparentMode;

    d = new QPdfEnginePrivate;

    pagesize_ = QPrinter::A4;
    QRect r = paperRect();
    d->setDimensions(r.width(),r.height());
    brushSoftMask = penSoftMask = 0;
}

QPdfEngine::~QPdfEngine()
{
    delete d;

    delete outFile_;
}


void QPdfEngine::setProperty(PrintEnginePropertyKey key, const QVariant &value)
{
    switch (key) {
    case PPK_Creator:
        d->creator = value.toString();
        break;
    case PPK_DocumentName:
        d->title = value.toString();
        break;
    case PPK_Orientation:
        d->orientation = QPrinter::Orientation(value.toInt());
        break;
    case PPK_OutputFileName: {
        if (isActive()) {
            qWarning("QPdfEngine::setFileName: Not possible while painting");
            return;
        }
        QString filename = value.toString();

        if (filename.isEmpty())
            return;

        outFile_->setFileName(filename);
        setDevice(outFile_);
    }
        break;
    case PPK_PageSize: {
        pagesize_ = QPrinter::PageSize(value.toInt());
        QRect r = paperRect();
        d->setDimensions(r.width(),r.height());
    }
        break;
    case PPK_FullPage:
        d->fullPage = value.toBool();
        break;
    default:
        break;
    }
}

QVariant QPdfEngine::property(PrintEnginePropertyKey key) const
{
    switch (key) {
    case PPK_ColorMode:
        return QPrinter::Color;
    case PPK_Creator:
        return d->creator;
    case PPK_DocumentName:
        return d->title;
    case PPK_FullPage:
        return d->fullPage;
    case PPK_NumberOfCopies:
        return 1;
    case PPK_Orientation:
        return d->orientation;
    case PPK_OutputFileName:
        return outFile_->fileName();
    case PPK_PageRect:
        return pageRect();
    case PPK_PageSize:
        return pagesize_;
    case PPK_PaperRect:
        return paperRect();
    case PPK_PaperSource:
        return QPrinter::Auto;
    case PPK_Resolution:
        return 600;
    case PPK_SupportedResolutions:
        return QList<QVariant>() << resolution;
    default:
        break;
    }
    return QVariant();
}

void QPdfEngine::setAuthor(const QString &author)
{
    d->author = author;
}

QString QPdfEngine::author() const
{
    return d->author;
}

QRect QPdfEngine::paperRect() const
{
    PaperSize s = paperSizes[pagesize_];
    int w = qRound(s.width);
    int h = qRound(s.height);
    if (d->orientation == QPrinter::Portrait)
        return QRect(0, 0, w, h);
    else
        return QRect(0, 0, h, w);
}

QRect QPdfEngine::pageRect() const
{
    QRect r = paperRect();
    if (d->fullPage)
        return r;
    // would be nice to get better margins than this.
    return QRect(resolution/3, resolution/3, r.width()-2*resolution/3, r.height()-2*resolution/3);
}

void QPdfEngine::setDevice(QIODevice* dev)
{
    if (isActive()) {
        qWarning("QPdfEngine::setDevice: Device cannot be set while painting");
        return;
    }
    device_ = dev;
}

bool QPdfEngine::begin (QPaintDevice *)
{
    if (!device_) {
        qWarning("QPdfEngine::begin: No valid device");
        return false;
    }

    if (device_->isOpen())
        device_->close();
    if(!device_->open(QIODevice::WriteOnly)) {
        qWarning("QPdfEngine::begin: Cannot open IO device");
        return false;
    }

    hasPen = true;
    hasBrush = false;
    clipEnabled = false;
    allClipped = false;

    d->unsetDevice();
    d->setDevice(device_);
    setActive(true);
    d->writeHeader();
    newPage();

    return true;
}

bool QPdfEngine::end ()
{
    d->writeTail();

    device_->close();
    d->unsetDevice();
    setActive(false);
    return true;
}

void QPdfEngine::drawPoints (const QPointF *points, int pointCount)
{
    if (!points)
        return;

    QPainterPath p;
    for (int i=0; i!=pointCount;++i) {
        p.moveTo(points[i]);
        p.lineTo(points[i]);
    }
    drawPath(p);
}

void QPdfEngine::drawLines (const QLineF *lines, int lineCount)
{
    if (!lines)
        return;

    QPainterPath p;
    for (int i=0; i!=lineCount;++i) {
        p.moveTo(lines[i].p1());
        p.lineTo(lines[i].p2());
    }
    drawPath(p);
}

void QPdfEngine::drawRects (const QRectF *rects, int rectCount)
{
    if (!rects)
        return;

    QPainterPath p;
    for (int i=0; i!=rectCount; ++i) {
        p.addRect(rects[i]);
    }
    drawPath(p);
}

void QPdfEngine::drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
    if (!points || !pointCount)
        return;

    QPainterPath p;

    bool hb = hasBrush;

    switch(mode) {
    case OddEvenMode:
        p.setFillRule(Qt::OddEvenFill);
        break;
    case ConvexMode:
    case WindingMode:
        p.setFillRule(Qt::WindingFill);
        break;
    case PolylineMode:
        hasBrush = false;
        break;
    default:
        break;
    }

    p.moveTo(points[0]);
    for (int i = 1; i < pointCount; ++i)
        p.lineTo(points[i]);

    if (mode != PolylineMode)
        p.closeSubpath();
    drawPath(p);

    hasBrush = hb;
}

void QPdfEngine::drawPath (const QPainterPath &p)
{
    if (clipEnabled && allClipped)
        return;
    
    QPdf::PathFlags type = hasBrush ? QPdf::FillOnly : QPdf::PathNone;
    if (hasPen) 
        type = (type == QPdf::PathNone) ? QPdf::StrokeOnly : QPdf::StrokeAndFill;
    //qDebug("drawPath: type=%d hasPen=%d hasBrush=%d\n<< %s >>", type, hasPen, hasBrush, QPdf::generatePath(p, type).constData());

    if (type == QPdf::StrokeAndFill && (penSoftMask || brushSoftMask)) {
        // need to do separate stroking and filling
        // ### update soft masks
        *d->currentPage << QPdf::generatePath(p, QPdf::FillOnly);
        type = QPdf::StrokeOnly;
    }
    if (type != QPdf::PathNone) {
        // ### update soft mask if needed
        *d->currentPage << QPdf::generatePath(p, type);
    }
}

void QPdfEngine::drawPixmap (const QRectF & rectangle, const QPixmap & pixmap, const QRectF & sr)
{
    if (sr.isEmpty() || rectangle.isEmpty() || pixmap.isNull())
        return;
    QBrush b = brush;

    QPixmap pm = pixmap.copy(sr.toRect());
    QImage image = pm.toImage();

    *d->currentPage << "q\n";
    *d->currentPage
        << QPdf::generateMatrix(QMatrix(rectangle.width() / sr.width(), 0, 0, rectangle.height() / sr.height(),
                                        rectangle.x(), rectangle.y()));
    bool bitmap = true;
    int object = d->addImage(image, &bitmap);
    if (bitmap) {
        if (backgroundMode == Qt::OpaqueMode) {
            // draw background
            brush = backgroundBrush;
            setBrush();
            QByteArray fill;
            QPdf::ByteStream s(&fill);
            s << "0 0 " << rectangle.width() << rectangle.height() << "re f\n";
            *d->currentPage << fill;
        }
        // set current pen as brush
        brush = pen.brush();
        setBrush();
    }
    d->currentPage->streamImage(image.width(), image.height(), object);
    *d->currentPage << "Q\n";

    brush = b;
}

void QPdfEngine::drawImage(const QRectF & rectangle, const QImage & image, const QRectF & sr, Qt::ImageConversionFlags)
{
    if (sr.isEmpty() || rectangle.isEmpty() || image.isNull())
        return;

    QImage im = image.copy(sr.toRect());

    *d->currentPage << "q\n";
    *d->currentPage
        << QPdf::generateMatrix(QMatrix(rectangle.width() / sr.width(), 0, 0, rectangle.height() / sr.height(),
                                        rectangle.x(), rectangle.y()));
    bool bitmap = false;
    int object = d->addImage(im, &bitmap);
    d->currentPage->streamImage(image.width(), image.height(), object);
    *d->currentPage << "Q\n";
}

void QPdfEngine::drawTiledPixmap (const QRectF &rectangle, const QPixmap &pixmap, const QPointF &point)
{
    bool bitmap = (pixmap.depth() == 1);
    QBrush b = brush;
    QPointF bo = brushOrigin;
    bool hp = hasPen;
    hasPen = false;
    bool hb = hasBrush;
    hasBrush = true;

    if (bitmap) {
        if (backgroundMode == Qt::OpaqueMode) {
            // draw background
            brush = backgroundBrush;
            setBrush();
            QByteArray fill;
            QPdf::ByteStream s(&fill);
            s << rectangle.x() << rectangle.y() << rectangle.width() << rectangle.height() << "re f\n";
            *d->currentPage << fill;
        }
    }
    brush = QBrush(pixmap);
    if (bitmap)    
        // #### fix bitmap case where we have a brush pen
        brush.setColor(pen.color());

    brushOrigin = -point;
    *d->currentPage << "q\n";
    setBrush();

    drawRects(&rectangle, 1);
    *d->currentPage << "Q\n";

    hasPen = hp;
    hasBrush = hb;
    brush = b;
    brushOrigin = bo;
}

void QPdfEngine::updateState(const QPaintEngineState &state)
{
    QPaintEngine::DirtyFlags flags = state.state();

    if (flags & DirtyTransform) {
        QMatrix m = state.matrix();
        if (m == matrix)
            flags &= ~DirtyTransform;
        else
            matrix = m;
    }
    if (flags & DirtyPen) {
        pen = state.pen();
        hasPen = pen != Qt::NoPen;
    }
    if (flags & DirtyBrush) {
        brush = state.brush();
        hasBrush = brush != Qt::NoBrush;
    }
    if (flags & DirtyBrushOrigin) {
        brushOrigin = state.brushOrigin();
        flags |= DirtyBrush;
    }
    
    if (flags & DirtyBackground)
        backgroundBrush = state.backgroundBrush();
    if (flags & DirtyBackgroundMode)
        backgroundMode = state.backgroundMode();

    bool ce = clipEnabled;
    if (flags & DirtyClipEnabled) 
        clipEnabled = state.isClipEnabled();
    if (flags & DirtyClipPath) 
        updateClipPath(state.clipPath(), state.clipOperation());
    if (flags & DirtyClipRegion) {
        QPainterPath path;
        QVector<QRect> rects = state.clipRegion().rects();
        for (int i = 0; i < rects.size(); ++i)
            path.addRect(rects.at(i));
        updateClipPath(path, state.clipOperation());
        flags |= DirtyClipPath;
    }

    if (ce != clipEnabled)
        flags |= DirtyClipPath;
    else if (!clipEnabled)
        flags &= ~DirtyClipPath;
    
    if (flags & DirtyTransform|DirtyClipPath) {
        *d->currentPage << "Q q\n";
        flags |= DirtyTransform|DirtyClipPath|DirtyPen|DirtyBrush;
    }

    if (flags & DirtyClipPath) {
        allClipped = false;
        if (clipEnabled && !clips.isEmpty()) {
            for (int i = 0; i < clips.size(); ++i) {
                if (clips.at(i).isEmpty()) {
                    allClipped = true;
                    break;
                }
            }
            if (!allClipped) {
                for (int i = 0; i < clips.size(); ++i) {
                    *d->currentPage << QPdf::generatePath(clips.at(i), QPdf::PathIsClip);
                }
            }
        }
    }

    if (flags & DirtyTransform)
        *d->currentPage << QPdf::generateMatrix(matrix);

    if (flags & DirtyPen)
        setPen();
    if (flags & DirtyBrush)
        setBrush();
}

void QPdfEngine::updateClipPath(const QPainterPath &p, Qt::ClipOperation op)
{
    QPainterPath path = matrix.map(p);
    //qDebug() << "updateClipPath: " << matrix << p.boundingRect() << path.boundingRect();

    if (op == Qt::NoClip) {
        clipEnabled = false;
    } else if (op == Qt::ReplaceClip) {
        clips.clear();
        clips.append(path);
    } else if (op == Qt::IntersectClip) {
        clips.append(path);
    } else { // UniteClip
        // ask the painter for the current clipping path. that's the easiest solution
        path = painter()->clipPath();
        path = matrix.map(path);
        clips.clear();
        clips.append(path);
    }
}

void QPdfEngine::setPen()
{
    bool specifyColor;
    QBrush b = pen.brush();
    int patternObject = d->addBrushPattern(b, matrix, QPointF(), &specifyColor);

    *d->currentPage << (patternObject ? "/PCSp CS " : "/CSp CS ");
    if (specifyColor) {
        QColor rgba = b.color();
        *d->currentPage << rgba.redF()
                        << rgba.greenF()
                        << rgba.blueF();
    }
    if (patternObject)
        *d->currentPage << "/Pat" << patternObject;
    *d->currentPage << "SCN\n";

    *d->currentPage << pen.widthF() << "w ";

    int pdfCapStyle = 0;
    switch(pen.capStyle()) {
    case Qt::FlatCap:
        pdfCapStyle = 0;
        break;
    case Qt::SquareCap:
        pdfCapStyle = 2;
        break;
    case Qt::RoundCap:
        pdfCapStyle = 1;
        break;
    default:
        break;
    }
    *d->currentPage << pdfCapStyle << "J ";
    
    int pdfJoinStyle = 0;
    switch(pen.joinStyle()) {
    case Qt::MiterJoin:
        pdfJoinStyle = 0;
        break;
    case Qt::BevelJoin:
        pdfJoinStyle = 2;
        break;
    case Qt::RoundJoin:
        pdfJoinStyle = 1;
        break;
    default:
        break;
    }
    *d->currentPage << pdfJoinStyle << "j ";

    *d->currentPage << QPdf::generateDashes(pen) << " 0 d\n";

    int penObj = d->addPenGState(pen);
    if (penObj)
        *d->currentPage << "/GState" << penObj << "gs\n";
}

void QPdfEngine::setBrush()
{
    bool specifyColor;
    int patternObject = d->addBrushPattern(brush, matrix, brushOrigin, &specifyColor);
    
    *d->currentPage << (patternObject ? "/PCSp cs " : "/CSp cs ");
    if (specifyColor) {
        QColor rgba = brush.color();
        *d->currentPage << rgba.redF()
                        << rgba.greenF()
                        << rgba.blueF();
    }
    if (patternObject)
        *d->currentPage << "/Pat" << patternObject;
    *d->currentPage << "scn\n";
    
    int gStateObject = d->addBrushGState(brush);
    if (gStateObject) 
        *d->currentPage << "/GState" << gStateObject << "gs\n";
}


int QPdfEngine::metric(QPaintDevice::PaintDeviceMetric metricType) const
{
    int val;
    QRect r = d->fullPage ? paperRect() : pageRect();
    switch (metricType) {
    case QPaintDevice::PdmWidth:
        val = r.width();
        break;
    case QPaintDevice::PdmHeight:
        val = r.height();
        break;
    case QPaintDevice::PdmDpiX:
    case QPaintDevice::PdmDpiY:
    case QPaintDevice::PdmPhysicalDpiX:
    case QPaintDevice::PdmPhysicalDpiY:
        val = 72;
        break;
    case QPaintDevice::PdmWidthMM:
        val = qRound(r.width()*25.4/72.);
        break;
    case QPaintDevice::PdmHeightMM:
        val = qRound(r.height()*25.4/72.);
        break;
    case QPaintDevice::PdmNumColors:
        val = INT_MAX;
        break;
    case QPaintDevice::PdmDepth:
        val = 32;
        break;
    default:
        qWarning("QPdfEngine::metric: Invalid metric command");
        return 0;
    }
    return val;
}

QPaintEngine::Type QPdfEngine::type() const
{
    return QPaintEngine::User;
}

bool QPdfEngine::newPage()
{
    if (!isActive())
        return false;
    d->newPage();
    return true;
}

QPdfEnginePrivate::QPdfEnginePrivate()
{
    currentObject = 1;
    width_ = 0;
    height_ = 0;
    streampos = 0;

    currentPage = new QPdfPage;

    stream = new QDataStream;
    pageOrder = QPrinter::FirstPageFirst;
    orientation = QPrinter::Portrait;
    fullPage = false;
}

QPdfEnginePrivate::~QPdfEnginePrivate()
{
    delete currentPage;
    delete stream;
}


int QPdfEnginePrivate::addPenGState(const QPen &pen)
{
    QColor rgba = pen.color();
    if (rgba.alphaF() == 1.)
        return 0;
    QByteArray penDef;
    QPdf::ByteStream s(&penDef);
    s << "<<";
    if (rgba.alphaF() < 1.) 
        s << "/CA " << rgba.alphaF();
    s << ">>\n";

    int penObj = addXrefEntry(-1);
    xprintf(penDef.constData());
    xprintf("endobj\n");

    currentPage->graphicStates.append(penObj);
    return penObj;
}

int QPdfEnginePrivate::addBrushGState(const QBrush &brush)
{
    QColor rgba = brush.color();
    if (rgba.alphaF() == 1.)
        return 0;
    QByteArray brushDef;
    QPdf::ByteStream s(&brushDef);
    s << "<<";
    if (rgba.alphaF() < 1.) 
        s << "/ca " << rgba.alphaF();
    s << ">>\n";

    int brushObj = addXrefEntry(-1);
    xprintf(brushDef.constData());
    xprintf("endobj\n");

    currentPage->graphicStates.append(brushObj);
    return brushObj;
}

int QPdfEnginePrivate::addBrushPattern(const QBrush &b, const QMatrix &m, const QPointF &brushOrigin, bool *specifyColor)
{
    int paintType = 2; // Uncolored tiling
    int w = 8;
    int h = 8;
    *specifyColor = true;
    QMatrix matrix = m;
    matrix.translate(brushOrigin.x(), brushOrigin.y());
    matrix = matrix * QMatrix(1, 0, 0, -1, 0, height_);
    //qDebug() << brushOrigin << matrix;

    int imageObject = 0;
    QByteArray pattern = QPdf::patternForBrush(b);
    if (pattern.isEmpty()) {
        if (b.style() != Qt::TexturePattern)
            return 0;
        QImage image = b.texture().toImage();
        bool bitmap = true;
        imageObject = addImage(image, &bitmap);
        QImage::Format f = image.format();
        if (f != QImage::Format_MonoLSB && f != QImage::Format_Mono) {
            paintType = 1; // Colored tiling
            *specifyColor = false;
        }
        w = image.width();
        h = image.height();
        QMatrix m(w, 0, 0, -h, 0, h);
        QPdf::ByteStream s(&pattern);
        s << QPdf::generateMatrix(m);
        s << "/Im" << imageObject << " Do\n";
    } 
    
    QByteArray str;
    QPdf::ByteStream s(&str);
    s << "<<\n"
        "/Type /Pattern\n"
        "/PatternType 1\n"
        "/PaintType " << paintType << "\n"
        "/TilingType 1\n"
        "/BBox [0 0 " << w << h << "]\n"
        "/XStep " << w << "\n"
        "/YStep " << h << "\n"
        "/Matrix ["
      << matrix.m11()
      << matrix.m12()
      << matrix.m21()
      << matrix.m22()
      << matrix.dx()
      << matrix.dy() << "]\n"
        "/Resources \n<< "; // open resource tree
    if (imageObject) {
        s << "/XObject << /Im" << imageObject << ' ' << imageObject << "0 R >> ";
    }
    s << ">>\n"
        "/Length " << pattern.length() << "\n"
        ">>\n"
        "stream\n"
      << pattern
      << "endstream\n"
        "endobj\n";

    int patternObj = addXrefEntry(-1);
    xprintf(str.constData());
    currentPage->patterns.append(patternObj);
    return patternObj;
}

int QPdfEnginePrivate::addImage(const QImage &img, bool *bitmap)
{
    if (img.isNull())
        return -1;

    QImage image = img;
    QImage::Format format = image.format();
    if (image.depth() == 1 && *bitmap) {
        if (format == QImage::Format_MonoLSB)
            image = image.convertToFormat(QImage::Format_Mono);
    } else {
        *bitmap = false;
        if (format != QImage::Format_RGB32 && format != QImage::Format_ARGB32)
            image = image.convertToFormat(QImage::Format_ARGB32);
    }

    int w = image.width();
    int h = image.height();
    int d = image.depth();

    if (d == 1) {
        int bytesPerLine = (w + 7) >> 3;
        QByteArray data;
        data.resize(bytesPerLine * h);
        char *rawdata = data.data();
        for (int y = 0; y < h; ++y) {
            memcpy(rawdata, image.scanLine(y), bytesPerLine * sizeof(char));
            rawdata += bytesPerLine;
        }
        return writeImage(data, w, h, d, 0, 0);
    }

    QByteArray imageData;
    QByteArray softMaskData;

    imageData.resize(3 * w * h);
    softMaskData.resize(w * h);

    uchar *data = (uchar *)imageData.data();
    uchar *sdata = (uchar *)softMaskData.data();
    bool hasAlpha = false;
    bool hasMask = false;
    for (int y = 0; y < h; ++y) {
        const QRgb *rgb = (const QRgb *)image.scanLine(y);
        for (int x = 0; x < w; ++x) {
            *(data++) = qRed(*rgb);
            *(data++) = qGreen(*rgb);
            *(data++) = qBlue(*rgb);
            uchar alpha = qAlpha(*rgb);
            *sdata++ = alpha;
            hasMask |= (alpha < 255);
            hasAlpha |= (alpha != 0 && alpha != 255);
            ++rgb;
        }
    }
    int maskObject = 0;
    int softMaskObject = 0;
    if (hasAlpha) {
        softMaskObject = writeImage(softMaskData, w, h, 8, 0, 0);
    } else if (hasMask) {
        // dither the soft mask to 1bit and add it. This also helps PDF viewers
        // without transparency support
        int bytesPerLine = (w + 7) >> 3;
        QByteArray mask(bytesPerLine * h, 0);
        uchar *mdata = (uchar *)mask.data();
        const uchar *sdata = (const uchar *)softMaskData.constData();
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                if (*sdata) 
                    mdata[x>>3] |= (0x80 >> (x&7));
                ++sdata;
            }
            mdata += bytesPerLine;
        }
        maskObject = writeImage(mask, w, h, 1, 0, 0);
    }

    return writeImage(imageData, w, h, 32, maskObject, softMaskObject);
}


void QPdfEnginePrivate::newPage()
{
    writePage();
    
    delete currentPage;
    currentPage = new QPdfPage;
    pages.append(requestObject());
        
    *currentPage << "/GSa gs /CSp cs /CSp CS\n";
    QMatrix tmp(1.0, 0.0, 0.0, -1.0, 0.0, height_);
    if (!fullPage)
        tmp.translate(resolution/3, resolution/3);
    *currentPage << QPdf::generateMatrix(tmp) << "q\n";
}


// For strings up to 10000 bytes only !
void QPdfEnginePrivate::xprintf(const char* fmt, ...)
{
    if (!stream)
        return;

    const int msize = 10000;
    static char buf[msize];

    va_list args;
    va_start(args, fmt);
    int bufsize = vsprintf(buf, fmt, args);

    Q_ASSERT(bufsize<msize);

    va_end(args);

    stream->writeRawData(buf, bufsize);
    streampos += bufsize;
}

int QPdfEnginePrivate::writeCompressed(const char *src, int len)
{
#ifndef QT_NO_COMPRESS
    if(do_compress) {
        uLongf destLen = len + len/100 + 13; // zlib requirement
        Bytef* dest = new Bytef[destLen];
        if (Z_OK == ::compress(dest, &destLen, (const Bytef*) src, (uLongf)len)) {
            stream->writeRawData((const char*)dest, destLen);
        } else {
            qWarning("QPdfStream::writeCompressed(): compress error");
            destLen = 0;
        }
        delete [] dest;
        len = destLen;
    } else
#endif
    {
        stream->writeRawData(src,len);
    }
    streampos += len;
    return len;
}


void QPdfEnginePrivate::setDevice(QIODevice* device)
{
    stream->setDevice(device);
    streampos = 0;
}

void QPdfEnginePrivate::unsetDevice()
{
    stream->unsetDevice();
}

int QPdfEnginePrivate::writeImage(const QByteArray &data, int width, int height, int depth,
                                  int maskObject, int softMaskObject)
{
    int image = addXrefEntry(-1);
    xprintf("<<\n"
            "/Type /XObject\n"
            "/Subtype /Image\n"
            "/Width %d\n"
            "/Height %d\n", width, height);

    if (depth == 1) {
        xprintf("/ImageMask true\n"
                "/Decode [1 0]\n");
    } else {
        xprintf("/BitsPerComponent 8\n"
                "/ColorSpace %s\n", (depth == 32) ? "/DeviceRGB" : "/DeviceGray");
    }
    if (maskObject > 0)
        xprintf("/Mask %d 0 R\n", maskObject);
    if (softMaskObject > 0)
        xprintf("/SMask %d 0 R\n", softMaskObject);

    int lenobj = requestObject();
    xprintf("/Length %d 0 R\n", lenobj);
    if (interpolateImages)
        xprintf("/Interpolate true\n");
    if (do_compress)
        xprintf("/Filter /FlateDecode\n");
    xprintf(">>\nstream\n");
    int len = writeCompressed(data);
    xprintf("endstream\n"
            "endobj\n");
    addXrefEntry(lenobj);
    xprintf("%d\n"
            "endobj\n", len);
    return image;
}


void QPdfEnginePrivate::writeHeader()
{
    addXrefEntry(0,false);

    xprintf("%%PDF-1.4\n");

    writeInfo();

    catalog = addXrefEntry(-1);
    pageRoot = requestObject();
    xprintf("<<\n"
            "/Type /Catalog\n"
            "/Pages %d 0 R\n"
            ">>\n"
            "endobj\n", pageRoot);

    // graphics state
    graphicsState = addXrefEntry(-1);
    xprintf("<<\n"
            "/Type /ExtGState\n"
            "/SA true\n"
            "/SM 0.02\n"
            "/ca 1.0\n"
            "/CA 1.0\n"
            "/AIS false\n"
            ">>\n"
            "endobj\n");

    // color spaces
    colorSpace = addXrefEntry(-1);
    xprintf("[ /CalRGB\n"
            "<<\n"
            "/WhitePoint [0.9505 1.0000 1.0890]\n"
            "/Gamma [2.2 2.2 2.2]\n"
            "/Matrix [ 0.4497 0.2446 0.0252 0.3163 0.6720 0.1412 0.1845 0.0833 0.9227]\n"
            //"/Matrix [ 1 0 0 0 1 0 0 0 1]\n"
            ">>\n"
            "]\n"
            "endobj\n");

    colorSpaceGray = addXrefEntry(-1);
    xprintf("[ /CalGray\n"
            "<<\n"
            "/WhitePoint [0.9505 1.0000 1.0890]\n"
            "/Gamma 2.2\n"
            ">>\n"
            "]\n"
            "endobj\n");

    // color space for pattern
    patternColorSpace = addXrefEntry(-1);
    xprintf("[/Pattern %d 0 R]\n"
            "endobj\n", colorSpace);
}

void QPdfEnginePrivate::writeInfo()
{
    time_t now;
    tm *newtime;

    time(&now);
    newtime = gmtime(&now);
    QByteArray y;

    if (newtime && newtime->tm_year+1900 > 1992)
        y += QByteArray::number(newtime->tm_year+1900);

    info = addXrefEntry(-1);
    xprintf("<<\n"
            "/Title (%s)\n"
            "/Author (%s)\n"
            "/Creator (%s)\n"
            "/Producer (Qt %s (C) 1992-%s Trolltech AS)\n",
            title.toUtf8().constData(), author.toUtf8().constData(), creator.toUtf8().constData(),
            qVersion(), y.constData());

    if (newtime) {
        xprintf("/CreationDate (D:%d%02d%02d%02d%02d%02d)\n",
            newtime->tm_year+1900,
            newtime->tm_mon+1,
            newtime->tm_mday,
            newtime->tm_hour,
            newtime->tm_min,
            newtime->tm_sec);
    }
    xprintf(">>\n"
            "endobj\n");
}

void QPdfEnginePrivate::writePageRoot()
{
    addXrefEntry(pageRoot);

    xprintf("<<\n"
            "/Type /Pages\n"
            "/Kids \n"
            "[\n");
    int size = pages.size();
    for (int i = 0; i < size; ++i)
        xprintf("%d 0 R\n", pages[pageOrder == QPrinter::FirstPageFirst ? i : size-i-1]);
    xprintf("]\n");

    //xprintf("/Group <</S /Transparency /I true /K false>>\n");

    xprintf("/Count %d\n"
            "/MediaBox [%d %d %d %d]\n",
            pages.size(), 0, 0, width_, height_);

    xprintf("/ProcSet [/PDF /Text /ImageB /ImageC]\n"
            ">>\n"
            "endobj\n");
}


void QPdfEnginePrivate::writePage()
{
    if (pages.empty())
        return;

    *currentPage << "Q\n";
    
    uint pageStream = requestObject();
    uint pageStreamLength = requestObject();
    uint resources = requestObject();

    addXrefEntry(pages.last());
    xprintf("<<\n"
            "/Type /Page\n"
            "/Parent %d 0 R\n"
            "/Contents %d 0 R\n"
            "/Resources %d 0 R\n"
            ">>\n"
            "endobj\n",
            pageRoot, pageStream, resources);


    addXrefEntry(resources);
    xprintf("<<\n"
            "/ColorSpace <<\n"
            "/PCSp %d 0 R\n"
            "/CSp %d 0 R\n"
            "/CSpg %d 0 R\n"
            "/DefaultRGB %d 0 R\n"
            "/DefaultGray %d 0 R\n"
            ">>\n"
            "/ExtGState <<\n"
            "/GSa %d 0 R\n",
            patternColorSpace, colorSpace, colorSpaceGray, colorSpace, colorSpaceGray, graphicsState);

    for (int i = 0; i < currentPage->graphicStates.size(); ++i) 
        xprintf("/GState%d %d 0 R\n", currentPage->graphicStates.at(i), currentPage->graphicStates.at(i));
    xprintf(">>\n");

    xprintf("/Pattern <<\n");
    for (int i = 0; i < currentPage->patterns.size(); ++i) 
        xprintf("/Pat%d %d 0 R\n", currentPage->patterns.at(i), currentPage->patterns.at(i));
    xprintf(">>\n");

    xprintf("/XObject <<\n");
    for (int i = 0; i<currentPage->images.size(); ++i) {
        xprintf("/Im%d %d 0 R\n", currentPage->images.at(i), currentPage->images.at(i));
    }
    xprintf(">>\n");

    xprintf(">>\n"
            "endobj\n");

    addXrefEntry(pageStream);
    xprintf("<<\n"
            "/Length %d 0 R\n", pageStreamLength); // object number for stream length object
    if (do_compress)
        xprintf("/Filter /FlateDecode\n");

    xprintf(">>\n");
    xprintf("stream\n");
    QByteArray content = currentPage->content();
    int len = writeCompressed(content);
    xprintf("endstream\n"
            "endobj\n");

    addXrefEntry(pageStreamLength);
    xprintf("%d\nendobj\n",len);
}

void QPdfEnginePrivate::writeTail()
{
    writePage();
    writePageRoot();
    addXrefEntry(xrefPositions.size(),false);
    xprintf("xref\n"
            "0 %d\n"
            "%010d 65535 f \n", xrefPositions.size()-1, xrefPositions[0]);

    for (int i = 1; i < xrefPositions.size()-1; ++i)
        xprintf("%010d 00000 n \n", xrefPositions[i]);

    xprintf("trailer\n"
            "<<\n"
            "/Size %d\n"
            "/Info %d 0 R\n"
            "/Root %d 0 R\n"
            ">>\n"
            "startxref\n%d\n"
            "%%%%EOF\n",
            xrefPositions.size()-1, info, catalog, xrefPositions.last());
}

int QPdfEnginePrivate::addXrefEntry(int object, bool printostr)
{
    if (object < 0)
        object = requestObject();

    if (object>=xrefPositions.size())
        xrefPositions.resize(object+1);

    xrefPositions[object] = streampos;
    if (printostr)
        xprintf("%d 0 obj\n",object);

    return object;
}

#endif // QT_NO_PRINTER
