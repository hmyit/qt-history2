/****************************************************************************
**
** Definition of QSqlDriver class
**
** Created : 2000-11-03
**
** Copyright (C) 2000 Trolltech AS.  All rights reserved.
**
** This file is part of the sql module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition licenses may use this
** file in accordance with the Qt Commercial License Agreement provided
** with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef QSQLDRIVER_H
#define QSQLDRIVER_H

#include "qfeatures.h"

#if !defined( QT_MODULE_SQL ) || defined( QT_LICENSE_PROFESSIONAL )
#define QM_EXPORT_SQL
#else
#define QM_EXPORT_SQL Q_EXPORT
#endif

#ifndef QT_NO_SQL

#ifndef QT_H
#include "qobject.h"
#include "qstring.h"
#include "qsqlerror.h"
#include "qsqlquery.h"
#include "qsqlfield.h"
#include "qsqlindex.h"
#include "qstringlist.h"
#endif // QT_H

class QSqlDatabase;

class QM_EXPORT_SQL QSqlDriver : public QObject
{
    friend class QSqlDatabase;
    Q_OBJECT
public:
    enum DriverFeature { Transactions, QuerySize, BLOB };

    QSqlDriver( QObject * parent=0, const char * name=0 );
    ~QSqlDriver();

    bool		  isOpen() const;
    bool	          isOpenError() const;

    virtual bool          beginTransaction();
    virtual bool          commitTransaction();
    virtual bool          rollbackTransaction();
    virtual QStringList   tables( const QString& user ) const;
    virtual QSqlIndex     primaryIndex( const QString& tableName ) const;
    virtual QSqlRecord    record( const QString& tableName ) const;
    virtual QSqlRecord    record( const QSqlQuery& query ) const;
    virtual QString       nullText() const;
    virtual QString       formatValue( const QSqlField* field, bool trimStrings = FALSE ) const;
    QSqlError	          lastError() const;

    virtual bool          feature( DriverFeature f ) const = 0;
    virtual bool          open( const QString & db,
				const QString & user = QString::null,
				const QString & password = QString::null,
				const QString & host = QString::null,
				int port = -1 ) = 0;
    virtual void          close() = 0;
    virtual QSqlQuery     createQuery() const = 0;

protected:
    virtual void          setOpen( bool o );
    virtual void          setOpenError( bool e );
    virtual void	  setLastError( const QSqlError& e );
private:
    int		          dbState;
    QSqlError	          error;
#if defined(Q_DISABLE_COPY)
    QSqlDriver( const QSqlDriver & );
    QSqlDriver &operator=( const QSqlDriver & );
#endif
};

#endif	// QT_NO_SQL
#endif
