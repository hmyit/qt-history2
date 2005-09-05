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

#ifndef QREADWRITELOCK_H
#define QREADWRITELOCK_H

#include <QtCore/qglobal.h>
#include <limits.h>

QT_MODULE(Core)

#ifndef QT_NO_THREAD

struct QReadWriteLockPrivate;

class Q_CORE_EXPORT QReadWriteLock
{
public:
    QReadWriteLock();
    ~QReadWriteLock();

    void lockForRead();
    bool tryLockForRead();

    void lockForWrite();
    bool tryLockForWrite();

    void unlock();

private:
    Q_DISABLE_COPY(QReadWriteLock)
    QReadWriteLockPrivate *d;
};

class Q_CORE_EXPORT QReadLocker
{
public:
    inline QReadLocker(QReadWriteLock *readWriteLock);

    inline ~QReadLocker()
    { unlock(); }

    inline void unlock()
    { if (q_lock) q_lock->unlock(); }

    inline void relock()
    { if (q_lock) q_lock->lockForRead(); }

    inline QReadWriteLock *readWriteLock() const
    { return q_lock; }

private:
    Q_DISABLE_COPY(QReadLocker)
    QReadWriteLock *q_lock;
};

inline QReadLocker::QReadLocker(QReadWriteLock *areadWriteLock)
    : q_lock(areadWriteLock)
{ relock(); }

class Q_CORE_EXPORT QWriteLocker
{
public:
    inline QWriteLocker(QReadWriteLock *readWriteLock);

    inline ~QWriteLocker()
    { unlock(); }

    inline void unlock()
    { if (q_lock) q_lock->unlock(); }

    inline void relock()
    { if (q_lock) q_lock->lockForWrite(); }

    inline QReadWriteLock *readWriteLock() const
    { return q_lock; }

private:
    Q_DISABLE_COPY(QWriteLocker)
    QReadWriteLock *q_lock;
};

inline QWriteLocker::QWriteLocker(QReadWriteLock *areadWriteLock)
    : q_lock(areadWriteLock)
{ relock(); }

#else // QT_NO_THREAD

class Q_CORE_EXPORT QReadWriteLock
{
public:
    inline explicit QReadWriteLock() { }
    inline ~QReadWriteLock() { }

    static inline void lockForRead() { }
    static inline bool tryLockForRead() { return true; }

    static inline void lockForWrite() { }
    static inline bool tryLockForWrite() { return true; }

    static inline void unlock() { }

private:
    Q_DISABLE_COPY(QReadWriteLock)
};

class Q_CORE_EXPORT QReadLocker
{
public:
    inline QReadLocker(QReadWriteLock *) { }
    inline ~QReadLocker() { }

    static inline void unlock() { }
    static inline void relock() { }
    static inline QReadWriteLock *readWriteLock() { return 0; }

private:
    Q_DISABLE_COPY(QReadLocker)
};

class Q_CORE_EXPORT QWriteLocker
{
public:
    inline explicit QWriteLocker(QReadWriteLock *) { }
    inline ~QWriteLocker() { }

    static inline void unlock() { }
    static inline void relock() { }
    static inline QReadWriteLock *readWriteLock() { return 0; }

private:
    Q_DISABLE_COPY(QWriteLocker)
};

#endif // QT_NO_THREAD

#endif // QREADWRITELOCK_H
