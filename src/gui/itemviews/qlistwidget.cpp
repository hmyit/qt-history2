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

#include "qlistwidget.h"
#include <private/qlistview_p.h>

class QListModel : public QAbstractListModel
{
public:
    QListModel(QListWidget *parent = 0);

    QListWidgetItem *at(int row) const;
    void insert(int row, QListWidgetItem *item);
    void append(QListWidgetItem *item);
    void remove(QListWidgetItem *item);
    QListWidgetItem *take(int row);

    int rowCount() const;

    QModelIndex index(QListWidgetItem *item) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex::Null,
                      QModelIndex::Type type = QModelIndex::View) const;

    QVariant data(const QModelIndex &index, int role = QAbstractItemModel::DisplayRole) const;
    bool setData(const QModelIndex &index, int role, const QVariant &value);

    bool insertRows(int row, const QModelIndex &parent = QModelIndex::Null, int count = 1);
    bool removeRows(int row, const QModelIndex &parent = QModelIndex::Null, int count = 1);

    bool isSelectable(const QModelIndex &index) const;
    bool isEditable(const QModelIndex &index) const;

private:
    QList<QListWidgetItem*> lst;
};

QListModel::QListModel(QListWidget *parent)
    : QAbstractListModel(parent)
{
}

QListWidgetItem *QListModel::at(int row) const
{
    if (row >= 0 && row < lst.count())
        return lst.at(row);
    return 0;
}

void QListModel::append(QListWidgetItem *item)
{
    lst.append(item);
    int row = lst.count() - 1;
    emit rowsInserted(QModelIndex::Null, row, row);
}

void QListModel::remove(QListWidgetItem *item)
{
    int row = lst.indexOf(item);
    if (row != -1)
        lst.removeAt(row);
}

void QListModel::insert(int row, QListWidgetItem *item)
{
    if (row >= 0 && row <= lst.count()) {
        lst.insert(row, item);
        emit rowsInserted(QModelIndex::Null, row, row);
    }
}

QListWidgetItem *QListModel::take(int row)
{
    if (row >= 0 && row <= lst.count()) {
        emit rowsRemoved(QModelIndex::Null, row, row);
        return lst.takeAt(row);
    }
    return 0;
}

int QListModel::rowCount() const
{
    return lst.count();
}

QModelIndex QListModel::index(QListWidgetItem *item) const
{
    int row = lst.indexOf(item);
    if (row == -1)
        return QModelIndex::Null;
    return createIndex(row, 0, item, QModelIndex::View);
}

QModelIndex QListModel::index(int row, int column,
                              const QModelIndex &parent,
                              QModelIndex::Type type) const
{
    if (isValid(row, column, parent))
        return createIndex(row, column, lst.at(row), type);
    return QModelIndex::Null;
}

QVariant QListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= (int)lst.count())
        return QVariant();
    return lst.at(index.row())->data(role);
}

bool QListModel::setData(const QModelIndex &index, int role, const QVariant &value)
{
    if (!index.isValid() || index.row() >= (int)lst.count())
        return false;
    lst.at(index.row())->setData(role, value);
    emit dataChanged(index, index);
    return true;
}

bool QListModel::insertRows(int row, const QModelIndex &, int count)
{
    QListWidget *view = ::qt_cast<QListWidget*>(QObject::parent());
    if (row < rowCount())
        for (int r = row; r < row + count; ++r)
            lst.insert(r, new QListWidgetItem(view));
    else
        for (int r = 0; r < count; ++r)
            lst.append(new QListWidgetItem(view));
    emit rowsInserted(QModelIndex::Null, row, row + count - 1);
    return true;
}

bool QListModel::removeRows(int row, const QModelIndex &, int count)
{
    if (row < rowCount()) {
        emit rowsRemoved(QModelIndex::Null, row, row + count - 1);
        for (int r = 0; r < count; ++r)
            delete lst.takeAt(row);
        return true;
    }
    return false;
}

bool QListModel::isSelectable(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= lst.count())
        return false;
    return lst.at(index.row())->isSelectable();
}

bool QListModel::isEditable(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= lst.count())
        return false;
    return lst.at(index.row())->isEditable();
}

/*!
    \class QListWidgetItem
    \brief The QListWidgetItem class holds a single list widget data
    item.

    ###
*/

/*!
    Creates a new list widget item that isEditable() and
    isSelectable(), but which has no text() or icon().
*/

QListWidgetItem::QListWidgetItem(QListWidget *view)
    : view(view)
{
    if (view)
        view->appendItem(this);
}

/*!
    Destructs this list widget item.
*/

QListWidgetItem::~QListWidgetItem()
{
    view->removeItem(this);
}

void QListWidgetItem::openPersistentEditor()
{
    view->openPersistentEditor(this);
}

void QListWidgetItem::closePersistentEditor()
{
    view->closePersistentEditor(this);
}

/*!
    \fn QString QListWidgetItem::text() const

    Returns this list widget item's text.

    \sa setText()
*/

/*!
    \fn QIconSet QListWidgetItem::icon() const

    Returns this list widget item's iconset.

    \sa setIcon()
*/

/*!
    \fn bool QListWidgetItem::isEditable() const

    Returns true if this list widget item is editable; otherwise
    returns false.

    \sa setEditable()
*/

/*!
    \fn bool QListWidgetItem::isSelectable() const

    Returns true if this list widget item is selectable; otherwise
    returns false.

    \sa setSelectable()
*/

/*!
    \fn void QListWidgetItem::setText(const QString &text)

    Sets this list widget item's \a text.

    \sa text()
*/

/*!
    \fn void QListWidgetItem::setIcon(const QIconSet &icon)

    Sets this list widget item's \a icon.

    \sa icon()
*/

/*!
    \fn void QListWidgetItem::setEditable(bool editable)

    If \a editable is true, this list widget item can be edited;
    otherwise it cannot be edited.

    \sa isEditable()
*/

/*!
    \fn void QListWidgetItem::setSelectable(bool selectable)

    If \a selectable is true, this list widget item can be selected;
    otherwise it cannot be selected.

    \sa isSelectable()
*/

/*!
    \fn bool QListWidgetItem::operator!=(const QListWidgetItem &other) const

    Returns true if this list widget item and the \a other list widget
    item have at least one role for which their values differ;
    otherwise returns false.
*/

/*!
    Returns true if this list widget item and the \a other list widget
    item have the same values for every role; otherwise returns false.
*/

class QListWidgetPrivate : public QListViewPrivate
{
    Q_DECLARE_PUBLIC(QListWidget)
public:
    QListWidgetPrivate() : QListViewPrivate() {}
    inline QListModel *model() const { return ::qt_cast<QListModel*>(q_func()->model()); }
};

#define d d_func()
#define q q_func()

#ifdef QT_COMPAT
/*!
    Use the single-argument overload and call setObjectName() instead.
*/
QListWidget::QListWidget(QWidget *parent, const char* name)
    : QListView(*new QListWidgetPrivate(), parent)
{
    setObjectName(name);
    setModel(new QListModel(this));
    setItemDelegate(new QWidgetBaseItemDelegate(this));
}
#endif

/*!
    \class QListWidget
    \brief The QListWidget class provides a list or icon view with a
    predefined model.

    \mainclass

    ###
*/

/*!
    Constructs a new QListWidget with the given \a parent.
*/

QListWidget::QListWidget(QWidget *parent)
    : QListView(*new QListWidgetPrivate(), parent)
{
    setModel(new QListModel(this));
    setItemDelegate(new QWidgetBaseItemDelegate(this));
}

/*!
    Destructs this list widget.
*/

QListWidget::~QListWidget()
{
}

/*!
    Returns the \a{row}-th item.

    \sa setItem() row()
*/

QListWidgetItem *QListWidget::item(int row) const
{
    return d->model()->at(row);
}

/*!
    Returns the row for the \a item.

    \sa item()
*/

int QListWidget::row(const QListWidgetItem *item) const
{
    return d->model()->index(const_cast<QListWidgetItem*>(item)).row();
}


/*!
    Inserts \a item in position \a row in the list.

    \sa appendItem()
*/

void QListWidget::insertItem(int row, QListWidgetItem *item)
{
    if (item->view == 0)
        item->view = this;
    d->model()->insert(row, item);
}

/*!
    Inserts items with the text set to \a labels starting at \a row.

    \sa insertItem(), appendItem()
*/

void QListWidget::insertItems(const QStringList &labels, int row)
{
    int insertRow = (row > -1 && row <= count()) ? row : count();
    for (int i=0; i<labels.count(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(this);
        item->setText(labels.at(i));
        d->model()->insert(insertRow + i, item);
    }
}

/*!
    Appends the given \a item to the list.

    \sa insertItem()
*/

void QListWidget::appendItem(QListWidgetItem *item)
{
    if (item->view == 0)
        item->view = this;
    d->model()->append(item);
}

/*!
    Removes the item at \a row from the list without deleting it.

    \sa insertItem() appendItem()
*/

QListWidgetItem *QListWidget::takeItem(int row)
{
    QListWidgetItem *item =  d->model()->take(row);
    item->view = 0;
    return item;
}

int QListWidget::count() const
{
    return d->model()->rowCount();
}


/*!
  Removes the \a item from the list.
*/

void QListWidget::removeItem(QListWidgetItem *item)
{
    d->model()->remove(item);
}

void QListWidget::setModel(QAbstractItemModel *model)
{
    QListView::setModel(model);
}

void QListWidget::openPersistentEditor(QListWidgetItem *item)
{
    QModelIndex index = d->model()->index(item);
    QAbstractItemView::openPersistentEditor(index);
}

void QListWidget::closePersistentEditor(QListWidgetItem *item)
{
    QModelIndex index = d->model()->index(item);
    QAbstractItemView::closePersistentEditor(index);
}
