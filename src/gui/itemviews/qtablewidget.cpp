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

#include "qtablewidget.h"
#include <qheaderview.h>
#include <qabstractitemmodel.h>
#include <private/qtableview_p.h>

class QTableModel : public QAbstractTableModel
{
    friend class QTableWidget;
public:
    QTableModel(int rows = 0, int columns = 0, QObject *parent = 0);
    ~QTableModel();

    virtual void setRowCount(int rows);
    virtual void setColumnCount(int columns);

    virtual bool insertRows(int row, const QModelIndex &parent = QModelIndex::Null, int count = 1);
    virtual bool insertColumns(int column, const QModelIndex &parent = QModelIndex::Null, int count = 1);

    virtual bool removeRows(int row, const QModelIndex &parent = QModelIndex::Null, int count = 1);
    virtual bool removeColumns(int column, const QModelIndex &parent = QModelIndex::Null, int count = 1);

    void setItem(int row, int column, QTableWidgetItem *item);
    QTableWidgetItem *item(int row, int column) const;
    QTableWidgetItem *item(const QModelIndex &index) const;
    
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex::Null,
                      QModelIndex::Type type = QModelIndex::View) const;

    int rowCount() const;
    int columnCount() const;

    QVariant data(const QModelIndex &index, int role = QAbstractItemModel::DisplayRole) const;
    bool setData(const QModelIndex &index, int role, const QVariant &value);

    bool isSelectable(const QModelIndex &index) const;
    bool isEditable(const QModelIndex &index) const;

    bool isValid(const QModelIndex &index) const;
    int tableIndex(int row, int column) const;

private:
    int r, c;
    QVector<QTableWidgetItem*> table;
    QVector<QTableWidgetItem*> leftHeader;
    QVector<QTableWidgetItem*> topHeader;
};

QTableModel::QTableModel(int rows, int columns, QObject *parent)
    : QAbstractTableModel(parent), r(rows), c(columns),
      table(rows * columns), leftHeader(rows), topHeader(columns) {}

QTableModel::~QTableModel()
{
}

void QTableModel::setRowCount(int rows)
{
    if (r == rows)
        return;
    int _r = qMin(r, rows);
    int s = rows * c;
    r = rows;

    int top = qMax(_r - 1, 0);
    int bottom = qMax(r - 1, 0);

    if (r < _r)
        emit rowsRemoved(QModelIndex::Null, top, bottom);

    table.resize(s); // FIXME: this will destroy the layout
    leftHeader.resize(r);
    for (int j = _r; j < r; ++j)
        leftHeader[j] = 0;

    if (r >= _r)
        emit rowsInserted(QModelIndex::Null, top, bottom);
}

void QTableModel::setColumnCount(int columns)
{
    if (c == columns)
        return;
    int _c = qMin(c, columns);
    int s = r * columns;
    c = columns;

    int left = qMax(_c - 1, 0);
    int right = qMax(c - 1, 0);

    if (c < _c)
        emit columnsRemoved(QModelIndex::Null, left, right);

    table.resize(s); // FIXME: this will destroy the layout
    topHeader.resize(c);
    for (int j = _c; j < c; ++j)
        topHeader[j] = 0;

    if (c >= _c)
        emit columnsInserted(QModelIndex::Null, left, right);
}

bool QTableModel::insertRows(int, const QModelIndex &, int)
{
// FIXME: not implemented
    qDebug("insertRows: not implemented");
    return false;
}

bool QTableModel::insertColumns(int, const QModelIndex &, int)
{
// FIXME: not implemented
    qDebug("insertColumns: not implemented");
    return false;
}

bool QTableModel::removeRows(int, const QModelIndex &, int)
{
// FIXME: not implemented
    qDebug("removeRows: not implemented");
    return false;
}

bool QTableModel::removeColumns(int, const QModelIndex &, int)
{
// FIXME: not implemented
    qDebug("removeColumns: not implemented");
    return false;
}

void QTableModel::setItem(int row, int column, QTableWidgetItem *item)
{
    table[tableIndex(row, column)] = item;
}

QTableWidgetItem *QTableModel::item(int row, int column) const
{
    return table.at(tableIndex(row, column));
}

QTableWidgetItem *QTableModel::item(const QModelIndex &index) const
{
    if (!isValid(index))
        return 0;
    if (index.type() == QModelIndex::VerticalHeader)
        return leftHeader.at(index.row());
    else if (index.type() == QModelIndex::HorizontalHeader)
        return topHeader.at(index.column());
    else
        return table.at(tableIndex(index.row(), index.column()));
    return 0;
}

QModelIndex QTableModel::index(int row, int column, const QModelIndex &, QModelIndex::Type type) const
{
    if (row >= 0 && row < r && column >= 0 && column < c) {
        QTableWidgetItem *item = table.at(tableIndex(row, column)); // FIXME: headers ?
        return createIndex(row, column, item, type);
    }
    return QModelIndex::Null;
}

int QTableModel::rowCount() const
{
    return r;
}

int QTableModel::columnCount() const
{
    return c;
}

QVariant QTableModel::data(const QModelIndex &index, int role) const
{
    QTableWidgetItem *itm = item(index);
    if (itm)
        return itm->data(role);
    if (index.type() == QModelIndex::VerticalHeader && role == QAbstractItemModel::DisplayRole)
        return QString::number(index.row());
    if (index.type() == QModelIndex::HorizontalHeader && role == QAbstractItemModel::DisplayRole)
        return QString::number(index.column());
    return QVariant();
}

bool QTableModel::setData(const QModelIndex &index, int role, const QVariant &value)
{
    QTableWidgetItem *itm = item(index);
    if (itm) {
        itm->setData(role, value);
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

bool QTableModel::isSelectable(const QModelIndex &index) const
{
    QTableWidgetItem *itm = item(index);
    return itm ? itm->isSelectable() : false;
}

bool QTableModel::isEditable(const QModelIndex &index) const
{
    QTableWidgetItem *itm = item(index);
    return itm ? itm->isEditable() : false;
}

bool QTableModel::isValid(const QModelIndex &index) const
{
    return index.isValid() && index.row() < r && index.column() < c;
}

int QTableModel::tableIndex(int row, int column) const
{
    return (row * c) + column;
}

/*!
    \class QTableWidget qtablewidget.h
    \brief The QTableWidget class provides a table view that uses the
    predefined QTableModel model.

    \ingroup model-view
    \mainclass

    If you want a table that uses your own data model you should
    subclass QTableView rather than this class.

    Items are set with setItem(), or with setText() or setIcon();
    these last two are convenience functions that create a QTableItem
    for you. The label and icon for a given row is set with
    setRowText() and setRowIconSet(), and for a given column with
    setColumnText() and setColumnIconSet(). The number of rows is set
    with setRowCount(), and the number of columns with
    setColumnCount().

    \sa \link model-view-programming.html Model/View Programming\endlink
*/

class QTableWidgetPrivate : public QTableViewPrivate
{
    Q_DECLARE_PUBLIC(QTableWidget)
public:
    QTableWidgetPrivate() : QTableViewPrivate() {}
    inline QTableModel *model() const { return ::qt_cast<QTableModel*>(q_func()->model()); }
};

#define d d_func()
#define q q_func()

/*!
    Creates a new table view with the given \a parent. The table view
    uses a QTableModel to hold its data.
*/
QTableWidget::QTableWidget(QWidget *parent)
    : QTableView(*new QTableWidgetPrivate, parent)
{
    setModel(new QTableModel(0, 0, this));
    setItemDelegate(new QWidgetBaseItemDelegate(this));
    verticalHeader()->setItemDelegate(new QWidgetBaseItemDelegate(verticalHeader()));
    horizontalHeader()->setItemDelegate(new QWidgetBaseItemDelegate(horizontalHeader()));
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
    d->model()->setRowCount(rows);
}

/*!
    Sets the number of columns in this table's model to \a columns. If
    this is less than columnCount(), the data in the unwanted columns
    is discarded.

    \sa setRowCount()
*/
void QTableWidget::setColumnCount(int columns)
{
    d->model()->setColumnCount(columns);
}

/*!
    Returns the item for the given \a row and \a column.

    \sa setItem() text() icon()
*/
QTableWidgetItem *QTableWidget::item(int row, int column) const
{
    return d->model()->item(row, column);
}

/*!
    Sets the item for the given \a row and \a column to \a item.

    \sa item() setText() setIcon()
*/
void QTableWidget::setItem(int row, int column, QTableWidgetItem *item)
{
    d->model()->setItem(row, column, item);
}


QTableWidgetItem *QTableWidget::verticalHeaderItem(int row) const
{
    return d->model()->leftHeader.at(row);
}

void QTableWidget::setVerticalHeaderItem(int row, QTableWidgetItem *item)
{
    d->model()->leftHeader[row] = item;
}

QTableWidgetItem *QTableWidget::horizontalHeaderItem(int column) const
{
    return d->model()->topHeader.at(column);
}

void QTableWidget::setHorizontalHeaderItem(int column, QTableWidgetItem *item)
{
    d->model()->topHeader[column] = item;
}
