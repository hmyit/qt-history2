/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qiconset.h#6 $
**
** Definition of QIconSet class
**
** Created : 980318
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

#ifndef QICONSET_H
#define QICONSET_H

#include "qpixmap.h"


struct QIconSetPrivate;


class QIconSet
{
public:
    enum Size { Automatic, Small, Large };

    enum Mode { Normal, Disabled, Active };

    QIconSet( const QPixmap &, Size = Automatic );
    QIconSet( const QIconSet & );
    virtual ~QIconSet();

    void reset( const QPixmap &, Size );

    virtual void setPixmap( const QPixmap &, Size, Mode = Normal );
    virtual void setPixmap( QString , Size, Mode = Normal );
    QPixmap pixmap( Size, Mode ) const;
    QPixmap pixmap() const;
    bool isGenerated( Size, Mode ) const;

    QIconSet &operator=( const QIconSet & );

private:
    QIconSetPrivate * d;
};


#endif
