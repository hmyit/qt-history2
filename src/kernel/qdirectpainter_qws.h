/****************************************************************************
**
** Definition of QDirectPainter class.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QDIRECTPAINTER_QWS_H
#define QDIRECTPAINTER_QWS_H

#ifndef QT_H
#include "qpainter.h"
#endif // QT_H

#ifdef Q_WS_QWS
#ifndef QT_NO_DIRECTPAINTER
class Q_EXPORT QDirectPainter : public QPainter {
public:
    QDirectPainter( const QWidget* );
    ~QDirectPainter();

    uchar* frameBuffer();
    int lineStep();
    int transformOrientation();

    int numRects() const;
    const QRect& rect(int i) const;
    QRegion region() const;

    int depth() const;
    int width() const;
    int height() const;
    int xOffset() const;
    int yOffset() const;

    QPoint offset() const;
    QSize size() const;

    void setAreaChanged( const QRect& );

private:
    class Private;
    Private* d;
};

#endif
#endif

#endif // QDIRECTPAINTER_QWS_H
