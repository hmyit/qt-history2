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

#ifndef QT_CLEAN_NAMESPACE
#define QT_CLEAN_NAMESPACE
#endif

#include "qsound.h"

#ifndef QT_NO_SOUND

#include "qhash.h"
#include "qsocketnotifier.h"
#include "qapplication.h"


#ifdef QT_NAS_SUPPORT

#include <audio/audiolib.h>
#include <audio/soundlib.h>

static AuServer *nas=0;

static AuBool eventPred(AuServer *, AuEvent *e, AuPointer p)
{
    if (e && (e->type == AuEventTypeElementNotify)) {
        if (e->auelementnotify.flow == *((AuFlowID *)p))
            return true;
    }
    return false;
}

class QAuBucketNAS : public QAuBucket {
public:
    QAuBucketNAS(AuBucketID b, AuFlowID f = 0) : id(b), flow(f), stopped(true), numplaying(0) { }
    ~QAuBucketNAS()
    {
        if (nas) {
            AuSync(nas, false);
            AuDestroyBucket(nas, id, NULL);

            AuEvent ev;
            while (AuScanEvents(nas, AuEventsQueuedAfterFlush, true, eventPred, &flow, &ev))
                ;
        }
    }

    AuBucketID id;
    AuFlowID flow;
    bool     stopped;
    int      numplaying;
};

class QAuServerNAS : public QAuServer {
    Q_OBJECT

    QSocketNotifier* sn;

public:
    QAuServerNAS(QObject* parent);
    ~QAuServerNAS();

    void init(QSound*);
    void play(const QString& filename);
    void play(QSound*);
    void stop(QSound*);
    bool okay();
    void setDone(QSound*);

public slots:
    void dataReceived();
    void soundDestroyed(QObject *o);

private:
    QAuBucketNAS* bucket(QSound* s)
    {
        return (QAuBucketNAS*)QAuServer::bucket(s);
    }
};

QAuServerNAS::QAuServerNAS(QObject* parent) :
    QAuServer(parent,"Network Audio System")
{
    nas = AuOpenServer(NULL, 0, NULL, 0, NULL, NULL);
    if (nas) {
        AuSetCloseDownMode(nas, AuCloseDownDestroy, NULL);
        // Ask Qt for async messages...
        sn=new QSocketNotifier(AuServerConnectionNumber(nas),
                QSocketNotifier::Read);
        QObject::connect(sn, SIGNAL(activated(int)),
                this, SLOT(dataReceived()));
    } else {
        sn = 0;
    }
}

QAuServerNAS::~QAuServerNAS()
{
    if (nas)
        AuCloseServer(nas);
    delete sn;
    nas = 0;
}

typedef QHash<void*,QAuServerNAS*> AuServerHash;
static AuServerHash *inprogress=0;

void QAuServerNAS::soundDestroyed(QObject *o)
{
    if (inprogress) {
        QSound *so = static_cast<QSound *>(o);
        while (inprogress->remove(so))
            ; // Loop while remove returns true
    }
}

void QAuServerNAS::play(const QString& filename)
{
    if (nas) {
        int iv=100;
        AuFixedPoint volume=AuFixedPointFromFraction(iv,100);
        AuSoundPlayFromFile(nas, filename.local8Bit(), AuNone, volume,
                            NULL, NULL, NULL, NULL, NULL, NULL);
        AuFlush(nas);
        dataReceived();
        AuFlush(nas);
        qApp->flushX();
    }
}

static void callback(AuServer*, AuEventHandlerRec*, AuEvent* e, AuPointer p)
{
    if (inprogress->find(p) && e) {
        if (e->type==AuEventTypeElementNotify &&
                    e->auelementnotify.kind==AuElementNotifyKindState) {
            if (e->auelementnotify.cur_state == AuStateStop) {
                AuServerHash::Iterator it = inprogress->find(p);
                if (it != inprogress->end())
                    (*it)->setDone((QSound*)p);
            }
        }
    }
}

void QAuServerNAS::setDone(QSound* s)
{
    if (nas) {
        decLoop(s);
        if (s->loopsRemaining() && !bucket(s)->stopped) {
            bucket(s)->stopped = true;
            play(s);
        } else {
            if (--(bucket(s)->numplaying) == 0)
                bucket(s)->stopped = true;
            inprogress->remove(s);
        }
    }
}

void QAuServerNAS::play(QSound* s)
{
    if (nas) {
        ++(bucket(s)->numplaying);
        if (!bucket(s)->stopped) {
            stop(s);
        }

        bucket(s)->stopped = false;
        if (!inprogress)
            inprogress = new AuServerHash;
        inprogress->insert(s,this);
        int iv=100;
        AuFixedPoint volume=AuFixedPointFromFraction(iv,100);
        QAuBucketNAS *b = bucket(s);
        AuSoundPlayFromBucket(nas, b->id, AuNone, volume,
                              callback, s, 0, &b->flow, NULL, NULL, NULL);
        AuFlush(nas);
        dataReceived();
        AuFlush(nas);
        qApp->flushX();
    }
}

void QAuServerNAS::stop(QSound* s)
{
    if (nas && !bucket(s)->stopped) {
        bucket(s)->stopped = true;
        AuStopFlow(nas, bucket(s)->flow, NULL);
        AuFlush(nas);
        dataReceived();
        AuFlush(nas);
        qApp->flushX();
    }
}

void QAuServerNAS::init(QSound* s)
{
    connect(s, SIGNAL(destroyed(QObject*)),
            this, SLOT(soundDestroyed(QObject*)));

    if (nas) {
        AuBucketID b_id =
            AuSoundCreateBucketFromFile(nas, s->fileName().local8Bit(),
                                        0 /*AuAccessAllMasks*/, NULL, NULL);
        setBucket(s, new QAuBucketNAS(b_id));
    }
}

bool QAuServerNAS::okay()
{
    return !!nas;
}

void QAuServerNAS::dataReceived()
{
    AuHandleEvents(nas);
}

#include "qsound_x11.moc"

#endif


class QAuServerNull : public QAuServer
{
public:
    QAuServerNull(QObject* parent);

    void play(const QString&) { }
    void play(QSound*s) { while(decLoop(s) > 0) /* nothing */ ; }
    void stop(QSound*) { }
    bool okay() { return false; }
};

QAuServerNull::QAuServerNull(QObject* parent)
    : QAuServer(parent)
{
}


QAuServer* qt_new_audio_server()
{
#ifdef QT_NAS_SUPPORT
    QAuServer* s = new QAuServerNAS(qApp);
    if (s->okay())
        return s;
    else
        delete s;
#endif
    return new QAuServerNull(qApp);
}

#endif // QT_NO_SOUND
