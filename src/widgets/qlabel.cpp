/**********************************************************************
** $Id: //depot/qt/main/src/widgets/qlabel.cpp#47 $
**
** Implementation of QLabel widget class
**
** Created : 941215
**
** Copyright (C) 1994-1996 by Troll Tech AS.  All rights reserved.
**
***********************************************************************/

#include "qlabel.h"
#include "qpixmap.h"
#include "qpainter.h"
#include "qdrawutl.h"
#include "qaccel.h"
#include "qkeycode.h"
#include <ctype.h>

RCSTAG("$Id: //depot/qt/main/src/widgets/qlabel.cpp#47 $");

// ### clean up for 2.0

#include <qintdict.h>

struct QLabel_Private
{
    QWidget * buddy;
    QAccel * accel;
};

static QIntDict<QLabel_Private> *qlabel_extraStuff = 0;

static void cleanupLabel()
{
    delete qlabel_extraStuff;
    qlabel_extraStuff = 0;
}

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
     QLabel * phoneLabel = new QLineEdit( phone, "&Phone", this );
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
    initMetaObject();
    lpixmap    = 0;
    align      = AlignLeft | AlignVCenter | ExpandTabs;
    extraMargin= -1;
    autoresize = FALSE;
}

/*!
  Constructs a label with a text. The label is left-aligned, vertically
  centered, has an automatic margin and with manual resizing.

  The \e parent, \e name and \e f arguments are passed to the QFrame
  constructor.

  \sa setAlignment(), setFrameStyle(), setMargin(), setAutoResize()
*/

QLabel::QLabel( const char *text, QWidget *parent, const char *name, WFlags f )
	: QFrame( parent, name, f ), ltext(text)
{
    initMetaObject();
    lpixmap    = 0;
    align      = AlignLeft | AlignVCenter | ExpandTabs;
    extraMargin= -1;
    autoresize = FALSE;
}


/*!  Constructs a label with an accelerator key.

  The \a parent, \a name and \a f arguments are passed to the QFrame
  constructor.  Note that the \a parent argument does \e not default
  to 0.

  In a dialog, you might create three data entry widgets and a label
  for each, and set up the geometry so each label is just to the left
  of its data entry widget (its "buddy"), somewhat like this:

  \code
    QLineEdit * name( this, "customer name" );
    QLabel * alName( name, "&Name:", this );
    QLineEdit * phone( this, "customer telephone number" );
    QLabel * alPhone( phone, "&Phone:", this );
    QMultiLineEdit * address( this, "customer address:" );
    QLabel * alAddress( address, "&Address", this );
    // geometry management setup not shown
  \\endcode

  With the code above, the focus jumps to the Address field when the
  user presses Alt-A, to the Name field when the user presses Alt-N,
  and to the Phone field when the user presses Alt-P.

  \sa setText() setBuddy()
*/

QLabel::QLabel( QWidget * buddy, const char * text,
		QWidget *parent, const char *name, WFlags f )
    : QFrame( parent, name, f ), ltext("")
{
    initMetaObject();
    lpixmap    = 0;
    align      = ShowPrefix | AlignLeft | AlignVCenter | ExpandTabs;
    extraMargin= -1;
    autoresize = FALSE;

    setBuddy( buddy );
    setText( text );
}


/*!
  Destroys the label.
*/

QLabel::~QLabel()
{
    delete lpixmap;
    QLabel_Private * d;
    if ( qlabel_extraStuff && (d=qlabel_extraStuff->find( (long)this )) ) {
	qlabel_extraStuff->take( (long)this );
	delete d;
    }
}


/*!
  \fn const char *QLabel::text() const
  Returns the label text.
  \sa setText()
*/

/*!  Sets the label contents to \e text, updates the optional
  accelerator and redraws the contents.

  The label resizes itself if auto-resizing is enabled.	 Nothing
  happens if \e text is the same as the current label.

  \sa text(), setPixmap(), setAutoResize()
*/

void QLabel::setText( const char *text )
{
    if ( ltext == text )
	return;
    ltext = text;
    if ( lpixmap ) {
	delete lpixmap;
	lpixmap = 0;
    }
    QLabel_Private * d;
    if ( qlabel_extraStuff && (d=qlabel_extraStuff->find( (long)this )) ) {
	const char * p = strchr( ltext, '&' );
	d->accel->clear();
	if ( p && isalpha(p[1]) ) {
	    d->accel->connectItem( d->accel->insertItem( ALT+toupper(p[1]) ),
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
    QLabel_Private * d;
    if ( qlabel_extraStuff && (d=qlabel_extraStuff->find( (long)this )) )
	d->accel->clear();
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
    if ( !(align & ShowPrefix)
	 && qlabel_extraStuff
	 && qlabel_extraStuff->find( (long)this ) )
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
    if ( autoresize != enable ) {
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
    QPainter p;
    p.begin( this );
    QRect br;
    QPixmap *pix = pixmap();
    if ( pix )
	br = QRect( 0, 0, pix->width(), pix->height() );
    else
	br = p.boundingRect( 0,0, 1000,1000, alignment(), text() );
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
    p.end();

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
    qDrawItem( p, style(), cr.x(), cr.y(), cr.width(), cr.height(),
	       align, colorGroup(), isEnabled(),
	       lpixmap, ltext );
}


/*!
  Updates the label, not the frame.
*/

void QLabel::updateLabel()
{
    QPainter paint;
    paint.begin( this );
    paint.eraseRect( contentsRect() );
    drawContents( &paint );
    paint.end();
}



/*!
  Internal slot, used to set focus for accelerator labels.
*/

void QLabel::acceleratorSlot()
{
    if ( !qlabel_extraStuff )
	return;

    QLabel_Private * that = qlabel_extraStuff->find( (long)this );
    if ( that && that->buddy && !that->buddy->hasFocus() ) {
	QWidget * w = that->buddy;
	while ( w ) {
	    if ( !w->isVisible() )
		return;
	    w = w->parentWidget();
	}
	that->buddy->setFocus();
    }
}


/*!
  Internal slot, used to clean up if the buddy widget dies.
*/

void QLabel::buddyDied()
{
    if ( !qlabel_extraStuff )
	return;

    QLabel_Private * that = qlabel_extraStuff->find( (long)this );
    if ( that )
	that->buddy = 0;
}


/*!
  Sets the buddy of this label to \a buddy.

  When the user presses the accelerator key indicated by this label,
  the keyboard focus is transferred to the label's buddy.

  \sa label() setText()
*/

void QLabel::setBuddy( QWidget * buddy )
{
    setAlignment( alignment() | ShowPrefix );

    if ( !qlabel_extraStuff ) {
	qlabel_extraStuff = new QIntDict<QLabel_Private>;
	CHECK_PTR( qlabel_extraStuff );
	qAddPostRoutine( cleanupLabel );
    }
    QLabel_Private * that = qlabel_extraStuff->find( (long)this );
    if ( that ) {
	if ( that->buddy )
	    disconnect( that->buddy, SIGNAL(destroyed()),
			this, SLOT(buddyDied()) );
    } else {
	that = new QLabel_Private;
	that->buddy = buddy;
	that->accel = new QAccel( this, "accel label accel" );
	const char * p = strchr( ltext, '&' );
	if ( p && isalpha(p[1]) ) {
	    that->accel->connectItem( that->accel->insertItem(ALT+
							      toupper(p[1])),
				      this, SLOT(acceleratorSlot()) );
	}
	qlabel_extraStuff->insert( (long)this, that );
    }

    that->buddy = buddy;
    connect( buddy, SIGNAL(destroyed()), this, SLOT(buddyDied()) );
}


/*!
  Returns the buddy of this label.
*/

QWidget * QLabel::buddy() const
{
    if ( !qlabel_extraStuff )
	return 0;

    QLabel_Private * that = qlabel_extraStuff->find( (long)this );
    return that && that->buddy ? that->buddy : 0;
}
