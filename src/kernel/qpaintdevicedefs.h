/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qpaintdevicedefs.h#31 $
**
** Definition of QPaintDevice constants and flags
**
** Created : 940721
**
** Copyright (C) 1992-1999 Troll Tech AS.  All rights reserved.
**
** This file is part of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Troll Tech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** Licensees holding valid Qt Professional Edition licenses may use this
** file in accordance with the Qt Professional Edition License Agreement
** provided with the Qt Professional Edition.
**
** See http://www.troll.no/pricing.html or email sales@troll.no for
** information about the Professional Edition licensing, or see
** http://www.troll.no/qpl/ for QPL licensing information.
**
*****************************************************************************/

#ifndef QPAINTDEVICEDEFS_H
#define QPAINTDEVICEDEFS_H

#ifndef QT_H
#include "qwindowdefs.h"
#endif // QT_H


// Painter device cmd() identifiers (for programmable, extended devices)

#define PDC_RESERVED_START	0		// codes 0-199 are reserved
#define PDC_RESERVED_STOP	199		//   for Troll Tech

#define PDC_NOP			0		//  <void>
#define PDC_DRAW_FIRST		1
#define PDC_DRAWPOINT		1		// point
#define PDC_MOVETO		2		// point
#define PDC_LINETO		3		// point
#define PDC_DRAWLINE		4		// point,point
#define PDC_DRAWRECT		5		// rect
#define PDC_DRAWROUNDRECT	6		// rect,ival,ival
#define PDC_DRAWELLIPSE		7		// rect
#define PDC_DRAWARC		8		// rect,ival,ival
#define PDC_DRAWPIE		9		// rect,ival,ival
#define PDC_DRAWCHORD		10		// rect,ival,ival
#define PDC_DRAWLINESEGS	11		// ptarr
#define PDC_DRAWPOLYLINE	12		// ptarr
#define PDC_DRAWPOLYGON		13		// ptarr,ival
#define PDC_DRAWQUADBEZIER	14		// ptarr
#define PDC_DRAWTEXT		15		// point,str
#define PDC_DRAWTEXTFRMT	16		// rect,ival,str
#define PDC_DRAWPIXMAP		17		// point,pixmap
#define PDC_DRAWIMAGE		18		// point,image
#define PDC_DRAW_LAST		18
#define PDC_BEGIN		30		//  <void>
#define PDC_END			31		//  <void>
#define PDC_SAVE		32		//  <void>
#define PDC_RESTORE		33		//  <void>
#define PDC_SETDEV		34		// device - PRIVATE
#define PDC_SETBKCOLOR		40		// color
#define PDC_SETBKMODE		41		// ival
#define PDC_SETROP		42		// ival
#define PDC_SETBRUSHORIGIN	43		// point
#define PDC_SETFONT		45		// font
#define PDC_SETPEN		46		// pen
#define PDC_SETBRUSH		47		// brush
#define PDC_SETTABSTOPS		48		// ival
#define PDC_SETTABARRAY		49		// ival,ivec
#define PDC_SETUNIT		50		// ival
#define PDC_SETVXFORM		51		// ival
#define PDC_SETWINDOW		52		// rect
#define PDC_SETVIEWPORT		53		// rect
#define PDC_SETWXFORM		54		// ival
#define PDC_SETWMATRIX		55		// matrix,ival
#define PDC_SAVEWMATRIX	56
#define PDC_RESTOREWMATRIX	57
#define PDC_SETCLIP		60		// ival
#define PDC_SETCLIPRGN		61		// rgn
#define PDC_DRAWTEXT2		62		// point,str
#define PDC_DRAWTEXT2FRMT	63		// rect,ival,str

class QIODevice;
class QString;

union QPDevCmdParam {
    int			 ival;
    int			*ivec;
    QString	        *str;
    const QPoint	*point;
    const QRect		*rect;
    const QPointArray	*ptarr;
    const QPixmap	*pixmap;
    const QImage	*image;
    const QColor	*color;
    const QFont		*font;
    const QPen		*pen;
    const QBrush	*brush;
    const QRegion	*rgn;
    const QWMatrix	*matrix;
    QIODevice		*device;
};

// Painter device metric() identifiers (for all devices)

#define PDM_WIDTH		1
#define PDM_HEIGHT		2
#define PDM_WIDTHMM		3
#define PDM_HEIGHTMM		4
#define PDM_NUMCOLORS		5
#define PDM_DEPTH		6


#endif // QPAINTDEVICEDEFS_H
