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

#ifndef QOBJECT_H
#define QOBJECT_H

#ifndef QT_NO_QOBJECT

#include "QtCore/qobjectdefs.h"
#include "QtCore/qnamespace.h"
#include "QtCore/qstring.h"
#include "QtCore/qbytearray.h"
#include "QtCore/qlist.h"

#ifdef QT_INCLUDE_COMPAT
#include "QtCore/qcoreevent.h"
#endif

class QEvent;
class QTimerEvent;
class QChildEvent;
struct QMetaObject;
class QVariant;
class QObjectPrivate;
class QObject;
class QThread;
class QWidget;
#ifndef QT_NO_REGEXP
class QRegExp;
#endif
#ifndef QT_NO_USERDATA
class QObjectUserData;
#endif

typedef QList<QObject*> QObjectList;

#ifndef QT_NO_MEMBER_TEMPLATES
template<typename T> inline T qFindChild(const QObject *, const QString & = QString());
template<typename T> inline QList<T> qFindChildren(const QObject *, const QString & = QString());
# ifndef QT_NO_REGEXP
template<typename T> inline QList<T> qFindChildren(const QObject *, const QRegExp &);
# endif
#endif

class QObjectData {
public:
    virtual ~QObjectData() = 0;
    QObject *q_ptr;
    QObject *parent;
    QObjectList children;

    uint isWidget : 1;
    uint pendTimer : 1;
    uint blockSig : 1;
    uint wasDeleted : 1;
    uint ownObjectName : 1;
    uint sendChildEvents : 1;
    uint unused : 26;
    int postedEvents;
#ifdef QT3_SUPPORT
    int postedChildInsertedEvents;
#else
    int reserved;
#endif
};


class Q_CORE_EXPORT QObject
{
    Q_OBJECT
    Q_PROPERTY(QString objectName READ objectName WRITE setObjectName)
    Q_DECLARE_PRIVATE(QObject)

public:
    explicit QObject(QObject *parent=0);
    virtual ~QObject();

    virtual bool event(QEvent *);
    virtual bool eventFilter(QObject *, QEvent *);

#ifdef qdoc
    static QString tr(const char *, const char *);
    static QString trUtf8(const char *, const char *);
    virtual const QMetaObject *metaObject() const;
#endif
#ifdef QT_NO_TRANSLATION
    static QString tr(const char *sourceText, const char * = 0)
        { return QString::fromLatin1(sourceText); }
#ifndef QT_NO_TEXTCODEC
    static QString trUtf8(const char *sourceText, const char * = 0)
        { return QString::fromUtf8(sourceText); }
#endif
#endif //QT_NO_TRANSLATION

    QString objectName() const;
    void setObjectName(const QString &name);

    inline bool isWidgetType() const { return d_ptr->isWidget; }

    inline bool signalsBlocked() const { return d_ptr->blockSig; }
    bool blockSignals(bool b);

    QThread *thread() const;

    int startTimer(int interval);
    void killTimer(int id);

#ifndef QT_NO_MEMBER_TEMPLATES
    template<typename T>
    inline T findChild(const QString &name) const
    { return qFindChild<T>(this, name); }

    template<typename T>
    inline QList<T> findChildren(const QString &name) const
    { return qFindChildren<T>(this, name); }

#ifndef QT_NO_REGEXP
    template<typename T>
    inline QList<T> findChildren(const QRegExp &re) const
    { return qFindChildren<T>(this, re); }
#endif
#endif

#ifdef QT3_SUPPORT
    QT3_SUPPORT QObject *child(const char *objName, const char *inheritsClass = 0,
                   bool recursiveSearch = true) const;
    QT3_SUPPORT QObjectList queryList(const char *inheritsClass = 0,
                          const char *objName = 0,
                          bool regexpMatch = true,
                          bool recursiveSearch = true) const;
#endif
    inline const QObjectList &children() const { return d_ptr->children; }

    void setParent(QObject *);
    void installEventFilter(QObject *);
    void removeEventFilter(QObject *);


    static bool connect(const QObject *sender, const char *signal,
                        const QObject *receiver, const char *member,
                        Qt::ConnectionType = Qt::AutoConnection);
    inline bool connect(const QObject *sender, const char *signal,
                        const char *member, Qt::ConnectionType type = Qt::AutoConnection) const
        { return connect(sender, signal, this, member, type); }

    static bool disconnect(const QObject *sender, const char *signal,
                           const QObject *receiver, const char *member);
    inline bool disconnect(const char *signal = 0,
                           const QObject *receiver = 0, const char *member = 0)
        { return disconnect(this, signal, receiver, member); }
    inline bool disconnect(const QObject *receiver, const char *member = 0)
        { return disconnect(this, 0, receiver, member); }

    void dumpObjectTree();
    void dumpObjectInfo();

#ifndef QT_NO_PROPERTIES
    bool setProperty(const char *name, const QVariant &value);
    QVariant property(const char *name) const;
#endif // QT_NO_PROPERTIES

#ifndef QT_NO_USERDATA
    static uint registerUserData();
    void setUserData(uint id, QObjectUserData* data);
    QObjectUserData* userData(uint id) const;
#endif // QT_NO_USERDATA

    bool isAncestorOf(const QObject *child) const;

signals:
    void destroyed(QObject * = 0);

public:
    inline QObject *parent() const { return d_ptr->parent; }

    inline bool inherits(const char *classname) const
        { return const_cast<QObject *>(this)->qt_metacast(classname) != 0; }

public slots:
    void deleteLater();

protected:
    QObject *sender() const;
    int receivers(const char* signal) const;

    virtual void timerEvent(QTimerEvent *);
    virtual void childEvent(QChildEvent *);
    virtual void customEvent(QEvent *);

    virtual void connectNotify(const char *signal);
    virtual void disconnectNotify(const char *signal);

#ifdef QT3_SUPPORT
public:
    QT3_SUPPORT_CONSTRUCTOR QObject(QObject *parent, const char *name);
    inline QT3_SUPPORT void insertChild(QObject *o)
        { if (o) o->setParent(this); }
    inline QT3_SUPPORT void removeChild(QObject *o)
        { if (o) o->setParent(0); }
    inline QT3_SUPPORT bool isA(const char *classname) const
        { return qstrcmp(classname, metaObject()->className()) == 0; }
    inline QT3_SUPPORT const char *className() const { return metaObject()->className(); }
    inline QT3_SUPPORT const char *name() const { return objectName().latin1_helper(); }
    inline QT3_SUPPORT const char *name(const char *defaultName) const
        { QString s = objectName(); return s.isEmpty()?defaultName:s.latin1_helper(); }
    inline QT3_SUPPORT void setName(const char *name) { setObjectName(QLatin1String(name)); }
protected:
    inline QT3_SUPPORT bool checkConnectArgs(const char *signal,
                                  const QObject *,
                                  const char *member)
        { return QMetaObject::checkConnectArgs(signal, member); }
    static inline QT3_SUPPORT QByteArray normalizeSignalSlot(const char *signalSlot)
        { return QMetaObject::normalizedSignature(signalSlot); }
#endif

protected:
    QObject(QObjectPrivate &dd, QObject *parent = 0);

protected:
    QObjectData *d_ptr;

    static const QMetaObject staticQtMetaObject;

    friend struct QMetaObject;
    friend class QApplication;
    friend class QCoreApplication;
    friend class QCoreApplicationPrivate;
    friend class QWidget;

private:
    Q_DISABLE_COPY(QObject)
};

inline bool QObject::isAncestorOf(const QObject *child) const
{
    while (child) {
        if (child == this)
            return true;
        child = child->d_ptr->parent;
    }
    return false;
}


#ifndef QT_NO_USERDATA
class Q_CORE_EXPORT QObjectUserData {
public:
    virtual ~QObjectUserData();
};
#endif


Q_CORE_EXPORT void qt_qFindChildren_helper(const QObject *parent, const QString &name, const QRegExp *re,
                         const QMetaObject &mo, QList<void*> *list);
Q_CORE_EXPORT QObject *qt_qFindChild_helper(const QObject *parent, const QString &name, const QMetaObject &mo);


#if defined Q_CC_MSVC && _MSC_VER < 1300

template<typename T>
inline T qFindChild(const QObject *o, const QString &name = QString(), T = 0)
{ return static_cast<T>(qt_qFindChild_helper(o, name, ((T)0)->staticMetaObject)); }

template<typename T>
inline QList<T> qFindChildren(const QObject *o, const QString &name = QString(), T = 0)
{
    QList<T> list;
    qt_qFindChildren_helper(o, name, 0, ((T)0)->staticMetaObject,
                        reinterpret_cast<QList<void *>*>(&list));
    return list;
}

#ifndef QT_NO_REGEXP
template<typename T>
inline QList<T> qFindChildren(const QObject *o, const QRegExp &re, T = 0)
{
    QList<T> list;
    qt_qFindChildren_helper(o, 0, &re, ((T)0)->staticMetaObject,
                        reinterpret_cast<QList<void*>*>(&list));
    return list;
}

#endif

template <class T> inline T qobject_cast_helper(QObject *object, T)
{ return static_cast<T>(((T)0)->staticMetaObject.cast(object)); }

template <class T> inline T qobject_cast_helper(const QObject *object, T)
{ return static_cast<T>(const_cast<const QObject *>(((T)0)->staticMetaObject.cast(const_cast<QObject *>(object)))); }

template <class T>
inline T qobject_cast(QObject *object)
{ return qobject_cast_helper<T>(object, T(0)); }

template <class T>
inline T qobject_cast(const QObject *object)
{ return qobject_cast_helper<T>(object, T(0)); }

#define Q_DECLARE_INTERFACE(IFace, IId) \
const char * const IFace##_iid = IId; \
template <> inline IFace *qobject_cast_helper<IFace *>(QObject *object, IFace *) \
{ return (IFace *)(object ? object->qt_metacast(IFace##_iid) : 0); } \
template <> inline IFace *qobject_cast_helper<IFace *>(const QObject *object, IFace *) \
{ return (IFace *)(object ? const_cast<QObject *>(object)->qt_metacast(IFace##_iid) : 0); }

#else

#ifdef QT_NO_MEMBER_TEMPLATES
template<typename T>
inline T qFindChild(const QObject *o, const QString &name = QString())
{ return static_cast<T>(qt_qFindChild_helper(o, name, reinterpret_cast<T>(0)->staticMetaObject)); }

template<typename T>
inline QList<T> qFindChildren(const QObject *o, const QString &name = QString())
{
    QList<T> list;
    qt_qFindChildren_helper(o, name, 0, reinterpret_cast<T>(0)->staticMetaObject,
                         reinterpret_cast<QList<void *>*>(&list));
    return list;
}
#else
template<typename T>
inline T qFindChild(const QObject *o, const QString &name)
{ return static_cast<T>(qt_qFindChild_helper(o, name, reinterpret_cast<T>(0)->staticMetaObject)); }

template<typename T>
inline QList<T> qFindChildren(const QObject *o, const QString &name)
{
    QList<T> list;
    qt_qFindChildren_helper(o, name, 0, reinterpret_cast<T>(0)->staticMetaObject,
                         reinterpret_cast<QList<void *>*>(&list));
    return list;
}
#endif

#ifndef QT_NO_REGEXP
template<typename T>
inline QList<T> qFindChildren(const QObject *o, const QRegExp &re)
{
    QList<T> list;
    qt_qFindChildren_helper(o, 0, &re, reinterpret_cast<T>(0)->staticMetaObject,
                        reinterpret_cast<QList<void*>*>(&list));
    return list;
}
#endif

template <class T>
inline T qobject_cast(QObject *object)
{ return static_cast<T>(reinterpret_cast<T>(0)->staticMetaObject.cast(object)); }

template <class T>
inline T qobject_cast(const QObject *object)
{ return static_cast<T>(const_cast<const QObject *>(reinterpret_cast<T>(0)->staticMetaObject.cast(const_cast<QObject *>(object)))); }


#define Q_DECLARE_INTERFACE(IFace, IId) \
const char * const IFace##_iid = IId; \
template <> inline IFace *qobject_cast<IFace *>(QObject *object) \
{ return reinterpret_cast<IFace *>((object ? object->qt_metacast(IFace##_iid) : 0)); } \
template <> inline IFace *qobject_cast<IFace *>(const QObject *object) \
{ return reinterpret_cast<IFace *>((object ? const_cast<QObject *>(object)->qt_metacast(IFace##_iid) : 0)); }

#endif

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QObject *);
#endif

#endif

#endif // QOBJECT_H
