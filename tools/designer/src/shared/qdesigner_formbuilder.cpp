
#include "qdesigner_formbuilder.h"

#include <container.h>
#include <customwidget.h>
#include <pluginmanager.h>
#include <qextensionmanager.h>
#include <abstractformeditor.h>

#include <QWidget>

QDesignerFormBuilder::QDesignerFormBuilder(AbstractFormEditor *core)
    : m_core(core)
{
    Q_ASSERT(m_core);
    
    PluginManager pluginManager;
    
    m_customFactory.clear();
    QStringList plugins = pluginManager.registeredPlugins();
    
    foreach (QString plugin, plugins) {
        QObject *o = pluginManager.instance(plugin);
        
        if (ICustomWidget *c = qt_cast<ICustomWidget*>(o)) {
            if (!c->isInitialized())
                c->initialize(m_core);
                
            m_customFactory.insert(c->name(), c);
        }
    }
}

QWidget *QDesignerFormBuilder::createWidget(const QString &widgetName, QWidget *parentWidget, const QString &name)
{
    if (ICustomWidget *c = m_customFactory.value(widgetName)) {
        QWidget *widget = c->createWidget(parentWidget);
        widget->setObjectName(name);
        return widget;
    }
    
    return FormBuilder::createWidget(widgetName, parentWidget, name);
}

bool QDesignerFormBuilder::addItem(DomWidget *ui_widget, QWidget *widget, QWidget *parentWidget)
{
    if (FormBuilder::addItem(ui_widget, widget, parentWidget))
        return true;
        
    if (IContainer *container = qt_extension<IContainer*>(m_core->extensionManager(), parentWidget)) {
        container->addWidget(widget);
        return true;
    }
    
    return false;
}
