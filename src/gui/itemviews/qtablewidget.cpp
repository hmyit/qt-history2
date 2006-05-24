/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qtablewidget.h"

#ifndef QT_NO_TABLEWIDGET
#include <qheaderview.h>
#include <qitemdelegate.h>
#include <qpainter.h>
#include <qabstractitemmodel.h>
#include <private/qabstractitemmodel_p.h>
#include <private/qtableview_p.h>
#include <private/qwidgetitemdata_p.h>

// workaround for VC++ 6.0 linker bug
typedef bool(*LessThan)(const QPair<QTableWidgetItem*,int>&,const QPair<QTableWidgetItem*,int>&);

class QTableWidgetMimeData : public QMimeData
{
    Q_OBJECT
public:
    QList<QTableWidgetItem*> items;
};

class QTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum ItemFlagsExtension {
        ItemIsVerticalHeaderItem = Qt::ItemIsTristate * 2,
        ItemIsHorizontalHeaderItem = Qt::ItemIsTristate * 4
    }; // we need this to separate header items from other items

    QTableModel(int rows, int columns, QTableWidget *parent);
    ~QTableModel();

    bool insertRows(int row, int count = 1, const QModelIndex &parent = QModelIndex());
    bool insertColumns(int column, int count = 1, const QModelIndex &parent = QModelIndex());

    bool removeRows(int row, int count = 1, const QModelIndex &parent = QModelIndex());
    bool removeColumns(int column, int count = 1, const QModelIndex &parent = QModelIndex());

    void setItem(int row, int column, QTableWidgetItem *item);
    QTableWidgetItem *takeItem(int row, int column);
    QTableWidgetItem *item(int row, int column) const;
    QTableWidgetItem *item(const QModelIndex &index) const;
    void removeItem(QTableWidgetItem *item);

    void setHorizontalHeaderItem(int section, QTableWidgetItem *item);
    QTableWidgetItem *takeHorizontalHeaderItem(int section);
    void setVerticalHeaderItem(int section, QTableWidgetItem *item);
    QTableWidgetItem *takeVerticalHeaderItem(int section);
    QTableWidgetItem *horizontalHeaderItem(int section);
    QTableWidgetItem *verticalHeaderItem(int section);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const { return QAbstractTableModel::index(row, column, parent); }
    QModelIndex index(const QTableWidgetItem *item) const;

    void setRowCount(int rows);
    void setColumnCount(int columns);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role);

    Qt::ItemFlags flags(const QModelIndex &index) const;

    void sort(int column, Qt::SortOrder order);
    static bool itemLessThan(const QPair<QTableWidgetItem*,int> &left,
                             const QPair<QTableWidgetItem*,int> &right);
    static bool itemGreaterThan(const QPair<QTableWidgetItem*,int> &left,
                                const QPair<QTableWidgetItem*,int> &right);

    bool isValid(const QModelIndex &index) const;
    inline long tableIndex(int row, int column) const
        { return (row * horizontal.count()) + column; }

    void clear();
    void clearContents();
    void itemChanged(QTableWidgetItem *item);

    inline QTableWidgetItem *createItem() const
        { return prototype ? prototype->clone() : new QTableWidgetItem; }
    inline const QTableWidgetItem *itemPrototype() const
        { return prototype; }
    inline void setItemPrototype(const QTableWidgetItem *item)
        { prototype = item; }

    // dnd
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent);
    Qt::DropActions supportedDropActions() const;

    QMimeData *internalMimeData()  const;

private:
    const QTableWidgetItem *prototype;
    QVector<QTableWidgetItem*> table;
    QVector<QTableWidgetItem*> vertical;
    QVector<QTableWidgetItem*> horizontal;

    // A cache must be mutable if get-functions should have const modifiers
    mutable QModelIndexList cachedIndexes;
};

#include "qtablewidget.moc"

QTableModel::QTableModel(int rows, int columns, QTableWidget *parent)
    : QAbstractTableModel(parent),
      prototype(0),
      table(rows * columns, 0), vertical(rows, 0), horizontal(columns, 0)
{}

QTableModel::~QTableModel()
{
    clear();
}

bool QTableModel::insertRows(int row, int count, const QModelIndex &)
{
    if (count < 1 || row < 0 || row > vertical.count())
        return false;

    beginInsertRows(QModelIndex(), row, row + count - 1);
    int rc = vertical.count();
    int cc = horizontal.count();
    vertical.insert(row, count, 0);
    if (rc == 0)
        table.resize(cc * count);
    else
        table.insert(tableIndex(row, 0), cc * count, 0);
    endInsertRows();
    return true;
}

bool QTableModel::insertColumns(int column, int count, const QModelIndex &)
{
    if (count < 1 || column < 0 || column > horizontal.count())
        return false;

    beginInsertColumns(QModelIndex(), column, column + count - 1);
    int rc = vertical.count();
    int cc = horizontal.count();
    horizontal.insert(column, count, 0);
    if (cc == 0)
        table.resize(rc * count);
    else
        for (int row = 0; row < rc; ++row)
            table.insert(tableIndex(row, column), count, 0);
    endInsertColumns();
    return true;
}

bool QTableModel::removeRows(int row, int count, const QModelIndex &)
{
    if (count < 1 || row < 0 || row + count > vertical.count())
        return false;

    beginRemoveRows(QModelIndex(), row, row + count - 1);
    int i = tableIndex(row, 0);
    int n = count * columnCount();
    QTableWidgetItem *oldItem = 0;
    for (int j=i; j<n+i; ++j) {
        oldItem = table.at(j);
        if (oldItem)
            oldItem->model = 0;
        delete oldItem;
    }
    table.remove(qMax(i, 0), n);
    for (int v=row; v<row+count; ++v) {
        oldItem = vertical.at(v);
        if (oldItem)
            oldItem->model = 0;
        delete oldItem;
    }
    vertical.remove(row, count);
    endRemoveRows();
    return true;
}

bool QTableModel::removeColumns(int column, int count, const QModelIndex &)
{
    if (count < 1 || column < 0 || column + count >  horizontal.count())
        return false;

    beginRemoveColumns(QModelIndex(), column, column + count - 1);
    QTableWidgetItem *oldItem = 0;
    for (int row = rowCount() - 1; row >= 0; --row) {
        int i = tableIndex(row, column);
        for (int j=i; j<i+count; ++j) {
            oldItem = table.at(j);
            if (oldItem)
                oldItem->model = 0;
            delete oldItem;
        }
        table.remove(i, count);
    }
    for (int h=column; h<column+count; ++h) {
        oldItem = horizontal.at(h);
        if (oldItem)
            oldItem->model = 0;
        delete oldItem;
    }
    horizontal.remove(column, count);
    endRemoveColumns();
    return true;
}

void QTableModel::setItem(int row, int column, QTableWidgetItem *item)
{
    int i = tableIndex(row, column);
    if (i < 0 || i >= table.count())
        return;
    QTableWidgetItem *oldItem = table.at(i);
    if (item == oldItem)
        return;

    // remove old
    if (oldItem)
        oldItem->model = 0;
    delete table.at(i);

    // set new
    if (item)
        item->model = this;
    table[i] = item;
    QModelIndex idx = QAbstractTableModel::index(row, column);
    emit dataChanged(idx, idx);
}

QTableWidgetItem *QTableModel::takeItem(int row, int column)
{
    long i = tableIndex(row, column);
    QTableWidgetItem *itm = table.value(i);
    if (itm) {
        itm->model = 0;
        table[i] = 0;
    }
    return itm;
}

QTableWidgetItem *QTableModel::item(int row, int column) const
{
    return table.value(tableIndex(row, column));
}

QTableWidgetItem *QTableModel::item(const QModelIndex &index) const
{
    if (!isValid(index))
        return 0;
    return table.at(tableIndex(index.row(), index.column()));
}

void QTableModel::removeItem(QTableWidgetItem *item)
{
    int i = table.indexOf(item);
    if (i != -1) {
        table[i] = 0;
        QModelIndex idx = index(item);
        emit dataChanged(idx, idx);
        return;
    }

    i = vertical.indexOf(item);

    if (i != -1) {
        vertical[i] = 0;
        emit headerDataChanged(Qt::Vertical, i, i);
        return;
    }
    i = horizontal.indexOf(item);
    if (i != -1) {
        horizontal[i] = 0;
        emit headerDataChanged(Qt::Horizontal, i, i);
        return;
    }
}

void QTableModel::setHorizontalHeaderItem(int section, QTableWidgetItem *item)
{
    if (section < 0 || section >= horizontal.count())
        return;
    QTableWidgetItem *oldItem = horizontal.at(section);
    if (item == oldItem)
        return;

    if (oldItem)
        oldItem->model = 0;
    delete oldItem;

    if (item)
        item->model = this;
    horizontal[section] = item;
    emit headerDataChanged(Qt::Horizontal, section, section);
}

QTableWidgetItem *QTableModel::takeHorizontalHeaderItem(int section)
{
    if (section < 0 || section >= horizontal.count())
        return 0;
    QTableWidgetItem *itm = horizontal.at(section);
    if (itm) {
        itm->model = 0;
        horizontal[section] = 0;
    }
    return itm;
}

void QTableModel::setVerticalHeaderItem(int section, QTableWidgetItem *item)
{
    if (section < 0 || section >= vertical.count())
        return;
    QTableWidgetItem *oldItem = vertical.at(section);
    if (item == oldItem)
        return;

    if (oldItem)
        oldItem->model = 0;
    delete oldItem;

    if (item)
        item->model = this;
    vertical[section] = item;
    emit headerDataChanged(Qt::Vertical, section, section);
}

QTableWidgetItem *QTableModel::takeVerticalHeaderItem(int section)
{
    if (section < 0 || section >= vertical.count())
        return 0;
    QTableWidgetItem *itm = vertical.at(section);
    if (itm) {
        itm->model = 0;
        vertical[section] = 0;
    }
    return itm;
}

QTableWidgetItem *QTableModel::horizontalHeaderItem(int section)
{
    return horizontal.value(section);
}

QTableWidgetItem *QTableModel::verticalHeaderItem(int section)
{
    return vertical.value(section);
}

QModelIndex QTableModel::index(const QTableWidgetItem *item) const
{
    if (!item)
        return QModelIndex();
    int i = table.indexOf(const_cast<QTableWidgetItem*>(item));
    if (i < 0)
        return QModelIndex();
    int row = i / columnCount();
    int col = i % columnCount();
    return QAbstractTableModel::index(row, col);
}

void QTableModel::setRowCount(int rows)
{
    int rc = vertical.count();
    if (rc == rows)
        return;
    if (rc < rows)
        insertRows(qMax(rc, 0), rows - rc);
    else
        removeRows(qMax(rows, 0), rc - rows);
}

void QTableModel::setColumnCount(int columns)
{
    int cc = horizontal.count();
    if (cc == columns)
        return;
    if (cc < columns)
        insertColumns(qMax(cc, 0), columns - cc);
    else
        removeColumns(qMax(columns, 0), cc - columns);
}

int QTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : vertical.count();
}

int QTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : horizontal.count();
}

QVariant QTableModel::data(const QModelIndex &index, int role) const
{
    QTableWidgetItem *itm = item(index);
    if (itm)
        return itm->data(role);
    return QVariant();
}

bool QTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    QTableWidgetItem *itm = item(index);
    if (itm) {
        itm->setData(role, value);
        return true;
    }

    // don't create dummy table items for empty values
    if (!value.isValid())
        return false;

    QTableWidget *view = qobject_cast<QTableWidget*>(QObject::parent());
    if (!view)
        return false;

    itm = createItem();
    itm->setData(role, value);
    view->setItem(index.row(), index.column(), itm);
    return true;
}

Qt::ItemFlags QTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsDropEnabled;

    QTableWidgetItem *itm = item(index);
    if (itm)
        return itm->flags();
    return (Qt::ItemIsEditable
            |Qt::ItemIsSelectable
            |Qt::ItemIsUserCheckable
            |Qt::ItemIsEnabled
            |Qt::ItemIsDragEnabled
            |Qt::ItemIsDropEnabled);
}

void QTableModel::sort(int column, Qt::SortOrder order)
{
    QVector<QPair<QTableWidgetItem*, int> > sortable;
    QVector<int> unsortable;

    sortable.reserve(rowCount());
    unsortable.reserve(rowCount());

    for (int row = 0; row < rowCount(); ++row) {
        QTableWidgetItem *itm = item(row, column);

        if (itm)
            sortable.append(QPair<QTableWidgetItem*,int>(itm, row));
        else
            unsortable.append(row);
    }

    LessThan compare = (order == Qt::AscendingOrder ? &itemLessThan : &itemGreaterThan);
    qSort(sortable.begin(), sortable.end(), compare);

    emit layoutAboutToBeChanged();

    QVector<QTableWidgetItem*> sorted_table(table.count());
    QModelIndexList from;
    QModelIndexList to;
    for (int i = 0; i < rowCount(); ++i) {
        int r = (i < sortable.count()
                 ? sortable.at(i).second
                 : unsortable.at(i - sortable.count()));
        for (int c = 0; c < columnCount(); ++c) {
            QTableWidgetItem *itm = item(r, c);
            sorted_table[tableIndex(i, c)] = itm;
            from << createIndex(r, c, 0);
            to << createIndex(i, c, 0);
        }
    }

    table = sorted_table;
    changePersistentIndexList(from, to); // ### slow

    emit layoutChanged();
}

bool QTableModel::itemLessThan(const QPair<QTableWidgetItem*,int> &left,
                               const QPair<QTableWidgetItem*,int> &right)
{
    return *(left.first) < *(right.first);
}

bool QTableModel::itemGreaterThan(const QPair<QTableWidgetItem*,int> &left,
                                  const QPair<QTableWidgetItem*,int> &right)
{
    return !(*(left .first) < *(right.first));
}

QVariant QTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QTableWidgetItem *itm = 0;
    if (section < 0)
        return QVariant();
    if (orientation == Qt::Horizontal && section < horizontal.count())
        itm = horizontal.at(section);
    else if (section < vertical.count())
        itm = vertical.at(section);
    if (itm)
        return itm->data(role);
    if (role == Qt::DisplayRole)
        return section + 1;
    return QVariant();
}

bool QTableModel::setHeaderData(int section, Qt::Orientation orientation,
                                const QVariant &value, int role)
{
    if (section < 0 ||
       (orientation == Qt::Horizontal && horizontal.size() <= section) ||
       (orientation == Qt::Vertical && vertical.size() <= section))
    return false;

    QTableWidgetItem *itm = 0;
    if (orientation == Qt::Horizontal)
        itm = horizontal.at(section);
    else
        itm = vertical.at(section);
    if (itm) {
        itm->setData(role, value);
        return true;
    }
    return false;
}

bool QTableModel::isValid(const QModelIndex &index) const
{
    return index.isValid() && index.row() < vertical.count() && index.column() < horizontal.count();
}

void QTableModel::clear()
{
    for (int j = 0; j < vertical.count(); ++j) {
        if (vertical.at(j)) {
            vertical.at(j)->model = 0;
            delete vertical.at(j);
            vertical[j] = 0;
        }
    }
    for (int k = 0; k < horizontal.count(); ++k) {
        if (horizontal.at(k)) {
            horizontal.at(k)->model = 0;
            delete horizontal.at(k);
            horizontal[k] = 0;
        }
    }
    clearContents();
}

void QTableModel::clearContents()
{
    for (int i = 0; i < table.count(); ++i) {
        if (table.at(i)) {
            table.at(i)->model = 0;
            delete table.at(i);
            table[i] = 0;
        }
    }
    reset();
}

void QTableModel::itemChanged(QTableWidgetItem *item)
{
    if (!item)
        return;
    if (item->flags() & ItemIsVerticalHeaderItem) {
        int row = vertical.indexOf(item);
        if (row >= 0)
            emit headerDataChanged(Qt::Vertical, row, row);
    } else if (item->flags() & ItemIsHorizontalHeaderItem) {
        int column = horizontal.indexOf(item);
        if (column >= 0)
            emit headerDataChanged(Qt::Horizontal, column, column);
    } else {
        QModelIndex idx = index(item);
        if (idx.isValid())
            emit dataChanged(idx, idx);
    }
}

QStringList QTableModel::mimeTypes() const
{
    const QTableWidget *view = ::qobject_cast<const QTableWidget*>(QObject::parent());
    return view->mimeTypes();
}

QMimeData *QTableModel::internalMimeData()  const
{
    return QAbstractItemModel::mimeData(cachedIndexes);
}

QMimeData *QTableModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QTableWidgetItem*> items;
    for (int i = 0; i < indexes.count(); ++i)
        items << item(indexes.at(i));
    const QTableWidget *view = ::qobject_cast<const QTableWidget*>(QObject::parent());

    // cachedIndexes is a little hack to avoid copying from QModelIndexList to QList<QTreeWidgetItem*> and back again in the view
    cachedIndexes = indexes;
    QMimeData *mimeData = view->mimeData(items);
    cachedIndexes.clear();
    return mimeData;
}

bool QTableModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                              int row , int column, const QModelIndex &index)
{
    if (index.isValid()) {
        row = index.row();
        column = index.column();
    }else if (row == -1 || column == -1) {  // The user dropped outside the table.
        row = rowCount();
        column = 0;
    }

    QTableWidget *view = ::qobject_cast<QTableWidget*>(QObject::parent());
    return view->dropMimeData(row, column, data, action);
}

Qt::DropActions QTableModel::supportedDropActions() const
{
    const QTableWidget *view = ::qobject_cast<const QTableWidget*>(QObject::parent());
    return view->supportedDropActions();
}

/*!
    \class QTableWidgetSelectionRange

    \brief The QTableWidgetSelectionRange class provides a container for
    storing a selection range in a QTableWidget.

    \ingroup model-view

    The QTableWidgetSelectionRange class stores the top left and bottom
    right rows and columns of a selection range in a table. The
    selections in the table may consist of several selection ranges.

    \sa QTableWidget
*/

/*!
    Constructs an table selection range, i.e. a range
    whose rowCount() and columnCount() are 0.
*/
QTableWidgetSelectionRange::QTableWidgetSelectionRange()
    : top(-1), left(-1), bottom(-2), right(-2)
{
}

/*!
    Constructs the table selection range from the given \a top, \a
    left, \a bottom and \a right table rows and columns.

    \sa topRow(), leftColumn(), bottomRow(), rightColumn()
*/
QTableWidgetSelectionRange::QTableWidgetSelectionRange(int top, int left, int bottom, int right)
    : top(top), left(left), bottom(bottom), right(right)
{
}

/*!
    Constructs a the table selection range by copying the given \a
    other table selection range.
*/
QTableWidgetSelectionRange::QTableWidgetSelectionRange(const QTableWidgetSelectionRange &other)
    : top(other.top), left(other.left), bottom(other.bottom), right(other.right)
{
}

/*!
    Destroys the table selection range.
*/
QTableWidgetSelectionRange::~QTableWidgetSelectionRange()
{
}

/*!
    \fn int QTableWidgetSelectionRange::topRow() const

    Returns the top row of the range.

    \sa bottomRow(), leftColumn(), rowCount()
*/

/*!
    \fn int QTableWidgetSelectionRange::bottomRow() const

    Returns the bottom row of the range.

    \sa topRow(), rightColumn(), rowCount()
*/

/*!
    \fn int QTableWidgetSelectionRange::leftColumn() const

    Returns the left column of the range.

    \sa rightColumn(), topRow(), columnCount()
*/

/*!
    \fn int QTableWidgetSelectionRange::rightColumn() const

    Returns the right column of the range.

    \sa leftColumn(), bottomRow(), columnCount()
*/

/*!
    \since 4.1
    \fn int QTableWidgetSelectionRange::rowCount() const

    Returns the number of rows in the range.

    This is equivalent to bottomRow() - topRow() + 1.

    \sa columnCount(), topRow(), bottomRow()
*/

/*!
    \since 4.1
    \fn int QTableWidgetSelectionRange::columnCount() const

    Returns the number of columns in the range.

    This is equivalent to rightColumn() - leftColumn() + 1.

    \sa rowCount(), leftColumn(), rightColumn()
*/

/*!
    \class QTableWidgetItem
    \brief The QTableWidgetItem class provides an item for use with the
    QTableWidget class.

    \ingroup model-view

    Table items are used to hold pieces of information for table widgets.
    Items usually contain text, icons, or checkboxes

    The QTableWidgetItem class is a convenience class that replaces the
    \c QTableItem class in Qt 3. It provides an item for use with
    the QTableWidget class.

    Top-level items are constructed without a parent then inserted at the
    position specified by a pair of row and column numbers:

    \quotefile snippets/qtablewidget-using/mainwindow.cpp
    \skipto QTableWidgetItem *newItem
    \printuntil tableWidget->setItem(

    Each item can have its own background color which is set with
    the setBackgroundColor() function. The current background color can be
    found with backgroundColor().
    The text label for each item can be rendered with its own font and text
    color. These are specified with the setFont() and setTextColor() functions,
    and read with font() and textColor().

    By default, items are enabled, editable, selectable, checkable, and can be
    used both as the source of a drag and drop operation and as a drop target.
    Each item's flags can be changed by calling setFlags() with the appropriate
    value (see \l{Qt::ItemFlags}). Checkable items can be checked and unchecked
    with the setChecked() function. The corresponding checked() function
    indicates whether the item is currently checked.

    \section1 Subclassing

    When subclassing QTableWidgetItem to provide custom items, it is possible to
    define new types for them so that they can be distinguished from standard
    items. The constructors for subclasses that require this feature need to
    call the base class constructor with a new type value equal to or greater
    than \l UserType.

    \sa QTableWidget, {Model/View Programming}, QListWidgetItem, QTreeWidgetItem
*/

/*!
  \fn QSize QTableWidgetItem::sizeHint() const
  \since 4.1

  Returns the size hint set for the table item.
*/

/*!
  \fn void QTableWidgetItem::setSizeHint(const QSize &size)
  \since 4.1

  Sets the size hint for the table item to be \a size.
  If no size hint is set, the item delegate will compute the
  size hint based on the item data.
*/

/*!
    \fn Qt::CheckState QTableWidgetItem::checkState() const

    Returns the checked state of the table item.

    \sa flags()
*/

/*!
    \fn void QTableWidgetItem::setCheckState(Qt::CheckState state)

    Sets the check state of the table item to be \a state.
*/

/*!
    \fn QTableWidget *QTableWidgetItem::tableWidget() const

    Returns the table widget that contains the item.
*/

/*!
    \fn Qt::ItemFlags QTableWidgetItem::flags() const

    Returns the flags used to describe the item. These determine whether
    the item can be checked, edited, and selected.

    \sa setFlags()
*/

/*!
    \fn void QTableWidgetItem::setFlags(Qt::ItemFlags flags)

    Sets the flags for the item to the given \a flags. These determine whether
    the item can be selected or modified.

    \sa flags()
*/
void QTableWidgetItem::setFlags(Qt::ItemFlags aflags) {
    itemFlags = aflags;
    if (model)
        model->itemChanged(this);
}


/*!
    \fn QString QTableWidgetItem::text() const

    Returns the item's text.

    \sa setText()
*/

/*!
    \fn void QTableWidgetItem::setText(const QString &text)

    Sets the item's text to the \a text specified.

    \sa text() setFont() setTextColor()
*/

/*!
    \fn QIcon QTableWidgetItem::icon() const

    Returns the item's icon.

    \sa setIcon(), {QAbstractItemView::iconSize}{iconSize}
*/

/*!
    \fn void QTableWidgetItem::setIcon(const QIcon &icon)

    Sets the item's icon to the \a icon specified.

    \sa icon(), setText(), {QAbstractItemView::iconSize}{iconSize}
*/

/*!
    \fn QString QTableWidgetItem::statusTip() const

    Returns the item's status tip.

    \sa setStatusTip()
*/

/*!
    \fn void QTableWidgetItem::setStatusTip(const QString &statusTip)

    Sets the item's status tip to the string specified by \a statusTip.

    \sa statusTip() setToolTip() setWhatsThis()
*/

/*!
    \fn QString QTableWidgetItem::toolTip() const

    Returns the item's tooltip.

    \sa setToolTip()
*/

/*!
    \fn void QTableWidgetItem::setToolTip(const QString &toolTip)

    Sets the item's tooltip to the string specified by \a toolTip.

    \sa toolTip() setStatusTip() setWhatsThis()
*/

/*!
    \fn QString QTableWidgetItem::whatsThis() const

    Returns the item's "What's This?" help.

    \sa setWhatsThis()
*/

/*!
    \fn void QTableWidgetItem::setWhatsThis(const QString &whatsThis)

    Sets the item's "What's This?" help to the string specified by \a whatsThis.

    \sa whatsThis() setStatusTip() setToolTip()
*/

/*!
    \fn QFont QTableWidgetItem::font() const

    Returns the font used to render the item's text.

    \sa setFont()
*/

/*!
    \fn void QTableWidgetItem::setFont(const QFont &font)

    Sets the font used to display the item's text to the given \a font.

    \sa font() setText() setTextColor()
*/

/*!
    \fn QColor QTableWidgetItem::backgroundColor() const

    Returns the color used to render the item's background.

    \sa textColor() setBackgroundColor()
*/

/*!
    \fn void QTableWidgetItem::setBackgroundColor(const QColor &color)

    Sets the item's background color to the specified \a color.

    \sa backgroundColor() setTextColor()
*/

/*!
    \fn QColor QTableWidgetItem::textColor() const

    Returns the color used to render the item's text.

    \sa backgroundColor() setTextColor()
*/

/*!
    \fn void QTableWidgetItem::setTextColor(const QColor &color)

    Sets the color used to display the item's text to the given \a color.

    \sa textColor() setFont() setText()
*/

/*!
    \fn int QTableWidgetItem::textAlignment() const

    Returns the text alignment for the item's text.

    \sa Qt::Alignment
*/

/*!
    \fn void QTableWidgetItem::setTextAlignment(int alignment)

    Sets the text alignment for the item's text to the \a alignment
    specified.

    \sa Qt::Alignment
*/

/*!
    Constructs a table item of the specified \a type that does not belong
    to any table.

    \sa type()
*/
QTableWidgetItem::QTableWidgetItem(int type)
    :  rtti(type), view(0), model(0),
      itemFlags(Qt::ItemIsEditable
                |Qt::ItemIsSelectable
                |Qt::ItemIsUserCheckable
                |Qt::ItemIsEnabled
                |Qt::ItemIsDragEnabled
                |Qt::ItemIsDropEnabled)
{
}

/*!
    Constructs a table item with the given \a text.

    \sa type()
*/
QTableWidgetItem::QTableWidgetItem(const QString &text, int type)
    :  rtti(type), view(0), model(0),
      itemFlags(Qt::ItemIsEditable
                |Qt::ItemIsSelectable
                |Qt::ItemIsUserCheckable
                |Qt::ItemIsEnabled
                |Qt::ItemIsDragEnabled
                |Qt::ItemIsDropEnabled)
{
    setData(Qt::DisplayRole, text);
}

/*!
    Constructs a table item with the given \a icon and \a text.

    \sa type()
*/
QTableWidgetItem::QTableWidgetItem(const QIcon &icon, const QString &text, int type)
    :  rtti(type), view(0), model(0),
       itemFlags(Qt::ItemIsEditable
                |Qt::ItemIsSelectable
                |Qt::ItemIsUserCheckable
                |Qt::ItemIsEnabled
                |Qt::ItemIsDragEnabled
                |Qt::ItemIsDropEnabled)
{
    setData(Qt::DecorationRole, icon);
    setData(Qt::DisplayRole, text);
}

/*!
    Destroys the table item.
*/
QTableWidgetItem::~QTableWidgetItem()
{
    if (model)
        model->removeItem(this);
}

/*!
    Creates a copy of the item.
*/
QTableWidgetItem *QTableWidgetItem::clone() const
{
    return new QTableWidgetItem(*this);
}

/*!
    Sets the item's data for the given \a role to the specified \a value.

    \sa Qt::ItemDataRole, data()
*/
void QTableWidgetItem::setData(int role, const QVariant &value)
{
    bool found = false;
    role = (role == Qt::EditRole ? Qt::DisplayRole : role);
    for (int i = 0; i < values.count(); ++i) {
        if (values.at(i).role == role) {
            if (values[i].value == value)
                return;

            values[i].value = value;
            found = true;
            break;
        }
    }
    if (!found)
        values.append(QWidgetItemData(role, value));
    if (model)
        model->itemChanged(this);
}

/*!
    Returns the item's data for the given \a role.
*/
QVariant QTableWidgetItem::data(int role) const
{
    role = (role == Qt::EditRole ? Qt::DisplayRole : role);
    for (int i = 0; i < values.count(); ++i)
        if (values.at(i).role == role)
            return values.at(i).value;
    return QVariant();
}

/*!
    Returns true if the item is less than the \a other item; otherwise returns
    false.
*/
bool QTableWidgetItem::operator<(const QTableWidgetItem &other) const
{
    const QVariant v1 = data(Qt::DisplayRole), v2 = other.data(Qt::DisplayRole);
    if(v1.canConvert(QVariant::LongLong) && v2.canConvert(QVariant::LongLong))
        return v1.toLongLong() < v2.toLongLong();
    return v1.toString() < v2.toString();
}

#ifndef QT_NO_DATASTREAM

/*!
    Reads the item from stream \a in.

    \sa write()
*/
void QTableWidgetItem::read(QDataStream &in)
{
    in >> values;
}

/*!
    Writes the item to stream \a out.

    \sa read()
*/
void QTableWidgetItem::write(QDataStream &out) const
{
    out << values;
}

/*!
    \relates QTableWidgetItem

    Reads a table widget item from stream \a in into \a item.

    This operator uses QTableWidgetItem::read().

    \sa {Format of the QDataStream Operators}
*/
QDataStream &operator>>(QDataStream &in, QTableWidgetItem &item)
{
    item.read(in);
    return in;
}

/*!
    \relates QTableWidgetItem

    Writes the table widget item \a item to stream \a out.

    This operator uses QTableWidgetItem::write().

    \sa {Format of the QDataStream Operators}
*/
QDataStream &operator<<(QDataStream &out, const QTableWidgetItem &item)
{
    item.write(out);
    return out;
}

#endif // QT_NO_DATASTREAM

/*!
    \since 4.1

    Constructs a copy of \a other. Note that type() and tableWidget()
    are not copied.

    This function is useful when reimplementing clone().

    \sa data(), flags()
*/
QTableWidgetItem::QTableWidgetItem(const QTableWidgetItem &other)
    : rtti(Type), values(other.values), view(0), model(0),
      itemFlags(other.itemFlags)
{
}

/*!
    Assigns \a other's data and flags to this item. Note that type()
    and tableWidget() are not copied.

    This function is useful when reimplementing clone().

    \sa data(), flags()
*/
QTableWidgetItem &QTableWidgetItem::operator=(const QTableWidgetItem &other)
{
    values = other.values;
    itemFlags = other.itemFlags;
    return *this;
}

/*!
    \class QTableWidget
    \brief The QTableWidget class provides an item-based table view with a default model.

    \ingroup model-view
    \mainclass

    Table widgets provide standard table display facilities for applications.
    The items in a QTableWidget are provided by QTableWidgetItem.

    If you want a table that uses your own data model you should
    use QTableView rather than this class.

    Table widgets can be constructed with the required numbers of rows and
    columns:

    \quotefile snippets/qtablewidget-using/mainwindow.cpp
    \skipto tableWidget = new
    \printuntil tableWidget = new

    Alternatively, tables can be constructed without a given size and resized
    later:

    \quotefile snippets/qtablewidget-resizing/mainwindow.cpp
    \skipto tableWidget = new
    \printuntil tableWidget = new
    \skipto tableWidget->setRowCount(
    \printuntil tableWidget->setColumnCount(

    Items are created ouside the table (with no parent widget) and inserted
    into the table with setItem():

    \skipto QTableWidgetItem *newItem
    \printuntil tableWidget->setItem(

    Tables can be given both horizontal and vertical headers. The simplest way
    to create the headers is to supply a list of strings to the
    setHorizontalHeaderLabels() and setVerticalHeaderLabels() functions. These
    will provide simple textual headers for the table's columns and rows.
    More sophisticated headers can be created from existing table items
    that are usually constructed outside the table. For example, we can
    construct a table item with an icon and aligned text, and use it as the
    header for a particular column:

    \quotefile snippets/qtablewidget-using/mainwindow.cpp
    \skipto QTableWidgetItem *cubesHeaderItem
    \printuntil cubesHeaderItem->setTextAlignment

    The number of rows in the table can be found with rowCount(), and the
    number of columns with columnCount(). The table can be cleared with the
    clear() function.

    \table 100%
    \row \o \inlineimage windowsxp-tableview.png Screenshot of a Windows XP style table widget
         \o \inlineimage macintosh-tableview.png Screenshot of a Macintosh style table widget
         \o \inlineimage plastique-tableview.png Screenshot of a Plastique style table widget
    \row \o A \l{Windows XP Style Widget Gallery}{Windows XP style} table widget.
         \o A \l{Macintosh Style Widget Gallery}{Macintosh style} table widget.
         \o A \l{Plastique Style Widget Gallery}{Plastique style} table widget.
    \endtable

    \sa QTableWidgetItem, QTableView, {Model/View Programming}
*/

/*!
    \property QTableWidget::rowCount
    \brief the number of rows in the table
*/

/*!
    \property QTableWidget::columnCount
    \brief the number of columns in the table
*/

/*!
    \property QTableWidget::sortingEnabled
    \brief whether the items in the table can be sorted
    by clicking on the horizontal header.
*/

class QTableWidgetPrivate : public QTableViewPrivate
{
    Q_DECLARE_PUBLIC(QTableWidget)
public:
    QTableWidgetPrivate() : QTableViewPrivate() {}
    inline QTableModel *model() const { return ::qobject_cast<QTableModel*>(q_func()->model()); }
    void setup();

    // view signals
    void _q_emitItemPressed(const QModelIndex &index);
    void _q_emitItemClicked(const QModelIndex &index);
    void _q_emitItemDoubleClicked(const QModelIndex &index);
    void _q_emitItemActivated(const QModelIndex &index);
    void _q_emitItemEntered(const QModelIndex &index);
    // model signals
    void _q_emitItemChanged(const QModelIndex &index);
    // selection signals
    void _q_emitCurrentItemChanged(const QModelIndex &previous, const QModelIndex &current);
    // sorting
    void _q_sort();
};

void QTableWidgetPrivate::setup()
{
    Q_Q(QTableWidget);
    // view signals
    QObject::connect(q, SIGNAL(pressed(QModelIndex)), q, SLOT(_q_emitItemPressed(QModelIndex)));
    QObject::connect(q, SIGNAL(clicked(QModelIndex)), q, SLOT(_q_emitItemClicked(QModelIndex)));
    QObject::connect(q, SIGNAL(doubleClicked(QModelIndex)),
                     q, SLOT(_q_emitItemDoubleClicked(QModelIndex)));
    QObject::connect(q, SIGNAL(activated(QModelIndex)), q, SLOT(_q_emitItemActivated(QModelIndex)));
    QObject::connect(q, SIGNAL(entered(QModelIndex)), q, SLOT(_q_emitItemEntered(QModelIndex)));
    // model signals
    QObject::connect(model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                     q, SLOT(_q_emitItemChanged(QModelIndex)));
    // selection signals
    QObject::connect(q->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                     q, SLOT(_q_emitCurrentItemChanged(QModelIndex,QModelIndex)));
    QObject::connect(q->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                     q, SIGNAL(itemSelectionChanged()));
    // sorting
    QObject::connect(model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), q, SLOT(_q_sort()));
    QObject::connect(model(), SIGNAL(rowsInserted(QModelIndex,int,int)), q, SLOT(_q_sort()));
    QObject::connect(model(), SIGNAL(columnsRemoved(QModelIndex,int,int)), q, SLOT(_q_sort()));
}

void QTableWidgetPrivate::_q_emitItemPressed(const QModelIndex &index)
{
    Q_Q(QTableWidget);
    if (QTableWidgetItem *item = model()->item(index))
        emit q->itemPressed(item);
    emit q->cellPressed(index.row(), index.column());
}

void QTableWidgetPrivate::_q_emitItemClicked(const QModelIndex &index)
{
    Q_Q(QTableWidget);
    if (QTableWidgetItem *item = model()->item(index))
        emit q->itemClicked(item);
    emit q->cellClicked(index.row(), index.column());
}

void QTableWidgetPrivate::_q_emitItemDoubleClicked(const QModelIndex &index)
{
    Q_Q(QTableWidget);
    if (QTableWidgetItem *item = model()->item(index))
        emit q->itemDoubleClicked(item);
    emit q->cellDoubleClicked(index.row(), index.column());
}

void QTableWidgetPrivate::_q_emitItemActivated(const QModelIndex &index)
{
    Q_Q(QTableWidget);
    if (QTableWidgetItem *item = model()->item(index))
        emit q->itemActivated(item);
    emit q->cellActivated(index.row(), index.column());
}

void QTableWidgetPrivate::_q_emitItemEntered(const QModelIndex &index)
{
    Q_Q(QTableWidget);
    if (QTableWidgetItem *item = model()->item(index))
        emit q->itemEntered(item);
}

void QTableWidgetPrivate::_q_emitItemChanged(const QModelIndex &index)
{
    Q_Q(QTableWidget);
    if (QTableWidgetItem *item = model()->item(index))
        emit q->itemChanged(item);
    emit q->cellChanged(index.row(), index.column());
}

void QTableWidgetPrivate::_q_emitCurrentItemChanged(const QModelIndex &current,
                                                 const QModelIndex &previous)
{
    Q_Q(QTableWidget);
    QTableWidgetItem *currentItem = model()->item(current);
    QTableWidgetItem *previousItem = model()->item(previous);
    if (currentItem || previousItem)
        emit q->currentItemChanged(currentItem, previousItem);
    emit q->currentCellChanged(current.row(), current.column(), previous.row(), previous.column());
}

void QTableWidgetPrivate::_q_sort()
{
    Q_Q(QTableWidget);
    if (sortingEnabled) {
        int column = q->horizontalHeader()->sortIndicatorSection();
        Qt::SortOrder order = q->horizontalHeader()->sortIndicatorOrder();
        model()->sort(column, order);
    }
}

/*!
    \fn void QTableWidget::itemPressed(QTableWidgetItem *item)

    This signal is emitted whenever an item in the table is pressed.
    The \a item specified is the item that was pressed.
*/

/*!
    \fn void QTableWidget::itemClicked(QTableWidgetItem *item)

    This signal is emitted whenever an item in the table is clicked.
    The \a item specified is the item that was clicked.
*/

/*!
    \fn void QTableWidget::itemDoubleClicked(QTableWidgetItem *item)

    This signal is emitted whenever an item in the table is double
    clicked. The \a item specified is the item that was double clicked.
*/

/*!
    \fn void QTableWidget::itemActivated(QTableWidgetItem *item)

    This signal is emitted when the specified \a item has been activated
*/

/*!
    \fn void QTableWidget::itemEntered(QTableWidgetItem *item)

    This signal is emitted when the mouse cursor enters an item. The
    \a item is the item entered.

    This signal is only emitted when mouseTracking is turned on, or when a
    mouse button is pressed while moving into an item.
*/

/*!
    \fn void QTableWidget::itemChanged(QTableWidgetItem *item)

    This signal is emitted whenever the data of \a item has changed.
*/

/*!
    \fn void QTableWidget::currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous)

    This signal is emitted whenever the current item changes. The \a
    previous item is the item that previously had the focus, \a
    current is the new current item.
*/

/*!
    \fn void QTableWidget::itemSelectionChanged()

    This signal is emitted whenever the selection changes.

    \sa selectedItems() isItemSelected()
*/


/*!
  \since 4.1
  \fn void QTableWidget::cellPressed(int row, int column)

  This signal is emitted whenever a cell the table is pressed.
  The \a row and \a column specified is the cell that was pressed.
*/

/*!
  \since 4.1
  \fn void QTableWidget::cellClicked(int row, int column)

  This signal is emitted whenever a cell in the table is clicked.
  The \a row and \a column specified is the cell that was clicked.
*/

/*!
  \since 4.1
  \fn void QTableWidget::cellDoubleClicked(int row, int column)

  This signal is emitted whenever a cell in the table is double
  clicked. The \a row and \a column specified is the cell that was
  double clicked.
*/

/*!
  \since 4.1
  \fn void QTableWidget::cellActivated(int row, int column)

  This signal is emitted when the cell specified  by \a row and \a column
  has been activated
*/

/*!
  \since 4.1
  \fn void QTableWidget::cellEntered(int row, int column)

  This signal is emitted when the mouse cursor enters a cell. The
  cell is specified by \a row and \a column.

  This signal is only emitted when mouseTracking is turned on, or when a
  mouse button is pressed while moving into an item.
*/

/*!
  \since 4.1
  \fn void QTableWidget::cellChanged(int row, int column)

  This signal is emitted whenever the data of the item in the cell
  specidied by \a row and \a column has changed.
*/

/*!
  \since 4.1
  \fn void QTableWidget::currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)

  This signal is emitted whenever the current cell changes. The cell
  specified by \a previousRow and \a previousColumn is the cell that
  previously had the focus, the cell specified by \a currentRow and \a
  currentColumn is the new current cell.
*/


/*!
    \fn QTableWidgetItem *QTableWidget::itemAt(int ax, int ay) const

    Returns the item at the position (\a{ax}, \a{ay}) in the table
    widget's coordinate system, or returns 0 if the specified point is not
    covered by an item in the table widget.

    \sa item()
*/

/*!
    \enum QTableWidgetItem::ItemType

    This enum describes the types that are used to describe table widget items.

    \value Type     The default type for table widget items.
    \value UserType The minimum value for custom types. Values below UserType are
                    reserved by Qt.

    You can define new user types in QTableWidgetItem subclasses to ensure that
    custom items are treated specially.

    \sa type()
*/

/*!
    \fn int QTableWidgetItem::type() const

    Returns the type passed to the QTableWidgetItem constructor.
*/

/*!
    Creates a new table view with the given \a parent.
*/
QTableWidget::QTableWidget(QWidget *parent)
    : QTableView(*new QTableWidgetPrivate, parent)
{
    Q_D(QTableWidget);
    QTableView::setModel(new QTableModel(0, 0, this));
    d->setup();
}

/*!
    Creates a new table view with the given \a rows and \a columns, and with the given \a parent.
*/
QTableWidget::QTableWidget(int rows, int columns, QWidget *parent)
    : QTableView(*new QTableWidgetPrivate, parent)
{
    Q_D(QTableWidget);
    QTableView::setModel(new QTableModel(rows, columns, this));
    d->setup();
}

/*!
    Destroys this QTableWidget.
*/
QTableWidget::~QTableWidget()
{
}

/*!
    Sets the number of rows in this table's model to \a rows. If
    this is less than rowCount(), the data in the unwanted rows
    is discarded.

    \sa setColumnCount()
*/
void QTableWidget::setRowCount(int rows)
{
    Q_D(QTableWidget);
    if (rows < 0)
        return;
    d->model()->setRowCount(rows);
}

/*!
  Returns the number of rows.
*/

int QTableWidget::rowCount() const
{
    Q_D(const QTableWidget);
    return d->model()->rowCount();
}

/*!
    Sets the number of columns in this table's model to \a columns. If
    this is less than columnCount(), the data in the unwanted columns
    is discarded.

    \sa setRowCount()
*/
void QTableWidget::setColumnCount(int columns)
{
    Q_D(QTableWidget);
    if (columns < 0)
        return;
    d->model()->setColumnCount(columns);
}

/*!
  Returns the number of columns.
*/

int QTableWidget::columnCount() const
{
    Q_D(const QTableWidget);
    return d->model()->columnCount();
}

/*!
  Returns the row for the \a item.
*/
int QTableWidget::row(const QTableWidgetItem *item) const
{
    Q_D(const QTableWidget);
    return d->model()->index(item).row();
}

/*!
  Returns the column for the \a item.
*/
int QTableWidget::column(const QTableWidgetItem *item) const
{
    Q_D(const QTableWidget);
    return d->model()->index(item).column();
}


/*!
    Returns the item for the given \a row and \a column if one has been set; otherwise
    returns 0.

    \sa setItem()
*/
QTableWidgetItem *QTableWidget::item(int row, int column) const
{
    Q_D(const QTableWidget);
    return d->model()->item(row, column);
}

/*!
    Sets the item for the given \a row and \a column to \a item.

    The table takes ownership of the item.

    \sa item() takeItem()
*/
void QTableWidget::setItem(int row, int column, QTableWidgetItem *item)
{
    Q_D(QTableWidget);
    if (item) {
        item->view = this;
        d->model()->setItem(row, column, item);
    } else {
        delete takeItem(row, column);
    }
}

/*!
    Removes the item at \a row and \a column from the table without deleting it.
*/
QTableWidgetItem *QTableWidget::takeItem(int row, int column)
{
    Q_D(QTableWidget);
    QTableWidgetItem *item = d->model()->takeItem(row, column);
    if (item)
        item->view = 0;
    return item;
}

/*!
  Returns the vertical header item for row \a row.
*/
QTableWidgetItem *QTableWidget::verticalHeaderItem(int row) const
{
    Q_D(const QTableWidget);
    return d->model()->verticalHeaderItem(row);
}

/*!
  Sets the vertical header item for row \a row to \a item.
*/
void QTableWidget::setVerticalHeaderItem(int row, QTableWidgetItem *item)
{
    Q_D(QTableWidget);
    if (item) {
        item->view = this;
        item->itemFlags = Qt::ItemFlags(int(item->itemFlags) |
                    QTableModel::ItemIsVerticalHeaderItem);
        d->model()->setVerticalHeaderItem(row, item);
    } else {
        delete takeVerticalHeaderItem(row);
    }
}

/*!
  \since 4.1
    Removes the vertical header item at \a row from the header without deleting it.
*/
QTableWidgetItem *QTableWidget::takeVerticalHeaderItem(int row)
{
    Q_D(QTableWidget);
    QTableWidgetItem *itm = d->model()->takeVerticalHeaderItem(row);
    if (itm) {
        itm->view = 0;
        itm->itemFlags &= ~QTableModel::ItemIsVerticalHeaderItem;
    }
    return itm;
}

/*!
  Returns the horizontal header item for column \a column.
*/
QTableWidgetItem *QTableWidget::horizontalHeaderItem(int column) const
{
    Q_D(const QTableWidget);
    return d->model()->horizontalHeaderItem(column);
}

/*!
  Sets the horizontal header item for column \a column to \a item.
*/
void QTableWidget::setHorizontalHeaderItem(int column, QTableWidgetItem *item)
{
    Q_D(QTableWidget);
    if (item) {
        item->view = this;
        item->itemFlags = Qt::ItemFlags(int(item->itemFlags) |
                    QTableModel::ItemIsHorizontalHeaderItem);
        d->model()->setHorizontalHeaderItem(column, item);
    } else {
        delete takeHorizontalHeaderItem(column);
    }
}

/*!
  \since 4.1
    Removes the horizontal header item at \a column from the header without deleting it.
*/
QTableWidgetItem *QTableWidget::takeHorizontalHeaderItem(int column)
{
    Q_D(QTableWidget);
    QTableWidgetItem *itm = d->model()->takeHorizontalHeaderItem(column);
    if (itm) {
        itm->view = 0;
        itm->itemFlags &= ~QTableModel::ItemIsHorizontalHeaderItem;
    }
    return itm;
}

/*!
  Sets the vertical header labels using \a labels.
*/
void QTableWidget::setVerticalHeaderLabels(const QStringList &labels)
{
    Q_D(QTableWidget);
    QTableModel *model = d->model();
    QTableWidgetItem *item = 0;
    for (int i = 0; i < model->rowCount() && i < labels.count(); ++i) {
        item = model->verticalHeaderItem(i);
        if (!item) {
            item = model->createItem();
            setVerticalHeaderItem(i, item);
        }
        item->setText(labels.at(i));
    }
}

/*!
  Sets the horizontal header labels using \a labels.
*/
void QTableWidget::setHorizontalHeaderLabels(const QStringList &labels)
{
    Q_D(QTableWidget);
    QTableModel *model = d->model();
    QTableWidgetItem *item = 0;
    for (int i = 0; i < model->columnCount() && i < labels.count(); ++i) {
        item = model->horizontalHeaderItem(i);
        if (!item) {
            item = model->createItem();
            setHorizontalHeaderItem(i, item);
        }
        item->setText(labels.at(i));
    }
}

/*!
    Returns the row of the current item.

    \sa currentColumn(), setCurrentCell()
*/
int QTableWidget::currentRow() const
{
    return currentIndex().row();
}

/*!
    Returns the column of the current item.

    \sa currentRow(), setCurrentCell()
*/
int QTableWidget::currentColumn() const
{
    return currentIndex().column();
}

/*!
    Returns the current item.

    \sa setCurrentItem()
*/
QTableWidgetItem *QTableWidget::currentItem() const
{
    Q_D(const QTableWidget);
    return d->model()->item(currentIndex());
}

/*!
    Sets the current item to \a item.

    Depending on the current selection mode, the item may also be selected.

    \sa currentItem(), setCurrentCell()
*/
void QTableWidget::setCurrentItem(QTableWidgetItem *item)
{
    Q_D(QTableWidget);
    setCurrentIndex(d->model()->index(item));
}

/*!
    \since 4.1

    Sets the current cell to be the cell at position (\a row, \a
    column).

    Depending on the current selection mode, the cell may also be selected.

    \sa setCurrentItem(), currentRow(), currentColumn()
*/
void QTableWidget::setCurrentCell(int row, int column)
{
    setCurrentIndex(model()->index(row, column, QModelIndex()));
}

/*!
  Sorts all the rows in the table widget based on \a column and \a order.
*/
void QTableWidget::sortItems(int column, Qt::SortOrder order)
{
    Q_D(QTableWidget);
    d->model()->sort(column, order);
    horizontalHeader()->setSortIndicator(column, order);
}

/*!
  \reimp
*/
void QTableWidget::setSortingEnabled(bool enable)
{
    QTableView::setSortingEnabled(enable);
}

/*!
  \reimp
*/
bool QTableWidget::isSortingEnabled() const
{
    return QTableView::isSortingEnabled();
}

/*!
  Starts editing the \a item if it is editable.
*/

void QTableWidget::editItem(QTableWidgetItem *item)
{
    Q_D(QTableWidget);
    if (!item)
        return;
    edit(d->model()->index(item));
}

/*!
  Opens an editor for the give \a item. The editor remains open after editing.

  \sa closePersistentEditor()
*/
void QTableWidget::openPersistentEditor(QTableWidgetItem *item)
{
    Q_D(QTableWidget);
    if (!item)
        return;
    QModelIndex index = d->model()->index(item);
    QAbstractItemView::openPersistentEditor(index);
}

/*!
  Closes the persistent editor for \a item.

  \sa openPersistentEditor()
*/
void QTableWidget::closePersistentEditor(QTableWidgetItem *item)
{
    Q_D(QTableWidget);
    if (!item)
        return;
    QModelIndex index = d->model()->index(item);
    QAbstractItemView::closePersistentEditor(index);
}

/*!
  \since 4.1

  Returns the widget displayed in the cell in the given \a row and \a column.

  \sa setCellWidget()
*/
QWidget *QTableWidget::cellWidget(int row, int column) const
{
    QModelIndex index = model()->index(row, column, QModelIndex());
    return QAbstractItemView::indexWidget(index);
}

/*!
  \since 4.1

  Sets the \a widget to be displayed in the cell in the given \a row and \a column.

  \sa cellWidget()
*/
void QTableWidget::setCellWidget(int row, int column, QWidget *widget)
{
    QModelIndex index = model()->index(row, column, QModelIndex());
    QAbstractItemView::setIndexWidget(index, widget);
}

/*!
  Returns true if the \a item is selected, otherwise returns false.
*/

bool QTableWidget::isItemSelected(const QTableWidgetItem *item) const
{
    Q_D(const QTableWidget);
    QModelIndex index = d->model()->index(item);
    return selectionModel()->isSelected(index);
}

/*!
  Selects or deselects \a item depending on \a select.
*/
void QTableWidget::setItemSelected(const QTableWidgetItem *item, bool select)
{
    Q_D(QTableWidget);
    QModelIndex index = d->model()->index(item);
    selectionModel()->select(index, select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);
}

/*!
  Selects or deselects the \a range depending on \a select.
*/
void QTableWidget::setRangeSelected(const QTableWidgetSelectionRange &range, bool select)
{
    if (!model()->hasIndex(range.topRow(), range.leftColumn(), rootIndex()) ||
        !model()->hasIndex(range.bottomRow(), range.rightColumn(), rootIndex()))
        return;

    QModelIndex topLeft = model()->index(range.topRow(), range.leftColumn(), rootIndex());
    QModelIndex bottomRight = model()->index(range.bottomRow(), range.rightColumn(), rootIndex());

    selectionModel()->select(QItemSelection(topLeft, bottomRight),
                             select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);
}

/*!
  Returns a list of all selected ranges.

  \sa QTableWidgetSelectionRange
*/

QList<QTableWidgetSelectionRange> QTableWidget::selectedRanges() const
{
    const QList<QItemSelectionRange> ranges = selectionModel()->selection();
    QList<QTableWidgetSelectionRange> result;
    for (int i = 0; i < ranges.count(); ++i)
        result.append(QTableWidgetSelectionRange(ranges.at(i).top(),
                                                 ranges.at(i).left(),
                                                 ranges.at(i).bottom(),
                                                 ranges.at(i).right()));
    return result;
}

/*!
  Returns a list of all selected items.
*/

QList<QTableWidgetItem*> QTableWidget::selectedItems()
{
    Q_D(QTableWidget);
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    QList<QTableWidgetItem*> items;
    for (int i = 0; i < indexes.count(); ++i) {
        QModelIndex index = indexes.at(i);
        if(isIndexHidden(index))
            continue;
        QTableWidgetItem *item = d->model()->item(index);
        if (item)
            items.append(item);
    }
    return items;
}

/*!
  Finds items that matches the \a text using the given \a flags.
*/

QList<QTableWidgetItem*> QTableWidget::findItems(const QString &text, Qt::MatchFlags flags) const
{
    Q_D(const QTableWidget);
    QModelIndexList indexes;
    for (int column = 0; column < columnCount(); ++column)
        indexes += d->model()->match(model()->index(0, column, QModelIndex()),
                                     Qt::DisplayRole, text, -1, flags);
    QList<QTableWidgetItem*> items;
    for (int i = 0; i < indexes.size(); ++i)
        items.append(d->model()->item(indexes.at(i)));
    return items;
}

/*!
  Returns the visual row of the given \a logicalRow.
*/

int QTableWidget::visualRow(int logicalRow) const
{
    return verticalHeader()->visualIndex(logicalRow);
}

/*!
  Returns the visual column of the given \a logicalColumn.
*/

int QTableWidget::visualColumn(int logicalColumn) const
{
    return horizontalHeader()->visualIndex(logicalColumn);
}

/*!
  \fn QTableWidgetItem *QTableWidget::itemAt(const QPoint &point) const

  Returns a pointer to the item at the given \a point, or returns 0 if
  the point is not covered by an item in the table widget.

  \sa item()
*/

QTableWidgetItem *QTableWidget::itemAt(const QPoint &p) const
{
    Q_D(const QTableWidget);
    return d->model()->item(indexAt(p));
}

/*!
  Returns the rectangle on the viewport occupied by the item at \a item.
*/
QRect QTableWidget::visualItemRect(const QTableWidgetItem *item) const
{
    Q_D(const QTableWidget);
    if (!item)
        return QRect();
    QModelIndex index = d->model()->index(const_cast<QTableWidgetItem*>(item));
    Q_ASSERT(index.isValid());
    return visualRect(index);
}

/*!
    Scrolls the view if necessary to ensure that the \a item is visible.
    The \a hint parameter specifies more precisely where the
    \a item should be located after the operation.
*/

void QTableWidget::scrollToItem(const QTableWidgetItem *item, QAbstractItemView::ScrollHint hint)
{
    Q_D(QTableWidget);
    if (!item)
        return;
    QModelIndex index = d->model()->index(const_cast<QTableWidgetItem*>(item));
    Q_ASSERT(index.isValid());
    QTableView::scrollTo(index, hint);
}

/*!
    Returns the item prototype used by the table.

    Copies of the item prototype are returned by the createItem()
    function.

    \sa setItemPrototype()
*/
const QTableWidgetItem *QTableWidget::itemPrototype() const
{
    Q_D(const QTableWidget);
    return d->model()->itemPrototype();
}

/*!
    Sets the item prototype for the table to the specified \a item.

    \sa itemPrototype()
*/
void QTableWidget::setItemPrototype(const QTableWidgetItem *item)
{
    Q_D(QTableWidget);
    d->model()->setItemPrototype(item);
}

/*!
  Inserts an empty row into the table at \a row.
*/
void QTableWidget::insertRow(int row)
{
    Q_D(QTableWidget);
    d->model()->insertRows(row);
}

/*!
  Inserts an empty column into the table at \a column.
*/
void QTableWidget::insertColumn(int column)
{
    Q_D(QTableWidget);
    d->model()->insertColumns(column);
}

/*!
  Removes the row \a row and all its items from the table.
*/
void QTableWidget::removeRow(int row)
{
    Q_D(QTableWidget);
    d->model()->removeRows(row);
}

/*!
  Removes the column \a column and all its items from the table.
*/
void QTableWidget::removeColumn(int column)
{
    Q_D(QTableWidget);
    d->model()->removeColumns(column);
}

/*!
  Removes all items in the view.
  This will also remove all selections.
  The table dimentions stay the same.
*/

void QTableWidget::clear()
{
    Q_D(QTableWidget);
    selectionModel()->clear();
    d->model()->clear();
}

/*!
  Removes all items not in the headers from the view.
  This will also remove all selections.
  The table dimentions stay the same.
*/
void QTableWidget::clearContents()
{
    Q_D(QTableWidget);
    selectionModel()->clear();
    d->model()->clearContents();
}

/*!
    Returns a list of MIME types that can be used to describe a list of
    tablewidget items.

    \sa mimeData()
*/
QStringList QTableWidget::mimeTypes() const
{
    return d_func()->model()->QAbstractTableModel::mimeTypes();
}

/*!
    Returns an object that contains a serialized description of the specified
    \a items. The format used to describe the items is obtained from the
    mimeTypes() function.

    If the list of items is empty, 0 is returned rather than a serialized
    empty list.
*/
QMimeData *QTableWidget::mimeData(const QList<QTableWidgetItem*>) const
{
    return d_func()->model()->internalMimeData();
}

/*!
    Handles the \a data supplied by a drag and drop operation that ended with
    the given \a action in the given \a row and \a column.

    \sa supportedDropActions()
*/
bool QTableWidget::dropMimeData(int row, int column, const QMimeData *data, Qt::DropAction action)
{
    QModelIndex idx;
#ifndef QT_NO_DRAGANDDROP
    if (dropIndicatorPosition() == QAbstractItemView::OnItem) {
        // QAbstractTableModel::dropMimeData will overwrite on the index if row == -1 and column == -1
        idx = model()->index(row, column);
        row = -1;
        column = -1;
    }
#endif
    return d_func()->model()->QAbstractTableModel::dropMimeData(data, action , row, column, idx);
}

/*!
  Returns the drop actions supported by this view.

  \sa Qt::DropActions
*/
Qt::DropActions QTableWidget::supportedDropActions() const
{
    return d_func()->model()->QAbstractTableModel::supportedDropActions();
}

/*!
  Returns a list of pointers to the items contained in the \a data object.
  If the object was not created by a QTreeWidget in the same process, the list
  is empty.

*/
QList<QTableWidgetItem*> QTableWidget::items(const QMimeData *data) const
{
    const QTableWidgetMimeData *twd = qobject_cast<const QTableWidgetMimeData*>(data);
    if (twd)
        return twd->items;
    return QList<QTableWidgetItem*>();
}

/*!
  Returns the QModelIndex assocated with the given \a item.
*/

QModelIndex QTableWidget::indexFromItem(QTableWidgetItem *item) const
{
    Q_D(const QTableWidget);
    return d->model()->index(item);
}

/*!
  Returns a pointer to the QTableWidgetItem assocated with the given \a index.
*/

QTableWidgetItem *QTableWidget::itemFromIndex(const QModelIndex &index) const
{
    Q_D(const QTableWidget);
    return d->model()->item(index);
}

/*!
    \internal
*/
void QTableWidget::setModel(QAbstractItemModel * /*model*/)
{
    qFatal("QTableWidget::setModel() - Changing the model of the QTableWidget is not allowed.");
}

/*! \reimp */
bool QTableWidget::event(QEvent *e)
{
    return QTableView::event(e);
}

#include "moc_qtablewidget.cpp"
#endif // QT_NO_TABLEWIDGET
