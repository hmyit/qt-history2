#if 0
/****************************************************************************
**
** Definition of internal QPSPrinter class.
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QPSPRINTER_P_H
#define QPSPRINTER_P_H


//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qpsprinter.cpp and qprinter_x11.cpp.
// This header file may change from version to version without notice,
// or even be removed.
//
// We mean it.
//
//


#ifndef QT_H
#include "qpaintengine.h"
#endif // QT_H

#ifndef QT_NO_PRINTER

class QPrinter;
class QPSPrinterPrivate;

class Q_GUI_EXPORT QPSPrinter : public QPaintEngine
{
private:
    // QPrinter uses these
    QPSPrinter(QPrinter *, int);
   ~QPSPrinter();

    virtual bool begin(QPaintDevice *pdev);
    virtual bool end();

    void updatePen(const QPen &pen);
    void updateBrush(const QBrush &brush, const QPoint &pt);
    void updateFont(const QFont &font);
    void updateBackground(Qt::BGMode bgmode, const QBrush &bgBrush);
    void updateXForm(const QWMatrix &matrix);
    void updateClipRegion(const QRegion &region, bool clipEnabled);

    virtual void drawLine(const QPoint &p1, const QPoint &ps);
    virtual void drawRect(const QRect &r);
    virtual void drawPoint(const QPoint &p);
    virtual void drawPoints(const QPointArray &pa, int index = 0, int npoints = -1);
    virtual void drawRoundRect(const QRect &r, int xRnd, int yRnd);
    virtual void drawEllipse(const QRect &r);
    virtual void drawArc(const QRect &r, int a, int alen);
    virtual void drawPie(const QRect &r, int a, int alen);
    virtual void drawChord(const QRect &r, int a, int alen);
    virtual void drawLineSegments(const QPointArray &, int index = 0, int nlines = -1);
    virtual void drawPolyline(const QPointArray &pa, int index = 0, int npoints = -1);
    virtual void drawPolygon(const QPointArray &pa, bool winding = false, int index = 0, int npoints = -1);
    virtual void drawConvexPolygon(const QPointArray &, int index = 0, int npoints = -1);
#ifndef QT_NO_BEZIER
    virtual void drawCubicBezier(const QPointArray &, int index = 0);
#endif

    virtual void drawPixmap(const QRect &r, const QPixmap &pm, const QRect &sr, bool imask);
    virtual void drawTextItem(const QPoint &p, const QTextItem &ti, int textflags);
    virtual void drawTiledPixmap(const QRect &r, const QPixmap &pixmap, const QPoint &s, bool optim);

    virtual Qt::HANDLE handle() const { return 0; }
    virtual QPaintEngine::Type type() const { return QPaintEngine::PostScript; }


    void newPage();
    void abort();

    virtual bool updateState();

    friend class QPrinter;
private:
    // not used by QPrinter
    QPSPrinterPrivate *d;

    // Disabled copy constructor and operator=
    QPSPrinter(const QPSPrinter &);
    QPSPrinter &operator=(const QPSPrinter &);
};

#endif // QT_NO_PRINTER

#endif // QPSPRINTER_P_H
#endif // 0
