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

#include "qsqldriver.h"
#include "qsqlerror.h"
#include "qsqlfield.h"
#include "qsqlindex.h"
#include "qsqlquery.h"
#include "qsqlrecord.h"

#include "qsqltablemodel_p.h"

#include <qdebug.h>

#define q q_func()

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

void QSqlTableModelPrivate::revertInsertedRow()
{
    if (insertIndex == -1)
        return;

    int oldIndex = insertIndex;
    insertIndex = -1;
    emit q->rowsAboutToBeRemoved(QModelIndex::Null, oldIndex, oldIndex);
}

void QSqlTableModelPrivate::clearEditBuffer()
{
    editBuffer = rec;
}

void QSqlTableModelPrivate::revertCachedRow(int row)
{
    ModifiedRow r = cache.value(row);
    switch (r.op) {
    case QSql::None:
        Q_ASSERT_X(false, "QSqlTableModelPrivate::revertCachedRow()", "Invalid entry in cache map");
        return;
    case QSql::Update:
    case QSql::Delete:
        cache.remove(row);
        emit q->dataChanged(q->createIndex(row, 0),
                            q->createIndex(row, q->columns() - 1));
        break;
    case QSql::Insert: {
            QMap<int, QSqlTableModelPrivate::ModifiedRow>::Iterator it = cache.find(row);
            if (it == cache.end())
                return;
            it = cache.erase(it);
            while (it != cache.end()) {
                int oldKey = it.key();
                const QSqlTableModelPrivate::ModifiedRow oldValue = it.value();
                cache.erase(it);
                it = cache.insert(oldKey - 1, oldValue);
                ++it;
            }
            emit q->rowsAboutToBeRemoved(QModelIndex::Null, row, row);
        break; }
    }
}

bool QSqlTableModelPrivate::exec(const QString &stmt, bool prepStatement,
                                 const QSqlRecord &rec, const QSqlRecord &whereValues)
{
    if (stmt.isEmpty())
        return false;

    // lazy initialization of editQuery
    if (editQuery.driver() != db.driver())
        editQuery = QSqlQuery(db);

    if (prepStatement) {
        if (editQuery.lastQuery() != stmt) {
            if (!editQuery.prepare(stmt)) {
                error = editQuery.lastError();
                return false;
            }
        }
        int i;
        for (i = 0; i < rec.count(); ++i) {
            if (rec.isGenerated(i))
                editQuery.addBindValue(rec.value(i));
        }
        for (i = 0; i < whereValues.count(); ++i)
            editQuery.addBindValue(whereValues.value(i));

        if (!editQuery.exec()) {
            error = editQuery.lastError();
            return false;
        }
    } else {
        if (!editQuery.exec(stmt)) {
            error = editQuery.lastError();
            return false;
        }
    }
    qDebug("executed: %s (%d)", stmt.ascii(), editQuery.numRowsAffected());
    return true;
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

#define d d_func()

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

  The default edit strategy is OnRowChange.
 */
QSqlTableModel::QSqlTableModel(QObject *parent, QSqlDatabase db)
    : QSqlQueryModel(*new QSqlTableModelPrivate(this), parent)
{
    d->db = db.isValid() ? db : QSqlDatabase::database();
}

/*!  \interal
 */
QSqlTableModel::QSqlTableModel(QSqlTableModelPrivate &dd, QObject *parent, QSqlDatabase db)
    : QSqlQueryModel(dd, parent)
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

    revertAll();
    QSqlQuery qu(query, d->db);
    setQuery(qu);
    return qu.isActive();
}

/*!
    Returns the data for the item at position \a idx for the role \a role.
    Returns an invalid variant if \a idx is out of bounds.
 */
QVariant QSqlTableModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || role & ~(DisplayRole | EditRole))
        return QVariant();

    QModelIndex item = dataIndex(idx);

    switch (d->strategy) {
    case OnFieldChange:
    case OnRowChange:
        if (idx.row() == d->insertIndex) {
            QVariant val;
            if (item.column() < 0 || item.column() >= d->rec.count())
                return val;
            val = d->editBuffer.value(idx.column());
            if (val.type() == QVariant::Invalid)
                val = QVariant(d->rec.field(item.column()).type());
            return val;
        }
        if (d->editIndex == item.row()) {
            QVariant var = d->editBuffer.value(item.column());
            if (var.isValid())
                return var;
        }
        break;
    case OnManualSubmit: {
        const QSqlTableModelPrivate::ModifiedRow row = d->cache.value(idx.row());
        const QVariant var = row.rec.value(item.column());
        if (var.isValid() || row.op == QSql::Insert)
            return var;
        break; }
    }
    return QSqlQueryModel::data(item, role);
}

/*!
    Returns the header data for the given \a role in the \a section of the
    header with the specified \a orientation.
*/
QVariant QSqlTableModel::headerData(int section, Qt::Orientation orientation, int role)
{
    if (orientation == Qt::Vertical) {
        switch (d->strategy) {
        case OnFieldChange:
        case OnRowChange:
            if (d->insertIndex == section)
                return QLatin1String("*");
            break;
        case OnManualSubmit:
            QSql::Op op = d->cache.value(section).op;
            if (op == QSql::Insert)
                return QLatin1String("*");
            else if (op == QSql::Delete)
                return QLatin1String("!");
            break;
        }
    }
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
        case OnManualSubmit: {
            const QSqlTableModelPrivate::ModifiedRow row = d->cache.value(index.row());
            return row.op == QSql::Insert || row.op == QSql::Delete
                   || (row.op == QSql::Update && row.rec.value(index.column()).isValid());
        }
    }
    return false;
}

/*!
    Sets the data for the item \a index for the role \a role to \a value.
    Depending on the edit strategy, the value might be applied to the database at once or
    cached in the model.

    Returns true if the value could be set or false on error, for example if \a index is
    out of bounds.

    \sa editStrategy(), data(), submit(), submitAll(), revertRow()
 */
bool QSqlTableModel::setData(const QModelIndex &index, int role, const QVariant &value)
{
    if (role & ~EditRole)
        return QSqlQueryModel::setData(index, role, value);

    QSqlRecord rec = query().record();
    if (index.column() >= rec.count() || index.row() >= rows())
        return false;

    bool isOk = true;
    switch (d->strategy) {
    case OnFieldChange: {
        if (index.row() == d->insertIndex) {
            d->editBuffer.setValue(index.column(), value);
            return true;
        }
        d->clearEditBuffer();
        d->editBuffer.setValue(index.column(), value);
        isOk = updateRowInTable(index.row(), d->editBuffer);
        if (isOk)
            select();
        break; }
    case OnRowChange:
        if (index.row() == d->insertIndex) {
            d->editBuffer.setValue(index.column(), value);
            return true;
        }
        if (d->editIndex != index.row()) {
            if (d->editBuffer.isEmpty())
                d->clearEditBuffer();
            else if (d->editIndex != -1)
                submit();
        }
        d->editBuffer.setValue(index.column(), value);
        d->editIndex = index.row();
        emit dataChanged(index, index);
        break;
    case OnManualSubmit: {
        QSqlTableModelPrivate::ModifiedRow &row = d->cache[index.row()];
        if (row.op == QSql::None) {
            row.op = QSql::Update;
            row.rec = d->rec;
        }
        row.rec.setValue(index.column(), value);
        emit dataChanged(index, index);
        break; }
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

    This is a low-level method that operates directly on the database
    and should not be called directly. Use setData() to update values.
    The model will decide depending on its edit strategy when to modify
    the database.

    Note that only values that have the generated-flag set are updated.
    The generated-flag can be set with QSqlRecord::setGenerated() and
    tested with QSqlRecord::isGenerated().

    \sa QSqlRecord::isGenerated(), setData()
 */
bool QSqlTableModel::updateRowInTable(int row, const QSqlRecord &values)
{
    QSqlRecord rec(values);
    emit beforeUpdate(row, rec);

    const QSqlRecord whereValues = d->primaryValues(row);
    bool prepStatement = d->db.driver()->hasFeature(QSqlDriver::PreparedQueries);
    QString stmt = d->db.driver()->sqlStatement(QSqlDriver::UpdateStatement, d->tableName,
                                                rec, prepStatement);
    QString where = d->db.driver()->sqlStatement(QSqlDriver::WhereStatement, d->tableName,
                                                 whereValues, prepStatement);

    if (stmt.isEmpty() || where.isEmpty() || row < 0 || row >= rows()) {
        d->error = QSqlError(QLatin1String("No Fields to update"), QString(),
                                 QSqlError::StatementError);
        return false;
    }
    stmt.append(QLatin1Char(' ')).append(where);

    return d->exec(stmt, prepStatement, rec, whereValues);
}


/*!
    Inserts the values \a values into the currently active database table.

    This is a low-level method that operates directly on the database
    and should not be called directly. Use insertRow() and setData()
    to insert values. The model will decide depending on its edit strategy
    when to modify the database.

    Returns true if the values could be inserted, otherwise false.
    Error information can be retrieved with \l lastError().

    \sa lastError(), insertRow(), insertRows()
 */
bool QSqlTableModel::insertRowIntoTable(const QSqlRecord &values)
{
    QSqlRecord rec(values);
    emit beforeInsert(rec);

    bool prepStatement = d->db.driver()->hasFeature(QSqlDriver::PreparedQueries);
    QString stmt = d->db.driver()->sqlStatement(QSqlDriver::InsertStatement, d->tableName,
                                                rec, prepStatement);

    return d->exec(stmt, prepStatement, rec);
}

/*!
    Deletes the given \a row from the currently active database table.

    This is a low-level method that operates directly on the database
    and should not be called directly. Use removeRow() or removeRows()
    to delete values. The model will decide depending on its edit strategy
    when to modify the database.

    Returns true if the row was deleted; otherwise returns false.

    \sa removeRow(), removeRows()
*/
bool QSqlTableModel::deleteRowFromTable(int row)
{
    emit beforeDelete(row);

    QSqlRecord rec = d->primaryValues(row);
    bool prepStatement = d->db.driver()->hasFeature(QSqlDriver::PreparedQueries);
    QString stmt = d->db.driver()->sqlStatement(QSqlDriver::DeleteStatement, d->tableName,
                                                QSqlRecord(), prepStatement);
    QString where = d->db.driver()->sqlStatement(QSqlDriver::WhereStatement, d->tableName,
                                                rec, prepStatement);

    if (stmt.isEmpty() || where.isEmpty()) {
        d->error = QSqlError(QLatin1String("Unable to delete row"), QString(),
                             QSqlError::StatementError);
        return false;
    }
    stmt.append(QLatin1Char(' ')).append(where);

    return d->exec(stmt, prepStatement, rec);
}

/*!
    Submits all pending changes and returns true on success.
    Returns false on error, detailed error information can be
    obtained with lastError().

    \sa revertAll(), lastError()
 */
bool QSqlTableModel::submitAll()
{
    bool isOk = true;

    switch (d->strategy) {
    case OnFieldChange:
        return true;
    case OnRowChange:
        if (d->editBuffer.isEmpty())
            return true;
        if (d->insertIndex != -1) {
            if (!insertRowIntoTable(d->editBuffer))
                return false;
        } else {
            if (!updateRowInTable(d->editIndex, d->editBuffer))
                return false;
        }
        d->clearEditBuffer();
        d->editIndex = -1;
        d->insertIndex = -1;
        select();
        break;
    case OnManualSubmit: {
        QSqlTableModelPrivate::CacheMap::ConstIterator it = d->cache.constBegin();
        while (it != d->cache.constEnd()) {
            switch (it.value().op) {
            case QSql::Insert:
                isOk |= insertRowIntoTable(it.value().rec);
                break;
            case QSql::Update:
                isOk |= updateRowInTable(it.key(), it.value().rec);
                break;
            case QSql::Delete:
                isOk |= deleteRowFromTable(it.key());
                break;
            case QSql::None:
                qWarning("QSqlTableModel::submitAll: Invalid operation");
                break;
            }
            ++it;
        }
        if (isOk) {
            d->cache.clear();
            select();
        }
        break; }
    }
    return isOk;
}

/*!
    This reimplemented slot is called by the item delegates when the user stopped
    editing the current row.

    Submits the currently edited row if the model's strategy is set to OnRowChange.
    Does nothing for the other edit strategies. Use submitAll() to submit all pending
    changes for the OnManualSubmit strategy.

    Returns false on error, otherwise true. Use lastError() to query detailed error
    information.

    \sa revert(), revertRow(), submitAll(), revertAll(), lastError()
*/
bool QSqlTableModel::submit()
{
    if (d->strategy == OnRowChange)
        return submitAll();
    return true;
}

/*!
    This reimplemented slot is called by the item delegates when the user cancelled
    editing the current row.

    Reverts the changes if the model's strategy is set to OnRowChange. Does nothing
    for the other edit strategies. Use revertAll() to revert all pending changes
    for the OnManualSubmit strategy or revertRow() to revert a specific row.

    \sa submit(), submitAll(), revertRow(), revertAll()
*/
void QSqlTableModel::revert()
{
    if (d->strategy == OnRowChange)
        revertAll();
}

/*!
    \enum QSqlTableModel::EditStrategy

    This enum type describes which strategy to choose when editing values in the database.

    \value OnFieldChange  All changes to the model will be applied immediately to the database.
    \value OnRowChange  Changes will be applied when the current row changes.
    \value OnManualSubmit  All changes will be cached in the model until either submitAll()
                           or revertAll() is invoked.
*/


/*!
    Sets the strategy for editing values in the database to \a strategy.
    This will revert all pending changes.

    \sa EditStrategy, editStrategy()
 */
void QSqlTableModel::setEditStrategy(EditStrategy strategy)
{
    revertAll();
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
    Reverts all pending changes.
 */
void QSqlTableModel::revertAll()
{
    switch (d->strategy) {
    case OnFieldChange:
        break;
    case OnRowChange:
        d->editBuffer.clear();
        if (d->editIndex != -1) {
            int oldIndex = d->editIndex;
            d->editIndex = -1;
            emit dataChanged(createIndex(oldIndex, 0),
                             createIndex(oldIndex, columns()));
        }
        d->revertInsertedRow();
        break;
    case OnManualSubmit: {
        QSqlTableModelPrivate::CacheMap::ConstIterator it = d->cache.constBegin();
        while (it != d->cache.constEnd()) {
            d->revertCachedRow(it.key());
            it = d->cache.constBegin();
        }
        break; }
    }
}

/*!
  Reverts all changes for the specified \a row.
 */
void QSqlTableModel::revertRow(int row)
{
    switch (d->strategy) {
    case OnFieldChange:
    case OnRowChange:
        revertAll();
        break;
    case OnManualSubmit:
        d->revertCachedRow(row);
        break;
    }
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

    \sa removeRows()
*/
bool QSqlTableModel::removeColumns(int column, const QModelIndex &parent, int count)
{
    if (parent.isValid() || column < 0 || column + count > d->rec.count())
        return false;
    for (int i = 0; i < count; ++i)
        d->rec.remove(column + i);
    if (d->query.isActive())
        return select();
    return true;
}

/*!
    Removes \a count rows starting at \a row. Since this model
    does not support hierarchical structures, \a parent must be
    an invalid model index.

    Emits the beforeDelete() signal before a row is deleted.

    Returns true if all rows could be removed, otherwise false.
    Detailed error information can be retrieved with lastError().

    \sa removeColumns()
*/
bool QSqlTableModel::removeRows(int row, const QModelIndex &parent, int count)
{
    if (parent.isValid() || row < 0 || count <= 0)
        return false;

    int i;
    switch (d->strategy) {
    case OnFieldChange:
    case OnRowChange:
        for (i = 0; i < count; ++i) {
            if (row + i == d->insertIndex)
                d->revertInsertedRow();
            else if (!deleteRowFromTable(row + i))
                return false;
        }
        select();
        break;
    case OnManualSubmit:
        for (i = 0; i < count; ++i) {
            int idx = row + i;
            if (idx >= rows())
                return false;
            if (d->cache.value(idx).op == QSql::Insert)
                revertRow(idx);
            else
                d->cache[idx].op = QSql::Delete;
        }
        break;
    }
    return true;
}

/*!
    Inserts an empty row at position \a row. Note that \a parent has to be invalid, since
    this model does not support parent-child relations.

    Only one row at a time can be inserted when using the OnFieldChange or OnRowChange
    update strategies.

    The primeInsert() signal will be emitted for each new row. Connect to it if you
    want to initialize the new row with default values.

    Returns false if the parameters are out of bounds, otherwise true.

    \sa primeInsert()
 */
bool QSqlTableModel::insertRows(int row, const QModelIndex &parent, int count)
{
    if (row < 0 || count <= 0 || row > rows() || parent.isValid())
        return false;

    switch (d->strategy) {
    case OnFieldChange:
    case OnRowChange:
        if (count != 1)
            return false;
        d->insertIndex = row;
        // ### apply dangling changes...
        d->clearEditBuffer();
        emit primeInsert(row, d->editBuffer);
        break;
    case OnManualSubmit:
        if (!d->cache.isEmpty()) {
            QMap<int, QSqlTableModelPrivate::ModifiedRow>::Iterator it = d->cache.end();
            while (it != d->cache.begin() && (--it).key() >= row) {
                int oldKey = it.key();
                const QSqlTableModelPrivate::ModifiedRow oldValue = it.value();
                d->cache.erase(it);
                it = d->cache.insert(oldKey + count, oldValue);
            }
        }

        for (int i = 0; i < count; ++i) {
            d->cache[row + i] = QSqlTableModelPrivate::ModifiedRow(QSql::Insert, d->rec);
            emit primeInsert(row + i, d->cache[row + i].rec);
        }
        break;
    }
    emit rowsInserted(parent, row, row + count);
    return true;
}

/*!
    Convenience method that inserts the \a record after \a row.
    If \a row is negative, the record will be appended to the end.
    Calls insertRows() and setRecord() internally.

    Returns true if the row could be inserted, otherwise false.
 */
bool QSqlTableModel::insertRecord(int row, const QSqlRecord &record)
{
    if (row < 0)
        row = rows();
    if (!insertRow(row, QModelIndex::Null))
        return false;
    if (!setRecord(row, record))
        return false;
    if (d->strategy == OnFieldChange || d->strategy == OnRowChange)
        return submit();
    return true;
}

/*! \reimp
 */
int QSqlTableModel::rows() const
{
    int rc = QSqlQueryModel::rows();
    if (d->strategy == OnManualSubmit) {
        for (QSqlTableModelPrivate::CacheMap::ConstIterator it = d->cache.constBegin();
             it != d->cache.constEnd(); ++it) {
             if (it.value().op == QSql::Insert)
                 ++rc;
        }
    } else if (d->insertIndex >= 0) {
        ++rc;
    }
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
    const QModelIndex it = QSqlQueryModel::dataIndex(item);
    if (d->strategy == OnManualSubmit) {
        int rowOffset = 0;
        QSqlTableModelPrivate::CacheMap::ConstIterator i = d->cache.constBegin();
        while (i != d->cache.constEnd() && i.key() <= it.row()) {
            if (i.value().op == QSql::Insert)
                ++rowOffset;
            ++i;
        }
        return createIndex(it.row() - rowOffset, it.column(), it.data());
    } else {
        if (d->insertIndex >= 0 && it.row() >= d->insertIndex)
            return createIndex(it.row() - 1, it.column(), it.data());
    }
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

/*!
    Sets the values at the specified \a row to the values of \a record.
    Returns false if not all the values could be set, otherwise true.

    \sa record()
 */
bool QSqlTableModel::setRecord(int row, const QSqlRecord &record)
{
    if (row >= rows())
        return false;

    bool isOk = true;
    switch (d->strategy) {
    case OnFieldChange:
    case OnRowChange:
        for (int i = 0; i < record.count(); ++i) {
            int idx = d->rec.indexOf(record.fieldName(i));
            if (idx == -1)
                continue;
            isOk |= setData(createIndex(row, idx), QAbstractItemModel::EditRole, record.value(i));
        }
        return isOk;
    case OnManualSubmit: {
        QSqlTableModelPrivate::ModifiedRow &mrow = d->cache[row];
        if (mrow.op == QSql::None) {
            mrow.op = QSql::Update;
            mrow.rec = d->rec;
        }
        for (int i = 0; i < record.count(); ++i) {
            int idx = mrow.rec.indexOf(record.fieldName(i));
            if (idx == -1)
                isOk = false;
            else
                mrow.rec.setValue(idx, record.value(i));
        }
        return isOk; }
    }
    return false;
}
