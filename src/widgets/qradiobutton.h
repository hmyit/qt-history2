/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qradiobutton.h#25 $
**
** Definition of QRadioButton class
**
** Created : 940222
**
** Copyright (C) 1994-1997 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QRADIOBUTTON_H
#define QRADIOBUTTON_H

#ifndef QT_H
#include "qbutton.h"
#endif // QT_H


class QRadioButton : public QButton
{
    Q_OBJECT
public:
    QRadioButton( QWidget *parent=0, const char *name=0 );
    QRadioButton( const char *text, QWidget *parent=0, const char *name=0 );

    bool    isChecked() const;
    void    setChecked( bool check );

    QSize    sizeHint() const;

protected:
    bool    hitButton( const QPoint & ) const;
    void    drawButton( QPainter * );
    void    drawButtonLabel( QPainter * );

    void    mouseReleaseEvent( QMouseEvent * );
    void    keyPressEvent( QKeyEvent * );

private:
    void    init();
    uint    noHit : 1;

private:	// Disabled copy constructor and operator=
    QRadioButton( const QRadioButton & );
    QRadioButton &operator=( const QRadioButton & );
};


inline bool QRadioButton::isChecked() const
{ return isOn(); }

inline void QRadioButton::setChecked( bool check )
{ setOn( check ); }


#endif // QRADIOBUTTON_H
