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

#include "qsqltablemodel.h"

#include "qhash.h"
#include "qsqldriver.h"
#include "qsqlerror.h"
#include "qsqlfield.h"
#include "qsqlindex.h"
#include "qsqlquery.h"
#include "qsqlrecord.h"

#include "qsqlquerymodel_p.h"

class QSqlTableModelPrivate: public QSqlQueryModelPrivate
{
    Q_DECLARE_PUBLIC(QSqlTableModel);
public:
    QSqlTableModelPrivate()
        : editIndex(-1), insertIndex(-1), sortColumn(-1),
          sortOrder(Qt::AscendingOrder),
          strategy(QSqlTableModel::OnFieldChange) {}
    void clear();
    QSqlRecord primaryValues(int index);
    void clearEditBuffer();
    QSqlRecord record(const QVector<QVariant> &values) const;

    QSqlDatabase db;
    int editIndex;
    int insertIndex;

    int sortColumn;
    Qt::SortOrder sortOrder;

    QSqlTableModel::EditStrategy strategy;

    QSqlQuery editQuery;
    QSqlIndex primaryIndex;
    QString tableName;
    QString filter;

    QSqlRecord editBuffer;

    typedef QHash<int, QVector<QVariant> > CacheHash;
    CacheHash cache;
};

#define d d_func()

/*! \internal
    Populates our record with values
 */
QSqlRecord QSqlTableModelPrivate::record(const QVector<QVariant> &values) const
{
    QSqlRecord r = rec;
    for (int i = 0; i < r.count() && i < values.count(); ++i)
        r.setValue(i, values.at(i));
    return r;
}

void QSqlTableModelPrivate::clear()
{
    editIndex = -1;
    sortColumn = -1;
    sortOrder = Qt::AscendingOrder;
    tableName = QString();
    editQuery.clear();
    editBuffer.clear();
    cache.clear();
    primaryIndex.clear();
    rec.clear();
    filter.clear();
}

void QSqlTableModelPrivate::clearEditBuffer()
{
    editBuffer = d->rec;
}

QSqlRecord QSqlTableModelPrivate::primaryValues(int row)
{
    QSqlRecord record;
    if (!query.seek(row)) {
        error = query.lastError();
        return record;
    }
    if (primaryIndex.isEmpty()) {
        record = rec;
        for (int i = 0; i < record.count(); ++i)
            record.setValue(i, query.value(i));
    } else {
        record = primaryIndex;
        for (int i = 0; i < record.count(); ++i)
            record.setValue(i, query.value(rec.indexOf(record.fieldName(i))));
    }
    return record;
}

/*!
  \class QSqlTableModel
  \brief The QSqlTableModel class provides an editable data model
  for a single database table.

  \ingroup database
  \module sql

  QSqlTableModel is a data model that provides data from a database
  table. By default, the model can be edited.

  \code
  QSqlTableModel model;
  model.setTable("MYTABLE");
  model.select();
  \endcode

  Before the table can be used, a table name has to be set. After
  that, it is possible to set filters with setFilter() or modify
  the sort order with setSort().

  After all the desired options have been set, select() has to be
  called to populate the model with data.
*/

/*!
  \fn QSqlTableModel::beforeDelete(int row)

  This signal is emitted before the row \a row is deleted.
*/

/*!
    \fn void QSqlTableModel::primeInsert(int row, QSqlRecord &record)

    This signal is emitted when an insertion is initiated in the given
    \a row. The \a record parameter can be written to (since it is a
    reference), for example to populate some fields with default
    values.
*/

/*!
  \fn QSqlTableModel::beforeInsert(QSqlRecord &record)

  This signal is emitted before a new row is inserted. The
  values that are about to be inserted are stored in \a record
  and can be modified before they will be inserted.
*/

/*!
  \fn QSqlTableModel::beforeUpdate(int row, QSqlRecord &record)

  This signal is emitted before the row \a row is updated with
  the values from \a record.

  Note that only values that are marked as generated will be updated.
  The generated flag can be set with \l QSqlRecord::setGenerated()
  and checked with \l QSqlRecord::isGenerated().

  \sa QSqlRecord::isGenerated()
*/


/*!
  Creates an empty QSqlTableModel and sets the parent to \a parent
  and the database connection to \a db. If \a db is not valid, the
  default database connection will be used.
 */
QSqlTableModel::QSqlTableModel(QObject *parent, QSqlDatabase db)
    : QSqlQueryModel(*new QSqlTableModelPrivate, parent)
{
    d->db = db.isValid() ? db : QSqlDatabase::database();
}

/*!
  Destroys the object and frees any allocated resources.
 */
QSqlTableModel::~QSqlTableModel()
{
}

/*!
    Sets the table to \a tableName. Does not select data from the table, but
    fetches its field information.

    To populate the model with the table's data, call select().

    \sa select(), setFilter(), sort()
 */
void QSqlTableModel::setTable(const QString &tableName)
{
    clear();
    d->tableName = tableName;
    d->rec = d->db.record(tableName);
    d->primaryIndex = d->db.primaryIndex(tableName);
}

/*!
    Returns the name of the currently selected table.
 */
QString QSqlTableModel::tableName() const
{
    return d->tableName;
}

/*!
    Populates the model with data from the table that was set via setTable(), using the
    specified filter and sort condition.

    \sa setTable(), setFilter(), sort()
 */
bool QSqlTableModel::select()
{
    QString query = selectStatement();
    if (query.isEmpty())
        return false;

    cancelChanges();
    QSqlQuery q(query, d->db);
    setQuery(q);
    return q.isActive();
}

/*!
    Returns the data for the item at position \a idx for the role \a role.
    Returns an invalid variant if \a idx is out of bounds.
 */
QVariant QSqlTableModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid())
        return QVariant();

    QModelIndex item = dataIndex(idx);
    if (idx.row() == d->insertIndex) {
        QVariant val;
        if (item.column() < 0 || item.column() >= d->rec.count())
            return val;
        val = d->editBuffer.value(idx.column());
        if (val.type() == QVariant::Invalid)
            val = QVariant(d->rec.field(item.column()).type());
        return val;
    }

    switch (d->strategy) {
    case OnFieldChange:
        break;
    case OnRowChange:
        if (d->editIndex == item.row()) {
            QVariant var = d->editBuffer.value(item.column());
            if (var.isValid())
                return var;
        }
        break;
    case OnManualSubmit: {
        QVariant var = d->cache.value(item.row()).value(item.column());
        if (var.isValid())
            return var;
        break; }
    }
    return QSqlQueryModel::data(item, role);
}

QVariant QSqlTableModel::headerData(int section, Qt::Orientation orientation, int role)
{
    if (orientation == Qt::Vertical)
        return "*";
    return QSqlQueryModel::headerData(section, orientation, role);
}

/*!
    Returns true if the value at the index \a index is dirty, otherwise false.
    Dirty values are values that were modified in the model
    but not yet written into the database.

    If \a index is invalid or points to a non-existing row, false is returned.
 */
bool QSqlTableModel::isDirty(const QModelIndex &index) const
{
    if (!index.isValid())
        return false;

    switch (d->strategy) {
        case OnFieldChange:
            return false;
        case OnRowChange:
            return index.row() == d->editIndex && d->editBuffer.value(index.column()).isValid();
        case OnManualSubmit:
            return d->cache.value(index.row()).value(index.column()).isValid();
    }
    return false;
}

/*!
    Sets the data for the item \a index for the role \a role to \a value.
    Depending on the edit strategy, the value might be applied to the database at once or
    cached in the model.

    Returns true if the value could be set or false on error, for example if \a index is
    out of bounds.

    \sa editStrategy(), data(), submitChanges(), cancelChanges()
 */
bool QSqlTableModel::setData(const QModelIndex &index, int role, const QVariant &value)
{
    QSqlRecord rec = query().record();
    if (index.column() >= rec.count())
        return false;

    if (index.row() == d->insertIndex) {
        d->editBuffer.setValue(index.column(), value);
        return true;
    }

    bool isOk = true;
    switch (d->strategy) {
    case OnFieldChange: {
        d->clearEditBuffer();
        d->editBuffer.setValue(index.column(), value);
        isOk = update(index.row(), d->editBuffer);
        if (isOk)
            select();
        break;
    }
    case OnRowChange:
        if (d->editIndex != index.row()) {
            // ### TODO: refresh/emit after row change
            if (d->editBuffer.isEmpty())
                d->clearEditBuffer();
            else if (d->editIndex != -1)
                submitChanges();
        }
        d->editBuffer.setValue(index.column(), value);
        d->editIndex = index.row();
        emit dataChanged(index, index);
        break;
    case OnManualSubmit:
        if (!d->cache.contains(index.row())) {
            QVector<QVariant> vec;
            vec.resize(query().record().count());
            vec[index.column()] = value;
            d->cache[index.row()] = vec;
        } else {
            d->cache[index.row()][index.column()] = value;
        }
        emit dataChanged(index, index);
        break;
    }
    return isOk;
}

/*! \reimp
 */
void QSqlTableModel::setQuery(const QSqlQuery &query)
{
    QSqlQueryModel::setQuery(query);
}

/*!
    Updates the row \a row in the currently active database table
    with the values from \a values.

    Note that only values that have the generated-flag set are updated.
    The generated-flag can be set with QSqlRecord::setGenerated() and
    tested with QSqlRecord::isGenerated().

    \sa QSqlRecord::isGenerated()
 */
bool QSqlTableModel::update(int row, const QSqlRecord &values)
{
    QSqlRecord rec(values);
    emit beforeUpdate(row, rec);

    const QSqlRecord whereValues(d->primaryValues(row));
    bool prepStatement = d->db.driver()->hasFeature(QSqlDriver::PreparedQueries);
    QString stmt = d->db.driver()->sqlStatement(QSqlDriver::UpdateStatement, d->tableName,
                                                rec, prepStatement);
    QString where = d->db.driver()->sqlStatement(QSqlDriver::WhereStatement, d->tableName,
                                                 whereValues, prepStatement);

    if (stmt.isEmpty() || where.isEmpty() || row < 0 || row >= rowCount()) {
        d->error = QSqlError(QLatin1String("No Fields to update"), QString(),
                                 QSqlError::StatementError);
        return false;
    }
    stmt.append(QLatin1Char(' ')).append(where);

    if (prepStatement) {
        if (d->editQuery.lastQuery() != stmt) {
            if (!d->editQuery.prepare(stmt)) {
                d->error = d->editQuery.lastError();
                return false;
            }
        }
        int i;
        for (i = 0; i < rec.count(); ++i) {
            if (rec.isGenerated(i))
                d->editQuery.addBindValue(rec.value(i));
        }
        for (i = 0; i < whereValues.count(); ++i)
            d->editQuery.addBindValue(whereValues.value(i));

        if (!d->editQuery.exec()) {
            d->error = d->editQuery.lastError();
            return false;
        }
    } else {
        if (!d->editQuery.exec(stmt)) {
            d->error = d->editQuery.lastError();
            return false;
        }
    }
    qDebug("executed: %s, %d", d->editQuery.executedQuery().ascii(), d->editQuery.numRowsAffected());
    return true;
}

/*!
   Inserts the values \a values into the database table.
   Returns true if the values could be inserted, otherwise false.
   Error information can be retrieved with \l lastError().

   \sa lastError()
 */
bool QSqlTableModel::insert(const QSqlRecord &values)
{
    QSqlRecord rec(values);
    emit beforeInsert(rec);

    bool prepStatement = d->db.driver()->hasFeature(QSqlDriver::PreparedQueries);
    QString stmt = d->db.driver()->sqlStatement(QSqlDriver::InsertStatement, d->tableName,
                                                rec, prepStatement);

    if (stmt.isEmpty())
        return false;

    if (prepStatement) {
        if (d->editQuery.lastQuery() != stmt) {
            if (!d->editQuery.prepare(stmt)) {
                d->error = d->editQuery.lastError();
                return false;
            }
        }

        for (int i = 0; i < rec.count(); ++i) {
            if (rec.isGenerated(i))
                d->editQuery.addBindValue(rec.value(i));
        }

        if (!d->editQuery.exec()) {
            d->error = d->editQuery.lastError();
            return false;
        }
    } else {
        if (!d->editQuery.exec(stmt)) {
            d->error = d->editQuery.lastError();
            return false;
        }
    }
    qDebug("executed: %s", d->editQuery.executedQuery().ascii());
    return true;
}

/*!
    Submits all pending changes and returns true on success.
    \sa lastError()
 */
bool QSqlTableModel::submitChanges()
{
    bool isOk = true;

    switch (d->strategy) {
    case OnFieldChange:
        return true;
    case OnRowChange:
        if (d->editBuffer.isEmpty())
            return true;
        if (!update(d->editIndex, d->editBuffer))
            return false;
        d->clearEditBuffer();
        d->editIndex = -1;
        select();
        break;
    case OnManualSubmit:
        QSqlTableModelPrivate::CacheHash::const_iterator i = d->cache.constBegin();
        while (i != d->cache.constEnd()) {
            if (!update(i.key(), d->record(i.value()))) {
                isOk = false;
                break;
            }
            ++i;
        }
        if (isOk) {
            d->cache.clear();
            select();
        }
        break;
    }
    return isOk;
}


/*!
    \enum QSqlTableModel::EditStrategy

    This enum type describes which strategy to choose when editing values in the database.

    \value OnFieldChange  All changes to the model will be applied immediately to the database.
    \value OnRowChange  Changes will be applied when the current row changes.
    \value OnManualSubmit  All changes will be cached in the model until either submitChanges()
                           or cancelChanges() is invoked.
*/


/*!
    Sets the strategy for editing values in the database to \a strategy.

    \sa EditStrategy, editStrategy()
 */
void QSqlTableModel::setEditStrategy(EditStrategy strategy)
{
    cancelChanges();
    d->strategy = strategy;
}

/*!
    Returns the current edit strategy.

    \sa EditStrategy, setEditStrategy()
 */
QSqlTableModel::EditStrategy QSqlTableModel::editStrategy() const
{
    return d->strategy;
}

/*!
    Cancels all pending changes.
 */
void QSqlTableModel::cancelChanges()
{
    switch (d->strategy) {
    case OnFieldChange:
        break;
    case OnRowChange:
        d->editBuffer.clear();
        if (d->editIndex >= 0)
            emit dataChanged(createIndex(d->editIndex, 0), createIndex(d->editIndex, d->rec.count()));
        break;
    case OnManualSubmit: {
        QList<int> keys = d->cache.keys();
        d->cache.clear();
        for (int i = 0; i < keys.count(); ++i)
            emit dataChanged(createIndex(keys.at(i), 0), createIndex(keys.at(i), d->rec.count()));
        break; }
    }
    d->editQuery.clear();
    d->editBuffer.clear();
    d->cache.clear();
    d->editIndex = -1;
}

/*!
    Returns the primary key for the current table, or an empty QSqlIndex
    if the table is not set or has no primary key.
 */
QSqlIndex QSqlTableModel::primaryKey() const
{
    return d->primaryIndex;
}

/*!
    Protected method to allow subclasses to set the primary key to \a key.
 */
void QSqlTableModel::setPrimaryKey(const QSqlIndex &key)
{
    d->primaryIndex = key;
}

/*!
    Returns a pointer to the used QSqlDatabase or 0 if no database was set.
 */
QSqlDatabase QSqlTableModel::database() const
{
     return d->db;
}

/*! \reimp
 */
bool QSqlTableModel::isSortable() const
{
    return true;
}

/*!
    Sorts the data by \a column with the sort order \a order.
    This will immediately select data, use setSort()
    to set a sort order without populating the model with data.

    \sa setSort(), isSortable(), select(), orderByStatement()
 */
void QSqlTableModel::sort(int column, Qt::SortOrder order)
{
    d->sortColumn = column;
    d->sortOrder = order;
    select();
}

/*!
    Sets the sort oder for \a column to \a order. This does not
    affect the current data, to refresh the data using the new
    sort order, call select().

    \sa select(), sort(), isSortable(), orderByStatement()
 */
void QSqlTableModel::setSort(int column, Qt::SortOrder order)
{
   d->sortColumn = column;
   d->sortOrder = order;
}

/*!
    Returns a SQL 'ORDER BY' statement based on the currently set
    sort order.

    \sa sort()
 */
QString QSqlTableModel::orderByStatement() const
{
    QString s;
    QSqlField f = d->rec.field(d->sortColumn);
    if (!f.isValid())
        return s;
    s.append(QLatin1String("ORDER BY ")).append(f.name());
    s += d->sortOrder == Qt::AscendingOrder ? QLatin1String(" ASC") : QLatin1String(" DESC");
    return s;
}

/*!
    Returns the index of the field \a fieldName.
 */
int QSqlTableModel::fieldIndex(const QString &fieldName) const
{
    return d->rec.indexOf(fieldName);
}

/*!
    Returns a SQL SELECT statement.
 */
QString QSqlTableModel::selectStatement() const
{
    QString query;
    if (d->tableName.isEmpty()) {
        d->error = QSqlError(QLatin1String("No tablename given"), QString(),
                             QSqlError::StatementError);
        return query;
    }
    if (d->rec.isEmpty()) {
        d->error = QSqlError(QLatin1String("Unable to find table ") + d->tableName, QString(),
                             QSqlError::StatementError);
        return query;
    }

    query = d->db.driver()->sqlStatement(QSqlDriver::SelectStatement, d->tableName,
                                          d->rec, false);
    if (query.isEmpty()) {
        d->error = QSqlError(QLatin1String("Unable to select fields from table ") + d->tableName,
                             QString(), QSqlError::StatementError);
        return query;
    }
    if (!d->filter.isEmpty())
        query.append(QLatin1String(" WHERE ")).append(d->filter);
    QString orderBy(orderByStatement());
    if (!orderBy.isEmpty())
        query.append(QLatin1Char(' ')).append(orderBy);

    return query;
}

/*!
    Removes the given \a column from the \a parent model.

    \sa removeRow()
*/
bool QSqlTableModel::removeColumn(int column, const QModelIndex &parent)
{
    if (parent.isValid() || column < 0 || column >= d->rec.count())
        return false;
    d->rec.remove(column);
    if (d->query.isActive())
        return select();
    return true;
}

/*!
    Removes the given \a row from the \a parent model.

    \sa removeColumn()
*/
bool QSqlTableModel::removeRow(int row, const QModelIndex &parent)
{
    // ### also handle manual update strategy...?
    if (row < 0 || row >= rowCount() || parent.isValid())
        return false;

    emit beforeDelete(row);

    const QSqlRecord values(d->primaryValues(row));
    bool prepStatement = d->db.driver()->hasFeature(QSqlDriver::PreparedQueries);
    QString stmt = d->db.driver()->sqlStatement(QSqlDriver::DeleteStatement, d->tableName,
                                                QSqlRecord(), prepStatement);
    QString where = d->db.driver()->sqlStatement(QSqlDriver::WhereStatement, d->tableName,
                                                values, prepStatement);

    if (stmt.isEmpty() || where.isEmpty()) {
        d->error = QSqlError(QLatin1String("Unable to delete row"), QString(),
                             QSqlError::StatementError);
        return false;
    }
    stmt.append(QLatin1Char(' ')).append(where);

    if (prepStatement) {
        if (d->editQuery.lastQuery() != stmt) {
            if (!d->editQuery.prepare(stmt)) {
                d->error = d->editQuery.lastError();
                return false;
            }
        }
        for (int i = 0; i < values.count(); ++i)
            d->editQuery.addBindValue(values.value(i));

        if (!d->editQuery.exec()) {
            d->error = d->editQuery.lastError();
            return false;
        }
    } else {
        if (!d->editQuery.exec(stmt)) {
            d->error = d->editQuery.lastError();
            return false;
        }
    }

    return true;
}

/*!
    Inserts an empty row at position \a row. Note that \a parent has to be invalid, since
    this model does not support parent-child relations.

    Note that only one row can be inserted at a time, so \a count should always be 1.

    The primeInsert() signal will be emitted, so the newly inserted values can be initialized.

    Returns false if the parameters are out of bounds, otherwise true.

    \sa primeInsert()
 */
bool QSqlTableModel::insertRow(int row, const QModelIndex &parent, int count)
{
    if (count != 1 || row < 0 || row > rowCount() || parent.isValid())
        return false;

    d->insertIndex = row;
    // ### apply dangling changes... ?
    d->clearEditBuffer();
    emit primeInsert(row, d->editBuffer);
#define UGLY_WORKAROUND
#ifdef UGLY_WORKAROUND
    emit rowsRemoved(parent, row, rowCount() - 1);
    emit rowsInserted(parent, row, rowCount() + 1);
#else
    // broken atm
    emit rowsInserted(parent, row, row);
#endif
    return true;
}

/*! \reimp
 */
int QSqlTableModel::rowCount() const
{
    int rc = QSqlQueryModel::rowCount();
    if (d->insertIndex >= 0)
        ++rc;
    return rc;
}

/*!
  Returns the index of the value in the database result set for the
  given \a item for situations where the row and column of an item
  in the model does not map to the same row and column in the
  database result set.

  Returns an invalid model index if \a item is out of bounds or if
  \a item does not point to a value in the result set.
 */
QModelIndex QSqlTableModel::dataIndex(const QModelIndex &item) const
{
    QModelIndex it = QSqlQueryModel::dataIndex(item);
    if (d->insertIndex >= 0 && it.row() >= d->insertIndex)
        return createIndex(it.row() - 1, it.column(), it.data());
    return it;
}

/*!
    Returns the currently set filter.

    \sa setFilter(), select()
 */
QString QSqlTableModel::filter() const
{
    return d->filter;
}

/*!
    Sets the current filter to \a filter. Note that no new records are selected. To select new
    records, use select(). The \a filter will apply to any subsequent select() calls.

    The filter is a SQL WHERE clause without the keyword 'WHERE', e.g. \c{name='Harry'} which will
    be processed by the DBMS.

    \sa filter(), select()
 */
void QSqlTableModel::setFilter(const QString &filter)
{
    d->filter = filter;
    if (d->query.isActive())
        select();
}

/*! \reimp
 */
void QSqlTableModel::clear()
{
    d->clear();
    QSqlQueryModel::clear();
}

/*! \reimp
 */
QSqlTableModel::ItemFlags QSqlTableModel::flags(const QModelIndex &index) const
{
    if (index.data() || index.column() < 0 || index.column() >= d->rec.count()
        || index.row() < 0)
        return 0;
    if (d->rec.field(index.column()).isReadOnly())
        return ItemIsSelectable | ItemIsEnabled;
    return ItemIsSelectable | ItemIsEnabled | ItemIsEditable;
}
