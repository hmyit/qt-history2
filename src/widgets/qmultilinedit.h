/**********************************************************************
** $Id: //depot/qt/main/src/widgets/qmultilinedit.h#50 $
**
** Definition of QMultiLineEdit widget class
**
** Created : 961005
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees with valid Qt Professional Edition licenses may distribute and
** use this file in accordance with the Qt Professional Edition License
** provided at sale or upon request.
**
** See http://www.troll.no/pricing.html or email sales@troll.no for
** information about the Professional Edition licensing, or see
** http://www.troll.no/qpl/ for QPL licensing information.
**
*****************************************************************************/

#ifndef QMULTILINEDIT_H
#define QMULTILINEDIT_H

#ifndef QT_H
#include "qtableview.h"
#include "qstring.h"
#include "qlist.h"
#endif // QT_H

struct QMultiLineData;

class Q_EXPORT QMultiLineEdit : public QTableView
{
    Q_OBJECT
public:
    QMultiLineEdit( QWidget *parent=0, const char *name=0 );
   ~QMultiLineEdit();

    QString textLine( int line ) const;
    QString text() const;

    int numLines() const;

    QSize sizeHint() const;
    QSizePolicy sizePolicy() const;

    bool	isReadOnly() const;
    bool	isOverwriteMode() const;

    virtual void	setFont( const QFont &font );
    virtual void insertLine( const QString &s, int line = -1 );
    virtual void insertAt( const QString &s, int line, int col );
    virtual void removeLine( int line );

    void 	cursorPosition( int *line, int *col ) const;
    virtual void	setCursorPosition( int line, int col, bool mark = FALSE );
    void	getCursorPosition( int *line, int *col );
    bool	atBeginning() const;
    bool	atEnd() const;

    bool	autoUpdate()	const;
    virtual void	setAutoUpdate( bool );

    virtual void	setFixedVisibleLines( int lines );


    int 	maxLineWidth() const;

public slots:
    void       clear();
    virtual void       setText( const QString &);
    void       append( const QString &);
    void       deselect();
    void       selectAll();
    virtual void       setReadOnly( bool );
    virtual void       setOverwriteMode( bool );
    void       paste();
    void       copyText();
    void       cut();

signals:
    void	textChanged();
    void	returnPressed();

protected:
    void	paintCell( QPainter *, int row, int col );

    void	mousePressEvent( QMouseEvent * );
    void	mouseMoveEvent( QMouseEvent * );
    void	mouseReleaseEvent( QMouseEvent * );
    void	mouseDoubleClickEvent( QMouseEvent * );
    void 	wheelEvent( QWheelEvent * );
    void	keyPressEvent( QKeyEvent * );
    void	focusInEvent( QFocusEvent * );
    void	focusOutEvent( QFocusEvent * );
    void	timerEvent( QTimerEvent * );
    void	leaveEvent( QEvent * );
    void	resizeEvent( QResizeEvent * );

    bool	hasMarkedText() const;
    QString	markedText() const;
    int		textWidth( int );
    int		textWidth( const QString &);

    QPoint	cursorPoint() const;

protected:
    virtual void insert( const QString& );
    virtual void newLine();
    virtual void killLine();
    virtual void pageUp( bool mark=FALSE );
    virtual void pageDown( bool mark=FALSE );
    virtual void cursorLeft( bool mark=FALSE, bool wrap = TRUE );
    virtual void cursorRight( bool mark=FALSE, bool wrap = TRUE );
    virtual void cursorUp( bool mark=FALSE );
    virtual void cursorDown( bool mark=FALSE );
    virtual void backspace();
    virtual void del();
    virtual void home( bool mark=FALSE );
    virtual void end( bool mark=FALSE );

    bool	getMarkedRegion( int *line1, int *col1,
				 int *line2, int *col2 ) const;
    int		lineLength( int row ) const;
    QString	*getString( int row ) const;

protected:    
    bool	cursorOn;	
    
private slots:
    void	clipboardChanged();

private:
    QList<QString> *contents;
    QMultiLineData *mlData;
    void insertChar( char );

    bool	readOnly;
    bool	dummy;
    bool	markIsOn;
    bool	dragScrolling ;
    bool	dragMarking;
    bool	textDirty;
    bool	wordMark;
    bool	overWrite;

    int		cursorX;
    int		cursorY;
    int		markAnchorX;
    int		markAnchorY;
    int		markDragX;
    int		markDragY;
    int		curXPos;	// cell coord of cursor
    int		blinkTimer;
    int		scrollTimer;

    int		mapFromView( int xPos, int row );
    int		mapToView( int xIndex, int row );

    virtual void	setWidth( int );
    void	updateCellWidth();
    bool 	partiallyInvisible( int row );
    void	makeVisible();
    virtual void	setBottomCell( int row );

    void 	newMark( int posx, int posy, bool copy=TRUE );
    void 	markWord( int posx, int posy );
    int 	charClass( char );
    void	turnMarkOff();

private:	// Disabled copy constructor and operator=
#if defined(Q_DISABLE_COPY)
    QMultiLineEdit( const QMultiLineEdit & );
    QMultiLineEdit &operator=( const QMultiLineEdit & );
#endif
};

inline bool QMultiLineEdit::isReadOnly() const { return readOnly; }

inline bool QMultiLineEdit::isOverwriteMode() const { return overWrite; }

inline void QMultiLineEdit::setOverwriteMode( bool on )
{
    overWrite = on;
 }

inline int QMultiLineEdit::lineLength( int row ) const
{
    return contents->at( row )->length();
}

inline bool QMultiLineEdit::atEnd() const
{
    return cursorY == (int)contents->count() - 1
	&& cursorX == lineLength( cursorY ) ;
}

inline bool QMultiLineEdit::atBeginning() const
{
    return cursorY == 0 && cursorX == 0;
}

inline QString *QMultiLineEdit::getString( int row ) const
{
    return contents->at( row );
}

inline int QMultiLineEdit::numLines() const
{
    return contents->count();
}
#endif // QMULTILINED_H
