/****************************************************************************
**
** Definition of QThumbWheel class.
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef QTHUMBWHEEL_H
#define QTHUMBWHEEL_H

#ifndef QT_H
#include "q3frame.h"
#include "qrangecontrol.h"
#endif // QT_H

#ifndef QT_NO_THUMBWHEEL

class QThumbWheel : public Q3Frame, public QRangeControl
{
    Q_OBJECT

public:
    QThumbWheel( QWidget *parent=0, const char *name=0 );
    ~QThumbWheel();

    virtual void 	setOrientation( Qt::Orientation );
    Qt::Orientation  	orientation() const;
    virtual void	setTracking( bool enable );
    bool		tracking() const;
    virtual void	setTransmissionRatio( double r );
    double 		transmissionRatio() const;

public slots:
    virtual void setValue( int );

signals:
    void 	valueChanged( int value );

protected:
    void 	valueChange();
    void 	rangeChange();
    void 	stepChange();

    void 	keyPressEvent( QKeyEvent * );
    void 	mousePressEvent( QMouseEvent * );
    void 	mouseReleaseEvent( QMouseEvent * );
    void 	mouseMoveEvent( QMouseEvent * );
    void 	wheelEvent( QWheelEvent * );
    void	focusInEvent( QFocusEvent *e );
    void	focusOutEvent( QFocusEvent *e );

    void 	drawContents( QPainter * );

private:
    void 	init();
    int		valueFromPosition( const QPoint & );


    double 	rat;
    int		pressedAt;
    Qt::Orientation orient;
    uint	track : 1;
    uint	mousePressed : 1;

    class QThumbWheelPrivate;
    QThumbWheelPrivate *d;
};

inline Qt::Orientation QThumbWheel::orientation() const
{
    return orient;
}

inline bool QThumbWheel::tracking() const
{
    return (bool)track;
}

inline double QThumbWheel::transmissionRatio() const
{
    return rat;
}

#endif // QT_NO_WHEEL

#endif // QWHEEL_H
