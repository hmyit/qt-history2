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

#ifndef QTEXTCODECFACTORY_H
#define QTEXTCODECFACTORY_H

#ifndef QT_H
#include "qstringlist.h"
#endif // QT_H

class QTextCodec;

class Q_CORE_EXPORT QTextCodecFactory
{
public:
    static QTextCodec *createForName(const QString &);
    static QTextCodec *createForMib(int);
};

#endif // QTEXTCODECFACTORY_H
