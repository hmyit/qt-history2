/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qptr_x11.cpp#216 $
**
** Implementation of QPainter class for X11
**
** Created : 940112
**
** Copyright (C) 1994-1996 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#include "qpainter.h"
#include "qpaintdc.h"
#include "qwidget.h"
#include "qbitmap.h"
#include "qpmcache.h"
#include "qlist.h"
#include "qintdict.h"
#include <ctype.h>
#include <stdlib.h>
#define	 GC GC_QQQ
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

RCSTAG("$Id: //depot/qt/main/src/kernel/qptr_x11.cpp#216 $");


/*****************************************************************************
  Trigonometric function for QPainter

  We have implemented simple sine and cosine function that are called from
  QPainter::drawPie() and QPainter::drawChord() when drawing the outline of
  pies and chords.
  These functions are slower and less accurate than math.h sin() and cos(),
  but with still around 1/70000th sec. execution time (on a 486DX2-66) and
  8 digits accuracy, it should not be the bottleneck in drawing these shapes.
  The advantage is that you don't have to link in the math library.
 *****************************************************************************/

const double Q_PI   = 3.14159265358979323846;	// pi
const double Q_2PI  = 6.28318530717958647693;	// 2*pi
const double Q_PI2  = 1.57079632679489661923;	// pi/2

#if defined(_CC_GNU_) && defined(__i386__)

inline double qcos( double a )
{
    double r;
    __asm__ (
	"fcos"
	: "=t" (r) : "0" (a) );
    return(r);
}

inline double qsin( double a )
{
    double r;
    __asm__ (
	"fsin"
	: "=t" (r) : "0" (a) );
    return(r);
}

double qsincos( double a, bool calcCos=FALSE )
{
    return calcCos ? qcos(a) : qsin(a);
}

#else

double qsincos( double a, bool calcCos=FALSE )
{
    if ( calcCos )				// calculate cosine
	a -= Q_PI2;
    if ( a >= Q_2PI || a <= -Q_2PI ) {		// fix range: -2*pi < a < 2*pi
	int m = (int)(a/Q_2PI);
	a -= Q_2PI*m;
    }
    if ( a < 0.0 )				// 0 <= a < 2*pi
	a += Q_2PI;
    int sign = a > Q_PI ? -1 : 1;
    if ( a >= Q_PI )
	a = Q_2PI - a;
    if ( a >= Q_PI2 )
	a = Q_PI - a;
    if ( calcCos )
	sign = -sign;
    double a2  = a*a;				// here: 0 <= a < pi/4
    double a3  = a2*a;				// make taylor sin sum
    double a5  = a3*a2;
    double a7  = a5*a2;
    double a9  = a7*a2;
    double a11 = a9*a2;
    return (a-a3/6+a5/120-a7/5040+a9/362880-a11/39916800)*sign;
}

inline double qsin( double a ) { return qsincos(a,FALSE); }
inline double qcos( double a ) { return qsincos(a,TRUE); }

#endif


/*****************************************************************************
  QPainter internal GC (Graphics Context) allocator.

  The GC allocator offers two functions; alloc_gc() and free_gc() that
  reuse GC objects instead of calling XCreateGC() and XFreeGC(), which
  are a whole lot slower.
 *****************************************************************************/

struct QGC
{
    GC	 gc;
    char in_use;
    char mono;
};

const  int  gc_array_size = 256;
static QGC  gc_array[gc_array_size];		// array of GCs
static bool gc_array_init = FALSE;


static void init_gc_array()
{
    if ( !gc_array_init ) {
	memset( gc_array, 0, gc_array_size*sizeof(QGC) );
	gc_array_init = TRUE;
    }
}

static void cleanup_gc_array( Display *dpy )
{
    register QGC *p = gc_array;
    int i = gc_array_size;
    if ( gc_array_init ) {
	while ( i-- ) {
	    if ( p->gc )			// destroy GC
		XFreeGC( dpy, p->gc );
	    p++;
	}
	gc_array_init = FALSE;
    }
}

// #define DONT_USE_GC_ARRAY

static GC alloc_gc( Display *dpy, Drawable hd, bool monochrome=FALSE )
{
#if defined(DONT_USE_GC_ARRAY)
    return XCreateGC( dpy, hd, 0, 0 );		// will be slow!
#endif
    register QGC *p = gc_array;
    int i = gc_array_size;
    if ( !gc_array_init )			// not initialized
	init_gc_array();
    while ( i-- ) {
	if ( !p->gc ) {				// create GC (once)
	    p->gc = XCreateGC( dpy, hd, 0, 0 );
	    p->in_use = FALSE;
	    p->mono   = monochrome;
	}
	if ( !p->in_use && p->mono == monochrome ) {
	    p->in_use = TRUE;			// available/compatible GC
	    return p->gc;
	}
	p++;
    }
#if defined(CHECK_NULL)
    warning( "QPainter: Internal error; no available GC" );
#endif
    return XCreateGC( dpy, hd, 0, 0 );
}

static void free_gc( Display *dpy, GC gc )
{
#if defined(DONT_USE_GC_ARRAY)
    ASSERT( dpy != 0 );
    XFreeGC( dpy, gc );				// will be slow
    return;
#endif
    register QGC *p = gc_array;
    int i = gc_array_size;
    if ( gc_array_init ) {
	while ( i-- ) {
	    if ( p->gc == gc ) {
		p->in_use = FALSE;		// set available
		XSetClipMask( dpy, gc, None );	// make it reusable
		XSetFunction( dpy, gc, GXcopy );
		XSetFillStyle( dpy, gc, FillSolid );
		return;
	    }
	    p++;
	}
    }
}


/*****************************************************************************
  QPainter internal GC (Graphics Context) cache for solid pens and brushes.

  The GC cache makes a significant contribution to speeding up drawing.
  Setting new pen and brush colors will make the painter look for another
  GC with the same color instead of changing the color value of the GC
  currently in use. The cache structure is optimized for fast lookup.
  Only solid line pens with line width 0 and solid brushes are cached.
 *****************************************************************************/

struct QGCC					// cached GC
{
    GC	    gc;
    uint    pix;
    int	    count;
    int	    hits;
};

const  int   gc_cache_size = 29;		// multiply by 4
static QGCC *gc_cache_buf;
static QGCC *gc_cache[4*gc_cache_size];
static bool  gc_cache_init = FALSE;


static void init_gc_cache()
{
    if ( !gc_cache_init ) {
	gc_cache_init = TRUE;
	QGCC *g = gc_cache_buf = new QGCC[4*gc_cache_size];
	memset( g, 0, 4*gc_cache_size*sizeof(QGCC) );
	for ( int i=0; i<4*gc_cache_size; i++ )
	    gc_cache[i] = g++;
    }
}


// #define GC_CACHE_STAT
#if defined(GC_CACHE_STAT)
#include "qtstream.h"
#include "qbuffer.h"

static int g_numhits	= 0;
static int g_numcreates = 0;
static int g_numfaults	= 0;
#endif


static void cleanup_gc_cache()
{
    if ( !gc_cache_init )
	return;
#if defined(GC_CACHE_STAT)
    debug( "Number of cache hits = %d", g_numhits );
    debug( "Number of cache creates = %d", g_numcreates );
    debug( "Number of cache faults = %d", g_numfaults );
    for ( int i=0; i<gc_cache_size; i++ ) {
	QString	    str;
	QBuffer	    buf( str );
	buf.open(IO_ReadWrite);
	QTextStream s(&buf);
	s << i << ": ";
	for ( int j=0; j<4; j++ ) {
	    QGCC *g = gc_cache[i*4+j];
	    s << (g->gc ? 'X' : '-') << ',' << g->hits << ','
	      << g->count << '\t';
	}
	s << '\0';
	debug( str );
	buf.close();
    }
#endif
    delete [] gc_cache_buf;
    gc_cache_init = FALSE;
}


static bool obtain_gc( void **ref, GC *gc, uint pix, Display *dpy, HANDLE hd )
{
    if ( !gc_cache_init )
	init_gc_cache();

    int	  k = (pix % gc_cache_size) * 4;
    QGCC *g = gc_cache[k];
    QGCC *prev = 0;

#define NOMATCH (g->gc && g->pix != pix)

    if ( NOMATCH ) {
	prev = g;
	g = gc_cache[++k];
	if ( NOMATCH ) {
	    prev = g;
	    g = gc_cache[++k];
	    if ( NOMATCH ) {
		prev = g;
		g = gc_cache[++k];
		if ( NOMATCH ) {
		    if ( g->count == 0 ) {	// steal this GC
			g->pix	 = pix;
			g->count = 1;
			g->hits	 = 1;
			XSetForeground( dpy, g->gc, pix );
			gc_cache[k]   = prev;
			gc_cache[k-1] = g;
			*ref = (void *)g;
			*gc = g->gc;
			return TRUE;
		    } else {			// all GCs in use
#if defined(GC_CACHE_STAT)
			g_numfaults++;
#endif
			*ref = 0;
			return FALSE;
		    }
		}
	    }
	}
    }

#undef NOMATCH

    *ref = (void *)g;

    if ( g->gc ) {				// reuse existing GC
#if defined(GC_CACHE_STAT)
	g_numhits++;
#endif
	*gc = g->gc;
	g->count++;
	g->hits++;
	if ( prev && g->hits > prev->hits ) {	// maintain LRU order
	    gc_cache[k]   = prev;
	    gc_cache[k-1] = g;
	}
	return TRUE;
    } else {					// create new GC
#if defined(GC_CACHE_STAT)
	g_numcreates++;
#endif
	g->gc	 = alloc_gc( dpy, hd, FALSE );
	g->pix	 = pix;
	g->count = 1;
	g->hits	 = 1;
	*gc = g->gc;
	return FALSE;
    }
}

static inline void release_gc( void *ref )
{
    ((QGCC*)ref)->count--;
}


/*****************************************************************************
  QPainter member functions
 *****************************************************************************/

const int TxNone      = 0;			// transformation codes
const int TxTranslate = 1;
const int TxScale     = 2;
const int TxRotShear  = 3;


/*!
  Internal function that initializes the painter.
*/

void QPainter::initialize()
{
    init_gc_array();
    init_gc_cache();
}

/*!
  Internal function that cleans up the painter.
*/

void QPainter::cleanup()
{
    cleanup_gc_cache();
    cleanup_gc_array( qt_xdisplay() );
}


typedef Q_DECLARE(QIntDictM,QPaintDevice) QPaintDeviceDict;
static QPaintDeviceDict *pdev_dict = 0;

/*!
  Redirects all paint command for a paint device \e pdev to another paint
  device \e replacement.

  A redirected paint device is reset if \e replacement is 0.

  The following example redirects painting of a widget to a pixmap:
  \code
    QPixmap pm( myWidget->width(), myWidget->height() );
    pm.fill( myWidget->backgroundColor() );
    QPainter::redirect( myWidget, &pm );
    myWidget->repaint();
    QPainter::redirect( myWidget, 0 );
  \endcode
*/

void QPainter::redirect( QPaintDevice *pdev, QPaintDevice *replacement )
{
    if ( pdev_dict == 0 ) {
	if ( replacement == 0 )
	    return;
	pdev_dict = new QPaintDeviceDict;
	CHECK_PTR( pdev_dict );
    }
#if defined(CHECK_NULL)
    if ( pdev == 0 )
	warning( "QPainter::redirect: The pdev argument cannot be 0" );
#endif
    if ( replacement ) {
	pdev_dict->insert( (long)pdev, replacement );
    } else {
	pdev_dict->remove( (long)pdev );
	if ( pdev_dict->count() == 0 ) {
	    delete pdev_dict;
	    pdev_dict = 0;
	}
    }
}


void QPainter::init()
{
    flags = IsStartingUp;
    bg_col = white;				// default background color
    bg_mode = TransparentMode;			// default background mode
    rop = CopyROP;				// default ROP
    tabstops = 0;				// default tabbing
    tabarray = 0;
    tabarraylen = 0;
    ps_stack = 0;
    gc = gc_brush = 0;
    pdev = 0;
    dpy  = 0;
    txop = txinv = 0;
    penRef = brushRef = 0;
}

/*!
  Constructs a painter.

  Notice that all painter settings (setPen,setBrush etc.) are reset to
  default values when begin() is called.

  \sa begin(), end()
*/

QPainter::QPainter()
{
    init();
}

/*!
  Constructs a painter that begins painting the paint device \a pd
  immediately.

  This constructor is convenient for short-lived painters, e.g. in
  a \link QWidget::paintEvent() paint event\endlink and should be
  used only once. The constructor calls begin() for you and the QPainter
  destructor automatically calls end().

  Example using begin() and end():
  \code
    void MyWidget::paintEvent( QPaintEvent * )
    {
	QPainter p;
	p.begin( this );
	p.drawLine( ... );	// drawing code
	p.end();
    }
  \endcode

  Example using this constructor:
  \code
    void MyWidget::paintEvent( QPaintEvent * )
    {
	QPainter p( this );
	p.drawLine( ... );	// drawing code
    }
  \endcode

  \sa begin(), end()
*/

QPainter::QPainter( const QPaintDevice *pd )
{
    init();
    begin( pd );
    flags |= CtorBegin;
}

/*!
  Destroys the painter.
*/

QPainter::~QPainter()
{
    if ( isActive() ) {
	if ( (flags & CtorBegin) == 0 ) {
#if defined(CHECK_STATE)
	    warning( "QPainter: You called begin() but not end()" );
#endif
	}
	end();
    }
    if ( tabarray )				// delete tab array
	delete tabarray;
    if ( ps_stack )
	killPStack();
}


/*!
  \fn const QFont &QPainter::font() const
  Returns the current painter font.
  \sa setFont(), QFont
*/

/*!
  Sets a new painter font.

  This font is used by all subsequent drawText() functions.
  The text color is the same as the pen color.

  \sa font(), drawText()
*/

void QPainter::setFont( const QFont &font )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	warning( "QPainter::setFont: Will be reset by begin()" );
#endif
    if ( cfont.d != font.d ) {
	cfont = font;
	setf(DirtyFont);
    }
}


void QPainter::updateFont()
{
    clearf(DirtyFont);
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].font = &cfont;
	if ( !pdev->cmd(PDC_SETFONT,this,param) || !hd )
	    return;
    }
    setf(NoCache);
    if ( penRef )
	updatePen();				// force a non-cached GC
    XSetFont( dpy, gc, cfont.handle() );
}


void QPainter::updatePen()
{
    static char dash_line[]	    = { 7, 3 };
    static char dot_line[]	    = { 1, 3 };
    static char dash_dot_line[]	    = { 7, 3, 2, 3 };
    static char dash_dot_dot_line[] = { 7, 3, 2, 3, 2, 3 };

    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].pen = &cpen;
	if ( !pdev->cmd(PDC_SETPEN,this,param) || !hd )
	    return;
    }

    int	 ps = cpen.style();
    bool cacheIt = !testf(ClipOn|MonoDev|NoCache) &&
		   (ps == NoPen || ps == SolidLine) &&
		   cpen.width() == 0 && rop == CopyROP;

    if ( cacheIt ) {
	if ( gc ) {
	    if ( penRef )
		release_gc( penRef );
	    else
		free_gc( dpy, gc );
	}
	if ( obtain_gc(&penRef, &gc, cpen.color().pixel(), dpy, hd) )
	    return;
	if ( !penRef )
	    gc = alloc_gc( dpy, hd, FALSE );
    } else {
	if ( gc ) {
	    if ( penRef ) {
		release_gc( penRef );
		penRef = 0;
		gc = alloc_gc( dpy, hd, testf(MonoDev) );
	    }
	} else {
	    gc = alloc_gc( dpy, hd, testf(MonoDev) );
	}
    }

    char *dashes = 0;				// custom pen dashes
    int dash_len = 0;				// length of dash list
    int s = LineSolid;

    switch( ps ) {
	case NoPen:
	case SolidLine:
	    s = LineSolid;
	    break;
	case DashLine:
	    dashes = dash_line;
	    dash_len = sizeof(dash_line);
	    break;
	case DotLine:
	    dashes = dot_line;
	    dash_len = sizeof(dot_line);
	    break;
	case DashDotLine:
	    dashes = dash_dot_line;
	    dash_len = sizeof(dash_dot_line);
	    break;
	case DashDotDotLine:
	    dashes = dash_dot_dot_line;
	    dash_len = sizeof(dash_dot_dot_line);
	    break;
    }

    XSetForeground( dpy, gc, cpen.color().pixel() );
    XSetBackground( dpy, gc, bg_col.pixel() );

    if ( dash_len ) {				// make dash list
	XSetDashes( dpy, gc, 0, dashes, dash_len );
	s = bg_mode == TransparentMode ? LineOnOffDash : LineDoubleDash;
    }
    XSetLineAttributes( dpy, gc, cpen.width(), s, CapButt, JoinMiter );
}


void QPainter::updateBrush()
{
static uchar dense1_pat[] = { 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff };
static uchar dense2_pat[] = { 0x77, 0xff, 0xdd, 0xff, 0x77, 0xff, 0xdd, 0xff };
static uchar dense3_pat[] = { 0x55, 0xbb, 0x55, 0xee, 0x55, 0xbb, 0x55, 0xee };
static uchar dense4_pat[] = { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa };
static uchar dense5_pat[] = { 0xaa, 0x44, 0xaa, 0x11, 0xaa, 0x44, 0xaa, 0x11 };
static uchar dense6_pat[] = { 0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00 };
static uchar dense7_pat[] = { 0x00, 0x44, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00 };
static uchar hor_pat[] = {			// horizontal pattern
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static uchar ver_pat[] = {			// vertical pattern
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20 };
static uchar cross_pat[] = {			// cross pattern
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0xff, 0xff, 0xff,
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
    0x08, 0x82, 0x20, 0xff, 0xff, 0xff, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0xff, 0xff, 0xff,
    0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
    0x08, 0x82, 0x20, 0xff, 0xff, 0xff, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20 };
static uchar bdiag_pat[] = {			// backward diagonal pattern
    0x20, 0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x04, 0x02, 0x02, 0x01, 0x01,
    0x80, 0x80, 0x40, 0x40, 0x20, 0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x04,
    0x02, 0x02, 0x01, 0x01, 0x80, 0x80, 0x40, 0x40 };
static uchar fdiag_pat[] = {			// forward diagonal pattern
    0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x10, 0x10, 0x20, 0x20, 0x40, 0x40,
    0x80, 0x80, 0x01, 0x01, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x10, 0x10,
    0x20, 0x20, 0x40, 0x40, 0x80, 0x80, 0x01, 0x01 };
static uchar dcross_pat[] = {			// diagonal cross pattern
    0x22, 0x22, 0x14, 0x14, 0x08, 0x08, 0x14, 0x14, 0x22, 0x22, 0x41, 0x41,
    0x80, 0x80, 0x41, 0x41, 0x22, 0x22, 0x14, 0x14, 0x08, 0x08, 0x14, 0x14,
    0x22, 0x22, 0x41, 0x41, 0x80, 0x80, 0x41, 0x41 };
static uchar *pat_tbl[] = {
    dense1_pat, dense2_pat, dense3_pat, dense4_pat, dense5_pat,
    dense6_pat, dense7_pat,
    hor_pat, ver_pat, cross_pat, bdiag_pat, fdiag_pat, dcross_pat };

    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].brush = &cbrush;
	if ( !pdev->cmd(PDC_SETBRUSH,this,param) || !hd )
	    return;
    }

    int	 bs	 = cbrush.style();
    bool cacheIt = !testf(ClipOn|MonoDev|NoCache) &&
		   (bs == NoBrush || bs == SolidPattern) &&
		   bro.x() == 0 && bro.y() == 0 && rop == CopyROP;

    if ( cacheIt ) {
	if ( gc_brush ) {
	    if ( brushRef )
		release_gc( brushRef );
	    else
		free_gc( dpy, gc_brush );
	}
	if ( obtain_gc(&brushRef, &gc_brush, cbrush.color().pixel(), dpy, hd) )
	    return;
	if ( !brushRef )
	    gc_brush = alloc_gc( dpy, hd, FALSE );
    } else {
	if ( gc_brush ) {
	    if ( brushRef ) {
		release_gc( brushRef );
		brushRef = 0;
		gc_brush = alloc_gc( dpy, hd, testf(MonoDev) );
	    }
	} else {
	    gc_brush = alloc_gc( dpy, hd, testf(MonoDev) );
	}
    }

    uchar *pat = 0;				// pattern
    int d = 0;					// defalt pattern size: d*d
    int s  = FillSolid;
    if ( bs >= Dense1Pattern && bs <= DiagCrossPattern ) {
	pat = pat_tbl[ bs-Dense1Pattern ];
	if ( bs <= Dense7Pattern )
	    d = 8;
	else if ( bs <= CrossPattern )
	    d = 24;
	else
	    d = 16;
    }

    XSetLineAttributes( dpy, gc_brush, 0, LineSolid, CapButt, JoinMiter );
    XSetForeground( dpy, gc_brush, cbrush.color().pixel() );
    XSetBackground( dpy, gc_brush, bg_col.pixel() );

    if ( bs == CustomPattern || pat ) {
	QPixmap *pm;
	if ( pat ) {
	    QString key;
	    key.sprintf( "$qt-brush$%d", bs );
	    pm = QPixmapCache::find( key );
	    if ( !pm ) {			// not already in pm dict
		pm = new QBitmap( d, d, pat, TRUE );
		CHECK_PTR( pm );
		QPixmapCache::insert( key, pm );
	    }
	    if ( cbrush.data->pixmap )
		delete cbrush.data->pixmap;
	    cbrush.data->pixmap = new QPixmap( *pm );
	}
	pm = cbrush.data->pixmap;
	if ( pm->depth() == 1 ) {
	    XSetStipple( dpy, gc_brush, pm->handle() );
	    s = bg_mode == TransparentMode ? FillStippled : FillOpaqueStippled;
	} else {
	    XSetTile( dpy, gc_brush, pm->handle() );
	    s = FillTiled;
	}
    }
    XSetFillStyle( dpy, gc_brush, s );
}


/*!
  Begins painting the paint device \a pd and returns TRUE if successful,
  or FALSE if it cannot begin painting. Call end() when you have finished
  painting.

  On the X Window System, paint commands are buffered and may not appear
  on the screen immediately. The flush() function flushes the buffer.

  As an alternative to calling begin() and end(), you can use the QPainter
  constructor that takes a paint device argument. This is for short-lived
  painters, for example in \link QWidget::paintEvent() paint events\endlink.

  This function initializes all painter settings:
  <ul>
  <li>The \link setFont() font\endlink is set to the default \link
  QApplication::setFont() application font\endlink, or to the \link
  QWidget::setFont() widget's font\endlink if \e pd is a widget.
  <li>The \link setPen() pen\endlink is set to QPen(black,0,SolidLine),
  or to QPen(\link QWidget::foregroundColor() widget->foreground()\endlink,
  0,SolidLine) if \e pd is a widget.
  <li>The \link setBrush() brush\endlink is set to QBrush(NoBrush).
  <li>The \link setBackgroundColor() background color\endlink is set
  to white, or to the \link QWidget::setBackgroundColor() widget's
  background color\endlink if \e pd is a widget.
  <li>The \link setBackgroundMode() background mode\endlink is set
  to \c TransparentMode.
  <li>The \link setRasterOp() raster operation\endlink is set to
  \c CopyROP.
  <li>The \link setBrushOrigin() brush origin\endlink is set to (0,0).
  <li>The \link setViewXForm() view transformation\endlink setting
  is turned off.
  <li>The \link setWindow() window\endlink is set to the paint device
  rectangle; (0,0,pd->width(),pd->height()).
  <li>The \link setViewport() viewport\endlink is set to the paint device
  rectangle; (0,0,pd->width(),pd->height()).
  <li>The \link setWorldXForm() world transformation\endlink setting
  is turned off.
  <li>The \link setWorldMatrix() world matrix\endlink is set to the
  \link QWMatrix::reset() identify matrix\endlink.
  <li>\link setClipping() Clipping\endlink is disabled.
  <li>The \link setClipRegion() clip region\endlink is set to an empty region.
  </ul>

  \warning A paint device can only be painted by one painter at a time.

  \sa end(), flush(), setFont(), setPen(), setBrush(), setBackgroundColor(),
  setBackgroundMode(), setRasterOp(), setBrushOrigin(), setViewXForm(),
  setWindow(), setViewport(), setWorldXForm(), setWorldMatrix(),
  setClipRegion()
*/

bool QPainter::begin( const QPaintDevice *pd )
{
    if ( isActive() ) {				// already active painting
#if defined(CHECK_STATE)
	warning( "QPainter::begin: Painter is already active."
		 "\n\tYou must end() the painter before a second begin()" );
#endif
	return FALSE;
    }
    if ( pd == 0 ) {
#if defined(CHECK_NULL)
	warning( "QPainter::begin: Paint device cannot be null" );
#endif
	return FALSE;
    }

    QWidget *copyFrom = 0;
    if ( pdev_dict ) {				// redirected paint device?
	pdev = pdev_dict->find( (long)pd );
	if ( pdev ) {
	    if ( pd->devType() == PDT_WIDGET )
		copyFrom = (QWidget *)pd;	// copy widget settings
	} else {
	    pdev = (QPaintDevice *)pd;
	}
    } else {
	pdev = (QPaintDevice *)pd;
    }

    if ( pdev->paintingActive() ) {		// somebody else is already painting
#if defined(CHECK_STATE)
	warning( "QPainter::begin: Another QPainter is already painting "
		 "this device;\n\tA paint device can only be painted by "
		 "one QPainter at a time" );
#endif
	return FALSE;
    }

    bool reinit = flags != IsStartingUp;	// 2nd or 3rd etc. time called
    flags = IsActive | DirtyFont;		// init flags
    int dt = pdev->devType();			// get the device type

    if ( (pdev->devFlags & PDF_EXTDEV) != 0 )	// this is an extended device
	setf(ExtDev);
    else if ( dt == PDT_PIXMAP )		// device is a pixmap
	((QPixmap*)pdev)->detach();		// will modify it

    dpy = pdev->dpy;				// get display variable
    hd	= pdev->hd;				// get handle to drawable

    if ( testf(ExtDev) ) {			// external device
	if ( !pdev->cmd(PDC_BEGIN,this,0) ) {	// could not begin painting
	    pdev = 0;
	    return FALSE;
	}
	if ( tabstops )				// update tabstops for device
	    setTabStops( tabstops );
	if ( tabarray )				// update tabarray for device
	    setTabArray( tabarray );
    }

    pdev->devFlags |= PDF_PAINTACTIVE;		// also tell paint device
    bro = curPt = QPoint( 0, 0 );
    if ( reinit ) {
	bg_mode = TransparentMode;		// default background mode
	rop = CopyROP;				// default ROP
	wxmat.reset();				// reset world xform matrix
	txop = txinv = 0;
	if ( dt != PDT_WIDGET ) {
	    QFont  defaultFont;			// default drawing tools
	    QPen   defaultPen;
	    QBrush defaultBrush;
	    cfont  = defaultFont;		// set these drawing tools
	    cpen   = defaultPen;
	    cbrush = defaultBrush;
	    bg_col = white;			// default background color
	}
    }
    wx = wy = vx = vy = 0;			// default view origins

    if ( dt == PDT_WIDGET ) {			// device is a widget
	QWidget *w = (QWidget*)pdev;
	cfont = w->font();			// use widget font
	cpen = QPen( w->foregroundColor() );	// use widget fg color
	if ( reinit ) {
	    QBrush defaultBrush;
	    cbrush = defaultBrush;
	}
	bg_col = w->backgroundColor();		// use widget bg color
	ww = vw = w->width();			// default view size
	wh = vh = w->height();
	if ( w->testWFlags(WPaintUnclipped) ) { // paint direct on device
	    setf( NoCache );
	    updatePen();
	    updateBrush();
	    XSetSubwindowMode( dpy, gc, IncludeInferiors );
	    XSetSubwindowMode( dpy, gc_brush, IncludeInferiors );
	}
    } else if ( dt == PDT_PIXMAP ) {		// device is a pixmap
	QPixmap *pm = (QPixmap*)pdev;
	if ( pm->isNull() ) {
#if defined(CHECK_NULL)
	    warning( "QPainter::begin: Cannot paint null pixmap" );
#endif
	    end();
	    return FALSE;
	}
	bool mono = pm->depth() == 1;		// monochrome bitmap
	if ( mono ) {
	    setf( MonoDev );
	    bg_col = color0;
	    cpen.setColor( color1 );
	}
	ww = vw = pm->width();			// default view size
	wh = vh = pm->height();
    } else if ( testf(ExtDev) ) {		// external device
	ww = vw = pdev->metric( PDM_WIDTH );
	wh = vh = pdev->metric( PDM_HEIGHT );
    }
    if ( ww == 0 )
	ww = wh = vw = vh = 1024;
    if ( copyFrom ) {				// copy redirected widget
	cfont = copyFrom->font();
	cpen = QPen( copyFrom->foregroundColor() );
	bg_col = copyFrom->backgroundColor();
    }
    if ( testf(ExtDev) ) {			// external device
	setBackgroundColor( bg_col );		// default background color
	setBackgroundMode( TransparentMode );	// default background mode
	setRasterOp( CopyROP );			// default raster operation
	updateFont();
    }
    updatePen();
    updateBrush();
    return TRUE;
}

/*!
  Ends painting.  Any resources used while painting are released.
  \sa begin()
*/

bool QPainter::end()				// end painting
{
    if ( !isActive() ) {
#if defined(CHECK_STATE)
	warning( "QPainter::end: Missing begin() or begin() failed" );
#endif
	return FALSE;
    }
    if ( testf(FontMet) )			// remove references to this
	QFontMetrics::reset( this );
    if ( testf(FontInf) )			// remove references to this
	QFontInfo::reset( this );

    if ( gc_brush ) {				// restore brush gc
	if ( brushRef ) {
	    release_gc( brushRef );
	    brushRef = 0;
	} else {
	    free_gc( dpy, gc_brush );
	}
	gc_brush = 0;

    }
    if ( gc ) {					// restore pen gc
	if ( penRef ) {
	    release_gc( penRef );
	    penRef = 0;
	} else {
	    free_gc( dpy, gc );
	}
	gc = 0;
    }

    if ( testf(ExtDev) )
	pdev->cmd( PDC_END, this, 0 );

    flags = 0;
    pdev->devFlags &= ~PDF_PAINTACTIVE;
    pdev = 0;
    dpy  = 0;
    return TRUE;
}


/*!
  Flushes any buffered drawing operations.
*/

void QPainter::flush()
{
    if ( isActive() && dpy )
	XFlush( dpy );
}


/*!
  Sets the background color of the painter to \e c.

  The background color is the color that is filled in when drawing
  opaque text, stippled lines and bitmaps.
  The background color has no effect when transparent background mode
  is set.

  \sa backgroundColor(), setBackgroundMode()
*/

void QPainter::setBackgroundColor( const QColor &c )
{
    if ( !isActive() ) {
#if defined(CHECK_STATE)
	warning( "QPainter::setBackgroundColor: Call begin() first" );
#endif
	return;
    }
    bg_col = c;
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].color = &bg_col;
	if ( !pdev->cmd(PDC_SETBKCOLOR,this,param) || !hd )
	    return;
    }
    if ( !penRef )
	updatePen();				// update pen setting
    if ( !brushRef )
	updateBrush();				// update brush setting
}

/*!
  Sets the background mode of the painter to \e m, which must be one of:
  <ul>
  <li> \c TransparentMode (default)
  <li> \c OpaqueMode
  </ul>

  Transparent mode draws stippled lines and text without setting the
  background pixels. Opaque mode fills these space with the current
  background color.

  In order to draw a bitmap or pixmap transparently, you must use
  QPixmap::setMask().

  \sa backgroundMode(), setBackgroundColor() */

void QPainter::setBackgroundMode( BGMode m )
{
    if ( !isActive() ) {
#if defined(CHECK_STATE)
	warning( "QPainter::setBackgroundMode: Call begin() first" );
#endif
	return;
    }
    if ( m != TransparentMode && m != OpaqueMode ) {
#if defined(CHECK_RANGE)
	warning( "QPainter::setBackgroundMode: Invalid mode" );
#endif
	return;
    }
    bg_mode = m;
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].ival = m;
	if ( !pdev->cmd(PDC_SETBKMODE,this,param) || !hd )
	    return;
    }
    if ( !penRef )
	updatePen();				// update pen setting
    if ( !brushRef )
	updateBrush();				// update brush setting
}

static short ropCodes[] = {
    GXcopy, GXor, GXxor, GXandInverted,
    GXcopyInverted, GXorInverted, GXequiv, GXand, GXinvert
};

/*!
  Sets the raster operation to \e r.

  The \e r parameter must be one of:
  <ul>
  <li> \c CopyROP:	dst = src.
  <li> \c OrROP:	dst = dst OR src.
  <li> \c XorROP:	dst = dst XOR src.
  <li> \c EraseROP:	dst = (NOT src) AND dst
  <li> \c NotCopyROP:	dst = NOT src
  <li> \c NotOrROP:	dst = (NOT src) OR dst
  <li> \c NotXorROP:	dst = (NOT src) XOR dst
  <li> \c NotEraseROP:	dst = src AND dst
  <li> \c NotROP:	dst = NOT dst
  </ul>

  \sa rasterOp()
*/

void QPainter::setRasterOp( RasterOp r )
{
    if ( !isActive() ) {
#if defined(CHECK_STATE)
	warning( "QPainter::setRasterOp: Call begin() first" );
#endif
	return;
    }
    if ( (uint)r > NotROP ) {
#if defined(CHECK_RANGE)
	warning( "QPainter::setRasterOp: Invalid ROP code" );
#endif
	return;
    }
    rop = r;
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].ival = r;
	if ( !pdev->cmd(PDC_SETROP,this,param) || !hd )
	    return;
    }
    if ( penRef )
	updatePen();				// get non-cached pen GC
    if ( brushRef )
	updateBrush();				// get non-cached brush GC
    XSetFunction( dpy, gc, ropCodes[rop] );
    XSetFunction( dpy, gc_brush, ropCodes[rop] );
}


/*!
  Sets the brush origin to \e (x,y).

  The brush origin specifies the (0,0) coordinate of the painter's brush.
  This setting is only necessary for pattern brushes or pixmap brushes.

  \sa brushOrigin()
*/

void QPainter::setBrushOrigin( int x, int y )
{
    if ( !isActive() ) {
#if defined(CHECK_STATE)
	warning( "QPainter::setBrushOrigin: Call begin() first" );
#endif
	return;
    }
    bro = QPoint(x,y);
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].point = &bro;
	if ( !pdev->cmd(PDC_SETBRUSHORIGIN,this,param) || !hd )
	    return;
    }
    if ( brushRef )
	updateBrush();				// get non-cached brush GC
    XSetTSOrigin( dpy, gc_brush, x, y );
}


/*!
  \internal
  Updates an internal integer transformation matrix.
*/

void QPainter::updateXForm()
{
    QWMatrix m;
    if ( testf(VxF) ) {
	m.translate( vx, vy );
	m.scale( 1.0*vw/ww, 1.0*vh/wh );
	m.translate( -wx, -wy );
    }
    if ( testf(WxF) ) {
	if ( testf(VxF) )
	    m = wxmat * m;
	else
	    m = wxmat;
    }
    wm11 = qRound((double)m.m11()*65536.0);	// make integer matrix
    wm12 = qRound((double)m.m12()*65536.0);
    wm21 = qRound((double)m.m21()*65536.0);
    wm22 = qRound((double)m.m22()*65536.0);
    wdx	 = qRound((double)m.dx() *65536.0);
    wdy	 = qRound((double)m.dy() *65536.0);

    txinv = FALSE;				// no inverted matrix
    txop  = TxNone;
    if ( wm12 == 0 && wm21 == 0 && wm11 >= 0 && wm22 >= 0 ) {
	if ( wm11 == 65536 && wm22 == 65536 ) {
	    if ( wdx != 0 || wdy != 0 )
		txop = TxTranslate;
	} else {
	    txop = TxScale;
	}
    } else {
	txop = TxRotShear;
    }
}


/*!
  \internal
  Updates an internal integer inverse transformation matrix.
*/

void QPainter::updateInvXForm()
{
#if defined(CHECK_STATE)
    ASSERT( txinv == FALSE );
#endif
    txinv = TRUE;				// creating inverted matrix
    bool invertible;
    QWMatrix m;
    if ( testf(VxF) ) {
	m.translate( vx, vy );
	m.scale( 1.0*vw/ww, 1.0*vh/wh );
	m.translate( -wx, -wy );
    }
    if ( testf(WxF) ) {
	if ( testf(VxF) )
	    m = wxmat * m;
	else
	    m = wxmat;
    }
    m = m.invert( &invertible );		// invert matrix
    im11 = qRound((double)m.m11()*65536.0);	// make integer matrix
    im12 = qRound((double)m.m12()*65536.0);
    im21 = qRound((double)m.m21()*65536.0);
    im22 = qRound((double)m.m22()*65536.0);
    idx	 = qRound((double)m.dx() *65536.0);
    idy	 = qRound((double)m.dy() *65536.0);
}


/*!
  \internal
  Maps a point from logical coordinates to device coordinates.
*/

void QPainter::map( int x, int y, int *rx, int *ry ) const
{
     switch ( txop ) {
	case TxNone:
	    *rx = x;  *ry = y;
	    break;
	case TxTranslate:
	    *rx = x + wdx/65536;
	    *ry = y + wdy/65536;
	    break;
	case TxScale:
	    *rx = wm11*x + wdx;
	    *rx = *rx > 0 ? (*rx + 32768)/65536 : (*rx - 32768)/65536;
	    *ry = wm22*y + wdy;
	    *ry = *ry > 0 ? (*ry + 32768)/65536 : (*ry - 32768)/65536;
	    break;
	default:
	    *rx = wm11*x + wm21*y+wdx;
	    *rx = *rx > 0 ? (*rx + 32768)/65536 : (*rx - 32768)/65536;
	    *ry = wm12*x + wm22*y+wdy;
	    *ry = *ry > 0 ? (*ry + 32768)/65536 : (*ry - 32768)/65536;
	    break;
    }
}

/*!
  \internal
  Maps a rectangle from logical coordinates to device coordinates.
  This internal function does not handle rotation and/or shear.
*/

void QPainter::map( int x, int y, int w, int h,
		    int *rx, int *ry, int *rw, int *rh ) const
{
     switch ( txop ) {
	case TxNone:
	    *rx = x;  *ry = y;
	    *rw = w;  *rh = h;
	    break;
	case TxTranslate:
	    *rx = x + wdx/65536;
	    *ry = y + wdy/65536;
	    *rw = w;  *rh = h;
	    break;
	case TxScale:
	    *rx = wm11*x + wdx;
	    *rx = *rx > 0 ? (*rx + 32768)/65536 : (*rx - 32768)/65536;
	    *ry = wm22*y + wdy;
	    *ry = *ry > 0 ? (*ry + 32768)/65536 : (*ry - 32768)/65536;
	    *rw = wm11*w;
	    *rw = *rw > 0 ? (*rw + 32768)/65536 : (*rw - 32768)/65536;
	    *rh = wm22*h;
	    *rh = *rh > 0 ? (*rh + 32768)/65536 : (*rh - 32768)/65536;
	    break;
	default:
#if defined(CHECK_STATE)
	    warning( "QPainter::map: Internal error" );
#endif
	    break;
    }
}

/*!
  \internal
  Maps a point from device coordinates to logical coordinates.
*/

void QPainter::mapInv( int x, int y, int *rx, int *ry ) const
{
#if defined(CHECK_STATE)
    if ( !txinv )
	warning( "QPainter::mapInv: Internal error" );
#endif
    *rx = im11*x + im21*y+idx;
    *rx = *rx > 0 ? (*rx + 32768)/65536 : (*rx - 32768)/65536;
    *ry = im12*x + im22*y+idy;
    *ry = *ry > 0 ? (*ry + 32768)/65536 : (*ry - 32768)/65536;
}

/*!
  \internal
  Maps a rectangle from device coordinates to logical coordinates.
  Cannot handle rotation and/or shear.
*/

void QPainter::mapInv( int x, int y, int w, int h,
		       int *rx, int *ry, int *rw, int *rh ) const
{
#if defined(CHECK_STATE)
    if ( !txinv || txop == TxRotShear )
	warning( "QPainter::mapInv: Internal error" );
#endif
    *rx = im11*x + idx;
    *rx = *rx > 0 ? (*rx + 32768)/65536 : (*rx - 32768)/65536;
    *ry = im22*y + idy;
    *ry = *ry > 0 ? (*ry + 32768)/65536 : (*ry - 32768)/65536;
    *rw = im11*w;
    *rw = *rw > 0 ? (*rw + 32768)/65536 : (*rw - 32768)/65536;
    *rh = im22*h;
    *rh = *rh > 0 ? (*rh + 32768)/65536 : (*rh - 32768)/65536;
}


/*!
  Returns the point \e pv transformed from user coordinates to device
  coordinates.

  \sa xFormDev(), QWMatrix::xForm()
*/

QPoint QPainter::xForm( const QPoint &pv ) const
{
    if ( txop == TxNone )
	return pv;
    int x=pv.x(), y=pv.y();
    map( x, y, &x, &y );
    return QPoint( x, y );
}

/*!
  Returns the rectangle \e rv transformed from user coordinates to device
  coordinates.

  If world transformation is enabled and rotation or shearing has been
  specified, then the bounding rectangle is returned.

  \sa xFormDev(), QWMatrix::xForm()
*/

QRect QPainter::xForm( const QRect &rv ) const
{
    if ( txop == TxNone )
	return rv;
    if ( txop == TxRotShear ) {			// rotation/shear
	QPointArray a( rv );
	a = xForm( a );
	return a.boundingRect();
    } else {					// translation/scale
	int x, y, w, h;
	rv.rect( &x, &y, &w, &h );
	map( x, y, w, h, &x, &y, &w, &h );
	return QRect( x, y, w, h );
    }
}

/*!
  Returns the point array \e av transformed from user coordinates to device
  coordinates.
  \sa xFormDev(), QWMatrix::xForm()
*/

QPointArray QPainter::xForm( const QPointArray &av ) const
{
    if ( txop == TxNone )
	return av;
    QPointArray a = av.copy();
    int x, y, i;
    for ( i=0; i<(int)a.size(); i++ ) {
	a.point( i, &x, &y );
	map( x, y, &x, &y );
	a.setPoint( i, x, y );
    }
    return a;
}

/*!
  Returns the point \e pv transformed from device coordinates to user
  coordinates.
  \sa xForm(), QWMatrix::xForm()
*/

QPoint QPainter::xFormDev( const QPoint &pd ) const
{
    if ( txop == TxNone )
	return pd;
    if ( !txinv ) {
	QPainter *that = (QPainter*)this;	// mutable
	that->updateInvXForm();
    }
    int x=pd.x(), y=pd.y();
    mapInv( x, y, &x, &y );
    return QPoint( x, y );
}

/*!
  Returns the rectangle \e rv transformed from device coordinates to user
  coordinates.

  If world transformation is enabled and rotation or shearing is used,
  then the bounding rectangle is returned.

  \sa xForm(), QWMatrix::xForm()
*/

QRect QPainter::xFormDev( const QRect &rd ) const
{
    if ( txop == TxNone )
	return rd;
    if ( !txinv ) {
	QPainter *that = (QPainter*)this;	// mutable
	that->updateInvXForm();
    }
    if ( txop == TxRotShear ) {			// rotation/shear
	QPointArray a( rd );
	a = xFormDev( a );
	return a.boundingRect();
    } else {					// translation/scale
	int x, y, w, h;
	rd.rect( &x, &y, &w, &h );
	mapInv( x, y, w, h, &x, &y, &w, &h );
	return QRect( x, y, w, h );
    }
}

/*!
  Returns the point array \e av transformed from device coordinates to user
  coordinates.
  \sa xForm(), QWMatrix::xForm()
*/

QPointArray QPainter::xFormDev( const QPointArray &ad ) const
{
    if ( txop == TxNone )
	return ad;
    QPointArray a = ad.copy();
    int x, y, i;
    for ( i=0; i<(int)a.size(); i++ ) {
	a.point( i, &x, &y );
	mapInv( x, y, &x, &y );
	a.setPoint( i, x, y );
    }
    return a;
}


/*!
  Enables clipping if \e enable is TRUE, or disables clipping if \e enable
  is FALSE.
  \sa hasClipping(), setClipRect(), setClipRegion()
*/

void QPainter::setClipping( bool enable )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	warning( "QPainter::setClipping: Will be reset by begin()" );
#endif
    if ( !isActive() || enable == testf(ClipOn) )
	return;
    setf( ClipOn, enable );
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].ival = enable;
	if ( !pdev->cmd(PDC_SETCLIP,this,param) || !hd )
	    return;
    }
    if ( enable ) {
	if ( penRef )
	    updatePen();
	XSetRegion( dpy, gc, crgn.handle() );
	if ( brushRef )
	    updateBrush();
	XSetRegion( dpy, gc_brush, crgn.handle() );
    } else {
	XSetClipMask( dpy, gc, None );
	XSetClipMask( dpy, gc_brush, None );
    }
}


/*!
  \overload void QPainter::setClipRect( const QRect &r )
*/

void QPainter::setClipRect( const QRect &r )
{
    QRegion rgn( r );
    setClipRegion( rgn );
}

/*!
  Sets the clip region to \e rgn and enables clipping.

  Note that the clip region is given in physical device coordinates and
  \e not subject to any \link setWorldMatrix() coordinate
  transformation\endlink.

  \sa setClipRect(), clipRegion(), setClipping()
*/

void QPainter::setClipRegion( const QRegion &rgn )
{
#if defined(CHECK_STATE)
    if ( !isActive() )
	warning( "QPainter::setClipRegion: Will be reset by begin()" );
#endif
    crgn = rgn;
    if ( testf(ExtDev) ) {
	QPDevCmdParam param[1];
	param[0].rgn = &crgn;
	if ( !pdev->cmd(PDC_SETCLIPRGN,this,param) || !hd )
	    return;
    }
    clearf( ClipOn );				// be sure to update clip rgn
    setClipping( TRUE );
}


/*!
  Internal function for drawing a polygon.
*/

void QPainter::drawPolyInternal( const QPointArray &a, bool close )
{
    if ( a.size() < 2 )
	return;

    int x1, y1, x2, y2;				// connect last to first point
    a.point( a.size()-1, &x1, &y1 );
    a.point( 0, &x2, &y2 );
    bool do_close = close && !(x1 == x2 && y1 == y2);

    if ( close && cbrush.style() != NoBrush ) {	// draw filled polygon
	XFillPolygon( dpy, hd, gc_brush, (XPoint*)a.data(), a.size(),
		      Nonconvex, CoordModeOrigin );
	if ( cpen.style() == NoPen ) {		// draw fake outline
	    XDrawLines( dpy, hd, gc_brush, (XPoint*)a.data(), a.size(),
			CoordModeOrigin );
	    if ( do_close )
		XDrawLine( dpy, hd, gc_brush, x1, y1, x2, y2 );
	}
    }
    if ( cpen.style() != NoPen ) {		// draw outline
	XDrawLines( dpy, hd, gc, (XPoint*)a.data(), a.size(), CoordModeOrigin);
	if ( do_close )
	    XDrawLine( dpy, hd, gc, x1, y1, x2, y2 );
    }
}


/*!
  Draws/plots a single point at \e (x,y) using the current pen.
*/

void QPainter::drawPoint( int x, int y )
{
    if ( !isActive() || cpen.style() == NoPen )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[1];
	    QPoint p( x, y );
	    param[0].point = &p;
	    if ( !pdev->cmd(PDC_DRAWPOINT,this,param) || !hd )
		return;
	}
	map( x, y, &x, &y );
    }
    XDrawPoint( dpy, hd, gc, x, y );
}

/*!
  Draws/plots an array of points using the current pen.  The
  \a index and \a npoints arguments allow a subsequence of the
  array to be drawn.
*/
void QPainter::drawPoints( const QPointArray& a, int index, int npoints )
{
    if ( npoints < 0 )
	npoints = a.size() - index;
    if ( index + npoints > (int)a.size() )
	npoints = a.size() - index;
    if ( !isActive() || npoints < 1 || index < 0 || cpen.style() == NoPen )
	return;
    QPointArray pa = a;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[1];
	    for (int i=0; i<npoints; i++) {
		QPoint p( pa[index+i].x(), pa[index+i].y() );
		param[0].point = &p;
		if ( !pdev->cmd(PDC_DRAWPOINT,this,param))
		    return;
	    }
	    if ( !hd ) return;
	}
	if ( txop != TxNone )
	    pa = xForm( a );
    }
    XDrawPoints( dpy, hd, gc, (XPoint*)(pa.data()+index), npoints,
		    CoordModeOrigin );
}

/*!
  Sets the current point.
  \sa lineTo(), drawLine()
*/

void QPainter::moveTo( int x, int y )
{
    if ( !isActive() )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[1];
	    QPoint p( x, y );
	    param[0].point = &p;
	    if ( !pdev->cmd(PDC_MOVETO,this,param) || !hd )
		return;
	}
	map( x, y, &x, &y );
    }
    curPt = QPoint( x, y );
}

/*!
  Draws a line from the current point to \e (x,y) and sets this to the new
  current point. Both endpoints are are drawn.
  \sa moveTo(), drawLine()
*/

void QPainter::lineTo( int x, int y )
{
    if ( !isActive() )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[1];
	    QPoint p( x, y );
	    param[0].point = &p;
	    if ( !pdev->cmd(PDC_LINETO,this,param) || !hd )
		return;
	}
	map( x, y, &x, &y );
    }
    if ( cpen.style() != NoPen )
	XDrawLine( dpy, hd, gc, curPt.x(), curPt.y(), x, y );
    curPt = QPoint( x, y );
}

/*!
  Draws a line from \e (x1,y2) to \e (x2,y2). Both endpoints are drawn.
  \sa moveTo(), lineTo()
*/

void QPainter::drawLine( int x1, int y1, int x2, int y2 )
{
    if ( !isActive() )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[2];
	    QPoint p1(x1, y1), p2(x2, y2);
	    param[0].point = &p1;
	    param[1].point = &p2;
	    if ( !pdev->cmd(PDC_DRAWLINE,this,param) || !hd )
		return;
	}
	map( x1, y1, &x1, &y1 );
	map( x2, y2, &x2, &y2 );
    }
    if ( cpen.style() != NoPen )
	XDrawLine( dpy, hd, gc, x1, y1, x2, y2 );
    curPt = QPoint( x2, y2 );
}


static void fix_neg_rect( int *x, int *y, int *w, int *h )
{
    if ( *w < 0 ) {
	*w = -*w;
	*x -= *w - 1;
    }
    if ( *h < 0 ) {
	*h = -*h;
	*y -= *h - 1;
    }
}


/*!
  Draws a rectangle with upper left corner at \e (x,y) and with
  width \e w and height \e h.

  \sa drawRoundRect()
*/

void QPainter::drawRect( int x, int y, int w, int h )
{
    if ( !isActive() )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[1];
	    QRect r( x, y, w, h );
	    param[0].rect = &r;
	    if ( !pdev->cmd(PDC_DRAWRECT,this,param) || !hd )
		return;
	}
	if ( txop == TxRotShear ) {		// rotate/shear polygon
	    QPointArray a( QRect(x,y,w,h), TRUE );
	    drawPolyInternal( xForm(a) );
	    return;
	}
	map( x, y, w, h, &x, &y, &w, &h );
    }
    if ( w <= 0 || h <= 0 ) {
	if ( w == 0 || h == 0 )
	    return;
	fix_neg_rect( &x, &y, &w, &h );
    }
    if ( cbrush.style() != NoBrush ) {
	if ( cpen.style() == NoPen ) {
	    XFillRectangle( dpy, hd, gc_brush, x, y, w, h );
	    return;
	}
	if ( w > 2 && h > 2 )
	    XFillRectangle( dpy, hd, gc_brush, x+1, y+1, w-2, h-2 );
    }
    if ( cpen.style() != NoPen )
	XDrawRectangle( dpy, hd, gc, x, y, w-1, h-1 );
}

/*!
  Draws a Windows focus rectangle with upper left corner at \e (x,y) and with
  width \e w and height \e h.

  This function draws a stippled XOR rectangle that is used to indicate
  keyboard focus (when the \link QApplication::style() GUI style\endlink
  is \c WindowStyle).

  \warning This function draws nothing if the coordinate system has been
  \link rotate() rotated\endlink or \link shear() sheared\endlink.

  \sa drawRect(), QApplication::style()
*/

void QPainter::drawWinFocusRect( int x, int y, int w, int h )
{
    if ( !isActive() || txop == TxRotShear )
	return;
    static char winfocous_line[] = { 1, 1 };

    QPen     old_pen = cpen;
    RasterOp old_rop = (RasterOp)rop;
    setPen( white );
    setRasterOp( XorROP );

    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[1];
	    QRect r( x, y, w, h );
	    param[0].rect = &r;
	    if ( !pdev->cmd(PDC_DRAWRECT,this,param) || !hd )
		return;
	}
	map( x, y, w, h, &x, &y, &w, &h );
    }
    if ( w <= 0 || h <= 0 ) {
	if ( w == 0 || h == 0 )
	    return;
	fix_neg_rect( &x, &y, &w, &h );
    }
    XSetDashes( dpy, gc, 0, winfocous_line, 2 );
    XSetLineAttributes( dpy, gc, 0, LineOnOffDash, CapButt, JoinMiter );

    XDrawRectangle( dpy, hd, gc, x, y, w-1, h-1 );

    setRasterOp( old_rop );
    setPen( old_pen );
}

/*!
  Draws a rectangle with round corners at \e (x,y), with width \e w
  and height \e h.

  The \e xRnd and \e yRnd arguments specify how rounded the corners
  should be.  0 is angled corners, 99 is maximum roundedness.

  The width and height include both lines.

  \sa drawRect()
*/

void QPainter::drawRoundRect( int x, int y, int w, int h, int xRnd, int yRnd )
{
    if ( !isActive() )
	return;
    if ( xRnd <= 0 || yRnd <= 0 ) {
	drawRect( x, y, w, h );			// draw normal rectangle
	return;
    }
    if ( xRnd >= 100 )				// fix ranges
	xRnd = 99;
    if ( yRnd >= 100 )
	yRnd = 99;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[3];
	    QRect r( x, y, w, h );
	    param[0].rect = &r;
	    param[1].ival = xRnd;
	    param[2].ival = yRnd;
	    if ( !pdev->cmd(PDC_DRAWROUNDRECT,this,param) || !hd )
		return;
	}
	if ( txop == TxRotShear ) {		// rotate/shear polygon
	    QPointArray a;
	    if ( w <= 0 || h <= 0 )
		fix_neg_rect( &x, &y, &w, &h );
	    w--;
	    h--;
	    int rxx = w*xRnd/200;
	    int ryy = h*yRnd/200;
	    int rxx2 = 2*rxx;
	    int ryy2 = 2*ryy;
	    int xx, yy;
	    a.makeEllipse( x, y, rxx2, ryy2 );
	    int s = a.size()/4;
	    int i = 0;
	    while ( i < s ) {
		a.point( i, &xx, &yy );
		xx += w - rxx2;
		a.setPoint( i++, xx, yy );
	    }
	    i = 2*s;
	    while ( i < 3*s ) {
		a.point( i, &xx, &yy );
		yy += h - ryy2;
		a.setPoint( i++, xx, yy );
	    }
	    while ( i < 4*s ) {
		a.point( i, &xx, &yy );
		xx += w - rxx2;
		yy += h - ryy2;
		a.setPoint( i++, xx, yy );
	    }
	    drawPolyInternal( xForm(a) );
	    return;
	}
	map( x, y, w, h, &x, &y, &w, &h );
    }
    w--;
    h--;
    if ( w <= 0 || h <= 0 ) {
	if ( w == 0 || h == 0 )
	    return;
	fix_neg_rect( &x, &y, &w, &h );
    }
    int rx = (w*xRnd)/200;
    int ry = (h*yRnd)/200;
    int rx2 = 2*rx;
    int ry2 = 2*ry;
    if ( cbrush.style() != NoBrush ) {		// draw filled round rect
	int dp, ds;
	if ( cpen.style() == NoPen ) {
	    dp = 0;
	    ds = 1;
	}
	else {
	    dp = 1;
	    ds = 0;
	}
#define SET_ARC(px,py,w,h,a1,a2) \
    a->x=px; a->y=py; a->width=w; a->height=h; a->angle1=a1; a->angle2=a2; a++
	XArc arcs[4];
	XArc *a = arcs;
	SET_ARC( x+w-rx2, y, rx2, ry2, 0, 90*64 );
	SET_ARC( x, y, rx2, ry2, 90*64, 90*64 );
	SET_ARC( x, y+h-ry2, rx2, ry2, 180*64, 90*64 );
	SET_ARC( x+w-rx2, y+h-ry2, rx2, ry2, 270*64, 90*64 );
	XFillArcs( dpy, hd, gc_brush, arcs, 4 );
#undef SET_ARC
#define SET_RCT(px,py,w,h) \
    r->x=px; r->y=py; r->width=w; r->height=h; r++
	XRectangle rects[3];
	XRectangle *r = rects;
	SET_RCT( x+rx, y+dp, w-rx2, ry );
	SET_RCT( x+dp, y+ry, w+ds, h-ry2 );
	SET_RCT( x+rx, y+h-ry, w-rx2, ry+ds );
	XFillRectangles( dpy, hd, gc_brush, rects, 3 );
#undef SET_RCT
    }
    if ( cpen.style() != NoPen ) {		// draw outline
#define SET_ARC(px,py,w,h,a1,a2) \
    a->x=px; a->y=py; a->width=w; a->height=h; a->angle1=a1; a->angle2=a2; a++
	XArc arcs[4];
	XArc *a = arcs;
	SET_ARC( x+w-rx2, y, rx2, ry2, 0, 90*64 );
	SET_ARC( x, y, rx2, ry2, 90*64, 90*64 );
	SET_ARC( x, y+h-ry2, rx2, ry2, 180*64, 90*64 );
	SET_ARC( x+w-rx2, y+h-ry2, rx2, ry2, 270*64, 90*64 );
	XDrawArcs( dpy, hd, gc, arcs, 4 );
#undef SET_ARC
#define SET_SEG(xp1,yp1,xp2,yp2) \
    s->x1=xp1; s->y1=yp1; s->x2=xp2; s->y2=yp2; s++
	XSegment segs[4];
	XSegment *s = segs;
	SET_SEG( x+rx, y, x+w-rx, y );
	SET_SEG( x+rx, y+h, x+w-rx, y+h );
	SET_SEG( x, y+ry, x, y+h-ry );
	SET_SEG( x+w, y+ry, x+w, y+h-ry );
	XDrawSegments( dpy, hd, gc, segs, 4 );
#undef SET_SET
    }
}

/*!
  Draws an ellipse with center at \e (x+w/2,y+h/2) and size \e (w,h).
*/

void QPainter::drawEllipse( int x, int y, int w, int h )
{
    if ( !isActive() )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[1];
	    QRect r( x, y, w, h );
	    param[0].rect = &r;
	    if ( !pdev->cmd(PDC_DRAWELLIPSE,this,param) || !hd )
		return;
	}
	if ( txop == TxRotShear ) {		// rotate/shear polygon
	    QPointArray a;
	    a.makeEllipse( x, y, w, h );
	    drawPolyInternal( xForm(a) );
	    return;
	}
	map( x, y, w, h, &x, &y, &w, &h );
    }
    w--;
    h--;
    if ( w <= 0 || h <= 0 ) {
	if ( w == 0 || h == 0 )
	    return;
	fix_neg_rect( &x, &y, &w, &h );
    }
    if ( cbrush.style() != NoBrush ) {		// draw filled ellipse
	XFillArc( dpy, hd, gc_brush, x, y, w, h, 0, 360*64 );
	if ( cpen.style() == NoPen ) {
	    XDrawArc( dpy, hd, gc_brush, x, y, w, h, 0, 360*64 );
	    return;
	}
    }
    if ( cpen.style() != NoPen )		// draw outline
	XDrawArc( dpy, hd, gc, x, y, w, h, 0, 360*64 );
}


/*!
  Draws an arc defined by the rectangle \e (x,y,w,h), the start
  angle \e a and the arc length \e alen.

  The angles \e a and \e alen are 1/16th of a degree, i.e. a full
  circle equals 5760 (16*360). Positive values of \e a and \e alen mean
  counter-clockwise while negative values mean clockwise direction.
  Zero degrees is at the 3'o clock position.

  Example:
  \code
    QPainter p;
    p.begin( myWidget );
    p.drawArc( 10,10, 70,100, 100*16, 160*16 ); // draws a "(" arc
    p.end();
  \endcode

  \sa drawPie(), drawChord()
*/

void QPainter::drawArc( int x, int y, int w, int h, int a, int alen )
{
    if ( !isActive() )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[3];
	    QRect r( x, y, w, h );
	    param[0].rect = &r;
	    param[1].ival = a;
	    param[2].ival = alen;
	    if ( !pdev->cmd(PDC_DRAWARC,this,param) || !hd )
		return;
	}
	if ( txop == TxRotShear ) {		// rotate/shear
	    QPointArray pa;
	    pa.makeArc( x, y, w, h, a, alen );	// arc polyline
	    drawPolyInternal( xForm(pa), FALSE );
	    return;
	}
	map( x, y, w, h, &x, &y, &w, &h );
    }
    w--;
    h--;
    if ( w <= 0 || h <= 0 ) {
	if ( w == 0 || h == 0 )
	    return;
	fix_neg_rect( &x, &y, &w, &h );
    }
    if ( cpen.style() != NoPen )
	XDrawArc( dpy, hd, gc, x, y, w, h, a*4, alen*4 );
}


/*!
  Draws a pie defined by the rectangle \e (x,y,w,h), the start
  angle \e a and the arc length \e alen.

  The pie is filled with the current \link setBrush() brush\endlink.

  The angles \e a and \e alen are 1/16th of a degree, i.e. a full
  circle equals 5760 (16*360). Positive values of \e a and \e alen mean
  counter-clockwise while negative values mean clockwise direction.
  Zero degrees is at the 3'o clock position.

  \sa drawArc(), drawPie()
*/

void QPainter::drawPie( int x, int y, int w, int h, int a, int alen )
{
    // Make sure "a" is 0..360*16, as otherwise a*4 may overflow 16 bits.
    if ( a > (360*16) ) {
	a = a % (360*16);
    } else if ( a < 0 ) {
	a = a % (360*16);
	if ( a < 0 ) a += (360*16);
    }

    if ( !isActive() )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[3];
	    QRect r( x, y, w, h );
	    param[0].rect = &r;
	    param[1].ival = a;
	    param[2].ival = alen;
	    if ( !pdev->cmd(PDC_DRAWPIE,this,param) || !hd )
		return;
	}
	if ( txop == TxRotShear ) {		// rotate/shear
	    QPointArray pa;
	    pa.makeArc( x, y, w, h, a, alen );	// arc polyline
	    int n = pa.size();
	    pa.resize( n+2 );
	    pa.setPoint( n, x+w/2, y+h/2 );	// add legs
	    pa.setPoint( n+1, pa.at(0) );
	    drawPolyInternal( xForm(pa) );
	    return;
	}
	map( x, y, w, h, &x, &y, &w, &h );
    }
    XSetArcMode( dpy, gc_brush, ArcPieSlice );
    w--;
    h--;
    if ( w <= 0 || h <= 0 ) {
	if ( w == 0 || h == 0 )
	    return;
	fix_neg_rect( &x, &y, &w, &h );
    }

    GC g = gc;
    bool nopen = cpen.style() == NoPen;

    if ( cbrush.style() != NoBrush ) {		// draw filled pie
	XFillArc( dpy, hd, gc_brush, x, y, w, h, a*4, alen*4 );
	if ( nopen ) {
	    g = gc_brush;
	    nopen = FALSE;
	}
    }
    if ( !nopen ) {				// draw pie outline
	double w2 = 0.5*w;			// with, height in ellipsis
	double h2 = 0.5*h;
	double xc = (double)x+w2;
	double yc = (double)y+h2;
	double ra1 = Q_PI/2880.0*a;		// convert a,alen to radians
	double ra2 = ra1 + Q_PI/2880.0*alen;
	int xic = qRound(xc);
	int yic = qRound(yc);
	XDrawLine( dpy, hd, g, xic, yic,
		   qRound(xc + qcos(ra1)*w2), qRound(yc - qsin(ra1)*h2));
	XDrawLine( dpy, hd, g, xic, yic,
		   qRound(xc + qcos(ra2)*w2), qRound(yc - qsin(ra2)*h2));
	XDrawArc( dpy, hd, g, x, y, w, h, a*4, alen*4 );
    }
}


/*!
  Draws a chord defined by the rectangle \e (x,y,w,h), the start
  angle \e a and the arc length \e alen.

  The chord is filled with the current \link setBrush() brush\endlink.

  The angles \e a and \e alen are 1/16th of a degree, i.e. a full
  circle equals 5760 (16*360). Positive values of \e a and \e alen mean
  counter-clockwise while negative values mean clockwise direction.
  Zero degrees is at the 3'o clock position.

  \sa drawArc(), drawPie()
*/

void QPainter::drawChord( int x, int y, int w, int h, int a, int alen )
{
    if ( !isActive() )
	return;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[3];
	    QRect r( x, y, w, h );
	    param[0].rect = &r;
	    param[1].ival = a;
	    param[2].ival = alen;
	    if ( !pdev->cmd(PDC_DRAWCHORD,this,param) || !hd )
		return;
	}
	if ( txop == TxRotShear ) {		// rotate/shear
	    QPointArray pa;
	    pa.makeArc( x, y, w-1, h-1, a, alen ); // arc polygon
	    int n = pa.size();
	    pa.resize( n+1 );
	    pa.setPoint( n, pa.at(0) );		// connect endpoints
	    drawPolyInternal( xForm(pa) );
	    return;
	}
	map( x, y, w, h, &x, &y, &w, &h );
    }
    XSetArcMode( dpy, gc_brush, ArcChord );
    w--;
    h--;
    if ( w <= 0 || h <= 0 ) {
	if ( w == 0 || h == 0 )
	    return;
	fix_neg_rect( &x, &y, &w, &h );
    }

    GC g = gc;
    bool nopen = cpen.style() == NoPen;

    if ( cbrush.style() != NoBrush ) {		// draw filled chord
	XFillArc( dpy, hd, gc_brush, x, y, w, h, a*4, alen*4 );
	if ( nopen ) {
	    g = gc_brush;
	    nopen = FALSE;
	}
    }
    if ( !nopen ) {				// draw chord outline
	double w2 = 0.5*w;			// with, height in ellipsis
	double h2 = 0.5*h;
	double xc = (double)x+w2;
	double yc = (double)y+h2;
	double ra1 = Q_PI/2880.0*a;		// convert a,alen to radians
	double ra2 = ra1 + Q_PI/2880.0*alen;
	XDrawLine( dpy, hd, g,
		   qRound(xc + qcos(ra1)*w2), qRound(yc - qsin(ra1)*h2),
		   qRound(xc + qcos(ra2)*w2), qRound(yc - qsin(ra2)*h2));
	XDrawArc( dpy, hd, g, x, y, w, h, a*4, alen*4 );
    }
    XSetArcMode( dpy, gc_brush, ArcPieSlice );
}


/*!
  Draws \e nlines separate lines from points defined in \e a, starting at
  a[\e index]. If \e nlines is -1 all points until the end of the array
  are used (i.e. (a.size()-index)/2 lines are drawn).

  Draws the 1st line from \e a[index] to \e a[index+1].
  Draws the 2nd line from \e a[index+2] to \e a[index+3] etc.

  \sa drawPolyline(), drawPolygon()
*/

void QPainter::drawLineSegments( const QPointArray &a, int index, int nlines )
{
    if ( nlines < 0 )
	nlines = a.size()/2 - index/2;
    if ( index + nlines*2 > (int)a.size() )
	nlines = (a.size() - index)/2;
    if ( !isActive() || nlines < 1 || index < 0 )
	return;
    QPointArray pa = a;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    if ( nlines != (int)pa.size()/2 ) {
		pa.resize( nlines*2 );
		for ( int i=0; i<nlines*2; i++ )
		    pa.setPoint( i, a.point(index+i) );
		index = 0;
	    }
	    QPDevCmdParam param[1];
	    param[0].ptarr = (QPointArray*)&pa;
	    if ( !pdev->cmd(PDC_DRAWLINESEGS,this,param) || !hd )
		return;
	}
	if ( txop != TxNone && cpen.style() != NoPen )
	    pa = xForm( a );
    }
    if ( cpen.style() != NoPen )
	XDrawSegments( dpy, hd, gc, (XSegment*)(pa.data()+index), nlines );
}


/*!
  Draws the polyline defined by the \e npoints points in \e a starting
  at \e a[index].

  If \e npoints is -1 all points until the end of the array are used
  (i.e. a.size()-index-1 line segments are drawn).

  \sa drawLineSegments(), drawPolygon()
*/

void QPainter::drawPolyline( const QPointArray &a, int index, int npoints )
{
    if ( npoints < 0 )
	npoints = a.size() - index;
    if ( index + npoints > (int)a.size() )
	npoints = a.size() - index;
    if ( !isActive() || npoints < 2 || index < 0 )
	return;
    QPointArray pa = a;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    if ( npoints != (int)pa.size() ) {
		pa.resize( npoints );
		for ( int i=0; i<npoints; i++ )
		    pa.setPoint( i, a.point(index+i) );
		index = 0;
	    }
	    QPDevCmdParam param[1];
	    param[0].ptarr = (QPointArray*)&pa;
	    // ### ??? We do this even if NoPen?
	    if ( !pdev->cmd(PDC_DRAWPOLYLINE,this,param) || !hd )
		return;
	}
	if ( txop != TxNone && cpen.style() != NoPen )
	    pa = xForm( a );
    }
    if ( cpen.style() != NoPen )
	XDrawLines( dpy, hd, gc, (XPoint*)(pa.data()+index), npoints,
		    CoordModeOrigin );
}


/*!
  Draws the polygon defined by the \e npoints points in \e a starting at
  \e a[index].

  If \e npoints is -1 all points until the end of the array are used
  (i.e. a.size()-index line segments define the polygon).

  The first point is always connected to the last point.

  The polygon is filled with the current \link setBrush() brush\endlink.
  If \e winding is TRUE, the polygon is filled using the winding
  fill algorithm. If \e winding is FALSE, the polygon is filled using the
  even-odd (alternative) fill algorithm.

  \sa drawLineSegments(), drawPolygon()
*/

void QPainter::drawPolygon( const QPointArray &a, bool winding,
			    int index, int npoints )
{
    if ( npoints < 0 )
	npoints = a.size() - index;
    if ( index + npoints > (int)a.size() )
	npoints = a.size() - index;
    if ( !isActive() || npoints < 2 || index < 0 )
	return;
    QPointArray pa = a;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    if ( npoints != (int)a.size() ) {
		pa.resize( npoints );
		for ( int i=0; i<npoints; i++ )
		    pa.setPoint( i, a.point(index+i) );
		index = 0;
	    }
	    QPDevCmdParam param[2];
	    param[0].ptarr = (QPointArray*)&pa;
	    param[1].ival = winding;
	    if ( !pdev->cmd(PDC_DRAWPOLYGON,this,param) || !hd )
		return;
	}
	if ( txop != TxNone )
	    pa = xForm( a );
    }
    if ( winding )				// set to winding fill rule
	XSetFillRule( dpy, gc_brush, WindingRule );

    int x1, y1, x2, y2;				// connect last to first point
    pa.point( index+npoints-1, &x1, &y1 );
    pa.point( index, &x2, &y2 );
    bool closed = x1 == x2 && y1 == y2;

    if ( cbrush.style() != NoBrush ) {		// draw filled polygon
	XFillPolygon( dpy, hd, gc_brush, (XPoint*)(pa.data()+index),
		      npoints, Complex, CoordModeOrigin );
	if ( cpen.style() == NoPen ) {		// draw fake outline
	    XDrawLines( dpy, hd, gc_brush, (XPoint*)(pa.data()+index),
			npoints, CoordModeOrigin );
	    if ( !closed )
		XDrawLine( dpy, hd, gc_brush, x1, y1, x2, y2 );
	}
    }
    if ( cpen.style() != NoPen ) {		// draw outline
	XDrawLines( dpy, hd, gc, (XPoint*)(pa.data()+index), npoints,
		    CoordModeOrigin );
	if ( !closed )
	    XDrawLine( dpy, hd, gc, x1, y1, x2, y2 );
    }
    if ( winding )				// set to normal fill rule
	XSetFillRule( dpy, gc_brush, EvenOddRule );
}


/*!
  Draws a cubic Bezier curve defined by the control points in \e a,
  starting at \e a[index].

  \e a must have 4 points or more.  The control point \e a[index+4] and
  beyond are ignored.
*/

void QPainter::drawQuadBezier( const QPointArray &a, int index )
{
    if ( !isActive() )
	return;
    if ( a.size() - index < 4 ) {
#if defined(CHECK_RANGE)
	warning( "QPainter::drawQuadBezier: Cubic Bezier needs 4 control "
		 "points" );
#endif
	return;
    }
    QPointArray pa( a );
    if ( index != 0 || a.size() > 4 ) {
	pa = QPointArray( 4 );
	for ( int i=0; i<4; i++ )
	    pa.setPoint( i, a.point(index+i) );
    }
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[1];
	    param[0].ptarr = (QPointArray*)&pa;
	    if ( !pdev->cmd(PDC_DRAWQUADBEZIER,this,param) || !hd )
		return;
	}
	if ( txop != TxNone )
	    pa = xForm( pa );
    }
    if ( cpen.style() != NoPen ) {
	pa = pa.quadBezier();
	XDrawLines( dpy, hd, gc, (XPoint*)pa.data(), pa.size(),
		    CoordModeOrigin);
    }
}


/*!
  Draws a pixmap at \e (x,y) by copying a part of the pixmap into the
  paint device.

  \arg \e (x,y) specify the point in the paint device.
  \arg \e (sx,sy) specify an offset in the pixmap.
  \arg \e (sw,sh) specify the area of the area of the pixmap to
  be copied.  The value -1 means to the right/bottom of the
  pixmap.

  The pixmap is clipped if a \link QPixmap::setMask() mask\endlink has
  been set.

  \sa bitBlt(), QPixmap::setMask()
*/

void QPainter::drawPixmap( int x, int y, const QPixmap &pixmap,
			   int sx, int sy, int sw, int sh )
{
    if ( !isActive() || pixmap.isNull() )
	return;
    if ( sw < 0 )
	sw = pixmap.width()  - sx;
    if ( sh < 0 )
	sh = pixmap.height() - sy;
    if ( testf(ExtDev|VxF|WxF) ) {
	if ( testf(ExtDev) || txop == TxScale || txop == TxRotShear ) {
	    if ( sx != 0 || sy != 0 ||
		 sw != pixmap.width() || sh != pixmap.height() ) {
		QPixmap tmp( sw, sh, pixmap.depth() );
		bitBlt( &tmp, 0, 0, &pixmap, sx, sy, sw, sh, CopyROP, TRUE );
		if ( pixmap.mask() ) {
		    QBitmap mask( sw, sh );
		    bitBlt( &mask, 0, 0, pixmap.mask(), sx, sy, sw, sh,
			    CopyROP, TRUE );
		    tmp.setMask( mask );
		}
		drawPixmap( x, y, tmp );
		return;
	    }
	    if ( testf(ExtDev) ) {
		QPDevCmdParam param[2];
		QPoint p(x,y);
		param[0].point	= &p;
		param[1].pixmap = &pixmap;
		if ( !pdev->cmd(PDC_DRAWPIXMAP,this,param) || !hd )
		    return;
	    }
	    if ( txop == TxScale || txop == TxRotShear ) {
		QWMatrix mat( wm11/65536.0, wm12/65536.0,
			      wm21/65536.0, wm22/65536.0,
			      wdx/65536.0,  wdy/65536.0 );
		mat = QPixmap::trueMatrix( mat, sw, sh );
		QPixmap pm = pixmap.xForm( mat );
		if ( !pm.mask() && txop == TxRotShear ) {
		    QBitmap bm_clip( sw, sh, 1 );
		    bm_clip.fill( color1 );
		    pm.setMask( bm_clip.xForm(mat) );
		}
		map( x, y, &x, &y );		// compute position of pixmap
		int dx, dy;
		mat.map( 0, 0, &dx, &dy );
		x -= dx;  y -= dy;
		ushort save_flags = flags;
		flags = IsActive | (save_flags & ClipOn);
		drawPixmap( x, y, pm );
		flags = save_flags;
		return;
	    }
	}
	map( x, y, &x, &y );
    }

    QBitmap *mask = (QBitmap *)pixmap.mask();
    bool mono = pixmap.depth() == 1;

    if ( mask && !hasClipping() ) {
	if ( mono ) {				// needs GCs pen color
	    bool selfmask = pixmap.data->selfmask;
	    if ( selfmask ) {
		XSetFillStyle( dpy, gc, FillStippled );
		XSetStipple( dpy, gc, pixmap.handle() );
	    } else {
		XSetFillStyle( dpy, gc, FillOpaqueStippled );
		XSetStipple( dpy, gc, pixmap.handle() );
		XSetClipMask( dpy, gc, mask->handle() );
		XSetClipOrigin( dpy, gc, x-sx, y-sy );
	    }
	    XSetTSOrigin( dpy, gc, x-sx, y-sy );
	    XFillRectangle( dpy, hd, gc, x, y, sw, sh );
	    XSetTSOrigin( dpy, gc, 0, 0 );
	    XSetFillStyle( dpy, gc, FillSolid );
	    if ( !selfmask ) {
		XSetClipOrigin( dpy, gc, 0, 0 );
		XSetClipMask( dpy, gc, None );
	    }
	} else {
	    bitBlt( pdev, x, y, &pixmap, sx, sy, sw, sh, (RasterOp)rop );
	}
	return;
    }

    if ( mask ) {				// pixmap has clip mask
	// Implies that clipping is on
	// Create a new mask that combines the mask with the clip region

	QBitmap *comb = new QBitmap( sw, sh );
	comb->detach();
	GC cgc = qt_xget_temp_gc( TRUE );	// get temporary mono GC
	XSetForeground( dpy, cgc, 0 );
	XFillRectangle( dpy, comb->handle(), cgc, 0, 0, sw, sh );
	XSetBackground( dpy, cgc, 0 );
	XSetForeground( dpy, cgc, 1 );
	XSetRegion( dpy, cgc, crgn.handle() );
	XSetClipOrigin( dpy, cgc, -x, -y );
	XSetFillStyle( dpy, cgc, FillOpaqueStippled );
	XSetStipple( dpy, cgc, mask->handle() );
	XSetTSOrigin( dpy, cgc, -sx, -sy );
	XFillRectangle( dpy, comb->handle(), cgc, 0, 0, sw, sh );
	XSetTSOrigin( dpy, cgc, 0, 0 );		// restore cgc
	XSetFillStyle( dpy, cgc, FillSolid );
	XSetClipOrigin( dpy, cgc, 0, 0 );
	XSetClipMask( dpy, cgc, None );
	mask = comb;				// it's deleted below

	XSetClipMask( dpy, gc, mask->handle() );
	XSetClipOrigin( dpy, gc, x, y );
    }

    if ( mono ) {
	XSetBackground( dpy, gc, bg_col.pixel() );
	XSetFillStyle( dpy, gc, FillOpaqueStippled );
	XSetStipple( dpy, gc, pixmap.handle() );
	XSetTSOrigin( dpy, gc, x-sx, y-sy );
	XFillRectangle( dpy, hd, gc, x, y, sw, sh );
	XSetTSOrigin( dpy, gc, 0, 0 );
	XSetFillStyle( dpy, gc, FillSolid );
    } else {
	XCopyArea( dpy, pixmap.handle(), hd, gc, sx, sy, sw, sh, x, y );
    }

    if ( mask ) {				// restore clipping
	XSetClipOrigin( dpy, gc, 0, 0 );
	XSetRegion( dpy, gc, crgn.handle() );
	delete mask;				// delete comb, created above
    }
}


//
// Generate a string that describes a transformed bitmap. This string is used
// to insert and find bitmaps in the global pixmap cache.
//
static QString gen_xbm_key( const QWMatrix &m, const QFontInfo &fi,
			    const char *str, int len )
{
    QString s = str;
    s.truncate( len );
    QString k;
    QString fd;
    if ( fi.rawMode() )
	fd.sprintf( "&%s", fi.family() );
    else
	fd.sprintf( "x%s_%i_%i_%i_%i_%i_%i_%i",
		    fi.family(), fi.pointSize(), fi.italic(), fi.weight(),
		    fi.underline(), fi.strikeOut(), fi.fixedPitch(),
		    fi.charSet() );
    k.resize( len + 100 + fd.length() );
    k.sprintf( "$qt$%s,%g,%g,%g,%g,%g,%g,%s", (const char *)s,
	       m.m11(), m.m12(), m.m21(),m.m22(), m.dx(), m.dy(),
	       (const char *)fd );
    return k;
}


static QPixmap *get_text_bitmap( const QWMatrix &m, const QFontInfo &fi,
				 const char *str, int len )
{
    QString k = gen_xbm_key( m, fi, str, len );
    return QPixmapCache::find( k );
}


static void ins_text_bitmap( const QWMatrix &m, const QFontInfo &fi,
			     const char *str, int len, QPixmap *pm )
{
    QString k = gen_xbm_key( m, fi, str, len );
    if ( !QPixmapCache::insert(k,pm) )		// cannot insert pixmap
	delete pm;
}

/*!
  Draws at most \e len characters from \e str at position \e (x,y).

  \e (x,y) is the base line position.  Note that the meaning of \a y
  is not the same for the two drawText() varieties.
*/

void QPainter::drawText( int x, int y, const char *str, int len )
{
    if ( !isActive() )
	return;
    if ( len < 0 )
	len = strlen( str );
    if ( len == 0 )				// empty string
	return;

    if ( testf(DirtyFont|ExtDev|VxF|WxF) ) {
	if ( testf(DirtyFont) )
	    updateFont();
	if ( testf(ExtDev) ) {
	    QPDevCmdParam param[2];
	    QPoint p( x, y );
	    QString newstr = str;
	    newstr.truncate( len );
	    param[0].point = &p;
	    param[1].str = newstr.data();
	    if ( !pdev->cmd(PDC_DRAWTEXT,this,param) || !hd )
		return;
	}
	if ( txop == TxScale || txop == TxRotShear ) {
	    QFontMetrics fm = fontMetrics();
	    QFontInfo	 fi = fontInfo();
	    QRect bbox = fm.boundingRect( str, len );
	    int w=bbox.width(), h=bbox.height();
	    int tx=-bbox.x(),  ty=-bbox.y();	// text position
	    bool empty = w == 0 || h == 0;
	    QWMatrix mat1( wm11/65536.0, wm12/65536.0,
			   wm21/65536.0, wm22/65536.0,
			   wdx /65536.0, wdy /65536.0 );
	    QWMatrix mat = QPixmap::trueMatrix( mat1, w, h );
	    QPixmap *wx_bm = get_text_bitmap( mat, fi, str, len );
	    bool create_new_bm = wx_bm == 0;
	    if ( create_new_bm && !empty ) {	// no such cached bitmap
		QPixmap bm( w, h, 1 );		// create bitmap
		bm.fill( color0 );
		QPainter paint;
		paint.begin( &bm );		// draw text in bitmap
		paint.setFont( cfont );
		paint.drawText( tx, ty, str, len );
		paint.end();
		wx_bm = new QPixmap( bm.xForm( mat ) ); // transform bitmap
		if ( wx_bm->isNull() ) {
		    delete wx_bm;		// nothing to draw
		    return;
		}
	    }

	    if ( bg_mode == OpaqueMode ) {	// opaque fill
		extern XFontStruct *qt_get_xfontstruct( QFontData * );
		XFontStruct *fs = qt_get_xfontstruct( cfont.d );
		int direction;
		int ascent;
		int descent;
		XCharStruct overall;
		XTextExtents( fs, str, len, &direction, &ascent, &descent,
			      &overall );
		int fx = x;
		int fy = y - ascent;
		int fw = overall.width;
		int fh = ascent + descent;
		int m, n;
		QPointArray a(5);
		mat1.map( fx,	 fy,	&m, &n );  a.setPoint( 0, m, n );
						   a.setPoint( 4, m, n );
		mat1.map( fx+fw, fy,	&m, &n );  a.setPoint( 1, m, n );
		mat1.map( fx+fw, fy+fh, &m, &n );  a.setPoint( 2, m, n );
		mat1.map( fx,	 fy+fh, &m, &n );  a.setPoint( 3, m, n );
		QBrush oldBrush = cbrush;
		setBrush( backgroundColor() );
		updateBrush();
		XFillPolygon( dpy, hd, gc_brush, (XPoint*)a.data(), 4,
			      Nonconvex, CoordModeOrigin );
		XDrawLines( dpy, hd, gc_brush, (XPoint*)a.data(), 5,
			    CoordModeOrigin );
		setBrush( oldBrush );
	    }
	    if ( empty )
		return;
	    float fx=x, fy=y, nfx, nfy;
	    mat1.map( fx,fy, &nfx,&nfy );
	    float tfx=tx, tfy=ty, dx, dy;
	    mat.map( tfx, tfy, &dx, &dy );	// compute position of bitmap
	    x = qRound(nfx-dx);
	    y = qRound(nfy-dy);
	    XSetFillStyle( dpy, gc, FillStippled );
	    XSetStipple( dpy, gc, wx_bm->handle() );
	    XSetTSOrigin( dpy, gc, x, y );
	    XFillRectangle( dpy, hd, gc, x, y,wx_bm->width(),wx_bm->height() );
	    XSetTSOrigin( dpy, gc, 0, 0 );
	    XSetFillStyle( dpy, gc, FillSolid );
	    if ( create_new_bm )
		ins_text_bitmap( mat, fi, str, len, wx_bm );
	    return;
	}
	if ( txop == TxTranslate )
	    map( x, y, &x, &y );
    }

    if ( cfont.underline() || cfont.strikeOut() ) {
	QFontMetrics fm = fontMetrics();
	int lw = fm.lineWidth();
	int tw = fm.width( str, len );
	if ( cfont.underline() )		// draw underline effect
	    XFillRectangle( dpy, hd, gc, x, y+fm.underlinePos(),
			    tw, lw );
	if ( cfont.strikeOut() )		// draw strikeout effect
	    XFillRectangle( dpy, hd, gc, x, y-fm.strikeOutPos(),
			    tw, lw );
    }
    if ( bg_mode == TransparentMode )
	XDrawString( dpy, hd, gc, x, y, str, len );
    else
	XDrawImageString( dpy, hd, gc, x, y, str, len );
}


//
// The drawText function takes two special parameters; 'internal' and 'brect'.
//
// The 'internal' parameter contains a pointer to an array of encoded
// information that keeps internal geometry data.
// If the drawText function is called repeatedly to display the same text,
// it makes sense to calculate text width and linebreaks the first time,
// and use these parameters later to print the text because we save a lot of
// CPU time.
// The 'internal' parameter will not be used if it is a null pointer.
// The 'internal' parameter will be generated if it is not null, but points
// to a null pointer, i.e. internal != 0 && *internal == 0.
// The 'internal' parameter will be used if it contains a non-null pointer.
//
// If the 'brect parameter is a non-null pointer, then the bounding rectangle
// of the text will be returned in 'brect'.
//

/*!
  Draws at most \e len characters from \e str in the rectangle \e (x,y,w,h).

  Note that the meaning of \a y is not the same for the two drawText()
  varieties.

  This function draws formatted text.  The \e tf text formatting is
  the bitwise OR of the following flags:  <ul>
  <li> \c AlignLeft aligns to the left border.
  <li> \c AlignRight aligns to the right border.
  <li> \c AlignHCenter aligns horizontally centered.
  <li> \c AlignTop aligns to the top border.
  <li> \c AlignBottom aligns to the bottom border.
  <li> \c AlignVCenter aligns vertically centered
  <li> \c AlignCenter (= \c AlignHCenter | AlignVCenter)
  <li> \c SingleLine ignores newline characters in the text.
  <li> \c DontClip never clips the text to the rectangle.
  <li> \c ExpandTabs expands tabulators.
  <li> \c ShowPrefix displays "&x" as "x" underlined.
  <li> \c WordBreak breaks the text to fit the rectangle.
  <li> \c GrayText grays out the text.
  </ul>

  Horizontal alignment defaults to AlignLeft and vertical alignment
  defaults to AlignTop.

  If several of the horizontal or several of the vertical alignment flags   
  are set, the resulting alignment is undefined.

  If ExpandTabs is set and no \link setTabStops() tab stops \endlink or
  \link setTabArray() tab array \endlink have been set tabs will expand to
  the closest reasonable tab stop based on the current font. For \link
  QFont::setFixedPitch() fixed pitch\endlink (fixed width) fonts you are
  guaranteed that each tab stop will be at a multiple of eight of the
  width of the characters in the font.

  \a brect (if non-null) is set to the actual bounding rectangle of
  the output.  \a internal is, yes, internal.

  These flags are defined in qwindefs.h.

  \sa boundingRect()
*/

void QPainter::drawText( int x, int y, int w, int h, int tf,
			 const char *str, int len, QRect *brect,
			 char **internal )
{
    if ( !isActive() )
	return;
    if ( len < 0 )
	len = strlen( str );
    if ( len == 0 )				// empty string
	return;

    if ( testf(DirtyFont|ExtDev) ) {
	if ( testf(DirtyFont) )
	    updateFont();
	if ( testf(ExtDev) && (tf & DontPrint) == 0 ) {
	    QPDevCmdParam param[3];
	    QRect r( x, y, w, h );
	    QString newstr = str;
	    newstr.truncate( len );
	    param[0].rect = &r;
	    param[1].ival = tf;
	    param[2].str = newstr.data();
	    if ( pdev->devType() != PDT_PRINTER ) {
		if ( !pdev->cmd(PDC_DRAWTEXTFRMT,this,param) || !hd )
		    return;			// QPrinter wants PDC_DRAWTEXT
	    }
	}
    }

    if ( w <= 0 || h <= 0 )
	fix_neg_rect( &x, &y, &w, &h );

    QFontMetrics fm = fontMetrics();		// get font metrics

    struct text_info {				// internal text info
	char  tag[4];				// contains "qptr"
	int   w;				// width
	int   h;				// height
	int   tf;				// flags (alignment etc.)
	int   len;				// text length
	int   maxwidth;				// max text width
	int   nlines;				// number of lines
	int   codelen;				// length of encoding
    };

    ushort codearray[200];
    int	   codelen    = 200;
    bool   code_alloc = FALSE;
    ushort *codes     = codearray;
    ushort cc;					// character code
    bool   decode     = internal && *internal;	// decode from internal data
    bool   encode     = internal && !*internal; // build internal data

    if ( len > 150 && !decode ) {		// need to alloc code array
	codelen = len + len/2;
	codes	= (ushort *)malloc( codelen*sizeof(ushort) );
	code_alloc = TRUE;
    }

    const int BEGLINE  = 0x8000;		// encoding 0x8zzz, zzz=width
    const int TABSTOP  = 0x4000;		// encoding 0x4zzz, zzz=tab pos
    const int PREFIX   = 0x2000;		// encoding 0x20zz, zz=char
    const int WIDTHBITS= 0x1fff;		// bits for width encoding
    const int MAXWIDTH = 0x1fff;		// max width value

    char *p = (char *)str;
    int nlines;					// number of lines
    int index;					// index for codes
    int begline;				// index at beginning of line
    int breakindex;				// index where to break
    int breakwidth;				// width of text at breakindex
    int maxwidth;				// maximum width of a line
    int bcwidth;				// width of break char
    int tabindex;				// tab array index
    int cw;					// character width
    int k;					// index for p
    int tw;					// text width
    short charwidth[255];			// character widths
    memset( charwidth, -1, 255*sizeof(short) );

#define CWIDTH(x) (charwidth[x]>=0 ? charwidth[x] : (charwidth[x]=fm.width(x)))
#undef	UCHAR
#define UCHAR(x)  (uchar)(x)

    bool wordbreak  = (tf & WordBreak)	== WordBreak;
    bool expandtabs = (tf & ExpandTabs) == ExpandTabs;
    bool singleline = (tf & SingleLine) == SingleLine;
    bool showprefix = (tf & ShowPrefix) == ShowPrefix;

    int	 spacewidth = CWIDTH( (int)' ' );	// width of space char

    nlines = 0;
    index  = 1;					// first index contains BEGLINE
    begline = breakindex = breakwidth = maxwidth = bcwidth = tabindex = 0;
    k = tw = 0;

    if ( decode )				// skip encoding
	k = len;

    int localTabStops = 0;	       		// tab stops
    if ( tabstops )
	localTabStops = tabstops;
    else
	localTabStops = fm.width('x')*8;       	// default to 8 times x

    while ( k < len ) {				// convert string to codes

	if ( UCHAR(*p) > 32 ) {			// printable character
	    if ( *p == '&' && showprefix ) {
		cc = '&';			// assume ampersand
		if ( k < len-1 ) {
		    k++;
		    p++;
		    if ( *p != '&' && UCHAR(*p) > 32 )
			cc = PREFIX | UCHAR(*p);// use prefix char
		}
	    } else {
		cc = UCHAR(*p);
	    }
	    cw = CWIDTH( cc & 0xff );

	} else {				// not printable (except ' ')

	    if ( *p == 32 ) {			// the space character
		cc = ' ';
		cw = spacewidth;
	    } else if ( *p == '\n' ) {		// newline
		if ( singleline ) {
		    cc = ' ';			// convert newline to space
		    cw = spacewidth;
		} else {
		    cc = BEGLINE;
		    cw = 0;
		}
	    } else if ( *p == '\t' ) {		// TAB character
		if ( expandtabs ) {
		    cw = 0;
		    if ( tabarray ) {		// use tab array
			while ( tabindex < tabarraylen ) {
			    if ( tabarray[tabindex] > tw ) {
				cw = tabarray[tabindex] - tw;
				tabindex++;
				break;
			    }
			    tabindex++;
			}
		    }
		    if ( cw == 0 )		// use fixed tab stops
			cw = localTabStops - tw%localTabStops;
		    cc = TABSTOP | QMIN(tw+cw,MAXWIDTH);
		} else {			// convert TAB to space
		    cc = ' ';
		    cw = spacewidth;
		}
	    } else {				// ignore character
		k++;
		p++;
		continue;
	    }

	    if ( wordbreak ) {			// possible break position
		breakindex = index;
		breakwidth = tw;
		bcwidth = cw;
	    }
	}

	if ( wordbreak && breakindex > 0 && tw+cw > w ) {
	    if ( index == breakindex ) {	// break at current index
		cc = BEGLINE;
		cw = 0;
	    } else {				// break at breakindex
		codes[begline] = BEGLINE | QMIN(breakwidth,MAXWIDTH);
		maxwidth = QMAX(maxwidth,breakwidth);
		begline = breakindex;
		nlines++;
		tw -= breakwidth + bcwidth;
		breakindex = tabindex = 0;
	    }
	}

	tw += cw;				// increment text width

	if ( cc == BEGLINE ) {
	    codes[begline] = BEGLINE | QMIN(tw,MAXWIDTH);
	    maxwidth = QMAX(maxwidth,tw);
	    begline = index;
	    nlines++;
	    tw = 0;
	    breakindex = tabindex = 0;
	}
	codes[index++] = cc;
	if ( index >= codelen - 1 ) {		// grow code array
	    codelen *= 2;
	    if ( code_alloc ) {
		codes = (ushort *)realloc( codes, sizeof(ushort)*codelen );
	    } else {
		codes = (ushort *)malloc( sizeof(ushort)*codelen );
		code_alloc = TRUE;
	    }
	}
	k++;
	p++;
    }

    if ( decode ) {				// decode from internal data
	char	  *data = *internal;
	text_info *ti	= (text_info*)data;
	if ( strncmp(ti->tag,"qptr",4)!=0 || ti->w != w || ti->h != h ||
	     ti->tf != tf || ti->len != len ) {
#if defined(CHECK_STATE)
	    warning( "QPainter::drawText: Internal text info is invalid" );
#endif
	    return;
	}
	maxwidth = ti->maxwidth;		// get internal values
	nlines	 = ti->nlines;
	codelen	 = ti->codelen;
	codes	 = (ushort *)(data + sizeof(text_info));
    } else {
	codes[begline] = BEGLINE | QMIN(tw,MAXWIDTH);
	maxwidth = QMAX(maxwidth,tw);
	nlines++;
	codes[index++] = 0;
	codelen = index;
    }

    if ( encode ) {				// build internal data
	char	  *data = new char[sizeof(text_info)+codelen*sizeof(ushort)];
	text_info *ti	= (text_info*)data;
	strncpy( ti->tag, "qptr", 4 );		// set tag
	ti->w	     = w;			// save parameters
	ti->h	     = h;
	ti->tf	     = tf;
	ti->len	     = len;
	ti->maxwidth = maxwidth;
	ti->nlines   = nlines;
	ti->codelen  = codelen;
	memcpy( data+sizeof(text_info), codes, codelen*sizeof(ushort) );
	*internal = data;
    }

    int	    fascent  = fm.ascent();		// get font measurements
    int	    fheight  = fm.height();
    QRegion save_rgn = crgn;			// save the current region
    bool    clip_on  = testf(ClipOn);
    int	    xp, yp;
    char    p_array[200];
    bool    p_alloc;

    if ( (tf & AlignVCenter) == AlignVCenter )	// vertically centered text
	yp = h/2 - nlines*fheight/2;
    else if ( (tf & AlignBottom) == AlignBottom)// bottom aligned
	yp = h - nlines*fheight;
    else					// top aligned
	yp = 0;
    if ( (tf & AlignRight) == AlignRight )
	xp = w - maxwidth;			// right aligned
    else if ( (tf & AlignHCenter) == AlignHCenter )
	xp = w/2 - maxwidth/2;			// centered text
    else
	xp = 0;					// left aligned

#if defined(CHECK_RANGE)
    int hAlignFlags = 0;
    if ( (tf & AlignRight) == AlignRight )
	hAlignFlags++;
    if ( (tf & AlignHCenter) == AlignHCenter )
	hAlignFlags++;
    if ( (tf & AlignLeft ) == AlignLeft )
	hAlignFlags++;

    if ( hAlignFlags > 1 ) 
	warning("QPainter::drawText: More than one of AlignRight, AlignLeft\n"
		"                    and AlignHCenter set in the tf parameter."
		);

    int vAlignFlags = 0;
    if ( (tf & AlignTop) == AlignTop )
	vAlignFlags++;
    if ( (tf & AlignVCenter) == AlignVCenter )
	vAlignFlags++;
    if ( (tf & AlignBottom ) == AlignBottom )
	vAlignFlags++;

    if ( hAlignFlags > 1 )
	warning("QPainter::drawText: More than one of AlignTop, AlignBottom\n"
		"                    and AlignVCenter set in the tf parameter."
		);
#endif // CHECK_RANGE

    QRect br( x+xp, y+yp, maxwidth, nlines*fheight );
    if ( brect )				// set bounding rect
	*brect = br;

    if ( (tf & DontPrint) != 0 ) {		// don't print any text
	if ( code_alloc )
	    free( codes );
	return;
    }

    if ( len > 200 ) {
	p = new char[len];			// buffer for printable string
	CHECK_PTR( p );
	p_alloc = TRUE;
    } else {
	p = p_array;
	p_alloc = FALSE;
    }

    if ( br.x() >= x && br.y() >= y && br.width() < w && br.height() < h )
	tf |= DontClip;				// no need to clip

    if ( (tf & DontClip) == 0 ) {		// clip text
	QRegion new_rgn;
	QRect r( x, y, w, h );
	if ( txop == TxRotShear ) {		// world xform active
	    QPointArray a( r );			// complex region
	    a = xForm( a );
	    new_rgn = QRegion( a );
	} else {
	    r = xForm( r );
	    new_rgn = QRegion( r );
	}
	if ( clip_on )				// combine with existing region
	    new_rgn = new_rgn.intersect( crgn );
	setClipRegion( new_rgn );
    }

    QPixmap  *pm;
    QPainter *pp;

    if ( (tf & GrayText) == GrayText ) {	// prepare to draw gray text
	pm = new QPixmap( w, fheight );
	CHECK_PTR( pm );
	pp = new QPainter;
	CHECK_PTR( pp );
	pp->begin( pm );
	pp->setBackgroundColor( bg_col );
	pp->setFont( cfont );
	pp->setPen( cpen.color() );
	pp->updatePen();
	pp->setBrush( QBrush(bg_col, Dense4Pattern) );
	pp->updateBrush();
    } else {
	pm = 0;
	pp = 0;
    }

    yp += fascent;

    register ushort *cp = codes;

    while ( *cp ) {				// finally, draw the text

	tw = *cp++ & WIDTHBITS;			// text width

	if ( tw == 0 ) {			// ignore empty line
	    while ( *cp && (*cp & BEGLINE) != BEGLINE )
		cp++;
	    yp += fheight;
	    continue;
	}

	if ( (tf & AlignRight) == AlignRight )
	    xp = w - tw;			// right aligned
	else if ( (tf & AlignHCenter) == AlignHCenter )
	    xp = w/2 - tw/2;			// centered text
	else
	    xp = 0;				// left aligned

	if ( pp )				// erase pixmap if gray text
	    pp->eraseRect( 0, 0, w, fheight );

	int bx = xp;				// base x position
	while ( TRUE ) {
	    k = 0;
	    while ( *cp && (*cp & (BEGLINE|TABSTOP)) == 0 ) {
		if ( (*cp & PREFIX) == PREFIX ) {
		    int xcpos = fm.width( p, k );
		    if ( pp )			// gray text
			pp->fillRect( xp+xcpos, fascent+fm.underlinePos(),
				      CWIDTH( *cp&0xff ), fm.lineWidth(),
				      cpen.color() );
		    else
			fillRect( x+xp+xcpos, y+yp+fm.underlinePos(),
				  CWIDTH( *cp&0xff ), fm.lineWidth(),
				  cpen.color() );
		}
		p[k++] = (char)*cp++;
	    }
	    if ( pp )				// gray text
		pp->drawText( xp, fascent, p, k );
	    else
		drawText( x+xp, y+yp, p, k );	// draw the text
	    if ( (*cp & TABSTOP) == TABSTOP )
		xp = bx + (*cp++ & WIDTHBITS);
	    else				// *cp == 0 || *cp == BEGLINE
		break;
	}
	if ( pp ) {				// gray text
	    pp->cpen.setStyle( NoPen );
	    pp->drawRect( bx, 0, tw, fheight );
	    drawPixmap( x, y+yp-fascent, *pm );
	}

	yp += fheight;
    }

    if ( pp ) {					// gray text
	pp->end();
	delete pp;
	delete pm;
    }

    if ( (tf & DontClip) == 0 ) {		// restore clipping
	if ( clip_on ) {			// set original region
	    setClipRegion( save_rgn );
	} else {				// clipping was off
	    crgn = save_rgn;
	    setClipping( FALSE );
	}
    }

    if ( p_alloc )
	delete [] p;
    if ( code_alloc )
	free( codes );
}


/*!

  Returns the bounding rectangle of the aligned text that would be
  printed with the corresponding drawText() function (the first \e len
  characters from \e str).  The drawing, and hence the bounding
  rectangle, is constrained to the rectangle \e (x,y,w,h).

  The \e tf text formatting is the bitwise OR of the following flags:
  <ul>
  <li> \c AlignLeft aligns to the left border.
  <li> \c AlignRight aligns to the right border.
  <li> \c AlignHCenter aligns horizontally centered.
  <li> \c AlignTop aligns to the top border.
  <li> \c AlignBottom aligns to the bottom border.
  <li> \c AlignVCenter aligns vertically centered
  <li> \c AlignCenter (= \c AlignHCenter | \c AlignVCenter)
  <li> \c SingleLine ignores newline characters in the text.
  <li> \c ExpandTabs expands tabulators.
  <li> \c ShowPrefix displays "&x" as "x" underlined.
  <li> \c WordBreak breaks the text to fit the rectangle.
  </ul>

  These flags are defined in qwindefs.h.

  \sa drawText(), fontMetrics()
*/

QRect QPainter::boundingRect( int x, int y, int w, int h, int tf,
			      const char *str, int len, char **internal )
{
    QRect brect;
    if ( str && *str )
	drawText( x, y, w, h, tf | DontPrint, str, len, &brect, internal );
    else
	brect.setRect( x,y, 0,0 );
    return brect;
}
