/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qpicture.h#6 $
**
** Definition of QPicture class
**
** Author  : Haavard Nord
** Created : 940729
**
** Copyright (C) 1994,1995 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QPICTURE_H
#define QPICTURE_H

#include "qpaintd.h"
#include "qbuffer.h"


class QPicture : public QPaintDevice		// picture class
{
public:
    QPicture();
   ~QPicture();

    bool	play( QPainter * );

    bool	load( const char *fileName );	// read from file
    bool	save( const char *fileName );	// write to file

public:
    bool	cmd( int, QPDevCmdParam * );

private:
    bool	exec( QPainter *, QDataStream &s, long );
    QBuffer	pictb;				// internal buffer
    long	trecs;
    bool	formatOk;
};


#endif // QPICTURE_H
