/****************************************************************************
** $Id$
**
** Implementation of Qt/Embedded keyboard drivers
**
** Created : 991025
**
** Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses for Qt/Embedded may use this file in accordance with the
** Qt Embedded Commercial License Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qkbd_qws.h"

#ifndef QT_NO_QWS_KEYBOARD

#include "qwindowsystem_qws.h"
#include "qgfx_qws.h"
#include "qtimer.h"
#include <stdlib.h>


class QWSKbPrivate : public QObject
{
    Q_OBJECT
public:
    QWSKbPrivate( QWSKeyboardHandler * ) {
	arTimer = new QTimer( this );
	connect( arTimer, SIGNAL(timeout()), SLOT(autoRepeat()) );
	repeatdelay = 400;
	repeatperiod = 80;
    }

    void beginAutoRepeat( int uni, int code, int mod ) {
	unicode = uni;
	keycode = code;
	modifier = mod;
	arTimer->start( repeatperiod, TRUE );
    }
    void endAutoRepeat() {
	arTimer->stop();
    }

private slots:
    void autoRepeat() {
	handler->processKeyEvent( unicode, keycode, modifier, FALSE, TRUE );
	handler->processKeyEvent( unicode, keycode, modifier, TRUE, TRUE );
	arTimer->start( repeatdelay, TRUE );
    }

private:
    QWSKeyboardHandler *handler;
    int unicode;
    int keycode;
    int modifier;
    int repeatdelay;
    int repeatperiod;
    QTimer *arTimer;
};

/*!
    \class QWSKeyboardHandler qkbd_qws.h
    \brief The QWSKeyboardHandler class implements the keyboard driver
    for Qt/Embedded.

    \ingroup qws

    The keyboard driver handles events from system devices and
    generates key events.

    A QWSKeyboardHandler will usually open some system device in its
    constructor, create a QSocketNotifier on that opened device and
    when it receives data, it will call processKeyEvent() to send the
    event to Qt/Embedded for relaying to clients.
*/


/*!
    Constructs a keyboard handler. The handler \e may be passed to the
    system for later destruction with QWSServer::setKeyboardHandler(),
    although even without doing this, the handler can function,
    calling processKeyEvent() to emit events.
*/
QWSKeyboardHandler::QWSKeyboardHandler()
{
    d = new QWSKbPrivate( this );
}

/*!
    Destroys a keyboard handler. Note that if you have called
    QWSServer::setKeyboardHandler(), you must not delete the handler.
*/
QWSKeyboardHandler::~QWSKeyboardHandler()
{
    delete d;
}


/*!
    Subclasses call this function to send a key event. The server may
    additionally filter the event before sending it on to
    applications.

    \table
    \header \i Parameter \i Meaning
    \row \i \a unicode
	 \i The Unicode value for the key, or 0xFFFF is none is appropriate.
    \row \i \a keycode
	 \i The Qt keycode for the key (see \l{Qt::Key} for the list of codes).
    \row \i \a modifiers
	 \i The set of modifier keys (see \l{Qt::Modifier}).
    \row \i \a isPress
	 \i Whether this is a press or a release.
    \row \i \a autoRepeat
	 \i Whether this event was generated by an auto-repeat
	    mechanism, or an actual key press.
    \endtable
*/
void QWSKeyboardHandler::processKeyEvent(int unicode, int keycode, int modifiers,
			bool isPress, bool autoRepeat)
{
    qwsServer->processKeyEvent( unicode, keycode, modifiers, isPress, autoRepeat );
}

/*!
    Transforms an arrow key (\c Key_Left, \c Key_Up, \c Key_Right, \c
    Key_Down) to the orientation of the display.
 */
int QWSKeyboardHandler::transformDirKey(int key)
{
    static int dir_keyrot = -1;
    if (dir_keyrot < 0) {
	// get the rotation
	char *kerot = getenv("QWS_CURSOR_ROTATION");
	if (kerot) {
	    if (strcmp(kerot, "90") == 0)
		dir_keyrot = 1;
	    else if (strcmp(kerot, "180") == 0)
		dir_keyrot = 2;
	    else if (strcmp(kerot, "270") == 0)
		dir_keyrot = 3;
	    else
		dir_keyrot = 0;
	} else {
	    dir_keyrot = 0;
	}
    }
    int xf = qt_screen->transformOrientation() + dir_keyrot;
    return (key-Qt::Key_Left+xf)%4+Qt::Key_Left;
}

/*!
    Begin auto repeating the specified key press. After a short delay
    the key sequence will be sent periodically until endAutoRepeat()
    is called.

    \sa endAutoRepeat()
*/
void QWSKeyboardHandler::beginAutoRepeat( int uni, int code, int mod )
{
    d->beginAutoRepeat( uni, code, mod );
}

/*!
    Stop auto-repeating a key press.

    \sa beginAutoRepeat()
*/
void QWSKeyboardHandler::endAutoRepeat()
{
    d->endAutoRepeat();
}

#include "qkbd_qws.moc"

#endif // QT_NO_QWS_KEYBOARD

