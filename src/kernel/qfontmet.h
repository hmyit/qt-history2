/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qfontmet.h#13 $
**
** Definition of QFontMetrics class
**
** Author  : Eirik Eng
** Created : 940514
**
** Copyright (C) 1994,1995 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QFONTMET_H
#define QFONTMET_H

#include "qfont.h"
#include "qrect.h"

class QFontMetrics
{
public:
    QFontMetrics( const QFont & );

    int			ascent()	const;
    int			descent()	const;
    int			height()	const;
    int			leading()	const;
    int			lineSpacing()	const;

    int			width( const char *, int len = -1 ) const;
    int			width( char ) const;
    QRect		boundingRect(  const char *, int len = -1 ) const;
    QRect		boundingRect(  char ) const;
    int			maxWidth() const;

    int			underlinePos()	const;
    int			strikeOutPos()	const;
    int			lineWidth()	const;

    void		setFont( const QFont & );
    const QFont	       &font() const;
private:
    QFont f;
};

#endif // QFONTMET_H

