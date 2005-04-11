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

#ifndef TABORDEREDITOR_PLUGIN_H
#define TABORDEREDITOR_PLUGIN_H

#include "tabordereditor_global.h"

#include <QtDesigner/abstractformeditorplugin.h>

#include <QtCore/QPointer>
#include <QtCore/QHash>

class TabOrderEditorTool;
class QDesignerFormWindowInterface;
class QAction;

class QT_TABORDEREDITOR_EXPORT TabOrderEditorPlugin: public QObject, public QDesignerFormEditorPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QDesignerFormEditorPluginInterface)
public:
    TabOrderEditorPlugin();
    virtual ~TabOrderEditorPlugin();

    virtual bool isInitialized() const;
    virtual void initialize(QDesignerFormEditorInterface *core);
    QAction *action() const;

    virtual QDesignerFormEditorInterface *core() const;

public slots:
    void activeFormWindowChanged(QDesignerFormWindowInterface *formWindow);
    
private slots:
    void addFormWindow(QDesignerFormWindowInterface *formWindow);
    void removeFormWindow(QDesignerFormWindowInterface *formWindow);

private:
    QPointer<QDesignerFormEditorInterface> m_core;
    QHash<QDesignerFormWindowInterface*, TabOrderEditorTool*> m_tools;
    bool m_initialized;
    QAction *m_action;
};

#endif // TABORDEREDITOR_PLUGIN_H
