/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qpicture.h#23 $
**
** Definition of QPicture class
**
** Created : 940729
**
** Copyright (C) 1992-1998 Troll Tech AS.  All rights reserved.
**
** This file is part of Troll Tech's internal development tree for Qt.
**
** This header text will be replaced by an appropriate text by the
** mkdist script which generates external distributions.
**
** If you are using the Qt Professional Edition or the Qt Free Edition,
** please notify Troll Tech at <info@troll.no> if you see this text.
**
** To Troll Tech developers: This header was generated by the script
** fixcopyright-int. It has the same number of text lines as the free
** and professional editions to avoid line number inconsistency.
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
    QString data() const;
    virtual void	setData( QString data, uint size );

    bool	play( QPainter * );

    bool	load( QString fileName );
    bool	save( QString fileName );

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

inline QString QPicture::data() const
{
    return pictb.buffer().data();
}


#endif // QPICTURE_H
