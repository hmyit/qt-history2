/****************************************************************************
**
** Definition of QSqlManager class
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

#ifndef QSQLMANAGER_P_H
#define QSQLMANAGER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//
//

#include "qfeatures.h"

#ifndef QT_NO_SQL

#ifndef QT_H
#include "qglobal.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qsqlerror.h"
#include "qsqlindex.h"
#include "qsqlcursor.h"
#endif // QT_H

class QSqlCursor;
class QSqlForm;

class Q_EXPORT QSqlCursorManager
{
public:
    QSqlCursorManager();
    virtual ~QSqlCursorManager();

    virtual void setSort( const QSqlIndex& sort );
    virtual void setSort( const QStringList& sort );
    QStringList  sort() const;
    virtual void setFilter( const QString& filter );
    QString filter() const;
    virtual void setCursor( QSqlCursor* cursor, bool autoDelete = FALSE );
    QSqlCursor* cursor() const;

    virtual void refresh();
    virtual bool findBuffer( const QSqlIndex& idx, int atHint = 0 );

private:
    class QSqlCursorManagerPrivate;
    QSqlCursorManagerPrivate* d;
};

class Q_EXPORT QSqlFormManager
{
public:
    QSqlFormManager();
    virtual ~QSqlFormManager();

    virtual void setForm( QSqlForm* form );
    QSqlForm* form();
    virtual void setRecord( QSqlRecord* record );
    QSqlRecord* record();

    virtual void clearValues();
    virtual void readFields();
    virtual void writeFields();

private:
    class QSqlFormManagerPrivate;
    QSqlFormManagerPrivate* d;
};


#endif
#endif
