/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qprogressbar.cpp#94 $
**
** Implementation of QProgressBar class
**
** Created : 970521
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "qprogressbar.h"
#ifndef QT_NO_PROGRESSBAR
#include "qpainter.h"
#include "qdrawutil.h"
#include "qapplication.h"
#include "qpixmap.h"
#if defined(QT_ACCESSIBILITY_SUPPORT)
#include "qaccessiblewidget.h"
#endif
#include <limits.h>

/*!
  \class QProgressBar qprogressbar.h
  \brief The QProgressBar widget provides a horizontal progress bar.
  \ingroup advanced

  A progress bar is used to give the user an indication of progress
  of an operation and to reassure user that the application has not crashed.

  QProgressBar only implements the basic progress display, whereas
  QProgressDialog provides a fuller encapsulation.

  <img src=qprogbar-m.png> <img src=qprogbar-w.png>

  \sa QProgressDialog
  <a href="guibooks.html#fowler">GUI Design Handbook: Progress Indicator</a>
*/


/*!
  Constructs a progress bar.

  The total number of steps is set to 100 by default.

  \e parent, \e name and \e f are sent to the QFrame::QFrame()
  constructor.

  \sa setTotalSteps()
*/

QProgressBar::QProgressBar( QWidget *parent, const char *name, WFlags f )
    : QFrame( parent, name, f | WRepaintNoErase | WResizeNoErase ),
      total_steps( 100 ),
      progress_val( -1 ),
      percentage( -1 ),
      center_indicator( TRUE ),
      auto_indicator( TRUE ),
      percentage_visible( TRUE ),
      d( 0 )
{
    setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    initFrame();
}


/*!
  Constructs a progress bar.

  \a totalSteps is the total number of steps in the operation of which
  this progress bar shows the progress.  For example, if the operation
  is to examine 50 files, this value would be 50. Before
  examining the first file, call setProgress(0); call setProgress(50) after examining
  the last file .

  \e parent, \e name and \e f are sent to the QFrame::QFrame()
  constructor.

  \sa setTotalSteps(), setProgress()
*/

QProgressBar::QProgressBar( int totalSteps,
			    QWidget *parent, const char *name, WFlags f )
    : QFrame( parent, name, f | WRepaintNoErase | WResizeNoErase ),
      total_steps( totalSteps ),
      progress_val( -1 ),
      percentage( -1 ),
      center_indicator( TRUE ),
      auto_indicator( TRUE ),
      percentage_visible( TRUE ),
      d( 0 )
{
    setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    initFrame();
}


/*!
  Reset the progress bar.
  The progress bar "rewinds."
*/

void QProgressBar::reset()
{
    progress_val = -1;
    percentage = -1;
    setIndicator(progress_str, progress_val, total_steps);
    repaint( FALSE );
}


/*!
  \property QProgressBar::totalSteps
  \brief The total number of steps.

  If totalSteps is null, the progress bar will display a busy indicator.

  \sa totalSteps()
*/

void QProgressBar::setTotalSteps( int totalSteps )
{
    total_steps = totalSteps;
    if ( isVisible() &&
	 ( setIndicator(progress_str, progress_val, total_steps) || !total_steps ) )
	repaint( FALSE );
}


/*!
  \property QProgressBar::progress
  \brief the current amount of progress

  This property is -1 if the progress counting has not started.
*/

void QProgressBar::setProgress( int progress )
{
    if ( progress == progress_val ||
	 progress < 0 || ( ( progress > total_steps ) && total_steps ) )
	return;

    progress_val = progress;

    setIndicator( progress_str, progress_val, total_steps );

    repaint( FALSE );

#if defined(QT_ACCESSIBILITY_SUPPORT)
    emit accessibilityChanged( QAccessible::ValueChanged );
#endif
}


/*!\reimp
*/
QSize QProgressBar::sizeHint() const
{
    constPolish();
    QFontMetrics fm = fontMetrics();
    return QSize( style().progressChunkWidth() * 7 + fm.width( '0' ) * 4, fm.height()+8);
}


/*!
  \reimp
*/
QSize QProgressBar::minimumSizeHint() const
{
    return sizeHint();
}

/*!
  \property QProgressBar::centerIndicator

  \brief where the indicator string should be displayed

  If set to TRUE, the indicator is displayed centered.
  Changing this property sets indicatorFollowsStyle to FALSE.

  \sa indicatorFollowsStyle
*/

void QProgressBar::setCenterIndicator( bool on )
{
    if ( !auto_indicator && on == center_indicator )
	return;
    auto_indicator   = FALSE;
    center_indicator = on;
    repaint( FALSE );
}

/*!
  \property QProgressBar::indicatorFollowsStyle
  \brief whether the display of the indicator string should follow the GUI style or not.

  \sa centerIndicator
*/

void QProgressBar::setIndicatorFollowsStyle( bool on )
{
    if ( on == auto_indicator )
	return;
    auto_indicator = on;
    repaint( FALSE );
}

/*!
  \property QProgressBar::percentageVisible
  \brief whether the current progress value is displayed or not.
*/
void QProgressBar::setPercentageVisible( bool on )
{
    if ( on == percentage_visible )
	return;
    percentage_visible = on;
    repaint( FALSE );
}

/*!
  \reimp
*/
void QProgressBar::show()
{
    setIndicator( progress_str, progress_val, total_steps );
    QFrame::show();
}

void QProgressBar::initFrame()
{
    setFrameStyle(QFrame::NoFrame);
}

/*! \reimp
 */
void QProgressBar::styleChange( QStyle& old )
{
    initFrame();
    QFrame::styleChange( old );
}


/*!
  This method is called to generate the text displayed in the center of
  the progress bar.

  The progress may be negative, indicating that the bar is in the "reset" state
  before any progress is set.

  The default implementation is the percentage of completion or blank in the
  reset state.

  To allow efficient repainting of the progress bar, this method should return FALSE if the string is unchanged from the
  last call to the method, .
*/

bool QProgressBar::setIndicator( QString & indicator, int progress,
				 int totalSteps )
{
    if ( !totalSteps )
	return FALSE;
    if ( progress < 0 ) {
	indicator = QString::fromLatin1("");
	return TRUE;
    } else {
	// Get the values down to something usable.
	if ( totalSteps > INT_MAX/1000 ) {
	    progress /= 1000;
	    totalSteps /= 1000;
	}

	int np = progress * 100 / totalSteps;
	if ( np != percentage ) {
	    percentage = np;
	    indicator.sprintf( "%d%%", np );
	    return TRUE;
	} else {
	    return FALSE;
	}
    }
}


/*!\reimp
*/
void QProgressBar::drawContents( QPainter *p )
{
    const QRect bar = contentsRect();

    QPixmap pm;
    pm.resize( bar.size() );

    QPainter paint( &pm );
    QBrush fbrush = backgroundPixmap() ? QBrush( backgroundColor(), *backgroundPixmap() ) : QBrush( backgroundColor() );
    paint.fillRect( bar, fbrush );

    paint.setFont( p->font() );

    if ( !total_steps ) { // draw busy indicator
	int bw = bar.width();
	int x = progress_val % ( bw * 2 );
	if ( x > bw )
	    x = 2 * bw - x;
	x += bar.x();
	style().drawProgressBar( &paint, bar.x(), bar.y(), bar.width(), bar.height(), colorGroup() );
	paint.setPen( QPen( colorGroup().highlight(), 4 ) );
	paint.drawLine( x, bar.y()+1, x, bar.height()-2 );
    } else {
	const int unit_width = style().progressChunkWidth();

	bool hasExtraIndicator = percentage_visible && total_steps && (
				 style() != MotifStyle && auto_indicator ||
				!auto_indicator && !center_indicator );

	int textw = 0;
	if ( hasExtraIndicator ) {
	    QFontMetrics fm = p->fontMetrics();
	    textw = fm.width(QString::fromLatin1("100%")) + 6;
	}
	int u = (bar.width() - textw ) / unit_width;
	int p_v = progress_val;
	int t_s = total_steps;
	if ( u > 0 && progress_val >= INT_MAX / u && t_s >= u ) {
	    // scale down to something usable.
	    p_v /= u;
	    t_s /= u;
	}
	int nu = ( u * p_v + t_s/2 ) / t_s;

	style().drawProgressBar( &paint, bar.x(), bar.y(), u*unit_width + 4, bar.height(), colorGroup() );
	
	// Draw nu units out of a possible u of unit_width width, each
	// a rectangle bordered by background color, all in a sunken panel
	// with a percentage text display at the end.
	int x = 0;
	for (int i=0; i<nu; i++) {
	    style().drawProgressChunk( &paint, bar.x() + x + 2, bar.y() + 2,
			 unit_width, bar.height() - 4, palette().active() );
	    x += unit_width;
	}
	if ( !hasExtraIndicator && percentage_visible && total_steps ) {
	    paint.setPen( colorGroup().highlightedText() );
	    paint.setClipRect( bar.x(), bar.y(), x+2, bar.height() );
	    paint.drawText( bar, AlignCenter | SingleLine, progress_str );
	    if ( progress_val != total_steps ) {
		paint.setClipRect( bar.x() + x+2, bar.y(), bar.width() - x - 2, bar.height() );
		paint.setPen( colorGroup().highlight() );
		paint.drawText( bar, AlignCenter | SingleLine, progress_str );	
	    }
	} else if ( hasExtraIndicator ) {
	    paint.setPen( colorGroup().foreground() );
	    paint.drawText( bar.x()+u*unit_width + 4, bar.y(), textw, bar.height(),
		AlignCenter | SingleLine, progress_str );
	}
    }

    paint.end();

    p->drawPixmap( bar.x(), bar.y(), pm );
}


#if defined(QT_ACCESSIBILITY_SUPPORT)
/*! \reimp */
QAccessibleInterface *QProgressBar::accessibleInterface()
{
    return new QAccessibleRangeControl( this, QAccessible::ProgressBar );
}
#endif

#endif
