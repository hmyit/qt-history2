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

#ifndef QACTION_P_H
#define QACTION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtGui/qaction.h"
#include "QtGui/qmenu.h"
#include "private/qobject_p.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_ACTION

#ifdef QT3_SUPPORT
class QMenuItemEmitter;
#endif

class QShortcutMap;

class Q_AUTOTEST_EXPORT QActionPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QAction)
public:
    QActionPrivate();
    ~QActionPrivate();

    QPointer<QActionGroup> group;
    QString text;
    QString iconText;
    QIcon icon;
    QString tooltip;
    QString statustip;
    QString whatsthis;
#ifndef QT_NO_SHORTCUT
    QKeySequence shortcut;
    QList<QKeySequence> alternateShortcuts;
#endif
    QVariant userData;
#ifndef QT_NO_SHORTCUT
    int shortcutId;
    QList<int> alternateShortcutIds;
    Qt::ShortcutContext shortcutContext;
    uint autorepeat : 1;
#endif
    QFont font;
    QPointer<QMenu> menu;
    uint enabled : 1, forceDisabled : 1;
    uint visible : 1, forceInvisible : 1;
    uint checkable : 1;
    uint checked : 1;
    uint separator : 1;
    uint fontSet : 1;
    QAction::MenuRole menuRole;
    int iconVisibleInMenu : 3;  // Only has values -1, 0, and 1
    QList<QWidget *> widgets;
#ifndef QT_NO_SHORTCUT
    void redoGrab(QShortcutMap &map);
    void redoGrabAlternate(QShortcutMap &map);
    void setShortcutEnabled(bool enable, QShortcutMap &map);

    static QShortcutMap *globalMap;
#endif // QT_NO_SHORTCUT

#ifdef QT3_SUPPORT //for menubar/menu compat
    QMenuItemEmitter *act_signal;
    int id, param;
#endif
    void sendDataChanged();
};

#endif // QT_NO_ACTION

QT_END_NAMESPACE

#endif // QACTION_P_H
