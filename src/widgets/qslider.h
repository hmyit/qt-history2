/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qslider.h#8 $
**
** Definition of QSlider class
**
** Created : 961019
**
** Copyright (C) 1994-1996 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QSLIDER_H
#define QSLIDER_H

#include "qwidget.h"
#include "qrangect.h"


class QSlider : public QWidget, public QRangeControl
{
    Q_OBJECT
public:
    enum Orientation { Horizontal, Vertical };

    QSlider( QWidget *parent=0, const char *name=0 );
    QSlider( Orientation, QWidget *parent=0, const char *name=0 );
    QSlider( int minValue, int maxValue, int step, int value, Orientation,
		QWidget *parent=0, const char *name=0 );

    void	setOrientation( Orientation );
    Orientation orientation() const;
    void	setTracking( bool enable );
    bool	tracking() const;

    virtual void setPalette( const QPalette & );
    QRect	sliderRect() const;
signals:
    void	valueChanged( int value );
    void	sliderPressed();
    void	sliderMoved( int value );
    void	sliderReleased();

protected:
    void	timerEvent( QTimerEvent * );

    void	keyPressEvent( QKeyEvent * );

    void	resizeEvent( QResizeEvent * );
    void	paintEvent( QPaintEvent * );

    void	mousePressEvent( QMouseEvent * );
    void	mouseReleaseEvent( QMouseEvent * );
    void	mouseMoveEvent( QMouseEvent * );

    void	valueChange();
    void	rangeChange();

    virtual void paintSlider( QPainter *, const QRect& );

private:

    void	drawWinBackground( QPainter*, QColorGroup & );
    void	init();
    int		positionFromValue( int ) const;
    int		valueFromPosition( int ) const;
    void	moveSlider( int );
    void	paintSlider( int, int );
    void	resetState();
    bool	track;
    int		slideWidth() const;
    int		available() const;

    int		timerId;
    QCOORD	sliderPos;
    int		sliderVal;
    QCOORD	clickOffset;
    enum State { None, Dragging, TimingUp, TimingDown };
    State	state;
    Orientation orient;

private:	// Disabled copy constructor and operator=
    QSlider( const QSlider & ) {}
    QSlider &operator=( const QSlider & ) { return *this; }
};

inline void QSlider::setTracking( bool t )
{
    track = t;
}

inline bool QSlider::tracking() const
{
    return track;
}

inline QSlider::Orientation QSlider::orientation() const
{
    return orient;
}

#endif //QSLIDER_H
