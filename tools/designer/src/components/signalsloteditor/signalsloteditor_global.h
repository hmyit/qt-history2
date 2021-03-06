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

#ifndef SIGNALSLOTEDITOR_GLOBAL_H
#define SIGNALSLOTEDITOR_GLOBAL_H

#include <QtCore/qglobal.h>

#ifdef Q_OS_WIN
#ifdef QT_SIGNALSLOTEDITOR_LIBRARY
# define QT_SIGNALSLOTEDITOR_EXPORT
#else
# define QT_SIGNALSLOTEDITOR_EXPORT
#endif
#else
#define QT_SIGNALSLOTEDITOR_EXPORT
#endif

#endif // SIGNALSLOTEDITOR_GLOBAL_H
