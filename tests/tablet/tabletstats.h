/****************************************************************************
** $Id: $
**
** Copyright ( C ) 1992-2001 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#ifndef _TABLET_STATS_
#define _TABLET_STATS_

#include <qwidget.h>
#include <qframe.h>
#include "canvas.h"

class QLabel;

class MyOrientation : public QFrame  
{
    Q_OBJECT
public:
    MyOrientation( QWidget *parent = 0, const char *name = 0 );
    virtual ~MyOrientation();

public slots:
    void newOrient( int tiltX, int tiltY );

protected:
    void resizeEvent( QResizeEvent *e );
};

class StatsCanvas : public Canvas
{
    Q_OBJECT
public:
    StatsCanvas( QWidget *parent = 0, const char* name = 0 ) : Canvas( parent, name )
    {};
    ~StatsCanvas() {};
signals:
    void signalNewPressure( int );
    void signalNewTilt( int, int );
    void signalNewDev( int );
    void signalNewLoc( int, int );

protected:
    void tabletEvent( QTabletEvent *e );
    void mouseMoveEvent( QMouseEvent *e );
    void paintEvent( QPaintEvent *e );
    void mouseReleaseEvent( QMouseEvent *e );

private:
    QRect r;
};

class TabletStats : public QWidget
{
    Q_OBJECT
public:
    TabletStats( QWidget *parent, const char* name );
    ~TabletStats();

private slots:
    void slotPressureChanged( int newPress );
    void slotTiltChanged( int newTiltX, int newTiltY );
    void slotDevChanged( int newDev );
    void slotLocationChanged( int newX, int newY );

protected:
    QLabel *lblXTilt,
	   *lblYTilt,
	   *lblPressure,
	   *lblDev,
	   *lblXPos,
	   *lblYPos;
    StatsCanvas *statCan;
    MyOrientation *orient;
};

#endif
