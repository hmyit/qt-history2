/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qwindefs.h#11 $
**
** Definition of general window system dependent functions, types and
** constants
**
** Author  : Haavard Nord
** Created : 931029
**
** Copyright (C) 1993,1994 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QWINDEFS_H
#define QWINDEFS_H

#include "qobjdefs.h"


// Class forward definitions

class QApplication;
class QPaintDevice;
class QPaintDeviceMetrics;
class QWidget;
class QWidgetMapper;
class QView;
class QColor;
class QCursor;
class QPoint;
class QSize;
class QRect;
class QPointArray;
class QPainter;
class QRegion;
class QFont;
class QFontMetrics;
class QPen;
class QBrush;
class QWXFMatrix;
class QPixMap;
class QBitMap;
class QMetaFile;
class QPrinter;


// Window system setting

#if defined(_OS_MAC_)
#define _WS_MAC_
#elif defined(_OS_MSDOS_)
#define _WS_WIN16_
#elif defined(_OS_WINNT_)
#define _WS_WIN32_
#elif defined(_OS_OS2_)
#define _WS_PM_
#elif defined(UNIX)
#define _WS_X11_
#endif

#if defined(_WS_WIN16_) || defined(_WS_WIN32_)
#define _WS_WIN_
#endif


// Window system dependent definitions

#if defined(_WS_MAC_)
#endif // _WS_MAC_


#if defined(_WS_WIN_)

#if defined(_WS_WIN32_)
typedef void *HANDLE;
typedef void *WId;
typedef void *HDC;
#elif defined(_WS_WIN16_)
typedef uint HANDLE;
typedef uint WId;
typedef uint HDC;
#endif
typedef struct tagMSG MSG;

HANDLE qWinAppInst();
HANDLE qWinPrevAppInst();
int    qWinAppCmdShow();

#endif // _WS_WIN16_ or _WS_WIN32_


#if defined(_WS_PM_)

typedef ulong HANDLE;
typedef ulong WId;
typedef ulong HAB;
typedef ulong HPS;
typedef ulong HDC;

typedef struct _QMSG   QMSG;
typedef struct _POINTL POINTL;
typedef struct _RECTL  RECTL;

HAB qPMAppInst();

#endif // _WS_PM_


#if defined(_WS_X11_)

typedef unsigned long WId;
typedef unsigned long Atom;
typedef unsigned long Window;
typedef unsigned long Pixmap;
typedef unsigned long Cursor;
typedef unsigned long Font;
typedef struct _XDisplay Display;
typedef union  _XEvent XEvent;
typedef struct _XGC *GC;
typedef struct _XRegion *Region;
struct QXFontStruct;

Display *qXDisplay();
int	 qXScreen();
Window	 qXRootWin();

#endif // _WS_X11_


// Useful macros etc.

class QListM_QPainter;				// internal class for QPainter
#define QPnList QListM_QPainter


// Global platform-independent types and functions

typedef short QCOOT;				// coordinate type

char *qAppName();				// get application name


// Misc functions

void  qAddPreRoutine( void (*)() );
void  qAddPostRoutine( void (*)() );

class QAddPreRoutine {				// class for registering pre-
public:						//    routines
    typedef void (*vf)();
    QAddPreRoutine( void (*p)( int, char ** ) ) { qAddPreRoutine((vf)p); }
    QAddPreRoutine( void (*p)() ) { qAddPreRoutine(p); }
};


// GUI styles

enum GUIStyle {
    MacStyle,
    WindowsStyle,
    PMStyle,
    MotifStyle
};


// Widget flags

typedef ulong WFlags;

#define WState_Created	0x00000001		// widget state flags
#define WState_Disabled 0x00000002
#define WState_Visible	0x00000004
#define WState_Active	0x00000008
#define WState_Paint	0x00000010

#define WType_Overlap	0x00000040		// widget type flags
#define WType_Modal	0x00000080

#define WStyle_Title	0x00000100		// widget style flags
#define WStyle_Border	0x00000200
#define WStyle_Close	0x00000400
#define WStyle_Resize	0x00000800
#define WStyle_Minimize 0x00001000
#define WStyle_Maximize 0x00002000
#define WStyle_MinMax	(WStyle_Minimize | WStyle_Maximize)
#define WStyle_VScroll	0x00004000
#define WStyle_HScroll	0x00008000
#define WStyle_Scroll	(WStyle_VScroll | WStyle_HScroll)
#define WStyle_All	0x0000ff00

#define WWin_AltCreate	0x00010000		// widget flags for Windows
#define WWin_AltClass	0x00020000

#define WEtc_MouseMove	0x01000000		// misc widget flags
#define WEtc_InConfig	0x02000000


// Extra Widget data

struct QWExtra {
    GUIStyle guistyle;				// GUI Style
    short  minw, minh;				// minimum size
    short  maxw, maxh;				// maximum size
    short  incw, inch;				// size increments
};


// Raster operations

enum RasterOp					// raster op/transfer mode
    { CopyROP, OrROP, XorROP, EraseROP,
      NotCopyROP, NotOrROP, NotXorROP, NotEraseROP, NotROP };


// Text output flags

enum TextAlignment				// text alignment
    { AlignLeft=0x0, AlignRight, AlignTop, AlignBottom, AlignCenter };


#endif // QWINDEFS_H
