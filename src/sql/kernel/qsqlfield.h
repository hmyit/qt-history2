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

#ifndef QSQLFIELD_H
#define QSQLFIELD_H

#include "qcorevariant.h"
#include "qstring.h"

class QSqlFieldPrivate;

class Q_SQL_EXPORT QSqlField
{
public:
    enum RequiredStatus { Unknown = -1, Optional = 0, Required = 1 };

    QSqlField(const QString& fieldName = QString(),
              QCoreVariant::Type type = QCoreVariant::Invalid);

    QSqlField(const QSqlField& other);
    QSqlField& operator=(const QSqlField& other);
    bool operator==(const QSqlField& other) const;
    inline bool operator!=(const QSqlField &other) const { return !operator==(other); }
    ~QSqlField();

    void setValue(const QCoreVariant& value);
    inline QCoreVariant value() const
    { return val; }
    void setName(const QString& name);
    QString name() const;
    bool isNull() const;
    void setReadOnly(bool readOnly);
    bool isReadOnly() const;
    void clear();
    QCoreVariant::Type type() const;
    void setType(QCoreVariant::Type type);

    void setRequiredStatus(RequiredStatus status);
    inline void setRequired(bool required)
    { setRequiredStatus(required ? Required : Optional); }
    void setLength(int fieldLength);
    void setPrecision(int precision);
    void setDefaultValue(const QCoreVariant &value);
    void setSqlType(int type);
    void setAutoGenerated(bool autogen);

    RequiredStatus requiredStatus() const;
    int length() const;
    int precision() const;
    QCoreVariant defaultValue() const;
    int typeID() const;
    bool isAutoGenerated() const;
    bool isValid() const;

#ifdef QT_COMPAT
    inline QT_COMPAT void setNull() { clear(); }
#endif

private:
    void detach();
    QCoreVariant val;
    QSqlFieldPrivate* d;
};

#ifndef QT_NO_DEBUG_OUTPUT
Q_SQL_EXPORT QDebug operator<<(QDebug, const QSqlField &);
#endif

#endif
