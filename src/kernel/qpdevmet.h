/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qpdevmet.h#7 $
**
** Definition of QPaintDeviceMetrics class
**
** Author  : Haavard Nord
** Created : 941109
**
** Copyright (C) 1994-1996 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QPDEVMET_H
#define QPDEVMET_H

#include "qwindefs.h"
#include "qpaintd.h"
#include "qpaintdc.h"


class QPaintDeviceMetrics			// paint device metrics
{
public:
    QPaintDeviceMetrics( const QPaintDevice * );

    int	  width()	const	{ return (int)pdev->metric(PDM_WIDTH); }
    int	  height()	const	{ return (int)pdev->metric(PDM_HEIGHT); }
    int	  widthMM()	const	{ return (int)pdev->metric(PDM_WIDTHMM); }
    int	  heightMM()	const	{ return (int)pdev->metric(PDM_HEIGHTMM); }
    int	  numColors()	const	{ return (int)pdev->metric(PDM_NUMCOLORS); }
    int	  depth()	const	{ return (int)pdev->metric(PDM_DEPTH); }

private:
    QPaintDevice *pdev;
};


#endif // QPDEVMET_H
