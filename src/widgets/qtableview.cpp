/**********************************************************************
** $Id: //depot/qt/main/src/widgets/qtableview.cpp#39 $
**
** Implementation of QTableView class
**
** Created : 941115
**
** Copyright (C) 1994-1996 by Troll Tech AS.  All rights reserved.
**
** --------------------------------------------------------------------
** NOTE: The hSbActive and vSbActive flags must be set and reset ONLY
**	 in the setHorScrollBar and setVerScrollBar functions. If not,
**	 scroll bar update optimizing will not work and strange things
**	 might happen.
***********************************************************************/

#include "qtablevw.h"
#include "qscrbar.h"
#include "qpainter.h"
#include "qdrawutl.h"
#include <limits.h>

RCSTAG("$Id: //depot/qt/main/src/widgets/qtableview.cpp#39 $");


const int sbDim = 16;

enum ScrollBarDirtyFlags {
    verGeometry	  = 0x01,
    verSteps	  = 0x02,
    verRange	  = 0x04,
    verValue	  = 0x08,
    horGeometry	  = 0x10,
    horSteps	  = 0x20,
    horRange	  = 0x40,
    horValue	  = 0x80,
    verMask	  = 0x0F,
    horMask	  = 0xF0
};


class CornerSquare : public QWidget		// internal class
{
public:
    CornerSquare( QWidget *, const char * = 0 );
    void paintEvent( QPaintEvent * );
};

CornerSquare::CornerSquare( QWidget *parent, const char *name )
	: QWidget( parent, name )
{
}

void CornerSquare::paintEvent( QPaintEvent * )
{
    if ( style() == MotifStyle ) {
	QPainter p;
	QColorGroup g = colorGroup();
	p.begin( this );
	qDrawShadePanel( &p, rect(), colorGroup() );
	p.end();
    }
}


/*!
  \class QTableView qtablevw.h
  \brief This is the abstract base class of all the table views.

  \ingroup abstractwidgets

  A table view consists of a number of abstract cells organized in rows
  and columns and a visible part called a view.

  The behavior of the widget can be finely tuned using
  setTableFlags(); subclasses like QListBox are little more than a
  call to setTableFlags(), some table content manipulation, and an
  implementation of paintCell().  Subclasses that need cells with
  variable width or height must reimplement cellHeight() and/or
  cellWidth().

  When you read this documentation, it is important to understand the
  distinctions between the four coordinate systems involved.

  <ol>
  <li> The cell coordinates.  (0,0) is the top left corner of a cell.
  This is used by functions such as paintCell().

  <li> The table coordinates.  (0,0) is the top left corner of the cell at
  row 0 and column 0, and uses absolute x and y coordinates.  This is used
  by functions such as setXOffset() or maxYOffset().

  <li> The widget coordinates.	(0,0) is the top left corner of the
  widget, \e including the frame.  This is used by functions such as
  repaint().

  <li> The view coordinates.  (0,0) is the top left corner of the
  view, \e excluding the frame.	 This is the least-used coordinate
  system, used by functions such as viewWidth().
  </ol>

  It is rather unfortunate that we have to use four different
  coordinate systems, but if we were to provide a flexible and
  powerful base class, there wasn't any way around it.
*/


/*!
  Constructs a table view.  All the arguments are passed to the QFrame
  constructor.

  The \link setTableFlags() table flags\endlink are all cleared (set to zero).
  Set \c Tbl_autoVScrollBar or \c Tbl_autoHScrollBar to get automatic scroll
  bars and \c Tbl_clipCellPainting to get safe clipping.

  The \link setCellHeight() cell height\endlink and \link setCellWidth()
  cell width\endlink are set to 0.

  Frame line shapes (QFrame::HLink and QFrame::VLine) are disallowed,
  see QFrame::setStyle().
*/

QTableView::QTableView( QWidget *parent, const char *name, WFlags f )
    : QFrame( parent, name, f, FALSE )
{
    initMetaObject();

    nRows		 = nCols      = 0;	// zero rows/cols
    xCellOffs		 = yCellOffs  = 0;	// zero offset
    xCellDelta		 = yCellDelta = 0;	// zero cell offset
    xOffs		 = yOffs      = 0;	// zero total pixel offset
    cellH		 = cellW      = 0;	// user defined cell size
    tFlags		 = 0;
    doUpdate		 = TRUE;
    vScrollBar		 = hScrollBar = 0;	// no scroll bars
    cornerSquare	 = 0;
    sbDirty		 = 0;
    eraseInPaint	 = FALSE;
    verSliding		 = FALSE;
    verSnappingOff	 = FALSE;
    horSliding		 = FALSE;
    horSnappingOff	 = FALSE;
    coveringCornerSquare = FALSE;
    inSbUpdate		 = FALSE;
}

/*!
  Destroys the table view.
*/

QTableView::~QTableView()
{
    delete vScrollBar;
    delete hScrollBar;
    delete cornerSquare;
}


/*!
  \internal
  Reimplements QWidget::setBackgroundColor() for binary compatibility.
  \sa setPalette()
*/

void QTableView::setBackgroundColor( const QColor &c )
{
    QWidget::setBackgroundColor( c );
}

/*!
  Reimplements QWidget::setPalette().
  Sets the palette for the scroll bars, too.
  \sa setBackgroundColor()
*/

void QTableView::setPalette( const QPalette &p )
{
    QWidget::setPalette( p );
    if ( cornerSquare )
	cornerSquare->setPalette( p );
    if ( vScrollBar )
	vScrollBar->setPalette( p );
    if ( hScrollBar )
	hScrollBar->setPalette( p );
}

/*!
  Reimplements QWidget::show() in order to do scroll bar tricks.
*/

void QTableView::show()
{
    showOrHideScrollBars();
    QWidget::show();
}


/*!
  \overload void QTableView::repaint( bool erase )
  Repaints the entire view.
*/

/*!
  Repaints the table view directly by calling paintEvent() directly,
  unless updates are disabled.

  Erases the view area \e (x,y,w,h) if \e erase is TRUE.

  If \e w is negative, it is replaced with <code>width() - x</code>.
  If \e h is negative, it is replaced width <code>height() - y</code>.

  Doing a repaint() usually is faster than doing an update(), but
  calling update() many times in a row will generate a single paint
  event.

  At present, QTableView is the only widget that reimplements \link
  QWidget::repaint() repaint()\endlink.	 It does this because by
  clearing and then repainting one cell at at time, it can make the
  screen flicker less than it would otherwise.
*/

void QTableView::repaint( int x, int y, int w, int h, bool erase )
{
    if ( !isVisible() )				// ignore if not visible
	return;
    if ( w < 0 )
	w = width()  - x;
    if ( h < 0 )
	h = height() - y;
    QPaintEvent e( QRect(x,y,w,h) );
    if ( erase )
	eraseInPaint = TRUE;			// erase when painting
    paintEvent( &e );
    eraseInPaint = FALSE;
}

/*!
  \overload void QTableView::repaint( const QRect &r, bool erase )
*/


/*!
  \fn int QTableView::numRows() const
  Returns the number of rows in the table.
  \sa numCols(), setNumRows()
*/

/*!
  Sets the number of rows of the table to \e rows (must be non-negative).

  The table repaints itself automatically if autoUpdate() is set.

  \sa numCols(), setNumCols(), numRows()
*/

void QTableView::setNumRows( int rows )
{
    if ( rows < 0 ) {
#if defined(CHECK_RANGE)
	warning( "QTableView::setNumRows: Negative argument." );
#endif
	return;
    }
    if ( nRows == rows )
	return;
    int oldRows = nRows;
    nRows = rows;
    if ( autoUpdate() && isVisible() ) {
	int maxRow  = lastRowVisible();
	if ( maxRow < oldRows || maxRow < nRows )
	    repaint();
    }
    updateScrollBars( verRange );
}

/*!
  \fn int QTableView::numCols() const
  Returns the number of columns in the table
  \sa numRows(), setNumCols()
*/

/*!
  Sets the number of columns of the table to \e cols (must be non-negative).

  The table repaints itself automatically if autoUpdate() is set.

  \sa numCols(), numRows(), setNumRows()
*/

void QTableView::setNumCols( int cols )
{
    if ( cols < 0 ) {
#if defined(CHECK_RANGE)
	warning( "QTableView::setNumCols: Negative argument." );
#endif
	return;
    }
    if ( nCols == cols )
	return;
    int oldCols = nCols;
    nCols = cols;
    if ( autoUpdate() && isVisible() ) {
	int maxCol = lastColVisible();
	if ( maxCol < oldCols || maxCol < nCols )
	    repaint();
    }
    updateScrollBars( horRange );
}


/*!
  \fn int QTableView::topCell() const
  Returns the index of the first line in the table that is visible in
  the view.  The index of the very first line is 0.
  \sa leftCell(), setTopCell()
*/

/*!
  Scrolls the table such that \e row becomes the top row.
  The index of the very first line is 0.
  \sa setYOffset(), setTopLeftCell(), setLeftCell()
*/

void QTableView::setTopCell( int row )
{
    setTopLeftCell( row, -1 );
    return;
}

/*!
  \fn int QTableView::leftCell() const
  Returns the index of the first column in the table that is visible in
  the view.  The index of the very leftmost column is 0.
  \sa topCell(), setLeftCell()
*/

/*!
  Scrolls the table such that \e col becomes the leftmost
  column.  The index of the very leftmost column is 0.
  \sa setXOffset(), setTopLeftCell(), setTopCell()
*/

void QTableView::setLeftCell( int col )
{
    setTopLeftCell( -1, col );
    return;
}

/*!
  Scrolls the table such that the cell at row \e row and colum \e
  col becomes the top left cell in the view.  The cell at the extreme
  top left of the table is at position (0,0).
  \sa setLeftCell(), setTopCell(), setOffset()
*/

void QTableView::setTopLeftCell( int row, int col )
{
    int newX = xOffs;
    int newY = yOffs;

    if ( col >= 0 ) {
	if ( cellW ) {
	    newX = col*cellW;
	    if ( newX > maxXOffset() )
		newX = maxXOffset();
	} else {
	    newX = 0;
	    while ( col )
		newX += cellWidth( --col );   // optimize using current! ###
	}
    }
    if ( row >= 0 ) {
	if ( cellH ) {
	    newY = row*cellH;
	    if ( newY > maxYOffset() )
		newY = maxYOffset();
	} else {
	    newY = 0;
	    while ( row )
		newY += cellHeight( --row );   // optimize using current! ###
	}
    }
    setOffset( newX, newY );
}


/*!
  \fn int QTableView::xOffset() const

  Returns the coordinate in pixels of the table point which is
  currently on the left edge of the view.

  \sa setXOffset(), yOffset(), leftCell()
*/

/*!
  Scrolls the table such that \e x becomes the leftmost pixel in the view.

  The interaction with \link setTableFlags() Tbl_snapToHGrid
  \endlink is tricky.  If \e updateScrBars is TRUE, the scroll bars are
  updated.

  \sa xOffset(), setYOffset(), setOffset(), setLeftCell()
*/

void QTableView::setXOffset( int x )
{
    setOffset( x, yOffset() );
}

/*!
  \fn int QTableView::yOffset() const

  Returns the coordinate in pixels of the table point which is currently on
  the top edge of the view.
  \sa setYOffset(), xOffset(), topCell()
*/


/*!
  Scrolls the table such that \e y becomes the top pixel in the view.

  The interaction with \link setTableFlags() Tbl_snapToVGrid
  \endlink is tricky.  If \e updateScrBars is TRUE, the scroll bars are
  updated.

  \sa yOffset(), setXOffset(), setOffset(), setTopCell()
*/

void QTableView::setYOffset( int y )
{
    setOffset( xOffset(), y );
}

/*!
  Scrolls the table such that \e (x,y) becomes the top left pixel
  in the view.

  The interaction with \link setTableFlags() Tbl_snapTo*Grid \endlink
  is tricky.  If \e updateScrBars is TRUE, the scroll bars are
  updated.

  \sa xOffset(), yOffset(), setXOffset(), setYOffset(), setTopLeftCell()
*/

void QTableView::setOffset( int x, int y, bool updateScrBars )
{
    if ( (!testTableFlags(Tbl_snapToHGrid) || xCellDelta == 0) &&
	 (!testTableFlags(Tbl_snapToVGrid) || yCellDelta == 0) &&
	 (x == xOffs && y == yOffs) )
	return;

    if ( x < 0 )
	x = 0;
    if ( y < 0 )
	y = 0;

    if ( cellW ) {
	if ( x > maxXOffset() )
	    x = maxXOffset();
	xCellOffs = x / cellW;
	if ( !testTableFlags(Tbl_snapToHGrid) ) {
	    xCellDelta	= (short)(x % cellW);
	} else {
	    x		= xCellOffs*cellW;
	    xCellDelta	= 0;
	}
    } else {
	int xn=0, xcd, col = 0;
	while ( col < nCols && x > xn+(xcd=cellWidth(col)) ) {
	    xn += xcd;
	    col++;
	}
	xCellOffs = col;
	if ( testTableFlags(Tbl_snapToHGrid) ) {
	    xCellDelta = 0;
	    x = xn;
	} else {
	    xCellDelta = (short)(x-xn);
	}
    }
    if ( cellH ) {
	if ( y > maxYOffset() )
	    y = maxYOffset();
	yCellOffs = y / cellH;
	if ( !testTableFlags(Tbl_snapToVGrid) ) {
	    yCellDelta	= (short)(y % cellH);
	} else {
	    y		= yCellOffs*cellH;
	    yCellDelta	= 0;
	}
	yCellDelta  = (short)(y % cellH);
    } else {
	int yn=0, yrd, row=0;
	while ( row < nRows && y > yn+(yrd=cellHeight(row)) ) {
	    yn += yrd;
	    row++;
	}
	yCellOffs = row;
	if ( testTableFlags(Tbl_snapToHGrid) ) {
	    yCellDelta = 0;
	    y = yn;
	} else {
	    yCellDelta = (short)(y-yn);
	}
    }
    int dx = (x - xOffs);
    int dy = (y - yOffs);
    xOffs = x;
    yOffs = y;
    if ( autoUpdate() && isVisible() )
	scroll( dx, dy );
    if ( updateScrBars )
	updateScrollBars( verValue | horValue );
}


/*!
  \fn int QTableView::cellWidth() const

  Returns the column width, in pixels.	Returns 0 if the columns have
  variable widths.

  \sa setCellWidth(), cellHeight()
*/

/*!
  Returns the width of column \e col, in pixels.

  This function is virtual and must be reimplemented by subclasses that
  have variable cell widths.

  \sa setCellWidth(), cellHeight(), totalWidth()
*/

int QTableView::cellWidth( int )
{
    return cellW;
}


/*!
  Sets the width in pixels of the table cells to \e cellWidth.

  Setting it to zero means that the column width is variable.  When
  set to 0 (this is the default) QTableView calls the virtual function
  cellWidth() to get the width.

  \sa cellWidth(), setCellHeight(), totalWidth(), numCols()
*/

void QTableView::setCellWidth( int cellWidth )
{
    if ( cellW == cellWidth )
	return;
#if defined(CHECK_RANGE)
    if ( cellWidth < 0 || cellWidth > SHRT_MAX ) {
	warning( "QTableView::setCellWidth: Argument out of range" );
	return;
    }
#endif
    cellW = (short)cellWidth;
    if ( autoUpdate() && isVisible() )
	repaint();
    updateScrollBars( horSteps | horRange );
}

/*!
  \fn int QTableView::cellHeight() const

  Returns the row height, in pixels.  Returns 0 if the rows have
  variable heights.

  \sa setCellHeight(), cellWidth()
*/


/*!
  Returns the height of row \e row, in pixels.

  This function is virtual and must be reimplemented by subclasses that
  have variable cell heights.

  \sa setCellHeight(), cellWidth(), totalHeight()
*/

int QTableView::cellHeight( int )
{
    return cellH;
}

/*!
  Sets the height in pixels of the table cells to \e cellHeight.

  Setting it to zero means that the row height is variable.  When set
  to 0 (this is the default) QTableView calls the virtual function
  cellHeight() to get the height.

  \sa cellHeight(), setCellWidth(), totalHeight(), numRows()
*/

void QTableView::setCellHeight( int cellHeight )
{
    if ( cellH == cellHeight )
	return;
#if defined(CHECK_RANGE)
    if ( cellHeight < 0 || cellHeight > SHRT_MAX ) {
	warning( "QTableView::setCellHeight: Argument out of range" );
	return;
    }
#endif
    cellH = (short)cellHeight;
    if ( autoUpdate() && isVisible() )
	repaint();
    updateScrollBars( verSteps | verRange );
}


/*!
  Returns the total width of the table in pixels.

  This function is virtual and should be reimplmented by subclasses that
  have variable cell widths and a non-trivial cellWidth() function, or a
  large number of columns in the table.

  The function (or rather, its default implementation) is rather slow
  for very wide tables.

  \sa cellWidth(), totalHeight()
*/

int QTableView::totalWidth()
{
    if ( cellW ) {
	return cellW*nCols;
    } else {
	int tw = 0;
	for( int i = 0 ; i < nCols ; i++ )
	    tw += cellWidth( i );
	return tw;
    }
}

/*!
  Returns the total height of the table in pixels.

  This function is virtual and should be reimplmented by subclasses that
  have variable cell heights and a non-trivial cellHeight() function, or a
  large number of rows in the table.

  The function (or rather, its default implementation) is rather slow
  for very tall tables.

  \sa cellHeight(), totalWidth()
*/

int QTableView::totalHeight()
{
    if ( cellH ) {
	return cellH*nRows;
    } else {
	int th = 0;
	for( int i = 0 ; i < nRows ; i++ )
	    th += cellHeight( i );
	return th;
    }
}


/*!
  \fn uint QTableView::tableFlags() const

  Returns the union of the table flags that are currently set.

  \sa setTableFlags(), clearTableFlags(), testTableFlags()
*/

/*!
  \fn bool QTableView::testTableFlags( uint f ) const

  Returns TRUE if any of the table flags in \e f are currently set,
  otherwise FALSE.

  \sa setTableFlags(), clearTableFlags(), tableFlags()
*/

/*!
  Sets the table flags to \e f.

  If a flag setting changes the appearance of the table the table is
  repainted if and only if autoUpdate() is TRUE.

  The table flags are mostly single bits, though there are some multibit
  flags for convenience. Here is a complete list:

  <dl compact>
  <dt> Tbl_vScrollBar <dd> The table has a vertical scroll bar.
  <dt> Tbl_hScrollBar <dd> The table has a horizontal scroll bar.
  <dt> Tbl_autoVScrollBar <dd> The table has a vertical scroll bar if
  and only if the table is taller than the view.
  <dt> Tbl_autoHScrollBar <dd> The table has a horizontal scroll bar if
  and only if the table is wider than the view.
  <dt> Tbl_autoScrollBars <dd> The union of the previous two flags.
  <dt> Tbl_clipCellPainting <dd> The table uses QPainter::setClipRect() to
  make sure that paintCell() will not draw outside the cell
  boundaries.
  <dt> Tbl_cutCellsV <dd> The table will never show part of a
  cell at the bottom of the table; if there is not space for all of
  a cell the space is left blank.
  <dt> Tbl_cutCellsH <dd> The table will never show part of a
  cell at the right side of the table; if there is not space for all of
  a cell the space is left blank.
  <dt> Tbl_cutCells <dd> The union of the previous two flags.
  <dt> Tbl_scrollLastHCell <dd> When the user scrolls horizontally,
  let him/her scroll the last cell leftward until it is at the left
  edge of the view.  If this flag is not set, the user can only scroll
  to the point where last cell is completely visible.
  <dt> Tbl_scrollLastVCell <dd> When the user scrolls vertically, let
  him/her scroll the last cell upward until it is at the top edge of
  the view.  If this flag is not set, the user can only scroll to the
  point where last cell is completely visible.
  <dt> Tbl_scrollLastCell <dd> The union of the previous two flags.
  <dt> Tbl_smoothHScrolling <dd> The table scrolls as smoothly as
  possible when the user scrolls horizontally. When this flag is not
  set scrolling is done one cell at a time.
  <dt> Tbl_smoothVScrolling <dd> The table scrolls as smoothly as
  possible when scrolling vertically. When this flag is not set
  scrolling is done one cell at a time.
  <dt> Tbl_smoothScrolling <dd> The union of of previous two flags.
  <dt> Tbl_snapToHGrid <dd> Except when the user is actually scrolling,
  the leftmost column shown snaps to the leftmost edge of the view.
  <dt> Tbl_snapToVGrid <dd> Except when the user is actually
  scrolling, the top line snaps to the top edge of the view.
  <dt> Tbl_snapToGrid <dd> The union of the previous two flags.
  </dl>

  You can specify more than one flag at a time using bitwise OR.

  Example:
  \code
    setTableFlags( Tbl_smoothScrolling | Tbl_autoScrollBars );
  \endcode

  \sa clearTableFlags(), testTableFlags(), tableFlags()
*/

void QTableView::setTableFlags( uint f )
{
    f = (f ^ tFlags) & f;			// clear flags already set
    tFlags |= f;

    bool updateOn = autoUpdate();
    setAutoUpdate( FALSE );

    uint repaintMask = Tbl_cutCellsV | Tbl_cutCellsH;

    if ( f & Tbl_vScrollBar ) {
	setVerScrollBar( TRUE );
    }
    if ( f & Tbl_hScrollBar ) {
	setHorScrollBar( TRUE );
    }
    if ( f & Tbl_autoVScrollBar ) {
	updateScrollBars( verRange );
    }
    if ( f & Tbl_autoHScrollBar ) {
	updateScrollBars( horRange );
    }
    if ( f & Tbl_scrollLastHCell ) {
	updateScrollBars( horRange );
    }
    if ( f & Tbl_scrollLastVCell ) {
	updateScrollBars( verRange );
    }
    if ( f & Tbl_snapToHGrid ) {
	updateScrollBars( horRange );
    }
    if ( f & Tbl_snapToVGrid ) {
	updateScrollBars( verRange );
    }
    if ( f & Tbl_snapToGrid ) {			// Note: checks for 2 flags
	if ( (f & Tbl_snapToHGrid) != 0 && xCellDelta != 0 || //have to scroll?
	     (f & Tbl_snapToVGrid) != 0 && yCellDelta != 0 ) {
	    snapToGrid( (f & Tbl_snapToHGrid) != 0,	// do snapping
			(f & Tbl_snapToVGrid) != 0 );
	    repaintMask |= Tbl_snapToGrid;	// repaint table
	}
    }

    if ( updateOn ) {
	setAutoUpdate( TRUE );
	updateScrollBars();
	if ( isVisible() && (f & repaintMask) )
	    repaint();
    }

}

/*!
  Clears the \link setTableFlags() table flags\endlink that are set
  in \e f.

  Example (clears a single flag):
  \code
    clearTableFlags( Tbl_snapToGrid );
  \endcode

  The default argument clears all flags.

  \sa setTableFlags(), testTableFlags(), tableFlags()
*/

void QTableView::clearTableFlags( uint f )
{
    f = (f ^ ~tFlags) & f;		// clear flags that are already 0
    tFlags &= ~f;

    bool updateOn = autoUpdate();
    setAutoUpdate( FALSE );

    uint repaintMask = Tbl_cutCellsV | Tbl_cutCellsH;

    if ( f & Tbl_vScrollBar ) {
	setVerScrollBar( FALSE );
    }
    if ( f & Tbl_hScrollBar ) {
	setHorScrollBar( FALSE );
    }
    if ( f & Tbl_scrollLastHCell ) {
	int maxX = maxXOffset();
	if ( xOffs > maxX ) {
	    setOffset( maxX, yOffs );
	    repaintMask |= Tbl_scrollLastVCell;
	}
	updateScrollBars( horRange );
    }
    if ( f & Tbl_scrollLastVCell ) {
	int maxY = maxYOffset();
	if ( yOffs > maxY ) {
	    setOffset( xOffs, maxY );
	    repaintMask |= Tbl_scrollLastVCell;
	}
	updateScrollBars( verRange );
    }
    if ( f & Tbl_smoothScrolling ) {	      // Note: checks for 2 flags
	if ((f & Tbl_smoothHScrolling) != 0 && xCellDelta != 0 ||//must scroll?
	    (f & Tbl_smoothVScrolling) != 0 && yCellDelta != 0 ) {
	    snapToGrid( (f & Tbl_smoothHScrolling) != 0,      // do snapping
			(f & Tbl_smoothVScrolling) != 0 );
	    repaintMask |= Tbl_smoothScrolling;		     // repaint table
	}
    }
    if ( f & Tbl_snapToHGrid ) {
	updateScrollBars( horRange );
    }
    if ( f & Tbl_snapToVGrid ) {
	updateScrollBars( verRange );
    }
    if ( updateOn ) {
	setAutoUpdate( TRUE );
	updateScrollBars();	     // returns immediately if nothing to do
	if ( isVisible() && (f & repaintMask) )
	    repaint();
    }

}


/*!
  \fn bool QTableView::autoUpdate() const

  Returns TRUE if the view updates itself automatically whenever it
  is changed in some way.

  \sa setAutoUpdate()
*/

/*!
  Sets the auto-update option of the table view to \e enable.

  If \e enable is TRUE (this is the default) then the view updates itself
  automatically whenever it has changed in some way (for example when a
  \link setTableFlags() flag\endlink is changed).

  If \e enable is FALSE, the view does NOT repaint itself, or update
  its internal state variables itself when it is changed.  This can be
  useful to avoid flicker during large changes, and is singularly
  useless otherwise: Disable auto-update, do the changes, re-enable
  auto-update, and call repaint().

  \warning Do not leave the view in this state for a long time
  (i.e. between events ). If, for example, the user interacts with the
  view when auto-update is off, strange things can happen.

  Setting auto-update to TRUE does not repaint the view, you must call
  repaint() to do this.

  \sa autoUpdate(), repaint()
*/

void QTableView::setAutoUpdate( bool enable )
{
    if ( (bool)doUpdate == enable )
	return;
    doUpdate = enable;
    if ( doUpdate )
	showOrHideScrollBars();
}


/*!
  Repaints the cell at row \e row, column \e col if it is inside the view.

  If \e erase is TRUE, the relevant part of the view is cleared to the
  background color/pixmap before the contents are repainted.

  \sa isVisible()
*/

void QTableView::updateCell( int row, int col, bool erase )
{
    int xPos, yPos;
    if ( !colXPos( col, &xPos ) )
	return;
    if ( !rowYPos( row, &yPos ) )
	return;
    QRect uR = QRect( xPos, yPos,
		      cellW ? cellW : cellWidth(col),
		      cellH ? cellH : cellHeight(row) );
    repaint( uR.intersect(viewRect()), erase );
}


/*!
  \fn QRect QTableView::cellUpdateRect() const

  This function should only be called from the updateCell() function
  in subclasses. It returns the portion of a cell that actually needs
  to be updated. This is only useful for non-trivial updateCell().
*/

/*!
  Returns the rectangle which is the actual table, excluding any
  frame, in the widget coordinate system.

  Somewhat similar to clientRect(), but does not include any frames.
*/

QRect QTableView::viewRect() const
{
    return QRect( frameWidth(), frameWidth(), viewWidth(), viewHeight() );
}


/*!
  Returns the index of the last (bottom) row in the view.
  The index of the first row is 0.

  If no rows are visible it returns -1.	 This can happen if the
  view is too small for the first row and Tbl_cutCellsV is set.

  \sa lastColVisible()
*/

int QTableView::lastRowVisible() const
{
    int cellMaxY;
    int row = findRawRow( maxViewY(), &cellMaxY );
    if ( row == -1 ) {				// maxViewY() past end?
	row = nRows - 1;			// yes: return last row
    } else {
	if ( testTableFlags(Tbl_cutCellsV) && cellMaxY > maxViewY() ) {
	    if ( row == yCellOffs )		// cut by right margin?
		return -1;			// yes, nothing in the view
	    else
	       row = row - 1;			// cut by margin, one back
	}
    }
    return row;
}

/*!
  Returns the index of the last (right) column in the view.
  The index of the first column is 0.

  If no columns are visible it returns -1.  This can happen if the
  view is too narrow for the first column and Tbl_cutCellsH is set.

  \sa lastRowVisible()
*/

int QTableView::lastColVisible() const
{
    int cellMaxX;
    int col = findRawCol( maxViewX(), &cellMaxX );
    if ( col == -1 ) {				// maxViewX() past end?
	col = nCols - 1;			// yes: return last col
    } else {
	if ( testTableFlags(Tbl_cutCellsH) && cellMaxX > maxViewX() ) {
	    if ( col == xCellOffs )		// cut by bottom margin?
		return -1;			// yes, nothing in the view
	    else
	       col = col - 1;			// cell by margin, one back
	}
    }
    return col;
}

/*!
  Returns TRUE if \e row is at least partially visible.
  \sa colIsVisible()
*/

bool QTableView::rowIsVisible( int row ) const
{
    return rowYPos( row, 0 );
}

/*!
  Returns TRUE if \e col is at least partially visible.
  \sa rowIsVisible()
*/

bool QTableView::colIsVisible( int col ) const
{
    return colXPos( col, 0 );
}


/*!
  \internal
  Called when both scroll bars are active at the same time. Covers the
  bottom left corner between the two scroll bars with an empty widget.
*/

void QTableView::coverCornerSquare( bool enable )
{
    coveringCornerSquare = enable;
    if ( !cornerSquare && enable ) {
	cornerSquare = new CornerSquare( this );
	CHECK_PTR( cornerSquare );
	cornerSquare->setGeometry( maxViewX() + frameWidth() + 2 ,
				   maxViewY() + frameWidth() + 2,
				   sbDim, sbDim );
    }
    if ( autoUpdate() && cornerSquare ) {
	if ( enable )
	    cornerSquare->show();
	else
	    cornerSquare->hide();
    }
}


/*!
  \internal
  Scroll the view to a position such that:

  If \e horizontal is TRUE, the leftmost column shown fits snugly
  with the left edge of the view.

  If \e vertical is TRUE, the top row shown fits snugly with the top
  of the view.

  You can achieve the same effect automatically by setting any of the
  \link setTableFlags() Tbl_snapTo*Grid \endlink table flags.
*/

void QTableView::snapToGrid( bool horizontal, bool vertical )
{
    int newXCell = -1;
    int newYCell = -1;
    if ( horizontal && xCellDelta != 0 ) {
	int w = cellW ? cellW : cellWidth( xCellOffs );
	if ( xCellDelta >= w/2 )
	    newXCell = xCellOffs + 1;
	else
	    newXCell = xCellOffs;
    }
    if ( vertical && yCellDelta != 0 ) {
	int h = cellH ? cellH : cellHeight( yCellOffs );
	if ( yCellDelta >= h/2 )
	    newYCell = yCellOffs + 1;
	else
	    newYCell = yCellOffs;
    }
    setTopLeftCell( newXCell, newYCell );
}

/*!
  \internal
  This internal slot is connected to the horizontal scroll bar's
  QScrollBar::valueChanged() signal.

  Moves the table horizontally to offset \e val without updating the
  scroll bar.
*/

void QTableView::horSbValue( int val )
{
    if ( horSliding ) {
	horSliding = FALSE;
	if ( horSnappingOff ) {
	    horSnappingOff = FALSE;
	    tFlags |= Tbl_snapToHGrid;
	}
    }
    setOffset( val, yOffs, FALSE );
}

/*!
  \internal
  This internal slot is connected to the horizontal scroll bar's
  QScrollBar::sliderMoved() signal.

  Scrolls the table smoothly horizontally even if \c Tbl_snapToHGrid is set.
*/

void QTableView::horSbSliding( int val )
{
    if ( testTableFlags(Tbl_snapToHGrid) &&
	 testTableFlags(Tbl_smoothHScrolling) ) {
	tFlags &= ~Tbl_snapToHGrid;	// turn off snapping while sliding
	setOffset( val, yOffs, FALSE );
	tFlags |= Tbl_snapToHGrid;	// turn on snapping again
    } else {
	setOffset( val, yOffs, FALSE );
    }
}

/*!
  \internal
  This internal slot is connected to the horizontal scroll bar's
  QScrollBar::sliderReleased() signal.
*/

void QTableView::horSbSlidingDone( )
{
    if ( testTableFlags(Tbl_snapToHGrid) &&
	 testTableFlags(Tbl_smoothHScrolling) )
	snapToGrid( TRUE, FALSE );
}

/*!
  \internal
  This internal slot is connected to the vertical scroll bar's
  QScrollBar::valueChanged() signal.

  Moves the table vertically to offset \e val without updating the
  scroll bar.
*/

void QTableView::verSbValue( int val )
{
    if ( verSliding ) {
	verSliding = FALSE;
	if ( verSnappingOff ) {
	    verSnappingOff = FALSE;
	    tFlags |= Tbl_snapToVGrid;
	}
    }
    setOffset( xOffs, val, FALSE );
}

/*!
  \internal
  This internal slot is connected to the vertical scroll bar's
  QScrollBar::sliderMoved() signal.

  Scrolls the table smoothly vertically even if \c Tbl_snapToVGrid is set.
*/

void QTableView::verSbSliding( int val )
{
    if ( testTableFlags(Tbl_snapToVGrid) &&
	 testTableFlags(Tbl_smoothVScrolling) ) {
	tFlags &= ~Tbl_snapToVGrid;	// turn off snapping while sliding
	setOffset( xOffs, val, FALSE );
	tFlags |= Tbl_snapToVGrid;	// turn on snapping again
    } else {
	setOffset( xOffs, val, FALSE );
    }
}

/*!
  \internal
  This internal slot is connected to the vertical scroll bar's
  QScrollBar::sliderReleased() signal.
*/

void QTableView::verSbSlidingDone( )
{
    if ( testTableFlags(Tbl_snapToVGrid) &&
	 testTableFlags(Tbl_smoothVScrolling) )
	snapToGrid( FALSE, TRUE );
}


/*!
  This virtual function is called before painting of table cells
  is started. It can be reimplemented by subclasses that want to
  to set up the painter in a special way and that do not want to
  do so for each cell.

  The default implementation does nothing.
*/

void QTableView::setupPainter( QPainter * )
{
}

/*!
  \fn void QTableView::paintCell( QPainter *p, int row, int col )

  This pure virtual function is called to paint the single cell at \e
  (row,col) using \e p, which is open when paintCell() is called and
  must remain open.

  The coordinate system is \link QPainter::translate() translated
  \endlink such that the origin is at the top left corner of the cell
  to be painted.  Do not scale or shear the coordinate system (or if
  you do, restore the transformation matrix before you return).

  By default, the painter is not clipped, which is probably not what
  you want.  Call setTableFlags(Tbl_clipCellPainting) to enable clipping.

  \sa paintEvent(), QPainter(), setTableFlags()
*/


/*!
  Handles paint events for the table view, by calling paintCell()
  and drawFrame().

  It is optimized to repaint only the cells that need to be repainted.
*/

void QTableView::paintEvent( QPaintEvent *e )
{
    QRect updateR = e->rect();			// update rectangle
#if DEBUG_INFO
    debug("Update rect = ( %i, %i, %i, %i )",
	   updateR.x(),updateR.y(), updateR.width(), updateR.height() );
#endif

    if ( sbDirty ) {
	bool e = eraseInPaint;
	updateScrollBars();
	eraseInPaint = e;
    }

    QPainter paint;
    paint.begin( this );

    if ( !contentsRect().contains( updateR ) ) {// update frame ?
	drawFrame( &paint );
	if ( updateR.left() < frameWidth() )
	    updateR.setLeft( frameWidth() );
	if ( updateR.top() < frameWidth() )
	    updateR.setTop( frameWidth() );
    }

    int maxWX = maxViewX();
    int maxWY = maxViewY();
    if ( updateR.right() > maxWX )
	updateR.setRight( maxWX );
    if ( updateR.bottom() > maxWY )
	updateR.setBottom( maxWY );

    setupPainter( &paint );			// prepare for painting table

    int firstRow = findRow( updateR.y() );
    int firstCol = findCol( updateR.x() );
    int	 xStart, yStart;
    if ( !colXPos( firstCol, &xStart ) || !rowYPos( firstRow, &yStart ) ) {
	paint.eraseRect( updateR ); // erase area outside cells but in view
	paint.end();
	return;
    }
    int	  maxX	= updateR.right();
    int	  maxY	= updateR.bottom();
    int	  row	= firstRow;
    int	  col;
    int	  yPos	= yStart;
    int	  xPos = maxX+1; // in case the while() is empty
    int	  nextX;
    int	  nextY;
    QRect winR = viewRect();
    QRect cellR;
    QRect cellUR;
    QWMatrix matrix;

    while ( yPos <= maxY && row < nRows ) {
	nextY = yPos + (cellH ? cellH : cellHeight( row ));
	if ( testTableFlags( Tbl_cutCellsV ) && nextY > ( maxWY + 1 ) )
	    break;
	col  = firstCol;
	xPos = xStart;
	while ( xPos <= maxX && col < nCols ) {
	    nextX = xPos + (cellW ? cellW : cellWidth( col ));
	    if ( testTableFlags( Tbl_cutCellsH ) && nextX > ( maxWX + 1 ) )
		break;

	    cellR.setRect( xPos, yPos, cellW ? cellW : cellWidth(col),
				       cellH ? cellH : cellHeight(row) );
	    cellUR = cellR.intersect( updateR );
	    cellUpdateR = cellUR;
	    cellUpdateR.moveBy( -xPos, -yPos ); // cell coordinates
	    if ( eraseInPaint )
		paint.eraseRect( cellUR );

	    matrix.translate( xPos, yPos );
	    paint.setWorldMatrix( matrix );
	    if ( testTableFlags(Tbl_clipCellPainting) ||
		 frameWidth() > 0 && !winR.contains( cellR ) ) {
		paint.setClipRect( cellUR );
		paintCell( &paint, row, col );
		paint.setClipping( FALSE );
	    } else {
		paintCell( &paint, row, col );
	    }
	    matrix.reset();
	    paint.setWorldMatrix( matrix );
	    col++;
	    xPos = nextX;
	}
	row++;
	yPos = nextY;
    }

    if ( eraseInPaint ) {
	// If we are erasing while painting any areas in the view that
	// are not covered by cells but are covered by the paint event
	// rectangle these must be erased. We know that xPos is the last
	// x pixel updated + 1 and that yPos is the last y pixel updated + 1.
	if ( xPos > maxX + 1 )
	    xPos = maxX + 1;
	if ( yPos > maxY + 1 )
	    yPos = maxY + 1;
	if ( xPos <= maxX )	// erase to the right along full view
	    paint.eraseRect( xPos, frameWidth(),
			     maxX - xPos + 1, maxY - frameWidth() + 1 );
	if ( yPos <= maxY )	// erase under cells updated above
	    paint.eraseRect( frameWidth(), yPos,
			     xPos - frameWidth(), maxY - yPos + 1 );
    }
    paint.end();
}

/*!
  Handles resize events for the table view.

  The scroll bars are moved and cells repainted as necessary.
*/

void QTableView::resizeEvent( QResizeEvent * )
{
    bool update = autoUpdate();
    setAutoUpdate( FALSE );
    int maxX = maxXOffset();			// ### can be slow
    int maxY = maxYOffset();
    if ( xOffs > maxX )
	setXOffset( maxX );
    if ( yOffs > maxY )
	setYOffset( maxY );
    setAutoUpdate( update );
    updateScrollBars( horSteps | horGeometry | horRange |
		      verSteps | verGeometry | verRange );
    showOrHideScrollBars();
    updateFrameSize();
}


/*!
  Redraws all visible cells in the table view.
*/

void QTableView::updateView()
{
    repaint( frameWidth(), frameWidth(), viewWidth(), viewHeight() );
}

/*!
  Returns a pointer to the vertical scroll bar, mainly so you can
  connect() to its signals.  Note that the scroll bar works in pixel
  values, use findRow() to translate to cell numbers.
*/

const QScrollBar *QTableView::verticalScrollBar() const
{
    QTableView *that = (QTableView*)this; // semantic const
    if ( !vScrollBar ) {
	QScrollBar *sb = new QScrollBar( QScrollBar::Vertical, that );
	CHECK_PTR(sb);
	sb->setTracking( FALSE );
	sb->setFocusEnabled( FALSE );
	connect( sb, SIGNAL(valueChanged(int)),
		 SLOT(verSbValue(int)));
	connect( sb, SIGNAL(sliderMoved(int)),
		 SLOT(verSbSliding(int)));
	connect( sb, SIGNAL(sliderReleased()),
		 SLOT(verSbSlidingDone()));
	sb->hide();
	that->vScrollBar = sb;
	return sb;
    }
    return vScrollBar;
}

/*!
  Returns a pointer to the horizontal scroll bar, mainly so you can
  connect() to its signals. Note that the scroll bar works in pixel
  values, use findCol() to translate to cell numbers.
*/

const QScrollBar *QTableView::horizontalScrollBar() const
{
    QTableView *that = (QTableView*)this; // semantic const
    if ( !hScrollBar ) {
	QScrollBar *sb = new QScrollBar( QScrollBar::Horizontal, that );
	sb->setFocusEnabled( FALSE );
	CHECK_PTR(sb);
	sb->setTracking( FALSE );
	connect( sb, SIGNAL(valueChanged(int)),
		 SLOT(horSbValue(int)));
	connect( sb, SIGNAL(sliderMoved(int)),
		 SLOT(horSbSliding(int)));
	connect( sb, SIGNAL(sliderReleased()),
		 SLOT(horSbSlidingDone()));
	sb->hide();
	that->hScrollBar = sb;
	return sb;
    }
    return hScrollBar;
}

/*!
  Enables or disables the horizontal scroll bar, as required by
  setAutoUpdate() and the \link setTableFlags() table flags\endlink.
*/

void QTableView::setHorScrollBar( bool on, bool update )
{
    if ( on ) {
	tFlags |= Tbl_hScrollBar;
	if ( !hScrollBar ) {
	    hScrollBar = new QScrollBar( QScrollBar::Horizontal, this );
	    hScrollBar->setFocusEnabled( FALSE );
	    hScrollBar->setTracking( FALSE );
	    connect( hScrollBar, SIGNAL(valueChanged(int)),
		     SLOT(horSbValue(int)));
	    connect( hScrollBar, SIGNAL(sliderMoved(int)),
		     SLOT(horSbSliding(int)));
	    connect( hScrollBar, SIGNAL(sliderReleased()),
		     SLOT(horSbSlidingDone()));
	    hScrollBar->hide();
	}
	if ( update )
	    updateScrollBars( horMask | verMask );
	else
	    sbDirty |= horMask | verMask;
	if ( testTableFlags( Tbl_vScrollBar ) )
	    coverCornerSquare( TRUE );
	if ( autoUpdate() ) {
	    sbDirty |= horMask;		// so updateScrollBars will show()
	    if ( isVisible() )
		erase( frameWidth(), height() - sbDim - 1,
		       viewWidth(), 1 );
	}
    } else {
	tFlags &= ~Tbl_hScrollBar;
	if ( !hScrollBar )
	    return;
	coverCornerSquare( FALSE );
	if ( autoUpdate() )
	    hScrollBar->hide();
	if ( update )
	    updateScrollBars( verMask );
	else
	    sbDirty |= verMask;
	if ( autoUpdate() && isVisible() )
	    repaint( frameWidth(), height() - frameWidth() - sbDim - 1,
		     viewWidth(), sbDim + frameWidth() );
    }
    updateFrameSize();
}


/*!
  Enables or disables the vertical scroll bar, as required by
  setAutoUpdate() and the \link setTableFlags() table flags\endlink.
*/

void QTableView::setVerScrollBar( bool on, bool update )
{
    if ( on ) {
	tFlags |= Tbl_vScrollBar;
	if ( !vScrollBar ) {
	    vScrollBar = new QScrollBar( QScrollBar::Vertical, this );
	    vScrollBar->setTracking( FALSE );
	    vScrollBar->setFocusEnabled( FALSE );
	    connect( vScrollBar, SIGNAL(valueChanged(int)),
		     SLOT(verSbValue(int)));
	    connect( vScrollBar, SIGNAL(sliderMoved(int)),
		     SLOT(verSbSliding(int)));
	    connect( vScrollBar, SIGNAL(sliderReleased()),
		     SLOT(verSbSlidingDone()));
	    vScrollBar->hide();
	}
	if ( update )
	    updateScrollBars( verMask | horMask );
	else
	    sbDirty |= horMask | verMask;
	if ( testTableFlags( Tbl_hScrollBar ) )
	    coverCornerSquare( TRUE );
	if ( autoUpdate() ) {
	    sbDirty |= verMask;		// so updateScrollBars will show()
	    if ( isVisible() )
		erase( width() - sbDim - 1, frameWidth(),
		       1, viewHeight() );
	}
    } else {
	tFlags &= ~Tbl_vScrollBar;
	if ( !vScrollBar )
	    return;
	coverCornerSquare( FALSE );
	if ( autoUpdate() )
	    vScrollBar->hide();
	if ( update )
	    updateScrollBars( horMask );
	else
	    sbDirty |= horMask;
	if ( autoUpdate() && isVisible() )
	    repaint( width() - frameWidth() - sbDim - 1, frameWidth(),
		     sbDim + frameWidth(), viewHeight() );
    }
    updateFrameSize();
}




int QTableView::findRawRow( int yPos, int *cellMaxY, int *cellMinY,
			    bool goOutsideView ) const
{
    int r = -1;
    if ( nRows == 0 )
	return r;
    if ( goOutsideView || yPos >= frameWidth() && yPos <= maxViewY() ) {
	if ( yPos < frameWidth() ) {
#if defined(CHECK_RANGE)
	    warning( "QTableView::findRawRow: internal error: "
		     "yPos < frameWidth() && goOutsideView "
		     "not supported. (%d,%d)", yPos, yOffs );
#endif
	    return -1;
	}
	if ( cellH ) {				     // uniform cell height
	    r = (yPos - frameWidth() + yCellDelta)/cellH; // cell offs from top
	    if ( cellMaxY )
		*cellMaxY = (r + 1)*cellH + frameWidth() - yCellDelta - 1;
	    if ( cellMinY )
		*cellMinY = r*cellH + frameWidth() - yCellDelta;
	    r += yCellOffs;			     // absolute cell index
	} else {				     // variable cell height
	    QTableView *tw = (QTableView *)this;
	    r	     = yCellOffs;
	    int h    = frameWidth() - yCellDelta;
	    int oldH = h;
	    ASSERT( r < nRows );
	    while ( r < nRows ) {
		oldH = h;
		h += tw->cellHeight( r );	     // Start of next cell
		if ( yPos < h )
		    break;
		r++;
	    }
	    if ( cellMaxY )
		*cellMaxY = h - 1;
	    if ( cellMinY )
		*cellMinY = oldH;
	}
    }
    return r;

}


int QTableView::findRawCol( int xPos, int *cellMaxX, int *cellMinX ,
			    bool goOutsideView ) const
{
    int c = -1;
    if ( nCols == 0 )
	return c;
    if ( goOutsideView || xPos >= frameWidth() && xPos <= maxViewX() ) {
	if ( xPos < frameWidth() ) {
#if defined(CHECK_RANGE)
	    warning( "QTableView::findRawCol: intermal error: "
		     "xPos < frameWidth() && goOutsideView "
		     "not supported. (%d,%d)", xPos, xOffs );
#endif
	    return -1;
	}
	if ( cellW ) {				// uniform cell width
	    c = (xPos - frameWidth() + xCellDelta)/cellW; //cell offs from left
	    if ( cellMaxX )
		*cellMaxX = (c + 1)*cellW + frameWidth() - xCellDelta - 1;
	    if ( cellMinX )
		*cellMinX = c*cellW + frameWidth() - xCellDelta;
	    c += xCellOffs;			// absolute cell index
	} else {				// variable cell width
	    QTableView *tw = (QTableView *)this;
	    c	     = xCellOffs;
	    int w    = frameWidth() - xCellDelta;
	    int oldW = w;
	    ASSERT( c < nCols );
	    while ( c < nCols ) {
		oldW = w;
		w += tw->cellWidth( c );	// Start of next cell
		if ( xPos < w )
		    break;
		c++;
	    }
	    if ( cellMaxX )
		*cellMaxX = w - 1;
	    if ( cellMinX )
		*cellMinX = oldW;
	}
    }
    return c;
}


/*!
  Returns the index of the row at position \e yPos.
  Returns -1 if \e yPos is outside the valid range.

  \sa findCol(), rowYPos()
*/

int QTableView::findRow( int yPos ) const
{
    int cellMaxY;
    int row = findRawRow( yPos, &cellMaxY );
    if ( testTableFlags(Tbl_cutCellsV) && cellMaxY > maxViewY() )
	row = - 1;				//  cell cut by bottom margin
    if ( row >= nRows )
	row = -1;
    return row;
}


/*!
  Returns the index of the column at position \e xPos.
  Returns -1 if \e xPos is outside the valid range.

  \sa findRow(), colXPos()
*/

int QTableView::findCol( int xPos ) const
{
    int cellMaxX;
    int col = findRawCol( xPos, &cellMaxX );
    if ( testTableFlags(Tbl_cutCellsH) && cellMaxX > maxViewX() )
	col = - 1;				//  cell cut by right margin
    if ( col >= nCols )
	col = -1;
    return col;
}


/*!
  Computes the position in the widget of row \e row.

  Returns TRUE and stores the result in \e *yPos if the row is
  visible.  Returns FALSE and does not modify \e *yPos if \e row is
  invisible or invalid.

  \sa colXPos(), findRow()
*/

bool QTableView::rowYPos( int row, int *yPos ) const
{
    int y;
    if ( row >= yCellOffs ) {
	if ( cellH ) {
	    int lastVisible = lastRowVisible();
	    if ( row > lastVisible || lastVisible == -1 )
		return FALSE;
	    y = (row - yCellOffs)*cellH + frameWidth() - yCellDelta;
	} else {
	    y = frameWidth() - yCellDelta;	// y of leftmost cell in view
	    int r = yCellOffs;
	    QTableView *tw = (QTableView *)this;
	    int maxY = maxViewY();
	    while ( r < row && y <= maxY )
		y += tw->cellHeight( r++ );
	    if ( y > maxY )
		return FALSE;

	}
    } else {
	return FALSE;
    }
    if ( yPos )
	*yPos = y;
    return TRUE;
}


/*!
  Computes the position in the widget of column \e column.

  Returns TRUE and stores the result in \e *xPos if the column is
  visible.  Returns FALSE and does not modify \e *xPos if \e col is
  invisible or invalid.

  \sa rowYPos(), findCol()
*/

bool QTableView::colXPos( int col, int *xPos ) const
{
    int x;
    if ( col >= xCellOffs ) {
	if ( cellW ) {
	    int lastVisible = lastColVisible();
	    if ( col > lastVisible || lastVisible == -1 )
		return FALSE;
	    x = (col - xCellOffs)*cellW + frameWidth() - xCellDelta;
	} else {
	    x = frameWidth() - xCellDelta;	// x of uppermost cell in view
	    int c = xCellOffs;
	    QTableView *tw = (QTableView *)this;
	    int maxX = maxViewX();
	    while ( c < col && x <= maxX )
		x += tw->cellWidth( c++ );
	    if ( x > maxX )
		return FALSE;
	}
    } else {
	return FALSE;
    }
    if ( xPos )
	*xPos = x;
    return TRUE;
}


/*!
  Moves the visible area of the table rightwards by \e xPixels and
  downwards by \e yPixels pixels.  Both may be negative.

  This is an reimplementation of QWidget::scroll() to reduce flicker.

  \sa setXOffset(), setYOffset(), setOffset(), setTopCell(),
  setLeftCell(), setTopLeftOffset()
*/

void QTableView::scroll( int xPixels, int yPixels )
{
    if ( xPixels == 0 && yPixels == 0 )
	return;
    if ( xPixels != 0 && yPixels != 0 ||
	 QABS(xPixels) > viewWidth() || QABS(yPixels) > viewHeight() ) {
	repaint();
	return;
    }

    int xStart, yStart, width, height;

    if ( xPixels != 0 ) {
	yStart = frameWidth();
	height = viewHeight();
	xStart = xPixels < 0 ? frameWidth() : frameWidth() + xPixels;

	width = viewWidth() - QABS(xPixels);

	// If we are cutting cells horizontally
	// the rightmost limit for drawing will vary
	// this makes scrolling tricky since we might
	// have to erase an area no longer used for
	// cell drawing.

	int oldLim = 0, newLim = 0; // both =0 just to get around -Wunused
	if ( testTableFlags(Tbl_cutCellsH) ) {
	    int maxX = maxViewX();
	    int oldLastMaxX, oldLastMinX;
	    int newLastMaxX, newLastMinX;
	    findRawCol( maxX - xPixels,
			&oldLastMaxX, &oldLastMinX, TRUE );
	    findRawCol( maxX,&newLastMaxX, &newLastMinX, TRUE );
	    oldLastMaxX += xPixels;
	    oldLastMinX += xPixels;
	    oldLim = oldLastMaxX <= maxX ? oldLastMaxX : oldLastMinX - 1;
	    newLim = newLastMaxX <= maxX ? newLastMaxX : newLastMinX - 1;
	    width -= maxX - oldLim;

	    bitBlt( this, xStart - xPixels, yStart,
		    this, xStart, yStart, width, height );

	    if ( xPixels < 0 )
		repaint( frameWidth(), yStart, -xPixels, height );
	    else
		repaint( frameWidth() + width, yStart,
			 maxX - frameWidth() - width, height );
	} else {
	    bitBlt( this, xStart - xPixels, yStart,
		    this, xStart, yStart, width, height );
	    if ( xPixels < 0 )
		repaint( frameWidth(), yStart, -xPixels, height );
	    else
		repaint( frameWidth() + width, yStart, xPixels, height );
	}
    }

    if ( yPixels != 0 ) {
	xStart = frameWidth();
	width  = viewWidth();
	yStart = yPixels < 0 ? frameWidth() : frameWidth() + yPixels;

	height = viewHeight() - QABS(yPixels);

      // If we are cutting cells vertically
      // the lower limit for drawing will vary
      // this makes scrolling tricky since we might
      // have to erase an area no longer used for
      // cell drawing.

	int oldLim = 0, newLim = 0; // both =0 just to get around -Wunused
	if ( testTableFlags(Tbl_cutCellsV) ) {
	    int maxY = maxViewY();
	    int oldLastMaxY, oldLastMinY;
	    int newLastMaxY, newLastMinY;
	    findRawRow( maxY - yPixels,
			&oldLastMaxY, &oldLastMinY, TRUE );
	    findRawRow( maxY,&newLastMaxY, &newLastMinY, TRUE );
	    oldLastMaxY += yPixels;
	    oldLastMinY += yPixels;
	    oldLim = oldLastMaxY <= maxY ? oldLastMaxY : oldLastMinY - 1;
	    newLim = newLastMaxY <= maxY ? newLastMaxY : newLastMinY - 1;
	    height -= maxY - oldLim;

	    bitBlt( this, xStart, yStart - yPixels,
		    this, xStart, yStart, width, height );

	    if ( yPixels < 0 ) {
		repaint( xStart, frameWidth(), width, -yPixels );
	    } else {
		repaint( xStart, frameWidth() + height,
			 width, maxY - frameWidth() - height );
	    }
	} else {
	    bitBlt( this, xStart, yStart - yPixels,
		    this, xStart, yStart, width, height );

	    if ( yPixels < 0 ) {
		repaint( xStart, frameWidth(), width, -yPixels );
	    } else {
		repaint( xStart, frameWidth() + height, width, yPixels );
	    }
	}
    }
}


/*!
  Returns the rightmost pixel of the table view in screen
  coordinates.	This excludes the frame and any scroll bar, but
  includes blank pixels to the right of the visible table data.

  \sa maxViewY(), viewWidth(), contentsRect()
*/

int QTableView::maxViewX() const
{
    return width() - 1 - frameWidth()
		   - (tFlags & Tbl_vScrollBar ? sbDim + 1 : 0);
}


/*!
  Returns the bottom pixel of the table view in screen
  coordinates.	This excludes the frame and any scroll bar, but
  includes blank pixels below the visible table data.

  \sa maxViewX(), viewHeight(), contentsRect()
*/

int QTableView::maxViewY() const
{
    return height() - 1 - frameWidth()
		    - (tFlags & Tbl_hScrollBar ? sbDim + 1 : 0);
}


/*!
  Returns the width of the table view, as such.	 This does not
  include any scroll bar or frame, but does include background pixels
  to the right of the table data.

  \sa maxViewX(), viewHeight(), contentsRect()
*/

int QTableView::viewWidth() const
{
    return maxViewX() - frameWidth() + 1;
}


/*!
  Returns the height of the table view, as such.  This does not
  include any scroll bar or frame, but does include background pixels
  below the table data.
  \sa maxViewY() viewWidth() contentsRect()
*/

int QTableView::viewHeight() const
{
    return maxViewY() - frameWidth() + 1;
}


void QTableView::doAutoScrollBars()
{
    int viewW = width()	 - frameWidth()*2;
    int viewH = height() - frameWidth()*2;
    bool vScrollOn = testTableFlags(Tbl_vScrollBar);
    bool hScrollOn = testTableFlags(Tbl_hScrollBar);
    int w = 0;
    int h = 0;
    int i;

    if ( testTableFlags(Tbl_autoHScrollBar) ) {
	if ( cellW ) {
	    w = cellW*nCols;
	} else {
	    i = 0;
	    while ( i < nCols && w <= viewW )
		w += cellWidth( i++ );
	}
	if ( w > viewW )
	    hScrollOn = TRUE;
	else
	    hScrollOn = FALSE;
    }

    if ( testTableFlags(Tbl_autoVScrollBar) ) {
	if ( cellH ) {
	    h = cellH*nRows;
	} else {
	    i = 0;
	    while ( i < nRows && h <= viewH )
		h += cellHeight( i++ );
	}

	if ( h > viewH )
	    vScrollOn = TRUE;
	else
	    vScrollOn = FALSE;
    }

    if ( testTableFlags(Tbl_autoHScrollBar) && vScrollOn && !hScrollOn )
	if ( w > viewW - sbDim - 1 )
	    hScrollOn = TRUE;

    if ( testTableFlags(Tbl_autoVScrollBar) && hScrollOn && !vScrollOn )
	if ( h > viewH - sbDim - 1 )
	    vScrollOn = TRUE;

    setHorScrollBar( hScrollOn , FALSE );
    setVerScrollBar( vScrollOn , FALSE );
}


/*!
  \fn void QTableView::updateScrollBars()

  Updates the scroll bars' contents and presence to match the table's
  state.  Generally you should not need to call this.

  \sa setTableFlags()
*/

/*!
  Updates the scroll bars' contents and presence to match the table's
  state \c or \e f.

  \sa setTableFlags()
*/

void QTableView::updateScrollBars( uint f )
{
    sbDirty = sbDirty | f;
    if ( inSbUpdate )
	return;
    inSbUpdate = TRUE;

    if ( testTableFlags(Tbl_autoHScrollBar) && (sbDirty & horRange) ||
	 testTableFlags(Tbl_autoVScrollBar) && (sbDirty & verRange) )
					// if range change and auto
	doAutoScrollBars();		// turn scroll bars on/off if needed
    if ( !autoUpdate() || !isVisible() ) {
	inSbUpdate = FALSE;
	return;
    }
    if ( testTableFlags(Tbl_hScrollBar) && (sbDirty & horMask) != 0 ) {
	if ( sbDirty & horGeometry )
	    hScrollBar->setGeometry( 0,height() - sbDim,
				     viewWidth() + frameWidth()*2+1, sbDim );

	if ( sbDirty & horSteps ) {
	    if ( cellW )
		hScrollBar->setSteps( cellW, viewWidth() );
	    else
		hScrollBar->setSteps( 16, viewWidth() );
	}

	if ( sbDirty & horRange )
	    hScrollBar->setRange( 0, maxXOffset() );

	if ( sbDirty & horValue )
	    hScrollBar->setValue( xOffs );

			// show scrollbar only when it has a sane geometry
	if ( !hScrollBar->isVisible() )
	    hScrollBar->show();
    }

    if ( testTableFlags(Tbl_vScrollBar) && (sbDirty & verMask) != 0 ) {
	if ( sbDirty & verGeometry )
	    vScrollBar->setGeometry( width() - sbDim, 0,
				     sbDim, viewHeight() + frameWidth()*2+1 );

	if ( sbDirty & verSteps ) {
	    if ( cellH )
		vScrollBar->setSteps( cellH, viewHeight() );
	    else
		vScrollBar->setSteps( 16, viewHeight() );  // fttb! ###
	}

	if ( sbDirty & verRange )
	    vScrollBar->setRange( 0, maxYOffset() );

	if ( sbDirty & verValue )
	    vScrollBar->setValue( yOffs );

			// show scrollbar only when it has a sane geometry
	if ( !vScrollBar->isVisible() )
	    vScrollBar->show();
    }
    if ( coveringCornerSquare &&
	 ( (sbDirty & verGeometry ) || (sbDirty & horGeometry)) )
	cornerSquare->move( maxViewX() + frameWidth() + 2,
			    maxViewY() + frameWidth() + 2 );

    sbDirty = 0;
    inSbUpdate = FALSE;
}


void QTableView::updateFrameSize()
{
    int rw = width()  - ( testTableFlags(Tbl_vScrollBar) ? sbDim + 1 : 0 );
    int rh = height() - ( testTableFlags(Tbl_hScrollBar) ? sbDim + 1 : 0 );
    if ( rw < 0 )
	rw = 0;
    if ( rh < 0 )
	rh = 0;
    int fh = frameRect().height();
    int fw = frameRect().width();
    if ( rw > fw )
	update( fw - frameWidth(), 0, frameWidth(), fh );
    if ( rh > fh )
	update( 0, fh - frameWidth(), fw, frameWidth() );
    setFrameRect( QRect(0,0,rw,rh) );
}


/*!
  Returns the maximum horizontal offset within the table of the
  view's left edge.

  This is used mainly to set the horizontal scroll bar's range.

  \sa maxColOffset(), maxYOffset(), totalWidth()
*/

int QTableView::maxXOffset()
{
    int tw = totalWidth();
    int maxOffs;
    if ( testTableFlags(Tbl_scrollLastHCell) ) {
	if ( nCols != 1)
	    maxOffs =  tw - ( cellW ? cellW : cellWidth( nCols - 1 ) );
	else
	    maxOffs = tw - viewWidth();
    } else {
	if ( testTableFlags(Tbl_snapToHGrid) ) {
	    if ( cellW ) {
		maxOffs =  tw - (viewWidth()/cellW)*cellW;
	    } else {
		int  ww	   = viewWidth();
		int  oldWw = ww;
		int i = nCols - 1;
		while ( i >= 0 ) {
		    ww -= cellWidth( i );
		    if ( ww < 0 )
			break;
		    i--;
		    oldWw = ww;
		}
		maxOffs = tw - oldWw;
	    }
	} else {
	    maxOffs = tw - viewWidth();
	}
    }
    return maxOffs > 0 ? maxOffs : 0;
}


/*!
  Returns the maximum horizontal offset within the table of the
  view's top edge.

  This is used mainly to set the vertical scroll bar's range.

  \sa maxRowOffset(), maxXOffset(), totalHeight()
*/

int QTableView::maxYOffset()
{
    int th = totalHeight();
    int maxOffs;
    if ( testTableFlags(Tbl_scrollLastVCell) ) {
	if ( nRows != 1)
	    maxOffs =  th - ( cellH ? cellH : cellHeight( nRows - 1 ) );
	else
	    maxOffs = th - viewHeight();
    } else {
	if ( testTableFlags(Tbl_snapToVGrid) ) {
	    if ( cellH ) {
		maxOffs =  th - (viewHeight()/cellH)*cellH;
	    } else {
		int  wh	   = viewHeight();
		int  oldWh = wh;
		int i = nRows - 1;
		while ( i >= 0 ) {
		    wh -= cellHeight( i );
		    if ( wh < 0 )
			break;
		    i--;
		    oldWh = wh;
		}
		maxOffs = th - oldWh;
	    }
	} else {
	    maxOffs = th - viewHeight();
	}
    }
    return maxOffs > 0 ? maxOffs : 0;
}


/*!
  Returns the idex of the last column which may be at the left edge
  of the view.

  Depending on the \link setTableFlags Tbl_scrollLastHCell\endlink flag,
  this may or may not be the last column.

  \sa maxXOffset(), maxRowOffset()
*/

int QTableView::maxColOffset()
{
    int mx = maxXOffset();
    if ( cellW )
	return mx/cellW;
    else {
	int xcd, col=0;
	while ( col < nCols && mx > (xcd=cellWidth(col)) ) {
	    mx -= xcd;
	    col++;
	}
	return col;
    }
}


/*!
  Returns the index of the last row which may be at the top edge of
  the view.

  Depending on the \link setTableFlags Tbl_scrollLastVCell\endlink flag,
  this may or may not be the last row.

  \sa maxYOffset(), maxColOffset()
*/

int QTableView::maxRowOffset()
{
    int my = maxYOffset();
    if ( cellH )
	return my/cellH;
    else {
	int ycd, row=0;
	while (my > (ycd=cellHeight(row)) ) {
	    my -= ycd;
	    row++;
	}
	return row;
    }
}


void QTableView::showOrHideScrollBars()
{
    if ( !autoUpdate() )
	return;
    if ( vScrollBar ) {
	if ( testTableFlags(Tbl_vScrollBar) ) {
	    if ( !vScrollBar->isVisible() )
		sbDirty |= verMask;
	} else {
	    if ( vScrollBar->isVisible() )
	       vScrollBar->hide();
	}
    }
    if ( hScrollBar ) {
	if ( testTableFlags(Tbl_hScrollBar) ) {
	    if ( !hScrollBar->isVisible() )
		sbDirty |= horMask;
	} else {
	    if ( hScrollBar->isVisible() )
		hScrollBar->hide();
	}
    }
    if ( cornerSquare ) {
	if ( testTableFlags(Tbl_hScrollBar) &&
	     testTableFlags(Tbl_vScrollBar) ) {
	    if ( !cornerSquare->isVisible() )
		cornerSquare->show();
	} else {
	    if ( cornerSquare->isVisible() )
		cornerSquare->hide();
	}
    }
}
