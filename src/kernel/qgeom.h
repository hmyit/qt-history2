/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qgeom.h#17 $
**
**  Geometry Management
**
**  Created:  960416
**
** Copyright (C) 1996 by Troll Tech AS.	 All rights reserved.
**
*****************************************************************************/

#ifndef QGEOM_H
#define QGEOM_H

#include "qbasic.h"
#include "qlist.h"

class QLayout
{
public:
    virtual ~QLayout();
    int defaultBorder() const { return defBorder; }

    bool activate();
    void freeze( int w, int h );
    void freeze() { freeze( 0, 0 ); }
    
    const char *name() const { return objName; }

protected:
    QLayout( QWidget *parent,  int border,
	     int autoBorder, const char *name );
    QLayout( int autoBorder = -1, const char *name=0 );

    QBasicManager *basicManager() { return bm; }
    virtual QChain *mainVerticalChain() = 0;
    virtual QChain *mainHorizontalChain() = 0;

    virtual void initBM() = 0;
    void addChildLayout( QLayout *);

    static QChain *verChain( QLayout *l ) { return l->mainVerticalChain(); }
    static QChain *horChain( QLayout *l ) { return l->mainHorizontalChain(); }

private:
    const char *objName;
    QBasicManager * bm;
    QLayout *parentLayout;
    QList<QLayout> *children;
    int defBorder;
    bool    topLevel;

private:	// Disabled copy constructor and operator=
    QLayout( const QLayout & ) {}
    QLayout &operator=( const QLayout & ) { return *this; }

};


class QBoxLayout : public QLayout
{
public:
    QBoxLayout( QWidget *parent, QBasicManager::Direction, int border=0,
		int autoBorder = -1, const char *name=0 );

    QBoxLayout(	QBasicManager::Direction, int autoBorder = -1,
		const char *name=0 );

    ~QBoxLayout();
    enum alignment { alignCenter, alignTop, alignLeft,
		     alignBottom, alignRight };

    void addWidget( QWidget *, int stretch = 0, alignment a = alignCenter );
    void addSpacing( int size );
    void addStretch( int stretch = 0 );
    void addLayout( QLayout *layout, int stretch = 0 );

    QBasicManager::Direction direction() const { return dir; }

    void addStrut( int );
protected:
    QChain *mainVerticalChain();
    QChain *mainHorizontalChain();
    void initBM();

private:
    void addB( QLayout *, int stretch );

    QBasicManager::Direction dir;
    QChain * parChain;
    QChain * serChain;
    bool    pristine;

private:	// Disabled copy constructor and operator=
    QBoxLayout( const QBoxLayout & ) : QLayout(0) {}
    QBoxLayout &operator=( const QBoxLayout & ) { return *this; }

};



class QGridLayout : public QLayout
{
public:
    QGridLayout( QWidget *parent, int nRows, int nCols, int border=0,
		 int autoBorder = -1, const char *name=0 );
    QGridLayout( int nRows, int nCols, int autoBorder = -1,
		 const char *name=0 );
    ~QGridLayout();
    void addWidget( QWidget *, int row, int col, int align = 0 );
    void addMultiCellWidget( QWidget *, int fromRow, int toRow, 
			       int fromCol, int toCol, int align = 0 );
    void addLayout( QLayout *layout, int row, int col);

    void setRowStretch( int row, int stretch );
    void setColStretch( int col, int stretch );
    //void addStrut( int size, int col);

protected:
    QChain *mainVerticalChain() { return verChain; }
    QChain *mainHorizontalChain() { return horChain; }
    void initBM();

private:
    QArray<QChain*> *rows;
    QArray<QChain*> *cols;

    QChain *horChain;
    QChain *verChain;
    void init ( int r, int c );

    int rr;
    int cc;

private:	// Disabled copy constructor and operator=
    QGridLayout( const QGridLayout & ) :QLayout(0) {}
    QGridLayout &operator=( const QGridLayout & ) { return *this; }
};

#endif
