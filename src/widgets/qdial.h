/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qdial.h#10 $
**
** Definition of the dial widget
**
** Created : 990104
**
** Copyright (C) 1999 by Troll Tech AS.	 All rights reserved.
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


#ifndef QDIAL_H
#define QDIAL_H

#ifndef QT_H
#include "qwidget.h"
#include "qrangecontrol.h"
#endif // QT_H

//class QTimer;
class QDialPrivate;

class Q_EXPORT QDial: public QWidget, public QRangeControl
{
    Q_OBJECT
public:
    QDial( QWidget *parent=0, const char *name=0 );
    QDial( int minValue, int maxValue, int pageStep, int value,
	   QWidget *parent=0, const char *name=0 );
    ~QDial();

    virtual void setTracking( bool enable );
    bool tracking() const;

    QSize sizeHint() const;
    QSizePolicy sizePolicy() const;

    virtual void setWrapping( bool on );
    bool wrapping() const;

    int notchSize() const;
    virtual void setNotchTarget( double );
    double notchTarget() const;

public slots:
    virtual void setValue( int );
    void addLine();
    void subtractLine();
    void addPage();
    void subtractPage();

signals:
    void valueChanged( int value );
    void dialPressed();
    void dialMoved( int value );
    void dialReleased();

protected:
    void resizeEvent( QResizeEvent * );
    void paintEvent( QPaintEvent * );

    void keyPressEvent( QKeyEvent * );
    void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void mouseMoveEvent( QMouseEvent * );
    void wheelEvent( QWheelEvent * );
    void focusInEvent( QFocusEvent * );
    void focusOutEvent( QFocusEvent * );

    void valueChange();
    void rangeChange();

    void repaintScreen();
    void paint( QPainter &p );
    
protected slots:
    void blink();


private:
    QDialPrivate * d;

    int valueFromPoint( const QPoint & ) const;
    double angle( const QPoint &, const QPoint & ) const;

private: // Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QDial( const QDial & );
    QDial &operator=( const QDial & );
#endif

};

#endif
