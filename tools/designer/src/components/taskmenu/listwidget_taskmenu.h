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

#ifndef LISTWIDGET_TASKMENU_H
#define LISTWIDGET_TASKMENU_H

#include <QtGui/QListWidget>
#include <QtCore/QPointer>

#include <qdesigner_taskmenu.h>
#include <default_extensionfactory.h>

class QLineEdit;
class AbstractFormWindow;

class ListWidgetTaskMenu: public QDesignerTaskMenu
{
    Q_OBJECT
public:
    ListWidgetTaskMenu(QListWidget *button, QObject *parent = 0);
    virtual ~ListWidgetTaskMenu();

    virtual QList<QAction*> taskActions() const;

private slots:
    void editItems();
    void updateSelection();

private:
    QListWidget *m_listWidget;
    QPointer<AbstractFormWindow> m_formWindow;
    QPointer<QLineEdit> m_editor;
    mutable QList<QAction*> m_taskActions;
};

class ListWidgetTaskMenuFactory: public DefaultExtensionFactory
{
    Q_OBJECT
public:
    ListWidgetTaskMenuFactory(QExtensionManager *extensionManager = 0);

protected:
    virtual QObject *createExtension(QObject *object, const QString &iid, QObject *parent) const;
};

#endif // LISTWIDGET_TASKMENU_H
