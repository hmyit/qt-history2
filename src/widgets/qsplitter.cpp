/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qsplitter.cpp#47 $
**
**  Splitter widget
**
**  Created:  980105
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees with valid Qt Professional Edition licenses may distribute and
** use this file in accordance with the Qt Professional Edition License
** provided at sale or upon request.
**
** See http://www.troll.no/pricing.html or email sales@troll.no for
** information about the Professional Edition licensing, or see
** http://www.troll.no/qpl/ for QPL licensing information.
**
*****************************************************************************/
#include "qsplitter.h"

#include "qpainter.h"
#include "qdrawutil.h"
#include "qbitmap.h"
#include "qlayoutengine.h"
#include "qlist.h"
#include "qarray.h"

class QSplitterHandle : public QWidget
{
public:
    QSplitterHandle( Qt::Orientation o,
		       QSplitter *parent, QString name=0 );
    void setOrientation( Qt::Orientation o );
    Qt::Orientation orientation() const { return orient; }

    bool opaque() const { return s->opaqueResize(); }

    QSize sizeHint() const;
    QSizePolicy sizePolicy() const;

    int id() const { return myId; }
    void setId( int i ) { myId = i; }

protected:
    //    void resizeEvent( QResizeEvent * );
    void paintEvent( QPaintEvent * );
    void mouseMoveEvent( QMouseEvent * );
    void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );

private:
    Qt::Orientation orient;
    bool opaq;
    int myId;

    QSplitter *s;
};

static int opaqueOldPos = -1; //### there's only one mouse, but this is a bit risky


QSplitterHandle::QSplitterHandle( Qt::Orientation o,
				      QSplitter *parent, QString name )
    : QWidget( parent, name )
{
    s = parent;
    setOrientation(o);
}

QSizePolicy QSplitterHandle::sizePolicy() const
{
    if ( orient == Horizontal )
	return QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );
    else
    	return QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );

}

QSize QSplitterHandle::sizeHint() const
{
    if ( style() == WindowsStyle )
	return QSize(6,6);
    else
	return QSize(10,10);
}


#if 0
int QSplitter::hit( QPoint pnt )
{
    //### fancy 2-dim hit for Motif...
    QCOORD p = pick(pnt);
    if ( w1 && p > pick( w1->geometry().bottomRight() ) &&
	 w2 && p < pick( w2->pos() ) )
	return 1;
    else
	return 0;
}
#endif


void QSplitterHandle::setOrientation( Qt::Orientation o )
{
    orient = o;
    if ( o == QSplitter::Horizontal )
	setCursor( splitHCursor );
    else
	setCursor( splitVCursor );
}

void QSplitterHandle::mouseMoveEvent( QMouseEvent *e )
{
    QCOORD pos = s->pick(parentWidget()->mapFromGlobal(e->globalPos()));
    if ( opaque() ) {
	s->moveSplitter( pos, id() );
    } else {
	int min = pos; int max = pos;
	s->getRange( id(), &min, &max );
	s->setRubberband( QMAX( min, QMIN(max, pos )));
    }
}

void QSplitterHandle::mousePressEvent( QMouseEvent * )
{
    //mouseOffset..
}

void QSplitterHandle::mouseReleaseEvent( QMouseEvent *e )
{
        if ( !opaque() ) {
	    QCOORD pos = s->pick(parentWidget()->mapFromGlobal(e->globalPos()));
	    s->setRubberband( -1 );
	    s->moveSplitter( pos, id() );
    }
}

void QSplitterHandle::paintEvent( QPaintEvent * )
{
    QPainter p( this );
    s->drawSplitter( &p, 0, 0, width(), height() );
}


class QSplitterLayoutStruct
{
public:
    QSplitter::ResizeMode mode;
    QCOORD sizer;
    bool isSplitter;
    QWidget *wid;
};

class QSplitterData
{
public:
    QSplitterData() : num(0), opaque( FALSE ) {}

    QList<QSplitterLayoutStruct> list;
    int num;
    bool opaque;
};



/*!
  \class QSplitter qsplitter.h
  \brief QSplitter implements a splitter widget.

  \ingroup realwidgets

  A splitter lets the user control the size of child widgets by
  dragging the boundary between the children. Any number of widgets
  may be controlled.

  To show a QListBox and a QMultiLineEdit side by side:

  \code
    QSplitter *split = new QSplitter( parent );
    QListBox *lb = new QListBox( split );
    QMultiLineEdit *lb = new QMultiLineEdit( split );
  \endcode


  In QSplitter the boundary can be either horizontal or vertical.  The
  default is horizontal (the children are side by side) and you
  can use setOrientation( QSplitter::Vertical ) to set it to vertical.

  By default, both widgets can be as large or as small as the user
  wishes. You can naturally use setMinimumSize() and/or
  setMaximumSize() on the children. Use setResizeMode() to specify that
  a widget should keep its size when the splitter is resized.

  QSplitter normally resizes the children only at the end of a
  resize operation, but if you call setOpaqueResize( TRUE ), the
  widgets are resized as often as possible.

  <img src=qsplitter-m.gif> <img src=qsplitter-w.gif>

  \sa QTabBar
*/


/*!
  Creates a horizontal splitter.
*/

QSplitter::QSplitter( QWidget *parent, const char *name )
    :QFrame(parent,name,WPaintUnclipped)
{
     orient = Horizontal;
     init();
}
/*!
  Creates splitter with orientation \a o.
*/

QSplitter::QSplitter( Orientation o, QWidget *parent, const char *name )
    :QFrame(parent,name,WPaintUnclipped)
{
     orient = o;
     init();
}

/*!
  Destructs the splitter.
*/

QSplitter::~QSplitter()
{
    delete data;
}


void QSplitter::init()
{
    //ratio = -1;
    data = new QSplitterData;

    if ( style() == WindowsStyle )
	bord = 3;
    else
	bord = 5;
}

/*!
  \fn void QSplitter::refresh()

  Updates the splitter state. You should not need to call this
  function during normal operations.
*/


/*!  Sets the orientation to \a o.  By default the orientation is
  horizontal (the two widgets are side by side).

  \sa orientation()
*/

void QSplitter::setOrientation( Orientation o )
{
    if ( orient == o )
	return;
    orient = o;


    QSplitterLayoutStruct *s = data->list.first();
    while ( s ) {
	if ( s->isSplitter ) {
	    ((QSplitterHandle*)s->wid)->setOrientation( o );
	}
	s = data->list.next();
    }
    recalc( isVisible() );
}

/*!
   \fn Orientation QSplitter::orientation() const

   Returns the orientation (\c Horizontal or \c Vertical) of the splitter.
   \sa setOrientation()
*/

void QSplitter::resizeEvent( QResizeEvent * )
{
    doResize();
}

/*!
  Inserts the widget \a w at the end, or at the beginning if \a first is TRUE

  It is the responsibility of the caller of this function to make
  sure that \a w is not already in the splitter, and to call recalcId if needed.
  (If \a first is TRUE, then recalcId is very probably needed.)
*/
QSplitterLayoutStruct *QSplitter::addWidget( QWidget *w, bool first )
{
    QSplitterLayoutStruct *s;
    if ( data->num++ ) {
	s = new QSplitterLayoutStruct;
	QSplitterHandle *sh = new QSplitterHandle( orientation(), this );
	s->wid = sh;
	sh->setId(data->list.count());
	s->isSplitter = TRUE;
	s->sizer = pick( sh->sizeHint() );
	if ( first )
	    data->list.insert( 0, s );
	else
	    data->list.append( s );
    }
    s = new QSplitterLayoutStruct;
    s->wid = w;
    s->sizer = pick( w->size() );
    s->isSplitter = FALSE;
    if ( first )
	data->list.insert( 0, s );
    else
	data->list.append( s );
    return s;
}

/*!
  Tells the splitter that a child widget has been inserted/removed.
*/
void QSplitter::childEvent( QChildEvent *c )
{
    if ( !c->child()->isWidgetType() )
	return;

    if ( c->type() == QEvent::ChildInserted ) {
	//debug( "QSplitter::child %p inserted", c->child() );
	QSplitterLayoutStruct *s = data->list.first();
	while ( s ) {
	    if ( s->wid == c->child() )
		return;
	    s = data->list.next();
	}
	addWidget( (QWidget*)c->child() );
	recalc( isVisible() );

    } else if ( c->type() == QEvent::ChildRemoved ) {
	//debug( "QSplitter::child %p removed", c->child() );
	QSplitterLayoutStruct *p = 0;
	QSplitterLayoutStruct *s = data->list.first();
	while ( s ) {
	    if ( s->wid == c->child() ) {
		data->list.removeRef( s );
		delete s;
		if ( p && p->isSplitter ) {
		    data->list.removeRef( p );
		    delete p->wid;
		    delete p;
		}
		doResize();
		return;
	    }
	    p = s;
	    s = data->list.next();
	}

    }
}

/*!
  Shows a rubber band at position \a p. If \a p is negative, the rubber band is removed.
*/

void QSplitter::setRubberband( int p )
{
    QPainter paint( this );
    paint.setPen( gray );
    paint.setBrush( gray );
    paint.setRasterOp( XorROP );
    QRect r = contentsRect();
    const int rBord = 3; //###

    if ( orient == Horizontal ) {
	if ( opaqueOldPos >= 0 )
	    paint.drawRect( opaqueOldPos + bord - rBord , r.y(),
			    2*rBord, r.height() );
	if ( p >= 0 )
	    paint.drawRect( p  + bord - rBord, r.y(), 2*rBord, r.height() );
    } else {
	if ( opaqueOldPos >= 0 )
	    paint.drawRect( r.x(), opaqueOldPos + bord - rBord,
			    r.width(), 2*rBord );
	if ( p >= 0 )
	    paint.drawRect( r.x(), p + bord - rBord, r.width(), 2*rBord );
    }
    opaqueOldPos = p;
}


/*!
  Tells the splitter that a child widget has changed layout parameters
*/

void QSplitter::layoutHintEvent( QEvent * )
{
    recalc( isVisible() );
}


/*!
  Draws the splitter handle in the rectangle described by \a x, \a y,
  \a w, \a h using painter \a p.
*/
void QSplitter::drawSplitter( QPainter *p, QCOORD x, QCOORD y, QCOORD w, QCOORD h )
{
    static const int motifOffset = 10;
    if ( style() == WindowsStyle ) {
	qDrawWinPanel( p, x, y, w, h, colorGroup() );
    } else {
    	if ( orient == Horizontal ) {
	    QCOORD xPos = x + w/2;
	    QCOORD kPos = motifOffset;
	    QCOORD kSize = bord*2 - 2;

	    qDrawShadeLine( p, xPos, kPos + kSize - 1 ,
			    xPos, h, colorGroup() );
	    qDrawShadePanel( p, xPos-bord+1, kPos,
			     kSize, kSize, colorGroup(), FALSE, 1,
			     &colorGroup().fillButton());
	    qDrawShadeLine( p, xPos, 0, xPos, kPos ,colorGroup() );
	} else {
	    QCOORD yPos = y + h/2;
	    QCOORD kPos = w - motifOffset - 2*bord;
	    QCOORD kSize = bord*2 - 2;

	    qDrawShadeLine( p, 0, yPos, kPos, yPos, colorGroup() );
	    qDrawShadePanel( p, kPos, yPos-bord+1,
			     kSize, kSize, colorGroup(), FALSE, 1,
			     &colorGroup().fillButton());
	    qDrawShadeLine( p, kPos + kSize -1, yPos,
			    w, yPos, colorGroup() );
	}
    }
}

/*!
  Moves the left/top edge of the splitter handle with id \a \id as close as possible to
  \a p which is the distance from the left (or top) edge of the widget.

*/
void QSplitter::moveSplitter( QCOORD p, int id )
{
    p = adjustPos( p, id );
    
    QSplitterLayoutStruct *s = data->list.at(id);
    int oldP = orient == Horizontal? s->wid->x() : s->wid->y();
    bool upLeft = p < oldP;
    
    moveAfter( p, id, upLeft );
    moveBefore( p-1, id-1, upLeft );

    storeSizes();
}

void QSplitter::setG( QWidget *w, int p, int s )
{
    if ( orient == Horizontal )
	w->setGeometry( p, contentsRect().y(), s, contentsRect().height() );
    else
	w->setGeometry( contentsRect().x(), p, contentsRect().width(), s );
}


/*!
  Places the right/bottom edge of the widget at \a id at position \a pos.
*/
void QSplitter::moveBefore( int pos, int id, bool upLeft )
{
    QSplitterLayoutStruct *s = data->list.at(id);
    if ( !s )
	return;
    QWidget *w = s->wid;
    if ( s->isSplitter ) {
	int d = s->sizer;
	if ( upLeft ) {
	    setG( w, pos-d+1, d );
	    moveBefore( pos-d, id-1, upLeft );
	} else {
	    moveBefore( pos-d, id-1, upLeft );
	    setG( w, pos-d+1, d );
	}
    } else {
	int left = pick( w->pos() );
	int d = pos - left + 1;
	d = QMAX( pick(w->minimumSize()), QMIN(d, pick(w->maximumSize())));
	int newLeft = pos-d+1;
	setG( w, newLeft, d );
	if ( left != newLeft )
	    moveBefore( newLeft-1, id-1, upLeft );
    }
}


/*!
  Places the left/top edge of the widget at \a id at position \a pos.
*/
void QSplitter::moveAfter( int pos, int id, bool upLeft )
{
    QSplitterLayoutStruct *s = data->list.at(id);
    if ( !s )
	return;
    QWidget *w = s->wid;
    if ( s->isSplitter ) {
	int d = s->sizer;
	if ( upLeft ) {
	    setG( w, pos, d );
	    moveAfter( pos+d, id+1, upLeft );
	} else {
	    moveAfter( pos+d, id+1, upLeft );
	    setG( w, pos, d );
	}
    } else {
	int right = pick( w->geometry().bottomRight() );

       	int d = right - pos + 1;
	d = QMAX( pick(w->minimumSize()), QMIN(d, pick(w->maximumSize())));
	int newRight = pos+d-1;
	setG( w, pos, d );
	if ( right != newRight )
	    moveAfter( newRight+1, id+1, upLeft );
    }
}



/*!
  Returns the valid range of the splitter with id \a id in \a min and \a max.
 */
void QSplitter::getRange( int id, int *min, int *max )
{
    int minB = 0;	//before
    int maxB = 0;
    int minA = 0;
    int maxA = 0;	//after
    int n = data->list.count();
    if ( id < 0 || id >= n )
	return;
    int i;
    for ( i = 0; i < id; i++ ) {
	QSplitterLayoutStruct *s = data->list.at(i);
	if ( s->isSplitter ) {
	    minB += s->sizer;
	    maxB += s->sizer;
	} else {
	    minB += pick( s->wid->minimumSize() );
	    maxB += pick( s->wid->maximumSize() );
	}
    }
    for ( i = id; i < n; i++ ) {
	QSplitterLayoutStruct *s = data->list.at(i);
	if ( s->isSplitter ) {
	    minA += s->sizer;
	    maxA += s->sizer;
	} else {
	    minA += pick( s->wid->minimumSize() );
	    maxA += pick( s->wid->maximumSize() );
	}
    }
    QRect r = contentsRect();
    if ( min )
	*min = pick(r.topLeft()) + QMAX( minB, pick(r.size())-maxA );
    if ( max )
	*max = pick(r.topLeft()) + QMIN( maxB, pick(r.size())-minA );

}

/*!
  Returns the legal position closest to \a p of the splitter with id \a id.
*/

int QSplitter::adjustPos( int p, int id )
{
    int min = 0;
    int max = 0;
    getRange( id, &min, &max );
    p = QMAX( min, QMIN( p, max ) );

    return p;
}

void QSplitter::doResize()
{
    QRect r = contentsRect();
    int i;
    int n = data->list.count();
    QArray<QLayoutStruct> a( n );
    for ( i = 0; i< n; i++ ) {
	a[i].init();
	QSplitterLayoutStruct *s = data->list.at(i);
	if ( s->isSplitter || s->mode == KeepSize ) {
	    a[i].sizeHint = a[i].maximumSize = a[i].minimumSize = s->sizer;
	} else { //proportional
	    a[i].stretch = s->sizer;
	    a[i].maximumSize = pick( s->wid->maximumSize() );
	    a[i].sizeHint = a[i].minimumSize = pick( s->wid->minimumSize() );
	    //	    a[i].expansive = TRUE; 	//may not be necessary, but cannot hurt
	}
    }

    qGeomCalc( a, n, pick( r.topLeft() ), pick( r.size() ), 0 );
    for ( i = 0; i< n; i++ ) {
	QSplitterLayoutStruct *s = data->list.at(i);
	if ( orient == Horizontal )
	    s->wid->setGeometry( a[i].pos, r.top(), a[i].size, r.height() );
	else
	    s->wid->setGeometry( r.left(), a[i].pos, r.width(), a[i].size );
    }
	
}


void QSplitter::recalc( bool update )
{
    int fi = 2*frameWidth();
    int maxl = fi;
    int minl = fi;
    int maxt = QCOORD_MAX;
    int mint = fi;
    int n = data->list.count();
    for ( int i = 0; i< n; i++ ) {
	QSplitterLayoutStruct *s = data->list.at(i);
	if ( s->isSplitter ) {
	    minl += s->sizer;
	    maxl += s->sizer;
	} else {
	    minl += pick( s->wid->minimumSize() );
	    maxl += pick( s->wid->maximumSize() );
	    mint = QMAX( mint,  trans( s->wid->minimumSize() ));
	    maxt = QMIN( maxt,  trans( s->wid->maximumSize() ));
	}
    }
    maxl = QMIN( maxl, QCOORD_MAX );
    if ( maxt < mint )
	maxt = mint;

    if ( orient == Horizontal ) {
	setMaximumSize( maxl, maxt );
	setMinimumSize( minl, mint );
    } else {
	setMaximumSize( maxt, maxl );
	setMinimumSize( mint, minl );
    }
    if ( update )
	doResize();
}


/*!
  Sets resize mode of \a w to \a mode.
  \a mode can be one of:

  \define QSplitter::ResizeMode

  <ul>
    <li> \c Stretch (the default) - \a w will resize when the splitter resizes
    <li> \c KeepSize - \a w will keep its size.
  </ul>
*/

void QSplitter::setResizeMode( QWidget *w, ResizeMode mode )
{
    QSplitterLayoutStruct *s = data->list.first();
    while ( s ) {
	if ( s->wid == w  ) {
	    s->mode = mode;
	    return;
	}
	s = data->list.next();
    }
    s = addWidget( w, TRUE );
    s->mode = mode;
}


/*!
  Returns TRUE if opaque resize is on, FALSE otherwise.

  \sa setOpaqueResize()
*/
bool QSplitter::opaqueResize() const
{
    return data->opaque;
}

/*!
  Sets opaque resize to \a on. Opaque resize is initially turned off.

  \sa opaqueResize()
*/

void QSplitter::setOpaqueResize( bool on )
{
    data->opaque = on;
}


/*!
  Moves \a w to the leftmost/top position.
*/

void QSplitter::moveToFirst( QWidget *w )
{
    bool found = FALSE;
    QSplitterLayoutStruct *s = data->list.first();
    while ( s ) {
	if ( s->wid == w  ) {
	    found = TRUE;
	    QSplitterLayoutStruct *p = data->list.prev();
	    if ( p ) { // not already at first place
		data->list.take(); //take p
		data->list.take(); // take s
		data->list.insert( 0, p );
		data->list.insert( 0, s );
	    }
	    break;
	}
	s = data->list.next();
    }
     if ( !found )
	addWidget( w, TRUE );
     recalcId();
     //     recalc( isVisible() );
}

/*!
  Moves \a w to the rightmost/bottom position.
*/

void QSplitter::moveToLast( QWidget *w )
{
    bool found = FALSE;
    QSplitterLayoutStruct *s = data->list.first();
    while ( s ) {
	if ( s->wid == w  ) {
	    found = TRUE;
	    data->list.take(); // take s
	    QSplitterLayoutStruct *p = data->list.current();
	    if ( p ) { // the splitter handle after s
		data->list.take(); //take p
		data->list.append( p );
	    }
	    data->list.append( s );
	    break;
	}
	s = data->list.next();
    }
     if ( !found )
	addWidget( w);
     recalcId();
     //     recalc( isVisible() );
}

void QSplitter::recalcId()
{
    int n = data->list.count();
    for ( int i = 0; i < n; i++ ) {
	QSplitterLayoutStruct *s = data->list.at(i);
	if ( s->isSplitter )
	    ((QSplitterHandle*)s->wid)->setId(i);
    }
}


/*!
  Returns a size based on the child widgets.
*/
QSize QSplitter::sizeHint() const
{
    return QWidget::sizeHint();
#if 0
    QSize s1 = w1 ? w1->sizeHint() : QSize(0,0);
    QSize s2 = w2 ? w2->sizeHint() : QSize(0,0);
    if ( orient == Horizontal ) {
	return QSize(s1.width()+s2.width(),QMAX(s1.height(),s2.height()));
    } else {
	return QSize(QMAX(s1.width(),s2.width()),s1.height()+s2.height());
    }
#endif
}

/*!
  Returns a size based on the child widgets.
*/
QSizePolicy QSplitter::sizePolicy() const
{
    return QFrame::sizePolicy();
}




/*!
  Calculates stretch parameters from current sizes
*/

void QSplitter::storeSizes()
{
    QSplitterLayoutStruct *s = data->list.first();
    while ( s ) {
	if ( !s->isSplitter )
	    s->sizer = pick( s->wid->size() );
	    s = data->list.next();	
    }
}


#if 0

/*!
  Hides \a w if \a hide is TRUE, and updates the splitter.

  \warning Due to a limitation in the current implementation,
  calling QWidget::hide() will not work.
*/

void QSplitter::setHidden( QWidget *w, bool hide )
{
    if ( w == w1 ) {
	w1show = !hide;
    } else if ( w == w2 ) {
	w2show = !hide;
    } else {
#ifdef CHECK_RANGE
	warning( "QSplitter::setHidden(), unknown widget" );
#endif	
	return;
    }
    if ( hide )
	w->hide();
    else
	w->show();
    recalc( TRUE );
}


/*!
  Returns the hidden status of \a w
*/

bool QSplitter::isHidden( QWidget *w ) const
{
    if ( w == w1 )
	return !w1show;
     else if ( w == w2 )
	return !w2show;
#ifdef CHECK_RANGE
    else
	warning( "QSplitter::isHidden(), unknown widget" );
#endif	
    return FALSE;
}
#endif
