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

#ifndef QFOCUSFRAME_H
#define QFOCUSFRAME_H

#include <QtGui/qwidget.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QFocusFramePrivate;
class QStyleOption;

class Q_GUI_EXPORT QFocusFrame : public QWidget
{
    Q_OBJECT
public:
    QFocusFrame(QWidget *parent=0);
    ~QFocusFrame();

    void setWidget(QWidget *widget);
    QWidget *widget() const;

protected:
    bool event(QEvent *e);

    bool eventFilter(QObject *, QEvent *);
    void paintEvent(QPaintEvent *);
    void initStyleOption(QStyleOption *option) const;

private:
    Q_DECLARE_PRIVATE(QFocusFrame)
    Q_DISABLE_COPY(QFocusFrame)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QFOCUSFRAME_H
