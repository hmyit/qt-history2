/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qsocknot.h#1 $
**
** Definition of QSocketNotifier class
**
** Author  : Haavard Nord
** Created : 951114
**
** Copyright (C) 1995 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QSOCKNOT_H
#define QSOCKNOT_H

#include "qobject.h"


class QSocketNotifier : public QObject
{
    Q_OBJECT
public:
    enum Type { Read, Write, Exception };

    QSocketNotifier( int socket, Type, QObject *parent=0, const char *name=0 );
   ~QSocketNotifier();

    int		socket()	const;
    Type	type()		const;

    bool	enabled()	const;
    void	setEnabled( bool );

signals:
    void	activated( int socket );

protected:
    bool	event( QEvent * );

private:
    int		sockfd;
    Type	sntype;
    bool	snenabled;
};


inline int QSocketNotifier::socket() const
{ return sockfd; }

inline QSocketNotifier::Type QSocketNotifier::type() const
{ return sntype; }

inline bool QSocketNotifier::enabled() const
{ return snenabled; }


#endif // QSOCKNOT_H
