#ifndef PROPERTYSHEET_H
#define PROPERTYSHEET_H

#include <extension.h>
#include <QStringList>
#include <QVariant>
#include <QMap>

class QVariant;

struct IPropertySheet
{
    virtual ~IPropertySheet() {}

    virtual int count() const = 0;
    
    virtual int indexOf(const QString &name) const = 0;
        
    virtual QString propertyName(int index) const = 0;
    virtual QString propertyGroup(int index) const = 0;
    virtual void setPropertyGroup(int index, const QString &group) = 0;
    
    virtual bool isVisible(int index) const = 0;
    virtual void setVisible(int index, bool b) = 0;

    virtual QVariant property(int index) const = 0;
    virtual void setProperty(int index, const QVariant &value) = 0;
    
    virtual bool isChanged(int index) const = 0;
    virtual void setChanged(int index, bool changed) = 0;
};
Q_DECLARE_EXTENSION_INTERFACE(IPropertySheet, "http://trolltech.com/Qt/IDE/PropertySheet")

struct EnumType
{
    QVariant value;
    QMap<QString, QVariant> items;
};

struct FlagType
{
    QVariant value;
    QMap<QString, QVariant> items;
};

#endif // PROPERTYSHEET_H
