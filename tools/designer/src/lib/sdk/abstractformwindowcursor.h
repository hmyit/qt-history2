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

#ifndef ABSTRACTFORMWINDOWCURSOR_H
#define ABSTRACTFORMWINDOWCURSOR_H

#include <QtDesigner/sdk_global.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QDesignerFormWindowInterface;
class QWidget;
class QVariant;
class QString;

class QDESIGNER_SDK_EXPORT QDesignerFormWindowCursorInterface
{
public:
    enum MoveOperation
    {
        NoMove,

        Start,
        End,
        Next,
        Prev,
        Left,
        Right,
        Up,
        Down
    };

    enum MoveMode
    {
        MoveAnchor,
        KeepAnchor
    };

public:
    virtual ~QDesignerFormWindowCursorInterface() {}

    virtual QDesignerFormWindowInterface *formWindow() const = 0;

    virtual bool movePosition(MoveOperation op, MoveMode mode = MoveAnchor) = 0;

    virtual int position() const = 0;
    virtual void setPosition(int pos, MoveMode mode = MoveAnchor) = 0;

    virtual QWidget *current() const = 0;

    virtual int widgetCount() const = 0;
    virtual QWidget *widget(int index) const = 0;

    virtual bool hasSelection() const = 0;
    virtual int selectedWidgetCount() const = 0;
    virtual QWidget *selectedWidget(int index) const = 0;

    virtual void setProperty(const QString &name, const QVariant &value) = 0;
    virtual void setWidgetProperty(QWidget *widget, const QString &name, const QVariant &value) = 0;
    virtual void resetWidgetProperty(QWidget *widget, const QString &name) = 0;

    bool isWidgetSelected(QWidget *widget) const;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // ABSTRACTFORMWINDOWCURSOR_H
