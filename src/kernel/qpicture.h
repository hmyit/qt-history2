/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qpicture.h#20 $
**
** Definition of QPicture class
**
** Created : 940729
**
** Copyright (C) 1994-1997 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QPICTURE_H
#define QPICTURE_H

#ifndef QT_H
#include "qpaintdevice.h"
#include "qbuffer.h"
#endif // QT_H


class QPicture : public QPaintDevice		// picture class
{
public:
    QPicture();
   ~QPicture();

    bool	isNull() const;

    uint	size() const;
    const char *data() const;
    void	setData( const char *data, uint size );

    bool	play( QPainter * );

    bool	load( const char *fileName );
    bool	save( const char *fileName );

protected:
    bool	cmd( int, QPainter *, QPDevCmdParam * );
    int		metric( int ) const;

private:
    bool	exec( QPainter *, QDataStream &, int );
    QBuffer	pictb;
    int		trecs;
    bool	formatOk;

private:	// Disabled copy constructor and operator=
    QPicture( const QPicture & );
    QPicture &operator=( const QPicture & );
};


inline bool QPicture::isNull() const
{
    return pictb.buffer().isNull();
}

inline uint QPicture::size() const
{
    return pictb.buffer().size();
}

inline const char *QPicture::data() const
{
    return pictb.buffer().data();
}


#endif // QPICTURE_H
