/****************************************************************************
**
** Definition of QSqlNavigator class
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

#ifndef QSQLNAVIGATOR_H
#define QSQLNAVIGATOR_H

#include "qfeatures.h"

#ifndef QT_NO_SQL

#ifndef QT_H
#include "qglobal.h"
#include "qsqlerror.h"
#include "qsqlindex.h"
#include "qsqlrecord.h"
#include "qsqlcursor.h"
#endif // QT_H

class QSqlCursor;
class QSqlForm;

class Q_EXPORT QSqlCursorNavigator
{
public:
    QSqlCursorNavigator();
    virtual ~QSqlCursorNavigator();

    virtual void setSort( const QSqlIndex& sort );
    virtual void setSort( const QStringList& sort );
    QStringList  sort() const;
    virtual void setFilter( const QString& filter );
    QString filter() const;
    virtual void setCursor( QSqlCursor* cursor, bool autoDelete = FALSE );
    QSqlCursor* cursor() const;

    virtual void refresh();
    virtual bool findBuffer( const QSqlIndex& idx, int atHint = 0 );
    virtual void currentChanged( const QSqlRecord* record );

    virtual void beforeInsert( QSqlRecord* buf );
    virtual void beforeUpdate( QSqlRecord* buf );
    virtual void beforeDelete( QSqlRecord* buf );
    virtual void cursorChanged( QSqlCursor::Mode mode );

    virtual int insert();
    virtual int update();
    virtual int del();

protected:
    virtual void handleError( const QSqlError& e );

private:
    class QSqlCursorNavigatorPrivate;
    QSqlCursorNavigatorPrivate* d;
};

class Q_EXPORT QSqlFormNavigator : public QSqlCursorNavigator
{
public:
    QSqlFormNavigator();
    ~QSqlFormNavigator();

    enum Boundry {
	Unknown,
	None,
	BeforeBeginning,
	Beginning,
	End,
	AfterEnd
    };

    virtual bool first();
    virtual bool last();
    virtual bool next();
    virtual bool prev();
    virtual void clearValues();

    virtual int insert();
    virtual int update();
    virtual int del();

    virtual void readFields();
    virtual void writeFields();

    Boundry boundry();
    virtual void setBoundryChecking( bool active );
    bool boundryChecking() const;

    virtual void setForm( QSqlForm* form );
    QSqlForm* form();

    virtual void firstRecordAvailable( bool available );
    virtual void lastRecordAvailable( bool available );
    virtual void nextRecordAvailable( bool available );
    virtual void prevRecordAvailable( bool available );

private:
    void updateBoundry();
    class QSqlFormNavigatorPrivate;
    QSqlFormNavigatorPrivate* d;
};

#endif
#endif
