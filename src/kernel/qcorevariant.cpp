/****************************************************************************
**
** Implementation of QCoreVariant class.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qcorevariant.h"
#ifndef QT_NO_VARIANT
#include "qbitarray.h"
#include "qbytearray.h"
#include "qdatastream.h"
#include "qdatetime.h"
#include "qlist.h"
#include "qmap.h"
#include "qstring.h"
#include "qstringlist.h"

#include <float.h>

#ifndef DBL_DIG
#define DBL_DIG 10
#endif //DBL_DIG

typedef QMap<QString,QCoreVariant> QVariantMap;

template<> QBitArray qt_cast<QBitArray>(const QCoreVariant &v) { return v.toBitArray(); }
template<> QString qt_cast<QString>(const QCoreVariant &v) { return v.toString(); }
#ifndef QT_NO_STRINGLIST
template<> QStringList qt_cast<QStringList>(const QCoreVariant &v)
{ return v.toStringList(); }
#endif
template<> QDate qt_cast<QDate>(const QCoreVariant &v) { return v.toDate(); }
template<> QTime qt_cast<QTime>(const QCoreVariant &v) { return v.toTime(); }
template<> QDateTime qt_cast<QDateTime>(const QCoreVariant &v) { return v.toDateTime(); }
#ifndef QT_NO_TEMPLATE_VARIANT
template<> QList<QCoreVariant> qt_cast<QList<QCoreVariant> >(const QCoreVariant &v) 
{ return v.toList(); }
template<> QMap<QString,QCoreVariant> qt_cast<QMap<QString,QCoreVariant> >(const QCoreVariant &v)
{ return v.toMap(); }
#endif

QCoreVariant::Private QCoreVariant::shared_invalid = { Q_ATOMIC_INIT(1), Invalid, true, {0}, 0 };

// takes a type, returns the internal void* pointer castet
// to a pointer of the input type
template<typename T> inline T *v_cast(void *&p)
{
    if (QTypeInfo<T>::isLarge)
        return static_cast<T*>(p);
    else
        return reinterpret_cast<T*>(&p);
}

#define QCONSTRUCT(vType) \
    if (QTypeInfo<vType >::isLarge) \
	x->value.ptr = new vType(*static_cast<const vType *>(v)); \
    else \
	new (&x->value.ptr) vType(*static_cast<const vType *>(v))

#define QCONSTRUCT_EMPTY(vType) \
    if (QTypeInfo<vType >::isLarge) \
	x->value.ptr = new vType; \
    else \
	new (&x->value.ptr) vType
  
static void construct(QCoreVariant::Private *x, const void *v)
{
    if (v) {
	switch( x->type ) {
	case QCoreVariant::String:
	    QCONSTRUCT(QString);
	    break;
#ifndef QT_NO_STRINGLIST
	case QCoreVariant::StringList:
	    QCONSTRUCT(QStringList);
	    break;
#endif //QT_NO_STRINGLIST
#ifndef QT_NO_TEMPLATE_VARIANT
	case QCoreVariant::Map:
	    QCONSTRUCT(QVariantMap);
	    break;
	case QCoreVariant::List:
	    QCONSTRUCT(QList<QCoreVariant>);
	    break;
#endif
	case QCoreVariant::Date:
	    QCONSTRUCT(QDate);
	    break;
	case QCoreVariant::Time:
	    QCONSTRUCT(QTime);
	    break;
	case QCoreVariant::DateTime:
	    QCONSTRUCT(QDateTime);
	    break;
	case QCoreVariant::ByteArray:
	    QCONSTRUCT(QByteArray);
	    break;
	case QCoreVariant::BitArray:
	    QCONSTRUCT(QBitArray);
	    break;
	case QCoreVariant::Int:
	    x->value.i = *static_cast<const int *>(v);
	    break;
	case QCoreVariant::UInt:
	    x->value.u = *static_cast<const uint *>(v);
	    break;
	case QCoreVariant::Bool:
	    x->value.b = *static_cast<const bool *>(v);
	    break;
	case QCoreVariant::Double:
	    x->value.d = *static_cast<const double *>(v);
	    break;
	case QCoreVariant::LongLong:
	    x->value.ll = *static_cast<const Q_LLONG *>(v);
	    break;
	case QCoreVariant::ULongLong:
	    x->value.ull = *static_cast<const Q_ULLONG *>(v);
	    break;
	case QCoreVariant::Invalid:
	    break;
	default:
	    Q_ASSERT( 0 );
	}
	x->is_null = false;
    } else {
	switch (x->type) {
	case QCoreVariant::Invalid:
	    break;
	case QCoreVariant::String:
	    QCONSTRUCT_EMPTY(QString);
	    break;
#ifndef QT_NO_STRINGLIST
	case QCoreVariant::StringList:
	    QCONSTRUCT_EMPTY(QStringList);
	    break;
#endif //QT_NO_STRINGLIST
#ifndef QT_NO_TEMPLATE_VARIANT
	case QCoreVariant::Map:
	    QCONSTRUCT_EMPTY(QVariantMap);
	    break;
	case QCoreVariant::List:
	    QCONSTRUCT_EMPTY(QList<QCoreVariant>);
	    break;
#endif
	case QCoreVariant::Date:
	    QCONSTRUCT_EMPTY(QDate);
	    break;
	case QCoreVariant::Time:
	    QCONSTRUCT_EMPTY(QTime);
	    break;
	case QCoreVariant::DateTime:
	    QCONSTRUCT_EMPTY(QDateTime);
	    break;
	case QCoreVariant::ByteArray:
	    QCONSTRUCT_EMPTY(QByteArray);
	    break;
	case QCoreVariant::BitArray:
	    QCONSTRUCT_EMPTY(QBitArray);
	    break;
	case QCoreVariant::Int:
	    x->value.i = 0;
	    break;
	case QCoreVariant::UInt:
	    x->value.u = 0;
	    break;
	case QCoreVariant::Bool:
	    x->value.b = 0;
	    break;
	case QCoreVariant::Double:
	    x->value.d = 0;
	    break;
	case QCoreVariant::LongLong:
	    x->value.ll = Q_LLONG(0);
	    break;
	case QCoreVariant::ULongLong:
	    x->value.ull = Q_ULLONG(0);
	    break;
	default:
	    Q_ASSERT( 0 );
	}

    }
}

#define QCLEAR(vType) \
    if (QTypeInfo<vType >::isLarge) \
	delete static_cast<vType *>(p->value.ptr); \
    else \
	reinterpret_cast<vType *>(&p->value.ptr)->~vType()

static void clear(QCoreVariant::Private *p)
{
    switch (p->type) {
    case QCoreVariant::String:
	QCLEAR(QString);
	break;
#ifndef QT_NO_STRINGLIST
    case QCoreVariant::StringList:
	QCLEAR(QStringList);
	break;
#endif //QT_NO_STRINGLIST
#ifndef QT_NO_TEMPLATE_VARIANT
    case QCoreVariant::Map:
	QCLEAR(QVariantMap);
	break;
    case QCoreVariant::List:
	QCLEAR(QList<QCoreVariant>);
	break;
#endif
    case QCoreVariant::Date:
	QCLEAR(QDate);
	break;
    case QCoreVariant::Time:
	QCLEAR(QTime);
	break;
    case QCoreVariant::DateTime:
	QCLEAR(QDateTime);
	break;
    case QCoreVariant::ByteArray:
	QCLEAR(QByteArray);
	break;
    case QCoreVariant::BitArray:
	QCLEAR(QBitArray);
	break;
    case QCoreVariant::Invalid:
    case QCoreVariant::Int:
    case QCoreVariant::UInt:
    case QCoreVariant::LongLong:
    case QCoreVariant::ULongLong:
    case QCoreVariant::Bool:
    case QCoreVariant::Double:
	break;
    default:
	qFatal("cannot handle GUI types of QCoreVariant without a Gui application");
    }

    p->type = QCoreVariant::Invalid;
    p->is_null = true;
    if (p->str_cache) {
	reinterpret_cast<QString *>(&p->str_cache)->~QString();
	p->str_cache = 0;
    }
}

// used internally by construct() only
#define QISNULL(vType) \
    if (QTypeInfo<vType >::isLarge) \
	return static_cast<const vType *>(d->value.ptr)->isNull(); \
    else \
	return reinterpret_cast<const vType *>(&d->value.ptr)->isNull()

static bool isNull(const QCoreVariant::Private *d)
{
    switch( d->type ) {
    case QCoreVariant::String:
	QISNULL(QString);
    case QCoreVariant::Date:
	QISNULL(QDate);
    case QCoreVariant::Time:
	QISNULL(QTime);
    case QCoreVariant::DateTime:
	QISNULL(QDateTime);
    case QCoreVariant::ByteArray:
	QISNULL(QByteArray);
    case QCoreVariant::BitArray:
	QISNULL(QBitArray);
#ifndef QT_NO_STRINGLIST
    case QCoreVariant::StringList:
#endif //QT_NO_STRINGLIST
#ifndef QT_NO_TEMPLATE_VARIANT
    case QCoreVariant::Map:
    case QCoreVariant::List:
#endif
    case QCoreVariant::Invalid:
    case QCoreVariant::Int:
    case QCoreVariant::UInt:
    case QCoreVariant::LongLong:
    case QCoreVariant::ULongLong:
    case QCoreVariant::Bool:
    case QCoreVariant::Double:
	break;
    default:
	qFatal("cannot handle GUI types of QCoreVariant without a Gui application");
    }
    return d->is_null;
}

#ifndef QT_NO_DATASTREAM

#define QLOAD(vType) \
    if (QTypeInfo<vType >::isLarge) \
        s >> *static_cast<vType *>(d->value.ptr); \
    else \
        s >> *reinterpret_cast<vType *>(&d->value.ptr)

static void load(QCoreVariant::Private *d, QDataStream &s)
{
    switch (d->type) {
    case QCoreVariant::Invalid: {
	// Since we wrote something, we should read something
	QString x;
	s >> x;
	d->is_null = true;
	break;
    }
#ifndef QT_NO_TEMPLATE_VARIANT
    case QCoreVariant::Map:
	QLOAD(QVariantMap);
	break;
    case QCoreVariant::List:
	QLOAD(QList<QCoreVariant>);
	break;
#endif
    case QCoreVariant::String:
	QLOAD(QString);
	break;
#ifndef QT_NO_STRINGLIST
    case QCoreVariant::StringList:
	QLOAD(QStringList);
	break;
#endif // QT_NO_STRINGLIST
    case QCoreVariant::Int:
	s >> d->value.i;
	break;
    case QCoreVariant::UInt:
	s >> d->value.u;
	break;
    case QCoreVariant::LongLong:
	s >> d->value.ll;
	break;
    case QCoreVariant::ULongLong:
	s >> d->value.ull;
	break;
    case QCoreVariant::Bool: {
	Q_INT8 x;
	s >> x;
	d->value.b = x;
    }
	break;
    case QCoreVariant::Double:
	s >> d->value.d;
	break;
    case QCoreVariant::Date:
	QLOAD(QDate);
	break;
    case QCoreVariant::Time:
	QLOAD(QTime);
	break;
    case QCoreVariant::DateTime:
	QLOAD(QDateTime);
	break;
    case QCoreVariant::ByteArray:
	QLOAD(QByteArray);
	break;
    case QCoreVariant::BitArray:
	QLOAD(QBitArray);
	break;
    default:
	qFatal("cannot handle GUI types of QCoreVariant without a Gui application");
    }
}

#define QSAVE(vType) \
    if (QTypeInfo<vType >::isLarge) \
        s << *static_cast<const vType *>(d->value.ptr); \
    else \
        s << *reinterpret_cast<const vType *>(&d->value.ptr)

static void save(const QCoreVariant::Private *d, QDataStream &s)
{
    switch (d->type) {
#ifndef QT_NO_TEMPLATE_VARIANT
    case QCoreVariant::List:
	QSAVE(QList<QCoreVariant>);
	break;
    case QCoreVariant::Map:
	QSAVE(QVariantMap);
	break;
#endif
    case QCoreVariant::String:
	QSAVE(QString);
	break;
#ifndef QT_NO_STRINGLIST
    case QCoreVariant::StringList:
	QSAVE(QStringList);
	break;
#endif
    case QCoreVariant::Int:
	s << d->value.i;
	break;
    case QCoreVariant::UInt:
	s << d->value.u;
	break;
    case QCoreVariant::LongLong:
	s << d->value.ll;
	break;
    case QCoreVariant::ULongLong:
	s << d->value.ull;
	break;
    case QCoreVariant::Bool:
	s << (Q_INT8)d->value.b;
	break;
    case QCoreVariant::Double:
	s << d->value.d;
	break;
    case QCoreVariant::Date:
	QSAVE(QDate);
	break;
    case QCoreVariant::Time:
	QSAVE(QTime);
	break;
    case QCoreVariant::DateTime:
	QSAVE(QDateTime);
	break;
    case QCoreVariant::ByteArray:
	QSAVE(QByteArray);
	break;
    case QCoreVariant::BitArray:
	QSAVE(QBitArray);
	break;
    case QCoreVariant::Invalid:
	s << QString();
	break;
    default:
	qFatal("cannot handle GUI types of QCoreVariant without a Gui application");
    }
}
#endif // QT_NO_DATASTREAM

#define QCOMPARE(vType) \
    if (QTypeInfo<vType >::isLarge) \
        return *static_cast<const vType *>(a->value.ptr) == \
	    *static_cast<const vType *>(b->value.ptr); \
    else \
	return *reinterpret_cast<const vType *>(&a->value.ptr) \
	    == *reinterpret_cast<const vType *>(&b->value.ptr);

static bool compare(const QCoreVariant::Private *a, const QCoreVariant::Private *b)
{
    switch(a->type) {
#ifndef QT_NO_TEMPLATE_VARIANT
    case QCoreVariant::List:
	QCOMPARE(QList<QCoreVariant>);
    case QCoreVariant::Map: {
	QVariantMap *m1 = v_cast<QVariantMap>((void*)a->value.ptr);
	QVariantMap *m2 = v_cast<QVariantMap>((void*)b->value.ptr);
	if (m1->count() != m2->count())
	    return false;
	QVariantMap::ConstIterator it = m1->constBegin();
	QVariantMap::ConstIterator it2 = m2->constBegin();
	while (it != m1->constEnd()) {
	    if (*it != *it2)
		return false;
	    ++it;
	    ++it2;
	}
	return true;
    }
#endif
    case QCoreVariant::String:
	QCOMPARE(QString);
#ifndef QT_NO_STRINGLIST
    case QCoreVariant::StringList:
	QCOMPARE(QStringList);
#endif
    case QCoreVariant::Int:
	return a->value.i == b->value.i;
    case QCoreVariant::UInt:
	return a->value.u == b->value.u;
    case QCoreVariant::LongLong:
	return a->value.ll == b->value.ll;
    case QCoreVariant::ULongLong:
	return a->value.ull == b->value.ull;
    case QCoreVariant::Bool:
	return a->value.b == b->value.b;
    case QCoreVariant::Double:
	return a->value.d == b->value.d;
    case QCoreVariant::Date:
	QCOMPARE(QDate);
    case QCoreVariant::Time:
	QCOMPARE(QTime);
    case QCoreVariant::DateTime:
	QCOMPARE(QDateTime);
    case QCoreVariant::ByteArray:
	QCOMPARE(QByteArray);
    case QCoreVariant::BitArray:
	QCOMPARE(QBitArray);
    case QCoreVariant::Invalid:
	break;
    default:
	qFatal("cannot handle GUI types of QCoreVariant without a Gui application");
    }
    return false;
}

static void cast(QCoreVariant::Private *d, QCoreVariant::Type t, void *result, bool *ok)
{
    Q_ASSERT(d->type != (uint)t);
    switch (t) {
    case QCoreVariant::String: {
	QString *str = static_cast<QString *>(result);
	switch (d->type) {
	case QCoreVariant::Int:
	    *str = QString::number(d->value.i);
	    break;
	case QCoreVariant::UInt:
	    *str = QString::number(d->value.u);
	    break;
	case QCoreVariant::LongLong:
	    *str = QString::number(d->value.ll);
	    break;
	case QCoreVariant::ULongLong:
	    *str = QString::number(d->value.ull);
	    break;
	case QCoreVariant::Double:
	    *str = QString::number(d->value.d, 'g', DBL_DIG);
	    break;
#if !defined(QT_NO_SPRINTF) && !defined(QT_NO_DATESTRING)
	case QCoreVariant::Date:
	    *str = v_cast<QDate>(d->value.ptr)->toString(Qt::ISODate);
	    break;
	case QCoreVariant::Time:
	    *str = v_cast<QTime>(d->value.ptr)->toString(Qt::ISODate);
	    break;
	case QCoreVariant::DateTime:
	    *str = v_cast<QDateTime>(d->value.ptr)->toString(Qt::ISODate);
	    break;
#endif
	case QCoreVariant::Bool:
	    *str = d->value.b ? "true" : "false";
	    break;
	case QCoreVariant::ByteArray:
	    *str = QString(v_cast<QByteArray>(d->value.ptr)->constData());
	    break;
	default:
	    break;
	}
	break;
    }
#ifndef QT_NO_TEMPLATE_VARIANT
    case QCoreVariant::StringList:
	if (d->type == QCoreVariant::List) {
	    QStringList *slst = static_cast<QStringList *>(result);
	    QList<QCoreVariant> *list = v_cast<QList<QCoreVariant> >(d->value.ptr);
	    for (int i = 0; i < list->size(); ++i)
		slst->append(list->at(i).toString());
	}
#endif
	break;
    case QCoreVariant::Date: {
	QDate *dt = static_cast<QDate *>(result);
	if (d->type == QCoreVariant::DateTime)
	    *dt = v_cast<QDateTime>(d->value.ptr)->date();
#ifndef QT_NO_DATESTRING
	else if (d->type == QCoreVariant::String)
	    *dt = QDate::fromString(*v_cast<QString>(d->value.ptr), Qt::ISODate);
#endif
	break;
    }
    case QCoreVariant::Time: {
	QTime *t = static_cast<QTime *>(result);
	switch ( d->type ) {
	case QCoreVariant::DateTime:
	    *t = v_cast<QDateTime>(d->value.ptr)->time();
	    break;
#ifndef QT_NO_DATESTRING
	case QCoreVariant::String:
	    *t = QTime::fromString( *v_cast<QString>(d->value.ptr), Qt::ISODate );
	    break;
#endif
	default:
	    break;
	}
	break;
    }
    case QCoreVariant::DateTime: {
	QDateTime *dt = static_cast<QDateTime *>(result);
	switch ( d->type ) {
#ifndef QT_NO_DATESTRING
	case QCoreVariant::String:
	    *dt = QDateTime::fromString(*v_cast<QString>(d->value.ptr), Qt::ISODate);
	    break;
#endif
	case QCoreVariant::Date:
	    *dt = QDateTime(*v_cast<QDate>(d->value.ptr));
	    break;
	default:
	    break;
	}
	break;
    }
    case QCoreVariant::ByteArray: {
	QByteArray *ba = static_cast<QByteArray *>(result);
	if (d->type == QCoreVariant::String)
	    *ba = v_cast<QString>(d->value.ptr)->toAscii();
    }
    break;
    case QCoreVariant::Int: {
	int *i = static_cast<int *>(result);
	switch (d->type) {
	case QCoreVariant::String:
	    *i = v_cast<QString>(d->value.ptr)->toInt(ok);
	    break;
	case QCoreVariant::ByteArray:
	    *i = QString(*v_cast<QByteArray>(d->value.ptr)).toInt(ok);
	    break;
	case QCoreVariant::Int:
	    *i = d->value.i;
	    break;
	case QCoreVariant::UInt:
	    *i = (int)d->value.u;
	    break;
	case QCoreVariant::LongLong:
	    *i = (int)d->value.ll;
	    break;
	case QCoreVariant::ULongLong:
	    *i = (int)d->value.ull;
	    break;
	case QCoreVariant::Double:
	    *i = (int)d->value.d;
	    break;
	case QCoreVariant::Bool:
	    *i = (int)d->value.b;
	    break;
	default:
	    *i = 0;
	    break;
	}
	break;
    }
    case QCoreVariant::UInt: {
	uint *u = static_cast<uint *>(result);
	switch (d->type) {
	case QCoreVariant::String:
	    *u = v_cast<QString>(d->value.ptr)->toUInt(ok);
	    break;
	case QCoreVariant::ByteArray:
	    *u = QString(*v_cast<QByteArray>(d->value.ptr)).toUInt(ok);
	    break;
	case QCoreVariant::Int:
	    *u = (uint)d->value.i;
	    break;
	case QCoreVariant::UInt:
	    *u = d->value.u;
	    break;
	case QCoreVariant::LongLong:
	    *u = (uint)d->value.ll;
	    break;
	case QCoreVariant::ULongLong:
	    *u = (uint)d->value.ull;
	    break;
	case QCoreVariant::Double:
	    *u = (uint)d->value.d;
	    break;
	case QCoreVariant::Bool:
	    *u = (uint)d->value.b;
	    break;
	default:
	    *u = 0;
	    break;
	}
	break;
    }
    case QCoreVariant::LongLong: {
	Q_LLONG *l = static_cast<Q_LLONG *>(result);
	switch (d->type) {
	case QCoreVariant::String:
	    *l = v_cast<QString>(d->value.ptr)->toLongLong(ok);
	    break;
	case QCoreVariant::ByteArray:
	    *l = QString(*v_cast<QByteArray>(d->value.ptr)).toLongLong(ok);
	    break;
	case QCoreVariant::Int:
	    *l = (Q_LLONG)d->value.i;
	    break;
	case QCoreVariant::UInt:
	    *l = (Q_LLONG)d->value.u;
	    break;
	case QCoreVariant::LongLong:
	    *l = d->value.ll;
	    break;
	case QCoreVariant::ULongLong:
	    *l = (Q_LLONG)d->value.ull;
	    break;
	case QCoreVariant::Double:
	    *l = (Q_LLONG)d->value.d;
	    break;
	case QCoreVariant::Bool:
	    *l = (Q_LLONG)d->value.b;
	    break;
	default:
	    *l = 0;
	    break;
	}
	break;
    }
    case QCoreVariant::ULongLong: {
	Q_ULLONG *l = static_cast<Q_ULLONG *>(result);
	switch (d->type) {
	case QCoreVariant::Int:
	    *l = (Q_ULLONG)d->value.i;
	    break;
	case QCoreVariant::UInt:
	    *l = (Q_ULLONG)d->value.u;
	    break;
	case QCoreVariant::LongLong:
	    *l = (Q_ULLONG)d->value.ll;
	    break;
	case QCoreVariant::ULongLong:
	    *l = d->value.ull;
	    break;
	case QCoreVariant::Double:
	    *l = (Q_ULLONG)d->value.d;
	    break;
	case QCoreVariant::Bool:
	    *l = (Q_ULLONG)d->value.b;
	    break;
	case QCoreVariant::String:
	    *l = v_cast<QString>(d->value.ptr)->toULongLong(ok);
	    break;
	case QCoreVariant::ByteArray:
	    *l = QString(*v_cast<QByteArray>(d->value.ptr)).toULongLong(ok);
	    break;
	default:
	    *l = 0;
	    break;
	}
	break;
    }
    case QCoreVariant::Bool: {
	bool *b = static_cast<bool *>(result);
	switch(d->type) {
	case QCoreVariant::Double:
	    *b = d->value.d != 0.0;
	    break;
	case QCoreVariant::Int:
	    *b = d->value.i != 0;
	    break;
	case QCoreVariant::UInt:
	    *b = d->value.u != 0;
	    break;
	case QCoreVariant::LongLong:
	    *b = d->value.ll != 0;
	    break;
	case QCoreVariant::ULongLong:
	    *b = d->value.ull != 0;
	    break;
	case QCoreVariant::String:
	{
	    QString str = v_cast<QString>(d->value.ptr)->toLower();
	    *b = !(str == "0" || str == "false" || str.isEmpty());
	    break;
	}
	default:
	    *b = false;
	    break;
	}
	break;
    }
    case QCoreVariant::Double: {
	double *f = static_cast<double *>(result);
	switch (d->type) {
	case QCoreVariant::String:
	    *f = v_cast<QString>(d->value.ptr)->toDouble(ok);
	    break;
	case QCoreVariant::ByteArray:
	    *f = QString(*v_cast<QByteArray>(d->value.ptr)).toDouble(ok);
	    break;
	case QCoreVariant::Double:
	    *f = d->value.d;
	    break;
	case QCoreVariant::Int:
	    *f = (double)d->value.i;
	    break;
	case QCoreVariant::Bool:
	    *f = (double)d->value.b;
	    break;
	case QCoreVariant::UInt:
	    *f = (double)d->value.u;
	    break;
	case QCoreVariant::LongLong:
	    *f = (double)d->value.ll;
	    break;
	case QCoreVariant::ULongLong:
#if defined(Q_CC_MSVC) && !defined(Q_CC_MSVC_NET)
	    *f = (double)(Q_LLONG)d->value.ull;
#else
	    *f = (double)d->value.ull;
#endif
	    break;
	default:
	    *f = 0.0;
	    break;
	}
	break;
    }
#ifndef QT_NO_STRINGLIST
    case QCoreVariant::List:
	if (d->type == QCoreVariant::StringList) {
	    QList<QCoreVariant> *lst = static_cast<QList<QCoreVariant> *>(result);
	    QStringList *slist = v_cast<QStringList>(d->value.ptr);
	    for (int i = 0; i < slist->size(); ++i)
		lst->append(QCoreVariant(slist->at(i)));
	}
#endif //QT_NO_STRINGLIST
	break;

    default:
	Q_ASSERT(0);
    }
}

static bool canCast(QCoreVariant::Private *d, QCoreVariant::Type t)
{
    if (d->type == (uint)t)
	return true;

    switch ( t ) {
    case QCoreVariant::Bool:
	return d->type == QCoreVariant::Double || d->type == QCoreVariant::Int
	    || d->type == QCoreVariant::UInt || d->type == QCoreVariant::LongLong
	    || d->type == QCoreVariant::ULongLong || d->type == QCoreVariant::String;
    case QCoreVariant::Int:
	return d->type == QCoreVariant::String || d->type == QCoreVariant::Double
	    || d->type == QCoreVariant::Bool || d->type == QCoreVariant::UInt
	    || d->type == QCoreVariant::LongLong || d->type == QCoreVariant::ULongLong;
    case QCoreVariant::UInt:
	return d->type == QCoreVariant::String || d->type == QCoreVariant::Double
	    || d->type == QCoreVariant::Bool || d->type == QCoreVariant::Int
	    || d->type == QCoreVariant::LongLong || d->type == QCoreVariant::ULongLong;
    case QCoreVariant::LongLong:
	return d->type == QCoreVariant::String || d->type == QCoreVariant::Double
	    || d->type == QCoreVariant::Bool || d->type == QCoreVariant::Int
	    || d->type == QCoreVariant::UInt || d->type == QCoreVariant::ULongLong;
    case QCoreVariant::ULongLong:
	return d->type == QCoreVariant::String || d->type == QCoreVariant::Double
	    || d->type == QCoreVariant::Bool || d->type == QCoreVariant::Int
	    || d->type == QCoreVariant::UInt || d->type == QCoreVariant::LongLong;
    case QCoreVariant::Double:
	return d->type == QCoreVariant::String || d->type == QCoreVariant::Int
	    || d->type == QCoreVariant::Bool || d->type == QCoreVariant::UInt
	    || d->type == QCoreVariant::LongLong || d->type == QCoreVariant::ULongLong;
    case QCoreVariant::String:
	return d->type == QCoreVariant::ByteArray || d->type == QCoreVariant::Int
	    || d->type == QCoreVariant::UInt || d->type == QCoreVariant::Bool
	    || d->type == QCoreVariant::Double || d->type == QCoreVariant::Date
	    || d->type == QCoreVariant::Time || d->type == QCoreVariant::DateTime
	    || d->type == QCoreVariant::LongLong || d->type == QCoreVariant::ULongLong;
    case QCoreVariant::ByteArray:
	return d->type == QCoreVariant::CString || d->type == QCoreVariant::String;
    case QCoreVariant::Date:
	return d->type == QCoreVariant::String || d->type == QCoreVariant::DateTime;
    case QCoreVariant::Time:
	return d->type == QCoreVariant::String || d->type == QCoreVariant::DateTime;
    case QCoreVariant::DateTime:
	return d->type == QCoreVariant::String || d->type == QCoreVariant::Date;
#ifndef QT_NO_STRINGLIST
    case QCoreVariant::List:
	return d->type == QCoreVariant::StringList;
#endif
#ifndef QT_NO_TEMPLATE_VARIANT
    case QCoreVariant::StringList:
	if (d->type == QCoreVariant::List) {
	    const QList<QCoreVariant> &varlist = *v_cast<QList<QCoreVariant> >(d->value.ptr);
	    for (int i = 0; i < varlist.size(); ++i) {
		if (!varlist.at(i).canCast(QCoreVariant::String))
		    return false;
	    }
	    return true;
	}
	return false;
#endif
    default:
	return false;
    }
}

const QCoreVariant::Handler qt_kernel_variant_handler = {
    construct,
    clear,
    isNull,
#ifndef QT_NO_DATASTREAM
    load,
    save,
#endif
    compare,
    cast,
    canCast
};

Q_CORE_EXPORT const QCoreVariant::Handler *qcoreVariantHandler()
{
    return &qt_kernel_variant_handler;
}


const QCoreVariant::Handler *QCoreVariant::handler = &qt_kernel_variant_handler;

/*!
    \class QCoreVariant qvariant.h
    \brief The QCoreVariant class acts like a union for the most common Qt data types.

    \ingroup objectmodel
    \ingroup misc
    \mainclass

    Because C++ forbids unions from including types that have
    non-default constructors or destructors, most interesting Qt
    classes cannot be used in unions. Without QCoreVariant, this would be
    a problem for QObject::property() and for database work, etc.

    A QCoreVariant object holds a single value of a single type() at a
    time. (Some type()s are multi-valued, for example a string list.)
    You can find out what type, T, the variant holds, convert it to a
    different type using one of the asT() functions, e.g. asSize(),
    get its value using one of the toT() functions, e.g. toSize(), and
    check whether the type can be converted to a particular type using
    canCast().

    The methods named toT() (for any supported T, see the \c Type
    documentation for a list) are const. If you ask for the stored
    type, they return a copy of the stored object. If you ask for a
    type that can be generated from the stored type, toT() copies and
    converts and leaves the object itself unchanged. If you ask for a
    type that cannot be generated from the stored type, the result
    depends on the type (see the function documentation for details).

    Note that two data types supported by QCoreVariant are explicitly
    shared, namely QImage and QPointArray, and in these
    cases the toT() methods return a shallow copy. In almost all cases
    you must make a deep copy of the returned values before modifying
    them.

    The asT() functions are not const. They do conversion like the
    toT() methods, set the variant to hold the converted value, and
    return a reference to the new contents of the variant.

    Here is some example code to demonstrate the use of QCoreVariant:

    \code
    QDataStream out(...);
    QCoreVariant v(123);          // The variant now contains an int
    int x = v.toInt();        // x = 123
    out << v;                 // Writes a type tag and an int to out
    v = QCoreVariant("hello");    // The variant now contains a QByteArray
    v = QCoreVariant(tr("hello"));// The variant now contains a QString
    int y = v.toInt();        // y = 0 since v cannot be converted to an int
    QString s = v.toString(); // s = tr("hello")  (see QObject::tr())
    out << v;                 // Writes a type tag and a QString to out
    ...
    QDataStream in(...);      // (opening the previously written stream)
    in >> v;                  // Reads an Int variant
    int z = v.toInt();        // z = 123
    qDebug("Type is %s",      // prints "Type is int"
	    v.typeName());
    v.asInt() += 100;	      // The variant now hold the value 223.
    v = QCoreVariant( QStringList() );
    v.asStringList().append( "Hello" );
    \endcode

    You can even store QList<QCoreVariant>s and
    QMap<QString,QCoreVariant>s in a variant, so you can easily construct
    arbitrarily complex data structures of arbitrary types. This is
    very powerful and versatile, but may prove less memory and speed
    efficient than storing specific types in standard data structures.

    QCoreVariant also supports the notion of NULL values, where you have a
    defined type with no value set.
    \code
    QCoreVariant x, y( QString() ), z( QString("") );
    x.asInt();
    // x.isNull() == true, y.isNull() == true, z.isNull() == false
    \endcode

    See the \link collection.html Collection Classes\endlink.
*/

/*!
    \enum QCoreVariant::Type

    This enum type defines the types of variable that a QCoreVariant can
    contain.

    \value Invalid  no type
    \value BitArray  a QBitArray
    \value ByteArray  a QByteArray
    \value Bitmap  a QBitmap
    \value Bool  a bool
    \value Brush  a QBrush
    \value Color  a QColor
    \value ColorGroup  internal.
    \value Cursor  a QCursor
    \value Date  a QDate
    \value DateTime  a QDateTime
    \value Double  a double
    \value Font  a QFont
    \value IconSet  a QIconSet
    \value Image  a QImage
    \value Int  an int
    \value KeySequence  a QKeySequence
    \value List  a QList<QCoreVariant>
    \value LongLong a long long
    \value ULongLong an unsigned long long
    \value Map  a QMap<QString,QCoreVariant>
    \value Palette  a QPalette
    \value Pen  a QPen
    \value Pixmap  a QPixmap
    \value Point  a QPoint
    \value PointArray  a QPointArray
    \value Rect  a QRect
    \value Region  a QRegion
    \value Size  a QSize
    \value SizePolicy  a QSizePolicy
    \value String  a QString
    \value StringList  a QStringList
    \value Time  a QTime
    \value UInt  an unsigned int

    Note that Qt's definition of bool depends on the compiler.
    \c qglobal.h has the system-dependent definition of bool.
*/

/*!
  \fn QCoreVariant::QCoreVariant()

    Constructs an invalid variant.
*/

/*!
  \fn QCoreVariant::QCoreVariant(Type type, void *v)

    \internal

    Constructs a variant of type \a type, and initializes with \a v if
    \a not 0.
*/


QCoreVariant::Private *QCoreVariant::create(Type t, const void *v)
{
    Private *x = new Private;
    x->ref = 1;
    x->type = t;
    x->is_null = true;
    x->str_cache = 0;
    handler->construct(x, v);
    return x;
}

/*!
  \fn QCoreVariant::~QCoreVariant()

    Destroys the QCoreVariant and the contained object.

    Note that subclasses that reimplement clear() should reimplement
    the destructor to call clear(). This destructor calls clear(), but
    because it is the destructor, QCoreVariant::clear() is called rather
    than a subclass's clear().
*/

/*!
  \fn QCoreVariant::QCoreVariant(const QCoreVariant &p)

    Constructs a copy of the variant, \a p, passed as the argument to
    this constructor. Usually this is a deep copy, but a shallow copy
    is made if the stored data type is explicitly shared, as e.g.
    QImage is.
*/

#ifndef QT_NO_DATASTREAM
/*!
    Reads the variant from the data stream, \a s.
*/
QCoreVariant::QCoreVariant(QDataStream &s)
{
    d = new Private;
    d->ref = 1;
    d->is_null = true;
    d->str_cache = 0;
    s >> *this;
}
#endif //QT_NO_DATASTREAM

/*!
  \fn QCoreVariant::QCoreVariant(const QString &val)

    Constructs a new variant with a string value, \a val.
*/

/*!
  \fn QCoreVariant::QCoreVariant(const char *val)

    Constructs a new variant with a C-string value of \a val if \a val
    is non-null. The variant creates a deep copy of \a val.

    If \a val is null, the resulting variant has type Invalid.
*/

/*!
  \fn QCoreVariant::QCoreVariant(const QStringList &val)

    Constructs a new variant with a string list value, \a val.
*/

/*!
  \fn QCoreVariant::QCoreVariant(const QMap<QString,QCoreVariant> &val)

    Constructs a new variant with a map of QCoreVariants, \a val.
*/

/*!
  \fn QCoreVariant::QCoreVariant(const QDate &val)

    Constructs a new variant with a date value, \a val.
*/

/*!
  \fn QCoreVariant::QCoreVariant(const QTime &val)

    Constructs a new variant with a time value, \a val.
*/

/*!
  \fn QCoreVariant::QCoreVariant(const QDateTime &val)

    Constructs a new variant with a date/time value, \a val.
*/

/*!
  \fn QCoreVariant::QCoreVariant(const QByteArray &val)

    Constructs a new variant with a bytearray value, \a val.
*/

/*!
  \fn QCoreVariant::QCoreVariant(const QBitArray &val)

    Constructs a new variant with a bitarray value, \a val.
*/


/*!
  \fn QCoreVariant::QCoreVariant(int val)

    Constructs a new variant with an integer value, \a val.
*/

/*!
  \fn QCoreVariant::QCoreVariant(uint val)

    Constructs a new variant with an unsigned integer value, \a val.
*/

/*!
  \fn QCoreVariant::QCoreVariant(Q_LLONG val)

    Constructs a new variant with a long long integer value, \a val.
*/

/*!
  \fn QCoreVariant::QCoreVariant(Q_ULLONG val)

    Constructs a new variant with an unsigned long long integer value, \a val.
*/


/*!
  \fn QCoreVariant::QCoreVariant(bool val)

    Constructs a new variant with a boolean value, \a val. The integer
    argument is a dummy, necessary for compatibility with some
    compilers.
*/


/*!
  \fn QCoreVariant::QCoreVariant(double val)

    Constructs a new variant with a floating point value, \a val.
*/

/*!
  \fn QCoreVariant::QCoreVariant(const QList<QCoreVariant> &val)

    Constructs a new variant with a list value, \a val.
*/

/*!
    Assigns the value of the variant \a variant to this variant.

    This is a deep copy of the variant, but note that if the variant
    holds an explicitly shared type such as QImage, a shallow copy is
    performed.
*/
QCoreVariant& QCoreVariant::operator=(const QCoreVariant &variant)
{
    Private *x = variant.d;
    ++x->ref;
    x = qAtomicSetPtr(&d, x);
    if (!--x->ref)
	cleanUp(x);
    return *this;
}

/*!
    \internal
*/
void QCoreVariant::detach_helper()
{
    Private *x = new Private;
    x->ref = 1;
    x->type = d->type;
    x->is_null = true;
    handler->construct(x, data());
    x->is_null = d->is_null;
    x->str_cache = 0;
    if (d->str_cache)
	new (&x->str_cache) QString(*reinterpret_cast<QString *>(&d->str_cache));
    x = qAtomicSetPtr(&d, x);
    if (!--x->ref)
	cleanUp(x);
}

/*!
    Returns the name of the type stored in the variant. The returned
    strings describe the C++ datatype used to store the data: for
    example, "QFont", "QString", or "QList<QCoreVariant>". An Invalid
    variant returns 0.
*/
const char *QCoreVariant::typeName() const
{
    return typeToName((Type)d->type);
}

/*!
    Convert this variant to type Invalid and free up any resources
    used.
*/
void QCoreVariant::clear()
{
    if (d->ref != 1) {
	if (!--d->ref)
	    cleanUp(d);
	d = &shared_invalid;
	return;
    }
    handler->clear(d);
}

/* Attention!

   For dependency reasons, this table is duplicated in moc.y. If you
   change one, change both.

   (Search for the word 'Attention' in moc.y.)
*/
static const int ntypes = 35;
static const char* const type_map[ntypes] =
{
    0,
    "QMap<QString,QCoreVariant>",
    "QList<QCoreVariant>",
    "QString",
    "QStringList",
    "QFont",
    "QPixmap",
    "QBrush",
    "QRect",
    "QSize",
    "QColor",
    "QPalette",
#ifdef QT_COMPAT
    "QColorGroup",
#else
    "",
#endif
    "QIconSet",
    "QPoint",
    "QImage",
    "int",
    "uint",
    "bool",
    "double",
    "",
    "QPointArray",
    "QRegion",
    "QBitmap",
    "QCursor",
    "QSizePolicy",
    "QDate",
    "QTime",
    "QDateTime",
    "QByteArray",
    "QBitArray",
    "QKeySequence",
    "QPen",
    "Q_LLONG",
    "Q_ULLONG"
};


/*!
    Converts the enum representation of the storage type, \a typ, to
    its string representation.
*/
const char *QCoreVariant::typeToName(Type typ)
{
    if (typ >= ntypes)
	return 0;
    return type_map[typ];
}


/*!
    Converts the string representation of the storage type gven in \a
    name, to its enum representation.

    If the string representation cannot be converted to any enum
    representation, the variant is set to \c Invalid.
*/
QCoreVariant::Type QCoreVariant::nameToType(const char *name)
{
    if (name) {
	if (strcmp(name, "") == 0)
	    return Invalid;
	if (strcmp(name, "QCString") == 0)
	    return ByteArray;
	for (int i = 1; i < ntypes; ++i) {
	    if (strcmp(type_map[i], name) == 0)
		return (Type)i;
	}
    }
    return Invalid;
}

#ifndef QT_NO_DATASTREAM
/*!
    Internal function for loading a variant from stream \a s. Use the
    stream operators instead.

    \internal
*/
void QCoreVariant::load(QDataStream &s)
{
    Q_UINT32 u;
    s >> u;
    QCoreVariant::Private *x = create((QCoreVariant::Type)u, 0);
    x->is_null = false;
    x->str_cache = 0;
    handler->load(x, s);
    x = qAtomicSetPtr(&d, x);
    if (!--x->ref)
	cleanUp(x);
}

/*!
    Internal function for saving a variant to the stream \a s. Use the
    stream operators instead.

    \internal
*/
void QCoreVariant::save(QDataStream &s) const
{
    s << (Q_UINT32)type();
    handler->save(d, s);
}

/*!
    Reads a variant \a p from the stream \a s.

    \sa \link datastreamformat.html Format of the QDataStream
    operators \endlink
*/
QDataStream& operator>>(QDataStream &s, QCoreVariant &p)
{
    p.load(s);
    return s;
}

/*!
    Writes a variant \a p to the stream \a s.

    \sa \link datastreamformat.html Format of the QDataStream
    operators \endlink
*/
QDataStream& operator<<(QDataStream &s, const QCoreVariant &p)
{
    p.save(s);
    return s;
}

/*!
    Reads a variant type \a p in enum representation from the stream \a s.
*/
QDataStream& operator>>(QDataStream &s, QCoreVariant::Type &p)
{
    Q_UINT32 u;
    s >> u;
    p = (QCoreVariant::Type)u;

    return s;
}

/*!
    Writes a variant type \a p to the stream \a s.
*/
QDataStream& operator<<(QDataStream &s, const QCoreVariant::Type p)
{
    s << (Q_UINT32)p;

    return s;
}

#endif //QT_NO_DATASTREAM

/*!
    \fn Type QCoreVariant::type() const

    Returns the storage type of the value stored in the variant.
    Usually it's best to test with canCast() whether the variant can
    deliver the data type you are interested in.
*/

/*!
    \fn bool QCoreVariant::isValid() const

    Returns true if the storage type of this variant is not
    QCoreVariant::Invalid; otherwise returns false.
*/

/*! \fn QByteArray QCoreVariant::toCString() const
  \obsolete
    Returns the variant as a QCString if the variant has type()
    CString or String; otherwise returns 0.

    \sa asCString()
*/

#define Q_VARIANT_TO(f) \
Q##f QCoreVariant::to##f() const { \
    if ( d->type == f ) \
        return *v_cast<Q##f >(d->value.ptr); \
    Q##f ret; \
    handler->cast(d, f, &ret, 0); \
    return ret; \
}

#ifndef QT_NO_STRINGLIST
Q_VARIANT_TO(StringList)
#endif
Q_VARIANT_TO(Date)
Q_VARIANT_TO(Time)
Q_VARIANT_TO(DateTime)
Q_VARIANT_TO(ByteArray)

/*!
  \fn QString QCoreVariant::toString() const

    Returns the variant as a QString if the variant has type() String,
    ByteArray, Int, Uint, Bool, Double, Date, Time, DateTime,
    KeySequence, Font or Color; otherwise returns QString::null.

    \sa asString()
*/


/*!
  \fn QStringList QCoreVariant::toStringList() const

    Returns the variant as a QStringList if the variant has type()
    StringList or List of a type that can be converted to QString;
    otherwise returns an empty list.

    Note that if you want to iterate over the list, you should iterate
    over a copy, e.g.
    \code
    QStringList list = myVariant.toStringList();
    QStringList::Iterator it = list.begin();
    while( it != list.end() ) {
	myProcessing( *it );
	++it;
    }
    \endcode

    \sa asStringList()
*/



QString QCoreVariant::toString() const
{
    if (d->type == String)
	return *reinterpret_cast<QString *>(&d->value.ptr);
    if (d->str_cache)
	return *reinterpret_cast<QString *>(&d->str_cache);

    QString ret;
    handler->cast(d, String, &ret, 0);
    new (&d->str_cache) QString(ret);
    return ret;
}
#ifndef QT_NO_TEMPLATE_VARIANT
/*!
    Returns the variant as a QMap<QString,QCoreVariant> if the variant has
    type() Map; otherwise returns an empty map.

    Note that if you want to iterate over the map, you should iterate
    over a copy, e.g.
    \code
    QMap<QString, QCoreVariant> map = myVariant.toMap();
    QMap<QString, QCoreVariant>::Iterator it = map.begin();
    while( it != map.end() ) {
	myProcessing( *it );
	++it;
    }
    \endcode

    \sa asMap()
*/
QMap<QString, QCoreVariant> QCoreVariant::toMap() const
{
    if (d->type != Map)
	return QMap<QString,QCoreVariant>();

    return *v_cast<QVariantMap>(d->value.ptr);
}
#endif

/*!
  \fn QDate QCoreVariant::toDate() const

    Returns the variant as a QDate if the variant has type() Date,
    DateTime or String; otherwise returns an invalid date.

    Note that if the type() is String an invalid date will be returned
    if the string cannot be parsed as a Qt::ISODate format date.

    \sa asDate()
*/


/*!
  \fn QTime QCoreVariant::toTime() const

    Returns the variant as a QTime if the variant has type() Time,
    DateTime or String; otherwise returns an invalid time.

    Note that if the type() is String an invalid time will be returned
    if the string cannot be parsed as a Qt::ISODate format time.

    \sa asTime()
*/

/*!
  \fn QDateTime QCoreVariant::toDateTime() const

    Returns the variant as a QDateTime if the variant has type()
    DateTime, Date or String; otherwise returns an invalid date/time.

    Note that if the type() is String an invalid date/time will be
    returned if the string cannot be parsed as a Qt::ISODate format
    date/time.

    \sa asDateTime()
*/

/*!
  \fn QByteArray QCoreVariant::toByteArray() const

    Returns the variant as a QByteArray if the variant has type()
    ByteArray; otherwise returns an empty bytearray.

    \sa asByteArray()
*/

/*!
    Returns the variant as a QBitArray if the variant has type()
    BitArray; otherwise returns an empty bitarray.

    \sa asBitArray()
*/
QBitArray QCoreVariant::toBitArray() const
{
    if (d->type == BitArray)
	return *v_cast<QBitArray>(d->value.ptr);
    return QBitArray();
}

/*!
    Returns the variant as an int if the variant has type() String,
    Int, UInt, Double, Bool or KeySequence; otherwise returns
    0.

    If \a ok is non-null: \a *ok is set to true if the value could be
    converted to an int; otherwise \a *ok is set to false.

    \sa asInt() canCast()
*/
int QCoreVariant::toInt(bool *ok) const
{
    if (d->type == Int) {
	if (ok)
	    *ok = true;
	return d->value.i;
    }

    bool c = canCast(Int);
    if (ok)
	*ok = c;
    int res = 0;
    if (c)
	handler->cast(d, Int, &res, ok);

    return res;
}

/*!
    Returns the variant as an unsigned int if the variant has type()
    String, ByteArray, UInt, Int, Double, or Bool; otherwise returns 0.

    If \a ok is non-null: \a *ok is set to true if the value could be
    converted to an unsigned int; otherwise \a *ok is set to false.

    \sa asUInt()
*/
uint QCoreVariant::toUInt(bool *ok) const
{
    if (d->type == UInt) {
	if (ok)
	    *ok = true;
	return d->value.u;
    }

    bool c = canCast(UInt);
    if (ok)
	*ok = c;
    uint res = 0;
    if (c)
	handler->cast(d, UInt, &res, ok);

    return res;
}

/*!
    Returns the variant as a long long int if the variant has type()
    LongLong, ULongLong, any type allowing a toInt() conversion;
    otherwise returns 0.

    If \a ok is non-null: \a *ok is set to true if the value could be
    converted to an int; otherwise \a *ok is set to false.

    \sa asLongLong() canCast()
*/
Q_LLONG QCoreVariant::toLongLong(bool *ok) const
{
    if (d->type == LongLong) {
	if (ok)
	    *ok = true;
	return d->value.ll;
    }

    bool c = canCast(LongLong);
    if (ok)
	*ok = c;
    Q_LLONG res = 0;
    if (c)
	handler->cast(d, LongLong, &res, ok);

    return res;
}

/*!
    Returns the variant as as an unsigned long long int if the variant
    has type() LongLong, ULongLong, any type allowing a toUInt()
    conversion; otherwise returns 0.

    If \a ok is non-null: \a *ok is set to true if the value could be
    converted to an int; otherwise \a *ok is set to false.

    \sa asULongLong() canCast()
*/
Q_ULLONG QCoreVariant::toULongLong(bool *ok) const
{
    if (d->type == ULongLong) {
	if (ok)
	    *ok = true;
	return d->value.ull;
    }

    bool c = canCast(ULongLong);
    if (ok)
	*ok = c;
    Q_ULLONG res = 0;
    if (c)
	handler->cast(d, ULongLong, &res, ok);

    return res;
}

/*!
    Returns the variant as a bool if the variant has type() Bool.

    Returns true if the variant has type Int, UInt or Double and its
    value is non-zero, or if the variant has type String and its lower-case
    content is not empty, "0" or "false"; otherwise returns false.

    \sa asBool()
*/
bool QCoreVariant::toBool() const
{
    if (d->type == Bool)
	return d->value.b;

    bool res = false;
    handler->cast(d, Bool, &res, 0);

    return res;
}

/*!
    Returns the variant as a double if the variant has type() String,
    ByteArray, Double, Int, UInt, LongLong, ULongLong or Bool; otherwise
    returns 0.0.

    If \a ok is non-null: \a *ok is set to true if the value could be
    converted to a double; otherwise \a *ok is set to false.

    \sa asDouble()
*/
double QCoreVariant::toDouble(bool *ok) const
{
    if (d->type == Double) {
	if (ok)
	*ok = true;
	return d->value.d;
    }

    bool c = canCast(Double);
    if (ok)
	*ok = c;
    double res = 0;
    if (c)
	handler->cast(d, Double, &res, ok);

    return res;
}

#ifndef QT_NO_TEMPLATE_VARIANT
/*!
  \fn QList<QCoreVariant> QCoreVariant::toList() const

    Returns the variant as a QList<QCoreVariant> if the variant has
    type() List or StringList; otherwise returns an empty list.

    Note that if you want to iterate over the list, you should iterate
    over a copy, e.g.
    \code
    QList<QCoreVariant> list = myVariant.toList();
    QList<QCoreVariant>::Iterator it = list.begin();
    while( it != list.end() ) {
	myProcessing( *it );
	++it;
    }
    \endcode

    \sa asList()
*/
QList<QCoreVariant> QCoreVariant::toList() const
{
    if (d->type == List)
	return *v_cast<QList<QCoreVariant> >(d->value.ptr);
    QList<QCoreVariant> res;
    handler->cast(d, List, &res, 0);
    return res;
}
#endif


/*!
    \fn QString& QCoreVariant::asString()

    Tries to convert the variant to hold a string value. If that is
    not possible the variant is set to an empty string.

    Returns a reference to the stored string.

    \sa toString()
*/

/*!
    \fn QCString& QCoreVariant::asCString()

    \obsolete

    Tries to convert the variant to hold a string value. If that is
    not possible the variant is set to an empty string.

    Returns a reference to the stored string.

    \sa toCString()
*/

/*!
    \fn QStringList& QCoreVariant::asStringList()

    Tries to convert the variant to hold a QStringList value. If that
    is not possible the variant is set to an empty string list.

    Returns a reference to the stored string list.

    Note that if you want to iterate over the list, you should
    iterate over a copy, e.g.
    \code
    QStringList list = myVariant.asStringList();
    QStringList::Iterator it = list.begin();
    while( it != list.end() ) {
	myProcessing( *it );
	++it;
    }
    \endcode

    \sa toStringList()
*/

/*!
    \fn QFont& QCoreVariant::asFont()

    Tries to convert the variant to hold a QFont. If that is not
    possible the variant is set to the application's default font.

    Returns a reference to the stored font.

    \sa toFont()
*/

/*!
    \fn QPixmap& QCoreVariant::asPixmap()

    Tries to convert the variant to hold a pixmap value. If that is
    not possible the variant is set to a null pixmap.

    Returns a reference to the stored pixmap.

    \sa toPixmap()
*/

/*!
    \fn QImage& QCoreVariant::asImage()

    Tries to convert the variant to hold an image value. If that is
    not possible the variant is set to a null image.

    Returns a reference to the stored image.

    \sa toImage()
*/

/*!
    \fn QBrush& QCoreVariant::asBrush()

    Tries to convert the variant to hold a brush value. If that is not
    possible the variant is set to a default black brush.

    Returns a reference to the stored brush.

    \sa toBrush()
*/

/*!
    \fn QSizePolicy& QCoreVariant::asSizePolicy()

    Tries to convert the variant to hold a QSizePolicy value. If that
    fails, the variant is set to an arbitrary (valid) size policy.
*/


/*!
    \fn QColor& QCoreVariant::asColor()

    Tries to convert the variant to hold a QColor value. If that is
    not possible the variant is set to an invalid color.

    Returns a reference to the stored color.

    \sa toColor() QColor::isValid()
*/

/*!
    \fn QPalette& QCoreVariant::asPalette()

    Tries to convert the variant to hold a QPalette value. If that is
    not possible the variant is set to a palette of black colors.

    Returns a reference to the stored palette.

    \sa toString()
*/

/*!
    \fn QIconSet& QCoreVariant::asIconSet()

    Tries to convert the variant to hold a QIconSet value. If that is
    not possible the variant is set to an empty iconset.

    Returns a reference to the stored iconset.

    \sa toIconSet()
*/

/*!
    \fn QPointArray& QCoreVariant::asPointArray()

    Tries to convert the variant to hold a QPointArray value. If that
    is not possible the variant is set to an empty point array.

    Returns a reference to the stored point array.

    \sa toPointArray()
*/

/*!
    \fn QBitmap& QCoreVariant::asBitmap()

    Tries to convert the variant to hold a bitmap value. If that is
    not possible the variant is set to a null bitmap.

    Returns a reference to the stored bitmap.

    \sa toBitmap()
*/

/*!
    \fn QRegion& QCoreVariant::asRegion()

    Tries to convert the variant to hold a QRegion value. If that is
    not possible the variant is set to a null region.

    Returns a reference to the stored region.

    \sa toRegion()
*/

/*!
    \fn QCursor& QCoreVariant::asCursor()

    Tries to convert the variant to hold a QCursor value. If that is
    not possible the variant is set to a default arrow cursor.

    Returns a reference to the stored cursor.

    \sa toCursor()
*/

/*!
    \fn QDate& QCoreVariant::asDate()

    Tries to convert the variant to hold a QDate value. If that is not
    possible then the variant is set to an invalid date.

    Returns a reference to the stored date.

    \sa toDate()
*/

/*!
    \fn QTime& QCoreVariant::asTime()

    Tries to convert the variant to hold a QTime value. If that is not
    possible then the variant is set to an invalid time.

    Returns a reference to the stored time.

    \sa toTime()
*/

/*!
    \fn QDateTime& QCoreVariant::asDateTime()

    Tries to convert the variant to hold a QDateTime value. If that is
    not possible then the variant is set to an invalid date/time.

    Returns a reference to the stored date/time.

    \sa toDateTime()
*/

/*!
    \fn QByteArray& QCoreVariant::asByteArray()

    Tries to convert the variant to hold a QByteArray value. If that
    is not possible then the variant is set to an empty bytearray.

    Returns a reference to the stored bytearray.

    \sa toByteArray()
*/

/*!
    \fn QBitArray& QCoreVariant::asBitArray()

    Tries to convert the variant to hold a QBitArray value. If that is
    not possible then the variant is set to an empty bitarray.

    Returns a reference to the stored bitarray.

    \sa toBitArray()
*/

/*!
    \fn QKeySequence& QCoreVariant::asKeySequence()

    Tries to convert the variant to hold a QKeySequence value. If that
    is not possible then the variant is set to an empty key sequence.

    Returns a reference to the stored key sequence.

    \sa toKeySequence()
*/

/*! \fn QPen& QCoreVariant::asPen()

  Tries to convert the variant to hold a QPen value. If that
  is not possible then the variant is set to an empty pen.

  Returns a reference to the stored pen.

  \sa toPen()
*/

/*!
  \fn int &QCoreVariant::asInt()

    Returns the variant's value as int reference.
*/

/*!
  \fn uint &QCoreVariant::asUInt()

    Returns the variant's value as unsigned int reference.
*/

/*!
  \fn Q_LLONG &QCoreVariant::asLongLong()

    Returns the variant's value as long long reference.
*/

/*!
  \fn Q_ULLONG &QCoreVariant::asULongLong()

    Returns the variant's value as unsigned long long reference.
*/

/*!
  \fn bool &QCoreVariant::asBool()

    Returns the variant's value as bool reference.
*/

/*!
  \fn double &QCoreVariant::asDouble()

    Returns the variant's value as double reference.
*/

/*!
  \fn QList<QCoreVariant>& QCoreVariant::asList()

    Returns the variant's value as variant list reference.

    Note that if you want to iterate over the list, you should iterate
    over a copy, e.g.
    \code
    QList<QCoreVariant> list = myVariant.asList();
    QList<QCoreVariant>::Iterator it = list.begin();
    while( it != list.end() ) {
	myProcessing( *it );
	++it;
    }
    \endcode
*/

/*!
  \fn QMap<QString, QCoreVariant>& QCoreVariant::asMap()

    Returns the variant's value as variant map reference.

    Note that if you want to iterate over the map, you should iterate
    over a copy, e.g.
    \code
    QMap<QString, QCoreVariant> map = myVariant.asMap();
    QMap<QString, QCoreVariant>::Iterator it = map.begin();
    while( it != map.end() ) {
	myProcessing( *it );
	++it;
    }
    \endcode
*/

/*!
    Returns true if the variant's type can be cast to the requested
    type, \a t. Such casting is done automatically when calling the
    toInt(), toBool(), ... or asInt(), asBool(), ... methods.

    The following casts are done automatically:
    \table
    \header \i Type \i Automatically Cast To
    \row \i Bool \i Double, Int, UInt, LongLong, ULongLong
    \row \i Color \i String
    \row \i Date \i String, DateTime
    \row \i DateTime \i String, Date, Time
    \row \i Double \i String, Int, Bool, UInt
    \row \i Font \i String
    \row \i Int \i String, Double, Bool, UInt
    \row \i List \i StringList (if the list contains strings or
    something that can be cast to a string)
    \row \i String \i CString, Int, Uint, Bool, Double, Date,
    Time, DateTime, KeySequence, Font, Color
    \row \i CString \i String
    \row \i StringList \i List
    \row \i Time \i String
    \row \i UInt \i String, Double, Bool, Int
    \row \i KeySequence \i String, Int
    \endtable
*/
bool QCoreVariant::canCast(Type t) const
{
    return handler->canCast(d, t);
}

/*!
    Casts the variant to the requested type. If the cast cannot be
    done, the variant is set to the default value of the requested
    type (e.g. an empty string if the requested type \a t is
    QCoreVariant::String, an empty point array if the requested type \a t
    is QCoreVariant::PointArray, etc). Returns true if the current type of
    the variant was successfully cast; otherwise returns false.

    \sa canCast()
*/

bool QCoreVariant::cast(Type t)
{
    if (d->type == (uint)t)
	return true;

    // clear str_cache
    if (d->str_cache) {
	reinterpret_cast<QString *>(&d->str_cache)->~QString();
	d->str_cache = 0;
    }

    bool c = handler->canCast(d, t);

    Private *x = create(t, 0);
    x = qAtomicSetPtr(&d, x);
    if (c)
	handler->cast(x, t, data(), 0);
    if (!--x->ref)
	cleanUp(x);
    return c;
}

/*!
    Compares this QCoreVariant with \a v and returns true if they are
    equal; otherwise returns false.
*/

bool QCoreVariant::operator==(const QCoreVariant &v) const
{
    QCoreVariant v2 = v;
    if (d->type != v2.d->type) {
	if (!v2.canCast((Type)d->type))
	    return false;
	v2.cast((Type)d->type);
    }
    return handler->compare(d, v2.d);
}

/*!
    \fn bool QCoreVariant::operator!=( const QCoreVariant &v ) const
    Compares this QCoreVariant with \a v and returns true if they are not
    equal; otherwise returns false.
*/

/*! \internal

  Reads or sets the variant type and ptr
 */
void *QCoreVariant::rawAccess(void *ptr, Type typ, bool deepCopy)
{
    if (ptr) {
	clear();
	d->type = typ;
	d->value.ptr = ptr;
	d->is_null = false;
	if (deepCopy) {
	    Private *x = new Private;
	    x->ref = 1;
	    x->type = d->type;
	    handler->construct(x, data());
	    x->is_null = d->is_null;
	    x = qAtomicSetPtr(&d, x);
	    if (!--x->ref)
		cleanUp(x);
	}
    }
    if (!deepCopy)
	return d->value.ptr;
    Private *p = new Private;
    p->type = d->type;
    handler->construct(p, data());
    void *ret = (void*)p->value.ptr;
    p->type = Invalid;
    delete p;
    return ret;
}

#define QDATA(vType) \
    if (QTypeInfo<vType >::isLarge) \
        return d->value.ptr; \
    else \
        return &d->value.ptr

/*! \internal
 */
void* QCoreVariant::data()
{
    switch(d->type) {
    case Int:
    case UInt:
    case LongLong:
    case ULongLong:
    case Double:
    case Bool:
	return &d->value;
    case String:
        QDATA(QString);
#ifndef QT_NO_STRINGLIST
    case StringList:
        QDATA(QStringList);
#endif
#ifndef QT_NO_TEMPLATE_VARIANT
    case Map:
	QDATA(QVariantMap);
    case List:
	QDATA(QList<QCoreVariant>);
#endif
    case Date:
	QDATA(QDate);
    case Time:
	QDATA(QTime);
    case QCoreVariant::DateTime:
	QDATA(QDateTime);
    case QCoreVariant::ByteArray:
	QDATA(QByteArray);
    case QCoreVariant::BitArray:
	QDATA(QBitArray);
    default:
	return d->value.ptr;
    }
}


/*! \internal
 */
void *QCoreVariant::castOrDetach(Type t)
{
    if ( d->type != (uint)t )
	cast(t);
    else
	detach();
    return data();
}

/*!
  Returns true if this is a NULL variant, false otherwise.
*/
bool QCoreVariant::isNull() const
{
    return handler->isNull(d);
}


#endif //QT_NO_VARIANT
