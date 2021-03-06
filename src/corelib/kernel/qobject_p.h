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

#ifndef QOBJECT_P_H
#define QOBJECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qapplication_*.cpp, qwidget*.cpp and qfiledialog.cpp.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qobject.h"
#include "QtCore/qpointer.h"
#include "QtCore/qcoreevent.h"
#include "QtCore/qlist.h"
#include "QtCore/qvector.h"
#include "QtCore/qreadwritelock.h"
#include "QtCore/qvariant.h"

QT_BEGIN_NAMESPACE

class QVariant;
class QThreadData;
class QObjectConnectionListVector;

/* mirrored in QtTestLib, DON'T CHANGE without prior warning */
struct QSignalSpyCallbackSet
{
    typedef void (*BeginCallback)(QObject *caller, int method_index, void **argv);
    typedef void (*EndCallback)(QObject *caller, int method_index);
    BeginCallback signal_begin_callback,
                    slot_begin_callback;
    EndCallback signal_end_callback,
                slot_end_callback;
};
void Q_CORE_EXPORT qt_register_signal_spy_callbacks(const QSignalSpyCallbackSet &callback_set);

extern QSignalSpyCallbackSet Q_CORE_EXPORT qt_signal_spy_callback_set;

inline QObjectData::~QObjectData() {}

enum { QObjectPrivateVersion = QT_VERSION };

class Q_CORE_EXPORT QObjectPrivate : public QObjectData
{
    Q_DECLARE_PUBLIC(QObject)

public:
    // use this lock when implementing thread-safe QObject things (e.g. postEvent())
    static QReadWriteLock *readWriteLock();
    // note: must lockForRead() before calling isValidObject()
    static bool isValidObject(QObject *object);

    static QReadWriteLock *signalSlotLock();

    QObjectPrivate(int version = QObjectPrivateVersion);
    virtual ~QObjectPrivate();

#ifdef QT3_SUPPORT
    QList<QObject *> pendingChildInsertedEvents;
    void sendPendingChildInsertedEvents();
    void removePendingChildInsertedEvents(QObject *child);
#else
    // preserve binary compatibility with code compiled without Qt 3 support
    QList<QObject *> unused;
#endif

    // id of the thread that owns the object
    QThreadData *threadData;
    void moveToThread_helper();
    void setThreadData_helper(QThreadData *currentData, QThreadData *targetData);
    void _q_reregisterTimers(void *pointer);

    // object currently activating the object
    union {
        QObject *currentSender;
        QObject *currentChildBeingDeleted;
    };
    int currentSenderSignal;

    bool isSender(const QObject *receiver, const char *signal) const;
    QObjectList receiverList(const char *signal) const;
    QObjectList senderList() const;

    QList<QPointer<QObject> > eventFilters;

    void setParent_helper(QObject *);

    void deleteChildren();

    static void clearGuards(QObject *);

    struct ExtraData
    {
#ifndef QT_NO_USERDATA
        QVector<QObjectUserData *> userData;
#endif
        QList<QByteArray> propertyNames;
        QList<QVariant> propertyValues;
    };
    ExtraData *extraData;
    mutable quint32 connectedSignals;

    QString objectName;

    // Note: you must hold the signalSlotLock() before accessing the lists below or calling the functions
    struct Connection
    {
        QObject *receiver;
        int method;
        uint connectionType : 3; // 0 == auto, 1 == direct, 2 == queued, 4 == blocking
        QBasicAtomicPointer<int> argumentTypes;
    };
    typedef QList<Connection> ConnectionList;

    QObjectConnectionListVector *connectionLists;
    void addConnection(int signal, Connection *c);
    void removeReceiver(int signal, QObject *receiver);

    struct Sender
    {
        QObject *sender;
        int signal;
        int ref;
    };

    QList<Sender> senders;
    void refSender(QObject *sender, int signal);
    void derefSender(QObject *sender, int signal);
    void removeSender(QObject *sender, int signal);
};

Q_DECLARE_TYPEINFO(QObjectPrivate::Connection, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(QObjectPrivate::Sender, Q_MOVABLE_TYPE);

class QSemaphore;
class Q_CORE_EXPORT QMetaCallEvent : public QEvent
{
public:
    QMetaCallEvent(int id, const QObject *sender, int signalId,
                   int nargs = 0, int *types = 0, void **args = 0, QSemaphore *semaphore = 0);
    ~QMetaCallEvent();

    inline int id() const { return id_; }
    inline const QObject *sender() const { return sender_; }
    inline int signalId() const { return signalId_; }
    inline void **args() const { return args_; }

    virtual int placeMetaCall(QObject *object);

private:
    int id_;
    const QObject *sender_;
    int signalId_;
    int nargs_;
    int *types_;
    void **args_;
    QSemaphore *semaphore_;
};

class Q_CORE_EXPORT QBoolBlocker
{
public:
    inline QBoolBlocker(bool &b):block(b), reset(b){block = true;}
    inline ~QBoolBlocker(){block = reset; }
private:
    bool &block;
    bool reset;
};

QT_END_NAMESPACE

#endif // QOBJECT_P_H
