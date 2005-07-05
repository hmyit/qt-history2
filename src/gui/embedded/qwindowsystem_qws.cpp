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

#include "qwindowsystem_qws.h"
#include "qwsevent_qws.h"
#include "qwscommand_qws.h"
#include "qwsutils_qws.h"
#include "qwscursor_qws.h"
#include "qwsdisplay_qws.h"
#include "qmouse_qws.h"
#include "qcopchannel_qws.h"

#include "qapplication.h"
#include "qsocketnotifier.h"
#include "qpolygon.h"
#include "qimage.h"
#include "qcursor.h"
#include <private/qpaintengine_raster_p.h>
#include "qscreen_qws.h"
#include "qwindowdefs.h"
#include "private/qlock_p.h"
#include "qfile.h"
#include "qtimer.h"
#include "qpen.h"
#include "qdesktopwidget.h"
#include "qevent.h"
#include "qinputcontext.h"
#include "qpainter.h"


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifndef QT_NO_QWS_MULTIPROCESS
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#ifndef Q_OS_DARWIN
# include <sys/sem.h>
#endif
#include <sys/param.h>
#include <sys/mount.h>
#endif
#include <signal.h>
#include <fcntl.h>

#if !defined(QT_NO_SOUND) && !defined(Q_OS_DARWIN)
#ifdef QT_USE_OLD_QWS_SOUND
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#else
#include "qsoundqss_qws.h"
#endif
#endif

#include "qkbddriverfactory_qws.h"
#include "qmousedriverfactory_qws.h"

#include <qbuffer.h>

#include <private/qwidget_qws_p.h>

//#include <qdebug.h>

extern void qt_qws_set_max_window_rect(const QRect& r);

QWSServer *qwsServer=0;

class QWSServerData {
public:
    QWSServerData()
    {
        screensaverintervals = 0;
        saver = 0;
        cursorClient = 0;
        mouseState = 0;
    }
    ~QWSServerData()
    {
        qDeleteAll(deletedWindows);
        delete [] screensaverintervals;
        delete saver;
    }
    QTime screensavertime;
    QTimer* screensavertimer;
    int* screensaverintervals;
    QWSScreenSaver* saver;
    QWSClient *cursorClient;
    int mouseState;
    bool prevWin;
    QList<QWSWindow*> deletedWindows;
};

QWSScreenSaver::~QWSScreenSaver()
{
}

extern char *qws_display_spec;
extern void qt_init_display(); //qapplication_qws.cpp
extern QString qws_qtePipeFilename();

extern void qt_client_enqueue(const QWSEvent *); //qapplication_qws.cpp
typedef void MoveRegionF(const QWSRegionMoveCommand*);
typedef void RequestRegionF(int, QRegion);
typedef void SetAltitudeF(const QWSChangeAltitudeCommand*);
extern QList<QWSCommand*> *qt_get_server_queue();

static QRect maxwindow_rect;
static const char *defaultMouse =
#if defined(QT_QWS_CASSIOPEIA) || defined(QT_QWS_IPAQ) || defined(QT_QWS_EBX) || defined(QT_QWS_YOPY) || defined(QWS_CUSTOMTOUCHPANEL)
    "TPanel"
#else
    "Auto"
#endif
#if defined(QT_QWS_CASSIOPEIA)
    "/dev/tpanel"
#endif
    ;
static const char *defaultKeyboard = "TTY";
static int qws_keyModifiers = 0;

static QWSWindow *keyboardGrabber;
static bool keyboardGrabbing;

static int get_object_id()
{
    static int next=1000;
    return next++;
}
#ifndef QT_NO_QWS_IM
static QWSInputMethod *current_IM = 0;

static bool current_IM_ComposeMode = false;
static QWSWindow* current_IM_win=0;
static int current_IM_winId=-1;
#endif

void QWSWindow::bltToScreen(const QRegion &globalrgn)
{
    QRasterPaintEngine *pe = qwsServer->paintEngine;

    QPixmap *buf = backingStore->pixmap();

    if (!buf || buf->isNull())
        return;

    QRegion bltRegion = requested_region & globalrgn;

    if (bltRegion.isEmpty())
        return;

    QPoint topLeft = requested_region.boundingRect().topLeft();

    pe->updateClipRegion(bltRegion, Qt::ReplaceClip);

    backingStore->lock();
    QRectF destR(topLeft.x(),topLeft.y(), buf->width(), buf->height());
    QRectF sourceR(0, 0, buf->width(), buf->height());
    pe->drawPixmap(destR, *buf, sourceR);
    backingStore->unlock();
}

void QWSServer::compose(int level, QRegion exposed, QRegion &blend, QPixmap &blendbuffer, int changing_level)
{
    bool above_changing = level < changing_level; //0 is topmost

    QWSWindow *win = windows.value(level); //null ptr means background

    QRegion exposedBelow;
    bool opaque = true;

    if (win) {
        opaque = win->isOpaque();
        if (opaque) {
            exposedBelow = exposed - win->requested_region;
            if (above_changing)
                blend -= win->requested_region;
        } else {
            blend += exposed & win->requested_region;
            exposedBelow = exposed;
        }
    }
    if (win && !exposedBelow.isEmpty()) {
        compose(level+1, exposedBelow, blend, blendbuffer, changing_level);
    } else {
        QSize blendSize = blend.boundingRect().size();
        if (!blendSize.isNull())
            blendbuffer = QPixmap(blendSize);
    }
    if (!win) {
        paintBackground(exposed-blend);
    } else if (!above_changing) {
        win->bltToScreen(exposed-blend);
    }
    QRegion blendRegion = exposed&blend;
    if (win)
        blendRegion &= win->requested_region;
    if (!blendRegion.isEmpty()) {
        QPainter p(&blendbuffer);
        QRegion clipRgn = blendRegion;
        QPoint blendOffset = blend.boundingRect().topLeft();
        clipRgn.translate(-blendOffset);
        p.setClipRegion(clipRgn); //or should we translate the painter instead???
        if (!win) { //background
            if (bgBrush->style() != Qt::NoBrush) {
                p.setBrushOrigin(-blendOffset);
                p.fillRect(clipRgn.boundingRect(), *bgBrush);
            }
        } else {
            uchar opacity = win->opacity;
            QPixmap *buf = win->backingStore->pixmap();
            QPoint topLeft = win->requested_region.boundingRect().topLeft();
            Q_ASSERT (buf && !buf->isNull());
            if (opacity == 255) {
                win->backingStore->lock();
                p.drawPixmap(topLeft-blendOffset,*buf);
                win->backingStore->unlock();
            } else {
                QPixmap yuck(blendRegion.boundingRect().size());
                yuck.fill(QColor(0,0,0,opacity));

                QPainter pp;
                pp.begin(&yuck);
                pp.setCompositionMode(QPainter::CompositionMode_SourceIn);
                win->backingStore->lock();
                pp.drawPixmap(topLeft-blendRegion.boundingRect().topLeft(), *buf);
                win->backingStore->unlock();
                pp.end();

                p.drawPixmap(blendRegion.boundingRect().topLeft()-blendOffset, yuck);

            }
        }
    }
}



//#define QWS_REGION_DEBUG

/*!
    \class QWSWindow qwindowsystem_qws.h
    \brief The QWSWindow class provides server-specific functionality in Qt/Embedded.

    \ingroup qws

    When you run a Qt/Embedded application, it either runs as a server
    or connects to an existing server. If it runs as a server, some
    additional functionality is provided by the QWSServer class.

    This class maintains information about each window and allows
    operations to be performed on the windows.

    You can get the window's name(), caption() and winId(), along with
    the client() that owns the window.

    The region the window wants to draw on is returned by requestedRegion().

    The visibility of the window can be determined using
    isVisible(). Visibility can be changed using raise(), lower(), show(),
    hide() and setActiveWindow().
*/

/*!
    \fn int QWSWindow::winId() const

    Returns the window's Id.
*/

/*!
    \fn const QString &QWSWindow::name() const

    Returns the window's name.
*/

/*!
    \fn const QString &QWSWindow::caption() const

    Returns the window's caption.
*/

/*!
    \fn QWSClient* QWSWindow::client() const

    Returns the QWSClient that owns this window.
*/

/*!
    \fn QRegion QWSWindow::requestedRegion() const

    Returns the region that the window has requested to draw onto,
    including any window decorations.
    \omit
    \sa allocatedRegion()
    \endomit
*/

/* NO DOC
    \fn QRegion QWSWindow::allocatedRegion() const

    Returns the region that the window is allowed to draw onto,
    including any window decorations but excluding regions covered by
    other windows.

    \sa requestedRegion()
*/

/*!
    \fn bool QWSWindow::isVisible() const

    Returns true if the window is visible; otherwise returns false.
*/

/* NO DOC
    \fn bool QWSWindow::isPartiallyObscured() const

    Returns true if the window is partially obsured by another window
    or by the bounds of the screen; otherwise returns false.
*/

/*!
    \fn bool QWSWindow::isFullyObscured() const

    Returns true if the window is completely obsured by another window
    or by the bounds of the screen; otherwise returns false.
*/

QWSWindow::QWSWindow(int i, QWSClient* client)
        : id(i), modified(false),
          onTop(false), c(client), last_focus_time(0), opacity(255), opaque(true), d(0)
{
    backingStore = new QWSBackingStore;
}

/*!
    Raises the window above all other windows except "Stay on top" windows.
*/
void QWSWindow::raise()
{
    qwsServer->raiseWindow(this);
}

/*!
    Lowers the window below other windows.
*/
void QWSWindow::lower()
{
    qwsServer->lowerWindow(this);
}

/*!
    Shows the window.
*/
void QWSWindow::show()
{
    operation(QWSWindowOperationEvent::Show);
}

/*!
    Hides the window.
*/
void QWSWindow::hide()
{
    operation(QWSWindowOperationEvent::Hide);
}

/*!
    Make this the active window (i.e. sets the keyboard focus to this
    window).
*/
void QWSWindow::setActiveWindow()
{
    qwsServer->setFocus(this, true);
}

void QWSWindow::setName(const QString &n)
{
    rgnName = n;
}

/*!
  Sets the window's caption to \a c.
*/
void QWSWindow::setCaption(const QString &c)
{
    rgnCaption = c;
}


static int global_focus_time_counter=100;

void QWSWindow::focus(bool get)
{
    if (get)
        last_focus_time = global_focus_time_counter++;
    QWSFocusEvent event;
    event.simpleData.window = id;
    event.simpleData.get_focus = get;
    c->sendEvent(&event);
}

void QWSWindow::operation(QWSWindowOperationEvent::Operation o)
{
    QWSWindowOperationEvent event;
    event.simpleData.window = id;
    event.simpleData.op = o;
    c->sendEvent(&event);
}

/*!
    Destructor.
*/
QWSWindow::~QWSWindow()
{
#ifndef QT_NO_QWS_IM
    if (current_IM_win == this)
        current_IM_win = 0;
#endif
    delete backingStore;
}


/*********************************************************************
 *
 * Class: QWSClient
 *
 *********************************************************************/
//always use frame buffer
QWSClient::QWSClient(QObject* parent, QTcpSocket* sock, int id)
    : QObject(parent), command(0), cid(id)
{
#ifndef QT_NO_QWS_MULTIPROCESS
    if (!sock) {
        socketDescriptor = -1;
        csocket = 0;
        isClosed = false;
    } else {
        csocket = static_cast<QWSSocket*>(sock); //###

        isClosed = false;

        csocket->flush();
        socketDescriptor = csocket->socketDescriptor();
        connect(csocket, SIGNAL(readyRead()), this, SIGNAL(readyRead()));
        connect(csocket, SIGNAL(disconnected()), this, SLOT(closeHandler()));
        connect(csocket, SIGNAL(error(SocketError)), this, SLOT(errorHandler()));
    }
#else
    isClosed = false;
#endif //QT_NO_QWS_MULTIPROCESS
}

QWSClient::~QWSClient()
{
    cursors.clear();
}

void QWSClient::setIdentity(const QString& i)
{
    id = i;
}

void QWSClient::closeHandler()
{
    isClosed = true;
    emit connectionClosed();
}

void QWSClient::errorHandler()
{
#ifdef QWS_SOCKET_DEBUG
    qDebug("Client %p error %s", this, csocket ? csocket->errorString().toLatin1().constData() : "(no socket)");
#endif
    isClosed = true;
//####Do we need to clean out the pipes?

    emit connectionClosed();
}


int QWSClient::socket() const
{
    return socketDescriptor;
}

void QWSClient::sendEvent(QWSEvent* event)
{
#ifndef QT_NO_QWS_MULTIPROCESS
    if (csocket) {
        //qDebug() << "QWSClient::sendEvent type " << event->type << " socket state " << csocket->state();
        if (csocket->state() == QAbstractSocket::ConnectedState) {
            event->write(csocket);
//            csocket->flush(); //### triggers QAbstractSocket bug, and we don't seem to need it
        }
    }
    else
#endif
    {
        qt_client_enqueue(event);
    }
}

void QWSClient::sendConnectedEvent(const char *display_spec)
{
    QWSConnectedEvent event;
    event.simpleData.window = 0;
    event.simpleData.len = strlen(display_spec) + 1;
    event.simpleData.clientId = cid;
    char * tmp=(char *)display_spec;
    event.setData(tmp, event.simpleData.len);
    sendEvent(&event);
}

void QWSClient::sendMaxWindowRectEvent()
{
    QWSMaxWindowRectEvent event;
    event.simpleData.window = 0;
    event.simpleData.rect = maxwindow_rect;
    sendEvent(&event);
}


#ifndef QT_NO_QWS_PROPERTIES
void QWSClient::sendPropertyNotifyEvent(int property, int state)
{
    QWSPropertyNotifyEvent event;
    event.simpleData.window = 0; // not used yet
    event.simpleData.property = property;
    event.simpleData.state = state;
    sendEvent(&event);
}

void QWSClient::sendPropertyReplyEvent(int property, int len, char *data)
{
    QWSPropertyReplyEvent event;
    event.simpleData.window = 0; // not used yet
    event.simpleData.property = property;
    event.simpleData.len = len;
    event.setData(data, len);
    sendEvent(&event);
}
#endif //QT_NO_QWS_PROPERTIES
void QWSClient::sendSelectionClearEvent(int windowid)
{
    QWSSelectionClearEvent event;
    event.simpleData.window = windowid;
    sendEvent(&event);
}

void QWSClient::sendSelectionRequestEvent(QWSConvertSelectionCommand *cmd, int windowid)
{
    QWSSelectionRequestEvent event;
    event.simpleData.window = windowid;
    event.simpleData.requestor = cmd->simpleData.requestor;
    event.simpleData.property = cmd->simpleData.selection;
    event.simpleData.mimeTypes = cmd->simpleData.mimeTypes;
    sendEvent(&event);
}


/*********************************************************************
 *
 * Class: QWSServer
 *
 *********************************************************************/

/*!
    \class QWSServer qwindowsystem_qws.h
    \brief The QWSServer class provides server-specific functionality in Qt/Embedded.

    \ingroup qws

    When you run a Qt/Embedded application, it either runs as a server
    or connects to an existing server. If it runs as a server, some
    additional operations are provided by this class.

    This class is instantiated by QApplication for Qt/Embedded server
    processes. You should never construct this class yourself.

    A pointer to the QWSServer instance can be obtained via the global
    \c qwsServer variable.

    The mouse and keyboard devices can be opened with openMouse() and
    openKeyboard(). (Close them with closeMouse() and
    closeKeyboard().)

    The display is refreshed with refresh(), and painting can be
    enabled or disabled with enablePainting().

    Obtain the list of client windows with clientWindows() and find
    out which window is at a particular point with windowAt().

    Many static functions are provided, for example,
    setKeyboardFilter(), setKeyboardHandler(), setDefaultKeyboard()
    and setDefaultMouse().

    The size of the window rectangle can be set with
    setMaxWindowRect(), and the desktop's background can be set with
    setDesktopBackground().

    The screen saver is controlled with setScreenSaverInterval() and
    screenSaverActivate().
*/

/*!
    \enum QWSServer::IMState

    \internal

    \value InputMethodPreedit Composing.
    \value InputMethodCommit Finished composing.
    \omitvalue InputMethodCommitToPrev
*/

/*!
    \fn void QWSServer::markedText(const QString &text)

    This signal is emitted whenever some text is selected. The
    selection is passed in \a text.
*/

/*!
    \enum QWSServer::IMMouse

    \internal
*/

/*!
    \fn void QWSServer::setOverrideKeys(const KeyOverride *keyOveride)

    \internal

    Sets the override keys to \a keyOveride.
*/

/*!
    \fn const QList<QWSWindow*> &QWSServer::clientWindows()

    Returns the list of top-level windows. This list will change as
    applications add and remove wigdets so it should not be stored for
    future use. The windows are sorted in stacking order from
    top-most to bottom-most.
*/

/*!
    \fn void QWSServer::newChannel(const QString& channel)

    This signal is emitted when the QCopChannel \a channel is created.
*/

/*!
    \fn void QWSServer::removedChannel(const QString& channel)

    This signal is emitted immediately after the QCopChannel \a
    channel is destroyed. Note that a channel is not destroyed until
    all its listeners have unregistered.
*/

/*!
    \fn QWSServer::QWSServer(int flags, QObject *parent)

    Construct a QWSServer object.

    \warning This class is instantiated by QApplication for
    Qt/Embedded server processes. You should never construct this
    class yourself.

    The \a flags are used for keyboard and mouse setting. The server's
    parent is \a parent.
*/

struct QWSCommandStruct
{
    QWSCommandStruct(QWSCommand *c, QWSClient *cl) :command(c),client(cl){}
    QWSCommand *command;
    QWSClient *client;
};

QWSServer::QWSServer(int flags, QObject *parent) :
    QObject(parent), disablePainting(false)
{
    initServer(flags);
}

#ifdef QT3_SUPPORT
/*!
    Use the two-argument overload and call setObjectName() instead.
*/
QWSServer::QWSServer(int flags, QObject *parent, const char *name) :
    QObject(parent), disablePainting(false)
{
    setObjectName(name);
    initServer(flags);
}
#endif


static void ignoreSignal(int) {} // Used to eat SIGPIPE signals below

void QWSServer::initServer(int flags)
{
    d = new QWSServerData;
    Q_ASSERT(!qwsServer);
    qwsServer = this;

#ifndef QT_NO_QWS_MULTIPROCESS
    ssocket = new QWSServerSocket(qws_qtePipeFilename(), this);
    connect(ssocket, SIGNAL(newConnection()), this, SLOT(newConnection()));

    if ( !ssocket->isListening()) {
        perror("Error");
        qFatal("Failed to bind to %s", qws_qtePipeFilename().toLatin1().constData());
    }

    struct linger tmp;
    tmp.l_onoff=1;
    tmp.l_linger=0;
    setsockopt(ssocket->socketDescriptor(),SOL_SOCKET,SO_LINGER,(char *)&tmp,sizeof(tmp));


    signal(SIGPIPE, ignoreSignal); //we get it when we read
#endif
    focusw = 0;
    mouseGrabber = 0;
    mouseGrabbing = false;
    keyboardGrabber = 0;
    keyboardGrabbing = false;
#ifndef QT_NO_QWS_CURSOR
    haveviscurs = false;
    cursor = 0;
    nextCursor = 0;
#endif

#ifndef QT_NO_QWS_MULTIPROCESS

    if (!geteuid()) {
#if !defined(Q_OS_FREEBSD) && !defined(Q_OS_SOLARIS) && !defined(Q_OS_DARWIN)
        if(mount(0,"/var/shm", "shm", 0, 0)) {
            /* This just confuses people with 2.2 kernels
            if (errno != EBUSY)
                qDebug("Failed mounting shm fs on /var/shm: %s",strerror(errno));
            */
        }
#endif
    }
#endif

    // no selection yet
    selectionOwner.windowid = -1;
    selectionOwner.time.set(-1, -1, -1, -1);

    openDisplay();

    d->screensavertimer = new QTimer(this);
    d->screensavertimer->setSingleShot(true);
    connect(d->screensavertimer, SIGNAL(timeout()), this, SLOT(screenSaverTimeout()));
    screenSaverWake();

    clientMap[-1] = new QWSClient(this, 0, 0);

    // input devices
    if (!(flags&DisableMouse)) {
        openMouse();
    }
    initializeCursor();

#ifndef QT_NO_QWS_KEYBOARD
    if (!(flags&DisableKeyboard)) {
        openKeyboard();
    }
#endif
    if (!bgBrush)
        bgBrush = new QBrush(QColor(0x20, 0xb0, 0x50));
    refreshBackground();

#if !defined(QT_NO_SOUND) && !defined(Q_OS_DARWIN)
    soundserver = new QWSSoundServer(this);
#endif
}

/*!
    Destruct QWSServer
*/
QWSServer::~QWSServer()
{
    // destroy all clients
    for (ClientIterator it = clientMap.begin(); it != clientMap.end(); ++it)
        delete *it;

    qDeleteAll(windows);
    windows.clear();

    delete bgBrush;
    bgBrush = 0;
    closeDisplay();
    closeMouse();
#ifndef QT_NO_QWS_KEYBOARD
    closeKeyboard();
#endif
    delete d;
}

/*!
  \internal
*/
void QWSServer::releaseMouse(QWSWindow* w)
{
    if (w && mouseGrabber == w) {
        mouseGrabber = 0;
        mouseGrabbing = false;
#ifndef QT_NO_QWS_CURSOR
        if (nextCursor) {
            // Not grabbing -> set the correct cursor
            setCursor(nextCursor);
            nextCursor = 0;
        }
#endif
    }
}

/*!
  \internal
*/
void QWSServer::releaseKeyboard(QWSWindow* w)
{
    if (keyboardGrabber == w) {
        keyboardGrabber = 0;
        keyboardGrabbing = false;
    }
}

void QWSServer::handleWindowClose(QWSWindow *w)
{
    w->shuttingDown();
    if (focusw == w)
        setFocus(w,false);
    if (mouseGrabber == w)
        releaseMouse(w);
    if (keyboardGrabber == w)
        releaseKeyboard(w);
}


#ifndef QT_NO_QWS_MULTIPROCESS
/*!
  \internal
*/
void QWSServer::newConnection()
{
    while (QTcpSocket *sock = ssocket->nextPendingConnection()) {
        int socket = sock->socketDescriptor();

        clientMap[socket] = new QWSClient(this,sock, get_object_id());
        connect(clientMap[socket], SIGNAL(readyRead()),
                this, SLOT(doClient()));
        connect(clientMap[socket], SIGNAL(connectionClosed()),
                this, SLOT(clientClosed()));

        clientMap[socket]->sendConnectedEvent(qws_display_spec);

        if (!maxwindow_rect.isEmpty())
            clientMap[socket]->sendMaxWindowRectEvent();

        // pre-provide some object id's
        for (int i=0; i<20 && clientMap[socket]; i++)
            invokeCreate(0,clientMap[socket]);

    }
}
/*!
  \internal
*/
void QWSServer::clientClosed()
{
    QWSClient* cl = (QWSClient*)sender();

    // Remove any queued commands for this client
    int i = 0;
    while (i < commandQueue.size()) {
        QWSCommandStruct *cs = commandQueue.at(i);
        if (cs->client == cl) {
            commandQueue.removeAt(i);
            delete cs;
        } else {
            ++i;
        }
    }

#ifndef QT_NO_COP
    // Enfore unsubscription from all channels.
    QCopChannel::detach(cl);
#endif

    QRegion exposed;
    {
        // Shut down all windows for this client
        for (int i = 0; i < windows.size(); ++i) {
            QWSWindow* w = windows.at(i);
            if (w->forClient(cl))
                w->shuttingDown();
        }
    }
    {
        // Delete all windows for this client
        int i = 0;
        while (i < windows.size()) {
            QWSWindow* w = windows.at(i);
            if (w->forClient(cl)) {
                w->c = 0; //so we don't send events to it anymore
                releaseMouse(w);
                releaseKeyboard(w);
                exposed += w->requestedRegion(); //### too much, but how often do we do this...
//                rgnMan->remove(w->allocationIndex());
                if (focusw == w)
                    setFocus(focusw,0);
                if (mouseGrabber == w)
                    releaseMouse(w);
                windows.removeAll(w);
#ifndef QT_NO_QWS_PROPERTIES
                manager()->removeProperties(w->winId());
#endif
                emit windowEvent(w, Destroy);
                d->deletedWindows.append(w);
            } else {
                ++i;
            }
        }
        if (d->deletedWindows.count())
            QTimer::singleShot(0, this, SLOT(deleteWindowsLater()));
    }
    //qDebug("removing client %d with socket %d", cl->clientId(), cl->socket());
    clientMap.remove(cl->socket());
    if (cl == d->cursorClient)
        d->cursorClient = 0;
    if (qt_screen->clearCacheFunc)
        (qt_screen->clearCacheFunc)(qt_screen, cl->clientId());  // remove any remaining cache entries.
    cl->deleteLater();
    exposeRegion(exposed);
//    syncRegions();
}

void QWSServer::deleteWindowsLater()
{
    qDeleteAll(d->deletedWindows);
    d->deletedWindows.clear();
}

#endif //QT_NO_QWS_MULTIPROCESS


QWSCommand* QWSClient::readMoreCommand()
{
#ifndef QT_NO_QWS_MULTIPROCESS
    if (csocket) {
        // read next command
        if (!command) {
            int command_type = qws_read_uint(csocket);

            if (command_type>=0) {
                command = QWSCommand::factory(command_type);
            }
        }

        if (command) {
            if (command->read(csocket)) {
                // Finished reading a whole command.
                QWSCommand* result = command;
                command = 0;
                return result;
            }
        }

        // Not finished reading a whole command.
        return 0;
    }
    else
#endif
    {
        QList<QWSCommand*> *serverQueue = qt_get_server_queue();
        return serverQueue->isEmpty() ? 0 : serverQueue->takeFirst();
    }

}


/*!
  \internal
*/
void QWSServer::processEventQueue()
{
    if (qwsServer)
        qwsServer->doClient(qwsServer->clientMap[-1]);
}


#ifndef QT_NO_QWS_MULTIPROCESS
void QWSServer::doClient()
{
    static bool active = false;
    if (active) {
        qDebug("QWSServer::doClient() reentrant call, ignoring");
        return;
    }
    active = true;
    QWSClient* client = (QWSClient*)sender();
    doClient(client);
    active = false;
}
#endif

void QWSServer::doClient(QWSClient *client)
{
    QWSCommand* command=client->readMoreCommand();

    while (command) {
        QWSCommandStruct *cs = new QWSCommandStruct(command, client);
        commandQueue.append(cs);
        // Try for some more...
        command=client->readMoreCommand();
    }


    while (!commandQueue.isEmpty()) {
        QWSCommandStruct *cs = commandQueue.takeAt(0);
        switch (cs->command->type) {
        case QWSCommand::Identify:
            invokeIdentify((QWSIdentifyCommand*)cs->command, cs->client);
            break;
        case QWSCommand::Create:
            invokeCreate((QWSCreateCommand*)cs->command, cs->client);
            break;
        case QWSCommand::RegionName:
            invokeRegionName((QWSRegionNameCommand*)cs->command, cs->client);
            break;
        case QWSCommand::Region:
            invokeRegion((QWSRegionCommand*)cs->command, cs->client);
            break;
        case QWSCommand::RegionMove:
            invokeRegionMove((QWSRegionMoveCommand*)cs->command, cs->client);
            break;
        case QWSCommand::RegionDestroy:
            invokeRegionDestroy((QWSRegionDestroyCommand*)cs->command, cs->client);
            break;
#ifndef QT_NO_QWS_PROPERTIES
        case QWSCommand::AddProperty:
            invokeAddProperty((QWSAddPropertyCommand*)cs->command);
            break;
        case QWSCommand::SetProperty:
            invokeSetProperty((QWSSetPropertyCommand*)cs->command);
            break;
        case QWSCommand::RemoveProperty:
            invokeRemoveProperty((QWSRemovePropertyCommand*)cs->command);
            break;
        case QWSCommand::GetProperty:
            invokeGetProperty((QWSGetPropertyCommand*)cs->command, cs->client);
            break;
#endif
        case QWSCommand::SetSelectionOwner:
            invokeSetSelectionOwner((QWSSetSelectionOwnerCommand*)cs->command);
            break;
        case QWSCommand::RequestFocus:
            invokeSetFocus((QWSRequestFocusCommand*)cs->command, cs->client);
            break;
        case QWSCommand::ChangeAltitude:
            invokeSetAltitude((QWSChangeAltitudeCommand*)cs->command,
                               cs->client);
            break;
        case QWSCommand::SetOpacity:
            invokeSetOpacity((QWSSetOpacityCommand*)cs->command,
                               cs->client);
            break;

#ifndef QT_NO_QWS_CURSOR
        case QWSCommand::DefineCursor:
            invokeDefineCursor((QWSDefineCursorCommand*)cs->command, cs->client);
            break;
        case QWSCommand::SelectCursor:
            invokeSelectCursor((QWSSelectCursorCommand*)cs->command, cs->client);
            break;
        case QWSCommand::PositionCursor:
            invokePositionCursor((QWSPositionCursorCommand*)cs->command, cs->client);
            break;
#endif
        case QWSCommand::GrabMouse:
            invokeGrabMouse((QWSGrabMouseCommand*)cs->command, cs->client);
            break;
        case QWSCommand::GrabKeyboard:
            invokeGrabKeyboard((QWSGrabKeyboardCommand*)cs->command, cs->client);
            break;
#if !defined(QT_NO_SOUND) && !defined(Q_OS_DARWIN)
        case QWSCommand::PlaySound:
            invokePlaySound((QWSPlaySoundCommand*)cs->command, cs->client);
            break;
#endif
#ifndef QT_NO_COP
        case QWSCommand::QCopRegisterChannel:
            invokeRegisterChannel((QWSQCopRegisterChannelCommand*)cs->command,
                                   cs->client);
            break;
        case QWSCommand::QCopSend:
            invokeQCopSend((QWSQCopSendCommand*)cs->command, cs->client);
            break;
#endif
#ifndef QT_NO_QWS_IM
        case QWSCommand::IMUpdate:
            invokeIMUpdate((QWSIMUpdateCommand*)cs->command, cs->client);
            break;
        case QWSCommand::IMResponse:
            invokeIMResponse((QWSIMResponseCommand*)cs->command, cs->client);
            break;
        case QWSCommand::IMMouse:
            {
                if (current_IM) {
                    QWSIMMouseCommand *cmd = (QWSIMMouseCommand *) cs->command;
                    current_IM->mouseHandler(cmd->simpleData.index,
                                              cmd->simpleData.state);
                }
            }
            break;
#endif
        case QWSCommand::RepaintRegion:
            invokeRepaintRegion((QWSRepaintRegionCommand*)cs->command,
                                cs->client);
            break;
        }
        delete cs->command;
        delete cs;
    }
}


void QWSServer::showCursor()
{
#ifndef QT_NO_QWS_CURSOR
    qt_screencursor->show();
#endif
}

void QWSServer::hideCursor()
{
#ifndef QT_NO_QWS_CURSOR
    qt_screencursor->hide();
#endif
}

/*!
    If \a e is true, painting on the display is enabled; if \a e is
    false, painting is disabled.
*/
void QWSServer::enablePainting(bool e)
{
// ### don't like this
    if (e)
    {
        disablePainting = false;
        setWindowRegion(0, QRegion());
        showCursor();
//        syncRegions();
    }
    else
    {
        disablePainting = true;
        hideCursor();
        setWindowRegion(0, QRegion(0,0,swidth,sheight));
//        syncRegions();
    }
}

/*!
    Refreshes the entire display.
*/
void QWSServer::refresh()
{
    exposeRegion(QRegion(0,0,swidth,sheight));
//    syncRegions();
}

/*!
    \overload

    Refreshes the region \a r.
*/
void QWSServer::refresh(QRegion & r)
{
    exposeRegion(r);
//    syncRegions();
}

/*!
    Sets the area of the screen which Qt/Embedded applications will
    consider to be the maximum area to use for windows to \a r.

    \sa QWidget::showMaximized()
*/
void QWSServer::setMaxWindowRect(const QRect& r)
{
    QRect tr = qt_screen->mapToDevice(r,
        QSize(qt_screen->width(),qt_screen->height()));
    if (maxwindow_rect != tr) {
        maxwindow_rect = tr;
        qwsServer->sendMaxWindowRectEvents();
    }
}

/*!
  \internal
*/
void QWSServer::sendMaxWindowRectEvents()
{
    for (ClientIterator it = clientMap.begin(); it != clientMap.end(); ++it)
        (*it)->sendMaxWindowRectEvent();
}

/*!
    Set the mouse driver \a m to use if \c $QWS_MOUSE_PROTO is not
    defined. The default is platform-dependent.
*/
void QWSServer::setDefaultMouse(const char *m)
{
    defaultMouse = m;
}

/*!
    Set the keyboard driver to \a k, e.g. if \c $QWS_KEYBOARD is not
    defined. The default is platform-dependent.
*/
void QWSServer::setDefaultKeyboard(const char *k)
{
    defaultKeyboard = k;
}

static bool prevWin;


extern int *qt_last_x,*qt_last_y;


/*!
  \internal

  Send a mouse event. \a pos is the screen position where the mouse
  event occurred and \a state is a mask indicating which buttons are
  pressed.
*/
void QWSServer::sendMouseEvent(const QPoint& pos, int state, int wheel)
{
    const int btnMask = Qt::LeftButton | Qt::RightButton | Qt::MidButton;
    qwsServer->showCursor();

    if (state)
        qwsServer->screenSaverWake();

    if (qt_last_x) {
         *qt_last_x = pos.x();
         *qt_last_y = pos.y();
    }
     mousePosition = pos;
    qwsServer->d->mouseState = state;

    QWSMouseEvent event;

    //If grabbing window disappears, grab is still active until
    //after mouse release.
    QWSWindow *win = qwsServer->mouseGrabber ? qwsServer->mouseGrabber : qwsServer->windowAt(pos);
    event.simpleData.window = win ? win->id : 0;

#ifndef QT_NO_QWS_CURSOR
    qt_screencursor->move(pos.x(),pos.y());

    // Arrow cursor over desktop
    // prevWin remembers if the last event was over a window
    if (!win && prevWin) {
        if (!qwsServer->mouseGrabber)
            qwsServer->setCursor(QWSCursor::systemCursor(Qt::ArrowCursor));
        else
            qwsServer->nextCursor = QWSCursor::systemCursor(Qt::ArrowCursor);
        prevWin = false;
    }
    // reset prevWin
    if (win && !prevWin)
        prevWin = true;
#endif

    if ((state&btnMask) && !qwsServer->mouseGrabbing) {
        qwsServer->mouseGrabber = win;
    }

    event.simpleData.x_root=pos.x();
    event.simpleData.y_root=pos.y();
    event.simpleData.state=state | qws_keyModifiers;
    event.simpleData.delta = wheel;
    event.simpleData.time=qwsServer->timer.elapsed();

    QWSClient *serverClient = qwsServer->clientMap[-1];
    QWSClient *winClient = win ? win->client() : 0;

#ifndef QT_NO_QWS_IM
    //reset input method if we click outside
    //####### we can't do this; IM may want to do something different

    // TODO: add an attribute IMTransparent

    static int oldstate = 0;
    bool isPress = state > oldstate;
    oldstate = state;
    if (isPress && current_IM && current_IM_winId != -1) {
        QWSWindow *kbw = keyboardGrabber ? keyboardGrabber :
                         qwsServer->focusw;

        QWidget *target = winClient == serverClient ?
                          QApplication::widgetAt(pos) : 0;
        if (kbw != win && (!target || !(target->testAttribute(Qt::WA_InputMethodTransparent))))
            current_IM->mouseHandler(-1, MouseOutside);
    }
#endif

    if (serverClient)
       serverClient->sendEvent(&event);
    if (winClient && winClient != serverClient)
       winClient->sendEvent(&event);

    // Make sure that if we leave a window, that window gets one last mouse
    // event so that it knows the mouse has left.
    QWSClient *oldClient = qwsServer->d->cursorClient;
    if (oldClient && oldClient != winClient && oldClient != serverClient)
        oldClient->sendEvent(&event);

    qwsServer->d->cursorClient = winClient;

    if (!(state&btnMask) && !qwsServer->mouseGrabbing)
        qwsServer->releaseMouse(qwsServer->mouseGrabber);
}

/*!
    Returns the primary mouse handler.
*/
QWSMouseHandler *QWSServer::mouseHandler()
{
    return qwsServer->mousehandlers.first();
}

// called by QWSMouseHandler constructor, not user code.
/*!
  \internal
*/
void QWSServer::setMouseHandler(QWSMouseHandler* mh)
{
    qwsServer->mousehandlers.prepend(mh);
}

/*!
  \internal

  Caller owns data in list, and must delete contents
*/
QList<QWSInternalWindowInfo*> * QWSServer::windowList()
{
    QList<QWSInternalWindowInfo*> * ret=new QList<QWSInternalWindowInfo*>;
    for (int i=0; i < qwsServer->windows.size(); ++i) {
        QWSWindow *window = qwsServer->windows.at(i);
        QWSInternalWindowInfo * qwi=new QWSInternalWindowInfo();
        qwi->winid=window->winId();
        qwi->clientid=window->client()->clientId();
#ifndef QT_NO_QWS_PROPERTIES
        char * name;
        int len;
        qwsServer->propertyManager.getProperty(qwi->winid,
                                               QT_QWS_PROPERTY_WINDOWNAME,
                                               name,len);
        if(name) {
            char * buf=(char *)malloc(len+2);
            strncpy(buf,name,len);
            buf[len]=0;
            qwi->name=buf;
            free(buf);
        } else {
            qwi->name="unknown";
        }
#else
        qwi->name="unknown";
#endif
        ret->append(qwi);
    }

    return ret;
}

#ifndef QT_NO_COP
/*!
  \internal
*/
void QWSServer::sendQCopEvent(QWSClient *c, const QString &ch,
                               const QString &msg, const QByteArray &data,
                               bool response)
{
    Q_ASSERT(c);

    QWSQCopMessageEvent event;
    event.simpleData.is_response = response;
    event.simpleData.lchannel = ch.length();
    event.simpleData.lmessage = msg.length();
    event.simpleData.ldata = data.size();
    int l = event.simpleData.lchannel + event.simpleData.lmessage +
            event.simpleData.ldata;

    // combine channel, message and data into one block of raw bytes
    QByteArray raw(l, 0);
    char *d = (char*)raw.data();
    memcpy(d, ch.toLatin1().constData(), event.simpleData.lchannel);
    d += event.simpleData.lchannel;
    memcpy(d, msg.toLatin1().constData(), event.simpleData.lmessage);
    d += event.simpleData.lmessage;
    memcpy(d, data.data(), event.simpleData.ldata);

    event.setData(raw.data(), l);

    c->sendEvent(&event);
}
#endif

/*!
    Returns the window containing the point \a pos or 0 if there is no
    window under the point.
*/
QWSWindow *QWSServer::windowAt(const QPoint& pos)
{
    for (int i=0; i<windows.size(); ++i) {
        QWSWindow* w = windows.at(i);
        if (w->requested_region.contains(pos))
            return w;
    }
    return 0;
}

static int keyUnicode(int keycode)
{
    int code = 0xffff;

    if (keycode >= Qt::Key_A && keycode <= Qt::Key_Z)
        code = keycode - Qt::Key_A + 'a';
    else if (keycode >= Qt::Key_0 && keycode <= Qt::Key_9)
        code = keycode - Qt::Key_0 + '0';

    return code;
}

/*!
    Send a key event. You can use this to send key events generated by
    "virtual keyboards". \a unicode is the Unicode value of the key to
    send, \a keycode the Qt keycode (e.g. Qt::Key_Left), \a modifiers
    indicates whether, Shift/Alt/Ctrl keys are pressed, \a isPress is
    true if this is a key down event and false if it's a key up event,
    and \a autoRepeat is true if this is an autorepeat event (i.e. the
    user has held the key down and this is the second or subsequent
    key event being sent).
*/
void QWSServer::sendKeyEvent(int unicode, int keycode, Qt::KeyboardModifiers modifiers,
                             bool isPress, bool autoRepeat)
{
    qws_keyModifiers = modifiers;

    if (isPress) {
        if (keycode != Qt::Key_F34 && keycode != Qt::Key_F35)
            qwsServer->screenSaverWake();
    }

#ifndef QT_NO_QWS_IM

    if (!current_IM || !current_IM->filter(unicode, keycode, modifiers, isPress, autoRepeat))
        sendKeyEventUnfiltered(unicode, keycode, modifiers, isPress, autoRepeat);
}

void QWSServer::sendKeyEventUnfiltered(int unicode, int keycode, Qt::KeyboardModifiers modifiers,
                                       bool isPress, bool autoRepeat)
{
#endif

    QWSKeyEvent event;
    QWSWindow *win = keyboardGrabber ? keyboardGrabber :
        qwsServer->focusw;

    event.simpleData.window = win ? win->winId() : 0;

    event.simpleData.unicode =
#ifndef QT_NO_QWS_KEYBOARD
        unicode < 0 ? keyUnicode(keycode) :
#endif
        unicode;
    event.simpleData.keycode = keycode;
    event.simpleData.modifiers = modifiers;
    event.simpleData.is_press = isPress;
    event.simpleData.is_auto_repeat = autoRepeat;

    for (ClientIterator it = qwsServer->clientMap.begin(); it != qwsServer->clientMap.end(); ++it)
        (*it)->sendEvent(&event);
}

/*!
    \internal
*/
void QWSServer::beginDisplayReconfigure()
{
    qwsServer->enablePainting(false);
#ifndef QT_NO_QWS_CURSOR
    qt_screencursor->hide();
#endif
    QWSDisplay::grab(true);
    qt_screen->disconnect();
}

/*!
    \internal
*/
void QWSServer::endDisplayReconfigure()
{
    delete qwsServer->paintEngine;
    qt_screen->connect(QString());
    qwsServer->swidth = qt_screen->deviceWidth();
    qwsServer->sheight = qt_screen->deviceHeight();

    qwsServer->paintEngine = static_cast<QRasterPaintEngine*>(qt_screen->createScreenEngine());
    QWSDisplay::ungrab();
#ifndef QT_NO_QWS_CURSOR
    qt_screencursor->show();
#endif
    qt_qws_set_max_window_rect(QRect(0, 0, qt_screen->deviceWidth(), qt_screen->deviceHeight()));
    QSize olds = qApp->desktop()->size();
    qApp->desktop()->resize(qt_screen->width(), qt_screen->height());
    qApp->postEvent(qApp->desktop(), new QResizeEvent(qApp->desktop()->size(), olds));
    qwsServer->enablePainting(true);
    qwsServer->refresh();
    qDebug("Desktop size: %dx%d", qApp->desktop()->width(), qApp->desktop()->height());
}

void QWSServer::resetEngine()
{
#ifndef QT_NO_QWS_CURSOR
    qt_screencursor->hide();
    qt_screencursor->show();
#endif
    delete qwsServer->paintEngine;
    qwsServer->paintEngine = static_cast<QRasterPaintEngine*>(qt_screen->createScreenEngine());
}


#ifndef QT_NO_QWS_CURSOR
/*!
    If \a vis is true, makes the cursor visible; if \a vis is false,
    makes the cursor invisible.

    \sa isCursorVisible()
*/
void QWSServer::setCursorVisible(bool vis)
{
    if (qwsServer && qwsServer->haveviscurs != vis) {
        QWSCursor* c = qwsServer->cursor;
        qwsServer->setCursor(QWSCursor::systemCursor(Qt::BlankCursor));
        qwsServer->haveviscurs = vis;
        qwsServer->setCursor(c);
    }
}

/*!
    Returns true if the cursor is visible; otherwise returns false.

    \sa setCursorVisible()
*/
bool QWSServer::isCursorVisible()
{
    return qwsServer ? qwsServer->haveviscurs : true;
}
#endif

#ifndef QT_NO_QWS_IM


//### qt 3 support: ???


/*!
    This function sends an input method event to the server. The
    current state is passed in \a state and the current text in \a
    txt. The cursor's position in the text is given by \a cpos, and
    the selection length (which could be 0) is given in \a selLen.
*/
void QWSServer::sendIMEvent(IMState state, const QString& txt, int cpos, int selLen)
{
    QWSIMEvent event;

    QWSWindow *win = keyboardGrabber ? keyboardGrabber :
                     qwsServer->focusw;

    if (state == InputMethodCommitToPrev && current_IM_win)
        win = current_IM_win;

    event.simpleData.window = win ? win->winId() : 0;
    event.simpleData.replaceFrom = 0;
    event.simpleData.replaceLength = 0;

    QString com = (state == InputMethodPreedit) ? QString() : txt;
    QString pre = (state == InputMethodPreedit) ? txt : QString();

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QDataStream out(&buffer);

    out << pre;
    out << com;

    if (state == InputMethodPreedit) {
        if (cpos > 0)
            out << int(QInputMethodEvent::TextFormat) << 0 <<cpos << QVariant(int(QInputContext::PreeditFormat));

        if (selLen)
            out << int(QInputMethodEvent::TextFormat) << cpos << selLen << QVariant(int(QInputContext::SelectionFormat));
        if (cpos + selLen < txt.length())
            out << int(QInputMethodEvent::TextFormat) << cpos + selLen << txt.length() - cpos - selLen
                << QVariant(int(QInputContext::PreeditFormat));

        out << int(QInputMethodEvent::Cursor) << cpos << 0 << QVariant();
    }

    event.setData(buffer.data(), buffer.size());

    QWSClient *serverClient = qwsServer->clientMap[-1];
    if (serverClient)
        serverClient->sendEvent(&event);
    if (win && win->client() && win->client() != serverClient)
        win->client()->sendEvent(&event);

    current_IM_ComposeMode = (state == InputMethodPreedit);
    current_IM_win = win;
    current_IM_winId = win->winId();
}


/*!
  Sends an input method query for the specified \a property.

  You must reimplement the QWSInputMethod::responseHandler() event handler
  in a subclass of QWSInputMethod if you want to receive responses to
  input method queries.
*/
void QWSServer::sendIMQuery(int property)
{
    QWSIMQueryEvent event;

    QWSWindow *win = keyboardGrabber ? keyboardGrabber :
                     qwsServer->focusw;
    if (current_IM_ComposeMode && current_IM_win)
        win = current_IM_win;

    event.simpleData.window = win ? win->winId() : 0;
    event.simpleData.property = property;
    if (win && win->client())
        win->client()->sendEvent(&event);
}



/*!
    \internal

    Sets the current input method to \a im.
*/
void QWSServer::setCurrentInputMethod(QWSInputMethod *im)
{
    if (current_IM)
        current_IM->reset(); //??? send an update event instead ?
    current_IM = im;
}

/*!
    \fn static void QWSServer::resetInputMethod()

    \internal
*/

#endif //QT_NO_QWS_IM

#ifndef QT_NO_QWS_PROPERTIES
/*!
  \internal
*/
void QWSServer::sendPropertyNotifyEvent(int property, int state)
{
    ClientIterator it = clientMap.begin();
    while (it != clientMap.end()) {
        QWSClient *cl = *it;
        ++it;
        cl->sendPropertyNotifyEvent(property, state);
    }
}
#endif

void QWSServer::invokeIdentify(const QWSIdentifyCommand *cmd, QWSClient *client)
{
    client->setIdentity(cmd->id);
}

void QWSServer::invokeCreate(QWSCreateCommand *, QWSClient *client)
{
    QWSCreationEvent event;
    event.simpleData.objectid = get_object_id();
    client->sendEvent(&event);
}

void QWSServer::invokeRegionName(const QWSRegionNameCommand *cmd, QWSClient *client)
{
    QWSWindow* changingw = findWindow(cmd->simpleData.windowid, client);
    if (changingw) {
        changingw->setName(cmd->name);
        changingw->setCaption(cmd->caption);
        emit windowEvent(changingw, Name);
    }
}

void QWSServer::invokeRegion(QWSRegionCommand *cmd, QWSClient *client)
{
#ifdef QWS_REGION_DEBUG
    qDebug("QWSServer::invokeRegion %d rects (%d)",
            cmd->simpleData.nrectangles, cmd->simpleData.windowid);
#endif

    QWSWindow* changingw = findWindow(cmd->simpleData.windowid, 0);
    if (!changingw) {
        qWarning("Invalid window handle %08x",cmd->simpleData.windowid);
        return;
    }
    if (!changingw->forClient(client)) {
        qWarning("Disabled: clients changing other client's window region");
        return;
    }

//########    bool containsMouse = changingw->allocatedRegion().contains(mousePosition);

    QRegion region;
    region.setRects(cmd->rectangles, cmd->simpleData.nrectangles);

    changingw->backingStore->attach(cmd->simpleData.shmid, region.boundingRect().size());
    changingw->opaque = cmd->simpleData.opaque;

    setWindowRegion(changingw, region);

    bool isShow = !changingw->isVisible() && !region.isEmpty();
    if (isShow)
        emit windowEvent(changingw, Show);
    if (!region.isEmpty())
        emit windowEvent(changingw, Geometry);
    else
        emit windowEvent(changingw, Hide);
    if (region.isEmpty())
        handleWindowClose(changingw);
    // if the window under our mouse changes, send update.
//##########     if (containsMouse != changingw->allocatedRegion().contains(mousePosition))
//         updateClientCursorPos();
}

void QWSServer::invokeRegionMove(const QWSRegionMoveCommand *cmd, QWSClient *client)
{
    QWSWindow* changingw = findWindow(cmd->simpleData.windowid, 0);
    if (!changingw) {
        qWarning("invokeRegionMove: Invalid window handle %d",cmd->simpleData.windowid);
        return;
    }
    if (!changingw->forClient(client)) {
        qWarning("Disabled: clients changing other client's window region");
        return;
    }

//    changingw->setNeedAck(true);
    moveWindowRegion(changingw, cmd->simpleData.dx, cmd->simpleData.dy);
    emit windowEvent(changingw, Geometry);
}

void QWSServer::invokeRegionDestroy(const QWSRegionDestroyCommand *cmd, QWSClient *client)
{
    QWSWindow* changingw = findWindow(cmd->simpleData.windowid, 0);
    if (!changingw) {
        qWarning("invokeRegionDestroy: Invalid window handle %d",cmd->simpleData.windowid);
        return;
    }
    if (!changingw->forClient(client)) {
        qWarning("Disabled: clients changing other client's window region");
        return;
    }

    setWindowRegion(changingw, QRegion());
//    rgnMan->remove(changingw->allocationIndex());
    for (int i = 0; i < windows.size(); ++i) {
        if (windows.at(i) == changingw) {
            windows.takeAt(i);
            break;
        }
    }
//    syncRegions();
    handleWindowClose(changingw);
#ifndef QT_NO_QWS_PROPERTIES
    manager()->removeProperties(changingw->winId());
#endif
    emit windowEvent(changingw, Destroy);
    delete changingw;
}

void QWSServer::invokeSetFocus(const QWSRequestFocusCommand *cmd, QWSClient *client)
{
    int winId = cmd->simpleData.windowid;
    int gain = cmd->simpleData.flag;

    if (gain != 0 && gain != 1) {
        qWarning("Only 0(lose) and 1(gain) supported");
        return;
    }

    QWSWindow* changingw = findWindow(winId, 0);
    if (!changingw)
        return;

    if (!changingw->forClient(client)) {
       qWarning("Disabled: clients changing other client's focus");
        return;
    }

    setFocus(changingw, gain);
}

void QWSServer::setFocus(QWSWindow* changingw, bool gain)
{
#ifndef QT_NO_QWS_IM
    /*
      This is the logic:
      QWSWindow *loser = 0;
      if (gain && focusw != changingw)
         loser = focusw;
      else if (!gain && focusw == changingw)
         loser = focusw;
      But these five lines can be reduced to one:
    */
    if (current_IM) {
        QWSWindow *loser =  (!gain == (focusw==changingw)) ? focusw : 0;
        if (loser && loser->winId() == current_IM_winId)
            current_IM->updateHandler(QWSIMUpdateCommand::FocusOut);
    }
#endif
    if (gain) {
        if (focusw != changingw) {
            if (focusw) focusw->focus(0);
            focusw = changingw;
            focusw->focus(1);
            emit windowEvent(focusw, Active);
        }
    } else if (focusw == changingw) {
        if (changingw->client())
            changingw->focus(0);
        focusw = 0;
        // pass focus to window which most recently got it...
        QWSWindow* bestw=0;
        for (int i=0; i<windows.size(); ++i) {
            QWSWindow* w = windows.at(i);
            if (w != changingw && !w->hidden() &&
                    (!bestw || bestw->focusPriority() < w->focusPriority()))
                bestw = w;
        }
        if (!bestw && changingw->focusPriority()) { // accept focus back?
            bestw = changingw; // must be the only one
        }
        focusw = bestw;
        if (focusw)
            focusw->focus(1);
        emit windowEvent(focusw, Active);
    }
}



void QWSServer::invokeSetOpacity(const QWSSetOpacityCommand *cmd,
                                   QWSClient *client)
{
    int winId = cmd->simpleData.windowid;
    int opacity = cmd->simpleData.opacity;

    QWSWindow* changingw = findWindow(winId, 0);

    if (!changingw) {
        qWarning("invokeSetOpacity: Invalid window handle %d", winId);
        return;
    }

    int altitude = windows.indexOf(changingw);
    changingw->opacity = opacity;
    exposeRegion(changingw->requested_region, altitude);
}

void QWSServer::invokeSetAltitude(const QWSChangeAltitudeCommand *cmd,
                                   QWSClient *client)
{
    int winId = cmd->simpleData.windowid;
    int alt = cmd->simpleData.altitude;
    bool fixed = cmd->simpleData.fixed;
#if 0
    qDebug("QWSServer::invokeSetAltitude winId %d alt %d)", winId, alt);
#endif

    if (alt < -1 || alt > 1) {
        qWarning("QWSServer::invokeSetAltitude Only lower, raise and stays-on-top supported");
        return;
    }

    QWSWindow* changingw = findWindow(winId, 0);
    if (!changingw) {
        qWarning("invokeSetAltitude: Invalid window handle %d", winId);
        return;
    }

    if (fixed && alt >= 1) {
        changingw->onTop = true;
    }
    if (alt < 0)
        lowerWindow(changingw, alt);
    else
        raiseWindow(changingw, alt);

//      if (!changingw->forClient(client)) {
//         refresh();
//     }
}

#ifndef QT_NO_QWS_PROPERTIES
void QWSServer::invokeAddProperty(QWSAddPropertyCommand *cmd)
{
    manager()->addProperty(cmd->simpleData.windowid, cmd->simpleData.property);
}

void QWSServer::invokeSetProperty(QWSSetPropertyCommand *cmd)
{
    if (manager()->setProperty(cmd->simpleData.windowid,
                                    cmd->simpleData.property,
                                    cmd->simpleData.mode,
                                    cmd->data,
                                    cmd->rawLen)) {
        sendPropertyNotifyEvent(cmd->simpleData.property,
                                 QWSPropertyNotifyEvent::PropertyNewValue);
#ifndef QT_NO_QWS_IM
        if (cmd->simpleData.property == QT_QWS_PROPERTY_MARKEDTEXT) {
            QString s((const QChar*)cmd->data, cmd->rawLen/2);
            emit markedText(s);
        }
#endif
    }
}

void QWSServer::invokeRemoveProperty(QWSRemovePropertyCommand *cmd)
{
    if (manager()->removeProperty(cmd->simpleData.windowid,
                                       cmd->simpleData.property)) {
        sendPropertyNotifyEvent(cmd->simpleData.property,
                                 QWSPropertyNotifyEvent::PropertyDeleted);
    }
}

void QWSServer::invokeGetProperty(QWSGetPropertyCommand *cmd, QWSClient *client)
{
    char *data;
    int len;

    if (manager()->getProperty(cmd->simpleData.windowid,
                                    cmd->simpleData.property,
                                    data, len)) {
        client->sendPropertyReplyEvent(cmd->simpleData.property, len, data);
    } else {
        client->sendPropertyReplyEvent(cmd->simpleData.property, -1, 0);
    }
}
#endif //QT_NO_QWS_PROPERTIES

void QWSServer::invokeSetSelectionOwner(QWSSetSelectionOwnerCommand *cmd)
{
    qDebug("QWSServer::invokeSetSelectionOwner");

    SelectionOwner so;
    so.windowid = cmd->simpleData.windowid;
    so.time.set(cmd->simpleData.hour, cmd->simpleData.minute,
                 cmd->simpleData.sec, cmd->simpleData.ms);

    if (selectionOwner.windowid != -1) {
        QWSWindow *win = findWindow(selectionOwner.windowid, 0);
        if (win)
            win->client()->sendSelectionClearEvent(selectionOwner.windowid);
        else
            qDebug("couldn't find window %d", selectionOwner.windowid);
    }

    selectionOwner = so;
}

void QWSServer::invokeConvertSelection(QWSConvertSelectionCommand *cmd)
{
    qDebug("QWSServer::invokeConvertSelection");

    if (selectionOwner.windowid != -1) {
        QWSWindow *win = findWindow(selectionOwner.windowid, 0);
        if (win)
            win->client()->sendSelectionRequestEvent(cmd, selectionOwner.windowid);
        else
            qDebug("couldn't find window %d", selectionOwner.windowid);
    }
}

#ifndef QT_NO_QWS_CURSOR
void QWSServer::invokeDefineCursor(QWSDefineCursorCommand *cmd, QWSClient *client)
{
    if (cmd->simpleData.height > 64 || cmd->simpleData.width > 64) {
        qDebug("Cannot define cursor size > 64x64");
        return;
    }

    int dataLen = cmd->simpleData.height * ((cmd->simpleData.width+7) / 8);

    QWSCursor *curs = new QWSCursor(cmd->data, cmd->data + dataLen,
                                cmd->simpleData.width, cmd->simpleData.height,
                                cmd->simpleData.hotX, cmd->simpleData.hotY);

    client->cursors.insert(cmd->simpleData.id, curs);
}

void QWSServer::invokeSelectCursor(QWSSelectCursorCommand *cmd, QWSClient *client)
{
    int id = cmd->simpleData.id;
    QWSCursor *curs = 0;
    if (id <= Qt::LastCursor) {
        curs = QWSCursor::systemCursor(id);
    }
    else {
        QWSCursorMap cursMap = client->cursors;
        QWSCursorMap::Iterator it = cursMap.find(id);
        if (it != cursMap.end()) {
            curs = it.value();
        }
    }
    if (curs == 0) {
        curs = QWSCursor::systemCursor(Qt::ArrowCursor);
    }

    QWSWindow* win = findWindow(cmd->simpleData.windowid, 0);
    if (mouseGrabber) {
        // If the mouse is being grabbed, we don't want just anyone to
        // be able to change the cursor.  We do want the cursor to be set
        // correctly once mouse grabbing is stopped though.
        if (win != mouseGrabber)
            nextCursor = curs;
        else
            setCursor(curs);
    } else if (win && win->requestedRegion().contains(mousePosition)) { //##################### cursor
        // A non-grabbing window can only set the cursor shape if the
        // cursor is within its allocated region.
        setCursor(curs);
    }
}

void QWSServer::invokePositionCursor(QWSPositionCursorCommand *cmd, QWSClient *)
{
    QPoint newPos(cmd->simpleData.newX, cmd->simpleData.newY);
    if (newPos != mousePosition)
        sendMouseEvent(newPos, qwsServer->d->mouseState);
}
#endif

void QWSServer::invokeGrabMouse(QWSGrabMouseCommand *cmd, QWSClient *client)
{
    QWSWindow* win = findWindow(cmd->simpleData.windowid, 0);
    if (!win)
        return;

    if (cmd->simpleData.grab) {
        if (!mouseGrabber || mouseGrabber->client() == client) {
            mouseGrabbing = true;
            mouseGrabber = win;
        }
    } else {
        releaseMouse(mouseGrabber);
    }
}

void QWSServer::invokeGrabKeyboard(QWSGrabKeyboardCommand *cmd, QWSClient *client)
{
    QWSWindow* win = findWindow(cmd->simpleData.windowid, 0);
    if (!win)
        return;

    if (cmd->simpleData.grab) {
        if (!keyboardGrabber || (keyboardGrabber->client() == client)) {
            keyboardGrabbing = true;
            keyboardGrabber = win;
        }
    } else {
        releaseKeyboard(keyboardGrabber);
    }
}

#if !defined(QT_NO_SOUND)
void QWSServer::invokePlaySound(QWSPlaySoundCommand *cmd, QWSClient *)
{
#if !defined(Q_OS_DARWIN)
    soundserver->playFile(cmd->filename);
#endif
}
#endif

#ifndef QT_NO_COP
void QWSServer::invokeRegisterChannel(QWSQCopRegisterChannelCommand *cmd,
                                       QWSClient *client)
{
  // QCopChannel will force us to emit the newChannel signal if this channel
  // didn't already exist.
  QCopChannel::registerChannel(cmd->channel, client);
}

void QWSServer::invokeQCopSend(QWSQCopSendCommand *cmd, QWSClient *client)
{
    QCopChannel::answer(client, cmd->channel, cmd->message, cmd->data);
}

#endif

#ifndef QT_NO_QWS_IM
void QWSServer::resetInputMethod()
{
    if (current_IM && qwsServer) {
      current_IM->reset();
    }
    current_IM_winId = -1;
    current_IM_win = 0;
}

void QWSServer::invokeIMResponse(const QWSIMResponseCommand *cmd,
                                 QWSClient *)
{
    if (current_IM)
        current_IM->responseHandler(cmd->simpleData.property, cmd->result);
}

void QWSServer::invokeIMUpdate(const QWSIMUpdateCommand *cmd,
                                 QWSClient *)
{
    if (cmd->simpleData.type == QWSIMUpdateCommand::Update ||
        cmd->simpleData.type == QWSIMUpdateCommand::FocusIn)
        current_IM_winId = cmd->simpleData.windowid;

    if (current_IM)
        current_IM->updateHandler(cmd->simpleData.type);
}

#endif

void QWSServer::invokeRepaintRegion(QWSRepaintRegionCommand * cmd,
                                    QWSClient *)
{
    QRegion r;
    r.setRects(cmd->rectangles,cmd->simpleData.nrectangles);
    repaint_region(cmd->simpleData.windowid, cmd->simpleData.opaque, r);
}

QWSWindow* QWSServer::newWindow(int id, QWSClient* client)
{
    // Make a new window, put it on top.
    QWSWindow* w = new QWSWindow(id,client);

    // insert after "stays on top" windows
    bool added = false;
    for (int i = 0; i < windows.size(); ++i) {
        QWSWindow *win = windows.at(i);
        if (!win->onTop) {
            windows.insert(i, w);
            added = true;
            break;
        }
    }
    if (!added)
        windows.append(w);
    emit windowEvent(w, Create);
    return w;
}

QWSWindow* QWSServer::findWindow(int windowid, QWSClient* client)
{
    for (int i=0; i<windows.size(); ++i) {
        QWSWindow* w = windows.at(i);
        if (w->winId() == windowid)
            return w;
    }
    if (client)
        return newWindow(windowid,client);
    else
        return 0;
}


void QWSServer::raiseWindow(QWSWindow *changingw, int /*alt*/)
{
    if (changingw == windows.first())
        return;

    int windowPos = 0;

    //change position in list:
    for (int i = 0; i < windows.size(); ++i) {
        QWSWindow *w = windows.at(i);
        if (w == changingw) {
            windowPos = i;
            windows.takeAt(i);
            break;
        }
    }

    int newPos = -1;
    if (changingw->onTop) {
        windows.prepend(changingw);
        newPos = 0;
    } else {
        // insert after "stays on top" windows
        bool in = false;
        for (int i = 0; i < windows.size(); ++i) {
            QWSWindow *w = windows.at(i);
            if (!w->onTop) {
                windows.insert(i, changingw);
                in = true;
                newPos = i;
                break;
            }
        }
        if (!in) {
            windows.append(changingw);
            newPos = windows.size()-1;
        }
    }

    if (windowPos != newPos) {
        exposeRegion(changingw->requestedRegion(), newPos); //### exposes too much, including what was already visible
    }
//    syncRegions(changingw);
    emit windowEvent(changingw, Raise);
}

void QWSServer::lowerWindow(QWSWindow *changingw, int /*alt*/)
{
    if (changingw == windows.last())
        return;

    int i = windows.indexOf(changingw);
    windows.move(i,windows.size()-1);

    QRegion exposed = changingw->requestedRegion(); //### exposes too much, including what was already visible
    exposeRegion(exposed, i);
    emit windowEvent(changingw, Lower);
}

void QWSServer::moveWindowRegion(QWSWindow *changingw, int dx, int dy)
{
    if (!changingw) return;

    //### optimize with scroll
    QRegion oldRegion(changingw->requested_region);
    changingw->requested_region.translate(dx, dy);

    int idx = windows.indexOf(changingw);
    exposeRegion(oldRegion + changingw->requested_region, idx);
}

/*!
    Changes the requested region of window \a changingw to \a r
    If \a changingw is 0, the server's reserved region is changed.
*/
void QWSServer::setWindowRegion(QWSWindow* changingw, QRegion r)
{
    if (changingw->requested_region == r)
        return;
    QRegion oldRegion(changingw->requested_region);
    changingw->requested_region = r;

    int idx = windows.indexOf(changingw);
    exposeRegion(oldRegion + changingw->requested_region, idx);
}





#if !defined(QT_NO_QWS_CURSOR) && !defined(QT_QWS_ACCEL_CURSOR)
# define SCREEN_PAINT_START(r) bool swc_do_save=FALSE; \
                    if(qt_sw_cursor) \
                        swc_do_save = qt_screencursor->restoreUnder(r);
# define SCREEN_PAINT_END if(qt_sw_cursor && swc_do_save) \
                        qt_screencursor->saveUnder();
#else //QT_NO_QWS_CURSOR
# define SCREEN_PAINT_START(r)
# define SCREEN_PAINT_END
#endif //QT_NO_QWS_CURSOR

void QWSServer::exposeRegion(QRegion r, int changing)
{
    if (r.isEmpty())
        return;

    QRegion blendRegion;
    QPixmap blendBuffer;

    SCREEN_PAINT_START(r.boundingRect());

    compose(0, r, blendRegion, blendBuffer, changing);
    if (!blendBuffer.isNull()) {
        //bltToScreen
        QPoint topLeft = blendRegion.boundingRect().topLeft();

        QRectF destR(topLeft.x(),topLeft.y(), blendBuffer.width(), blendBuffer.height());
        QRectF sourceR(0,0, blendBuffer.width(), blendBuffer.height());
        paintEngine->updateClipRegion(blendRegion, Qt::ReplaceClip);
        paintEngine->drawPixmap(destR, blendBuffer, sourceR);
    }
    SCREEN_PAINT_END;
    qt_screen->setDirty(r.boundingRect());
}

/*!
    Closes the pointer device(s).
*/
void QWSServer::closeMouse()
{
    qDeleteAll(mousehandlers);
    mousehandlers.clear();
}

/*!
    Opens the mouse device(s).
*/
void QWSServer::openMouse()
{
    QByteArray mice = qgetenv("QWS_MOUSE_PROTO");
    if (mice.isEmpty()) {
#if defined(QT_QWS_CASSIOPEIA)
        mice = "TPanel:/dev/tpanel";
#endif
        if (mice.isEmpty())
            mice = defaultMouse;
    }
    closeMouse();
    bool needviscurs = true;
    if (mice != "None") {
        QList<QByteArray> mouse = mice.split(' ');
        for (QList<QByteArray>::Iterator m=mouse.begin(); m!=mouse.end(); ++m) {
            QString ms = *m;
            QWSMouseHandler* h = newMouseHandler(ms);
            (void)h;
            /* XXX handle mouse cursor visibility sensibly
               if (!h->inherits("QCalibratedMouseHandler"))
               needviscurs = true;
            */
        }
    }
#ifndef QT_NO_QWS_CURSOR
    setCursorVisible(needviscurs);
#endif
}

/*!
  Suspends mouse handling by suspending each registered mouse handler.

  \sa resumeMouse()
*/
void QWSServer::suspendMouse()
{
    for (int i=0; i < mousehandlers.size(); ++i)
        mousehandlers.at(i)->suspend();
}

/*!
  Resumes mouse handling by reactivating each registered mouse handler.

  \sa suspendMouse()
*/
void QWSServer::resumeMouse()
{
    for (int i=0; i < mousehandlers.size(); ++i)
        mousehandlers.at(i)->resume();
}



QWSMouseHandler* QWSServer::newMouseHandler(const QString& spec)
{
    static int init=0;
    if (!init && qt_screen) {
        init = 1;
    }

    int c = spec.indexOf(':');
    QString mouseProto;
    QString mouseDev;
    if (c >= 0) {
        mouseProto = spec.left(c);
        mouseDev = spec.mid(c+1);
    } else {
        mouseProto = spec;
    }

    QWSMouseHandler *handler = 0;
    handler = QMouseDriverFactory::create(mouseProto, mouseDev);
    return handler;
}

#ifndef QT_NO_QWS_KEYBOARD

/*!
    Closes keyboard device(s).
*/
void QWSServer::closeKeyboard()
{
    qDeleteAll(keyboardhandlers);
    keyboardhandlers.clear();
}

/*!
    Returns the primary keyboard handler.
*/
QWSKeyboardHandler* QWSServer::keyboardHandler()
{
    return qwsServer->keyboardhandlers.first();
}

/*!
    Sets the primary keyboard handler to \a kh.
*/
void QWSServer::setKeyboardHandler(QWSKeyboardHandler* kh)
{
    qwsServer->keyboardhandlers.prepend(kh);
}

/*!
    Opens the keyboard device(s).
*/
void QWSServer::openKeyboard()
{
    QString keyboards = qgetenv("QWS_KEYBOARD");
    if (keyboards.isEmpty()) {
#if defined(QT_QWS_CASSIOPEIA)
        keyboards = "Buttons";
#endif
        if (keyboards.isEmpty()) {
            keyboards = defaultKeyboard;        // last resort
        }
    }
    closeKeyboard();
    if (keyboards == "None")
        return;
    QString device;
    QString type;
    QStringList keyboard = keyboards.split(" ");
    for (QStringList::Iterator k=keyboard.begin(); k!=keyboard.end(); ++k) {
        QString spec = *k;
        int colon=spec.indexOf(':');
        if (colon>=0) {
            type = spec.left(colon);
            device = spec.mid(colon+1);
        } else {
            type = spec;
        }
        QWSKeyboardHandler* kh = QKbdDriverFactory::create(type, device);
        keyboardhandlers.append(kh);
    }
}

#endif //QT_NO_QWS_KEYBOARD

QPoint QWSServer::mousePosition;
QBrush *QWSServer::bgBrush = 0;

void QWSServer::move_region(const QWSRegionMoveCommand *cmd)
{
    QWSClient *serverClient = clientMap[-1];
    invokeRegionMove(cmd, serverClient);
}

void QWSServer::set_altitude(const QWSChangeAltitudeCommand *cmd)
{
    QWSClient *serverClient = clientMap[-1];
    invokeSetAltitude(cmd, serverClient);
}

void QWSServer::set_opacity(const QWSSetOpacityCommand *cmd)
{
    QWSClient *serverClient = clientMap[-1];
    invokeSetOpacity(cmd, serverClient);
}


void QWSServer::request_focus(const QWSRequestFocusCommand *cmd)
{
    invokeSetFocus(cmd, clientMap[-1]);
}

void QWSServer::set_identity(const QWSIdentifyCommand *cmd)
{
    invokeIdentify(cmd, clientMap[-1]);
}

void QWSServer::repaint_region(int wid, bool opaque, QRegion region)
{
    QWSWindow* changingw = findWindow(wid, 0);
    if (!changingw) {
        return;
    }
    changingw->opaque = opaque;
    int level = windows.indexOf(changingw);
    exposeRegion(region, level);
}

void QWSServer::request_region(int wid, int shmid, bool opaque, QRegion region)
{
    QWSClient *serverClient = clientMap[-1];
    QWSWindow* changingw = findWindow(wid, 0);
    if (!changingw) {
        return;
    }
    bool isShow = !changingw->isVisible() && !region.isEmpty();

    changingw->backingStore->attach(shmid, region.boundingRect().size());
    changingw->opaque = opaque;
    setWindowRegion(changingw, region);
    if (isShow)
        emit windowEvent(changingw, Show);
    if (!region.isEmpty())
        emit windowEvent(changingw, Geometry);
    else
        emit windowEvent(changingw, Hide);
    if (region.isEmpty())
        handleWindowClose(changingw);
}

void QWSServer::destroy_region(const QWSRegionDestroyCommand *cmd)
{
    invokeRegionDestroy(cmd, clientMap[-1]);
}

void QWSServer::name_region(const QWSRegionNameCommand *cmd)
{
    invokeRegionName(cmd, clientMap[-1]);
}

#ifndef QT_NO_QWS_IM
void QWSServer::im_response(const QWSIMResponseCommand *cmd)
 {
     invokeIMResponse(cmd, clientMap[-1]);
}

void QWSServer::im_update(const QWSIMUpdateCommand *cmd)
{
    invokeIMUpdate(cmd, clientMap[-1]);
}

void QWSServer::send_im_mouse(const QWSIMMouseCommand *cmd)
{
    if (current_IM)
        current_IM->mouseHandler(cmd->simpleData.index, cmd->simpleData.state);
}
#endif

void QWSServer::openDisplay()
{
    qt_init_display();

//    rgnMan = qt_fbdpy->regionManager();
    swidth = qt_screen->deviceWidth();
    sheight = qt_screen->deviceHeight();
    paintEngine = static_cast<QRasterPaintEngine*>(qt_screen->createScreenEngine());
}

void QWSServer::closeDisplay()
{
    delete paintEngine;
    qt_screen->shutdownDevice();
}

void QWSServer::paintBackground(const QRegion &rr)
{
    if (bgBrush->style() == Qt::NoBrush)
        return;
    QRegion r = rr;
    if (!r.isEmpty()) {
        Q_ASSERT (qt_fbdpy);

        r = qt_screen->mapFromDevice(r, QSize(swidth, sheight));

        paintEngine->updateClipRegion(r, Qt::ReplaceClip);
        QRect br(r.boundingRect());

        // background also handled in compose
        paintEngine->qwsFillRect(br.x(), br.y(), br.width(), br.height(), *bgBrush);
    }
}


void QWSServer::refreshBackground()
{
    QRegion r(0, 0, swidth, sheight);

    for (int i=0; i<windows.size(); ++i) {
        if (r.isEmpty())
            return; // Nothing left for deeper windows
        QWSWindow* w = windows.at(i);
        r -= w->requestedRegion();
    }
    SCREEN_PAINT_START(r.boundingRect());
    paintBackground(r);
    SCREEN_PAINT_END;
    qt_screen->setDirty(r.boundingRect());
}


/*!
    Sets the brush \a brush to be used as the background in the absence of
    obscuring windows.
*/
void QWSServer::setBackground(const QBrush &brush)
{
    *bgBrush = brush;

    if (qwsServer)
        qwsServer->refreshBackground();
}


#ifdef QT3_SUPPORT
/*!
    Sets the image \a img to be used as the background in the absence
    of obscuring windows.
*/
void QWSServer::setDesktopBackground(const QImage &img)
{
    if (img.isNull())
        setBackground(Qt::NoBrush);
    else
        setBackground(QBrush(QPixmap::fromImage(img)));
}

/*!
    \overload

    Sets the color \a c to be used as the background in the absence of
    obscuring windows.
*/
void QWSServer::setDesktopBackground(const QColor &c)
{
    setDesktopBackground(QBrush(c));
}
#endif

/*!
  \internal
 */
void QWSServer::startup(int flags)
{
    if (qwsServer)
        return;
    unlink(qws_qtePipeFilename().toLatin1().constData());
    (void)new QWSServer(flags);
}

/*!
  \internal
*/

void QWSServer::closedown()
{
    unlink(qws_qtePipeFilename().toLatin1().constData());
    delete qwsServer;
    qwsServer = 0;
}

void QWSServer::emergency_cleanup()
{
#ifndef QT_NO_QWS_KEYBOARD
    if (qwsServer)
        qwsServer->closeKeyboard();
#endif
}

#ifndef QT_NO_QWS_KEYBOARD
static QList<QWSServer::KeyboardFilter*> *keyFilters = 0;

/*!
  \internal
*/
void QWSServer::processKeyEvent(int unicode, int keycode, Qt::KeyboardModifiers modifiers,
                                bool isPress, bool autoRepeat)
{
    if (keyFilters) {
        for (int i = 0; i < keyFilters->size(); ++i) {
            QWSServer::KeyboardFilter *keyFilter = keyFilters->at(i);
            if (keyFilter->filter(unicode, keycode, modifiers, isPress, autoRepeat))
                return;
        }
    }
    sendKeyEvent(unicode, keycode, modifiers, isPress, autoRepeat);
}

/*!
    Adds a filter \a f to be invoked for all key events from physical
    keyboard drivers (events sent via processKeyEvent()).

    The filter is not invoked for keys generated by virtual keyboard
    drivers (events sent via sendKeyEvent()).

    Keyboard filters are removed by removeKeyboardFilter().
*/
void QWSServer::addKeyboardFilter(KeyboardFilter *f)
{
     if (!keyFilters)
        keyFilters = new QList<QWSServer::KeyboardFilter*>;
     if (f) {
        keyFilters->prepend(f);
     }
}

/*
//#######
 We should probably obsolete the whole keyboard filter thing since
 it's not useful for input methods anyway

 We could do removeKeyboardFilter(KeyboardFilter *f), but
 the "remove and delete the filter" concept does not match "user
 remembers the pointer".
*/

/*!
    Removes and deletes the most-recently added filter.
    The caller is responsible for matching each addition with a
    corresponding removal.
*/
void QWSServer::removeKeyboardFilter()
{
     if (!keyFilters || keyFilters->isEmpty())
         return;
     delete keyFilters->takeAt(0);
}
#endif // QT_NO_QWS_KEYBOARD

/*!
    Sets an array of timeouts for the screensaver to a list of \a ms
    milliseconds. A setting of zero turns off the screensaver. The
    array must be 0-terminated.
*/
void QWSServer::setScreenSaverIntervals(int* ms)
{
    if (!qwsServer)
        return;
    delete [] qwsServer->d->screensaverintervals;
    if (ms) {
        int* t=ms;
        int n=0;
        while (*t++) n++;
        if (n) {
            n++; // the 0
            qwsServer->d->screensaverintervals = new int[n];
            memcpy(qwsServer->d->screensaverintervals, ms, n*sizeof(int));
        } else {
            qwsServer->d->screensaverintervals = 0;
        }
    } else {
        qwsServer->d->screensaverintervals = 0;
    }
    qwsServer->screensaverinterval = 0;

    qwsServer->d->screensavertimer->stop();
    qt_screen->blank(false);
    qwsServer->screenSaverWake();
}

/*!
    Sets the timeout for the screensaver to \a ms milliseconds. A
    setting of zero turns off the screensaver.
*/
void QWSServer::setScreenSaverInterval(int ms)
{
    int v[2];
    v[0] = ms;
    v[1] = 0;
    setScreenSaverIntervals(v);
}

extern bool qt_disable_lowpriority_timers; //in qeventloop_unix.cpp

void QWSServer::screenSaverWake()
{
    if (d->screensaverintervals) {
        if (screensaverinterval != d->screensaverintervals) {
            if (d->saver) d->saver->restore();
            screensaverinterval = d->screensaverintervals;
        } else {
            if (!d->screensavertimer->isActive()) {
                qt_screen->blank(false);
                if (d->saver) d->saver->restore();
            }
        }
        d->screensavertimer->start(*screensaverinterval);
        d->screensavertime.start();
    }
    qt_disable_lowpriority_timers=false;
}

void QWSServer::screenSaverSleep()
{
    qt_screen->blank(true);
#if !defined(QT_QWS_IPAQ) && !defined(QT_QWS_EBX)
    d->screensavertimer->stop();
#else
    if (screensaverinterval) {
        d->screensavertimer->start(*screensaverinterval);
        d->screensavertime.start();
    } else {
        d->screensavertimer->stop();
    }
#endif
    qt_disable_lowpriority_timers=true;
}

/*!
    \internal

    Deletes the current screen saver and sets the screen saver to be
    \a ss.
*/
void QWSServer::setScreenSaver(QWSScreenSaver* ss)
{
    delete qwsServer->d->saver;
    qwsServer->d->saver = ss;
}

void QWSServer::screenSave(int level)
{
    if (d->saver) {
        if (d->saver->save(level)) {
            if (screensaverinterval && screensaverinterval[1]) {
                d->screensavertimer->start(*++screensaverinterval);
                d->screensavertime.start();
            } else {
                screensaverinterval = 0;
            }
        } else {
            // for some reason, the saver don't want us to change to the
            // next level, so we'll stay at this level for another interval
            if (screensaverinterval && *screensaverinterval) {
                d->screensavertimer->start(*screensaverinterval);
                d->screensavertime.start();
            }
        }
    } else {
        screensaverinterval = 0;//d->screensaverintervals;
        screenSaverSleep();
    }
}

void QWSServer::screenSaverTimeout()
{
    if (screensaverinterval) {
        if (d->screensavertime.elapsed() > *screensaverinterval*2) {
            // bogus (eg. unsuspend, system time changed)
            screenSaverWake(); // try again
            return;
        }
        screenSave(screensaverinterval-d->screensaverintervals);
    }
}

/*!
    Returns true if the screensaver is active (i.e. the screen is
    blanked); otherwise returns false.
*/
bool QWSServer::screenSaverActive()
{
    return qwsServer->screensaverinterval
        && !qwsServer->d->screensavertimer->isActive();
}

/*!
    If \a activate is true the screensaver is activated immediately;
    if \a activate is false the screensaver is deactivated.
*/
void QWSServer::screenSaverActivate(bool activate)
{
    if (activate)
        qwsServer->screenSaverSleep();
    else
        qwsServer->screenSaverWake();
}

void QWSServer::disconnectClient(QWSClient *c)
{
    QTimer::singleShot(0, c, SLOT(closeHandler()));
}

void QWSServer::updateClientCursorPos()
{
    QWSWindow *win = qwsServer->mouseGrabber ? qwsServer->mouseGrabber : qwsServer->windowAt(mousePosition);
    QWSClient *winClient = win ? win->client() : 0;
    if (winClient && winClient != d->cursorClient)
        sendMouseEvent(mousePosition, d->mouseState);
}

#ifndef QT_NO_QWS_IM

/*!
    \class QWSInputMethod
    \brief The QWSInputMethod class provides international input methods
    for Qt/Embedded.

    \ingroup qws

    Subclass this class to implement your own input method.

    An input methods consists of a keyboard filter and optionally a
    graphical interface. The keyboard filter intercepts key events
    from physical or virtual keyboards by implementing the filter()
    function.

    Use sendIMEvent() to send composition events.

    Use QWSServer::setCurrentInputMethod() to install an input method.

    This class is still subject to change.
*/

/*!
  Constructs a new input method
*/

QWSInputMethod::QWSInputMethod()
{

}

/*!
  Destructs the input method uninstalling it if it is currently installed.
*/
QWSInputMethod::~QWSInputMethod()
{
    if (current_IM == this)
        current_IM = 0;
}

/*!
    \fn bool QWSInputMethod::filter(int unicode, int keycode, int modifiers, bool isPress, bool autoRepeat)

    This function must be implemented in subclasses to handle key
    input from physical or virtual keyboards. Returning true will
    block the event from further processing.

    The Unicode value is given in \a unicode and the key code in \a
    keycode. Keyboard modifiers are OR-ed together in \a modifiers.
    If \a isPress is true this is a key press; otherwise it is a key
    release. If \a autoRepeat is true this is an auto-repeated key
    press.
*/


/*!
    Implemented in subclasses to reset the state of the input method.

    The default implementation calls sendIMEvent() with empty preedit
    and commit strings, if the input method is in compose mode.
*/
void QWSInputMethod::reset()
{
    if (current_IM_ComposeMode)
        sendIMEvent(QWSServer::InputMethodCommitToPrev, QString(), 0, 0);
}

/*!
  Handles update events, including resets and focus changes.

  Reimplementations must call the base implementation for all cases that it
  does not handle itself.

  \a type is a value defined in \l QWSIMUpdateCommand::UpdateType.
*/
void QWSInputMethod::updateHandler(int type)
{
    switch (type) {
    case QWSIMUpdateCommand::FocusOut:
    case QWSIMUpdateCommand::Reset:
        reset();
        break;

    default:
        break;
    }
}


/*!
  This event handler is implemented in subclasses to receive replies to an
  input method query.

  The specified \a property and \a result contain the property
  queried and the result returned in the reply.

  \sa sendIMQuery()
*/
void QWSInputMethod::responseHandler(int property, const QVariant &result)
{
    Q_UNUSED(property);
    Q_UNUSED(result);
}



/*!
  \fn void QWSInputMethod::mouseHandler(int x, int state)

  Implemented in subclasses to handle mouse presses/releases within
  the preedit text. The parameter \a x is the offset within
  the string that was sent with the InputMethodCompose event.
  \a state is either QWSServer::MousePress or QWSServer::MouseRelease

  if \a state < 0 then the mouse event is inside the widget, but outside the preedit text

  QWSServer::MouseOutside is sent when clicking in a different widget.

  The default implementation resets the input method on all mouse presses.

*/
void QWSInputMethod::mouseHandler(int, int state)
{
    if (state == QWSServer::MousePress || state == QWSServer::MouseOutside)
        reset();
}






/*!
    \fn QWSInputMethod::sendIMEvent(QWSServer::IMState state, const QString &txt, int cpos, int selLen)

    Causes a QIMEvent to be sent to the focus widget.

    \a txt is the text being composed (or the finished text if state
    is \l InputMethodCommit). \a cpos is the current cursor position.

    If \a state is \l InputMethodPreedit, \a selLen is the number of characters in
    the composition string (starting at \a cpos) that should be
    marked as selected by the input widget receiving the event.
*/

/*!
  \fn void QWSInputMethod::sendIMQuery(int property)

  Sends an input method query for the specified \a property.

  You must reimplement the responseHandler() event handler in your
  subclasses if you want to receive responses to input method queries.

  \sa responseHandler()
*/
#endif

/*!
    \fn  QWSWindow::QWSWindow(int i, QWSClient * client)

    Constructs a new top-level window, associated with the client \a
    client and giving it the id \a i.
*/

/*!
    \fn QWSServer::manager()

    Returns the QWSPropertyManager, which is used for implementing
    X11-style window properties.
*/

/*!
    \fn QWSServer::windowEvent(QWSWindow * w, QWSServer::WindowEvent e)

    This signal is emitted whenever something happens to a top-level
    window (e.g. it's created or destroyed). \a w is the window to
    which the event of type \a e has occurred.
*/

/*!
    \fn QWSServer::keyMap()

    Returns the keyboard mapping table used to convert keyboard
    scancodes to Qt keycodes and Unicode values. It's used by the
    keyboard driver in \c qkeyboard_qws.cpp.
*/

/*!
    \enum QWSServer::ServerFlags

    This enum is used to pass various options to the window system
    server.

    \value DisableKeyboard Ignore all keyboard input.
    \value DisableMouse Ignore all mouse input.
*/

/*
    \class QWSServer::KeyMap
    \brief The QWSServer::KeyMap class is used for mapping scancodes.

    \ingroup qws

    The KeyMap structure records an individual KeyMap entry in the
    array used to map keyboard scancodes to Qt key codes and Unicode
    values.
*/

/*!
    \class QWSServer::KeyboardFilter
    \brief The KeyboardFilter class provides a global keyboard event filter.

    \ingroup qws

    The KeyboardFilter class is used to implement a global, low-level
    filter on key events in the Qt/Embedded server application; this
    can be used to implement things like APM (advanced power
    management) suspend from a button without having to filter for it
    in all applications.
*/

/*!
    \fn QWSServer::KeyboardFilter::~KeyboardFilter()

    Destroys the keyboard filter.
*/

/*!
    \fn bool QWSServer::KeyboardFilter::filter(int unicode, int keycode, int modifiers, bool isPress, bool autoRepeat)

    Returns true if the specified key should be filtered; otherwise returns
    false. A true return value stops the key from being processed any further.

    The Unicode value is given in \a unicode and the key code in \a
    keycode. Keyboard modifiers are OR-ed together in \a modifiers.
    If \a isPress is true this is a key press; otherwise it is a key
    release. If \a autoRepeat is true this is an auto-repeated key
    press.

    All normal key events should be blocked while in compose mode.
*/

/*!
    \enum QWSServer::WindowEvent

    This specifies what sort of event has occurred to a top-level window:

    \value Create A new window has been created (QWidget constructor).
    \value Destroy The window has been closed and deleted (QWidget destructor).
    \value Hide The window has been hidden with QWidget::hide().
    \value Show The window has been shown with QWidget::show() or similar.
    \value Raise The window has been raised to the top of the desktop.
    \value Lower The window has been lowered.
    \value Geometry The window has changed size or position.
    \value Active The window has become the active window (has keyboard focus).
    \value Name The window has been named.
*/

