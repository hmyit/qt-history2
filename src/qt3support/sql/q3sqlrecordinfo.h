/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef Q3SQLRECORDINFO_H
#define Q3SQLRECORDINFO_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_SQL
#   include <Qt3Support/q3valuelist.h>
#   include <QtSql/qsqlrecord.h>
#   include <Qt3Support/q3sqlfieldinfo.h>
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Qt3Support)

#ifndef QT_NO_SQL

/* Q3SqlRecordInfo Class
   This class is obsolete, use QSqlRecord instead.
*/

typedef Q3ValueList<Q3SqlFieldInfo> Q3SqlFieldInfoList;

class Q_COMPAT_EXPORT Q3SqlRecordInfo: public Q3SqlFieldInfoList
{
public:
    Q3SqlRecordInfo(): Q3SqlFieldInfoList() {}
    Q3SqlRecordInfo(const Q3SqlFieldInfoList& other): Q3SqlFieldInfoList(other) {}
    Q3SqlRecordInfo(const QSqlRecord& other)
    {
        for (int i = 0; i < other.count(); ++i)
            push_back(Q3SqlFieldInfo(other.field(i)));
    }

    size_type contains(const QString& fieldName) const;
    Q3SqlFieldInfo find(const QString& fieldName) const;
    QSqlRecord toRecord() const;
};

inline Q3SqlRecordInfo::size_type Q3SqlRecordInfo::contains(const QString& fieldName) const
{
    size_type i = 0;
    QString fName = fieldName.toUpper();

    for(const_iterator it = begin(); it != end(); ++it) {
        if ((*it).name().toUpper() == fName) {
            ++i;
        }
    }
    return i;
}

inline Q3SqlFieldInfo Q3SqlRecordInfo::find(const QString& fieldName) const
{
    QString fName = fieldName.toUpper();
    for(const_iterator it = begin(); it != end(); ++it) {
        if ((*it).name().toUpper() == fName) {
            return *it;
        }
    }
    return Q3SqlFieldInfo();
}

inline QSqlRecord Q3SqlRecordInfo::toRecord() const
{
    QSqlRecord buf;
    for(const_iterator it = begin(); it != end(); ++it) {
        buf.append((*it).toField());
    }
    return buf;
}

#endif // QT_NO_SQL

QT_END_NAMESPACE

QT_END_HEADER

#endif // Q3SQLRECORDINFO_H
