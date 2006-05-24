/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qkbd_qws.h"

#ifndef QT_NO_QWS_KEYBOARD

#include "qwindowsystem_qws.h"
#include "qscreen_qws.h"
#include "qtimer.h"
#include <stdlib.h>


class QWSKbPrivate : public QObject
{
    Q_OBJECT
public:
    QWSKbPrivate(QWSKeyboardHandler *h) {
        handler = h;
        arTimer = new QTimer(this);
        arTimer->setSingleShot(true);
        connect(arTimer, SIGNAL(timeout()), SLOT(autoRepeat()));
        repeatdelay = 400;
        repeatperiod = 80;
    }

    void beginAutoRepeat(int uni, int code, Qt::KeyboardModifiers mod) {
        unicode = uni;
        keycode = code;
        modifier = mod;
        arTimer->start(repeatdelay);
    }
    void endAutoRepeat() {
        arTimer->stop();
    }

private slots:
    void autoRepeat() {
        handler->processKeyEvent(unicode, keycode, modifier, false, true);
        handler->processKeyEvent(unicode, keycode, modifier, true, true);
        arTimer->start(repeatperiod);
    }

private:
    QWSKeyboardHandler *handler;
    int unicode;
    int keycode;
    Qt::KeyboardModifiers modifier;
    int repeatdelay;
    int repeatperiod;
    QTimer *arTimer;
};

/*!
    \class QWSKeyboardHandler
    \ingroup qws

    \brief The QWSKeyboardHandler class implements a keyboard driver
    in Qtopia Core.

    A keyboard driver handles events from system devices and generates
    key events. Custom keyboard drivers can be added by subclassing
    the QKbdDriverPlugin class, using the QKbdDriverFactory class to
    dynamically load the driver into the application.

    A QWSKeyboardHandler object will usually open some system device,
    and create a QSocketNotifier object for that device. The
    QSocketNotifier class provides support for monitoring activity on
    a file descriptor. When the socket notifier receives data, it will
    call the keyboard handler's processKeyEvent() function to send the
    event to the \l {Qtopia Core} server application for relaying to
    clients.

    QWSKeyboardHandler also provides functions to control
    auto-repetion of key sequences, beginAutoRepeat() and
    endAutoRepeat(), and the transformDirKey() function enabling
    transformation of arrow keys according to the display orientation.

    \sa {Character Input}, {Qtopia Core}
*/


/*!
    Constructs a keyboard handler.

    The handler \e may be passed to the system for later destruction
    using the QWSServer::setKeyboardHandler() function, but this is
    not required (i.e. the handler can still function, calling
    processKeyEvent() to emit events).
*/
QWSKeyboardHandler::QWSKeyboardHandler()
{
    d = new QWSKbPrivate(this);
}

/*!
    Destroys this keyboard handler.

    Do not call the destructor if the handler has been passed to the
    QWSServer::setKeyboardHandler() function.
*/
QWSKeyboardHandler::~QWSKeyboardHandler()
{
    delete d;
}


/*!
    Sends a key event.

    This function is used by QWSKeyboardHandler subclasses to send a
    key event to the Qtopia Core server application. The server may
    additionally filter the event before passing it on to client
    applications.

    \table
    \header \o Parameter \o Meaning
    \row \o \a unicode
         \o The Unicode value for the key, or 0xFFFF is none is appropriate.
    \row \o \a keycode
         \o The Qt keycode for the key (see \l{Qt::Key} for the list of codes).
    \row \o \a modifiers
         \o The set of modifier keys (see \l{Qt::Modifier}).
    \row \o \a isPress
         \o Whether this is a press or a release.
    \row \o \a autoRepeat
         \o Whether this event was generated by an auto-repeat
            mechanism, or an actual key press.
    \endtable

    \sa beginAutoRepeat(), endAutoRepeat(), transformDirKey()
*/
void QWSKeyboardHandler::processKeyEvent(int unicode, int keycode, Qt::KeyboardModifiers modifiers,
                        bool isPress, bool autoRepeat)
{
    qwsServer->processKeyEvent(unicode, keycode, modifiers, isPress, autoRepeat);
}

/*!
    \fn int QWSKeyboardHandler::transformDirKey(int keycode)

    Transforms the arrow key specified by the given \a keycode, to the
    orientation of the display and returns the transformed keycode.

    The \a keycode is a Qt::Key value. The values identifying arrow
    keys are:

    \list
        \o Qt::Key_Left
        \o Qt::Key_Up
        \o Qt::Key_Right
        \o Qt::Key_Down
    \endlist

    \sa processKeyEvent()
 */
int QWSKeyboardHandler::transformDirKey(int key)
{
    static int dir_keyrot = -1;
    if (dir_keyrot < 0) {
        // get the rotation
        switch (qgetenv("QWS_CURSOR_ROTATION").toInt()) {
        case 90: dir_keyrot = 1; break;
        case 180: dir_keyrot = 2; break;
        case 270: dir_keyrot = 3; break;
        default: dir_keyrot = 0; break;
        }
    }
    int xf = qt_screen->transformOrientation() + dir_keyrot;
    return (key-Qt::Key_Left+xf)%4+Qt::Key_Left;
}

/*!
    \fn void QWSKeyboardHandler::beginAutoRepeat(int unicode, int keycode, Qt::KeyboardModifiers modifier)

    Begins auto repeating the specified key press: After a short delay
    the key press is sent periodically until the endAutoRepeat()
    function is called.

    The key press is specified by its \a unicode, \a keycode and \a
    modifier state.

    \sa endAutoRepeat(), processKeyEvent()
*/
void QWSKeyboardHandler::beginAutoRepeat(int uni, int code, Qt::KeyboardModifiers mod)
{
    d->beginAutoRepeat(uni, code, mod);
}

/*!
    Stops auto-repeating a key press.

    \sa beginAutoRepeat(), processKeyEvent()
*/
void QWSKeyboardHandler::endAutoRepeat()
{
    d->endAutoRepeat();
}

#include "qkbd_qws.moc"

#endif // QT_NO_QWS_KEYBOARD

