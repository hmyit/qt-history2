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

#ifndef QMAP_H
#define QMAP_H

#include <QtCore/qatomic.h>
#include <QtCore/qiterator.h>
#include <QtCore/qlist.h>

#ifndef QT_NO_STL
#include <map>
#endif

#include <new>
#undef QT_MAP_DEBUG

QT_BEGIN_HEADER

QT_MODULE(Core)

struct Q_CORE_EXPORT QMapData
{
    struct Node {
        Node *backward;
        Node *forward[1];
    };
    enum { LastLevel = 11, Sparseness = 3 };

    Node *backward;
    Node *forward[QMapData::LastLevel + 1];
    QBasicAtomic ref;
    int topLevel;
    int size;
    uint randomBits;
    uint insertInOrder : 1;
    uint sharable : 1;

    static QMapData *createData();
    void continueFreeData(int offset);
    Node *node_create(Node *update[], int offset);
    void node_delete(Node *update[], int offset, Node *node);
#ifdef QT_QMAP_DEBUG
    uint adjust_ptr(Node *node);
    void dump();
#endif

    static QMapData shared_null;
};


/*
    QMap uses qMapLessThanKey() to compare keys. The default
    implementation uses operator<(). For pointer types,
    qMapLessThanKey() casts the pointers to integers before it
    compares them, because operator<() is undefined on pointers
    that come from different memory blocks. (In practice, this
    is only a problem when running a program such as
    BoundsChecker.)
*/

template <class Key> inline bool qMapLessThanKey(const Key &key1, const Key &key2)
{
    return key1 < key2;
}

#ifndef QT_NO_PARTIAL_TEMPLATE_SPECIALIZATION
template <class Ptr> inline bool qMapLessThanKey(Ptr *key1, Ptr *key2)
{
    Q_ASSERT(sizeof(ulong) == sizeof(Ptr *));
    return reinterpret_cast<ulong>(key1) < reinterpret_cast<ulong>(key2);
}

template <class Ptr> inline bool qMapLessThanKey(const Ptr *key1, const Ptr *key2)
{
    Q_ASSERT(sizeof(ulong) == sizeof(const Ptr *));
    return reinterpret_cast<ulong>(key1) < reinterpret_cast<ulong>(key2);
}
#endif // QT_NO_PARTIAL_TEMPLATE_SPECIALIZATION

#if !defined(QT_NO_DATASTREAM)
class QDataStream;
template <class Key, class T> class QMap;
template <class aKey, class aT>
QDataStream &operator>>(QDataStream &in, QMap<aKey, aT> &map);
#endif

template <class Key, class T>
class QMap
{
    struct Node {
        Key key;
        T value;
        QMapData::Node *backward;
        QMapData::Node *forward[1];
    };
    union {
        QMapData *d;
        QMapData::Node *e;
    };

    struct PayloadNode
    {
        Key key;
        T value;
        QMapData::Node *backward;
    };
    enum { Payload = sizeof(PayloadNode) - sizeof(QMapData::Node *) };

    static inline Node *concrete(QMapData::Node *node) {
        return reinterpret_cast<Node *>(reinterpret_cast<char *>(node) - Payload);
    }
public:
    inline QMap() : d(&QMapData::shared_null) { d->ref.ref(); }
    inline QMap(const QMap<Key, T> &other) : d(other.d)
    { d->ref.ref(); if (!d->sharable) detach(); }
    inline ~QMap() { if (!d) return; if (!d->ref.deref()) freeData(d); }

    QMap<Key, T> &operator=(const QMap<Key, T> &other);
#ifndef QT_NO_STL
    explicit QMap(const typename std::map<Key, T> &other);
    std::map<Key, T> toStdMap() const;
#endif

    bool operator==(const QMap<Key, T> &other) const;
    inline bool operator!=(const QMap<Key, T> &other) const { return !(*this == other); }

    inline int size() const { return d->size; }

    inline bool isEmpty() const { return d->size == 0; }

    inline void detach() { if (d->ref != 1) detach_helper(); }
    inline bool isDetached() const { return d->ref == 1; }
    inline void setSharable(bool sharable) { if (!sharable) detach(); d->sharable = sharable; }

    void clear();

    int remove(const Key &key);
    T take(const Key &key);

    bool contains(const Key &key) const;
    const Key key(const T &value) const;
    const T value(const Key &key) const;
    const T value(const Key &key, const T &defaultValue) const;
    T &operator[](const Key &key);
    const T operator[](const Key &key) const;

    QList<Key> uniqueKeys() const;
    QList<Key> keys() const;
    QList<Key> keys(const T &value) const;
    QList<T> values() const;
    QList<T> values(const Key &key) const;
    int count(const Key &key) const;

    class const_iterator;

    class iterator
    {
        QMapData::Node *i;
    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef ptrdiff_t difference_type;
        typedef T value_type;
        typedef T *pointer;
        typedef T &reference;

        inline operator QMapData::Node *() const { return i; }
        inline iterator() : i(0) { }
        inline iterator(QMapData::Node *node) : i(node) { }

        inline const Key &key() const { return concrete(i)->key; }
        inline T &value() const { return concrete(i)->value; }
#ifdef QT3_SUPPORT
        inline QT3_SUPPORT T &data() const { return concrete(i)->value; }
#endif
        inline T &operator*() const { return concrete(i)->value; }
        inline T *operator->() const { return &concrete(i)->value; }
        inline bool operator==(const iterator &o) const { return i == o.i; }
        inline bool operator!=(const iterator &o) const { return i != o.i; }
        inline bool operator==(const const_iterator &o) const
            { return i == reinterpret_cast<const iterator &>(o).i; }
        inline bool operator!=(const const_iterator &o) const
            { return i != reinterpret_cast<const iterator &>(o).i; }

        inline iterator &operator++() {
            i = i->forward[0];
            return *this;
        }
        inline iterator operator++(int) {
            iterator r = *this;
            i = i->forward[0];
            return r;
        }
        inline iterator &operator--() {
            i = i->backward;
            return *this;
        }
        inline iterator operator--(int) {
            iterator r = *this;
            i = i->backward;
            return r;
        }
        inline iterator operator+(int j) const
        { iterator r = *this; if (j > 0) while (j--) ++r; else while (j++) --r; return r; }
        inline iterator operator-(int j) const { return operator+(-j); }
        inline iterator &operator+=(int j) { return *this = *this + j; }
        inline iterator &operator-=(int j) { return *this = *this - j; }
    };
    friend class iterator;

    class const_iterator
    {
        QMapData::Node *i;
    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef ptrdiff_t difference_type;
        typedef T value_type;
        typedef const T *pointer;
        typedef const T &reference;

        inline operator QMapData::Node *() const { return i; }
        inline const_iterator() : i(0) { }
        inline const_iterator(QMapData::Node *node) : i(node) { }
        inline const_iterator(const iterator &o)
        { i = reinterpret_cast<const const_iterator &>(o).i; }

        inline const Key &key() const { return concrete(i)->key; }
        inline const T &value() const { return concrete(i)->value; }
#ifdef QT3_SUPPORT
        inline QT3_SUPPORT const T &data() const { return concrete(i)->value; }
#endif
        inline const T &operator*() const { return concrete(i)->value; }
        inline const T *operator->() const { return &concrete(i)->value; }
        inline bool operator==(const const_iterator &o) const { return i == o.i; }
        inline bool operator!=(const const_iterator &o) const { return i != o.i; }

        inline const_iterator &operator++() {
            i = i->forward[0];
            return *this;
        }
        inline const_iterator operator++(int) {
            const_iterator r = *this;
            i = i->forward[0];
            return r;
        }
        inline const_iterator &operator--() {
            i = i->backward;
            return *this;
        }
        inline const_iterator operator--(int) {
            const_iterator r = *this;
            i = i->backward;
            return r;
        }
        inline const_iterator operator+(int j) const
        { const_iterator r = *this; if (j > 0) while (j--) ++r; else while (j++) --r; return r; }
        inline const_iterator operator-(int j) const { return operator+(-j); }
        inline const_iterator &operator+=(int j) { return *this = *this + j; }
        inline const_iterator &operator-=(int j) { return *this = *this - j; }
    };
    friend class const_iterator;

    // STL style
    inline iterator begin() { detach(); return iterator(e->forward[0]); }
    inline const_iterator begin() const { return const_iterator(e->forward[0]); }
    inline const_iterator constBegin() const { return const_iterator(e->forward[0]); }
    inline iterator end() {
        detach();
        return iterator(e);
    }
    inline const_iterator end() const { return const_iterator(e); }
    inline const_iterator constEnd() const { return const_iterator(e); }
    iterator erase(iterator it);
#ifdef QT3_SUPPORT
    inline QT3_SUPPORT iterator remove(iterator it) { return erase(it); }
    inline QT3_SUPPORT void erase(const Key &key) { remove(key); }
#endif

    // more Qt
    typedef iterator Iterator;
    typedef const_iterator ConstIterator;
    inline int count() const { return d->size; }
    iterator find(const Key &key);
    const_iterator find(const Key &key) const;
    const_iterator constFind(const Key &key) const;
    iterator lowerBound(const Key &key);
    const_iterator lowerBound(const Key &key) const;
    iterator upperBound(const Key &key);
    const_iterator upperBound(const Key &key) const;
    iterator insert(const Key &key, const T &value);
#ifdef QT3_SUPPORT
    QT3_SUPPORT iterator insert(const Key &key, const T &value, bool overwrite);
#endif
    iterator insertMulti(const Key &key, const T &value);
#ifdef QT3_SUPPORT
    inline QT3_SUPPORT iterator replace(const Key &key, const T &value) { return insert(key, value); }
#endif
    QMap<Key, T> &unite(const QMap<Key, T> &other);

    // STL compatibility
    typedef Key key_type;
    typedef T mapped_type;
    typedef ptrdiff_t difference_type;
    typedef int size_type;
    inline bool empty() const { return isEmpty(); }

#ifdef QT_QMAP_DEBUG
    inline void dump() const { d->dump(); }
#endif

private:
    void detach_helper();
    void freeData(QMapData *d);
    QMapData::Node *findNode(const Key &key) const;
    QMapData::Node *mutableFindNode(QMapData::Node *update[], const Key &key) const;
    QMapData::Node *node_create(QMapData *d, QMapData::Node *update[], const Key &key,
                                const T &value);

#if !defined(QT_NO_DATASTREAM)
#if !defined(Q_CC_BOR)
#if defined Q_CC_MSVC && _MSC_VER < 1300
    friend QDataStream &operator>> (QDataStream &in, QMap &map);
#else
    template <class aKey, class aT>
    friend QDataStream &operator>> (QDataStream &in, QMap<aKey, aT> &map);
#endif
#endif
#endif
};

template <class Key, class T>
Q_INLINE_TEMPLATE QMap<Key, T> &QMap<Key, T>::operator=(const QMap<Key, T> &other)
{
    if (d != other.d) {
        QMapData *x = other.d;
        x->ref.ref();
        x = qAtomicSetPtr(&d, x);
        if (!x->ref.deref())
            freeData(x);
        if (!d->sharable)
            detach_helper();
    }
    return *this;
}

template <class Key, class T>
Q_INLINE_TEMPLATE void QMap<Key, T>::clear()
{
    *this = QMap<Key, T>();
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMapData::Node *
QMap<Key, T>::node_create(QMapData *adt, QMapData::Node *aupdate[], const Key &akey, const T &avalue)
{
    QMapData::Node *abstractNode = adt->node_create(aupdate, Payload);
    Node *concreteNode = concrete(abstractNode);
    new (&concreteNode->key) Key(akey);
    new (&concreteNode->value) T(avalue);
    return abstractNode;
}

template <class Key, class T>
Q_INLINE_TEMPLATE QMapData::Node *QMap<Key, T>::findNode(const Key &akey) const
{
    QMapData::Node *cur = e;
    QMapData::Node *next = e;

    for (int i = d->topLevel; i >= 0; i--) {
        while ((next = cur->forward[i]) != e && qMapLessThanKey<Key>(concrete(next)->key, akey))
            cur = next;
    }

    if (next != e && !qMapLessThanKey<Key>(akey, concrete(next)->key)) {
        return next;
    } else {
        return e;
    }
}

template <class Key, class T>
Q_INLINE_TEMPLATE const T QMap<Key, T>::value(const Key &akey) const
{
    QMapData::Node *node = findNode(akey);
    if (node == e) {
        return T();
    } else {
        return concrete(node)->value;
    }
}

template <class Key, class T>
Q_INLINE_TEMPLATE const T QMap<Key, T>::value(const Key &akey, const T &adefaultValue) const
{
    QMapData::Node *node = findNode(akey);
    if (node == e) {
        return adefaultValue;
    } else {
        return concrete(node)->value;
    }
}

template <class Key, class T>
Q_INLINE_TEMPLATE const T QMap<Key, T>::operator[](const Key &akey) const
{
    return value(akey);
}

template <class Key, class T>
Q_INLINE_TEMPLATE T &QMap<Key, T>::operator[](const Key &akey)
{
    detach();

    QMapData::Node *update[QMapData::LastLevel + 1];
    QMapData::Node *node = mutableFindNode(update, akey);
    if (node == e)
        node = node_create(d, update, akey, T());
    return concrete(node)->value;
}

template <class Key, class T>
Q_INLINE_TEMPLATE int QMap<Key, T>::count(const Key &akey) const
{
    int cnt = 0;
    QMapData::Node *node = findNode(akey);
    if (node != e) {
        do {
            ++cnt;
            node = node->forward[0];
        } while (node != e && !qMapLessThanKey<Key>(akey, concrete(node)->key));
    }
    return cnt;
}

template <class Key, class T>
Q_INLINE_TEMPLATE bool QMap<Key, T>::contains(const Key &akey) const
{
    return findNode(akey) != e;
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::iterator QMap<Key, T>::insert(const Key &akey,
                                                                       const T &avalue)
{
    detach();

    QMapData::Node *update[QMapData::LastLevel + 1];
    QMapData::Node *node = mutableFindNode(update, akey);
    if (node == e) {
        node = node_create(d, update, akey, avalue);
    } else {
        concrete(node)->value = avalue;
    }
    return iterator(node);
}

#ifdef QT3_SUPPORT
template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::iterator QMap<Key, T>::insert(const Key &akey,
                                                                       const T &avalue,
                                                                       bool aoverwrite)
{
    detach();

    QMapData::Node *update[QMapData::LastLevel + 1];
    QMapData::Node *node = mutableFindNode(update, akey);
    if (node == e) {
        node = node_create(d, update, akey, avalue);
    } else {
        if (aoverwrite)
            concrete(node)->value = avalue;
    }
    return iterator(node);
}
#endif

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::iterator QMap<Key, T>::insertMulti(const Key &akey,
                                                                            const T &avalue)
{
    detach();

    QMapData::Node *update[QMapData::LastLevel + 1];
    mutableFindNode(update, akey);
    return iterator(node_create(d, update, akey, avalue));
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::const_iterator QMap<Key, T>::find(const Key &akey) const
{
    return const_iterator(findNode(akey));
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::const_iterator QMap<Key, T>::constFind(const Key &akey) const
{
    return const_iterator(findNode(akey));
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::iterator QMap<Key, T>::find(const Key &akey)
{
    detach();
    return iterator(findNode(akey));
}

template <class Key, class T>
Q_INLINE_TEMPLATE QMap<Key, T> &QMap<Key, T>::unite(const QMap<Key, T> &other)
{
    QMap<Key, T> copy(other);
    const_iterator it = copy.constEnd();
    const const_iterator begin = copy.constBegin();
    while (it != begin) {
        --it;
        insertMulti(it.key(), it.value());
    }
    return *this;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE void QMap<Key, T>::freeData(QMapData *x)
{
    if (QTypeInfo<Key>::isComplex || QTypeInfo<T>::isComplex) {
        QMapData::Node *y = reinterpret_cast<QMapData::Node *>(x);
        QMapData::Node *cur = y;
        QMapData::Node *next = cur->forward[0];
        while (next != y) {
            cur = next;
            next = cur->forward[0];
            Node *concreteNode = concrete(cur);
            concreteNode->key.~Key();
            concreteNode->value.~T();
        }
    }
    x->continueFreeData(Payload);
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE int QMap<Key, T>::remove(const Key &akey)
{
    detach();

    QMapData::Node *update[QMapData::LastLevel + 1];
    QMapData::Node *cur = e;
    QMapData::Node *next = e;
    int oldSize = d->size;

    for (int i = d->topLevel; i >= 0; i--) {
        while ((next = cur->forward[i]) != e && qMapLessThanKey<Key>(concrete(next)->key, akey))
            cur = next;
        update[i] = cur;
    }

    if (next != e && !qMapLessThanKey<Key>(akey, concrete(next)->key)) {
        bool deleteNext = true;
        do {
            cur = next;
            next = cur->forward[0];
            deleteNext = (next != e && !qMapLessThanKey<Key>(concrete(cur)->key, concrete(next)->key));
            concrete(cur)->key.~Key();
            concrete(cur)->value.~T();
            d->node_delete(update, Payload, cur);
        } while (deleteNext);
    }
    return oldSize - d->size;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE T QMap<Key, T>::take(const Key &akey)
{
    detach();

    QMapData::Node *update[QMapData::LastLevel + 1];
    QMapData::Node *cur = e;
    QMapData::Node *next = e;

    for (int i = d->topLevel; i >= 0; i--) {
        while ((next = cur->forward[i]) != e && qMapLessThanKey<Key>(concrete(next)->key, akey))
            cur = next;
        update[i] = cur;
    }

    if (next != e && !qMapLessThanKey<Key>(akey, concrete(next)->key)) {
        T t = concrete(next)->value;
        concrete(next)->key.~Key();
        concrete(next)->value.~T();
        d->node_delete(update, Payload, next);
        return t;
    }
    return T();
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE typename QMap<Key, T>::iterator QMap<Key, T>::erase(iterator it)
{
    QMapData::Node *update[QMapData::LastLevel + 1];
    QMapData::Node *cur = e;
    QMapData::Node *next = e;

    if (it == iterator(e))
        return it;

    for (int i = d->topLevel; i >= 0; i--) {
        while ((next = cur->forward[i]) != e && qMapLessThanKey<Key>(concrete(next)->key, it.key()))
            cur = next;
        update[i] = cur;
    }

    while (next != e) {
        cur = next;
        next = cur->forward[0];
        if (cur == it) {
            concrete(cur)->key.~Key();
            concrete(cur)->value.~T();
            d->node_delete(update, Payload, cur);
            return iterator(next);
        }

        for (int i = 0; i <= d->topLevel; ++i) {
            if (update[i]->forward[i] != cur)
                break;
            update[i] = cur;
        }
    }
    return end();
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE void QMap<Key, T>::detach_helper()
{
    union { QMapData *d; QMapData::Node *e; } x;
    x.d = QMapData::createData();
    if (d->size) {
        x.d->insertInOrder = true;
        QMapData::Node *update[QMapData::LastLevel + 1];
        QMapData::Node *cur = e->forward[0];
        update[0] = x.e;
        while (cur != e) {
            Node *concreteNode = concrete(cur);
            node_create(x.d, update, concreteNode->key, concreteNode->value);
            cur = cur->forward[0];
        }
        x.d->insertInOrder = false;
    }
    x.d = qAtomicSetPtr(&d, x.d);
    if (!x.d->ref.deref())
        freeData(x.d);
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QMapData::Node *QMap<Key, T>::mutableFindNode(QMapData::Node *aupdate[],
                                                                   const Key &akey) const
{
    QMapData::Node *cur = e;
    QMapData::Node *next = e;

    for (int i = d->topLevel; i >= 0; i--) {
        while ((next = cur->forward[i]) != e && qMapLessThanKey<Key>(concrete(next)->key, akey))
            cur = next;
        aupdate[i] = cur;
    }
    if (next != e && !qMapLessThanKey<Key>(akey, concrete(next)->key)) {
        return next;
    } else {
        return e;
    }
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QList<Key> QMap<Key, T>::uniqueKeys() const
{
    QList<Key> res;
    const_iterator i = begin();
    if (i != end()) {
        for (;;) {
            const Key &key = i.key();
            res.append(key);
            do {
                if (++i == end())
                    goto break_out_of_outer_loop;
            } while (!(key < i.key()));   // loop while (key == i.key())
        }
    }
break_out_of_outer_loop:
    return res;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QList<Key> QMap<Key, T>::keys() const
{
    QList<Key> res;
    const_iterator i = begin();
    while (i != end()) {
        res.append(i.key());
        ++i;
    }
    return res;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QList<Key> QMap<Key, T>::keys(const T &avalue) const
{
    QList<Key> res;
    const_iterator i = begin();
    while (i != end()) {
        if (i.value() == avalue)
            res.append(i.key());
        ++i;
    }
    return res;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE const Key QMap<Key, T>::key(const T &avalue) const
{
    const_iterator i = begin();
    while (i != end()) {
        if (i.value() == avalue)
            return i.key();
        ++i;
    }

    return Key();
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QList<T> QMap<Key, T>::values() const
{
    QList<T> res;
    const_iterator i = begin();
    while (i != end()) {
        res.append(i.value());
        ++i;
    }
    return res;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QList<T> QMap<Key, T>::values(const Key &akey) const
{
    QList<T> res;
    QMapData::Node *node = findNode(akey);
    if (node != e) {
        do {
            res.append(concrete(node)->value);
            node = node->forward[0];
        } while (node != e && !qMapLessThanKey<Key>(akey, concrete(node)->key));
    }
    return res;
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::const_iterator
QMap<Key, T>::lowerBound(const Key &akey) const
{
    QMapData::Node *update[QMapData::LastLevel + 1];
    mutableFindNode(update, akey);
    return const_iterator(update[0]->forward[0]);
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::iterator QMap<Key, T>::lowerBound(const Key &akey)
{
    detach();
    return static_cast<QMapData::Node *>(const_cast<const QMap *>(this)->lowerBound(akey));
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::const_iterator
QMap<Key, T>::upperBound(const Key &akey) const
{
    QMapData::Node *update[QMapData::LastLevel + 1];
    mutableFindNode(update, akey);
    QMapData::Node *node = update[0]->forward[0];
    while (node != e && !qMapLessThanKey<Key>(akey, concrete(node)->key))
        node = node->forward[0];
    return const_iterator(node);
}

template <class Key, class T>
Q_INLINE_TEMPLATE typename QMap<Key, T>::iterator QMap<Key, T>::upperBound(const Key &akey)
{
    detach();
    return static_cast<QMapData::Node *>(const_cast<const QMap *>(this)->upperBound(akey));
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE bool QMap<Key, T>::operator==(const QMap<Key, T> &other) const
{
    if (size() != other.size())
        return false;
    if (d == other.d)
        return true;

    const_iterator it1 = begin();
    const_iterator it2 = other.begin();

    while (it1 != end()) {
        if (!(it1.value() == it2.value()) || qMapLessThanKey(it1.key(), it2.key()) || qMapLessThanKey(it2.key(), it1.key()))
            return false;
        ++it2;
        ++it1;
    }
    return true;
}

#ifndef QT_NO_STL
template <class Key, class T>
Q_OUTOFLINE_TEMPLATE QMap<Key, T>::QMap(const std::map<Key, T> &other)
{
    d = QMapData::createData();
    d->insertInOrder = true;
    typename std::map<Key,T>::const_iterator it = other.end();
    while (it != other.begin()) {
        --it;
        insert((*it).first, (*it).second);
    }
    d->insertInOrder = false;
}

template <class Key, class T>
Q_OUTOFLINE_TEMPLATE  std::map<Key, T> QMap<Key, T>::toStdMap() const
{
    std::map<Key, T> map;
    const_iterator it = end();
    while (it != begin()) {
        --it;
        map.insert(std::pair<Key, T>(it.key(), it.value()));
    }
    return map;
}

#endif // QT_NO_STL

template <class Key, class T>
class QMultiMap : public QMap<Key, T>
{
public:
    QMultiMap() {}
    QMultiMap(const QMap<Key, T> &other) : QMap<Key, T>(other) {}

    inline typename QMap<Key, T>::iterator replace(const Key &key, const T &value);
    inline typename QMap<Key, T>::iterator insert(const Key &key, const T &value);

    inline QMultiMap &operator+=(const QMultiMap &other)
    { unite(other); return *this; }
    inline QMultiMap operator+(const QMultiMap &other) const
    { QMultiMap result = *this; result += other; return result; }

private:
    T &operator[](const Key &key);
    const T operator[](const Key &key) const;
};

template <class Key, class T>
Q_INLINE_TEMPLATE Q_TYPENAME QMap<Key, T>::iterator QMultiMap<Key, T>::replace(const Key &akey, const T &avalue)
{ return QMap<Key, T>::insert(akey, avalue); }

template <class Key, class T>
Q_INLINE_TEMPLATE Q_TYPENAME QMap<Key, T>::iterator QMultiMap<Key, T>::insert(const Key &akey, const T &avalue)
{ return QMap<Key, T>::insertMulti(akey, avalue); }


Q_DECLARE_ASSOCIATIVE_ITERATOR(Map)
Q_DECLARE_MUTABLE_ASSOCIATIVE_ITERATOR(Map)

QT_END_HEADER

#endif // QMAP_H
