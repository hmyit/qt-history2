/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qwidgetstack.h#10 $
**
** Definition of QWidgetStack class
**
** Created : 980306
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

#ifndef QWIDGETSTACK_H
#define QWIDGETSTACK_H

#ifndef QT_H
#include "qframe.h"
#include "qintdict.h"
#endif // QT_H


class QWidgetStackPrivate;

class QGridLayout;


class QWidgetStack: public QFrame
{
    Q_OBJECT
public:
    QWidgetStack( QWidget * parent = 0, QString name = 0 );
    ~QWidgetStack();

    void addWidget( QWidget *, int );

    void removeWidget( QWidget * );

    void show();

    QWidget * widget( int ) const;
    int id( QWidget * ) const;

    QWidget * visibleWidget() const;
    
    bool event( QEvent * );

signals:
    void aboutToShow( int );
    void aboutToShow( QWidget * );

public slots:
    void raiseWidget( int );
    void raiseWidget( QWidget * );

protected:
    void frameChanged();

    virtual void setChildGeometries();

private:
    bool isMyChild( QWidget * );
    QWidgetStackPrivate * d;
    QIntDict<QWidget> * dict;
    QGridLayout * l;
    QWidget * topWidget;
};


#endif
