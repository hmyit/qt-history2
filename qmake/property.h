/****************************************************************************
**
** Definition of QMakeProperty class.
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of qmake.
** EDITIONS: FREE, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef __PROPERTY_H__
#define __PROPERTY_H__

#include <qstring.h>

class QSettings;

class QMakeProperty
{
    QSettings *sett;
    QString keyBase(bool =true) const;
    bool initSettings();
    QString value(QString, bool just_check);
public:
    QMakeProperty();
    ~QMakeProperty();

    bool hasValue(QString);
    QString value(QString v) { return value(v, false); }
    void setValue(QString, const QString &);

    bool exec();
};

#endif /* __PROPERTY_H__ */
