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

#ifndef DEMOWIDGET_H
#define DEMOWIDGET_H

#include "attributes.h"

#include <qwidget.h>
#include <qbasictimer.h>

class DemoWidget : public QWidget
{
public:
    DemoWidget(QWidget *w=0);

    virtual void startAnimation();
    virtual void stopAnimation();

    void setAttributes(Attributes *attr) { attributes = attr; }

    void timerEvent(QTimerEvent *e);
    QSize sizeHint() const;

    void fillBackground(QPainter *p);

    double xfunc(double t);
    double yfunc(double t);

protected:
    int timeoutRate;
    int animationStep;

    Attributes *attributes;

    double a, b, c, d;

private:
    QBasicTimer animationTimer;
};

#endif // DEMOWIDGET_H
