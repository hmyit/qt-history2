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

#include <qplatformdefs.h>
#include "qitemeditorfactory.h"
#include <qcombobox.h>
#include <qdatetimeedit.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <limits.h>

/*!
    \class QItemEditorFactory
    \brief The QItemEditorFactory class provides widgets for editing item data
    in views.
*/

/*!
    \fn QItemEditorFactory::QItemEditorFactory()

    Constructs a new item editor factory.
*/

/*!
Creates an editor widget with the given \a parent for the specified \a type of data,
and returns it as a QWidget.

\sa registerEditor()*/
QWidget *QItemEditorFactory::createEditor(QVariant::Type type, QWidget *parent) const
{
    QItemEditorCreatorBase *creator = creatorMap.value(type, 0);
    if (!creator)
        return defaultFactory()->createEditor(type, parent);
    return creator->createWidget(parent);
}

/*!
Returns the property name used to identify the given \a type of data. */
QByteArray QItemEditorFactory::valuePropertyName(QVariant::Type type) const
{
    QItemEditorCreatorBase *creator = creatorMap.value(type, 0);
    if (!creator)
        return defaultFactory()->valuePropertyName(type);
    return creator->valuePropertyName();
}

/*!
Destroys the item editor factory.*/
QItemEditorFactory::~QItemEditorFactory()
{

}

/*!
Registers an item editor creator specified by \a creator for the given \a type of data.

\sa createEditor()*/
void QItemEditorFactory::registerEditor(QVariant::Type type, QItemEditorCreatorBase *creator)
{
   delete creatorMap.value(type, 0);
   creatorMap[type] = creator;
}

class QDefaultItemEditorFactory: public QItemEditorFactory
{
public:
    inline QDefaultItemEditorFactory() {}
    QWidget *createEditor(QVariant::Type type, QWidget *parent) const;
    QByteArray valuePropertyName(QVariant::Type) const;
};

QWidget *QDefaultItemEditorFactory::createEditor(QVariant::Type type, QWidget *parent) const
{
    switch (type) {
    case QVariant::Bool: {
        QComboBox *cb = new QComboBox(parent);
        cb->setFrame(false);
        cb->addItem("False");
        cb->addItem("True");
        return cb; }
    case QVariant::UInt: {
        QSpinBox *sb = new QSpinBox(parent);
        sb->setFrame(false);
        sb->setMaximum(INT_MAX);
        return sb; }
    case QVariant::Int: {
        QSpinBox *sb = new QSpinBox(parent);
        sb->setFrame(false);
        sb->setMinimum(INT_MIN);
        sb->setMaximum(INT_MAX);
        return sb; }
    case QVariant::Date: {
        QDateTimeEdit *ed = new QDateEdit(parent);
        ed->setFrame(false);
        return ed; }
    case QVariant::Time: {
        QDateTimeEdit *ed = new QTimeEdit(parent);
        ed->setFrame(false);
        return ed; }
    case QVariant::DateTime: {
        QDateTimeEdit *ed = new QDateTimeEdit(parent);
        ed->setFrame(false);
        return ed; }
    case QVariant::Pixmap:
        return new QLabel(parent);
    case QVariant::Double: {
        QDoubleSpinBox *sb = new QDoubleSpinBox(parent);
        sb->setFrame(false);
        return sb; }
    case QVariant::StringList: {
        QComboBox *cb = new QComboBox(parent);
        cb->setFrame(false);
        return cb; }
    case QVariant::String:
    default: {
        // the default editor is a lineedit
        QLineEdit *le = new QLineEdit(parent);
        le->setFrame(false);
        return le; }
    }
}

QByteArray QDefaultItemEditorFactory::valuePropertyName(QVariant::Type type) const
{
    switch (type) {
    case QVariant::Bool:
        return "currentItem";
    case QVariant::UInt:
    case QVariant::Int:
    case QVariant::Double:
        return "value";
    case QVariant::Date:
        return "date";
    case QVariant::Time:
        return "time";
    case QVariant::DateTime:
        return "dateTime";
    case QVariant::StringList:
        return "contents";
    case QVariant::String:
    default:
        // the default editor is a lineedit
        return "text";
    }
}

static QItemEditorFactory *q_default_factory = 0;
struct QDefaultFactoryCleaner
{
    inline QDefaultFactoryCleaner() {}
    ~QDefaultFactoryCleaner() { delete q_default_factory; q_default_factory = 0; }
};

/*!
Returns the default item editor factory.

\sa setDefaultFactory()*/
const QItemEditorFactory *QItemEditorFactory::defaultFactory()
{
    static const QDefaultItemEditorFactory factory;
    if (q_default_factory)
        return q_default_factory;
    return &factory;
}

/*!
Sets the default item editor factory to the given \a factory.

\sa defaultFactory()*/
void QItemEditorFactory::setDefaultFactory(QItemEditorFactory *factory)
{
    static const QDefaultFactoryCleaner cleaner;
    delete q_default_factory;
    q_default_factory = factory;
}
