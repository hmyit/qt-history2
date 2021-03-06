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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "shared_global_p.h"

#include <QtCore/QMap>
#include <QtCore/QStringList>
#include <QtCore/QSettings>

QT_BEGIN_NAMESPACE

class QDesignerFormEditorInterface;
class QDesignerCustomWidgetInterface;

class QDESIGNER_SHARED_EXPORT QDesignerPluginManager: public QObject
{
    Q_OBJECT
public:
    explicit QDesignerPluginManager(QDesignerFormEditorInterface *core);
    virtual ~QDesignerPluginManager();

    QDesignerFormEditorInterface *core() const;

    QObject *instance(const QString &plugin) const;

    QStringList registeredPlugins() const;

    QStringList findPlugins(const QString &path);

    QStringList pluginPaths() const;
    void setPluginPaths(const QStringList &plugin_paths);

    QStringList disabledPlugins() const;
    void setDisabledPlugins(const QStringList &disabled_plugins);

    QStringList failedPlugins() const;
    QString failureReason(const QString &pluginName) const;

    QList<QObject*> instances() const;
    QList<QDesignerCustomWidgetInterface*> registeredCustomWidgets() const;

    bool registerNewPlugins();

public slots:
    bool syncSettings();
    void ensureInitialized();

private:
    void updateRegisteredPlugins();
    void registerPath(const QString &path);
    void registerPlugin(const QString &plugin);

private:
    QDesignerFormEditorInterface *m_core;
    QStringList m_pluginPaths;
    QStringList m_registeredPlugins;
    QStringList m_disabledPlugins;

    typedef QMap<QString, QString> FailedPluginMap;
    FailedPluginMap m_failedPlugins;

    typedef QList<QDesignerCustomWidgetInterface*> CustomWidgetList;
    CustomWidgetList m_customWidgets;

    QStringList defaultPluginPaths() const;
};

QT_END_NAMESPACE

#endif // PLUGINMANAGER_H
