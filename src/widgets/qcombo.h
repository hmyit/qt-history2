/**********************************************************************
** $Id: //depot/qt/main/src/widgets/qcombo.h#9 $
**
** Definition of QComboBox class
**
** Author  : Eirik Eng
** Created : 950426
**
** Copyright (C) 1995 by Troll Tech AS.	 All rights reserved.
**
***********************************************************************/

#ifndef QCOMBO_H
#define QCOMBO_H

#include "qwidget.h"


struct QComboData;
class  QStrList;


class QComboBox : public QWidget
{
    Q_OBJECT
public:
    QComboBox( QWidget *parent=0, const char *name=0 );
   ~QComboBox();

    int		count() const;

    void	insertStrList( const QStrList *, int index=-1 );
    void	insertStrList( const char**, int numStrings=-1, int index=-1);

    void	insertItem( const char *string, int index=-1 );
    void	insertItem( const QPixmap &pixmap, int index=-1 );

    void	removeItem( int index );
    void	clear();

    const char *string( int index ) const;
    QPixmap    *pixmap( int index ) const;

    void	changeItem( const char *string, int index );
    void	changeItem( const QPixmap &pixmap, int index );

    int		currentItem()	const;
    void	setCurrentItem( int index );

    bool	autoResize()	const;
    void	setAutoResize( bool );
    void	adjustSize();

    void	setBackgroundColor( const QColor & );
    void	setPalette( const QPalette & );
    void	setFont( const QFont & );

signals:
    void	activated( int index );
    void	highlighted( int index );

private slots:
    void	internalActivate( int );
    void	internalHighlight( int );

protected:
    void	paintEvent( QPaintEvent * );
    void	mousePressEvent( QMouseEvent * );
    void	mouseMoveEvent( QMouseEvent * );
    void	mouseReleaseEvent( QMouseEvent * );
    void	mouseDoubleClickEvent( QMouseEvent * );
    void	keyPressEvent( QKeyEvent *e );

    void	popup();

private:
    void	init();
    void	reIndex();
    void	currentChanged();

    QComboData	*d;
};


#endif // QCOMBOBOX_H
