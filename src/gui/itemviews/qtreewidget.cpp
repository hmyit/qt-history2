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

#include "qtreewidget.h"
#include <qapplication.h>
#include <qheaderview.h>
#include <private/qtreeview_p.h>

class QTreeModel : public QAbstractItemModel
{
    friend class QTreeWidget;
    friend class QTreeWidgetItem;

public:
    QTreeModel(int columns = 0, QObject *parent = 0);
    ~QTreeModel();

    void setColumnCount(int columns);
    
    QTreeWidgetItem *item(const QModelIndex &index) const;

    QModelIndex index(QTreeWidgetItem *item) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex::Null,
                      QModelIndex::Type type = QModelIndex::View) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex::Null) const;
    int columnCount(const QModelIndex &parent = QModelIndex::Null) const;

    QVariant data(const QModelIndex &index, int role = QAbstractItemModel::DisplayRole) const;
    bool setData(const QModelIndex &index, int role, const QVariant &value);

    bool insertRows(int row, const QModelIndex &parent = QModelIndex::Null, int count = 1);
    bool removeRows(int row, const QModelIndex &parent = QModelIndex::Null, int count = 1);

    bool isSelectable(const QModelIndex &index) const;
    bool isEditable(const QModelIndex &index) const;

protected:
    void append(QTreeWidgetItem *item);
    void emitRowsInserted(QTreeWidgetItem *item);

private:
    QList<QTreeWidgetItem*> tree;
    QTreeWidgetItem *header;
};

/*
  \class QTreeModel qtreewidget.h
 The QTreeModel class manages the items stored in a tree view.

  \ingroup model-view
    \mainclass
*/

/*!
  \internal

  Constructs a tree model with a \a parent object and the given
  number of \a columns.
*/

QTreeModel::QTreeModel(int columns, QObject *parent)
    : QAbstractItemModel(parent), header(new QTreeWidgetItem())
{
    setColumnCount(columns);
}

/*!
  \internal

  Destroys this tree model.
*/

QTreeModel::~QTreeModel()
{
    for (int i = 0; i < tree.count(); ++i)
        delete tree.at(i);
    delete header;
}

/*!
  \internal

  Sets the number of \a columns in the tree model.
*/

void QTreeModel::setColumnCount(int columns)
{
    int c = header->columnCount();
    if (c == columns)
        return;
    int _c = c;
    c = columns;
    if (c < _c)
        emit columnsRemoved(QModelIndex::Null, qMax(_c - 1, 0), qMax(c - 1, 0));
    header->setColumnCount(c);
    for (int i = _c; i < c; ++i)
        header->setText(i, QString::number(i));
    if (c > _c)
        emit columnsInserted(QModelIndex::Null, qMax(_c - 1, 0), qMax(c - 1, 0));
}

/*!
  \internal

  Returns the tree view item corresponding to the \a index given.

  \sa QModelIndex
*/

QTreeWidgetItem *QTreeModel::item(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;
    if (index.type() != QModelIndex::View)
        return header;
    return static_cast<QTreeWidgetItem*>(index.data());
}

/*!
  \internal

  Returns the model index that refers to the tree view \a item.
*/

QModelIndex QTreeModel::index(QTreeWidgetItem *item) const
{
    if (!item)
        return QModelIndex::Null;
    const QTreeWidgetItem *par = item->parent();
    int row = par ? par->children.indexOf(item) : tree.indexOf(item);
    return createIndex(row, 0, item);
}

/*!
  \internal

  Returns the model index with the given \a row, \a column, \a type,
  and \a parent.
*/

QModelIndex QTreeModel::index(int row, int column, const QModelIndex &parent,
                              QModelIndex::Type type) const
{
    int r = tree.count();
    int c = header->columnCount();
    if (row < 0 || row >= r || column < 0 || column >= c)
        return QModelIndex::Null;
    if (!parent.isValid()) {// toplevel
        QTreeWidgetItem *itm = tree.at(row);
        if (itm)
            return createIndex(row, column, itm, type);
        return QModelIndex::Null;
    }
    QTreeWidgetItem *parentItem = item(parent);
    if (parentItem && row < parentItem->childCount()) {
        QTreeWidgetItem *itm = static_cast<QTreeWidgetItem*>(parentItem->child(row));
        if (itm)
            return createIndex(row, column, itm, type);
        return QModelIndex::Null;
    }
    return QModelIndex::Null;
}

/*!
  \internal

  Returns the parent model index of the index given as the \a child.
*/

QModelIndex QTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex::Null;
    QTreeWidgetItem *itm = reinterpret_cast<QTreeWidgetItem *>(child.data());
    if (!itm)
        return QModelIndex::Null;
    QTreeWidgetItem *parent = itm->parent();
    return index(parent);
}

/*!
  \internal

  Returns the number of rows in the \a parent model index.
*/

int QTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        QTreeWidgetItem *parentItem = item(parent);
        if (parentItem)
            return parentItem->childCount();
    }
    return tree.count();
}

/*!
  \internal

  Returns the number of columns in the item referred to by the given
  \a index.
*/

int QTreeModel::columnCount(const QModelIndex &) const
{
    return header->columnCount();
}

/*!
  \internal

  Returns the data corresponding to the given model \a index and
  \a role.
*/

QVariant QTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    QTreeWidgetItem *itm = item(index);
    if (itm)
        return itm->data(index.column(), role);
    return QVariant();
}

/*!
  \internal

  Sets the data for the item specified by the \a index and \a role
  to that referred to by the \a value.

  Returns true if successful; otherwise returns false.
*/

bool QTreeModel::setData(const QModelIndex &index, int role, const QVariant &value)
{
    if (!index.isValid())
        return false;
    QTreeWidgetItem *itm = item(index);
    if (itm) {
        itm->setData(index.column(), role, value);
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

/*!
  \internal

  Inserts a tree view item into the \a parent item at the given
  \a row. Returns true if successful; otherwise returns false.

  If no valid parent is given, the item will be inserted into this
  tree model at the row given.
*/

bool QTreeModel::insertRows(int row, const QModelIndex &parent, int)
{
    if (parent.isValid()) {
        QTreeWidgetItem *p =  item(parent);
        if (p) {
            p->children.insert(row, new QTreeWidgetItem(p));
            return true;
        }
        return false;
    }
    tree.insert(row, new QTreeWidgetItem());
    return true;
}

/*!
  \internal

  Removes the given \a row from the \a parent item, and returns true
  if successful; otherwise false is returned.
*/

bool QTreeModel::removeRows(int row, const QModelIndex &parent, int count)
{
    // FIXME: !!!!!!!
    if (parent.isValid()) {
        QTreeWidgetItem *p = item(parent);
        if (p) {
            p->children.removeAt(row);
            return true;
        }
        return false;
    }
    tree.removeAt(row);
    return true;
}

/*!
  \internal

  Returns true if the item at the \a index given is selectable;
  otherwise returns false.
*/

bool QTreeModel::isSelectable(const QModelIndex &) const
{
    return true;
}

/*!
  \internal

  Returns true if the item at the \a index given is editable;
  otherwise returns false.
*/

bool QTreeModel::isEditable(const QModelIndex &) const
{
    return true;
}

/*!
  \internal

  Appends the tree view \a item to the tree model.*/

void QTreeModel::append(QTreeWidgetItem *item)
{
    int r = tree.count();
    tree.push_back(item);
    emit rowsInserted(QModelIndex::Null, r, r);
}

/*!
\internal

Emits the rowsInserted() signal for the rows containing the given \a item.

\sa rowsInserted()*/

void QTreeModel::emitRowsInserted(QTreeWidgetItem *item)
{
    QModelIndex idx = index(item);
    QModelIndex parentIndex = parent(idx);
    emit rowsInserted(parentIndex, idx.row(), idx.row());
}

/*!
  Constructs a tree widget item. The item must be inserted
  into a tree view.

  \sa QTreeModel::append() QTreeWidget::append()
*/

QTreeWidgetItem::QTreeWidgetItem()
    : view(0), par(0)
{
}

/*!
    \fn QTreeWidgetItem::QTreeWidgetItem(QTreeWidget *view)

    Constructs a tree widget item and inserts it into the given tree
    \a view.
*/

QTreeWidgetItem::QTreeWidgetItem(QTreeWidget *view)
    : view(view), par(0)
{
    if (view)
        view->appendItem(this);
}

/*!
    Constructs a tree widget item with the given \a parent.
*/

QTreeWidgetItem::QTreeWidgetItem(QTreeWidgetItem *parent)
    : view(parent->view), par(parent)
{
    if (parent)
        parent->children.push_back(this);
    QTreeModel *model = ::qt_cast<QTreeModel*>(view->model());
    model->emitRowsInserted(this);
}

/*!
  Destroys this tree widget item.
*/

QTreeWidgetItem::~QTreeWidgetItem()
{
    for (int i = 0; i < children.count(); ++i)
        delete children.at(i);
}

QTreeWidgetItem::CheckedState QTreeWidgetItem::checkedState() const
{
    return static_cast<CheckedState>(retrieve(0, CheckRole).toInt());
}

void QTreeWidgetItem::setCheckedState(CheckedState state)
{
    store(0, CheckRole, static_cast<int>(state));
}

/*!
    Returns the text stored in the \a column.

  \sa data() QAbstractItemModel::Role
*/

QString QTreeWidgetItem::text(int column) const
{
    return retrieve(column, QAbstractItemModel::DisplayRole).toString();
}

/*!
    Sets the text for the item specified by the \a column to the given \a text.

    \sa text() setIcon()
*/

void QTreeWidgetItem::setText(int column, const QString &text)
{
    store(column, QAbstractItemModel::DisplayRole, text);
}

/*!
    Returns the icon stored in the \a column.

  \sa data() QAbstractItemModel::Role
*/

QIconSet QTreeWidgetItem::icon(int column) const
{
    return retrieve(column, QAbstractItemModel::DecorationRole).toIcon();
}

/*!
    Sets the icon for the item specified by the \a column to the given \a icon.

    \sa icon() setText()
*/

void QTreeWidgetItem::setIcon(int column, const QIconSet &icon)
{
    store(column, QAbstractItemModel::DecorationRole, icon);
}

/*!
    Returns the status tip text for the specified \a column.

    \sa setStatusTip() whatsThis() toolTip()
*/
QString QTreeWidgetItem::statusTip(int column) const
{
    return retrieve(column, QAbstractItemModel::StatusTipRole).toString();
}

/*!
    Sets the status tip text to \a statusTip for the specified \a
    column.

    \sa statusTip() setWhatsThis() setToolTip()
*/
void QTreeWidgetItem::setStatusTip(int column, const QString &statusTip)
{
    store(column, QAbstractItemModel::StatusTipRole, statusTip);
}

/*!
    Returns the tool tip text for the specified \a column.

    \sa setToolTip() whatsThis() statusTip()
*/
QString QTreeWidgetItem::toolTip(int column) const
{
    return retrieve(column, QAbstractItemModel::ToolTipRole).toString();
}

/*!
    Sets the tool tip text to \a toolTip for the specified \a
    column.

    \sa toolTip() setWhatsThis() setStatusTip()
*/
void QTreeWidgetItem::setToolTip(int column, const QString &toolTip)
{
    store(column, QAbstractItemModel::ToolTipRole, toolTip);
}

/*!
    Returns the What's This text for the specified \a column.

    \sa setWhatsThis() toolTip() statusTip()
*/
QString QTreeWidgetItem::whatsThis(int column) const
{
    return retrieve(column, QAbstractItemModel::WhatsThisRole).toString();
}

/*!
    Sets the What's This text to \a whatsThis for the specified \a
    column.

    \sa whatsThis() setToolTip() setStatusTip()
*/
void QTreeWidgetItem::setWhatsThis(int column, const QString &whatsThis)
{
    store(column, QAbstractItemModel::WhatsThisRole, whatsThis);
}

/*!
    Returns the text font for the specified \a column.

    \sa setFont() textColor()
*/
QFont QTreeWidgetItem::font(int column) const
{
    QVariant value = retrieve(column, FontRole);
    return value.isValid() ? value.toFont() : QApplication::font();
}

/*!
    Sets the \a font for the specified \a column.

    \sa font() setTextColor()
*/
void QTreeWidgetItem::setFont(int column, const QFont &font)
{
    store(column, FontRole, font);
}

/*!
    Returns the background color for the specified \a column.

    \sa setBackgroundColor() textColor()
*/
QColor QTreeWidgetItem::backgroundColor(int column) const
{
    QVariant value = retrieve(column, BackgroundColorRole);
    return value.isValid() ? value.toColor() : QColor();
}

/*!
    Sets the background \a color for the specified \a column.

    \sa backgroundColor() setTextColor()
*/
void QTreeWidgetItem::setBackgroundColor(int column, const QColor &color)
{
    store(column, BackgroundColorRole, color);
}

/*!
    Returns the text color for the specified \a column.

    \sa setTextColor() backgroundColor()
*/
QColor QTreeWidgetItem::textColor(int column) const
{
    QVariant value = retrieve(column, TextColorRole);
    return value.isValid() ? value.toColor() : QColor();
}

/*!
    Sets the text \a color for the specified \a column.

    \sa textColor() setBackgroundColor()
*/
void QTreeWidgetItem::setTextColor(int column, const QColor &color)
{
    store(column, TextColorRole, color);
}

/*!
    Returns the data stored in the \a column with the given \a role.

  \sa QAbstractItemModel::Role
*/

QVariant QTreeWidgetItem::data(int column, int role) const
{
    if (column < 0 || column >= values.count())
        return QVariant();
    role = (role == QAbstractItemModel::EditRole ? QAbstractItemModel::DisplayRole : role);
    switch (role) {
    case QAbstractItemModel::DisplayRole:
        return text(column);
    case QAbstractItemModel::DecorationRole:
        return icon(column);
    case QAbstractItemModel::StatusTipRole:
        return statusTip(column);
    case QAbstractItemModel::ToolTipRole:
        return toolTip(column);
    case QAbstractItemModel::WhatsThisRole:
        return whatsThis(column);
    case QWidgetBaseItem::FontRole:
        return font(column);
    case QWidgetBaseItem::BackgroundColorRole:
        return backgroundColor(column);
    case QWidgetBaseItem::TextColorRole:
        return textColor(column);
    }
    return QVariant();
}

/*!
    Sets the data for the item specified by the \a column and \a role
    to the given \a value.
*/

void QTreeWidgetItem::setData(int column, int role, const QVariant &value)
{
    role = (role == QAbstractItemModel::EditRole ? QAbstractItemModel::DisplayRole : role);
    switch (role) {
    case QAbstractItemModel::DisplayRole:
        setText(column, value.toString());
        break;
    case QAbstractItemModel::DecorationRole:
        setIcon(column, value.toIconSet());
        break;
    case QAbstractItemModel::StatusTipRole:
        setStatusTip(column, value.toString());
        break;
    case QAbstractItemModel::ToolTipRole:
        setToolTip(column, value.toString());
        break;
    case QAbstractItemModel::WhatsThisRole:
        setWhatsThis(column, value.toString());
        break;
    case QWidgetBaseItem::FontRole:
        setFont(column, value.toFont());
        break;
    case QWidgetBaseItem::BackgroundColorRole:
        setBackgroundColor(column, value.toColor());
        break;
    case QWidgetBaseItem::TextColorRole:
        setTextColor(column, value.toColor());
        break;
    }
}

/*!
    Sets the value for the item's \a column and \a role to the given
    \a value.

    \sa store()
*/
void QTreeWidgetItem::store(int column, int role, const QVariant &value)
{
    if (column >= values.count())
        setColumnCount(column + 1);
    QVector<Data> column_values = values.at(column);
    for (int i = 0; i < column_values.count(); ++i) {
        if (column_values.at(i).role == role) {
            values[column][i].value = value;
            return;
        }
    }
    values[column].append(Data(role, value));
}

/*!
    Returns the value for the item's \a column and \a role.

    \sa store()
*/
QVariant QTreeWidgetItem::retrieve(int column, int role) const
{
    const QVector<Data> column_values = values.at(column);
    for (int i = 0; i < column_values.count(); ++i)
        if (column_values.at(i).role == role)
            return column_values.at(i).value;
    return QVariant();
}

class QTreeWidgetPrivate : public QTreeViewPrivate
{
    Q_DECLARE_PUBLIC(QTreeWidget)
public:
    QTreeWidgetPrivate() : QTreeViewPrivate() {}
    inline QTreeModel *model() const { return ::qt_cast<QTreeModel*>(q_func()->model()); }
};

#define d d_func()
#define q q_func()

/*!
  \class QTreeWidget qtreewidget.h

  \brief The QTreeWidget class provides a tree view that uses a predefined
  tree model.

  \ingroup model-view

  The QTreeWidget class is a convenience class that replaces the \c QListView
  class. It provides a list view widget that takes advantage of Qt's
  model-view architecture.

  This class uses a default model to organize the data represented in the
  tree view, but also uses the QTreeWidgetItem class to provide a familiar
  interface for simple list structures.

  \omit
  In its simplest form, a tree view can be constructed and populated in
  the familiar way:

  \code
    QTreeWidget *view = new QTreeWidget(parent);

  \endcode
  \endomit

  \sa \link model-view-programming.html Model/View Programming\endlink QTreeModel QTreeWidgetItem
*/

/*!
  Constructs a tree view with the given \a parent widget, using the default
  model
*/

QTreeWidget::QTreeWidget(QWidget *parent)
    : QTreeView(*new QTreeViewPrivate(), parent)
{
    setModel(new QTreeModel(0, this));
    setItemDelegate(new QWidgetBaseItemDelegate(this));
    header()->setItemDelegate(new QWidgetBaseItemDelegate(header()));
}

/*!
  Retuns the number of header columns in the view.
*/

int QTreeWidget::columnCount() const
{
    return d->model()->columnCount();
}

/*!
  Sets the number of header \a columns in the tree view.
*/

void QTreeWidget::setColumnCount(int columns)
{
    d->model()->setColumnCount(columns);
}

QTreeWidgetItem *QTreeWidget::headerItem()
{
    return d->model()->header;
}

void QTreeWidget::setHeaderItem(QTreeWidgetItem *item)
{
    delete d->model()->header;
    d->model()->header = item;
}

/*!
  Appends a tree view \a item to the tree view.
*/

void QTreeWidget::appendItem(QTreeWidgetItem *item)
{
    d->model()->append(item);
}

/*!
  Returns true if the \a item is selected, otherwise returns false.
*/

bool QTreeWidget::isSelected(QTreeWidgetItem *item) const
{
    QModelIndex index = d->model()->index(item);
    return selectionModel()->isSelected(index);
}
