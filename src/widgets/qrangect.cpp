/****************************************************************************
** $Id: //depot/qt/main/src/widgets/qrangect.cpp#9 $
**
** Implementation of QRangeControl class
**
** Author  : Eirik Eng
** Created : 940427
**
** Copyright (C) 1994,1995 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#include "qrangect.h"
#include "qglobal.h"

RCSTAG("$Id: //depot/qt/main/src/widgets/qrangect.cpp#9 $")


/*----------------------------------------------------------------------------
  \class QRangeControl qrangect.h
  \brief The QRangeControl class provides an integer value within a range.

  This class has many functions to manipulate a value inside a range.
  It was specifically designed for the QScrollBar widget, but it can
  also be practical for other purposes.

  The three virtual functions valueChange(), rangeChange() and stepChange()
  can be reimplemented in a subclass to detect range control changes.
 ----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------
  Constructs a range control with min value 0, max value 99,
  line step 1, page step 10 and initial value 0.
 ----------------------------------------------------------------------------*/

QRangeControl::QRangeControl()
{
    minVal  = 0;
    maxVal  = 99;
    line    = 1;
    page    = 10;
    val	    = prevVal = 0;
}

/*----------------------------------------------------------------------------
  Constructs a range control with the specified parameters.
 ----------------------------------------------------------------------------*/

QRangeControl::QRangeControl( int minValue, int maxValue,
			      int lineStep, int pageStep,
			      int value )
{
    minVal = minValue;
    maxVal = maxValue;
    line   = QABS( lineStep );
    page   = QABS( pageStep );
    val	   = prevVal = value;
    adjustValue();
}


/*----------------------------------------------------------------------------
  \fn int QRangeControl::value() const
  Returns the current range control value.
  \sa setValue(), prevValue()
 ----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  \fn int QRangeControl::prevValue() const
  Returns the previous range control value.
  \sa value()
 ----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Sets the range control value to \e value.

  Adjusts the value if it is less than the \link minValue() min value\endlink
  or greater than the \link maxValue() max value\endlink.

  Calls the virtual valueChange() function if the value is different from the
  previous value.
 ----------------------------------------------------------------------------*/

void QRangeControl::setValue( int value )
{
    directSetValue( value );
    if ( prevVal != val )
	valueChange();
}

/*----------------------------------------------------------------------------
  Sets the range control value directly without calling valueChange().

  Adjusts the value if it is less than the \link minValue() min value\endlink
  or greater than the \link maxValue() max value\endlink.

  \sa setValue()
 ----------------------------------------------------------------------------*/

void QRangeControl::directSetValue(int value)
{
    prevVal = val;
    val	    = value;
    adjustValue();
}

/*----------------------------------------------------------------------------
  Equivalent to \code setValue( value()+pageStep() )\endcode
  \sa subtractPage()
 ----------------------------------------------------------------------------*/

void QRangeControl::addPage()
{
    prevVal = val;
    val	   += page;
    if ( val > maxVal )
	val = maxVal;
    if ( prevVal != val )
	valueChange();
}

/*----------------------------------------------------------------------------
  Equivalent to \code setValue( value()-pageStep() )\endcode
  \sa addPage()
 ----------------------------------------------------------------------------*/

void QRangeControl::subtractPage()
{
    prevVal = val;
    val	   -= page;
    if ( val < minVal )
	val = minVal;
    if ( prevVal != val )
	valueChange();
}

/*----------------------------------------------------------------------------
  Equivalent to \code setValue( value()+lineStep() )\endcode.
  \sa subtractLine()
 ----------------------------------------------------------------------------*/

void QRangeControl::addLine()
{
    prevVal = val;
    val	   += line;
    if ( val > maxVal )
	val = maxVal;
    if ( prevVal != val )
	valueChange();
}

/*----------------------------------------------------------------------------
  Equivalent to \code setValue( value()-lineStep() )\endcode.
  \sa addLine()
 ----------------------------------------------------------------------------*/

void QRangeControl::subtractLine()
{
    prevVal = val;
    val	   -= line;
    if ( val < minVal )
	val = minVal;
    if ( prevVal != val )
	valueChange();
}


/*----------------------------------------------------------------------------
  \fn int QRangeControl::minValue() const
  Returns the current minimum value in the range.
  \sa setRange(), maxValue()
 ----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  \fn int QRangeControl::maxValue() const
  Returns the current maximum value in the range.
  \sa setRange(), minValue()
 ----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Sets the range min value to \e minValue and the max value to \e maxValue.

  Calls the virtual rangeChange() function if the new min and max
  values are different from the previous setting.  Calls the virtual
  valueChange() function if the current value is outside the new range
  and has to be adjusted.

  \sa minValue(), maxValue()
 ----------------------------------------------------------------------------*/

void QRangeControl::setRange( int minValue, int maxValue )
{
    if ( minValue == minVal && maxValue == maxVal )
	return;
    if ( minValue > maxValue ) {
#if defined(CHECK_RANGE)
	warning( "QRangeControl::setRange: minValue > maxValue" );
#endif
	minVal = minValue;
	maxVal = minValue;
    } else {
	minVal = minValue;
	maxVal = maxValue;
    }
    int tmp = val;
    adjustValue();
    rangeChange();
    if ( tmp != val ) {
	prevVal = tmp;
	valueChange();
    }
}


/*----------------------------------------------------------------------------
  \fn int QRangeControl::lineStep() const
  Returns the current line step.
  \sa setSteps(), pageStep()
 ----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  \fn int QRangeControl::pageStep() const
  Returns the current page step.
  \sa setSteps(), lineStep()
 ----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Sets the range line step to \e lineStep and page step to \e pageStep.

  Call the virtual stepChange() function if the new line step and/or page step
  are different from the previous setting.

  \sa setRange()
 ----------------------------------------------------------------------------*/

void QRangeControl::setSteps(int lineStep,int pageStep)
{
    if (lineStep != line || pageStep != page) {
	line = QABS(lineStep);
	page = QABS(pageStep);
	stepChange();
    }
}


/*----------------------------------------------------------------------------
  \internal
  Adjusts the value to make sure it is never less than the min value or
  greater than the max value.
 ----------------------------------------------------------------------------*/

void QRangeControl::adjustValue()
{
    if ( val < minVal )
	val = minVal;
    if ( val > maxVal )
	val = maxVal;
}


/*----------------------------------------------------------------------------
  This virtual function is called whenever the range control value changes.
  \sa setValue(), addPage(), subtractPage(), addLine(), subtractLine()
 ----------------------------------------------------------------------------*/

void QRangeControl::valueChange()
{
}

/*----------------------------------------------------------------------------
  This virtual function is called whenever the range control range changes.
  \sa setRange()
 ----------------------------------------------------------------------------*/

void QRangeControl::rangeChange()
{
}

/*----------------------------------------------------------------------------
  This virtual function is called whenever the range control step value
  changes.
  \sa setSteps()
 ----------------------------------------------------------------------------*/

void QRangeControl::stepChange()
{
}
