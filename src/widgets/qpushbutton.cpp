/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qpushbutton.cpp#123 $
**
** Implementation of QPushButton class
**
** Created : 940221
**
** Copyright (C) 1992-1998 Troll Tech AS.  All rights reserved.
**
** This file is part of Troll Tech's internal development tree for Qt.
**
** This header text will be replaced by an appropriate text by the
** mkdist script which generates external distributions.
**
** If you are using the Qt Professional Edition or the Qt Free Edition,
** please notify Troll Tech at <info@troll.no> if you see this text.
**
** To Troll Tech developers: This header was generated by the script
** fixcopyright-int. It has the same number of text lines as the free
** and professional editions to avoid line number inconsistency.
**
*****************************************************************************/

#include "qpushbutton.h"
#include "qdialog.h"
#include "qfontmetrics.h"
#include "qpainter.h"
#include "qdrawutil.h"
#include "qpixmap.h"
#include "qbitmap.h"

/*!
  \class QPushButton qpushbutton.h
  \brief The QPushButton widget provides a push button with a text
	    or pixmap label.

  \ingroup realwidgets

  A default push button in a dialog emits the clicked signal if the user
  presses the Enter key.

  A push button has \c TabFocus as a default focusPolicy(), i.e. it can
  get keyboard focus by tabbing but not by clicking.

  <img src="qpushbt-m.gif"> <img src="qpushbt-w.gif">

  \sa QRadioButton QToolButton
  <a href="guibooks.html#fowler">GUI Design Handbook: Push Button</a>
*/


/*!
  Constructs a push button with no text.

  The \e parent and \e name arguments are sent to the QWidget constructor.
*/

QPushButton::QPushButton( QWidget *parent, const char *name )
	: QButton( parent, name, WResizeNoErase )
{
    init();
}

/*!
  Constructs a push button with a text.

  The \e parent and \e name arguments are sent to the QWidget constructor.
*/

QPushButton::QPushButton( const QString &text, QWidget *parent,
			  const char *name )
	: QButton( parent, name, WResizeNoErase )
{
    init();
    setText( text );
}

void QPushButton::init()
{
    autoDefButton = defButton = lastDown = lastDef = lastEnabled
		  = hasMenuArrow = FALSE;
    setBackgroundMode( PaletteButton );
}


/*!
  Makes the push button a toggle button if \e enable is TRUE, or a normal
  push button if \e enable is FALSE.

  Toggle buttons have an on/off state similar to \link QCheckBox check
  boxes. \endlink A push button is initially not a toggle button.

  \sa setOn(), toggle(), isToggleButton() toggled()
*/

void QPushButton::setToggleButton( bool enable )
{
    QButton::setToggleButton( enable );
}


/*!
  Switches a toggle button on if \e enable is TRUE or off if \e enable is
  FALSE.
  \sa isOn(), toggle(), toggled(), isToggleButton()
*/

void QPushButton::setOn( bool enable )
{
    if ( !isToggleButton() )
	return;
    QButton::setOn( enable );
}


/*!
  Toggles the state of a toggle button.
  \sa isOn(), setOn(), toggled(), isToggleButton()
*/

void QPushButton::toggle()
{
    if ( !isToggleButton() )
	return;
    QButton::setOn( !isOn() );
}


/*!
  \fn bool QPushButton::autoDefault() const
  Returns TRUE if the button is an auto-default button.

  \sa setAutoDefault()
*/

/*!
  Sets the push buttons to an auto-default button if \e enable is TRUE,
  or to a normal button if \e enable is FALSE.

  An auto-default button becomes the default push button automatically
  when it receives the keyboard input focus.

  \sa autoDefault(), setDefault()
*/

void QPushButton::setAutoDefault( bool enable )
{
    autoDefButton = enable;
}


/*!
  \fn bool QPushButton::isDefault() const
  Returns TRUE if the button is default.

  \sa setDefault()
*/

/*!
  Sets the button to be the default button if \e enable is TRUE, or
  to be a normal button if \e enable is FALSE.

  A default push button in a \link QDialog dialog\endlink emits the
  QButton::clicked() signal if the user presses the Enter key.	Only
  one push button in the dialog can be default.

  Default push buttons are only allowed in dialogs.

  \sa isDefault(), setAutoDefault(), QDialog
*/

void QPushButton::setDefault( bool enable )
{
    if ( (defButton && enable) || !(defButton || enable) )
	return;					// no change
    QWidget *p = topLevelWidget();
    if ( !p->inherits("QDialog") )		// not a dialog
	return;
    defButton = enable;
    if ( defButton )
	((QDialog*)p)->setDefault( this );
    if ( isVisible() )
	repaint( FALSE );
}


/*!
  Returns a size which fits the contents of the push button.
*/

QSize QPushButton::sizeHint() const
{
    int w, h;
    if ( pixmap() ) {
	QPixmap *pm = (QPixmap *)pixmap();
	w = pm->width()	 + 6;
	h = pm->height() + 6;
    } else {
	QString s( text() );
	if ( s.isEmpty() )
	    s = "XXXX";
	QFontMetrics fm = fontMetrics();
	QSize sz = fm.size( ShowPrefix, s );
	w = sz.width()	+ 6;
	h = sz.height() + sz.height()/8 + 10;
	w += h;
    }
    if ( style() == WindowsStyle ) {
	// in windows style, try a little harder to conform to
	// microsoft's size specifications
	if ( h <= 25 )
	    h = 22;
	if ( w < 85 &&
	     topLevelWidget() &&
	     topLevelWidget()->inherits( "QDialog" ) )
	    w = 80;
    }

    return QSize( w, h );
}


/*!
  Reimplements QWidget::move() for internal purposes.
*/

void QPushButton::move( int x, int y )
{
    QWidget::move( x, y );
}

/*!
  Reimplements QWidget::move() for internal purposes.
*/

void QPushButton::move( const QPoint &p )
{
    move( p.x(), p.y() );
}

/*!
  Reimplements QWidget::resize() for internal purposes.
*/

void QPushButton::resize( int w, int h )
{
    QWidget::resize( w, h );
}

/*!
  Reimplements QWidget::resize() for internal purposes.
*/

void QPushButton::resize( const QSize &s )
{
    resize( s.width(), s.height() );
}

/*!
  Reimplements QWidget::setGeometry() for internal purposes.
*/

void QPushButton::setGeometry( int x, int y, int w, int h )
{
    QWidget::setGeometry( x, y, w, h );
}

/*!
  Reimplements QWidget::setGeometry() for internal purposes.
*/

void QPushButton::setGeometry( const QRect &r )
{
    QWidget::setGeometry( r );
}

void QPushButton::resizeEvent( QResizeEvent *e )
{
    updateResizedBorder( e, 2 );
    repaint( 2, 2, width()-4, height()-4 );
    if ( autoMask())
	updateMask();
}

/*!
  Draws the push button, except its label.
  \sa drawButtonLabel()
*/

void QPushButton::drawButton( QPainter *paint )
{
    style().drawPushButton(this, paint);
    drawButtonLabel( paint );
    int x1, y1, x2, y2;
    rect().coords( &x1, &y1, &x2, &y2 );	// get coordinates
     if ( hasFocus() ) {
 	QRect r(x1+3, y1+3, x2-x1-5, y2-y1-5);
 	style().drawFocusRect( paint, r , colorGroup() );
     }
    lastDown = isDown();
    lastDef = defButton;
    lastEnabled = isEnabled();
}


/*!
  Draws the push button label.
  \sa drawButton()
*/

void QPushButton::drawButtonLabel( QPainter *paint )
{
    style().drawPushButtonLabel( this, paint );
}


void QPushButton::updateMask()
{
    QBitmap bm( size() );
    bm.fill( QColor::color0 );

    {
	QPainter p( &bm, this );
	p.setPen( QColor::color1 );
	p.setBrush( QColor::color1 );
	style().drawButtonMask( &p, 0, 0, width(), height() );
    }
    setMask( bm );
}

/*!
  Handles focus in events for the push button.
*/

void QPushButton::focusInEvent( QFocusEvent *e )
{
    if ( autoDefButton )
	setDefault( TRUE );
    QButton::focusInEvent( e );
}



/*!  Tells this button to draw a menu indication triangle if \a enable
  is TRUE,  and to not draw one if \a enable is FALSE (the default).

  setIsMenuButton() does not cause the button to do anything other
  than draw the menu indication.

  \sa isMenuButton()
*/

void QPushButton::setIsMenuButton( bool enable )
{
    if ( (bool)hasMenuArrow == enable )
	return;
    hasMenuArrow = enable ? 1 : 0;
    repaint( FALSE );
}


/*!  Returns TRUE if this button indicates to the user that pressing
  it will pop up a menu, and FALSE otherwise.  The default is FALSE.

  \sa setIsMenuButton()
*/

bool QPushButton::isMenuButton() const
{
    return hasMenuArrow;
}
