#include "qeventdispatcher_win.h"

#include "qcoreapplication.h"
#include "qdebug.h"
#include "qhash.h"
#include "qsocketnotifier.h"
#include "qwineventnotifier_p.h"

#include "qabstracteventdispatcher_p.h"
#include <private/qthread_p.h>

#define NO_NEW_WIN_DISPATCHING

struct QSockNot {
    QSocketNotifier *obj;
    int fd;
};
typedef QHash<int, QSockNot *> QSNDict;

struct TimerInfo {                                // internal timer info
    uint     ind;                                // - Qt timer identifier - 1
    uint     id;                                // - Windows timer identifier
    QObject *obj;                                // - object to receive events
};
typedef QList<TimerInfo*>  TimerVec;                // vector of TimerInfo structs
typedef QHash<int,TimerInfo*> TimerDict;                // fast dict of timers

class QEventDispatcherWin32Private : public QAbstractEventDispatcherPrivate
{
    Q_DECLARE_PUBLIC(QEventDispatcherWin32)
public:
    QEventDispatcherWin32Private();

    DWORD threadId;

    bool interrupt;

    // timers
    TimerVec timerVec;
    TimerDict timerDict;

    // socket notifiers
    HWND sn_win;
    QSNDict sn_read;
    QSNDict sn_write;
    QSNDict sn_except;

    QVector<HANDLE> winEventNotifierHandles;
    QList<QWinEventNotifier *> winEventNotifierList;
    int activateEventNotifiers();
    void activateEventNotifier(QWinEventNotifier * wen);
};

QEventDispatcherWin32Private::QEventDispatcherWin32Private()
    : threadId(GetCurrentThreadId()), interrupt(false), sn_win(0)
{ }

// will go throw the entire list an activate any that are set
int QEventDispatcherWin32Private::activateEventNotifiers()
{
    int n_act = 0;
    for (int i=0; i<winEventNotifierList.count(); i++) {
        if (WaitForSingleObject(winEventNotifierList.at(i)->handle(), 0) == WAIT_OBJECT_0) {
            activateEventNotifier(winEventNotifierList.at(i));
            ++n_act;
        }
    }
    return n_act;
}

void QEventDispatcherWin32Private::activateEventNotifier(QWinEventNotifier * wen)
{
    QEvent event(QEvent::SockAct);
    QCoreApplication::sendEvent(wen, &event);
}


Q_CORE_EXPORT bool winPeekMessage(MSG* msg, HWND hWnd, UINT wMsgFilterMin,
                     UINT wMsgFilterMax, UINT wRemoveMsg)
{
    QT_WA({ return PeekMessage(msg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg); } ,
          { return PeekMessageA(msg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg); });
}

Q_CORE_EXPORT bool winPostMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    QT_WA({ return PostMessage(hWnd, msg, wParam, lParam); } , 
          { return PostMessageA(hWnd, msg, wParam, lParam); });
}

Q_CORE_EXPORT bool winGetMessage(MSG* msg, HWND hWnd, UINT wMsgFilterMin,
                     UINT wMsgFilterMax)
{
    QT_WA({ return GetMessage(msg, hWnd, wMsgFilterMin, wMsgFilterMax); } ,
          { return GetMessageA(msg, hWnd, wMsgFilterMin, wMsgFilterMax); });
}

void CALLBACK qt_timer_proc(HWND hwnd, UINT message, UINT timerId, DWORD)
{
    MSG msg;
    msg.hwnd = hwnd;
    msg.message = message;
    msg.wParam = WPARAM(timerId);
    msg.lParam = LPARAM(qt_timer_proc);

    QCoreApplication *app = QCoreApplication::instance();
    long result;
    if (app && app->filterEvent(&msg, &result))
        return;

    QEventDispatcherWin32 *eventDispatcher =
        qt_cast<QEventDispatcherWin32 *>(QAbstractEventDispatcher::instance());
    Q_ASSERT(eventDispatcher != 0);
    QEventDispatcherWin32Private *d = eventDispatcher->d_func();

    TimerInfo *t = d->timerDict.value(timerId);
    if (!t)
        return;

    QTimerEvent e(t->ind);
    QCoreApplication::sendEvent(t->obj, &e);    
}

LRESULT CALLBACK qt_socketnotifier_proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp)
{
    if (message != WM_USER)
        return  DefWindowProc(hwnd, message, wp, lp);
      
    // socket notifier message
    MSG msg;
    msg.hwnd = hwnd;
    msg.message = message;
    msg.wParam = wp;
    msg.lParam = lp;

    QCoreApplication *app = QCoreApplication::instance();
    long result;
    if (app && app->filterEvent(&msg, &result))
        return result;

    int type = -1;
#ifndef Q_OS_TEMP
    switch (WSAGETSELECTEVENT(lp)) {
    case FD_READ:
    case FD_CLOSE:
    case FD_ACCEPT:
        type = 0;
        break;
    case FD_WRITE:
    case FD_CONNECT:
        type = 1;
        break;
    case FD_OOB:
        type = 2;
        break;
    }
#endif
    if (type >= 0) {

#ifdef GWLP_USERDATA
        QEventDispatcherWin32 *eventDispatcher =
            (QEventDispatcherWin32 *) GetWindowLongPtrA(hwnd, GWLP_USERDATA);
#else
        QEventDispatcherWin32 *eventDispatcher =
            (QEventDispatcherWin32 *) GetWindowLongA(hwnd, GWL_USERDATA);
#endif
        if (eventDispatcher) {
            QEventDispatcherWin32Private *d = eventDispatcher->d_func();
            QSNDict *sn_vec[3] = { &d->sn_read, &d->sn_write, &d->sn_except };
            QSNDict *dict = sn_vec[type];

            QSockNot *sn = dict ? dict->value(wp) : 0;
            if (sn) {
                QEvent event(QEvent::SockAct);
                QCoreApplication::sendEvent(sn->obj, &event);
            }
        }
    }
    return 0;
}

static HWND qt_create_sn_window(QEventDispatcherWin32 *eventDispatcher)
{
    extern HINSTANCE qWinAppInst();
    HINSTANCE hi = qWinAppInst();
    WNDCLASSA wc;
    wc.style = 0;
    wc.lpfnWndProc = qt_socketnotifier_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hi;
    wc.hIcon = 0;
    wc.hCursor = 0;
    wc.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "QtSocketNotifier_Internal_Widget";
    RegisterClassA(&wc);

    HWND wnd = CreateWindowA(wc.lpszClassName,  // classname
                             wc.lpszClassName,  // window name
                             0,                 // style
                             0, 0, 0, 0,        // geometry
                             0,                 // parent
                             0,                 // menu handle
                             hi,                // application
                             0);                // windows creation data.

#ifdef GWLP_USERDATA
    SetWindowLongPtrA(wnd, GWLP_USERDATA, (LONG_PTR)eventDispatcher);
#else
    SetWindowLongA(wnd, GWL_USERDATA, (LONG)eventDispatcher);
#endif

    if (!wnd) {
        qWarning("Failed to create socket notifier receiver window: %d\n", GetLastError());
    }
    return wnd;
}

QEventDispatcherWin32::QEventDispatcherWin32(QObject *parent)
    : QAbstractEventDispatcher(*new QEventDispatcherWin32Private, parent)
{
}

QEventDispatcherWin32::~QEventDispatcherWin32()
{
}

#ifndef NO_NEW_WIN_DISPATCHING
bool QEventDispatcherWin32::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    emit awake();

    QCoreApplication::sendPostedEvents();

    Q_D(QEventDispatcherWin32);
    bool shortcut = d->interrupt;

    QThreadData *data = QThreadData::current();
    bool canWait = (data->postEventList.size() == 0
                    && !d->interrupt
                    && (flags & QEventLoop::WaitForMoreEvents));




    MSG msg;

    if (flags & QEventLoop::ExcludeUserInputEvents) {
        // purge all userinput messages from eventDispatcher
        while (winPeekMessage(&msg, 0, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
            ;
        while (winPeekMessage(&msg, 0, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE))
            ;
        while (winPeekMessage(&msg, 0, WM_MOUSEWHEEL, WM_MOUSEWHEEL, PM_REMOVE))
            ;
        // ### tablet?

        // now that we have eaten all userinput we shouldn't wait for the next one...
        canWait = false;
    }

    // activate all full-speed timers until there is a message (timers have low priority)
    bool message = false;
    if (d->numZeroTimers) {
        while (d->numZeroTimers && !(message=winPeekMessage(&msg, 0, 0, 0, PM_NOREMOVE))) {
            d->activateZeroTimers();
        }
    }

    message = winPeekMessage(&msg, 0, 0, 0, PM_REMOVE);
    if (!message && !canWait) // still no message, and shouldn't block
        return false;

    // process all messages, unless userinput is blocked, then we process only one
    if (flags & QEventLoop::ExcludeUserInputEvents) {
        winProcessEvent(&msg);
        shortcut = d->interrupt;
    } else {
        // ### Don't send two timers in a row, since it could potentially block
        // other events from being fired.
        bool lastWasTimer = false;
        while (message && !shortcut && !lastWasTimer) {
            winProcessEvent(&msg);
            if (msg.message == WM_TIMER)
                lastWasTimer = true;
            message = winPeekMessage(&msg, 0, 0, 0, PM_REMOVE);
            shortcut = d->interrupt;
        }

        if (lastWasTimer)
            winProcessEvent(&msg);
    }

    // don't wait if there are pending socket notifiers
    canWait = d->sn_pending_list.isEmpty() && canWait;

    // don't wait if event notifiers notified any thing
    canWait = d->activateEventNotifiers() && canWait;

    shortcut = d->interrupt;

    // wait for next message if allowed to block
    if (canWait && !shortcut) {
        emit aboutToBlock();
        
        DWORD wakeMask = QS_ALLEVENTS;
        if (flags & QEventLoop::ExcludeUserInputEvents)
            wakeMask &= !(QS_KEY | QS_MOUSE | QS_HOTKEY);

        // has enough reserved space so this is cheap
        d->winEventNotifierHandles.resize(d->winEventNotifierList.count());
        for (int i=0; i<d->winEventNotifierList.count(); i++)
            d->winEventNotifierHandles[i] = d->winEventNotifierList.at(i)->handle();
        
        DWORD ret = MsgWaitForMultipleObjectsEx(d->winEventNotifierHandles.count(), (LPHANDLE)d->winEventNotifierHandles.data(),
                                                INFINITE, wakeMask, MWMO_ALERTABLE);
        if (ret - WAIT_OBJECT_0 >= 0 && ret - WAIT_OBJECT_0 < d->winEventNotifierHandles.count()) {
            d->activateEventNotifier(d->winEventNotifierList.at(ret - WAIT_OBJECT_0));
        }
    }

    if (!(flags & QEventLoop::ExcludeSocketNotifiers))
        d->activateSocketNotifiers();

    QCoreApplication::sendPostedEvents();

    if (d->interrupt) {
        d->interrupt = false;
        return false;
    }
    return true;
}

#else

bool QEventDispatcherWin32::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    Q_D(QEventDispatcherWin32);

    emit awake();

    QCoreApplication::sendPostedEvents();

    QThreadData *data = QThreadData::current();
    bool pending;
    do {
        MSG msg;
        pending = winPeekMessage(&msg, 0, 0, 0, PM_REMOVE);

        if (!pending) {
            bool canWait = (data->postEventList.size() == 0
                            && !d->interrupt
                            && (flags & QEventLoop::WaitForMoreEvents));
            if (!canWait)
                break;

            emit aboutToBlock();
            if (!winGetMessage(&msg, NULL, 0, 0)) {
                exit(0);
                return false;
            }
        }

        qDebug() << "processing message" << msg;

        if (!filterEvent(&msg)) {
            TranslateMessage(&msg);
            QT_WA({ DispatchMessage(&msg);  } ,
                  { DispatchMessageA(&msg); });
        }
    } while (!d->interrupt && pending);

    if (d->interrupt) {
        d->interrupt = false;
        return false;
    }
    return true;
}
#endif

bool QEventDispatcherWin32::hasPendingEvents()
{
    MSG msg;
    extern uint qGlobalPostedEventsCount();
    return qGlobalPostedEventsCount() || winPeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
}

void QEventDispatcherWin32::registerSocketNotifier(QSocketNotifier *notifier)
{
    if (notifier == 0 || notifier->socket() < 0 || notifier->type() < 0 || notifier->type() > 2) {
        qWarning("SocketNotifier: Internal error");
        return;
    }

    int socket = notifier->socket();
    int type = notifier->type();

    Q_D(QEventDispatcherWin32);
    QSNDict *sn_vec[3] = { &d->sn_read, &d->sn_write, &d->sn_except };
    QSNDict *dict = sn_vec[type];

    if (QCoreApplication::closingDown()) // ### d->exitloop?
        return; // after sn_cleanup, don't reinitialize.

    if (!d->sn_win)
        d->sn_win = qt_create_sn_window(this);

    if (dict->contains(socket)) {
        const char *t[] = { "read", "write", "exception" };
        qWarning("QSocketNotifier: Multiple socket notifiers for "
                    "same socket %d and type %s", socket, t[type]);
    }

    QSockNot *sn = new QSockNot;
    sn->obj = notifier;
    sn->fd  = socket;
    dict->insert(sn->fd, sn);

#ifndef Q_OS_TEMP
    int sn_event = 0;
    if (d->sn_read.contains(socket))
        sn_event |= FD_READ | FD_CLOSE | FD_ACCEPT;
    if (d->sn_write.contains(socket))
        sn_event |= FD_WRITE | FD_CONNECT;
    if (d->sn_except.contains(socket))
        sn_event |= FD_OOB;
    // BoundsChecker may emit a warning for WSAAsyncSelect when sn_event == 0
    // This is a BoundsChecker bug and not a Qt bug
    WSAAsyncSelect(socket, d->sn_win, sn_event ? WM_USER : 0, sn_event);
#else
/*
    fd_set        rd,wt,ex;
    FD_ZERO(&rd);
    FD_ZERO(&wt);
    FD_ZERO(&ex);
    if (sn_read && sn_read->find(sockfd))
        FD_SET(sockfd, &rd);
    if (sn_write && sn_write->find(sockfd))
        FD_SET(sockfd, &wt);
    if (sn_except && sn_except->find(sockfd))
        FD_SET(sockfd, &ex);
    select(1, &rd, &wt, &ex, NULL);
*/
#endif
}

void QEventDispatcherWin32::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    if (notifier == 0 || notifier->socket() < 0 || notifier->type() < 0 || notifier->type() > 2) {
        qWarning("SocketNotifier: Internal error");
        return;
    }

    int socket = notifier->socket();
    int type = notifier->type();

    Q_D(QEventDispatcherWin32);
    QSNDict *sn_vec[3] = { &d->sn_read, &d->sn_write, &d->sn_except };
    QSNDict *dict = sn_vec[type];
    QSockNot *sn = dict->value(socket);
    if (!sn)
        return;

    dict->remove(socket);
    delete sn;

#ifndef Q_OS_TEMP // ### This probably needs fixing
    int sn_event = 0;
    if (d->sn_read.contains(socket))
        sn_event |= FD_READ | FD_CLOSE | FD_ACCEPT;
    if (d->sn_write.contains(socket))
        sn_event |= FD_WRITE | FD_CONNECT;
    if (d->sn_except.contains(socket))
        sn_event |= FD_OOB;
    // BoundsChecker may emit a warning for WSAAsyncSelect when sn_event == 0
    // This is a BoundsChecker bug and not a Qt bug
    WSAAsyncSelect(socket, d->sn_win, sn_event ? WM_USER : 0, sn_event);
#else
    fd_set        rd,wt,ex;
    FD_ZERO(&rd);
    FD_ZERO(&wt);
    FD_ZERO(&ex);
    if (sn_read && sn_read->find(sockfd))
        FD_SET(sockfd, &rd);
    if (sn_write && sn_write->find(sockfd))
        FD_SET(sockfd, &wt);
    if (sn_except && sn_except->find(sockfd))
        FD_SET(sockfd, &ex);
    select(1, &rd, &wt, &ex, NULL);
#endif
}

int QEventDispatcherWin32::registerTimer(int interval, QObject *object)
{
    Q_D(QEventDispatcherWin32);

    register TimerInfo *t = new TimerInfo;    
    t->ind  = d->timerVec.isEmpty() ? 1 : d->timerVec.last()->ind + 1;
    t->obj  = object;
    t->id = SetTimer(0, 0, (uint) interval, (TIMERPROC) qt_timer_proc);

    if (t->id == 0) {
        qErrnoWarning("QEventDispatcherWin32::registerTimer: Failed to create a timer");
        delete t;                               // could not set timer
        return 0;
    }

    d->timerVec.append(t);                        // store in timer vector
    d->timerDict.insert(t->id, t);                // store in dict
    return t->ind;                              // return index in vector
}

bool QEventDispatcherWin32::unregisterTimer(int timerId)
{
    Q_D(QEventDispatcherWin32);
    if (d->timerVec.isEmpty() || timerId <= 0)
        return false;

    TimerInfo *t = 0;
    for (int i=0; i<d->timerVec.size(); ++i) {
        if (d->timerVec.at(i)->ind == timerId) {
            t = d->timerVec.at(i);
            break;
        }
    }
    if (!t)
        return false;

    KillTimer(0, t->id);
    d->timerDict.remove(t->id);
    d->timerVec.removeAll(t);
    delete t;
    return true;
}

bool QEventDispatcherWin32::unregisterTimers(QObject *object)
{
    Q_D(QEventDispatcherWin32);
    if (d->timerVec.isEmpty())
        return false;
    register TimerInfo *t;
    for (int i=0; i<d->timerVec.size(); i++) {
        t = d->timerVec.at(i);
        if (t && t->obj == object) {                // object found
            KillTimer(0, t->id);
            d->timerDict.remove(t->id);
            d->timerVec.removeAt(i);
            delete t;
        }
    }
    return true;
}

bool QEventDispatcherWin32::registerEventNotifier(QWinEventNotifier *notifier)
{
    Q_D(QEventDispatcherWin32);

    if (d->winEventNotifierList.contains(notifier))
        return true;

    if (d->winEventNotifierList.count() >= MAXIMUM_WAIT_OBJECTS - 2) {
        qWarning("QWinEventNotifier: Can not have more then %d enabled at one time", MAXIMUM_WAIT_OBJECTS - 2);
        return false;
    }
    d->winEventNotifierList.append(notifier);
    return true;
}

void QEventDispatcherWin32::unregisterEventNotifier(QWinEventNotifier *notifier)
{
    Q_D(QEventDispatcherWin32);

    int i = d->winEventNotifierList.indexOf(notifier);
    if (i != -1)
        d->winEventNotifierList.takeAt(i);
}

void QEventDispatcherWin32::wakeUp()
{
    Q_D(QEventDispatcherWin32);
    QT_WA({ PostThreadMessageW(d->threadId, WM_NULL, 0, 0); } ,
          { PostThreadMessageA(d->threadId, WM_NULL, 0, 0); });
}

void QEventDispatcherWin32::interrupt()
{
    Q_D(QEventDispatcherWin32);
    d->interrupt = true;
    wakeUp();
}

void QEventDispatcherWin32::flush()
{ }
