/****************************************************************************
** $Id: //depot/qt/main/src/tools/qstrvec.h#17 $
**
** Definition of QStrVec and QStrIVec classes
**
** Created : 931203
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

#ifndef QSTRVEC_H
#define QSTRVEC_H

#ifndef QT_H
#include "qstring.h"
#include "qvector.h"
#include "qdatastream.h"
#endif // QT_H


#if defined(QT_DLL)
template class Q_EXPORT QVector<char>
#endif

typedef QVector<char> QStrVecBase;


class Q_EXPORT QStrVec : public QStrVecBase
{
public:
    QStrVec()  { dc = TRUE; }
    QStrVec( uint size, bool deepc = TRUE ) : QStrVecBase(size) {dc=deepc;}
   ~QStrVec()  { clear(); }
private:
    GCI	 newItem( GCI d )	{ return dc ? qstrdup( (const char*)d ) : d; }
    void deleteItem( GCI d )	{ if ( dc ) delete[] (char*)d; }
    int	 compareItems( GCI s1, GCI s2 )
				{ return strcmp((const char*)s1,
						(const char*)s2); }
    QDataStream &read( QDataStream &s, GCI &d )
				{ s >> (char *&)d; return s; }
    QDataStream &write( QDataStream &s, GCI d ) const
				{ return s << (const char*)d; }
    bool dc;
};


class Q_EXPORT QStrIVec : public QStrVec	// case insensitive string vec
{
public:
    QStrIVec() {}
    QStrIVec( uint size, bool dc = TRUE ) : QStrVec( size, dc ) {}
   ~QStrIVec() { clear(); }
private:
    int	 compareItems( GCI s1, GCI s2 )
				{ return stricmp((const char*)s1,
						 (const char*)s2); }
};


#endif // QSTRVEC_H
