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

#include "qsqlfield.h"
#include "qatomic.h"
#include "qdebug.h"

class QSqlFieldPrivate
{
public:
    QSqlFieldPrivate(const QString &name,
              QCoreVariant::Type type) :
        nm(name), ro(false), type(type), req(QSqlField::Unknown),
        len(-1), prec(-1), tp(-1), gen(false)
    {
        ref = 1;
    }

    QSqlFieldPrivate& operator=(const QSqlFieldPrivate& other)
    {
        nm = other.nm;
        ro = other.ro;
        type = other.type;
        req = other.req;
        len = other.len;
        prec = other.prec;
        def = other.def;
        tp = other.tp;
        gen = other.gen;
        return *this;
    }

    bool operator==(const QSqlFieldPrivate& other) const
    {
        return (nm == other.nm
                && ro == other.ro
                && type == other.type
                && req == other.req
                && len == other.len
                && prec == other.prec
                && def == other.def
                && tp == other.tp
                && gen == other.gen);
    }

    QAtomic ref;
    QString nm;
    uint ro: 1;
    QCoreVariant::Type type;
    QSqlField::RequiredStatus req;
    int len;
    int prec;
    QCoreVariant def;
    int tp;
    bool gen;
};


/*!
    \class QSqlField
    \brief The QSqlField class manipulates the fields in SQL database tables
    and views.

    \ingroup database
    \module sql

    QSqlField represents the characteristics of a single column in a
    database table or view, such as the data type and column name. A
    field also contains the value of the database column, which can be
    viewed or changed.

    Field data values are stored as QCoreVariants. Using an incompatible
    type is not permitted. For example:

    \quotefromfile snippets/sqldatabase/sqldatabase.cpp
    \skipto QSqlField_snippets
    \skipto QSqlField field
    \printuntil setValue

    However, the field will attempt to cast certain data types to the
    field data type where possible:

    \skipto QSqlField field
    \printuntil setValue

    QSqlField objects are rarely created explicitly in application
    code. They are usually accessed indirectly through \l{QSqlRecord}s
    that already contain a list of fields. For example:

    \skipto QSqlQuery query
    \printline QSqlQuery query
    \dots
    \printline QSqlRecord record
    \printline QSqlField field

    A QSqlField object can provide some meta-data about the field, for
    example, its name(), variant type(), length(), precision(),
    defaultValue(), typeID(), and its requiredStatus(),
    isAutoGenerated() and isReadOnly(). The field's data can be
    checked to see if it isNull(), and its value() retrieved. When
    editing the data can be set with setValue() or set to NULL with
    clear().

    \sa QSqlRecord
*/

/*!
    \enum QSqlField::RequiredStatus

    Specifies whether the field is required or optional.

    \value Required  The field must be specified when inserting records.
    \value Optional  The fields doesn't have to be specified when inserting records.
    \value Unknown  The database driver couldn't determine whether the field is required or
                    optional.

    \sa requiredStatus()
*/

/*!
    Constructs an empty field called \a fieldName of variant type \a
    type.

    \sa setRequiredStatus() setLength() setPrecision() setDefaultValue() setAutoGenerated() setReadOnly()
*/
QSqlField::QSqlField(const QString& fieldName, QCoreVariant::Type type)
{
    d = new QSqlFieldPrivate(fieldName, type);
}

/*!
    Constructs a copy of \a other.
*/

QSqlField::QSqlField(const QSqlField& other)
{
    d = other.d;
    ++d->ref;
    val = other.val;
}

/*!
    Sets the field equal to \a other.
*/

QSqlField& QSqlField::operator=(const QSqlField& other)
{
    qAtomicAssign(d, other.d);
    val = other.val;
    return *this;
}


/*! \fn bool QSqlField::operator!=(const QSqlField &other) const
    Returns true if the field is unequal to \a other; otherwise returns
    false.
*/

/*!
    Returns true if the field is equal to \a other; otherwise returns
    false.
*/
bool QSqlField::operator==(const QSqlField& other) const
{
    return ((d == other.d || *d == *other.d)
            && val == other.val);
}

/*!
    Destroys the object and frees any allocated resources.
*/

QSqlField::~QSqlField()
{
    if (!--d->ref)
        delete d;
}

/*!
    Sets the required status of this field to \a required.

    \sa requiredStatus() setType() setLength() setPrecision() setDefaultValue() setAutoGenerated() setReadOnly()
*/
void QSqlField::setRequiredStatus(RequiredStatus required)
{
    detach();
    d->req = required;
}

/*! \fn void QSqlField::setRequired(bool required)

    Sets the required status of this field to \l Required if \a
    required is true; otherwise sets it to \l Optional.

    \sa setRequiredStatus(), requiredStatus()
*/

/*!
    Sets the field's length to \a fieldLength. For strings this is the
    maximum number of characters the string can hold; the meaning
    varies for other types.

    \sa length() setType() setRequiredStatus() setPrecision() setDefaultValue() setAutoGenerated() setReadOnly()
*/
void QSqlField::setLength(int fieldLength)
{
    detach();
    d->len = fieldLength;
}

/*!
    Sets the field's \a precision. This only affects numeric fields.

    \sa precision() setType() setRequiredStatus() setLength() setDefaultValue() setAutoGenerated() setReadOnly()
*/
void QSqlField::setPrecision(int precision)
{
    detach();
    d->prec = precision;
}

/*!
    Sets the default value used for this field to \a value.

    \sa defaultValue() value() setType() setRequiredStatus() setLength() setPrecision() setAutoGenerated() setReadOnly()
*/
void QSqlField::setDefaultValue(const QCoreVariant &value)
{
    detach();
    d->def = value;
}

/*!
    \internal
*/
void QSqlField::setSqlType(int type)
{
    detach();
    d->tp = type;
}

/*!
    Sets the auto-generated state. If \a autoGen is true, no SQL will
    be generated for this field; otherwise, Qt classes such as
    QSqlQueryModel and QSqlTableModel will generate SQL for this
    field.

    \sa isAutoGenerated() setType() setRequiredStatus() setLength() setPrecision() setDefaultValue() setReadOnly()
*/
void QSqlField::setAutoGenerated(bool autoGen)
{
    detach();
    d->gen = autoGen;
}


/*!
    Sets the value of the field to \a value. If the field is read-only
    (isReadOnly() returns true), nothing happens.

    If the data type of \a value differs from the field's current
    data type, an attempt is made to cast it to the proper type. This
    preserves the data type of the field in the case of assignment,
    e.g. a QString to an integer data type.

    To set the value to NULL, use clear().

    \sa value() isReadOnly() defaultValue()
*/

void QSqlField::setValue(const QCoreVariant& value)
{
    if (isReadOnly())
        return;
    val = value;
}

/*!
    Clears the value of the field and sets it to NULL.
    If the field is read-only, nothing happens.

    \sa setValue() isReadOnly() requiredStatus()
*/

void QSqlField::clear()
{
    if (isReadOnly())
        return;
    val = QCoreVariant(type());
}

/*!
    Sets the name of the field to \a name.

    \sa name()
*/

void QSqlField::setName(const QString& name)
{
    detach();
    d->nm = name;
}

/*!
    Sets the read only flag of the field's value to \a readOnly. A
    read-only field cannot have its value set with setValue() and
    cannot be cleared to NULL with clear().
*/
void QSqlField::setReadOnly(bool readOnly)
{
    detach();
    d->ro = readOnly;
}

/*!
    \fn QCoreVariant QSqlField::value() const

    Returns the value of the field as a QCoreVariant.

    Use isNull() to check if the field's value is NULL.
*/

/*!
    Returns the name of the field.

    \sa setName()
*/
QString QSqlField::name() const
{
    return d->nm;
}

/*!
    Returns the field's variant type.

    \sa setType()
*/
QCoreVariant::Type QSqlField::type() const
{
    return d->type;
}

/*!
    Set's the field's variant type to \a type.

    \sa type() setRequiredStatus() setLength() setPrecision() setDefaultValue() setAutoGenerated() setReadOnly()
*/
void QSqlField::setType(QCoreVariant::Type type)
{
    detach();
    d->type = type;
}


/*!
    Returns true if the field's value is read-only; otherwise returns
    false.

    \sa setReadOnly() type() requiredStatus() length() precision() defaultValue() isAutoGenerated()
*/
bool QSqlField::isReadOnly() const
{ return d->ro; }

/*!
    Returns true if the field's value is NULL; otherwise returns
    false.

    \sa value()
*/
bool QSqlField::isNull() const
{ return val.isNull(); }

/*! \internal
*/
void QSqlField::detach()
{
    qAtomicDetach(d);
}

/*!
    Returns true if this is a required field; otherwise returns false.
    An \c INSERT will fail if a required field does not have a value.

    \sa setRequiredStatus() type() length() precision() defaultValue() isAutoGenerated()
*/
QSqlField::RequiredStatus QSqlField::requiredStatus() const
{
    return d->req;
}

/*!
    Returns the field's length.

    \sa setLength() type() requiredStatus() precision() defaultValue() isAutoGenerated()
*/
int QSqlField::length() const
{
    return d->len;
}

/*!
    Returns the field's precision; this is only meaningful for numeric
    types.

    \sa setPrecision() type() requiredStatus() length() defaultValue() isAutoGenerated()
*/
int QSqlField::precision() const
{
    return d->prec;
}

/*!
    Returns the field's default value (which may be NULL).

    \sa setDefaultValue() type() requiredStatus() length() precision() isAutoGenerated()
*/
QCoreVariant QSqlField::defaultValue() const
{
    return d->def;
}

/*!
    \internal

    Returns the type ID for the field.
*/
int QSqlField::typeID() const
{
    return d->tp;
}

/*!
    Returns true if the field is auto-generated; otherwise returns
    false.

    \sa setAutoGenerated() type() requiredStatus() length() precision() defaultValue()
*/
bool QSqlField::isAutoGenerated() const
{
    return d->gen;
}

/*!
    Returns true if the field's variant type is valid; otherwise
    returns false.
*/
bool QSqlField::isValid() const
{
    return d->type != QCoreVariant::Invalid;
}

#ifndef QT_NO_DEBUG_OUTPUT
QDebug operator<<(QDebug dbg, const QSqlField &f)
{
#ifndef Q_NO_STREAMING_DEBUG
    dbg.nospace() << "QSqlField(\"" << f.name() << "\", " << QCoreVariant::typeToName(f.type());
    if (f.length() >= 0)
        dbg.nospace() << ", length: " << f.length();
    if (f.precision() >= 0)
        dbg.nospace() << ", precision: " << f.precision();
    if (f.requiredStatus() != QSqlField::Unknown)
        dbg.nospace() << ", required: "
                      << (f.requiredStatus() == QSqlField::Required ? "yes" : "no");
    dbg.nospace() << ", auto-generated: " << (f.isAutoGenerated() ? "yes" : "no");
    if (f.typeID() >= 0)
        dbg.nospace() << ", typeID: " << f.typeID();
    if (!f.defaultValue().isNull())
        dbg.nospace() << ", auto-value: \"" << f.defaultValue() << "\"";
    dbg.nospace() << ")";
    return dbg.space();
#else
    qWarning("This compiler does not support streaming QDebug");
    return dbg;
    Q_UNUSED(v);
#endif
}
#endif

/*!
    \fn void QSqlField::setNull()

    Use clear() instead.
*/

