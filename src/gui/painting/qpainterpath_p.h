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

#ifndef QPAINTERPATH_P_H
#define QPAINTERPATH_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtGui/qpainterpath.h"
#include "QtGui/qregion.h"
#include "QtCore/qlist.h"

QT_BEGIN_NAMESPACE

class QPolygonF;

class QPainterPathData : public QPainterPathPrivate
{
public:
    QPainterPathData() :
        cStart(0), fillRule(Qt::OddEvenFill),
        dirtyBounds(false), dirtyControlBounds(false)
    {
        ref = 1;
        require_moveTo = false;
    }

    QPainterPathData(const QPainterPathData &other) :
        QPainterPathPrivate(), cStart(other.cStart), fillRule(other.fillRule),
        dirtyBounds(other.dirtyBounds), bounds(other.bounds),
        dirtyControlBounds(other.dirtyControlBounds),
        controlBounds(other.controlBounds)
    {
        ref = 1;
        require_moveTo = false;
        elements = other.elements;
    }

    inline bool isClosed() const;
    inline void close();
    inline void maybeMoveTo();

    int cStart;
    Qt::FillRule fillRule;

    bool require_moveTo;

    bool   dirtyBounds;
    QRectF bounds;
    bool   dirtyControlBounds;
    QRectF controlBounds;
};


void Q_GUI_EXPORT qt_find_ellipse_coords(const QRectF &r, qreal angle, qreal length,
                                         QPointF* startPoint, QPointF *endPoint);

inline bool QPainterPathData::isClosed() const
{
    const QPainterPath::Element &first = elements.at(cStart);
    const QPainterPath::Element &last = elements.last();
    return first.x == last.x && first.y == last.y;
}

inline void QPainterPathData::close()
{
    Q_ASSERT(ref == 1);
    require_moveTo = true;
    const QPainterPath::Element &first = elements.at(cStart);
    const QPainterPath::Element &last = elements.last();
    if (first.x != last.x || first.y != last.y) {
        QPainterPath::Element e = { first.x, first.y, QPainterPath::LineToElement };
        elements << e;
    }
}

inline void QPainterPathData::maybeMoveTo()
{
    if (require_moveTo) {
        QPainterPath::Element e = elements.last();
        e.type = QPainterPath::MoveToElement;
        elements.append(e);
        require_moveTo = false;
    }
}

QT_END_NAMESPACE

#endif // QPAINTERPATH_P_H
