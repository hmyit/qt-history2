/****************************************************************************
** $Id: //depot/qt/main/tests/mdi/qworkspace.cpp#3 $
**
** Implementation of the QWorkspace class
**
** Created : 931107
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.troll.no/pricing.html or email sales@troll.no for
** information about the Professional Edition licensing, or see
** http://www.troll.no/qpl/ for QPL licensing information.
**
*****************************************************************************/
#include "qworkspace.h"
#include <qapplication.h>
#include <qobjectlist.h>
#include "qworkspacechild.h"


//
//  W A R N I N G
//  -------------
//
//  It is very unlikely that this code will be available in the final
//  Qt 2.0 release.  It will be available soon after then, but a number
//  of important API changes still need to be made.
//
//  Thus, it is important that you do NOT use this code in an application
//  unless you are willing for your application to be dependent on the
//  snapshot releases of Qt.
//

#define OFFSET 20


class QWorkspaceData {
public:
    QWorkspaceChild* active;
    QList<QWorkspaceChild> windows;
    QList<QWidget> icons;
    QWorkspaceChild* maxClient;
    QRect maxRestore;

    int px;
    int py;
};
QWorkspace::QWorkspace( QWidget *parent, const char *name )
    : QWidget( parent, name )
{
    d = new QWorkspaceData;
    d->active = 0;

    d->px = 0;
    d->py = 0;

}	

QWorkspace::~QWorkspace()
{
    delete d;
}


void QWorkspace::childEvent( QChildEvent * e)
{

    if (e->inserted() && e->child()->isWidgetType()) {
	QWidget* w = (QWidget*) e->child();
	if ( w->testWFlags( WStyle_Customize | WStyle_NoBorder )
	      || d->icons.contains( w ) )
	    return; 	    // nothing to do
	
	bool doShow = w->isVisible();
	
	QWorkspaceChild* child = new QWorkspaceChild( w, this );
	d->windows.append( child );
	place( child );
	child->raise();
	if ( doShow ) {
	  showClient( w );
	  activateClient( w );
	}
    } else if (e->removed() ) {
	if ( d->windows.contains( (QWorkspaceChild*)e->child() ) )
	    d->windows.remove( (QWorkspaceChild*)e->child() );
	if ( d->windows.isEmpty() )
	    hideMaxHandles();
	if ( d->icons.contains( (QWidget*)e->child() ) ){
	    d->icons.remove( (QWidget*)e->child() );
	    layoutIcons();
	}
	if( e->child() == d->active )
	    d->active = 0;
	if ( e->child() == d->maxClient  && !d->windows.isEmpty() ) {
	    activateClient( d->windows.first()->clientWidget() );
	}
    }
}


void QWorkspace::activateClient( QWidget* w)
{
    for (QWorkspaceChild* c = d->windows.first(); c; c = d->windows.next() ) {
	c->setActive( c->clientWidget() == w );
	if (c->clientWidget() == w)
	    d->active = c;
    }

    if (!d->active)
	return;
    
    if ( d->maxClient && d->maxClient != d->active )
	maximizeClient( d->active->clientWidget() );

    d->active->raise();

    emit clientActivated( w );
}


QWidget* QWorkspace::activeClient() const
{
    return d->active?d->active->clientWidget():0;
}



void QWorkspace::place( QWidget* w)
{
    int tx,ty;
    QWorkspaceChild* c = (QWorkspaceChild*) w;

    QRect maxRect = rect();
    if (d->px < maxRect.x())
	d->px = maxRect.x();
    if (d->py < maxRect.y())
	d->py = maxRect.y();

    d->px += OFFSET;
    d->py += 2*OFFSET;

    if (d->px > maxRect.width()/2)
	d->px =  maxRect.x() + OFFSET;
    if (d->py > maxRect.height()/2)
	d->py =  maxRect.y() + OFFSET;
    tx = d->px;
    ty = d->py;
    if (tx + c->width() > maxRect.right()){
	tx = maxRect.right() - c->width();
	if (tx < 0)
	    tx = 0;
	d->px =  maxRect.x();
    }
    if (ty + c->height() > maxRect.bottom()){
	ty = maxRect.bottom() - c->height();
	if (ty < 0)
	    ty = 0;
	d->py =  maxRect.y();
    }
    c->move( tx, ty );
}

void QWorkspace::insertIcon( QWidget* w )
{
    if (d->icons.contains(w) )
	return;
    d->icons.append( w );
    if (w->parentWidget() != this )
	w->reparent( this, 0, QPoint(0,0), FALSE);
    layoutIcons();
    if (isVisible())
	w->show();

}

void QWorkspace::removeIcon( QWidget* w)
{
    if (!d->icons.contains( w ) )
	return;
    d->icons.remove( w );
    w->hide();
 }

void QWorkspace::resizeEvent( QResizeEvent * )
{
    if ( d->maxClient )
	d->maxClient->adjustToFullscreen();
    layoutIcons();
}

void QWorkspace::layoutIcons()
{
    int x = 0;
    int y = height();
    for (QWidget* w = d->icons.first(); w ; w = d->icons.next() ) {
	
	if ( x > 0 && x + w->width() > width() ){
	    x = 0;
	    y -= w->height();
	}
	
	w->move(x, y-w->height());
	x = w->geometry().right();
    }
}

void QWorkspace::minimizeClient( QWidget* w)
{
    QWorkspaceChild* c = findChild( w );
    if ( c ) {
	c->hide();
	insertIcon( c->iconWidget() );
	if ( d->maxClient == c ) {
	    c->setGeometry( d->maxRestore );
	    d->maxClient = 0;
	    hideMaxHandles();
	}
	
    }
}

void QWorkspace::normalizeClient( QWidget* w)
{
    QWorkspaceChild* c = findChild( w );
    if ( c ) {
	if ( c == d->maxClient ) {
	    c->setGeometry( d->maxRestore );
	    d->maxClient = 0;
	}
	else {
	    removeIcon( c->iconWidget() );
	    c->show();
	}
    }
}

void QWorkspace::maximizeClient( QWidget* w)
{
    QWorkspaceChild* c = findChild( w );
    
    if ( c &&  c == d->maxClient ) {
	normalizeClient( w );
	return;
    }
    
    if ( c ) {
	if (d->icons.contains(c->iconWidget()) )
	    normalizeClient( w );
	QRect r( c->geometry() );
	c->adjustToFullscreen();
	c->show();
	if ( d->maxClient && d->maxClient != c ) {
	    d->maxClient->setGeometry( d->maxRestore );
	}
	d->maxClient = c;
	d->maxRestore = r;
	
	activateClient( w);
    }
}

void QWorkspace::showClient( QWidget* w)
{
    if ( d->maxClient )
	maximizeClient( w );
    else
	normalizeClient( w );
}


QWorkspaceChild* QWorkspace::findChild( QWidget* w)
{
    for (QWorkspaceChild* c = d->windows.first(); c; c = d->windows.next() ) {
	if (c->clientWidget() == w)
	    return c;
    }
    return 0;
}

void QWorkspace::showMaxHandles()
{
}

void QWorkspace::hideMaxHandles()
{
}
