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

/*
TRANSLATOR qdesigner_internal::ObjectInspector
*/

#include "objectinspector.h"
#include "formwindow.h"

// sdk
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerTaskMenuExtension>
#include <QtDesigner/QExtensionManager>
#include <QtDesigner/QDesignerFormWindowCursorInterface>
#include <QtDesigner/QDesignerFormWindowManagerInterface>
#include <QtDesigner/QDesignerWidgetDataBaseInterface>
#include <QtDesigner/QDesignerContainerExtension>
#include <QtDesigner/QDesignerMetaDataBaseInterface>
#include <QtDesigner/QDesignerPropertyEditorInterface>
// shared
#include <tree_widget_p.h>

// Qt
#include <QtGui/QAction>
#include <QtGui/QMenu>
#include <QtGui/QApplication>
#include <QtGui/QHeaderView>
#include <QtGui/QScrollBar>
#include <QtGui/QItemDelegate>
#include <QtGui/QPainter>
#include <QtGui/QVBoxLayout>

#include <QtCore/QStack>
#include <QtCore/QPair>
#include <QtCore/qdebug.h>


static inline QObject *objectOfItem(QTreeWidgetItem *item) {
    return qvariant_cast<QObject *>(item->data(0, 1000));
}

namespace qdesigner_internal {

ObjectInspector::ObjectInspector(QDesignerFormEditorInterface *core, QWidget *parent)
    : QDesignerObjectInspector(parent),
      m_core(core),
      m_treeWidget(new TreeWidget(this))
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);

    vbox->addWidget(m_treeWidget);

    m_treeWidget->setColumnCount(2);
    m_treeWidget->headerItem()->setText(0, tr("Object"));
    m_treeWidget->headerItem()->setText(1, tr("Class"));

    m_treeWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_treeWidget->header()->setResizeMode(1, QHeaderView::Stretch);
    m_treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeWidget->setTextElideMode (Qt::ElideMiddle);

    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_treeWidget, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(slotPopupContextMenu(QPoint)));

    connect(m_treeWidget, SIGNAL(itemPressed(QTreeWidgetItem*,int)),
            this, SLOT(slotSelectionChanged()));

    connect(m_treeWidget, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
            this, SLOT(slotSelectionChanged()));

    connect(m_treeWidget->header(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(slotHeaderDoubleClicked(int)));

}

ObjectInspector::~ObjectInspector()
{
}

QDesignerFormEditorInterface *ObjectInspector::core() const
{
    return m_core;
}

void ObjectInspector::slotPopupContextMenu(const QPoint &pos)
{
    if (m_formWindow->currentTool() != 0)
        return;

    QTreeWidgetItem *item = m_treeWidget->itemAt(pos);
    if (!item)
        return;

    QObject *object = objectOfItem(item);
    if (!object)
        return;

    QMenu *menu = 0;
    if (object->isWidgetType()) {
        if (qdesigner_internal::FormWindowBase *fwb = qobject_cast<qdesigner_internal::FormWindowBase*>(m_formWindow))
            menu = fwb->initializePopupMenu(qobject_cast<QWidget *>(object));
    } else {
        // Pull extension for non-widget
        QDesignerTaskMenuExtension *taskMenu = qt_extension<QDesignerTaskMenuExtension*>(core()->extensionManager(), object);
        if (taskMenu) {
            QList<QAction*> actions = taskMenu->taskActions();
            if (!actions.isEmpty()) {
                menu = new QMenu(this);
                menu->addActions(actions);
            }
        }
    }

    if (menu) {
        menu->exec(m_treeWidget->viewport()->mapToGlobal(pos));
        delete menu;
    }
}

bool ObjectInspector::sortEntry(const QObject *a, const QObject *b)
{
    return a->objectName() < b->objectName();
}

void ObjectInspector::setFormWindow(QDesignerFormWindowInterface *fw)
{
    const bool resizeToColumn =  m_formWindow != fw;
    m_formWindow = fw;

    const int oldWidth = m_treeWidget->columnWidth(0);
    const int xoffset = m_treeWidget->horizontalScrollBar()->value();
    const int yoffset = m_treeWidget->verticalScrollBar()->value();

    m_treeWidget->clear();

    if (!fw || !fw->mainContainer())
        return;

    const QDesignerFormWindowCursorInterface* cursor=fw->cursor();
    const QDesignerWidgetDataBaseInterface *db = fw->core()->widgetDataBase();

    m_treeWidget->setUpdatesEnabled(false);

    typedef QPair<QTreeWidgetItem*, QObject*> ItemObjectPair;
    QStack<ItemObjectPair> workingList;
    QObject *rootObject = fw->mainContainer();
    workingList.append(qMakePair(new QTreeWidgetItem(m_treeWidget), rootObject));

    // remember the selection and apply later
    typedef QVector<QTreeWidgetItem*> SelectionList;
    SelectionList selectionList;

    const QString qLayoutWidget = QLatin1String("QLayoutWidget");
    const QString designerPrefix = QLatin1String("QDesigner");
    static const QString noName = tr("<noname>");
    static const QString separator =  tr("separator");

    while (!workingList.isEmpty()) {
        QTreeWidgetItem *item = workingList.top().first;
        QObject *object = workingList.top().second;
        workingList.pop();

        const bool isWidget = object->isWidgetType();

        // MainWindow can be current, but not explicitly be selected.
        if (isWidget && (cursor && cursor->isWidgetSelected(static_cast<QWidget*>(object)) ||
                         object == cursor->current())) {
            selectionList.push_back(item);
        }

        QString className = QLatin1String(object->metaObject()->className());
        if (QDesignerWidgetDataBaseItemInterface *widgetItem = db->item(db->indexOfObject(object, true))) {
            className = widgetItem->name();

            if (isWidget && className == qLayoutWidget
                    && static_cast<QWidget*>(object)->layout()) {
                className = QLatin1String(static_cast<QWidget*>(object)->layout()->metaObject()->className());
            }

            item->setIcon(0, widgetItem->icon());
        }

        if (className.startsWith(designerPrefix))
            className.remove(1, designerPrefix.size() - 1);

        item->setText(1, className);
        item->setToolTip(1, className);
        item->setData(0, 1000, qVariantFromValue(object));

        QString objectName = object->objectName();
        if (objectName.isEmpty())
            objectName = noName;

        if (const QAction *act = qobject_cast<const QAction*>(object)) { // separator is reserved
            if (act->isSeparator()) {
                objectName = separator;
            }
            item->setIcon(0, act->icon());
        }

        item->setText(0, objectName);
        item->setToolTip(0, objectName);

        if (QDesignerContainerExtension *c = qt_extension<QDesignerContainerExtension*>(fw->core()->extensionManager(), object)) {
            for (int i=0; i<c->count(); ++i) {
                QObject *page = c->widget(i);
                Q_ASSERT(page != 0);

                QTreeWidgetItem *pageItem = new QTreeWidgetItem(item);
                workingList.append(qMakePair(pageItem, page));
            }
        } else {
            QList<QObject*> children = object->children();
            qSort(children.begin(), children.end(), ObjectInspector::sortEntry);

            foreach (QObject *child, children) {
                QWidget *widget = qobject_cast<QWidget*>(child);
                if (!widget || !fw->isManaged(widget))
                    continue;

                QTreeWidgetItem *childItem = new QTreeWidgetItem(item);
                workingList.append(qMakePair(childItem, child));
            }

            if (QWidget *widget = qobject_cast<QWidget*>(object)) {
                QList<QAction*> actions = widget->actions();
                foreach (QAction *action, actions) {
                    if (!fw->core()->metaDataBase()->item(action))
                        continue;

                    QObject *obj = action;
                    if (action->menu())
                        obj = action->menu();

                    QTreeWidgetItem *childItem = new QTreeWidgetItem(item);
                    workingList.append(qMakePair(childItem, obj));
                }
            }
        }

        m_treeWidget->expandItem(item);
    }

    m_treeWidget->horizontalScrollBar()->setValue(xoffset);
    m_treeWidget->verticalScrollBar()->setValue(yoffset);

    switch (selectionList.size()) {
    case 0:
        break;
    case 1:
        m_treeWidget->scrollToItem(selectionList[0]);
        m_treeWidget->setCurrentItem(selectionList[0]);
        break;
    default:
        foreach (QTreeWidgetItem* item, selectionList) {
            item->setSelected(true);
        }
        m_treeWidget->scrollToItem(selectionList[0]);
        break;
    }

    m_treeWidget->setUpdatesEnabled(true);
    m_treeWidget->update();

    if (resizeToColumn) {
        m_treeWidget->resizeColumnToContents(0);
    } else {
        m_treeWidget->setColumnWidth(0, oldWidth);
    }
}

void ObjectInspector::showContainersCurrentPage(QWidget *widget)
{
    if (!widget)
        return;

    FormWindow *fw = FormWindow::findFormWindow(widget);
    if (!fw)
        return;

    QWidget *w = widget->parentWidget();
    while (1) {
        if (fw->isMainContainer(w))
            return;

        if (!w)
            return;

        QDesignerContainerExtension *c = qt_extension<QDesignerContainerExtension*>(core()->extensionManager(), w);
        if (c && !c->widget(c->currentIndex())->isAncestorOf(widget)) {
            for (int i = 0; i < c->count(); i++)
                if (c->widget(i)->isAncestorOf(widget)) {
                    c->setCurrentIndex(i);
                    break;
                }
        }
        w = w->parentWidget();
    }
}

void ObjectInspector::slotSelectionChanged()
{
    if (!m_formWindow)
        return;

    Selection selection;
    getSelection(selection);

    if (!selection.m_cursorSelection.isEmpty())
        showContainersCurrentPage(selection.m_cursorSelection.last());
    if (!selection.m_selectedObjects.isEmpty())
        showContainersCurrentPage(qobject_cast<QWidget *>(selection.m_selectedObjects[0]));

    m_formWindow->clearSelection(false);

    if (!selection.m_cursorSelection.empty()) {

        // This will trigger an update
        foreach (QWidget* widget, selection.m_cursorSelection) {
            m_formWindow->selectWidget(widget);
        }
    } else {
        if (!selection.m_selectedObjects.empty()) {
            // refresh at least the property editor
            core()->propertyEditor()->setObject(selection.m_selectedObjects[0]);
            core()->propertyEditor()->setEnabled(selection.m_selectedObjects.size());
        }
    }

    QMetaObject::invokeMethod(m_formWindow->core()->formWindowManager(), "slotUpdateActions");
}

void ObjectInspector::getSelection(Selection &s) const
{
    s.clear();

    const QList<QTreeWidgetItem*> items = m_treeWidget->selectedItems();
    if (items.empty())
        return;

    // sort objects
    foreach (QTreeWidgetItem *item, items) {
        QObject *object = objectOfItem(item);
        QWidget *widget = qobject_cast<QWidget*>(object);
        if (widget && m_formWindow->isManaged(widget)) {
            s.m_cursorSelection.push_back(widget);
        } else {
            if (core()->metaDataBase()->item(object)) {
                // It is actually possible to select an action
                // twice if it is in a menu bar and in a tool bar.
                if (!s.m_selectedObjects.contains(object)) {
                    s.m_selectedObjects.push_back(object);
                }
            }
        }
    }
}

void ObjectInspector::findRecursion(QTreeWidgetItem *item, QObject *o,  ItemList &matchList)
{
    if (objectOfItem(item) == o)
        matchList += item;

    if (const int cc = item->childCount())
        for (int i = 0;i < cc; i++)
            findRecursion(item->child(i), o, matchList);
}

ObjectInspector::ItemList ObjectInspector::findItemsOfObject(QObject *o) const
{
    ItemList rc;
    if (const int tlc = m_treeWidget->topLevelItemCount())
        for (int i = 0;i < tlc; i++)
            findRecursion(m_treeWidget->topLevelItem (i), o, rc);
    return rc;
}

bool ObjectInspector::selectObject(QObject *o)
{

    	qDebug() << "s " << o;
    if (!core()->metaDataBase()->item(o))
        return false;

    const ItemList items = findItemsOfObject(o);
    if (items.empty()) {
	qDebug() << "e";	
        return false;
    }

    // Change in selection?
    const  ItemList currentSelectedItems = m_treeWidget->selectedItems();
    if (!currentSelectedItems.empty() && currentSelectedItems.toSet() == items.toSet()) {
	qDebug() << "m";
        return true;
    }
    // do select and update
    m_treeWidget->clearSelection();
    const ItemList::const_iterator cend = items.constEnd();
    for (ItemList::const_iterator it = items.constBegin(); it != cend; ++it )  {
        (*it)->setSelected(true);
    }
    slotSelectionChanged();
    	qDebug() << "s";
    return true;
}

void ObjectInspector::slotHeaderDoubleClicked(int column)
{
    m_treeWidget->resizeColumnToContents(column);
}
}
