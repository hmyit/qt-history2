/****************************************************************************
**
** Implementation of SQLite driver classes.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the sql module of the Qt GUI Toolkit.
** EDITIONS: FREE, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qsql_sqlite.h"

#include <qdatetime.h>
#include <qmap.h>
#include <qregexp.h>
#include <qfile.h>

#if (QT_VERSION-0 < 0x030000)
#  include <qvector.h>
#  if !defined Q_WS_WIN32
#    include <unistd.h>
#  endif
#  include "../../../3rdparty/libraries/sqlite/sqlite.h"
#else
#  include <qptrvector.h>
#  if !defined Q_WS_WIN32
#    include <unistd.h>
#  endif
#  include <sqlite.h>
#endif

typedef struct sqlite_vm sqlite_vm;

static QSqlVariant::Type nameToType(const QString& typeName)
{
    QString tName = typeName.toUpper();
    if (tName.startsWith("INT"))
        return QSqlVariant::Int;
    if (tName.startsWith("FLOAT") || tName.startsWith("NUMERIC"))
        return QSqlVariant::Double;
    if (tName.startsWith("BOOL"))
        return QSqlVariant::Bool;
    // SQLite is typeless - consider everything else as string
    return QSqlVariant::String;
}

class QSQLiteDriverPrivate
{
public:
    QSQLiteDriverPrivate();
    sqlite *access;
    bool utf8;
};

QSQLiteDriverPrivate::QSQLiteDriverPrivate() : access(0)
{
    utf8 = (qstrcmp(sqlite_encoding, "UTF-8") == 0);
}

class QSQLiteResultPrivate
{
public:
    QSQLiteResultPrivate(QSQLiteResult *res);
    void cleanup();
    bool fetchNext(QtSqlCachedResult::ValueCache &values, int idx, bool initialFetch);
    bool isSelect();
    // initializes the recordInfo and the cache
    void init(const char **cnames, int numCols);
    void finalize();

    QSQLiteResult* q;
    sqlite *access;

    // and we have too keep our own struct for the data (sqlite works via
    // callback.
    const char *currentTail;
    sqlite_vm *currentMachine;

    uint skippedStatus: 1; // the status of the fetchNext() that's skipped    
    uint skipRow: 1; // skip the next fetchNext()?
    uint utf8: 1;
    QSqlRecord rInf;
};

static const uint initial_cache_size = 128;

QSQLiteResultPrivate::QSQLiteResultPrivate(QSQLiteResult* res) : q(res), access(0), currentTail(0),
    currentMachine(0), skippedStatus(FALSE), skipRow(false), utf8(FALSE)
{
}

void QSQLiteResultPrivate::cleanup()
{
    finalize();
    rInf.clear();
    currentTail = 0;
    currentMachine = 0;
    skippedStatus = FALSE;
    skipRow = false;
    q->setAt(QSql::BeforeFirst);
    q->setActive(FALSE);
    q->cleanup();
}

void QSQLiteResultPrivate::finalize()
{
    if (!currentMachine)
        return;

    char* err = 0;
    int res = sqlite_finalize(currentMachine, &err);
    if (err) {
        q->setLastError(QSqlError("Unable to fetch results", err, QSqlError::Statement, res));
        sqlite_freemem(err);
    }
    currentMachine = 0;
}

// called on first fetch
void QSQLiteResultPrivate::init(const char **cnames, int numCols)
{
    if (!cnames)
        return;

    rInf.clear();
    if (numCols <= 0)
        return;
    q->init(numCols);

    for (int i = 0; i < numCols; ++i) {
        const char* lastDot = strrchr(cnames[i], '.');
        const char* fieldName = lastDot ? lastDot + 1 : cnames[i];
        rInf.append(QSqlField(fieldName, nameToType(cnames[i+numCols])));
    }
}

bool QSQLiteResultPrivate::fetchNext(QtSqlCachedResult::ValueCache &values, int idx, bool initialFetch)
{
    // may be caching.
    const char **fvals;
    const char **cnames;
    int colNum;
    int res;
    int i;

    if (skipRow) {
	// already fetched
	Q_ASSERT(!initialFetch);
	skipRow = false;
	return skippedStatus;
    }
    skipRow = initialFetch;

    if (!currentMachine)
	return FALSE;

    // keep trying while busy, wish I could implement this better.
    while ((res = sqlite_step(currentMachine, &colNum, &fvals, &cnames)) == SQLITE_BUSY) {
	// sleep instead requesting result again immidiately.
#if defined Q_WS_WIN32
	Sleep(1000);
#else
	sleep(1);
#endif
    }

    switch(res) {
    case SQLITE_ROW:
	// check to see if should fill out columns
	if (rInf.isEmpty())
	    // must be first call.
	    init(cnames, colNum);
	if (!fvals)
	    return FALSE;
	if (idx < 0 && !initialFetch)
	    return TRUE;
	for (i = 0; i < colNum; ++i)
	    values[i + idx] = utf8 ? QString::fromUtf8(fvals[i]) : QString(fvals[i]);
	return TRUE;
    case SQLITE_DONE:
	if (rInf.isEmpty())
	    // must be first call.
	    init(cnames, colNum);
	q->setAt(QSql::AfterLast);
	return FALSE;
    case SQLITE_ERROR:
    case SQLITE_MISUSE:
    default:
	// something wrong, don't get col info, but still return false
	finalize(); // finalize to get the error message.
	q->setAt(QSql::AfterLast);
	return FALSE;
    }
    return FALSE;
}

QSQLiteResult::QSQLiteResult(const QSQLiteDriver* db)
: QtSqlCachedResult(db)
{
    d = new QSQLiteResultPrivate(this);
    d->access = db->d->access;
    d->utf8 = db->d->utf8;
}

QSQLiteResult::~QSQLiteResult()
{
    d->cleanup();
    delete d;
}

/*
   Execute \a query.
*/
bool QSQLiteResult::reset (const QString& query)
{
    // this is where we build a query.
    if (!driver())
        return FALSE;
    if (!driver()-> isOpen() || driver()->isOpenError())
        return FALSE;

    d->cleanup();

    // Um, ok.  callback based so.... pass private static function for this.
    setSelect(FALSE);
    char *err = 0;
    int res = sqlite_compile(d->access,
                                d->utf8 ? query.utf8() : query.local8Bit(),
                                &(d->currentTail),
                                &(d->currentMachine),
                                &err);
    if (res != SQLITE_OK || err) {
        setLastError(QSqlError("Unable to execute statement", err, QSqlError::Statement, res));
        sqlite_freemem(err);
    }
    //if (*d->currentTail != '\000' then there is more sql to eval
    if (!d->currentMachine) {
	setActive(FALSE);
	return FALSE;
    }
    // we have to fetch one row to find out about
    // the structure of the result set
    d->skippedStatus = d->fetchNext(cache(), 0, true);
    setSelect(!d->rInf.isEmpty());
    setActive(TRUE);
    return TRUE;
}

bool QSQLiteResult::gotoNext(QtSqlCachedResult::ValueCache& row, int idx)
{
    return d->fetchNext(row, idx, false);
}

int QSQLiteResult::size()
{
    return -1;
}

int QSQLiteResult::numRowsAffected()
{
    return sqlite_changes(d->access);
}

QSqlRecord QSQLiteResult::record() const
{
    if (!isActive() || !isSelect())
	return QSqlRecord();
    return d->rInf;
}

/////////////////////////////////////////////////////////

QSQLiteDriver::QSQLiteDriver(QObject * parent)
    : QSqlDriver(parent)
{
    d = new QSQLiteDriverPrivate();
}

QSQLiteDriver::QSQLiteDriver(sqlite *connection, QObject *parent)
    : QSqlDriver(parent)
{
    d = new QSQLiteDriverPrivate();
    d->access = connection;
    setOpen(TRUE);
    setOpenError(FALSE);
}


QSQLiteDriver::~QSQLiteDriver()
{
    delete d;
}

bool QSQLiteDriver::hasFeature(DriverFeature f) const
{
    switch (f) {
    case Transactions:
        return TRUE;
#if (QT_VERSION-0 >= 0x030000)
    case Unicode:
        return d->utf8;
#endif
//   case BLOB:
    default:
        return FALSE;
    }
}

/*
   SQLite dbs have no user name, passwords, hosts or ports.
   just file names.
*/
bool QSQLiteDriver::open(const QString & db, const QString &, const QString &, const QString &, int, const QString &)
{
    if (isOpen())
        close();

    if (db.isEmpty())
	return FALSE;

    char* err = 0;
    d->access = sqlite_open(QFile::encodeName(db), 0, &err);
    if (err) {
        setLastError(QSqlError("Error to open database", err, QSqlError::Connection));
        sqlite_freemem(err);
        err = 0;
    }

    if (d->access) {
        setOpen(TRUE);
        return TRUE;
    }
    setOpenError(TRUE);
    return FALSE;
}

void QSQLiteDriver::close()
{
    if (isOpen()) {
        sqlite_close(d->access);
        d->access = 0;
        setOpen(FALSE);
        setOpenError(FALSE);
    }
}

QSqlQuery QSQLiteDriver::createQuery() const
{
    return QSqlQuery(new QSQLiteResult(this));
}

bool QSQLiteDriver::beginTransaction()
{
    if (!isOpen() || isOpenError())
        return FALSE;

    char* err;
    int res = sqlite_exec(d->access, "BEGIN", 0, this, &err);

    if (res == SQLITE_OK)
        return TRUE;

    setLastError(QSqlError("Unable to begin transaction", err, QSqlError::Transaction, res));
    sqlite_freemem(err);
    return FALSE;
}

bool QSQLiteDriver::commitTransaction()
{
    if (!isOpen() || isOpenError())
        return FALSE;

    char* err;
    int res = sqlite_exec(d->access, "COMMIT", 0, this, &err);

    if (res == SQLITE_OK)
        return TRUE;

    setLastError(QSqlError("Unable to commit transaction", err, QSqlError::Transaction, res));
    sqlite_freemem(err);
    return FALSE;
}

bool QSQLiteDriver::rollbackTransaction()
{
    if (!isOpen() || isOpenError())
        return FALSE;

    char* err;
    int res = sqlite_exec(d->access, "ROLLBACK", 0, this, &err);

    if (res == SQLITE_OK)
        return TRUE;

    setLastError(QSqlError("Unable to rollback Transaction", err, QSqlError::Transaction, res));
    sqlite_freemem(err);
    return FALSE;
}

QStringList QSQLiteDriver::tables(const QString &typeName) const
{
    QStringList res;
    if (!isOpen())
        return res;
    int type = typeName.toInt();

    QSqlQuery q = createQuery();
    q.setForwardOnly(TRUE);
#if (QT_VERSION-0 >= 0x030000)
    if ((type & (int)QSql::Tables) && (type & (int)QSql::Views))
        q.exec("SELECT name FROM sqlite_master WHERE type='table' OR type='view'");
    else if (typeName.isEmpty() || (type & (int)QSql::Tables))
        q.exec("SELECT name FROM sqlite_master WHERE type='table'");
    else if (type & (int)QSql::Views)
        q.exec("SELECT name FROM sqlite_master WHERE type='view'");
#else
        q.exec("SELECT name FROM sqlite_master WHERE type='table' OR type='view'");
#endif


    if (q.isActive()) {
        while(q.next())
            res.append(q.value(0).toString());
    }

#if (QT_VERSION-0 >= 0x030000)
    if (type & (int)QSql::SystemTables) {
        // there are no internal tables beside this one:
        res.append("sqlite_master");
    }
#endif

    return res;
}

QSqlIndex QSQLiteDriver::primaryIndex(const QString &tblname) const
{
    QSqlRecord rec(record(tblname)); // expensive :(

    if (!isOpen())
        return QSqlIndex();

    QSqlQuery q = createQuery();
    q.setForwardOnly(TRUE);
    // finrst find a UNIQUE INDEX
    q.exec("PRAGMA index_list('" + tblname + "');");
    QString indexname;
    while(q.next()) {
	if (q.value(2).toInt()==1) {
	    indexname = q.value(1).toString();
	    break;
	}
    }
    if (indexname.isEmpty())
	return QSqlIndex();

    q.exec("PRAGMA index_info('" + indexname + "');");

    QSqlIndex index(indexname);
    while(q.next()) {
	QString name = q.value(2).toString();
	QSqlVariant::Type type = QSqlVariant::Invalid;
	if (rec.contains(name))
	    type = rec.field(name)->type();
	index.append(QSqlField(name, type));
    }
    return index;
}

QSqlRecord QSQLiteDriver::record(const QString &tbl) const
{
    if (!isOpen())
        return QSqlRecord();

    QSqlQuery q = createQuery();
    q.setForwardOnly(TRUE);
    q.exec("SELECT * FROM " + tbl + " LIMIT 1");
    return q.record();
}
