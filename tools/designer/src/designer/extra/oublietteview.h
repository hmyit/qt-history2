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

#ifndef OUBLIETTEVIEW_H
#define OUBLIETTEVIEW_H

#include <QtGui/QScrollArea>

QT_BEGIN_NAMESPACE
class Oubliette;

class OublietteView : public QScrollArea
{
    Q_OBJECT
public:
    OublietteView();
    ~OublietteView();

public slots:
    void scrollToCharacter(const QPoint &pt);

private:
    Oubliette *m_oubliette;
};

QT_END_NAMESPACE

#endif
