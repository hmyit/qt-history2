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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QTOOLBARSEPARATOR_P_H
#define QTOOLBARSEPARATOR_P_H

#include <qwidget.h>

class QToolBar;

class QToolBarSeparator : public QWidget
{
    Q_OBJECT
    Qt::Orientation orient;

public:
    QToolBarSeparator(Qt::Orientation orientation, QToolBar *parent);

    QSize sizeHint() const;

    inline Qt::Orientation orientation() const
    { return orient; }
    inline void setOrientation(Qt::Orientation orientation)
    {
        orient = orientation;
        update();
    }

    void paintEvent(QPaintEvent *);
};

#endif // QTOOLBARSEPARATOR_P_H
