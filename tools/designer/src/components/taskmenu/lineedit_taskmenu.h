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

#ifndef LINEEDIT_TASKMENU_H
#define LINEEDIT_TASKMENU_H

#include <QtGui/QLineEdit>
#include <QtCore/QPointer>

#include <qdesigner_taskmenu.h>
#include <default_extensionfactory.h>

class QLineEdit;
class AbstractFormWindow;

class LineEditTaskMenu: public QDesignerTaskMenu
{
    Q_OBJECT
public:
    LineEditTaskMenu(QLineEdit *button, QObject *parent = 0);
    virtual ~LineEditTaskMenu();

    virtual QList<QAction*> taskActions() const;

private slots:
    void editText();
    void editIcon();
    void updateText(const QString &text);
    void updateSelection();

private:
    QLineEdit *m_lineEdit;
    QPointer<AbstractFormWindow> m_formWindow;
    QPointer<QLineEdit> m_editor;
    mutable QList<QAction*> m_taskActions;
};

class LineEditTaskMenuFactory: public DefaultExtensionFactory
{
    Q_OBJECT
public:
    LineEditTaskMenuFactory(QExtensionManager *extensionManager = 0);

protected:
    virtual QObject *createExtension(QObject *object, const QString &iid, QObject *parent) const;
};

#endif // LINEEDIT_TASKMENU_H
