/****************************************************************************
** $Id$
**
** Implementation of QSlider class
**
** Created : 961019
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

#include "qslider.h"
#ifndef QT_NO_SLIDER
#include "qpainter.h"
#include "qdrawutil.h"
#include "qtimer.h"
#include "qbitmap.h"
#include "qapplication.h"
#include "qstyle.h"
#if defined(QT_ACCESSIBILITY_SUPPORT)
#include "qaccessible.h"
#endif

static const int thresholdTime = 500;
static const int repeatTime = 100;

static int sliderStartVal = 0; //##### class member?


/*!
  \class QSlider qslider.h
  \brief The QSlider widget provides a vertical or horizontal slider.
  \ingroup basic
  \mainclass

  The slider is the classic widget for controlling a bounded value.
  It lets the user move a slider along a horizontal or vertical
  groove and translates the slider's position into an integer value
  within the legal range.

  QSlider inherits QRangeControl, which provides the "integer" side of
  the slider.  setRange() and value() are likely to be used by
  practically all slider users; see the \l QRangeControl documentation
  for information about the many other functions that class provides.

  The main functions offered by the slider itself are tickmark and
  orientation control; you can use setTickmarks() to indicate where
  you want the tickmarks to be, setTickInterval() to indicate how many
  of them you want and setOrientation() to indicate whether the
  slider is to be horizontal or vertical.

  A slider has a default focusPolicy() of \c WeakWheelFocus, i.e. it
  accepts focus on Tab and uses the mouse wheel and a
  suitable keyboard interface.

  <img src=qslider-m.png> <img src=qslider-w.png>

  \sa QScrollBar QSpinBox
  \link guibooks.html#fowler GUI Design Handbook: Slider\endlink
*/


/*! \enum QSlider::TickSetting

  This enum specifies where the tickmarks are to be drawn relative
  to the slider's groove and the handle the user moves.  The possible
  values are:
  \value NoMarks do not draw any tickmarks.
  \value Both draw tickmarks on both sides of the groove.
  \value Above draw tickmarks above the (horizontal) slider
  \value Below draw tickmarks below the (horizontal) slider
  \value Left draw tickmarks to the left of the (vertical) slider
  \value Right draw tickmarks to the right of the (vertical) slider
*/


/*!
  Constructs a vertical slider.

  The \a parent and \a name arguments are sent to the QWidget constructor.
*/

QSlider::QSlider( QWidget *parent, const char *name )
    : QWidget( parent, name  )
{
    orient = Vertical;
    init();
}

/*!
  Constructs a slider.

  The \a orientation must be \l Qt::Vertical or \l Qt::Horizontal.

  The \a parent and \a name arguments are sent to the QWidget constructor.
*/

QSlider::QSlider( Orientation orientation, QWidget *parent, const char *name )
    : QWidget( parent, name )
{
    orient = orientation;
    init();
}

/*!  Constructs a slider whose value can never be smaller than \a
  minValue or greater than \a maxValue, whose page step size is
  \a pageStep and whose value is initially \a value (which is
  guaranteed to be in range using bound()).

  If \a orientation is Qt::Vertical the slider is vertical and if it is
  Qt::Horizontal the slider is horizontal.

  The \a parent and \a name arguments are sent to the QWidget constructor.
*/

QSlider::QSlider( int minValue, int maxValue, int pageStep,
		  int value, Orientation orientation,
		  QWidget *parent, const char *name )
    : QWidget( parent, name ),
      QRangeControl( minValue, maxValue, 1, pageStep, value )
{
    orient = orientation;
    init();
    sliderVal = value;
}


void QSlider::init()
{
    extra = 0;
    timer = 0;
    sliderPos = 0;
    sliderVal = 0;
    clickOffset = 0;
    state = Idle;
    track = TRUE;
    ticks = NoMarks;
    tickInt = 0;
    setFocusPolicy( TabFocus  );
    initTicks();
}


/*!
  Does what's needed when someone changes the tickmark status
*/

void QSlider::initTicks()
{
    tickOffset = style().pixelMetric( QStyle::PM_SliderTickmarkOffset, this );
}


/*!
  \property QSlider::tracking
  \brief whether slider tracking is enabled

  If tracking is enabled (the default), the slider emits the
  valueChanged() signal whenever the slider is being dragged.  If
  tracking is disabled, the slider emits the valueChanged() signal
  when the user releases the mouse button (unless the value happens to
  be the same as before).
*/

void QSlider::setTracking( bool enable )
{
    track = enable;
}


/*!
  \fn void QSlider::valueChanged( int value )
  This signal is emitted when the slider value is changed, with the
  new slider \a value as an argument.
*/

/*!
  \fn void QSlider::sliderPressed()
  This signal is emitted when the user presses the slider with the mouse.
*/

/*!
  \fn void QSlider::sliderMoved( int value )
  This signal is emitted when the slider is dragged, with the
  new slider \a value as an argument.
*/

/*!
  \fn void QSlider::sliderReleased()
  This signal is emitted when the user releases the slider with the mouse.
*/

/*!
  Calculates slider position corresponding to value \a v.
*/

int QSlider::positionFromValue( int v ) const
{
    int  a = available();
    int x = QRangeControl::positionFromValue( v, a );
    if ( orient == Horizontal && QApplication::reverseLayout() )
	x = a - x;
    return x;
}

/*!
  Returns the available space in which the slider can move.
*/

int QSlider::available() const
{
    return style().pixelMetric( QStyle::PM_SliderSpaceAvailable, this );
}

/*!
  Calculates a value corresponding to slider position \a p.
*/

int QSlider::valueFromPosition( int p ) const
{
    int a = available();
    int x = QRangeControl::valueFromPosition( p, a );
    if ( orient == Horizontal && QApplication::reverseLayout() )
	x = maxValue() + minValue() - x;
    return x;
}

/*!
  Implements the virtual QRangeControl function.
*/

void QSlider::rangeChange()
{
    int newPos = positionFromValue( value() );
    if ( newPos != sliderPos ) {
	reallyMoveSlider( newPos );
    }
}

/*!
  Implements the virtual QRangeControl function.
*/

void QSlider::valueChange()
{
    if ( sliderVal != value() ) {
	int newPos = positionFromValue( value() );
	sliderVal = value();
	reallyMoveSlider( newPos );
    }
    emit valueChanged(value());
#if defined(QT_ACCESSIBILITY_SUPPORT)
    QAccessible::updateAccessibility( this, 0, QAccessible::ValueChanged );
#endif
}


/*!\reimp
*/
void QSlider::resizeEvent( QResizeEvent * )
{
    rangeChange();
    initTicks();
}


/*!
  Reimplements the virtual function QWidget::setPalette().

  Sets the background color to the mid color for Motif style sliders
  using palette \a p.
*/

void QSlider::setPalette( const QPalette &p )
{
    QWidget::setPalette( p );
}



/*!
  \property QSlider::orientation
  \brief the orientation of the slider

  The orientation must be \l Qt::Vertical or \l Qt::Horizontal.
*/

void QSlider::setOrientation( Orientation orientation )
{
    orient = orientation;
    rangeChange();
    update();
}

/*!
    \fn int QSlider::sliderStart() const

    Returns the start position of the slider.
*/


/*!
  Returns the slider handle rectangle. (This is the visual marker that
  the user can move.)
*/

QRect QSlider::sliderRect() const
{
    return style().querySubControlMetrics( QStyle::CC_Slider, this,
					   QStyle::SC_SliderHandle );
}

/*!
  Performs the actual moving of the slider.
*/

void QSlider::reallyMoveSlider( int newPos )
{
    QRect oldR = sliderRect();
    sliderPos = newPos;
    QRect newR = sliderRect();
    //since sliderRect isn't virtual, I know that oldR and newR
    // are the same size.
    if ( orient == Horizontal ) {
	if ( oldR.left() < newR.left() )
	    oldR.setRight( QMIN ( oldR.right(), newR.left()));
	else           //oldR.right() >= newR.right()
	    oldR.setLeft( QMAX ( oldR.left(), newR.right()));
    } else {
	if ( oldR.top() < newR.top() )
	    oldR.setBottom( QMIN ( oldR.bottom(), newR.top()));
	else           //oldR.bottom() >= newR.bottom()
	    oldR.setTop( QMAX ( oldR.top(), newR.bottom()));
    }
    repaint( oldR );
    repaint( newR, FALSE );
}


/*!\reimp
*/
void QSlider::paintEvent( QPaintEvent * )
{
    QPainter p( this );

    QStyle::SFlags flags = QStyle::Style_Default;
    if (isEnabled())
	flags |= QStyle::Style_Enabled;
    if (hasFocus())
	flags |= QStyle::Style_HasFocus;

    QStyle::SCFlags sub = QStyle::SC_SliderGroove | QStyle::SC_SliderHandle;
    if ( tickmarks() != NoMarks )
	sub |= QStyle::SC_SliderTickmarks;

    style().drawComplexControl( QStyle::CC_Slider, &p, this, rect(), colorGroup(),
				flags, sub, state == Dragging ? QStyle::SC_SliderHandle : QStyle::SC_None );
}


/*!\reimp
*/
void QSlider::mousePressEvent( QMouseEvent *e )
{
    int slideLength = style().pixelMetric( QStyle::PM_SliderLength, this );
    resetState();
    sliderStartVal = sliderVal;
    QRect r = sliderRect();

    if ( e->button() == RightButton ) {
	return;
    } else if ( r.contains( e->pos() ) ) {
	state = Dragging;
	clickOffset = (QCOORD)( goodPart( e->pos() ) - sliderPos );
	emit sliderPressed();
    } else if ( e->button() == MidButton ) {
	int pos = goodPart( e->pos() );
	moveSlider( pos - slideLength / 2 );
	state = Dragging;
	clickOffset = slideLength / 2;
    } else if ( orient == Horizontal && e->pos().x() < r.left() //### goodPart
		|| orient == Vertical && e->pos().y() < r.top() ) {
	state = TimingDown;
	subtractPage();
	if ( !timer )
	    timer = new QTimer( this );
	connect( timer, SIGNAL(timeout()), SLOT(repeatTimeout()) );
	timer->start( thresholdTime, TRUE );
    } else if ( orient == Horizontal && e->pos().x() > r.right() //### goodPart
		|| orient == Vertical && e->pos().y() > r.bottom() ) {
	state = TimingUp;
	addPage();
	if ( !timer )
	    timer = new QTimer( this );
	connect( timer, SIGNAL(timeout()), SLOT(repeatTimeout()) );
	timer->start( thresholdTime, TRUE );
    }
}

/*!\reimp
*/
void QSlider::mouseMoveEvent( QMouseEvent *e )
{
    if ( state != Dragging )
	return;

    QRect r = rect();
    int m = style().pixelMetric( QStyle::PM_MaximumDragDistance,
				 this );
    if ( m >= 0 ) {
	if ( orientation() == Horizontal )
	    r.setRect( r.x() - m, r.y() - 2*m/3,
		       r.width() + 2*m, r.height() + 3*m );
	else
	    r.setRect( r.x() - 2*m/3, r.y() - m,
		       r.width() + 3*m, r.height() + 2*m );
	if ( !r.contains( e->pos() ) ) {
	    moveSlider( positionFromValue( sliderStartVal) );
	    return;
	}
    }

    int pos = goodPart( e->pos() );
    moveSlider( pos - clickOffset );
}

/*!\reimp
*/
void QSlider::wheelEvent( QWheelEvent * e){
    static float offset = 0;
    static QSlider* offset_owner = 0;
    if (offset_owner != this){
	offset_owner = this;
	offset = 0;
    }
    offset += -e->delta()*QMAX(pageStep(),lineStep())/120;
    if (QABS(offset)<1)
	return;
    setValue( value() + int(offset) );
    offset -= int(offset);
    e->accept();
}


/*!\reimp
*/
void QSlider::mouseReleaseEvent( QMouseEvent * )
{
    resetState();
}

/*!\reimp
*/
void QSlider::focusInEvent( QFocusEvent * e)
{
    QWidget::focusInEvent( e );
}

/*!\reimp
*/
void QSlider::focusOutEvent( QFocusEvent * e )
{
    QWidget::focusOutEvent( e );
}

/*!
  Moves the left (or top) edge of the slider to position
  \a pos. The slider is actually moved to the step position nearest
  the given \a pos.
*/

void QSlider::moveSlider( int pos )
{
    int  a = available();
    int newPos = QMIN( a, QMAX( 0, pos ) );
    int newVal = valueFromPosition( newPos );
    if ( sliderVal != newVal ) {
	sliderVal = newVal;
	emit sliderMoved( sliderVal );
    }
    if ( tracking() && sliderVal != value() )
	setValue( sliderVal );
    if (style().styleHint(QStyle::SH_Slider_SnapToValue, this))
	newPos = positionFromValue( newVal );
    if ( sliderPos != newPos )
	reallyMoveSlider( newPos );
}


/*!
  Resets all state information and stops the timer.
*/

void QSlider::resetState()
{
    if ( timer ) {
	timer->stop();
	timer->disconnect();
    }
    switch ( state ) {
    case TimingUp:
    case TimingDown:
	break;
    case Dragging: {
	setValue( valueFromPosition( sliderPos ) );
	emit sliderReleased();
	break;
    }
    case Idle:
	break;
    default:
	qWarning("QSlider: (%s) in wrong state", name( "unnamed" ) );
    }
    state = Idle;
}


/*!\reimp
*/
void QSlider::keyPressEvent( QKeyEvent *e )
{
    bool sloppy = bool(style().styleHint(QStyle::SH_Slider_SloppyKeyEvents, this));
    switch ( e->key() ) {
    case Key_Left:
	if ( sloppy || orient == Horizontal )
	    subtractLine();
	break;
    case Key_Right:
	if ( sloppy || orient == Horizontal )
	    addLine();
	break;
    case Key_Up:
	if ( sloppy || orient == Vertical )
	    subtractLine();
	break;
    case Key_Down:
	if ( sloppy || orient == Vertical )
	    addLine();
	break;
    case Key_Prior:
	subtractPage();
	break;
    case Key_Next:
	addPage();
	break;
    case Key_Home:
	setValue( minValue() );
	break;
    case Key_End:
	setValue( maxValue() );
	break;
    default:
	e->ignore();
	return;
    }
}

void QSlider::setValue( int value )
{
    QRangeControl::setValue( value );
#if defined(QT_ACCESSIBILITY_SUPPORT)
    QAccessible::updateAccessibility( this, 0, QAccessible::ValueChanged );
#endif
}


/*!
  Moves the slider one pageStep() up or right.
*/

void QSlider::addStep()
{
    addPage();
}


/*!
  Moves the slider one pageStep() down or left.
*/

void QSlider::subtractStep()
{
    subtractPage();
}


/*!
  Waits for autorepeat.
*/

void QSlider::repeatTimeout()
{
    Q_ASSERT( timer );
    timer->disconnect();
    if ( state == TimingDown )
	connect( timer, SIGNAL(timeout()), SLOT(subtractStep()) );
    else if ( state == TimingUp )
	connect( timer, SIGNAL(timeout()), SLOT(addStep()) );
    timer->start( repeatTime, FALSE );
}


/*!
  Returns the relevant dimension of \a p.
*/

int QSlider::goodPart( const QPoint &p ) const
{
    return (orient == Horizontal) ?  p.x() : p.y();
}

/*!\reimp
*/
QSize QSlider::sizeHint() const
{
    constPolish();
    const int length = 84;
    //    int thick = style() == MotifStyle ? 24 : 16;
    int thick = style().pixelMetric( QStyle::PM_SliderThickness, this );
    const int tickSpace = 5;

    if ( ticks & Above )
	thick += tickSpace;
    if ( ticks & Below )
	thick += tickSpace;
    // if ( style() == WindowsStyle && ticks != Both && ticks != NoMarks )
    // thick += style().pixelMetric( QStyle::PM_SliderLength, this ) / 4; // pointed slider
    if ( orient == Horizontal )
	return QSize( length, thick ).expandedTo( QApplication::globalStrut() );
    else
	return QSize( thick, length ).expandedTo( QApplication::globalStrut() );
}



/*!
  \reimp
*/

QSize QSlider::minimumSizeHint() const
{
    QSize s = sizeHint();
    int length = style().pixelMetric(QStyle::PM_SliderLength, this);
    if ( orient == Horizontal )
	s.setWidth( length );
    else
	s.setHeight( length );

    return s;
}



/*!\reimp
*/
QSizePolicy QSlider::sizePolicy() const
{
    if ( orient == Horizontal )
	return QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    else
	return QSizePolicy(  QSizePolicy::Fixed, QSizePolicy::Expanding );
}


/*
  Returns the number of pixels to use for the business part of the
  slider (i.e., the non-tickmark portion). The remaining space is shared
  equally between the tickmark regions. This function and  sizeHint()
  are closely related; if you change one you almost certainly
  have to change the other.
*/

// style dependent stuff here! Remove when done!
// int QSlider::thickness() const
// {
//     return style().pixelMetric( QStyle::PM_SliderControlThickness, this );
//     int space = (orient == Horizontal) ? height() : width();
//     int n = 0;
//     if ( ticks & Above )
// 	n++;
//     if ( ticks & Below )
// 	n++;
//     if ( !n )
// 	return space;

//     int thick = 6;	// Magic constant to get 5 + 16 + 5
//     if ( style() == WindowsStyle && ticks != Both && ticks != NoMarks ) {
// 	thick += style().sliderLength() / 4;
//     }
//     space -= thick;
//     //### the two sides may be unequal in size
//     if ( space > 0 )
// 	thick += ( space * 2 ) / ( n + 2 );
//     return thick;
// }


/*!
  \property QSlider::tickmarks
  \brief the tickmark settings for this slider

    The valid values are in \l{QSlider::TickSetting}.

  \sa tickInterval
*/

void QSlider::setTickmarks( TickSetting s )
{
    ticks = s;
    initTicks();
    update();
}


/*!
  \property QSlider::tickInterval
  \brief the interval between tickmarks

  This is a value interval, not a pixel interval. If it is 0, the slider
  will choose between lineStep() and pageStep(). The initial value of
  tickInterval is 0.

  \sa QRangeControl::lineStep(), QRangeControl::pageStep()
*/

void QSlider::setTickInterval( int i )
{
    tickInt = QMAX( 0, i );
    update();
}


/*!
  \reimp
 */
void QSlider::styleChange( QStyle& old )
{
    QWidget::styleChange( old );
}

/*!
  \property QSlider::minValue
  \brief the current minimum value of the slider

  When setting this property, the \l QSlider::maxValue is adjusted, if
  necessary, to ensure that the range remains valid.

  \sa setRange()
*/
int QSlider::minValue() const
{
    return QRangeControl::minValue();
}

/*!
  \property QSlider::maxValue
  \brief the current maximum value of the slider

  When setting this property, the \l QSlider::minValue is adjusted, if
  necessary, to ensure that the range remains valid.

  \sa setRange()
*/
int QSlider::maxValue() const
{
    return QRangeControl::maxValue();
}

void QSlider::setMinValue( int minVal )
{
    QRangeControl::setMinValue( minVal );
}

void QSlider::setMaxValue( int maxVal )
{
    QRangeControl::setMaxValue( maxVal );
}

/*!
  \property QSlider::lineStep
  \brief the current line step

  When setting lineStep, the virtual stepChange() function will be called
  if the new line step is different from the previous setting.

  \sa setSteps() QRangeControl::pageStep() setRange()
*/
int QSlider::lineStep() const
{
    return QRangeControl::lineStep();
}

/*!
  \property QSlider::pageStep
  \brief the current line step

  When setting pageStep, the virtual stepChange() function will be called
  if the new page step is different from the previous setting.

  \sa QRangeControl::setSteps() setLineStep() setRange()
*/

int QSlider::pageStep() const
{
    return QRangeControl::pageStep();
}

void QSlider::setLineStep( int i )
{
    setSteps( i, pageStep() );
}

void QSlider::setPageStep( int i )
{
    setSteps( lineStep(), i );
}

/*!
  \property QSlider::value
  \brief the current slider value

  \sa QRangeControl::value() prevValue()
*/

int QSlider::value() const
{
    return QRangeControl::value();
}

#endif
