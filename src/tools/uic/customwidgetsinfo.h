/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of Qt Designer.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef CUSTOMWIDGETSINFO_H
#define CUSTOMWIDGETSINFO_H

#include "treewalker.h"
#include <qstringlist.h>
#include <qmap.h>

class Driver;

class CustomWidgetsInfo : public TreeWalker
{
public:
    CustomWidgetsInfo(Driver *driver);

    void accept(DomUI *node);

    void accept(DomCustomWidgets *node);
    void accept(DomCustomWidget *node);

    inline QStringList customWidgets() const
    { return m_customWidgets.keys(); }

    inline bool hasCustomWidget(const QString &name) const
    { return m_customWidgets.contains(name); }

    inline DomCustomWidget *customWidget(const QString &name) const
    { return m_customWidgets.value(name); }

private:
    Driver *driver;
    QMap<QString, DomCustomWidget*> m_customWidgets;
};

#endif // CUSTOMWIDGETSINFO_H
