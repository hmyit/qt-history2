/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qaccel.h#7 $
**
** Definition of QAccel class
**
** Author  : Haavard Nord
** Created : 950419
**
** Copyright (C) 1995 by Troll Tech AS.	 All rights reserved.
**
*****************************************************************************/

#ifndef QACCEL_H
#define QACCEL_H

#include "qobject.h"
#include "qkeycode.h"


class QAccelList;				// internal class


class QAccel : public QObject			// accelerator class
{
    Q_OBJECT
public:
    QAccel( QWidget *parent, const char *name=0 );
   ~QAccel();

    void	enable();			// enable accelerator
    void	disable();			// disable accelerator
    bool	isDisabled()	const	{ return !enabled; }

    int		count() const;

    void	insertItem( long key, int id=-1 );
    void	removeItem( int id );
    void	clear();

    long	key( int id );
    int		findKey( long key ) const;

    bool	isItemDisabled( int id ) const;
    bool	isItemEnabled( int id )	 const	{ return !isItemDisabled(id); }
    void	setItemEnabled( int id, bool enable );
    void	enableItem( int id )		{ setItemEnabled( id, TRUE ); }
    void	disableItem( int id )		{ setItemEnabled( id, FALSE );}

    bool	connectItem( int id,		// connect item to method
			     const QObject *receiver, const char *member );
    bool	disconnectItem( int id,
				const QObject *receiver, const char *member );

signals:
    void	activated( int id );		// key activated
    void	destroyed();			// accelerator destroyed

protected:
    bool	event( QEvent * );

private:
    QAccelList *aitems;
    bool	enabled;
};


#endif // QACCEL_H
