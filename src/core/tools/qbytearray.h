/****************************************************************************
**
** Definition of QByteArray class.
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the tools module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QBYTEARRAY_H
#define QBYTEARRAY_H

#ifndef QT_H
#include "qglobal.h"
#include "qatomic.h"
#endif // QT_H

#include <string.h>

/*****************************************************************************
  Safe and portable C string functions; extensions to standard string.h
 *****************************************************************************/

Q_CORE_EXPORT char *qstrdup(const char *);

inline uint qstrlen(const char *str)
{ return str ? (uint)strlen(str) : 0; }
inline char *qstrcpy(char *dst, const char *src)
{ return src ? strcpy(dst, src) : 0; }
Q_CORE_EXPORT char *qstrncpy(char *dst, const char *src, uint len);

inline int qstrcmp(const char *str1, const char *str2)
{
    return (str1 && str2) ? strcmp(str1, str2)
        : (str1 ? 1 : (str2 ? -1 : 0));
}
inline int qstrncmp(const char *str1, const char *str2, uint len)
{
    return (str1 && str2) ? strncmp(str1, str2, len)
        : (str1 ? 1 : (str2 ? -1 : 0));
}
Q_CORE_EXPORT int qstricmp(const char *, const char *);
Q_CORE_EXPORT int qstrnicmp(const char *, const char *, uint len);

#ifdef QT_COMPAT
inline QT_COMPAT void *qmemmove(void *dst, const void *src, uint len)
{ return memmove(dst, src, len); }
inline QT_COMPAT uint cstrlen(const char *str)
{ return (uint)strlen(str); }
inline QT_COMPAT char *cstrcpy(char *dst, const char *src)
{ return strcpy(dst,src); }
inline QT_COMPAT int cstrcmp(const char *str1, const char *str2)
{ return strcmp(str1,str2); }
inline QT_COMPAT int cstrncmp(const char *str1, const char *str2, uint len)
{ return strncmp(str1,str2,len); }
#endif

// qChecksum: Internet checksum

Q_CORE_EXPORT Q_UINT16 qChecksum(const char *s, uint len);

class QByteRef;
class QString;
class QDataStream;
template <typename T> class QList;

class Q_CORE_EXPORT QByteArray
{
public:
    QByteArray();
    QByteArray(const char *);
    QByteArray(const char *, int size);
    QByteArray(int size, char c);
    QByteArray(const QByteArray &);
    ~QByteArray();

    inline QByteArray &operator=(const QByteArray &);
    QByteArray &operator=(const char *str);

    inline int size() const;
    bool isEmpty() const;
    void resize(int size);

    QByteArray &fill(char c, int size = -1);

    int capacity() const;
    void reserve(int size);
    void squeeze();

    operator const char *() const;
    operator const void *() const;
    char *data();
    const char *data() const;
    inline const char *constData() const;
    inline void detach();
    bool isDetached() const;
    void clear();

    char at(int i) const;
    char operator[](int i) const;
    QByteRef operator[](int i);
    char operator[](uint i) const;
    QByteRef operator[](uint i);

    int indexOf(char c, int from = 0) const;
    inline int indexOf(const char *c, int from = 0) const;
    int indexOf(const QByteArray &a, int from = 0) const;
    int lastIndexOf(char c, int from = -1) const;
    inline int lastIndexOf(const char *c, int from = -1) const;
    int lastIndexOf(const QByteArray &a, int from = -1) const;

    QBool contains(char c) const;
    QBool contains(const char *a) const;
    QBool contains(const QByteArray &a) const;
    int count(char c) const;
    int count(const char *a) const;
    int count(const QByteArray &a) const;

    QByteArray left(int len)  const;
    QByteArray right(int len) const;
    QByteArray mid(int index, int len = -1) const;

    bool startsWith(const QByteArray& a) const;
    bool startsWith(char c) const;
    bool startsWith(const char *c) const;

    bool endsWith(const QByteArray& a) const;
    bool endsWith(char c) const;
    bool endsWith(const char *c) const;

    inline void truncate(int l) { if (l < d->size) resize(l); }

    QByteArray toLower() const;
    QByteArray toUpper() const;

    QByteArray trimmed() const;
    QByteArray simplified() const;
    QByteArray leftJustified(int width, char fill = ' ', bool truncate = false) const;
    QByteArray rightJustified(int width, char fill = ' ', bool truncate = false) const;

#ifdef QT_COMPAT
    inline QByteArray leftJustify(uint width, char fill = ' ', bool truncate = false) const
    { return leftJustified((int)width, fill, truncate); }
    inline QByteArray rightJustify(uint width, char fill = ' ', bool truncate = false) const
    { return rightJustified((int)width, fill, truncate); }
#endif

    QByteArray &prepend(char c);
    QByteArray &prepend(const char *s);
    QByteArray &prepend(const QByteArray &a);
    QByteArray &append(char c);
    QByteArray &append(const char *s);
    QByteArray &append(const QByteArray &a);
    QByteArray &insert(int i, char c);
    QByteArray &insert(int i, const char *s);
    QByteArray &insert(int i, const QByteArray &a);
    QByteArray &remove(int index, int len);
    QByteArray &replace(int index, int len, const char *s);
    QByteArray &replace(int index, int len, const QByteArray &s);
    QByteArray &replace(char before, const char *after);
    QByteArray &replace(char before, const QByteArray &after);
    QByteArray &replace(const char *before, const char *after);
    QByteArray &replace(const QByteArray &before, const QByteArray &after);
    QByteArray &replace(const QByteArray &before, const char *after);
    QByteArray &replace(char before, char after);
    QByteArray &operator+=(char c);
    QByteArray &operator+=(const char *s);
    QByteArray &operator+=(const QByteArray &a);

    QList<QByteArray> split(char sep) const;

#ifndef QT_NO_CAST_TO_ASCII
    QByteArray &append(const QString &s);
    QByteArray &insert(int i, const QString &s);
    QByteArray &replace(const QString &before, const char *after);
    QByteArray &replace(char c, const QString &after);
    QByteArray &replace(const QString &before, const QByteArray &after);

    QByteArray &operator+=(const QString &s);
    int indexOf(const QString &s, int from = 0) const;
    int lastIndexOf(const QString &s, int from = -1) const;
#endif
#ifndef QT_NO_CAST_FROM_ASCII
    inline bool operator==(const QString &s2) const;
    inline bool operator!=(const QString &s2) const;
    inline bool operator<(const QString &s2) const;
    inline bool operator>(const QString &s2) const;
    inline bool operator<=(const QString &s2) const;
    inline bool operator>=(const QString &s2) const;
#endif

    short toShort(bool *ok = 0, int base = 10) const;
    ushort toUShort(bool *ok = 0, int base = 10) const;
    int toInt(bool *ok = 0, int base = 10)   const;
    uint toUInt(bool *ok = 0, int base = 10)  const;
    long toLong(bool *ok = 0, int base = 10)  const;
    ulong toULong(bool *ok = 0, int base = 10) const;
    Q_LLONG toLongLong(bool *ok = 0, int base = 10) const;
    Q_ULLONG toULongLong(bool *ok = 0, int base = 10) const;
    float toFloat(bool *ok = 0) const;
    double toDouble(bool *ok = 0) const;

    QByteArray &setNum(short, int base = 10);
    QByteArray &setNum(ushort, int base = 10);
    QByteArray &setNum(int, int base = 10);
    QByteArray &setNum(uint, int base = 10);
    QByteArray &setNum(long, int base = 10);
    QByteArray &setNum(ulong, int base = 10);
    QByteArray &setNum(Q_LLONG, int base = 10);
    QByteArray &setNum(Q_ULLONG, int base = 10);
    QByteArray &setNum(float, char f = 'g', int prec = 6);
    QByteArray &setNum(double, char f = 'g', int prec = 6);

    static QByteArray number(long, int base = 10);
    static QByteArray number(ulong, int base = 10);
    static QByteArray number(int, int base = 10);
    static QByteArray number(uint, int base = 10);
    static QByteArray number(Q_LLONG, int base = 10);
    static QByteArray number(Q_ULLONG, int base = 10);
    static QByteArray number(double, char f = 'g', int prec = 6);
    static QByteArray fromRawData(const char *, int size);

    typedef char *iterator;
    typedef const char *const_iterator;
    typedef iterator Iterator;
    typedef const_iterator ConstIterator;
    iterator begin();
    const_iterator begin() const;
    const_iterator constBegin() const;
    iterator end();
    const_iterator end() const;
    const_iterator constEnd() const;

    // stl compatibility
    void push_back(char c);
    void push_back(const char *c);
    void push_back(const QByteArray &a);
    void push_front(char c);
    void push_front(const char *c);
    void push_front(const QByteArray &a);

    inline int count() const { return d->size; }
    int length() const { return d->size; }
    bool isNull() const;

    // compatibility
#ifdef QT_COMPAT
    explicit QByteArray(int size);
    QT_COMPAT QByteArray& duplicate(const QByteArray& a)
    { *this = a; return *this; }
    QT_COMPAT QByteArray& duplicate(const char *a, uint n)
    { *this = QByteArray(a, n); return *this; }
    QT_COMPAT QByteArray& setRawData(const char *a, uint n)
    { *this = fromRawData(a, n); return *this; }
    QT_COMPAT void resetRawData(const char *, uint)
    { clear(); }
    inline QT_COMPAT QByteArray lower() const { return toLower(); }
    inline QT_COMPAT QByteArray upper() const { return toUpper(); }
    inline QT_COMPAT QByteArray stripWhiteSpace() const { return trimmed(); }
    inline QT_COMPAT QByteArray simplifyWhiteSpace() const { return simplified(); }
    inline QT_COMPAT int find(char c, int from = 0) const { return indexOf(c, from); }
    inline QT_COMPAT int find(const char *c, int from = 0) const { return indexOf(c, from); }
    inline QT_COMPAT int find(const QByteArray &ba, int from = 0) const { return indexOf(ba, from); }
    inline QT_COMPAT int findRev(char c, int from = -1) const { return lastIndexOf(c, from); }
    inline QT_COMPAT int findRev(const char *c, int from = -1) const { return lastIndexOf(c, from); }
    inline QT_COMPAT int findRev(const QByteArray &ba, int from = -1) const { return lastIndexOf(ba, from); }
#ifndef QT_NO_CAST_TO_ASCII
    QT_COMPAT int find(const QString &s, int from = 0) const;
    QT_COMPAT int findRev(const QString &s, int from = -1) const;
#endif
#endif

    bool ensure_constructed();

private:
    operator QNoImplicitIntegralCast() const;
    struct Data {
        QAtomic ref;
        int alloc, size;
        char *data;
        char array[1];
    };
    static Data shared_null;
    static Data shared_empty;
    Data *d;
    QByteArray(Data *dd, int /* dummy */) : d(dd) {}
    void realloc(int alloc);
    void expand(int i);

    friend class QByteRef;
    friend class QString;
};

inline int QByteArray::size() const
{ return d->size; }
inline char QByteArray::at(int i) const
{ Q_ASSERT(i >= 0 && i < size()); return d->data[i]; }
inline char QByteArray::operator[](int i) const
{ Q_ASSERT(i >= 0 && i < size()); return d->data[i]; }
inline char QByteArray::operator[](uint i) const
{ Q_ASSERT(i < (uint)size()); return d->data[i]; }
inline bool QByteArray::isEmpty() const
{ return d->size == 0; }
inline QByteArray::operator const char *() const
{ return d->data; }
inline QByteArray::operator const void *() const
{ return d->data; }
inline char *QByteArray::data()
{ detach(); return d->data; }
inline const char *QByteArray::data() const
{ return d->data; }
inline const char *QByteArray::constData() const
{ return d->data; }
inline void QByteArray::detach()
{ if (d->ref != 1 || d->data != d->array) realloc(d->size); }
inline bool QByteArray::isDetached() const
{ return d->ref == 1; }
inline QByteArray::~QByteArray()
{ if (!d) return; if (!--d->ref) qFree(d); }
inline QByteArray::QByteArray(const QByteArray &a) : d(a.d)
{ ++d->ref; }
#ifdef QT_COMPAT
inline QByteArray::QByteArray(int size) : d(&shared_null)
{ ++d->ref; if (size > 0) fill('\0', size); }
#endif

inline int QByteArray::capacity() const
{ return d->alloc; }

inline void QByteArray::reserve(int size)
{ if (d->ref != 1 || size > d->alloc) realloc(size); }

inline void QByteArray::squeeze()
{ if (d->size < d->alloc) realloc(d->size); }

class Q_CORE_EXPORT QByteRef {
    QByteArray &a;
    int i;
    inline QByteRef(QByteArray &array, int idx)
        : a(array),i(idx) {}
    friend class QByteArray;
public:
    inline operator char() const
        { return i < a.d->size ? a.d->data[i] : 0; }
    inline QByteRef &operator=(char c)
        { if (a.d->ref != 1 || i >= a.d->size) a.expand(i);
          a.d->data[i] = c;  return *this; }
    inline QByteRef &operator=(const QByteRef &c)
        { if (a.d->ref != 1 || i >= a.d->size) a.expand(i);
          a.d->data[i] = c.a.d->data[c.i];  return *this; }
    inline bool operator==(char c) const
    { return a.d->data[i] == c; }
    inline bool operator!=(char c) const
    { return a.d->data[i] != c; }
    inline bool operator>(char c) const
    { return a.d->data[i] > c; }
    inline bool operator>=(char c) const
    { return a.d->data[i] >= c; }
    inline bool operator<(char c) const
    { return a.d->data[i] < c; }
    inline bool operator<=(char c) const
    { return a.d->data[i] <= c; }
};

inline QByteRef QByteArray::operator[](int i)
{ Q_ASSERT(i >= 0); return QByteRef(*this, i); }
inline QByteRef QByteArray::operator[](uint i)
{ return QByteRef(*this, i); }
inline QByteArray::iterator QByteArray::begin()
{ detach(); return d->data; }
inline QByteArray::const_iterator QByteArray::begin() const
{ return d->data; }
inline QByteArray::const_iterator QByteArray::constBegin() const
{ return d->data; }
inline QByteArray::iterator QByteArray::end()
{ detach(); return d->data + d->size; }
inline QByteArray::const_iterator QByteArray::end() const
{ return d->data + d->size; }
inline QByteArray::const_iterator QByteArray::constEnd() const
{ return d->data + d->size; }
inline QByteArray &QByteArray::operator+=(char c)
{ return append(c); }
inline QByteArray &QByteArray::operator+=(const char *s)
{ return append(s); }
inline QByteArray &QByteArray::operator+=(const QByteArray &a)
{ return append(a); }
inline void QByteArray::push_back(char c)
{ append(c); }
inline void QByteArray::push_back(const char *c)
{ append(c); }
inline void QByteArray::push_back(const QByteArray &a)
{ append(a); }
inline void QByteArray::push_front(char c)
{ prepend(c); }
inline void QByteArray::push_front(const char *c)
{ prepend(c); }
inline void QByteArray::push_front(const QByteArray &a)
{ prepend(a); }
inline QBool QByteArray::contains(const QByteArray &a) const
{ return QBool(indexOf(a) != -1); }
inline QBool QByteArray::contains(char c) const
{ return QBool(indexOf(c) != -1); }
inline QByteArray &QByteArray::operator=(const QByteArray & a)
{
    Data *x = a.d; ++x->ref;
    x = qAtomicSetPtr(&d, x);
    if (!--x->ref) qFree(x);
    return *this;
}

inline bool operator==(const QByteArray &a1, const QByteArray &a2)
{ return (a1.size() == a2.size()) && (memcmp(a1, a2, a1.size())==0); }
inline bool operator==(const QByteArray &a1, const char *a2)
{ return a2 ? strcmp(a1,a2) == 0 : a1.isEmpty(); }
inline bool operator==(const char *a1, const QByteArray &a2)
{ return a1 ? strcmp(a1,a2) == 0 : a2.isEmpty(); }
inline bool operator!=(const QByteArray &a1, const QByteArray &a2)
{ return !(a1==a2); }
inline bool operator!=(const QByteArray &a1, const char *a2)
{ return a2 ? strcmp(a1,a2) != 0 : !a1.isEmpty(); }
inline bool operator!=(const char *a1, const QByteArray &a2)
{ return a1 ? strcmp(a1,a2) != 0 : !a2.isEmpty(); }
inline bool operator<(const QByteArray &a1, const QByteArray &a2)
{ return strcmp(a1, a2) < 0; }
 inline bool operator<(const QByteArray &a1, const char *a2)
{ return qstrcmp(a1, a2) < 0; }
inline bool operator<(const char *a1, const QByteArray &a2)
{ return qstrcmp(a1, a2) < 0; }
inline bool operator<=(const QByteArray &a1, const QByteArray &a2)
{ return strcmp(a1, a2) <= 0; }
inline bool operator<=(const QByteArray &a1, const char *a2)
{ return qstrcmp(a1, a2) <= 0; }
inline bool operator<=(const char *a1, const QByteArray &a2)
{ return qstrcmp(a1, a2) <= 0; }
inline bool operator>(const QByteArray &a1, const QByteArray &a2)
{ return strcmp(a1, a2) > 0; }
inline bool operator>(const QByteArray &a1, const char *a2)
{ return qstrcmp(a1, a2) > 0; }
inline bool operator>(const char *a1, const QByteArray &a2)
{ return qstrcmp(a1, a2) > 0; }
inline bool operator>=(const QByteArray &a1, const QByteArray &a2)
{ return strcmp(a1, a2) >= 0; }
inline bool operator>=(const QByteArray &a1, const char *a2)
{ return qstrcmp(a1, a2) >= 0; }
inline bool operator>=(const char *a1, const QByteArray &a2)
{ return qstrcmp(a1, a2) >= 0; }
inline const QByteArray operator+(const QByteArray &a1, const QByteArray &a2)
{ return QByteArray(a1) += a2; }
inline const QByteArray operator+(const QByteArray &a1, const char *a2)
{ return QByteArray(a1) += a2; }
inline const QByteArray operator+(const QByteArray &a1, char a2)
{ return QByteArray(a1) += a2; }
inline const QByteArray operator+(const char *a1, const QByteArray &a2)
{ return QByteArray(a1) += a2; }
inline const QByteArray operator+(char a1, const QByteArray &a2)
{ return QByteArray(&a1, 1) += a2; }
inline int QByteArray::indexOf(const char *c, int i) const
{ return indexOf(fromRawData(c, strlen(c)), i); }
inline int QByteArray::lastIndexOf(const char *c, int i) const
{ return lastIndexOf(fromRawData(c, strlen(c)), i); }
inline QBool QByteArray::contains(const char *c) const
{ return contains(fromRawData(c, strlen(c))); }
inline QByteArray &QByteArray::replace(int index, int len, const char *c)
{ return replace(index, len, fromRawData(c, strlen(c))); }
inline QByteArray &QByteArray::replace(char before, const char *c)
{ return replace(before, fromRawData(c, strlen(c))); }
inline QByteArray &QByteArray::replace(const QByteArray &before, const char *c)
{ return replace(before, fromRawData(c, strlen(c))); }
inline QByteArray &QByteArray::replace(const char *before, const char *after)
{ return replace(fromRawData(before, strlen(before)), fromRawData(after, strlen(after))); }

inline QByteArray &QByteArray::setNum(short n, int base)
{ return setNum((Q_LLONG)n, base); }
inline QByteArray &QByteArray::setNum(ushort n, int base)
{ return setNum((Q_ULLONG)n, base); }
inline QByteArray &QByteArray::setNum(int n, int base)
{ return setNum((Q_LLONG)n, base); }
inline QByteArray &QByteArray::setNum(uint n, int base)
{ return setNum((Q_ULLONG)n, base); }
inline QByteArray &QByteArray::setNum(long n, int base)
{ return setNum((Q_LLONG)n, base); }
inline QByteArray &QByteArray::setNum(ulong n, int base)
{ return setNum((Q_ULLONG)n, base); }
inline QByteArray &QByteArray::setNum(float n, char f, int prec)
{ return setNum((double)n,f,prec); }


#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QByteArray &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QByteArray &);
#endif

#ifndef QT_NO_COMPRESS
Q_CORE_EXPORT QByteArray qCompress(const uchar* data, int nbytes, int compressionLevel = -1);
Q_CORE_EXPORT QByteArray qUncompress(const uchar* data, int nbytes);
inline QByteArray qCompress(const QByteArray& data, int compressionLevel = -1)
{ return qCompress((const uchar *)data.constData(), data.size(), compressionLevel); }
inline QByteArray qUncompress(const QByteArray& data)
{ return qUncompress((const uchar*)data.constData(), data.size()); }
#endif

Q_DECLARE_TYPEINFO(QByteArray, Q_MOVABLE_TYPE);
Q_DECLARE_SHARED(QByteArray);

#endif
