/****************************************************************************
** $Id: //depot/qt/main/src/tools/qfile.h#26 $
**
** Definition of QFile class
**
** Created : 930831
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

#ifndef QFILE_H
#define QFILE_H

#ifndef QT_H
#include "qiodevice.h"
#include "qstring.h"
#include <stdio.h>
#endif // QT_H

class QDir;


class QFile : public QIODevice			// file I/O device class
{
public:
    QFile();
    QFile( QString name );
   ~QFile();

    QString name()	const;
    void	setName( QString name );

    bool	exists()   const;
    static bool exists( QString fileName );

    bool	remove();
    static bool remove( QString fileName );

    bool	open( int );
    bool	open( int, FILE * );
    bool	open( int, int );
    void	close();
    void	flush();

    uint	size()	const;
    int		at()	const;
    bool	at( int );
    bool	atEnd() const;

    int		readBlock( char *data, uint len );
    int		writeBlock( const char *data, uint len );
    int		readLine( char *data, uint maxlen );
    int		readLine( QString&, uint maxlen );

    int		getch();
    int		putch( int );
    int		ungetch( int );

    int		handle() const;

protected:
    QString	fn;
    FILE       *fh;
    int		fd;
    int		length;
    bool	ext_f;

private:
    void	init();

private:	// Disabled copy constructor and operator=
    QFile( const QFile & );
    QFile &operator=( const QFile & );
};


inline QString QFile::name() const
{ return fn; }

inline int QFile::at() const
{ return index; }


#endif // QFILE_H
