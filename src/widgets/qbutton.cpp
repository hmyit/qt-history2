/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qbutton.cpp#65 $
**
** Implementation of QButton widget class
**
** Created : 940206
**
** Copyright (C) 1994-1996 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#include "qbutton.h"
#include "qbttngrp.h"
#include "qbitmap.h"
#include "qpainter.h"
#include "qkeycode.h"
#include "qtimer.h"

RCSTAG("$Id: //depot/qt/main/src/widgets/qbutton.cpp#65 $");

static const int autoRepeatPeriod = 200;

/*!
  \class QButton qbutton.h

  \brief The QButton class is the abstract base class of button
  widgets, and provides functionality common to buttons.

  \ingroup abstractwidgets

  The QButton class implements an abstract button, and lets subclasses
  specify how to reply to user actions and how to draw the button.

  The QButton class has three signals. The pressed() signal is emitted
  when the left mouse button is pressed while the cursor is inside the
  button. After being pressed, the button will be down until the left
  mouse button is again released, which causes a released() signal. If the
  left mouse button is released when the cursor is inside the button, the
  clicked() signal will be emitted.

  There are two types of buttons; standard buttons and toggle buttons. A
  standard button can either be pressed down or released. The QPushButton
  class is an example of a standard button. A toggle button has an
  additional flag that is toggled each time the button is clicked. The
  QRadioButton and QCheckBox classes are examples of toggle buttons.

  The button label can be a \link setText() text\endlink or a \link
  setPixmap() pixmap\endlink.  It is up to widget implementation to
  display the text or the pixmap.  All the button classess provided
  with Qt can show both texts and pixmaps.

  Another convention regarding QButtons is \link setAutoResize()
  auto-resizing\endlink.  Enabling auto-resizing makes a button resize
  itself whenever the contents change.

  \sa QButtonGroup
*/


/*!
  Constructs a standard button with a parent widget and a name.

  If \a parent is a QButtonGroup, this constructor calls
  QButtonGroup::insert().
*/

QButton::QButton( QWidget *parent, const char *name )
    : QWidget( parent, name )
{
    initMetaObject();
    bpixmap    = 0;
    toggleBt   = FALSE;				// button is not on/off
    buttonDown = FALSE;				// button is up
    buttonOn   = FALSE;				// button is off
    mlbDown    = FALSE;				// mouse left button up
    autoresize = FALSE;				// not auto resizing
    isTiming   = FALSE;				// not in keyboard mode
    repeat     = FALSE;				// not in autorepeat mode
    if ( parent && parent->inherits("QButtonGroup") ) {
	group = (QButtonGroup*)parent;
	group->insert( this );			// insert into button group
    } else {
	group = 0;
    }
    setFocusPolicy( TabFocus );
}

/*!
  Destroys the button and all its child widgets.
*/

QButton::~QButton()
{
    if ( group )				// remove from button group
	group->remove( this );
    delete bpixmap;
}


/*!
  \fn void QButton::pressed()
  This signal is emitted when the button is pressed down.

  \sa released(), clicked()
*/

/*!
  \fn void QButton::released()
  This signal is emitted when the button is released.

  \sa pressed(), clicked(), toggled()
*/

/*!
  \fn void QButton::clicked()
  This signal is emitted when the button is activated (i.e. first
  pressed down and then released when the mouse cursor is inside the
  button).

  \sa pressed(), released(), toggled()
*/

/*!
  \fn void QButton::toggled( bool on )
  This signal is emitted whenever a toggle button changes status.
  \e on is TRUE if the button is on, or FALSE if the button is off.

  This may be the result of a user action, toggle() slot activation,
  or because setOn() was called.

  \sa clicked()
*/


/*!
  \fn const char *QButton::text() const
  Returns the button text, or 0 if the button has no text.
  \sa setText()
*/

/*!
  Sets the button contents to \e text and redraws the contents.

  The button resizes itself if auto-resizing is enabled.

  \sa text(), setPixmap(), setAutoResize()
*/

void QButton::setText( const char *text )
{
    if ( btext == text )
	return;
    btext = text;
    if ( bpixmap ) {
	delete bpixmap;
	bpixmap = 0;
    }
    if ( autoresize )
	adjustSize();
    update();
}


/*!
  \fn const QPixmap *QButton::pixmap() const
  Returns the button pixmap, or 0 if the button has no pixmap.
*/

/*!
  Sets the button pixmap to \a pixmap and redraws the contents.

  If \a pixmap is monochrome (i.e. it is a QBitmap or its \link
  QPixmap::depth() depth\endlink is 1) and it does not have a mask, this
  function sets the pixmap to be its own mask. The purpose is to draw
  transparent bitmaps, which is important for e.g. toggle buttons.

  The button resizes itself if auto-resizing is enabled.

  \sa pixmap(), setText(), setAutoResize(), QPixmap::mask()
*/

void QButton::setPixmap( const QPixmap &pixmap )
{
    int  w, h;
    if ( bpixmap ) {
	w = bpixmap->width();
	h = bpixmap->height();
    } else {
	bpixmap = new QPixmap;
	CHECK_PTR( bpixmap );
	w = h = -1;
    }
    bool sameSize = w == bpixmap->width() && h == bpixmap->height();
    *bpixmap = pixmap;
    if ( bpixmap->depth() == 1 && !bpixmap->mask() )
	bpixmap->setMask( *((QBitmap *)bpixmap) );
    if ( !btext.isNull() )
	btext.resize( 0 );
    if ( autoresize && !sameSize )
	adjustSize();
    repaint( FALSE );
}


/*!
  \fn bool QButton::autoResize() const
  Returns TRUE if auto-resizing is enabled, or FALSE if auto-resizing is
  disabled.

  Auto-resizing is disabled by default.

  \sa setAutoResize()
*/


/*!
  Enables auto-resizing if \e enable is TRUE, or disables it if \e enable is
  FALSE.

  When auto-resizing is enabled, the button will resize itself whenever
  the contents change.

  \sa autoResize(), adjustSize()
*/

void QButton::setAutoResize( bool enable )
{
    if ( (bool)autoresize != enable ) {
	autoresize = enable;
	if ( autoresize )
	    adjustSize();			// calls resize which repaints
    }
}


/*! \fn bool autoRepeat() const

  Returns TRUE if the button is auto-repeating, else FALSE.

  The default is FALSE.

  \sa setAutoRepeat()

*/


/*!
  Turns on auto-repeat for the button if \a enable is TRUE, or
  turns it off if \a enable is FALSE.

  When auto-repeat is enabled, the clicked() signal is emitted at
  regular intervals while the buttons \link isDown() is down. \endlink

  setAutoRepeat() has no effect for \link setToggleButton() toggle
  buttons. \endlink

  \sa isDown(), autoRepeat(), clicked()
*/

void QButton::setAutoRepeat( bool enable )
{
    repeat = (uint)enable;
    if ( repeat && mlbDown )
	QTimer::singleShot( autoRepeatPeriod, this, SLOT(autoRepeatSlot()) );
}


/*!
  \fn bool QButton::isDown() const
  Returns TRUE if the button pressed down, or FALSE if it is standing up.
  \sa setDown()
*/

/*!
  Sets the state of the button to pressed down if \e enable is TRUE
  or to standing up if \e enable is FALSE.

  If the button is a toggle button, it is \e not toggled.
  Call toggle() as well if you need to do that.
  The pressed() and released() signals are not emitted by this function.

  This method is provided in case you need to override the mouse event
  handlers.

  \sa isDown(), setOn(), toggle(), toggled()
*/

void QButton::setDown( bool enable )
{
    mlbDown = FALSE; // the safe setting
    if ( (bool)buttonDown != enable ) {
	buttonDown = enable;
	repaint( FALSE );
    }
}


/*!
  \fn bool QButton::isOn() const
  Returns TRUE if this toggle button is switched on, or FALSE if it is
  switched off.
  \sa setOn(), toggleButton()
*/

/*!
  Switches a toggle button on if \e enable is TRUE or off if \e enable is
  FALSE.  This function should be called only for toggle buttons.
  \sa isOn(), toggleButton()
*/

void QButton::setOn( bool enable )
{
#if defined(CHECK_STATE)
    if ( !toggleBt )
	warning( "QButton::setOn: Only toggle buttons may be switched" );
#endif
    if ( (bool)buttonOn != enable ) {		// changed state
	buttonOn = enable;
	repaint( FALSE );
	emit toggled( buttonOn );
    }
}


/*!
  \fn bool QButton::isToggleButton() const
  Returns TRUE if the button is a toggle button.
  \sa setToggleButton()
*/

/*!
  Makes the button a toggle button if \e enable is TRUE, or a normal button
  if \e enable is FALSE.

  Note that this function is protected. It is called from subclasses
  to enable the toggle functionality. QCheckBox and QRadioButton are
  toggle buttons. QPushButton is initially not a toggle button, but
  QPushButton::setToggleButton() can be called to create toggle buttons.

  \sa isToggleButton()
*/

void QButton::setToggleButton( bool enable )
{
    toggleBt = enable;
}


/*!
  Returns TRUE if \e pos is inside the widget rectangle, or FALSE if it
  is outside.

  This virtual function is reimplemented by subclasses.
*/

bool QButton::hitButton( const QPoint &pos ) const
{
    return rect().contains( pos );
}

/*!
  Draws the button.  The default implementation does nothing.

  This virtual function is reimplemented by subclasses to draw real buttons.
*/

void QButton::drawButton( QPainter * )
{
    return;
}

/*!
  Draws the button text or pixmap.  The default implementation does nothing.

  This virtual function is reimplemented by subclasses to draw real buttons.
*/

void QButton::drawButtonLabel( QPainter * )
{
    return;
}


/*!
  Handles mouse press events for the button.
  \sa mouseReleaseEvent()
*/

void QButton::mousePressEvent( QMouseEvent *e )
{
    if ( e->button() != LeftButton || mlbDown )
	return;
    bool hit = hitButton( e->pos() );
    if ( hit ) {				// mouse press on button
	mlbDown = TRUE;				// left mouse button down
	buttonDown = TRUE;
	repaint( FALSE );
	emit pressed();
	if ( repeat )
	    QTimer::singleShot( autoRepeatPeriod,
				this, SLOT(autoRepeatSlot()) );
    }
}

/*!
  Handles mouse release events for the button.
  \sa mousePressEvent()
*/

void QButton::mouseReleaseEvent( QMouseEvent *e)
{
    if ( e->button() != LeftButton || !mlbDown )
	return;
    mlbDown = FALSE;				// left mouse button up
    bool hit = hitButton( e->pos() );
    buttonDown = FALSE;
    if ( hit ) {				// mouse release on button
	if ( toggleBt )
	    buttonOn = !buttonOn;
	repaint( FALSE );
	if ( toggleBt )
	    emit toggled( buttonOn );
	emit released();
	emit clicked();
    } else {
	repaint( FALSE );
	emit released();
    }
}

/*!
  Handles mouse move events for the button.
  \sa mousePressEvent(), mouseReleaseEvent()
*/

void QButton::mouseMoveEvent( QMouseEvent *e )
{
    if ( !((e->state() & LeftButton) && mlbDown) )
	return;					// left mouse button is up
    bool hit = hitButton( e->pos() );
    if ( hit ) {				// mouse move in button
	if ( !buttonDown ) {
	    buttonDown = TRUE;
	    repaint( FALSE );
	    emit pressed();
	}
    } else {					// mouse move outside button
	if ( buttonDown ) {
	    buttonDown = FALSE;
	    repaint( FALSE );
	    emit released();
	}
    }
}

/*!
  Handles paint events for the button.

  Opens the painter on the button and calls drawButton().
*/

void QButton::paintEvent( QPaintEvent * )
{
    QPainter paint;
    paint.begin( this );
    drawButton( &paint );
    paint.end();
}


/*!
  Handles focus in events for the button.
  \sa focusOutEvent()
*/

void QButton::focusInEvent( QFocusEvent * )
{
    repaint( FALSE );
}

/*!
  Handles focus out events for the button.
  \sa focusInEvent()
*/

void QButton::focusOutEvent( QFocusEvent * )
{
    repaint( FALSE );
}


/*!
  Handles keyboard events for the button.

  Space is the only key that has any effect; it emulates a
  mousePressEvent() followed by a mouseReleaseEvent().
*/

void QButton::keyPressEvent( QKeyEvent *e ) 
{
    if ( e->key() == Key_Space ) {
	QTimer *timer;
	if ( isTiming ) {
	    timer = CHILD(this,QTimer,"timer");
	    if ( timer ) {
		timer->stop();
		timer->start( autoRepeatPeriod, TRUE );
	    }
	    return;
	} else {
	    isTiming = TRUE;
	    timer = new QTimer( this, "timer" );
	    CHECK_PTR( timer );
	    connect( timer, SIGNAL(timeout()), SLOT(timerSlot()) );
	    timer->start( autoRepeatPeriod, TRUE );
	}
	buttonDown = TRUE;
	repaint( FALSE );
	emit pressed();
    } else {
	e->ignore();
    }
}


/*!
  Acts like a mouseReleaseEvent during keyboard interaction.
*/

void QButton::timerSlot()
{
    if ( !isTiming )
	return;
    isTiming   = FALSE;
    buttonDown = FALSE;
    QTimer *timer = CHILD(this,QTimer,"timer");
    delete timer;
    if ( toggleBt )
	buttonOn = !buttonOn;
    repaint( FALSE );
    if ( toggleBt )
	emit toggled( buttonOn );
    emit released();
    emit clicked();
}


/*!
  Emits a clicked() signal if autoRepeat() is on.
*/

void QButton::autoRepeatSlot()
{
    if ( mlbDown && isEnabled() && autoRepeat() ) {
	QTimer::singleShot( autoRepeatPeriod, this, SLOT(autoRepeatSlot()) );
	emit clicked();
    }
}




/*!
  Reimplemented for implementational reasons.
*/

void QButton::enabledChange( bool e )
{
    if ( !e )
	setDown( FALSE );
    QWidget::enabledChange( e );
}
