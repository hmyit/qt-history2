/****************************************************************************
**
** Copyright (C) 2004-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of an example program for Qt.
** EDITIONS: NOLIMITS
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/*
    treeitem.cpp

    A container for items of data supplied by the simple tree model.
*/

#include <QStringList>

#include "treeitem.h"

TreeItem::TreeItem(QStringList data, TreeItem *parent)
{
    parentItem = parent;
    itemData = data;
}

TreeItem::~TreeItem()
{
    QList<TreeItem*>::iterator it;
    for (it = childItems.begin(); it != childItems.end(); ++it)
        delete *it;
}

void TreeItem::appendChildItem(TreeItem *item)
{
    childItems.append(item);
}

TreeItem *TreeItem::childItem(int row)
{
    if (row >= 0 && row < childItems.count())
        return childItems[row];

    return 0;
}

int TreeItem::childCount() const
{
    return childItems.count();
}

int TreeItem::childRow(TreeItem *child) const
{
    return childItems.indexOf(child);
}

int TreeItem::columnCount() const
{
    return itemData.count();
}

QVariant TreeItem::data(int column) const
{
    if (column >= 0 && column < itemData.count())
        return itemData[column];

    return QVariant();
}

TreeItem *TreeItem::parent()
{
    return parentItem;
}

int TreeItem::row() const
{
    if (parentItem)
        return parentItem->childRow(const_cast<TreeItem*>(this));

    return 0;
}
