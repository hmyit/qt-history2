/**********************************************************************
** $Id: //depot/qt/main/src/widgets/qlabel.cpp#91 $
**
** Implementation of QLabel widget class
**
** Created : 941215
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

#include "qlabel.h"
#include "qbitmap.h"
#include "qpainter.h"
#include "qdrawutil.h"
#include "qaccel.h"
#include "qkeycode.h"
#include "qmovie.h"
#include <ctype.h>
#include "qimage.h"
#include "qbitmap.h"

class QLabelExtra
{
public:
    QLabelExtra(QLabel* l) :
	buddy(0),
	accel(new QAccel( l, "label accel" )),
	movie(0)
    { }

    QWidget * buddy;
    QAccel * accel;  // NON NULL
    QMovie * movie;
};

/*!
  \class QLabel qlabel.h
  \brief The QLabel widget displays a static text or pixmap.

  \ingroup realwidgets

  A label is a static text or pixmap field.

  It can have a frame (since QLabel inherits QFrame) and a "buddy" and
  an accelerator for moving keyboard focus to the buddy.

  The contents of a label can be specified as a normal text, as a
  numeric value (which is internally converted to a text) or, as a
  pixmap.  If the label is normal text and one of the letters is
  prefixed with '&', you can also specify a \e buddy for the label:

  \code
     QLineEdit * phone = new QLineEdit( this, "phone number" );
     QLabel * phoneLabel = new QLabel( phone, "&Phone", this );
  \endcode

  In this example, keyboard focus is transferred to the label's buddy
  (the QLineEdit) when the user presses <dfn>Alt-P.</dfn> This is
  handy for many dialogs.  You can also use the setBuddy() function to
  accomplish the same means.

  A label can be aligned in many different ways. The alignment setting
  specifies where to position the contents relative to the frame
  rectangle.  See setAlignment() for a description of the alignment
  flags.

  Enabling auto-resizing make the label resize itself whenever the
  contents change.  The top left corner does not move.

  This code sets up a sunken panel with a two-line text in the bottom
  right corner:

  \code
    QLabel *label = new QLabel;
    label->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    label->setText( "first line\nsecond line" );
    label->setAlignment( AlignBottom | AlignRight );
  \endcode

  Both lines are flush with the right side of the label.

  <img src=qlabel-m.gif> <img src=qlabel-w.gif>

  \sa QLineEdit QMovie
  <a href="guibooks.html#fowler">GUI Design Handbook: Label</a>
*/


/*!
  Constructs an empty label which is left-aligned, vertically centered,
  has an automatic margin and with manual resizing.

  The \e parent, \e name and \e f arguments are passed to the QFrame
  constructor.

  \sa setAlignment(), setFrameStyle(), setMargin(), setAutoResize()
*/

QLabel::QLabel( QWidget *parent, const char *name, WFlags f )
    : QFrame( parent, name, f )
{
    init();
}


/*!
  Constructs a label with a text. The label is left-aligned, vertically
  centered, has an automatic margin and with manual resizing.

  The \e parent, \e name and \e f arguments are passed to the QFrame
  constructor.

  \sa setAlignment(), setFrameStyle(), setMargin(), setAutoResize()
*/

QLabel::QLabel( const QString &text, QWidget *parent, const char *name, WFlags f )
	: QFrame( parent, name, f ), ltext(text)
{
    init();
}


/*!
  Constructs a label with an accelerator key.

  The \a parent, \a name and \a f arguments are passed to the QFrame
  constructor. Note that the \a parent argument does \e not default
  to 0.

  In a dialog, you might create two data entry widgets and a label
  for each, and set up the geometry so each label is just to the left
  of its data entry widget (its "buddy"), somewhat like this:

  \code
    QLineEdit *name    = new QLineEdit( this );
    QLabel    *name_l  = new QLabel( name, "&Name:", this );
    QLineEdit *phone   = new QLineEdit( this );
    QLabel    *phone_l = new QLabel( phone, "&Phone:", this );
    // geometry management setup not shown
  \endcode

  With the code above, the focus jumps to the Name field when the user
  presses Alt-N, and to the Phone field when the user presses Alt-P.

  \sa setText(), setBuddy()
*/

QLabel::QLabel( QWidget *buddy,  const QString &text,
		QWidget *parent, const char *name, WFlags f )
    : QFrame( parent, name, f ), ltext("")
{
    init();
    align      = ShowPrefix | AlignLeft | AlignVCenter | ExpandTabs;
    setBuddy( buddy );
    setText( text );
}


/*!
  Destroys the label.
*/

QLabel::~QLabel()
{
    unsetMovie();
    delete lpixmap;
    delete extra;
}


void QLabel::init()
{
    lpixmap    = 0;
    align      = AlignLeft | AlignVCenter | ExpandTabs;
    extraMargin= -1;
    autoresize = FALSE;
    extra      = 0;
}

/*!
  \fn QString QLabel::text() const
  Returns the label text.
  \sa setText()
*/

/*!
  Sets the label contents to \e text, updates the optional
  accelerator and redraws the contents.

  The label resizes itself if auto-resizing is enabled.	 Nothing
  happens if \e text is the same as the current label.

  \sa text(), setPixmap(), setAutoResize()
*/

void QLabel::setText( const QString &text )
{
    unsetMovie();
    if ( ltext == text )
	return;
    ltext = text;
    if ( lpixmap ) {
	delete lpixmap;
	lpixmap = 0;
    }
    if ( extra ) {
	extra->accel->clear();
	QString p = strchr( ltext, '&' );
	while( !p.isEmpty() && p[1] == '&' )
	    p = strchr( ((const char*)p)+2, '&' );
	if ( !p.isEmpty() && isalpha(p[1]) ) {
	    extra->accel->connectItem(
		extra->accel->insertItem( ALT+toupper(p[1]) ),
	        this, SLOT(acceleratorSlot()) );
	}
    }
    if ( autoresize ) {
	QSize s = sizeHint();
	if ( s.isValid() && s != size() )
	    resize( s );
	else
	    repaint();
    } else {
	updateLabel();
    }
}


/*!  Clears the label.  Equivalent with setText( "" ). */

void QLabel::clear()
{
    setText( "" );
}


/*!
  \fn QPixmap *QLabel::pixmap() const
  Returns the label pixmap.
  \sa setPixmap()
*/

/*!
  Sets the label contents to \e pixmap and redraws the contents.

  If the label has a buddy, the accelerator is disabled since the
  pixmap doesn't contain any suitable character.

  The label resizes itself if auto-resizing is enabled.	 Nothing
  happens if \e pixmap is the same as the current label.

  \sa pixmap(), setText(), setAutoResize()
*/

void QLabel::setPixmap( const QPixmap &pixmap )
{
    unsetMovie();
    int w, h;
    if ( lpixmap ) {
	w = lpixmap->width();
	h = lpixmap->height();
    } else {
	lpixmap = new QPixmap;
	CHECK_PTR( lpixmap );
	w = h = -1;
    }
    bool sameSize = w == lpixmap->width() && h == lpixmap->height();
    *lpixmap = pixmap;
    if ( lpixmap->depth() == 1 && !lpixmap->mask() )
	lpixmap->setMask( *((QBitmap *)lpixmap) );
    if ( !ltext.isNull() )
	ltext.resize( 0 );
    if ( autoresize && !sameSize )
	adjustSize();
    else
	updateLabel();
    if ( extra )
	extra->accel->clear();
}


/*!
  Sets the label contents to \e num (converts it to text) and redraws the
  contents.

  If the label has a buddy, the accelerator is disabled since the
  number doesn't contain any suitable character.

  The label resizes itself if auto-resizing is enabled.	 Nothing
  happens if \e num reads the same as the current label.

  \sa setAutoResize()
*/

void QLabel::setNum( int num )
{
    QString str;
    str.setNum( num );
    if ( str != ltext ) {
	setText( str );
	if ( autoresize )
	    adjustSize();
	else
	    updateLabel();
    }
}

/*!
  Sets the label contents to \e num (converts it to text) and redraws the
  contents.

  If the label has a buddy, the accelerator is disabled since the
  number doesn't contain any suitable character.

  The label resizes itself if auto-resizing is enabled.

  \sa setAutoResize()
*/

void QLabel::setNum( double num )
{
    QString str;
    str.sprintf( "%g", num );
    if ( str != ltext ) {
	setText( str );
	if ( autoresize )
	    adjustSize();
	else
	    updateLabel();
    }
}


/*!
  \fn int QLabel::alignment() const
  Returns the alignment setting.

  The default alignment is <code>AlignLeft | AlignVCenter |
  ExpandTabs</code> if the label doesn't have a buddy and
  <code>AlignLeft | AlignVCenter | ExpandTabs | ShowPrefix </code> if
  the label has a buddy.

  \sa setAlignment()
*/

/*!
  Sets the alignment of the label contents and redraws itself.

  The \e alignment is the bitwise OR of the following flags:
  <ul>
  <li> \c AlignLeft aligns to the left border.
  <li> \c AlignRight aligns to the right border.
  <li> \c AlignHCenter aligns horizontally centered.
  <li> \c AlignTop aligns to the top border.
  <li> \c AlignBottom aligns to the bottom border.
  <li> \c AlignVCenter aligns vertically centered
  <li> \c AlignCenter (= \c AlignHCenter | AlignVCenter)
  <li> \c ExpandTabs expands tabulators.
  <li> \c WordBreak enables automatic word breaking.
  </ul>

  If the label has a buddy, \c ShowPrefix is forced to TRUE.

  \sa alignment() setBuddy() setText()
*/

void QLabel::setAlignment( int alignment )
{
    if ( extra && extra->buddy )
	align = alignment | ShowPrefix;
    else
	align = alignment;
    updateLabel();
}


/*!
  \fn int QLabel::margin() const

  Returns the margin of the label.

  \sa setMargin()
*/

/*!
  Sets the margin of the label to \e margin pixels.

  The margin applies to the left edge if alignment() is \c AlignLeft,
  to the right edge if alignment() is \c AlignRight, to the top edge
  if alignment() is \c AlignTop, and to to the bottom edge if
  alignment() is \c AlignBottom.

  If \e margin is negative (as it is by default), the label computes the
  margin as follows: If the \link frameWidth() frame width\endlink is zero,
  the effective margin becomes 0. If the frame style is greater than zero,
  the effective margin becomes half the width of the "x" character (of the
  widget's current \link font() font\endlink.

  Setting a non-negative margin gives the specified margin in pixels.

  \sa margin(), frameWidth(), font()
*/

void QLabel::setMargin( int margin )
{
    extraMargin = margin;
}


/*!
  \fn bool QLabel::autoResize() const
  Returns TRUE if auto-resizing is enabled, or FALSE if auto-resizing is
  disabled.

  Auto-resizing is disabled by default.

  \sa setAutoResize()
*/

/*!
  Enables auto-resizing if \e enable is TRUE, or disables it if \e
  enable is FALSE.

  When auto-resizing is enabled, the label will resize itself whenever the
  contents change.  The top left corner is not moved.

  \sa autoResize(), adjustSize()
*/

void QLabel::setAutoResize( bool enable )
{
    if ( (bool)autoresize != enable ) {
	autoresize = enable;
	if ( autoresize )
	    adjustSize();			// calls resize which repaints
    }
}


/*!
  Returns a size which fits the contents of the label.

  \bug Does not work well with the WordBreak flag
*/

QSize QLabel::sizeHint() const
{
    QPainter p( this );
    QRect br;
    QPixmap *pix = pixmap();
    QMovie *mov = movie();
    if ( pix ) {
	br = QRect( 0, 0, pix->width(), pix->height() );
    } else if ( mov ) {
	br = QRect( 0, 0, mov->framePixmap().width(),
		mov->framePixmap().height() );
    } else {
	br = p.boundingRect( 0,0, 1000,1000, alignment(), text() );
	// adjust so "Yes" and "yes" will have the same height
	int h = fontMetrics().lineSpacing();
	br.setHeight( ((br.height() + h-1) / h)*h - fontMetrics().leading() );
    }
    int m  = 2*margin();
    int fw = frameWidth();
    if ( m < 0 ) {
	if ( fw > 0 )
	    m = p.fontMetrics().width( "x" );
	else
	    m = 0;
    }
    int w = br.width()	+ m + 2*fw;
    int h = br.height() + m + 2*fw;

    return QSize( w, h );
}


/*!
  Draws the label contents using the painter \e p.
*/

void QLabel::drawContents( QPainter *p )
{
    QRect cr = contentsRect();
    int m = margin();
    if ( m < 0 ) {
	if ( frameWidth() > 0 )
	    m = p->fontMetrics().width("x")/2;
	else
	    m = 0;
    }
    if ( m > 0 ) {
	if ( align & AlignLeft )
	    cr.setLeft( cr.left() + m );
	if ( align & AlignRight )
	    cr.setRight( cr.right() - m );
	if ( align & AlignTop )
	    cr.setTop( cr.top() + m );
	if ( align & AlignBottom )
	    cr.setBottom( cr.bottom() - m );
    }

    QMovie *mov = movie();
    if ( mov ) {
	// ### should add movie to qDrawItem
 	QRect r = qItemRect( p, style(),
 			cr.x(), cr.y(), cr.width(), cr.height(),
 			align, isEnabled(), &(mov->framePixmap()), ltext );
	// ### could resize movie frame at this point
	p->drawPixmap(r.x(), r.y(), mov->framePixmap() );
	return;
    }

    // Not a movie
    qDrawItem( p, style(), cr.x(), cr.y(), cr.width(), cr.height(),
	       align, colorGroup(), isEnabled(),
	       lpixmap, ltext );
}


void QLabel::setAutoMask(bool b)
{
    if (b)
	setBackgroundMode( PaletteText );
    else
	setBackgroundMode( PaletteBackground );
    QFrame::setAutoMask(b);
}

void QLabel::drawContentsMask( QPainter *p )
{
    QRect cr = contentsRect();
    int m = margin();
    if ( m < 0 ) {
	if ( frameWidth() > 0 )
	    m = p->fontMetrics().width("x")/2;
	else
	    m = 0;
    }
    if ( m > 0 ) {
	if ( align & AlignLeft )
	    cr.setLeft( cr.left() + m );
	if ( align & AlignRight )
	    cr.setRight( cr.right() - m );
	if ( align & AlignTop )
	    cr.setTop( cr.top() + m );
	if ( align & AlignBottom )
	    cr.setBottom( cr.bottom() - m );
    }

    QMovie *mov = movie();
    if ( mov ) {
	// ### should add movie to qDrawItem
	QRect r = qItemRect( p, style(),
			cr.x(), cr.y(), cr.width(), cr.height(),
			align, isEnabled(), &(mov->framePixmap()), ltext );
	// ### could resize movie frame at this point
	QPixmap pm = mov->framePixmap();
	if ( pm.mask() ) {
	    p->setPen( QColor::color1);
	    p->drawPixmap(r.x(), r.y(), *pm.mask() );
	}
	else
	    p->fillRect( r, QColor::color1 );
	return;
    }

    QColorGroup g(QColor::color1, QColor::color1, QColor::color1, QColor::color1, QColor::color1, QColor::color1, QColor::color1, QColor::color1, QColor::color0);
    qDrawItem( p, style(), cr.x(), cr.y(), cr.width(), cr.height(),
	       align, g, isEnabled(), lpixmap, ltext );
}


/*!
  Updates the label, not the frame.
*/

void QLabel::updateLabel()
{
    repaint(contentsRect());
    if ( autoMask() )
	updateMask();
}


/*!
  Internal slot, used to set focus for accelerator labels.
*/

void QLabel::acceleratorSlot()
{
    if ( extra && extra->buddy &&
	 !extra->buddy->hasFocus() &&
	 extra->buddy->isEnabledToTLW() &&
	 extra->buddy->isVisibleToTLW() &&
	 extra->buddy->focusPolicy() != NoFocus )
	extra->buddy->setFocus();
}


/*!
  Internal slot, used to clean up if the buddy widget dies.
*/

void QLabel::buddyDied() // I can't remember if I cried.
{
    if ( extra )
	extra->buddy = 0;
}


/*!
  Sets the buddy of this label to \a buddy.

  When the user presses the accelerator key indicated by this label,
  the keyboard focus is transferred to the label's buddy.

  \sa label(), setText()
*/

void QLabel::setBuddy( QWidget *buddy )
{
    if ( buddy )
	setAlignment( alignment() | ShowPrefix );
    else
	setAlignment( alignment() & ~ShowPrefix );

    if ( extra ) {
	if ( extra->buddy )
	    disconnect( extra->buddy, SIGNAL(destroyed()),
			this, SLOT(buddyDied()) );
    } else if ( buddy ) {
	extra = new QLabelExtra(this);
	extra->buddy = buddy;
    }

    extra->buddy = buddy;

    if ( !buddy )
	return;

    QString p = ltext.isEmpty() ? 0 : strchr( ltext, '&' );
    while( !p.isEmpty() && p[1] == '&' )
	p = strchr( ((const char*)p)+2, '&' );
    if ( !p.isEmpty() && isalnum(p[1]) ) {
	extra->accel->connectItem( extra->accel->insertItem(ALT+
							  toupper(p[1])),
				  this, SLOT(acceleratorSlot()) );
    }

    connect( buddy, SIGNAL(destroyed()), this, SLOT(buddyDied()) );
}


/*!
  Returns the buddy of this label.
*/

QWidget * QLabel::buddy() const
{
    return extra ? extra->buddy : 0;
}


void QLabel::movieUpdated(const QRect& rect)
{
    QMovie *mov = movie();
    if ( mov && !mov->isNull() ) {
	QRect r = contentsRect();
	r = qItemRect( 0, style(), r.x(), r.y(), r.width(), r.height(),
		       align, isEnabled(), &(mov->framePixmap()), ltext );
	r.moveBy(rect.x(), rect.y());
	r.setWidth(QMIN(r.width(), rect.width()));
	r.setHeight(QMIN(r.height(), rect.height()));
	repaint( r );
	if ( autoMask() )
	    updateMask();
    }
}

void QLabel::movieResized(const QSize& size)
{
    if (autoresize) adjustSize();
    movieUpdated(QRect(QPoint(0,0),size));
}

/*!
  Sets a QMovie to display in the label, or removes any existing movie
  if the given movie QMovie::isNull().

  Any current pixmap or text label is cleared.

  If the label has a buddy, the accelerator is disabled since the
  movie doesn't contain any suitable character.
*/
void QLabel::setMovie( const QMovie& movie )
{
    unsetMovie();

    if ( !extra ) {
	if ( movie.isNull() )
	    return;
	extra = new QLabelExtra(this);
    }

    if ( !movie.isNull() ) {
	if ( !extra->movie )
	    extra->movie = new QMovie;
	*extra->movie = movie;
    }
    extra->accel->clear();

    if ( lpixmap ) {
	delete lpixmap;
	lpixmap = 0;
    }

    if ( extra->movie ) {
	extra->movie->connectResize(this, SLOT(movieResized(const QSize&)));
	extra->movie->connectUpdate(this, SLOT(movieUpdated(const QRect&)));
    }
}

/*
  Unset the movie.
*/
void QLabel::unsetMovie()
{
    if (extra && extra->movie) {
	extra->movie->disconnectResize(this, SLOT(movieResized(const QSize&)));
	extra->movie->disconnectUpdate(this, SLOT(movieUpdated(const QRect&)));
	delete extra->movie;
	extra->movie = 0;
    }
}

/*!
  Returns the QMovie currently displaying in the label, or 0
  if none has been set.
*/
QMovie* QLabel::movie() const
{
    return extra ? extra->movie : 0;
}
