/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qtoolbutton.cpp#18 $
**
** Implementation of QToolButton class
**
** Created : 980320
**
** Copyright (C) 1997 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#include "qtoolbutton.h"

#include "qdrawutl.h"
#include "qpainter.h"
#include "qpixmap.h"
#include "qwmatrix.h"
#include "qapp.h"
#include "qtooltip.h"
#include "qtoolbar.h"
#include "qimage.h"
#include "qiconset.h"


RCSTAG("$Id: //depot/qt/main/src/widgets/qtoolbutton.cpp#18 $");


static QToolButton * threeDeeButton = 0;


class QToolButtonPrivate
{
};


/*! \class QToolButton qtoolbutton.h

  \brief The QToolButton class provides a push button whose appearance
  has been tailored for use in a QToolBar.

  \ingroup realwidgets

  This means that it implements the ridiculous Microsoft auto-raise
  feature.  And it isn't finished, either.
*/


/*!  Constructs an empty tool button. */

QToolButton::QToolButton( QWidget * parent, const char * name )
    : QButton( parent, name )
{
    init();
    setUsesBigPixmap( FALSE );
}


/*!  Set-up code common to all the constructors */

void QToolButton::init()
{
    d = 0;
    bpID = bp.serialNumber();
    spID = sp.serialNumber();

    utl = FALSE;
    ubp = TRUE;

    s = 0;
}


/*!  Creates a tool button that is a child of \a parent (which must be
  a QToolBar) and named \a name.

  The tool button will display \a pm, with text label or tool tip \a
  textLabel and status-bar message \a grouptext, connected to \a slot
  in object \a receiver, and returns the button.

  Note that \a grouptext is not used unless \a parent is managed by a
  QMainWindow.
*/

QToolButton::QToolButton( const QPixmap & pm, const char * textLabel,
			  const char * grouptext,
			  QObject * receiver, const char * slot,
			  QToolBar * parent, const char * name )
    : QButton( parent, name )
{
    init();
    setPixmap( pm );
    setTextLabel( textLabel );
    setUsesBigPixmap( FALSE );
    connect( this, SIGNAL(clicked()), receiver, slot );
    /*    connect( parent, SIGNAL(useBigPixmaps(bool)),
	     this, SLOT(setUsesBigPixmap(bool)) );
	     */
    if ( grouptext && *grouptext )
	warning( "QToolButton::QToolButton: (%s) Not using grouptext \"%s\"",
		 name, grouptext );
}


/*! Destroys the object and frees any allocated resources. */

QToolButton::~QToolButton()
{
    delete d;
    threeDeeButton = 0;
}


/*!
  Makes the tool button a toggle button if \e enable is TRUE, or a normal
  tool button if \e enable is FALSE.

  Toggle buttons have an on/off state similar to \link QCheckBox check
  boxes. \endlink A tool button is initially not a toggle button.

  \sa setOn(), toggle(), toggleButton() toggled()
*/

void QToolButton::setToggleButton( bool enable )
{
    QButton::setToggleButton( enable );
}


/*!  Returns a size suitable for this tool button.  This depends on
  \link style() GUI style,\endlink usesBigPixmap(), textLabel() and
  usesTextLabel().
*/

QSize QToolButton::sizeHint() const
{
    int w, h;

    if ( text() ) {
	w = fontMetrics().width( text() );
	h = fontMetrics().height(); // boundingRect()?
    } else if ( usesBigPixmap() ) {
	w = h = 32;
    } else {
	w = h = 16;
    }

    if ( usesTextLabel() ) {
	h += 4 + fontMetrics().height();
	int tw = fontMetrics().width( textLabel() );
	if ( tw > w )
	    w = tw;
    }
    return QSize( w + 7, h + 6 );
}



/* \fn bool QToolButton::usesBigPixmap() const

  Returns TRUE or FALSE.

*/


/* \fn bool QToolButton::usesTextLabel() const

  Returns TRUE or FALSE.

*/


/*! \fn const char * QToolButton::textLabel() const

  Returns the text label in use by this tool button, or 0.

  \sa setTextLabel() usesTextLabel() setUsesTextLabel() setText()
*/

/*!

*/

void QToolButton::setUsesBigPixmap( bool enable )
{
    if ( (bool)ubp == enable )
	return;

    ubp = enable;

    if ( parent() )
	QApplication::postEvent( parent(), new QEvent( Event_LayoutHint ) );
}


/*!  \fn bool QToolButton::usesBigPixmap() const

  Returns TRUE if this tool button uses the big (32-pixel) pixmaps,
  and FALSE if it does not.  \sa setUsesBigPixmap(), setPixmap(),
  usesTextLabel
*/


/*!

*/

void QToolButton::setUsesTextLabel( bool enable )
{
    if ( (bool)utl == enable )
	return;

    utl = enable;

    if ( parent() )
	QApplication::postEvent( parent(), new QEvent( Event_LayoutHint ) );
}


/*! \fn bool QToolButton::usesTextLabel() const

  Returns TRUE if this tool button puts a text label below the button
  pixmap, and FALSE if it does not. \sa setUsesTextLabel()
  setTextLabel() usesBigPixmap()
*/


/*!  Sets this tool button to be on if \a enable is TRUE, and off it
  \a enable is FALSE.

  This function has no effect on \link isToggleButton() non-toggling
  buttons. \endlink

  \sa isToggleButton() toggle()
*/

void QToolButton::setOn( bool enable )
{
    if ( !isToggleButton() )
	return;
    QButton::setOn( enable );
}


/*!  Toggles the state of this tool button.

  This function has no effect on \link isToggleButton() non-toggling
  buttons. \endlink

  \sa isToggleButton() toggle()
*/

void QToolButton::toggle()
{
    if ( !isToggleButton() )
	return;
    QButton::setOn( !isOn() );
}


/*!  Draws the edges and decoration of the button (pretty much
  nothing) and calls drawButtonLabel().

  \sa drawButtonLabel() QButton::paintEvent() */

void QToolButton::drawButton( QPainter * p )
{
    if ( uses3D() || isOn() ) {
	QPointArray a;
	a.setPoints( 3, 0, height()-1, 0, 0, width()-1, 0 );
	if ( isOn() && !isDown() && !uses3D() ) {
	    if ( style() == WindowsStyle ) {
		p->setBrush( QBrush(white,Dense4Pattern) );
		p->setPen( NoPen );
		p->setBackgroundMode( OpaqueMode );
		p->drawRect( 0,0, width(),height() );
		p->setBackgroundMode( TransparentMode );
	    } else {
		p->setBrush( colorGroup().mid() );
		p->setPen( NoPen );
		p->drawRect( 0,0, width(),height() );
	    }
	}
	p->setPen( isDown() || isOn()
		   ? colorGroup().dark()
		   : colorGroup().light() );
	p->drawPolyline( a );
	a[1] = QPoint( width()-1, height()-1 );
	p->setPen( isDown() || isOn()
		   ? colorGroup().light()
		   : colorGroup().dark() );
	p->drawPolyline( a );
    }
    drawButtonLabel( p );
}


/*!  Draws the contents of the button (pixmap and optionally text).

  \sa drawButton() QButton::paintEvent() */

void QToolButton::drawButtonLabel( QPainter * p )
{
    if ( text() ) {
	qDrawItem( p, style(), 1, 1, width()-2, height()-2,
		   AlignCenter + ShowPrefix,
		   colorGroup(), isEnabled(),
		   0, text() );
    } else {
	int x, y, fh;
	fh = fontMetrics().height();
	x = width()/2 - (usesBigPixmap() ? 12 : 8);
	y = height()/2 - (usesBigPixmap() ? 12 : 8);
	if ( usesTextLabel() )
	    y = y - fh/2 - 2;

	QPixmap pm;
	if ( usesBigPixmap() ) {
	    if ( !isEnabled() )
		pm = iconSet().pixmap( QIconSet::Large, QIconSet::Disabled );
	    else if ( uses3D() )
		pm = iconSet().pixmap( QIconSet::Large, QIconSet::Active );
	    else
		pm = iconSet().pixmap( QIconSet::Large, QIconSet::Normal );
	} else {
	    if ( !isEnabled() )
		pm = iconSet().pixmap( QIconSet::Small, QIconSet::Disabled );
	    else if ( uses3D() )
		pm = iconSet().pixmap( QIconSet::Small, QIconSet::Active );
	    else
		pm = iconSet().pixmap( QIconSet::Small, QIconSet::Normal );
	}

	qDrawItem( p, style(), x, y,
		   usesBigPixmap() ? 24 : 16, usesBigPixmap() ? 24 : 16,
		   AlignCenter, colorGroup(), TRUE, &pm, 0 );

	if ( usesTextLabel() ) {
	    y += (usesBigPixmap() ? 32 : 16) + 4;
	    p->setFont( font() );
	    qDrawItem( p, style(), 3, y, width()-6, fh,
		       AlignCenter + ShowPrefix,
		       colorGroup(), isEnabled(),
		       0, textLabel() );
	}
    }
}


/*! Reimplemented to handle the automatic 3D effects in Windows style. */

void QToolButton::enterEvent( QEvent * e )
{
    threeDeeButton = this;
    if ( isEnabled() )
	repaint();
    QButton::enterEvent( e );
}


/*! Reimplemented to handle the automatic 3D effects in Windows style. */

void QToolButton::leaveEvent( QEvent * e )
{
    QToolButton * o = threeDeeButton;
    threeDeeButton = 0;
    if ( o && o->isEnabled() )
	o->repaint();
    QButton::leaveEvent( e );
}



/*!  Returns TRUE if this button should be drawn using raised edges.
  \sa drawButton() */

bool QToolButton::uses3D() const
{
    return threeDeeButton == this && isEnabled();
}


/*!  Sets the label of this button to \a newLabel, and automatically
  sets it as tool tip too if \a tipToo is TRUE.
*/

void QToolButton::setTextLabel( const char * newLabel , bool tipToo )
{
    tl = newLabel;
    if ( !tipToo )
	return;

    if ( usesTextLabel() )
	QToolTip::remove( this );
    else
	QToolTip::add( this, newLabel );
}





/*!  Sets this tool button to display the icons in \a set.
  (setPixmap() is effectively a wrapper for this function.)

  QToolButton makes a copy of \a set, so you must delete \a set
  yourself.

  \sa iconSet() QIconSet
*/

void QToolButton::setIconSet( const QIconSet & set )
{
    if ( s )
	delete s;
    s = new QIconSet( set );
}


/*!  Returns a copy of the icon set in use.  If no icon set has been
  set, iconSeT() creates one from the pixmap().

  If the button doesn't have a pixmap either, iconSet()'s return value
  is meaningless.

  \sa setIconSet() QIconSet
*/

QIconSet QToolButton::iconSet() const
{
    QToolButton * that = (QToolButton *)this;

    if ( pixmap() && (!that->s || (that->s->pixmap().serialNumber() !=
				   pixmap()->serialNumber())) )
	that->setIconSet( *pixmap() );

    if ( that->s )
	return *that->s;

    QPixmap tmp1;
    QIconSet tmp2( tmp1, QIconSet::Small );
    return tmp2;
}
