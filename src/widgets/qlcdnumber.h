/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qlcdnumber.h#8 $
**
** Definition of QLCDNumber class
**
** Author  : Eirik Eng
** Created : 940518
**
** Copyright (C) 1994,1995 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QLCDNUM_H
#define QLCDNUM_H

#include "qframe.h"
#include "qbitarry.h"


class QLCDNumber : public QFrame		// LCD number widget
{
    Q_OBJECT
public:
    QLCDNumber( QWidget *parent=0, const char *name=0 );
    QLCDNumber( uint numDigits, QWidget *parent=0, const char *name=0 );
   ~QLCDNumber();

    enum Mode { HEX, DEC, OCT, BIN };

    uint    numDigits()	const	{ return ndigits; }
    void    setNumDigits( uint nDigits );

    bool    checkOverflow( double num ) const;
    bool    checkOverflow( long	  num ) const;

    QLCDNumber::Mode mode() const;

public slots:
    void    display( int num );
    void    display( long num );
    void    display( float num );
    void    display( double num );
    void    display( const char *str );
    void    setMode( Mode );
    void    smallDecimalPoint( bool );

protected signals:
    void    overflow();

protected:
    void    resizeEvent( QResizeEvent * );
    void    drawContents( QPainter * );

private:
    void    init();
    void    drawString( const char *, QPainter &, QBitArray * = 0,
			bool = TRUE );
    void    drawDigit( const QPoint &, QPainter &, int, char, char = ' ' );
    void    drawSegment( const QPoint &, char, QPainter &, int, bool = FALSE );

    uint    ndigits	: 8;
    uint    base	: 2;
    uint    smallPoint	: 1;
    QString digitStr;
    QBitArray points;
};


#endif // QLCDNUM_H
