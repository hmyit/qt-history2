/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qpushbt.cpp#26 $
**
** Implementation of QPushButton class
**
** Author  : Haavard Nord
** Created : 940221
**
** Copyright (C) 1994,1995 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#include "qpushbt.h"
#include "qfontmet.h"
#include "qpainter.h"
#include "qpixmap.h"
#include "qpmcache.h"

#if defined(DEBUG)
static char ident[] = "$Id: //depot/qt/main/src/widgets/qpushbt.cpp#26 $";
#endif


/*!
\class QPushButton qpushbt.h
\brief The QPushButton widget provides a push button with a text label.

\todo Describe default buttons etc.
*/


const int extraMacWidth = 6;			// extra size for def button
const int extraMacHeight = 6;
const int extraPMWidth = 2;
const int extraPMHeight = 2;
const int extraMotifWidth = 10;
const int extraMotifHeight = 10;


static bool extraSize( const QPushButton *b, int &wx, int &hx,
		       bool onlyWhenDefault )
{
    if ( onlyWhenDefault && !b->isDefault() ) {
	wx = hx = 0;
	return FALSE;
    }
    switch ( b->style() ) {
	case MacStyle:				// larger def Mac buttons
	    wx = extraMacWidth;
	    hx = extraMacHeight;
	    break;
	case MotifStyle:			// larger def Motif buttons
	    wx = extraMotifWidth;
	    hx = extraMotifHeight;
	    break;
	default:
	    wx = hx = 0;
	    return FALSE;
    }
    return TRUE;
}

static void resizeDefButton( QPushButton *b )
{
    int wx, hx;
    if ( !extraSize( b, wx, hx, FALSE ) )
	return;
    if ( !b->isDefault() ) {			// not default -> shrink
	wx = -wx;
	hx = -hx;
    }
    QRect r = b->geometry();
    b->QWidget::setGeometry( r.x()-wx/2, r.y()-hx/2,
			     r.width()+wx, r.height()+hx );
}


/*!
Constructs a push button with no text.

The \e parent and \e name arguments are sent to the QWidget constructor.
*/

QPushButton::QPushButton( QWidget *parent, const char *name )
	: QButton( parent, name )
{
    init();
}

/*!
Constructs a push button with a text.

The \e parent and \e name arguments are sent to the QWidget constructor.
*/

QPushButton::QPushButton( const char *text, QWidget *parent,
			  const char *name )
	: QButton( parent, name )
{
    init();
    setText( text );
}

void QPushButton::init()
{
    initMetaObject();
    autoDefButton = defButton = lastDown = lastDef = FALSE;
}


/*!
\fn bool QPushButton::autoDefault() const
Returns TRUE if the button is an auto-default button.

\sa setAutoDefault().
*/

/*!
Sets the push buttons to an auto-default button if \e enable is TRUE,
or to a normal button if \e enable is FALSE.

An auto default button becomes the default push button automatically
when it gets the keyboard focus.

\sa autoDefault() and setDefault().
*/

void QPushButton::setAutoDefault( bool enable )
{
    autoDefButton = enable;
}


/*!
\fn bool QPushButton::isDefault() const
Returns TRUE if the button is default.

\sa setDefault().
*/

/*!
Sets the button to be the default button if \e enable is TRUE, or
to be a normal button if \e enable is FALSE.

\sa default().
*/

void QPushButton::setDefault( bool enable )
{
    if ( (defButton && enable) || !(defButton || enable) )
	return;					// no change
    defButton = enable;
    if ( defButton )
	emit becameDefault();
    int gs = style();
    if ( gs != MacStyle && gs != MotifStyle ) {
	if ( isVisible() )
	    paintEvent( 0 );
    }
    else
	resizeDefButton( (QPushButton*)this );
}


/*!
Adjusts the size of the push button to fit the contents.

This function is called automatically whenever the contents change and
auto-resizing is enabled.

\sa setAutoResizing()
*/

void QPushButton::adjustSize()
{
    QFontMetrics fm = fontMetrics();
    QRect br = fm.boundingRect( text() );
    int w = br.width()  + 6;
    int h = br.height() + 6;
    resize( w + w/8 + 16, h + h/8 + 4 );
}


void QPushButton::move( int x, int y )
{
    int wx, hx;
    extraSize( this, wx, hx, TRUE );
    QWidget::move( x-wx/2, y-hx/2 );
}

void QPushButton::move( const QPoint &p )
{
    move( p.x(), p.y() );
}

void QPushButton::resize( int w, int h )
{
    int wx, hx;
    extraSize( this, wx, hx, TRUE );
    QWidget::resize( w+wx, h+hx );
}

void QPushButton::resize( const QSize &s )
{
    resize( s.width(), s.height() );
}

void QPushButton::setGeometry( int x, int y, int w, int h )
{
    int wx, hx;
    extraSize( this, wx, hx, TRUE );
    QWidget::setGeometry( x-wx/2, y-hx/2, w+wx, h+hx );
}

void QPushButton::setGeometry( const QRect &r )
{
    setGeometry( r.x(), r.y(), r.width(), r.height() );
}


/*!
Draws the button, but not the button face.

\sa drawButtonFace().
*/

void QPushButton::drawButton( QPainter *paint )
{
    register QPainter *p = paint;
    GUIStyle 	gs = style();
    QColorGroup g  = colorGroup();
    bool 	updated = isDown() != lastDown || lastDef != defButton;
    QColor	fillcol = g.background();
    int 	x1, y1, x2, y2;

    rect().coords( &x1, &y1, &x2, &y2 );	// get coordinates

#define SAVE_PUSHBUTTON_PIXMAPS
#if defined(SAVE_PUSHBUTTON_PIXMAPS)
    QString pmkey;				// pixmap key
    int w, h;
    w = x2 + 1;
    h = y2 + 1;
    pmkey.sprintf( "$qt_push_%d_%d_%d_%d_%d_%d", gs, palette().serialNumber(),
		   isDown(), defButton, w, h );
    QPixmap *pm = QPixmapCache::find( pmkey );
    if ( pm ) {					// pixmap exists
	p->drawPixmap( 0, 0, *pm );
	drawButtonFace( p );
	lastDown = isDown();
	lastDef = defButton;
	return;
    }
    bool use_pm = !isDown();
    QPainter pmpaint;
    if ( use_pm ) {
	pm = new QPixmap( w, h );		// create new pixmap
	CHECK_PTR( pm );
	pmpaint.begin( pm );
	p = &pmpaint;				// draw in pixmap
	p->setBackgroundColor( fillcol );
	p->eraseRect( 0, 0, w, h );
    }
#endif

    p->setPen( g.foreground() );
    p->setBrush( QBrush(fillcol,NoBrush) );
    if ( gs == MacStyle ) {			// Macintosh push button
	if ( defButton ) {
	    p->pen().setWidth( 3 );
	    x1++; y1++; x2--; y2--;
	    p->drawRoundRect( x1, y1, x2-x1+1, y2-y1+1, 25, 25 );
	    x1 += extraMacWidth/2;
	    y1 += extraMacHeight/2;
	    x2 -= extraMacWidth/2;
	    y2 -= extraMacHeight/2;
	    p->pen().setWidth( 0 );
	}
	if ( updated ) {			// fill
	    p->brush().setStyle( SolidPattern );
	    if ( isDown() )
		p->brush().setColor( g.foreground() );
	}
	p->drawRoundRect( x1, y1, x2-x1+1, y2-y1+1, 20, 20 );
    }
    else if ( gs == WindowsStyle ) {		// Windows push button
	QPointArray a;
	a.setPoints( 8, x1+1,y1, x2-1,y1, x1+1,y2, x2-1,y2,
		        x1,y1+1, x1,y2-1, x2,y1+1, x2,y2-1 );
	p->drawLineSegments( a );		// draw frame
	x1++; y1++;
	x2--; y2--;
	if ( defButton || (autoDefButton & isDown()) ) {
	    p->drawRect( x1, y1, x2-x1+1, y2-y1+1 );
	    x1++; y1++;
	    x2--; y2--;
	}
	if ( isDown() )
	    p->drawShadePanel( x1, y1, x2-x1+1, y2-y1+1, g.dark(), g.light(),
			       1, fillcol, updated );
	else
	    p->drawShadePanel( x1, y1, x2-x1+1, y2-y1+1, g.light(), g.dark(),
			       2, fillcol, updated );
    }
    else if ( gs == PMStyle ) {			// PM push button
	p->pen().setColor( g.dark() );
	if ( updated )				// fill
	    p->brush().setStyle( SolidPattern );
	p->drawRect( x1, y1, x2-x1+1, y2-y1+1 );
	if ( !defButton ) {
	    p->pen().setColor( g.background() );
	    p->drawPoint( x1, y1 );
	    p->drawPoint( x1, y2 );
	    p->drawPoint( x2, y1 );
	    p->drawPoint( x2, y2 );
	    p->pen().setColor( g.dark() );
	}
	x1++; y1++;
	x2--; y2--;
	QPointArray atop, abottom;
	atop.setPoints( 3, x1,y2-1, x1,y1, x2,y1 );
	abottom.setPoints( 3, x1,y2, x2,y2, x2,y1+1 );
	QColor tc, bc;
	if ( isDown() ) {
	    tc = g.dark();
	    bc = g.light();
	}
	else {
	    tc = g.light();
	    bc = g.dark();
	}
	p->pen().setColor( tc );
	p->drawPolyline( atop );
	p->pen().setColor( bc );
	p->drawPolyline( abottom );
    }
    else if ( gs == MotifStyle ) {		// Motif push button
	QColor tColor, bColor;
	if ( defButton ) {			// default Motif button
	    p->drawShadePanel( x1, y1, x2-x1+1, y2-y1+1, g.dark(), g.light() );
	    x1 += extraMotifWidth/2;
	    y1 += extraMotifHeight/2;
	    x2 -= extraMotifWidth/2;
	    y2 -= extraMotifHeight/2;
	}
	if ( isDown() ) {
	    tColor  = g.dark();
	    bColor  = g.light();
	    fillcol = g.mid();
	}
	else {
	    tColor = g.light();
	    bColor = g.dark();
	}
	p->drawShadePanel( x1, y1, x2-x1+1, y2-y1+1, tColor, bColor,
			   2, fillcol, updated );
    }
    if ( p->brush().style() != NoBrush )
	p->brush().setStyle( NoBrush );

#if defined(SAVE_PUSHBUTTON_PIXMAPS)
    if ( use_pm ) {
	pmpaint.end();
	p = paint;				// draw in default device
	p->drawPixmap( 0, 0, *pm );
	QPixmapCache::insert( pmkey, pm );	// save for later use
    }
#endif
    drawButtonFace( p );
    lastDown = isDown();
    lastDef = defButton;
}


/*!
Draws the button face.  The default implementation draws the button text.

This virtual function can be reimplemented by subclasses.
*/

void QPushButton::drawButtonFace( QPainter *paint )
{
    if ( !text() )
	return;
    register QPainter *p = paint;    
    GUIStyle    gs = style();
    QColorGroup g  = colorGroup();
    int dt;
    switch ( gs ) {
	case MacStyle:
	    p->pen().setColor( isDown() ? white : g.text() );
	    dt = 0;
	    break;
	case WindowsStyle:
	case PMStyle:
	case MotifStyle:
	    p->pen().setColor( g.text() );
	    dt = gs == WindowsStyle ? 2 : 0;
	    break;
    }
    QRect r = rect();
    int x, y, w, h;
    r.rect( &x, &y, &w, &h );
    if ( isDown() || isOn() ) {			// shift text
	x += dt;
	y += dt;
    }
    p->drawText( x+2, y+2, w-4, h-4,
		 AlignCenter|AlignVCenter|SingleLine|ShowPrefix, text() );
}
