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

#ifndef QSQL_SQLITE_H
#define QSQL_SQLITE_H

#include <qsqldriver.h>
#include <qsqlresult.h>
#include <qsqlrecord.h>
#include <qsqlindex.h>
#include <qsqlcachedresult_p.h>

#if defined (Q_OS_WIN32)
# include <qt_windows.h>
#endif

class QSQLite2DriverPrivate;
class QSQLite2ResultPrivate;
class QSQLite2Driver;
struct sqlite;

class QSQLite2Result : public QSqlCachedResult
{
    friend class QSQLite2Driver;
    friend class QSQLite2ResultPrivate;
public:
    QSQLite2Result(const QSQLite2Driver* db);
    ~QSQLite2Result();

protected:
    bool gotoNext(QSqlCachedResult::ValueCache& row, int idx);
    bool reset (const QString& query);
    int size();
    int numRowsAffected();
    QSqlRecord record() const;

private:
    QSQLite2ResultPrivate* d;
};

class QSQLite2Driver : public QSqlDriver
{
    friend class QSQLite2Result;
public:
    QSQLite2Driver(QObject *parent = 0);
    QSQLite2Driver(sqlite *connection, QObject *parent = 0);
    ~QSQLite2Driver();
    bool hasFeature(DriverFeature f) const;
    bool open(const QString & db,
                   const QString & user,
                   const QString & password,
                   const QString & host,
                   int port,
                   const QString & connOpts);
    bool open(const QString & db,
            const QString & user,
            const QString & password,
            const QString & host,
            int port) { return open (db, user, password, host, port, QString()); }
    void close();
    QSqlResult *createResult() const;
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    QStringList tables(QSql::TableType) const;

    QSqlRecord record(const QString& tablename) const;
    QSqlIndex primaryIndex(const QString &table) const;

private:
    QSQLite2DriverPrivate* d;
};
#endif
