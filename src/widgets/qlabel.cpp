/**********************************************************************
** $Id: //depot/qt/main/src/widgets/qlabel.cpp#14 $
**
** Implementation of QLabel widget class
**
** Author  : Eirik Eng
** Created : 941215
**
** Copyright (C) 1994,1995 by Troll Tech AS.  All rights reserved.
**
***********************************************************************/

#include "qlabel.h"
#include "qpainter.h"

#if defined(DEBUG)
static char ident[] = "$Id: //depot/qt/main/src/widgets/qlabel.cpp#14 $";
#endif


QLabel::QLabel( QWidget *parent, const char *name )
	: QFrame( parent, name )
{
    initMetaObject();
    align      = AlignLeft | AlignVCenter | ExpandTabs;
    autoResize = FALSE;
}

QLabel::QLabel( const char *label, QWidget *parent, const char *name )
	: QFrame( parent, name ), str(label)
{
    initMetaObject();
    align      = AlignLeft | AlignVCenter | ExpandTabs;
    autoResize = FALSE;
}


void QLabel::setLabel( const char *s )
{
    if ( str == s )				// no change
	return;
    str = s;
    if ( autoResize )
        adjustSize();
    updateLabel();
}

void QLabel::setLabel( long l )
{
    QString tmp;
    tmp.sprintf( "%ld", l );
    if ( tmp != str ) {
	str = tmp;
        if ( autoResize )
            adjustSize();
	updateLabel();
    }
}

void QLabel::setLabel( double d )
{
    QString tmp;
    tmp.sprintf( "%g", d );
    if ( tmp != str ) {
	str = tmp;
        if ( autoResize )
  	    adjustSize();
	updateLabel();
    }
}

void QLabel::setAlignment( int alignment )
{
    align = alignment;
    updateLabel();
}

void QLabel::setAutoResizing( bool enable )
{
    if ( autoResize != enable ) {
	autoResize = enable;
	if ( autoResize )
	    adjustSize();			// calls resize which repaints
    }
}

void QLabel::adjustSize()
{
    QFontMetrics fm( font() );
    resize( fm.width( str ) + 4 + frameWidth() + midLineWidth(),
	    fm.height()     + 4 + frameWidth() + midLineWidth() );
}


void QLabel::drawContents( QPainter *p )
{
    p->setPen( colorGroup().text() );
    p->drawText( contentsRect(), align, str );
}

void QLabel::updateLabel()			// update label, not frame
{
    QPainter paint;
    paint.begin( this );
    paint.eraseRect( contentsRect() );
    drawContents( &paint );
    paint.end();
}
