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

#include "widgetdatabase_p.h"
#include "widgetfactory_p.h"
#include "spacer_widget_p.h"
#include "abstractlanguage.h"
#include "pluginmanager_p.h"
#include "qdesigner_utils_p.h"

#include <QtDesigner/customwidget.h>
#include <QtDesigner/propertysheet.h>
#include <QtDesigner/QExtensionManager>
#include <QtDesigner/QDesignerFormEditorInterface>

#include <qalgorithms.h>
#include <QtCore/qdebug.h>
#include <QtCore/QMetaProperty>

QT_BEGIN_NAMESPACE

namespace {
    enum { debugWidgetDataBase = 0 };
}

namespace qdesigner_internal {

// ----------------------------------------------------------
WidgetDataBaseItem::WidgetDataBaseItem(const QString &name, const QString &group)
    : m_name(name),
      m_group(group),
      m_compat(0),
      m_container(0),
      m_form(0),
      m_custom(0),
      m_promoted(0)
{
}

QString WidgetDataBaseItem::name() const
{
    return m_name;
}

void WidgetDataBaseItem::setName(const QString &name)
{
    m_name = name;
}

QString WidgetDataBaseItem::group() const
{
    return m_group;
}

void WidgetDataBaseItem::setGroup(const QString &group)
{
    m_group = group;
}

QString WidgetDataBaseItem::toolTip() const
{
    return m_toolTip;
}

void WidgetDataBaseItem::setToolTip(const QString &toolTip)
{
    m_toolTip = toolTip;
}

QString WidgetDataBaseItem::whatsThis() const
{
    return m_whatsThis;
}

void WidgetDataBaseItem::setWhatsThis(const QString &whatsThis)
{
    m_whatsThis = whatsThis;
}

QString WidgetDataBaseItem::includeFile() const
{
    return m_includeFile;
}

void WidgetDataBaseItem::setIncludeFile(const QString &includeFile)
{
    m_includeFile = includeFile;
}

QIcon WidgetDataBaseItem::icon() const
{
    return m_icon;
}

void WidgetDataBaseItem::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

bool WidgetDataBaseItem::isCompat() const
{
    return m_compat;
}

void WidgetDataBaseItem::setCompat(bool b)
{
    m_compat = b;
}

bool WidgetDataBaseItem::isContainer() const
{
    return m_container;
}

void WidgetDataBaseItem::setContainer(bool b)
{
    m_container = b;
}

bool WidgetDataBaseItem::isCustom() const
{
    return m_custom;
}

void WidgetDataBaseItem::setCustom(bool b)
{
    m_custom = b;
}

QString WidgetDataBaseItem::pluginPath() const
{
    return m_pluginPath;
}

void WidgetDataBaseItem::setPluginPath(const QString &path)
{
    m_pluginPath = path;
}

bool WidgetDataBaseItem::isPromoted() const
{
    return m_promoted;
}

void WidgetDataBaseItem::setPromoted(bool b)
{
    m_promoted = b;
}

QString WidgetDataBaseItem::extends() const
{
    return m_extends;
}

void WidgetDataBaseItem::setExtends(const QString &s)
{
    m_extends = s;
}

void WidgetDataBaseItem::setDefaultPropertyValues(const QList<QVariant> &list)
{
    m_defaultPropertyValues = list;
}

QList<QVariant> WidgetDataBaseItem::defaultPropertyValues() const
{
    return m_defaultPropertyValues;
}

QStringList WidgetDataBaseItem::fakeSlots() const
{
    return m_fakeSlots;
}

void WidgetDataBaseItem::setFakeSlots(const QStringList &fs)
{
    m_fakeSlots = fs;
}

QStringList WidgetDataBaseItem::fakeSignals() const
{
     return m_fakeSignals;
}

void WidgetDataBaseItem::setFakeSignals(const QStringList &fs)
{
    m_fakeSignals = fs;
}

WidgetDataBaseItem *WidgetDataBaseItem::clone(const QDesignerWidgetDataBaseItemInterface *item)
{
    WidgetDataBaseItem *rc = new WidgetDataBaseItem(item->name(), item->group());

    rc->setToolTip(item->toolTip());
    rc->setWhatsThis(item->whatsThis());
    rc->setIncludeFile(item->includeFile());
    rc->setIcon(item->icon());
    rc->setCompat(item->isCompat());
    rc->setContainer(item->isContainer());
    rc->setCustom(item->isCustom() );
    rc->setPluginPath(item->pluginPath());
    rc->setPromoted(item->isPromoted());
    rc->setExtends(item->extends());
    rc->setDefaultPropertyValues(item->defaultPropertyValues());
    // fake slots and signals ignored here.

    return rc;
}

// ----------------------------------------------------------
WidgetDataBase::WidgetDataBase(QDesignerFormEditorInterface *core, QObject *parent)
    : QDesignerWidgetDataBaseInterface(parent),
      m_core(core)
{
#define DECLARE_LAYOUT(L, C)
#define DECLARE_COMPAT_WIDGET(W, C) DECLARE_WIDGET(W, C)
#define DECLARE_WIDGET(W, C) append(new WidgetDataBaseItem(QString::fromUtf8(#W)));

#include "widgets.table"

#undef DECLARE_COMPAT_WIDGET
#undef DECLARE_LAYOUT
#undef DECLARE_WIDGET
#undef DECLARE_WIDGET_1

    append(new WidgetDataBaseItem(QString::fromUtf8("Line")));
    append(new WidgetDataBaseItem(QString::fromUtf8("Spacer")));
    append(new WidgetDataBaseItem(QString::fromUtf8("QSplitter")));
    append(new WidgetDataBaseItem(QString::fromUtf8("QLayoutWidget")));
    // QDesignerWidget is used as central widget and as container for tab widgets, etc.
    WidgetDataBaseItem *designerWidgetItem = new WidgetDataBaseItem(QString::fromUtf8("QDesignerWidget"));
    designerWidgetItem->setContainer(true);
    append(designerWidgetItem);
    append(new WidgetDataBaseItem(QString::fromUtf8("QDesignerDialog")));
    append(new WidgetDataBaseItem(QString::fromUtf8("QDesignerMenu")));
    append(new WidgetDataBaseItem(QString::fromUtf8("QDesignerMenuBar")));
    append(new WidgetDataBaseItem(QString::fromUtf8("QDesignerDockWidget")));
    append(new WidgetDataBaseItem(QString::fromUtf8("QDesignerQ3WidgetStack")));
    append(new WidgetDataBaseItem(QString::fromUtf8("QAction")));

    // ### remove me
    // ### check the casts

#if 0 // ### enable me after 4.1
    item(indexOfClassName(QLatin1String("QToolBar")))->setContainer(true);
#endif

    item(indexOfClassName(QLatin1String("QTabWidget")))->setContainer(true);
    item(indexOfClassName(QLatin1String("QGroupBox")))->setContainer(true);
    item(indexOfClassName(QLatin1String("QScrollArea")))->setContainer(true);
    item(indexOfClassName(QLatin1String("QStackedWidget")))->setContainer(true);
    item(indexOfClassName(QLatin1String("QToolBox")))->setContainer(true);
    item(indexOfClassName(QLatin1String("QFrame")))->setContainer(true);
    item(indexOfClassName(QLatin1String("QLayoutWidget")))->setContainer(true);
    item(indexOfClassName(QLatin1String("QDesignerWidget")))->setContainer(true);
    item(indexOfClassName(QLatin1String("QDesignerDialog")))->setContainer(true);
    item(indexOfClassName(QLatin1String("QSplitter")))->setContainer(true);
    item(indexOfClassName(QLatin1String("QMainWindow")))->setContainer(true);
    item(indexOfClassName(QLatin1String("QDockWidget")))->setContainer(true);
    item(indexOfClassName(QLatin1String("QDesignerDockWidget")))->setContainer(true);
    item(indexOfClassName(QLatin1String("QDesignerQ3WidgetStack")))->setContainer(true);

    item(indexOfClassName(QLatin1String("QWidget")))->setContainer(true);
    item(indexOfClassName(QLatin1String("QDialog")))->setContainer(true);
}

WidgetDataBase::~WidgetDataBase()
{
}

QDesignerFormEditorInterface *WidgetDataBase::core() const
{
    return m_core;
}

int WidgetDataBase::indexOfObject(QObject *object, bool /*resolveName*/) const
{
    QExtensionManager *mgr = m_core->extensionManager();
    QDesignerLanguageExtension *lang = qt_extension<QDesignerLanguageExtension*> (mgr, m_core);

    QString id;

    if (lang)
        id = lang->classNameOf(object);

    if (id.isEmpty())
        id = WidgetFactory::classNameOf(m_core,object);

    return QDesignerWidgetDataBaseInterface::indexOfClassName(id);
}

void WidgetDataBase::loadPlugins()
{
    typedef QMap<QString, int> NameIndexMap;
    typedef QList<QDesignerWidgetDataBaseItemInterface*> ItemList;
    typedef QMap<QString, QDesignerWidgetDataBaseItemInterface*> NameItemMap;
    typedef QSet<QString> NameSet;
    // 1) create a map of existing custom classes
    NameIndexMap existingCustomClasses;
    NameSet nonCustomClasses;
    const int count = m_items.size();
    for (int i = 0; i < count; i++)    {
        const QDesignerWidgetDataBaseItemInterface* item =  m_items[i];
        if (item->isCustom() && !item->isPromoted())
            existingCustomClasses.insert(item->name(), i);
        else
            nonCustomClasses.insert(item->name());
    }
    // 2) create a list map of plugins and the map for the factory
    ItemList pluginList;
    QDesignerPluginManager *pluginManager = m_core->pluginManager();
    const QStringList plugins = pluginManager->registeredPlugins();
    pluginManager->ensureInitialized();
    foreach (QString plugin, plugins) {
        QObject *o = pluginManager->instance(plugin);
        if (QDesignerCustomWidgetInterface *c = qobject_cast<QDesignerCustomWidgetInterface*>(o)) {
            pluginList += createCustomWidgetItem(c, plugin);
        } else {
            if (QDesignerCustomWidgetCollectionInterface *coll = qobject_cast<QDesignerCustomWidgetCollectionInterface*>(o)) {
                foreach (QDesignerCustomWidgetInterface *c, coll->customWidgets()) {
                    pluginList += createCustomWidgetItem(c, plugin);
                }
            }
        }
    }
    // 3) replace custom classes or add new ones, remove them from existingCustomClasses,
    // leaving behind deleted items
    unsigned replacedPlugins = 0;
    unsigned addedPlugins = 0;
    unsigned removedPlugins = 0;
    if (!pluginList.empty()) {
        ItemList::const_iterator cend = pluginList.constEnd();
        for (ItemList::const_iterator it = pluginList.constBegin();it != cend; ++it )  {
            QDesignerWidgetDataBaseItemInterface* pluginItem = *it;
            const QString pluginName = pluginItem->name();
            NameIndexMap::iterator existingIt = existingCustomClasses.find(pluginName);
            if (existingIt == existingCustomClasses.end()) {
                // Add new class.
                if (nonCustomClasses.contains(pluginName)) {
                    designerWarning(QObject::tr("A custom widget plugin whose class name (%1) matches that of an existing class has been found.").arg(pluginName));
                } else {
                    append(pluginItem);
                    addedPlugins++;
                }
            } else {
                // replace existing info
                const int existingIndex = existingIt.value();
                delete m_items[existingIndex];
                m_items[existingIndex] = pluginItem;
                existingCustomClasses.erase(existingIt);
                replacedPlugins++;

            }
        }
    }
    // 4) remove classes that have not been matched. The stored indexes become invalid while deleting.
    if (!existingCustomClasses.empty()) {
        NameIndexMap::const_iterator cend = existingCustomClasses.constEnd();
        for (NameIndexMap::const_iterator it = existingCustomClasses.constBegin();it != cend; ++it )  {
            const int index = indexOfClassName(it.key());
            if (index != -1) {
                remove(index);
                removedPlugins++;
            }
        }
    }
    if (debugWidgetDataBase)
        qDebug() << "WidgetDataBase::loadPlugins(): " << addedPlugins << " added, " << replacedPlugins << " replaced, " << removedPlugins << "deleted.";
}

WidgetDataBaseItem *WidgetDataBase::createCustomWidgetItem(const QDesignerCustomWidgetInterface *c, const QString &plugin)
{
    WidgetDataBaseItem *item = new WidgetDataBaseItem(c->name(), c->group());
    item->setContainer(c->isContainer());
    item->setCustom(true);
    item->setIcon(c->icon());
    item->setIncludeFile(c->includeFile());
    item->setToolTip(c->toolTip());
    item->setWhatsThis(c->whatsThis());
    item->setPluginPath(plugin);
    return item;
}

void WidgetDataBase::remove(int index)
{
    Q_ASSERT(index < m_items.size());
    delete m_items.takeAt(index);
}

QList<QVariant> WidgetDataBase::defaultPropertyValues(const QString &name)
{
    WidgetFactory *factory = qobject_cast<WidgetFactory *>(m_core->widgetFactory());
    Q_ASSERT(factory);
    // Create non-widgets, widgets in order
    QObject* object = factory->createObject(name, 0);
    if (!object)
        object = factory->createWidget(name, 0);
    if (!object) {
        qDebug() << "** WARNING Factory failed to create " << name;
        return QList<QVariant>();
    }
    // Get properties from sheet.
    QList<QVariant> result;
    if (const QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(m_core->extensionManager(), object)) {
        const int propertyCount = sheet->count();
        for (int i = 0; i < propertyCount; ++i) {
            result.append(sheet->property(i));
        }
    }
    delete object;
    return result;
}

void WidgetDataBase::grabDefaultPropertyValues()
{
    const int itemCount = count();
    for (int i = 0; i < itemCount; ++i) {
        QDesignerWidgetDataBaseItemInterface *dbItem = item(i);
        const QList<QVariant> default_prop_values = defaultPropertyValues(dbItem->name());
        dbItem->setDefaultPropertyValues(default_prop_values);

    }
}

QDESIGNER_SHARED_EXPORT IncludeSpecification  includeSpecification(QString includeFile)
{
    const bool global = !includeFile.isEmpty() &&
                        includeFile[0] == QLatin1Char('<') &&
                        includeFile[includeFile.size() - 1] ==  QLatin1Char('>');
    if (global) {
        includeFile.remove(includeFile.size() - 1, 1);
        includeFile.remove(0, 1);
    }
    return IncludeSpecification(includeFile, global ? IncludeGlobal : IncludeLocal);
}

QDESIGNER_SHARED_EXPORT QString buildIncludeFile(QString includeFile, IncludeType includeType) {
    if (includeType == IncludeGlobal && !includeFile.isEmpty()) {
        includeFile.append(QLatin1Char('>'));
        includeFile.insert(0, QLatin1Char('<'));
    }
    return includeFile;
}


/* Appends a derived class to the database inheriting the data of the base class. Used
   for custom and promoted widgets.

   Depending on whether an entry exists, the existing or a newly created entry is
   returned. A return value of 0 indicates that the base class could not be found. */

QDESIGNER_SHARED_EXPORT QDesignerWidgetDataBaseItemInterface *
        appendDerived(QDesignerWidgetDataBaseInterface *db,
                      const QString &className, const QString &group,
                      const QString &baseClassName,
                      const QString &includeFile,
                      bool promoted, bool custom)
        {
    if (debugWidgetDataBase)
        qDebug() << "appendDerived " << className << " derived from " << baseClassName;
    // Check whether item already exists.
    QDesignerWidgetDataBaseItemInterface *derivedItem = 0;
    const int existingIndex = db->indexOfClassName(className);
    if ( existingIndex != -1)
        derivedItem =  db->item(existingIndex);
    if (derivedItem) {
        // Check the existing item for base class mismatch. This will likely
        // happen when loading a file written by an instance with missing plugins.
        // In that case, just warn and ignore the file properties.
        //
        // An empty base class indicates that it is not known (for example, for custom plugins).
        // In this case, the widget DB is later updated once the widget is created
        // by DOM (by querying the metaobject). Suppress the warning.
        const QString existingBaseClass = derivedItem->extends();
        if (existingBaseClass.isEmpty() || baseClassName ==  existingBaseClass)
            return derivedItem;

        // Warn about mismatches
        const char *baseWarning = "The file contains a custom widget '%1' whose base class (%2)"
          " differs from the current entry in the widget database (%3)."
           " The widget database is left unchanged.";
        designerWarning(QObject::tr(baseWarning).arg(className).arg(baseClassName).arg(existingBaseClass));
        return derivedItem;
    }
    // Create this item, inheriting its base properties
    const int baseIndex = db->indexOfClassName(baseClassName);
    if (baseIndex == -1) {
        if (debugWidgetDataBase)
            qDebug() << "appendDerived failed due to missing base class";
        return 0;
    }
    const QDesignerWidgetDataBaseItemInterface *baseItem = db->item(baseIndex);
    derivedItem = WidgetDataBaseItem::clone(baseItem);
    // Sort of hack: If base class is QWidget, we most likely
    // do not want to inherit the container attribute.
    static const QString qWidgetName = QLatin1String("QWidget");
    if (baseItem->name() == qWidgetName)
        derivedItem->setContainer(false);
    // set new props
    derivedItem->setName(className);
    derivedItem->setGroup(group);
    derivedItem->setCustom(custom);
    derivedItem->setPromoted(promoted);
    derivedItem->setExtends(baseClassName);
    derivedItem->setIncludeFile(includeFile);
    db->append(derivedItem);
    return derivedItem;
}

/* Return a list of database items to which a class can be promoted to. */

QDESIGNER_SHARED_EXPORT WidgetDataBaseItemList
        promotionCandidates(const QDesignerWidgetDataBaseInterface *db,
                            const QString &baseClassName)
{
    WidgetDataBaseItemList rc;
    // find existing promoted widgets deriving from base.
    const int count = db->count();
    for (int i = 0; i < count; ++i) {
        QDesignerWidgetDataBaseItemInterface *item = db->item(i);
        if (item->isPromoted() && item->extends() == baseClassName) {
            rc.push_back(item);
        }
    }
    return rc;
}
} // namespace qdesigner_internal

QT_END_NAMESPACE
