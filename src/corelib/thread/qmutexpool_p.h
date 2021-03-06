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

#ifndef QMUTEXPOOL_P_H
#define QMUTEXPOOL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of QSettings. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qmutex.h"

#ifndef QT_NO_THREAD

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QMutexPool
{
public:
    explicit QMutexPool(bool recursive = false, int size = 128);
    ~QMutexPool();

    QMutex *get(const void *address);
    static QMutexPool *instance();
    static QMutex *globalInstanceGet(const void *address);

private:
    QMutex mutex;
    QMutex **mutexes;
    int count;
    bool recurs;
};

extern Q_CORE_EXPORT QMutexPool *qt_global_mutexpool;

QT_END_NAMESPACE

#endif // QT_NO_THREAD

#endif // QMUTEXPOOL_P_H
