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

#ifndef QMACSTYLECG_MAC_H
#define QMACSTYLECG_MAC_H

#ifndef QT_H
#include "qwindowsstyle.h"
#endif // QT_H

#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_3)

#if defined(Q_WS_MAC) && !defined(QT_NO_STYLE_MAC)

class QPalette;

#if defined(QT_PLUGIN)
#define Q_GUI_EXPORT_STYLE_MAC
#else
#define Q_GUI_EXPORT_STYLE_MAC Q_GUI_EXPORT
#endif

class QMacStyleCGPrivate;

class Q_GUI_EXPORT_STYLE_MAC QMacStyleCG : public QWindowsStyle
{
public:
    QMacStyleCG();
    ~QMacStyleCG();

    void polish(QWidget *w);
    void unPolish(QWidget *w);
    void polish(QApplication *app);

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                       const QWidget *w = 0) const;
    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p,
                     const QWidget *w = 0) const;
    QRect subRect(SubRect r, const QStyleOption *opt, const QWidget *widget = 0) const;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p,
                            const QWidget *w = 0) const;
    SubControl querySubControl(ComplexControl cc, const QStyleOptionComplex *opt,
                               const QPoint &pt, const QWidget *w = 0) const;
    QRect querySubControlMetrics(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc,
                                 const QWidget *w = 0) const;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &contentsSize,
                           const QFontMetrics &fm, const QWidget *w = 0) const;

    int pixelMetric(PixelMetric metric, const QWidget *widget) const;

    int styleHint(StyleHint sh, const QWidget *, const Q3StyleOption &, QStyleHintReturn *) const;

    QPixmap stylePixmap(StylePixmap stylepixmap, const QWidget *widget,
                        const Q3StyleOption &opt) const;

    QPixmap stylePixmap(PixmapType pixmaptype, const QPixmap &pixmap,
                        const QPalette &pal, const Q3StyleOption& = Q3StyleOption::Default) const;
private:        // Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QMacStyleCG(const QMacStyleCG &);
    QMacStyleCG &operator=(const QMacStyleCG &);
#endif

protected:
    QMacStyleCGPrivate *d;
};

#endif // Q_WS_MAC

#endif
#endif // QMACSTYLECG_MAC_H
