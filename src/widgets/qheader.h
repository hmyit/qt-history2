/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qheader.h#19 $
**
** Definition of QHeader widget class (table header)
**
** Created : 961105
**
** Copyright (C) 1996-1997 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QHEADER_H
#define QHEADER_H

#ifndef QT_H
#include "qtableview.h"
#endif // QT_H

struct QHeaderData;

class QHeader : public QTableView
{
    Q_OBJECT
public:
    enum Orientation { Horizontal, Vertical };

    QHeader( QWidget *parent=0, const char *name=0 );
    QHeader( int, QWidget *parent=0, const char *name=0 );
    ~QHeader();

    int		addLabel( const char *, int size = -1 );
    void	setLabel( int, const char *, int size = -1 );
    const char*	label( int );
    void	setOrientation( Orientation );
    Orientation orientation() const;
    void	setTracking( bool enable );
    bool	tracking() const;

    void 	setClickEnabled( bool, int logIdx = -1 );
    void	setResizeEnabled( bool, int logIdx = -1 );
    void	setMovingEnabled( bool );

    void	setCellSize( int i, int s );
    int		cellSize( int i ) const;
    int		cellPos( int i ) const;
    int		cellAt( int i ) const;
    int		count() const;

    int 	offset() const;

    QSize	sizeHint() const;

    int		mapToLogical( int ) const;
    int		mapToActual( int ) const;

public slots:
    void	setOffset( int );

signals:
    void	sectionClicked( int );
    void	sizeChange( int section, int oldSize, int newSize );
    void	moved( int from, int to );
protected:
    //    void	timerEvent( QTimerEvent * );

    void	resizeEvent( QResizeEvent * );

    QRect	sRect( int i );

    void	paintCell( QPainter *, int, int );
    void	setupPainter( QPainter * );

    int		cellHeight( int );
    int		cellWidth( int );

    void	mousePressEvent( QMouseEvent * );
    void	mouseReleaseEvent( QMouseEvent * );
    void	mouseMoveEvent( QMouseEvent * );

private:
    void	init( int );

    void	paintRect( int p, int s );
    void	markLine( int idx );
    void	unMarkLine( int idx );
    int		pPos( int i ) const;
    int		pSize( int i ) const;

    int 	findLine( int );

    void	handleColumnResize(int, int, bool);

    void	moveAround( int fromIdx, int toIdx );

    int		handleIdx;
    int		oldHIdxSize;
    int		moveToIdx;
    enum State { Idle, Sliding, Pressed, Moving, Blocked };
    State	state;
    QCOORD	clickPos;
    bool	trackingIsOn;

    Orientation orient;

    QHeaderData *data;

private:	// Disabled copy constructor and operator=
    QHeader( const QHeader & );
    QHeader &operator=( const QHeader & );
};


inline QHeader::Orientation QHeader::orientation() const
{
    return orient;
}

inline void QHeader::setTracking( bool enable ) { trackingIsOn = enable; }
inline bool QHeader::tracking() const { return trackingIsOn; }

#endif //QHEADER_H
