#ifndef QDESIGNER_FORMBUILDER_H
#define QDESIGNER_FORMBUILDER_H

#include "shared_global.h"

#include <formbuilder.h>
#include <QMap>

class AbstractFormEditor;
struct ICustomWidget;

class QT_SHARED_EXPORT QDesignerFormBuilder: public FormBuilder
{
public:
    QDesignerFormBuilder(AbstractFormEditor *core);

    virtual QWidget *createWidget(DomWidget *ui_widget, QWidget *parentWidget = 0)
        { return FormBuilder::create(ui_widget, parentWidget); }
        
    inline AbstractFormEditor *core() const
    { return m_core; }
    
protected:
    virtual QWidget *createWidget(const QString &widgetName, QWidget *parentWidget, const QString &name);
    virtual bool addItem(DomWidget *ui_widget, QWidget *widget, QWidget *parentWidget);
    
private:
    AbstractFormEditor *m_core;
    QMap<QString, ICustomWidget*> m_customFactory;
};

#endif // QDESIGNER_FORMBUILDER_H
