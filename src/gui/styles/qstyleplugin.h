/****************************************************************************
**
** Definition of QStylePlugin class.
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QSTYLEPLUGIN_H
#define QSTYLEPLUGIN_H

#ifndef QT_H
#include "qplugin.h"
#include "qfactoryinterface.h"
#endif // QT_H

class QStyle;

struct Q_GUI_EXPORT QStyleFactoryInterface : public QFactoryInterface
{
    virtual QStyle *create(const QString &key) = 0;
};

Q_DECLARE_INTERFACE(QStyleFactoryInterface, "http://trolltech.com/Qt/QStyleFactoryInterface")

class Q_GUI_EXPORT QStylePlugin : public QObject, public QStyleFactoryInterface
{
    Q_OBJECT
    Q_INTERFACES(QStyleFactoryInterface:QFactoryInterface)
public:
    QStylePlugin(QObject *parent = 0);
    ~QStylePlugin();

    virtual QStringList keys() const = 0;
    virtual QStyle *create(const QString &key) = 0;
};

#endif // QSTYLEPLUGIN_H
