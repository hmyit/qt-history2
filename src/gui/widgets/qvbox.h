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

#ifndef QVBOX_H
#define QVBOX_H

#ifndef QT_H
#include "qhbox.h"
#endif // QT_H

#ifndef QT_NO_VBOX

class Q_GUI_EXPORT QVBox : public QHBox
{
    Q_OBJECT
public:
#ifdef QT_COMPAT
    QT_COMPAT_CONSTRUCTOR QVBox(QWidget* parent, const char* name, Qt::WFlags f=0);
#endif
    QVBox(QWidget* parent=0, Qt::WFlags f=0);

private:        // Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QVBox(const QVBox &);
    QVBox& operator=(const QVBox &);
#endif
};

#endif // QT_NO_VBOX

#endif // QVBOX_H
