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

#ifndef QHIVIEWWIDGET_P_H
#define QHIVIEWWIDGET_P_H

#include <QtGui/qapplication.h>
#include <QtGui/qwidget.h>
#include <private/qt_mac_p.h>

QT_BEGIN_NAMESPACE

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

class Q_GUI_EXPORT QHIViewWidget : public QWidget
{
    Q_OBJECT
public:
    QHIViewWidget(WindowRef windowref, bool createSubs=true, QWidget *parent=0, Qt::WindowFlags flags = 0);
    QHIViewWidget(HIViewRef hiviewref, bool createSubs=true, QWidget *parent=0, Qt::WindowFlags flags = 0);
    ~QHIViewWidget();

private:
    void createQWidgetsFromHIViews();
    void addViews_recursive(HIViewRef child, QWidget *parent);
};

QT_END_NAMESPACE

#endif // QHIVIEWWIDGET_P_H
