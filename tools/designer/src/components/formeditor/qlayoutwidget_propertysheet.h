#ifndef QLAYOUTWIDGET_PROPERTYSHEET_H
#define QLAYOUTWIDGET_PROPERTYSHEET_H

#include "default_propertysheet.h"

class QLayoutWidget;

class QLayoutWidgetPropertySheet: public QDesignerPropertySheet
{
    Q_OBJECT
    Q_INTERFACES(IPropertySheet)
public:
    QLayoutWidgetPropertySheet(QLayoutWidget *object, QObject *parent = 0);
    virtual ~QLayoutWidgetPropertySheet();

    virtual void setProperty(int index, const QVariant &value);
};

class QLayoutWidgetPropertySheetFactory: public DefaultExtensionFactory
{
    Q_OBJECT
    Q_INTERFACES(ExtensionFactory)
public:
    QLayoutWidgetPropertySheetFactory(QExtensionManager *parent = 0);

protected:
    virtual QObject *createExtension(QObject *object, const QString &iid, QObject *parent) const;
};

#endif // QLAYOUTWIDGET_PROPERTYSHEET_H
