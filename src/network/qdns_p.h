#ifndef QDNS_P_H
#define QDNS_P_H

#include "qdns.h"
#include <qmutex.h>
#include <qobject.h>
#include <qpointer.h>

#if !defined QT_NO_THREAD
#include <qthread.h>
#    define QDnsAgentBase QThread
#else
#    define QDnsAgentBase QObject
#endif

struct QDnsQuery
{
    inline QDnsQuery() : member(0) {}

    inline QDnsQuery(const QString &name, QObject *r, const char *m)
        : hostName(name), receiver(r), member(m) {}

    QString hostName;
    QPointer<QObject> receiver;
    const char *member;
};

class QDnsAgent : public QDnsAgentBase
{
    Q_OBJECT
public:
    inline QDnsAgent() {}

    void run();

    inline void addHostName(const QString &name,
                            QObject *receiver, const char *member)
    {
        QMutexLocker locker(&mutex);
        queries << QDnsQuery(name, receiver, member);
    }

signals:
    void resultsReady(QDnsHostInfo);

private:
    QList<QDnsQuery> queries;
    QMutex mutex;
};

class QDnsHostInfoPrivate
{
public:
    inline QDnsHostInfoPrivate()
        : err(QDnsHostInfo::NoError),
          errorStr(QT_TRANSLATE_NOOP("QDnsHostInfo", "Unknown error"))
    {
    }

    QDnsHostInfo::Error err;
    QString errorStr;
    QList<QHostAddress> addrs;
    QString hostName;
};

#endif // QDNS_P_H
