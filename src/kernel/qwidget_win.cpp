/****************************************************************************
**
** Implementation of QWidget and QWindow classes for Win32.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qapplication.h"
#include "qapplication_p.h"
#include "qpainter.h"
#include "qbitmap.h"
#include "qaccel.h"
#include "qimage.h"
#include "qlayout.h"
#include "qt_windows.h"
#include "qpaintdevicemetrics.h"
#include "qcursor.h"
#include <private/qapplication_p.h>
#include <private/qinputcontext_p.h>
#include "qevent.h"
#include "qwidget.h"
#include "qwidget_p.h"

#if defined(QT_TABLET_SUPPORT)
#define PACKETDATA  ( PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | \
		      PK_ORIENTATION | PK_CURSOR )
#define PACKETMODE  0
#include "qlibrary.h"
#include <wintab.h>
#include <pktdef.h>

typedef HCTX	( API *PtrWTOpen )(HWND, LPLOGCONTEXT, BOOL);
typedef BOOL	( API *PtrWTClose )(HCTX);
typedef UINT	( API *PtrWTInfo )(UINT, UINT, LPVOID);
typedef BOOL	( API *PtrWTEnable )(HCTX, BOOL);
typedef BOOL	( API *PtrWTOverlap )(HCTX, BOOL);
typedef int	( API *PtrWTPacketsGet )(HCTX, int, LPVOID);
typedef BOOL	( API *PtrWTGet )(HCTX, LPLOGCONTEXT);
typedef int     ( API *PtrWTQueueSizeGet )(HCTX);
typedef BOOL    ( API *PtrWTQueueSizeSet )(HCTX, int);

static PtrWTOpen ptrWTOpen = 0;
static PtrWTClose ptrWTClose = 0;
static PtrWTInfo ptrWTInfo = 0;
static PtrWTQueueSizeGet ptrWTQueueSizeGet = 0;
static PtrWTQueueSizeSet ptrWTQueueSizeSet = 0;
static void init_wintab_functions();
static void qt_tablet_init();
static void qt_tablet_cleanup();
extern HCTX qt_tablet_context;
extern bool qt_tablet_tilt_support;

static QWidget *qt_tablet_widget = 0;
#endif

#ifdef Q_OS_TEMP
#include "sip.h"
#endif

#if defined(QT_NON_COMMERCIAL)
#include "qnc_win.h"
#endif

#include "qwidget_p.h"

#if !defined(WS_EX_TOOLWINDOW)
#define WS_EX_TOOLWINDOW 0x00000080
#endif

#if !defined(GWLP_WNDPROC)
#define GWLP_WNDPROC GWL_WNDPROC
#endif

const QString qt_reg_winclass( int );		// defined in qapplication_win.cpp
void	    qt_olednd_unregister( QWidget* widget, QOleDropTarget *dst ); // dnd_win
QOleDropTarget* qt_olednd_register( QWidget* widget );


extern bool qt_nograb();
extern HRGN qt_win_bitmapToRegion(const QBitmap& bitmap);

static QWidget *mouseGrb    = 0;
static QCursor *mouseGrbCur = 0;
static QWidget *keyboardGrb = 0;
static HHOOK   journalRec  = 0;

extern "C" LRESULT CALLBACK QtWndProc( HWND, UINT, WPARAM, LPARAM );

extern void qt_set_paintevent_clipping( QPaintDevice* dev, const QRegion& region);
extern void qt_clear_paintevent_clipping();

/*****************************************************************************
  QWidget member functions
 *****************************************************************************/


void QWidget::create( WId window, bool initializeWindow, bool destroyOldWindow)
{
    if ( testWState(WState_Created) && window == 0 )
	return;
    setWState( WState_Created );			// set created flag

    if ( !parentWidget() || parentWidget()->isDesktop() )
	setWFlags( WType_TopLevel );		// top-level widget

    static int sw = -1, sh = -1;

    bool topLevel = testWFlags(WType_TopLevel);
    bool popup = testWFlags(WType_Popup);
    bool dialog = testWFlags(WType_Dialog);
    bool desktop  = testWFlags(WType_Desktop);
    HINSTANCE appinst  = qWinAppInst();
    HWND   parentw, destroyw = 0;
    WId	   id;

    QString windowClassName = qt_reg_winclass( getWFlags() );

    if ( !window )				// always initialize
	initializeWindow = TRUE;

    if ( popup ) {
	setWFlags(WStyle_StaysOnTop); // a popup stays on top
    }


    if ( sw < 0 ) {				// get the (primary) screen size
	sw = GetSystemMetrics( SM_CXSCREEN );
	sh = GetSystemMetrics( SM_CYSCREEN );
    }

    if ( window ) {
	// There's no way we can know the background color of the
	// other window because it could be cleared using the WM_ERASEBKND
	// message.  Therefore we assume white.
	bg_col = white;
    }

    if ( dialog || popup || desktop ) {		// these are top-level, too
	topLevel = TRUE;
	setWFlags( WType_TopLevel );
    }

    if ( desktop ) {				// desktop widget
	popup = FALSE;				// force this flags off
#ifndef Q_OS_TEMP
	if ( qt_winver != Qt::WV_NT && qt_winver != Qt::WV_95 )
	    crect.setRect( GetSystemMetrics( 76 /* SM_XVIRTUALSCREEN  */ ), GetSystemMetrics( 77 /* SM_YVIRTUALSCREEN  */ ),
			   GetSystemMetrics( 78 /* SM_CXVIRTUALSCREEN */ ), GetSystemMetrics( 79 /* SM_CYVIRTUALSCREEN */ ) );
	else
#endif
	    crect.setRect( 0, 0, GetSystemMetrics( SM_CXSCREEN ), GetSystemMetrics( SM_CYSCREEN ) );
    }

    parentw = parentWidget() ? parentWidget()->winId() : 0;

#ifdef UNICODE
    QString title;
    const TCHAR *ttitle = 0;
#endif
    const char *title95 = 0;
    int	 style = WS_CHILD;
    int	 exsty = WS_EX_NOPARENTNOTIFY;

    if ( window ) {
	style = GetWindowLongA( window, GWL_STYLE );
#ifndef QT_NO_DEBUG
	if ( !style )
	    qSystemWarning( "QWidget: GetWindowLong failed" );
#endif
	topLevel = FALSE; // #### needed for some IE plugins??
    } else if ( popup ) {
	style = WS_POPUP;
    } else if ( !topLevel ) {
	if ( !testWFlags(WStyle_Customize) )
	    setWFlags( WStyle_NormalBorder | WStyle_Title | WStyle_MinMax | WStyle_SysMenu  );
    } else if (!desktop ) {
	if ( testWFlags(WStyle_Customize) ) {
	    if ( testWFlags(WStyle_NormalBorder|WStyle_DialogBorder) == 0 ) {
		style = WS_POPUP;		// no border
	    } else {
		style = 0;
	    }
	} else {
	    style = WS_OVERLAPPED;
	    if ( testWFlags(WType_Dialog ) )
#ifndef Q_OS_TEMP
		setWFlags( WStyle_NormalBorder | WStyle_Title | WStyle_SysMenu | WStyle_ContextHelp );
	    else
		setWFlags( WStyle_NormalBorder | WStyle_Title | WStyle_MinMax | WStyle_SysMenu  );
	}
#else
		setWFlags( WStyle_NormalBorder | WStyle_Title | WStyle_SysMenu );
	    else
		setWFlags( WStyle_NormalBorder | WStyle_Title );
	}
	// Default to maximized show()
	QTLWExtra *top = topData();
	top->showMode = 2;
#endif
	// workaround for some versions of Windows
	if ( testWFlags( WStyle_MinMax ) )
	    clearWFlags( WStyle_ContextHelp );
    }
    if ( !desktop ) {
	// if ( !testWFlags( WPaintUnclipped ) )
	// ### Commented out for now as it causes some problems, but
	// this should be correct anyway, so dig some more into this
	    style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN ;
	if ( topLevel ) {
#ifndef Q_OS_TEMP
	    if ( testWFlags(WStyle_NormalBorder) ) {
		style |= WS_THICKFRAME;
		if( !testWFlags(WStyle_Title | WStyle_SysMenu | WStyle_Minimize | WStyle_Maximize | WStyle_ContextHelp) )
		    style |= WS_POPUP;
	    } else if ( testWFlags(WStyle_DialogBorder) )
		style |= WS_POPUP | WS_DLGFRAME;
#else
	    if ( testWFlags(WStyle_DialogBorder) )
		style |= WS_POPUP;
#endif
	    if ( testWFlags(WStyle_Title) )
		style |= WS_CAPTION;
	    if ( testWFlags(WStyle_SysMenu) )
		style |= WS_SYSMENU;
	    if ( testWFlags(WStyle_Minimize) )
		style |= WS_MINIMIZEBOX;
	    if ( testWFlags(WStyle_Maximize) )
		style |= WS_MAXIMIZEBOX;
	    if ( testWFlags(WStyle_Tool) | testWFlags(WType_Popup) )
		exsty |= WS_EX_TOOLWINDOW;
	    if ( testWFlags(WStyle_ContextHelp) )
		exsty |= WS_EX_CONTEXTHELP;
	}
    }
    if ( testWFlags(WStyle_Title) ) {
	QT_WA( {
	    title = QString::fromLocal8Bit( isTopLevel() ? qAppName() : name() );
	    ttitle = (TCHAR*)title.ucs2();
	} , {
	    title95 = isTopLevel() ? qAppName() : name();
	} );
    }

	// The WState_Created flag is checked by translateConfigEvent() in
	// qapplication_win.cpp. We switch it off temporarily to avoid move
	// and resize events during creation
    clearWState( WState_Created );

    if ( window ) {				// override the old window
	if ( destroyOldWindow )
	    destroyw = winid;
	id = window;
	setWinId( window );
	LONG res = SetWindowLongA( window, GWL_STYLE, style );
#ifndef QT_NO_DEBUG
	if ( !res )
	    qSystemWarning( "QWidget: Failed to set window style" );
#endif
	res = SetWindowLongA( window, GWLP_WNDPROC, (LONG)QtWndProc );
#ifndef QT_NO_DEBUG
	if ( !res )
	    qSystemWarning( "QWidget: Failed to set window procedure" );
#endif
    } else if ( desktop ) {			// desktop widget
#ifndef Q_OS_TEMP
	id = GetDesktopWindow();
	QWidget *otherDesktop = find( id );	// is there another desktop?
	if ( otherDesktop && otherDesktop->testWFlags(WPaintDesktop) ) {
	    otherDesktop->setWinId( 0 );	// remove id from widget mapper
	    setWinId( id );			// make sure otherDesktop is
	    otherDesktop->setWinId( id );	//   found first
	} else {
	    setWinId( id );
	}
#endif
    } else if ( topLevel ) {			// create top-level widget
	if ( popup )
	    parentw = 0;

#ifdef Q_OS_TEMP

	const TCHAR *cname = windowClassName.ucs2();

	id = CreateWindowEx( exsty, cname, ttitle, style, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, parentw, 0, appinst, 0 );
#else

	QT_WA( {
	    const TCHAR *cname = (TCHAR*)windowClassName.ucs2();
	    id = CreateWindowEx( exsty, cname, ttitle, style,
		    		CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				parentw, 0, appinst, 0 );
	} , {
	    id = CreateWindowExA( exsty, windowClassName.latin1(), title95, style,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				parentw, 0, appinst, 0 );
	} );

#endif

#ifndef QT_NO_DEBUG
	if ( id == NULL )
	    qSystemWarning( "QWidget: Failed to create window" );
#endif
	setWinId( id );
	if ( testWFlags( WStyle_StaysOnTop) )
	    SetWindowPos( id, HWND_TOPMOST, 0, 0, 100, 100, SWP_NOACTIVATE );
    } else {					// create child widget
	QT_WA( {
	    const TCHAR *cname = (TCHAR*)windowClassName.ucs2();
	    id = CreateWindowEx( exsty, cname, ttitle, style, 0, 0, 100, 30,
			    parentw, NULL, appinst, NULL );
	} , {
	    id = CreateWindowExA( exsty, windowClassName.latin1(), title95, style, 0, 0, 100, 30,
			    parentw, NULL, appinst, NULL );
	} );
#ifndef QT_NO_DEBUG
	if ( id == NULL )
	    qSystemWarning( "QWidget: Failed to create window" );
#endif
	SetWindowPos( id, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
	setWinId( id );
    }

    if ( desktop ) {
	setWState( WState_Visible );
    } else {
	RECT  fr, cr;
	GetWindowRect( id, &fr );		// update rects
	GetClientRect( id, &cr );
	if ( cr.top == cr.bottom && cr.left == cr.right ) {
	    if ( initializeWindow ) {
		int x, y, w, h;
		if ( topLevel ) {
		    x = sw/4;
		    y = 3*sh/10;
		    w = sw/2;
		    h = 4*sh/10;
		} else {
		    x = y = 0;
		    w = 100;
		    h = 30;
		}

		MoveWindow( winId(), x, y, w, h, TRUE );
	    }
	    GetWindowRect( id, &fr );		// update rects
	    GetClientRect( id, &cr );
	}
	if ( topLevel ){
	    // one cannot trust cr.left and cr.top, use a correction POINT instead
	    POINT pt;
	    pt.x = 0;
	    pt.y = 0;
	    ClientToScreen( id, &pt );
 	    crect = QRect( QPoint(pt.x, pt.y),
 			   QPoint(pt.x+cr.right, pt.y+cr.bottom) );

	    QTLWExtra *top = d->topData();
	    top->ftop = crect.top() - fr.top;
	    top->fleft = crect.left() - fr.left;
	    top->fbottom = fr.bottom - crect.bottom();
	    top->fright = fr.right - crect.right();
	    fstrut_dirty = FALSE;

	    d->createTLExtra();
 	} else {
	    crect.setCoords( cr.left, cr.top, cr.right, cr.bottom );
	    // in case extra data already exists (eg. reparent()).  Set it.
	}
    }

    setWState( WState_Created );		// accept move/resize events
    hdc = 0;					// no display context

    if ( window ) {				// got window from outside
	if ( IsWindowVisible(window) )
	    setWState( WState_Visible );
	else
	    clearWState( WState_Visible );
    }

#if defined(QT_NON_COMMERCIAL)
    QT_NC_WIDGET_CREATE
#endif

    if ( destroyw ) {
	DestroyWindow( destroyw );
    }

    setFontSys();
    QInputContext::enable( this, im_enabled & isEnabled() );
}


void QWidget::destroy( bool destroyWindow, bool destroySubWindows )
{
    deactivateWidgetCleanup();
    if ( testWState(WState_Created) ) {
	clearWState( WState_Created );
	for(int i = 0; i < d->children.size(); ++i ) { // destroy all widget children
	    register QObject *obj = d->children.at(i);
	    if ( obj->isWidgetType() )
		((QWidget*)obj)->destroy(destroySubWindows,
					 destroySubWindows);
	}
	if ( mouseGrb == this )
	    releaseMouse();
	if ( keyboardGrb == this )
	    releaseKeyboard();
	if ( testWFlags(WShowModal) )		// just be sure we leave modal
	    qt_leave_modal( this );
	else if ( testWFlags(WType_Popup) )
	    qApp->closePopup( this );
	if ( destroyWindow && !testWFlags(WType_Desktop) ) {
	    DestroyWindow( winId() );
	}
	setWinId( 0 );
    }
}


void QWidget::reparentSys( QWidget *parent, WFlags f, const QPoint &p,
			   bool showIt )
{
    QWidget* oldtlw = topLevelWidget();
    WId old_winid = winid;
    // hide and reparent our own window away. Otherwise we might get
    // destroyed when emitting the child remove event below. See QWorkspace.
    if ( isVisible() ) {
	ShowWindow( winid, SW_HIDE );
	SetParent( winid, 0 );
    }

    bool accept_drops = acceptDrops();
    if ( accept_drops )
	setAcceptDrops( FALSE ); // ole dnd unregister (we will register again below)
    if ( testWFlags(WType_Desktop) )
	old_winid = 0;
    setWinId( 0 );

    QObject::reparent(parent);
    bool     enable = isEnabled();		// remember status
    FocusPolicy fp = focusPolicy();
    QSize    s	    = size();
    QString capt= caption();
    widget_flags = f;
    clearWState(WState_Created | WState_Visible | WState_Hidden | WState_ExplicitShowHide);
    create();
    if ( isTopLevel() || (!parent || parent->isVisible() ) )
	setWState(WState_Hidden);
    QObjectList chlist = children();
    for(int i = 0; i < chlist.size(); ++i) { // reparent children
	QObject *obj = chlist.at(i);
	if ( obj->isWidgetType() ) {
	    QWidget *w = (QWidget *)obj;
	    if ( w->isPopup() )
		;
	    else if ( w->isTopLevel() )
		w->reparent( this, w->getWFlags(), w->pos(), !w->isHidden() );
	    else
		SetParent( w->winId(), winId() );
	}
    }

    setGeometry( p.x(), p.y(), s.width(), s.height() );
    setEnabled( enable );
    setFocusPolicy( fp );
    if ( !capt.isNull() ) {
	d->extra->topextra->caption = QString::null;
	setCaption( capt );
    }
    if ( showIt )
	show();
    if ( old_winid )
	DestroyWindow( old_winid );

    reparentFocusWidgets( oldtlw );		// fix focus chains

    if ( accept_drops )
	setAcceptDrops( TRUE );

}


QPoint QWidget::mapToGlobal( const QPoint &pos ) const
{
    if ( !isVisible() || isMinimized() )
	return mapTo( topLevelWidget(), pos ) + topLevelWidget()->pos() +
	(topLevelWidget()->geometry().topLeft() - topLevelWidget()->frameGeometry().topLeft());
    POINT p;
    p.x = pos.x();
    p.y = pos.y();
    ClientToScreen( winId(), &p );
    return QPoint( p.x, p.y );
}

QPoint QWidget::mapFromGlobal( const QPoint &pos ) const
{
    if ( !isVisible() || isMinimized() )
	return mapFrom( topLevelWidget(), pos - topLevelWidget()->pos() );
    POINT p;
    p.x = pos.x();
    p.y = pos.y();
    ScreenToClient( winId(), &p );
    return QPoint( p.x, p.y );
}


void QWidget::setFontSys( QFont *f )
{
    QInputContext::setFont( this, (f ? *f : font()) );
}

void QWidget::setMicroFocusHint(int x, int y, int width, int height, bool text, QFont *f)
{
    CreateCaret( winId(), 0, width, height );
    HideCaret( winId() );
    SetCaretPos( x, y );

    if ( text )
	QInputContext::setFocusHint( x, y, width, height, this );
    setFontSys( f );

    if ( QRect( x, y, width, height ) != microFocusHint() )
	d->extraData()->micro_focus_hint.setRect( x, y, width, height );
}

void QWidget::resetInputContext()
{
    QInputContext::accept();
}

void QWidget::setBackgroundColorDirect( const QColor &color )
{
    bg_col = color;
    if ( d->extra && d->extra->bg_pix ) {		// kill the background pixmap
	delete d->extra->bg_pix;
	d->extra->bg_pix = 0;
    }
}


static int allow_null_pixmaps = 0;


void QWidget::setBackgroundPixmapDirect( const QPixmap &pixmap )
{
    QPixmap old;
    if ( d->extra && d->extra->bg_pix )
	old = *d->extra->bg_pix;
    if ( !allow_null_pixmaps && pixmap.isNull() ) {
	if ( d->extra && d->extra->bg_pix ) {
	    delete d->extra->bg_pix;
	    d->extra->bg_pix = 0;
	}
    } else {
	if ( d->extra && d->extra->bg_pix )
	    delete d->extra->bg_pix;
	else
	    d->createExtra();
	d->extra->bg_pix = new QPixmap( pixmap );
    }
}


void QWidget::setBackgroundEmpty()
{
    allow_null_pixmaps++;
    setErasePixmap(QPixmap());
    allow_null_pixmaps--;
}


extern void qt_set_cursor( QWidget *, const QCursor & ); // qapplication_win.cpp

void QWidget::setCursor( const QCursor &cursor )
{
    if ( cursor.handle() != arrowCursor.handle()
	 || (d->extra && d->extra->curs) ) {
	d->createExtra();
	delete d->extra->curs;
	d->extra->curs = new QCursor(cursor);
    }
    setWState( WState_OwnCursor );
    qt_set_cursor( this, QWidget::cursor() );
}

void QWidget::unsetCursor()
{
    if ( d->extra ) {
	delete d->extra->curs;
	d->extra->curs = 0;
    }
    if ( !isTopLevel() )
	clearWState( WState_OwnCursor );
    qt_set_cursor( this, cursor() );
}

void QWidget::setCaption( const QString &caption )
{
    if ( QWidget::caption() == caption )
	return; // for less flicker
    d->topData()->caption = caption;

#if defined(QT_NON_COMMERCIAL)
    QT_NC_CAPTION
#else
    QString cap = caption;
#endif
    QT_WA( {
	SetWindowText( winId(), (TCHAR*)cap.ucs2() );
    } , {
	SetWindowTextA( winId(), cap.local8Bit() );
    } );

    QEvent e( QEvent::CaptionChange );
    QApplication::sendEvent( this, &e );
}

/*
  Create an icon mask the way Windows wants it using CreateBitmap.
*/

HBITMAP qt_createIconMask( const QBitmap &bitmap )
{
    QImage bm = bitmap.convertToImage();
    int w = bm.width();
    int h = bm.height();
    int bpl = ((w+15)/16)*2;			// bpl, 16 bit alignment
    uchar *bits = new uchar[bpl*h];
    bm.invertPixels();
    for ( int y=0; y<h; y++ )
	memcpy( bits+y*bpl, bm.scanLine(y), bpl );
    HBITMAP hbm = CreateBitmap( w, h, 1, 1, bits );
    delete [] bits;
    return hbm;
}


void QWidget::setIcon( const QPixmap &pixmap )
{
    QTLWExtra* x = d->topData();
    delete x->icon;
    x->icon = 0;
    if ( x->winIcon ) {
	DestroyIcon( x->winIcon );
	x->winIcon = 0;
    }
    if ( !pixmap.isNull() ) {			// valid icon
	QPixmap pm( pixmap.size(), pixmap.depth(), QPixmap::NormalOptim );
	QBitmap mask( pixmap.size(), FALSE, QPixmap::NormalOptim );
	if ( pixmap.mask() ) {
	    pm.fill( black );			// make masked area black
	    bitBlt( &mask, 0, 0, pixmap.mask() );
	} else {
	    mask.fill( color1 );
	}
	bitBlt( &pm, 0, 0, &pixmap );
	HBITMAP im = qt_createIconMask(mask);
	ICONINFO ii;
	ii.fIcon    = TRUE;
	ii.hbmMask  = im;
	ii.hbmColor = pm.hbm();
	ii.xHotspot = 0;
	ii.yHotspot = 0;
	x->icon = new QPixmap( pixmap );
	x->winIcon = CreateIconIndirect( &ii );
	DeleteObject( im );
    }
    SendMessageA( winId(), WM_SETICON, 0, /* ICON_SMALL */
		  (long)x->winIcon );
    SendMessageA( winId(), WM_SETICON, 1, /* ICON_BIG */
		  (long)x->winIcon );

    QEvent e( QEvent::IconChange );
    QApplication::sendEvent( this, &e );
}


void QWidget::setIconText( const QString &iconText )
{
    d->topData()->iconText = iconText;
}


QCursor *qt_grab_cursor()
{
    return mouseGrbCur;
}

// The procedure does nothing, but is required for mousegrabbing to work
LRESULT CALLBACK qJournalRecordProc( int nCode, WPARAM wParam, LPARAM lParam )
{
#ifndef Q_OS_TEMP
    return CallNextHookEx( journalRec, nCode, wParam, lParam );
#else
    return 0;
#endif
}

void QWidget::grabMouse()
{
    if ( !qt_nograb() ) {
	if ( mouseGrb )
	    mouseGrb->releaseMouse();
#ifndef Q_OS_TEMP
	journalRec = SetWindowsHookExA( WH_JOURNALRECORD, (HOOKPROC)qJournalRecordProc, GetModuleHandleA(0), 0 );
#endif
	SetCapture( winId() );
	mouseGrb = this;
    }
}

void QWidget::grabMouse( const QCursor &cursor )
{
    if ( !qt_nograb() ) {
	if ( mouseGrb )
	    mouseGrb->releaseMouse();
#ifndef Q_OS_TEMP
	journalRec = SetWindowsHookExA( WH_JOURNALRECORD, (HOOKPROC)qJournalRecordProc, GetModuleHandleA(0), 0 );
#endif
	SetCapture( winId() );
	mouseGrbCur = new QCursor( cursor );
	SetCursor( mouseGrbCur->handle() );
	mouseGrb = this;
    }
}

void QWidget::releaseMouse()
{
    if ( !qt_nograb() && mouseGrb == this ) {
	ReleaseCapture();
	if ( journalRec ) {
#ifndef Q_OS_TEMP
	    UnhookWindowsHookEx( journalRec );
#endif
	    journalRec = 0;
	}
	if ( mouseGrbCur ) {
	    delete mouseGrbCur;
	    mouseGrbCur = 0;
	}
	mouseGrb = 0;
    }
}

void QWidget::grabKeyboard()
{
    if ( !qt_nograb() ) {
	if ( keyboardGrb )
	    keyboardGrb->releaseKeyboard();
	keyboardGrb = this;
    }
}

void QWidget::releaseKeyboard()
{
    if ( !qt_nograb() && keyboardGrb == this )
	keyboardGrb = 0;
}


QWidget *QWidget::mouseGrabber()
{
    return mouseGrb;
}

QWidget *QWidget::keyboardGrabber()
{
    return keyboardGrb;
}

void QWidget::setActiveWindow()
{
    SetForegroundWindow( topLevelWidget()->winId() );
}


void QWidget::update()
{
    if ( (widget_state & (WState_Visible|WState_BlockUpdates)) == WState_Visible )
	InvalidateRect( winId(), 0, !testWFlags( WRepaintNoErase) );
}

void QWidget::update( int x, int y, int w, int h )
{
    if ( w && h &&
	 (widget_state & (WState_Visible|WState_BlockUpdates)) == WState_Visible ) {
	RECT r;
	r.left = x;
	r.top  = y;
	if ( w < 0 )
	    r.right = crect.width();
	else
	    r.right = x + w;
	if ( h < 0 )
	    r.bottom = crect.height();
	else
	    r.bottom = y + h;
	InvalidateRect( winId(), &r, !testWFlags( WRepaintNoErase) );
    }
}


void QWidget::repaint( int x, int y, int w, int h, bool erase )
{
    if ( (widget_state & (WState_Visible|WState_BlockUpdates)) == WState_Visible ) {
	if ( w < 0 )
	    w = crect.width()  - x;
	if ( h < 0 )
	    h = crect.height() - y;
	QRect r(x,y,w,h);
	if ( r.isEmpty() )
	    return; // nothing to do
	QRegion reg = r;
#ifndef Q_OS_TEMP
	if ( reg.handle() )
	    ValidateRgn( winId(), reg.handle() );
#endif
	QPaintEvent e( r, erase );
	if ( r != rect() )
	    qt_set_paintevent_clipping( this, r );
	if ( erase )
	    this->erase( x, y, w, h );
	QApplication::sendEvent( this, &e );
	qt_clear_paintevent_clipping();
    }
}

void QWidget::repaint( const QRegion& reg, bool erase )
{
    if ( (widget_state & (WState_Visible|WState_BlockUpdates)) == WState_Visible ) {
#ifndef Q_OS_TEMP
	ValidateRgn( winId(), reg.handle() );
#endif
	QPaintEvent e( reg, erase );
	qt_set_paintevent_clipping( this, reg );
	if ( erase )
	    this->erase( reg );
	QApplication::sendEvent( this, &e );
	qt_clear_paintevent_clipping();
    }
}

/*
  \internal
  Platform-specific part of QWidget::show().
*/

void QWidget::showWindow()
{
#if defined(QT_NON_COMMERCIAL)
    QT_NC_SHOW_WINDOW
#endif
    int sm = SW_SHOW;
    if ( isTopLevel() ) {
	switch ( d->topData()->showMode ) {
	case 1:
#ifdef Q_OS_TEMP
	    sm = SW_HIDE;
#else
	    sm = SW_SHOWMINIMIZED;
#endif
	    break;
	case 2:
	    sm = SW_SHOWMAXIMIZED;
	    break;
	default:
	    sm = SW_SHOW;
	    break;
	}
	d->topData()->showMode = 0; // reset
    }

    if ( testWFlags(WStyle_Tool) || isPopup() )
	sm = SW_SHOWNOACTIVATE;

    ShowWindow( winId(), sm );
    UpdateWindow( winId() );
}

/*
  \internal
  Platform-specific part of QWidget::hide().
*/

void QWidget::hideWindow()
{
    deactivateWidgetCleanup();
    ShowWindow( winId(), SW_HIDE );
}


#ifdef Q_OS_TEMP
# if UNDER_CE < 310
#  include <aygshell.h>
# else
#  define SHFS_SHOWTASKBAR            0x0001
#  define SHFS_SHOWSIPBUTTON          0x0004
   extern "C" BOOL __cdecl SHFullScreen(HWND hwndRequester, DWORD dwState);
# endif
#endif

void QWidget::showMinimized()
{
    if ( isTopLevel() ) {
	if ( d->topData()->fullscreen ) {
	    reparent( 0, d->topData()->savedFlags, d->topData()->normalGeometry.topLeft() );
	    d->topData()->fullscreen = 0;
#ifdef Q_OS_TEMP
	    SHFullScreen( winId(), SHFS_SHOWTASKBAR | SHFS_SHOWSIPBUTTON );
#endif
	}
#ifndef Q_OS_TEMP
	if ( isVisible() )
	    ShowWindow( winId(), SW_SHOWMINIMIZED );
	else
#endif
	{
	    d->topData()->showMode = 1;
	    show();
	}
    } else {
	show();
    }
    QEvent e( QEvent::ShowMinimized );
    QApplication::sendEvent( this, &e );
    clearWState( WState_Maximized);
    setWState( WState_Minimized );
}

bool QWidget::isMinimized() const
{
    // true for non-toplevels that have the minimized flag, e.g. MDI children
    return
#ifndef Q_OS_TEMP
	    IsIconic(winId()) || ( !isTopLevel() && testWState( WState_Minimized ) );
#else
	    testWState( WState_Minimized );
#endif
}

bool QWidget::isMaximized() const
{
    return
#ifndef Q_OS_TEMP
	    IsZoomed(winId()) || ( !isTopLevel() && testWState( WState_Maximized ) );
#else
	    testWState( WState_Maximized );
#endif
}

void QWidget::showMaximized()
{
    if ( isTopLevel() ) {
#ifndef Q_OS_TEMP
	if ( d->topData()->fullscreen ) {
	    reparent( 0, d->topData()->savedFlags, d->topData()->normalGeometry.topLeft() );
	    d->topData()->fullscreen = 0;
	} else if ( d->topData()->normalGeometry.width() < 0 ) {
	    d->topData()->normalGeometry = geometry();
	}
#else
	if ( topData()->normalGeometry.width() < 0 ) {
	    topData()->savedFlags = getWFlags();
	    topData()->normalGeometry = geometry();

	    reparent( 0,
		      WType_TopLevel | WStyle_Customize | WStyle_NoBorder |
		      (getWFlags() & 0xffff0000), // preserve some widget flags
		      topData()->normalGeometry.topLeft() );
	    topData()->fullscreen = 0;
	    SHFullScreen( winId(), SHFS_SHOWTASKBAR | SHFS_SHOWSIPBUTTON );
	}
#endif
	if ( isVisible() )
	    ShowWindow( winId(), SW_SHOWMAXIMIZED );
	else {
	    d->topData()->showMode = 2;
	    show();
	}
    }  else
	show();
    QEvent e( QEvent::ShowMaximized );
    QApplication::sendEvent( this, &e );
    clearWState( WState_Minimized );
    setWState( WState_Maximized );
}

void QWidget::showNormal()
{
    d->topData()->showMode = 0;
    if ( isTopLevel() ) {
#ifndef Q_OS_TEMP
	if ( d->topData()->fullscreen ) {
	    // when reparenting, preserve some widget flags
	    reparent( 0, d->topData()->savedFlags, d->topData()->normalGeometry.topLeft() );
#else
	if ( d->topData()->normalGeometry.width() > 0 ) {
	    int val = d->topData()->savedFlags;
	    int style = WS_OVERLAPPED,
		exsty = 0;

	    style |= (val & WStyle_DialogBorder ? WS_POPUP : 0);
	    style |= (val & WStyle_Title	? WS_CAPTION : 0);
	    style |= (val & WStyle_SysMenu	? WS_SYSMENU : 0);
	    style |= (val & WStyle_Minimize	? WS_MINIMIZEBOX : 0);
	    style |= (val & WStyle_Maximize	? WS_MAXIMIZEBOX : 0);
	    exsty |= (val & WStyle_Tool		? WS_EX_TOOLWINDOW : 0);
	    exsty |= (val & WType_Popup		? WS_EX_TOOLWINDOW : 0);
	    exsty |= (val & WStyle_ContextHelp	? WS_EX_CONTEXTHELP : 0);

	    SetWindowLong( winId(), GWL_STYLE, style );
	    if ( exsty )
		SetWindowLong( winId(), GWL_EXSTYLE, exsty );
	    // flush Window style cache
	    SHFullScreen( winId(), SHFS_SHOWTASKBAR | SHFS_SHOWSIPBUTTON );
	    SetWindowPos( winId(), HWND_TOP, 0, 0, 200, 200, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );
#endif
	    d->topData()->fullscreen = 0;
	    QRect r = d->topData()->normalGeometry;
	    if ( r.width() >= 0 ) {
		// the widget has been maximized
		d->topData()->normalGeometry = QRect(0,0,-1,-1);
		resize( r.size() );
		move( r.topLeft() );
	    }
	}
	show();
	ShowWindow( winId(), SW_SHOWNORMAL );
    } else {
	show();
    }
    if ( d->extra && d->extra->topextra )
	d->extra->topextra->fullscreen = 0;
    QEvent e( QEvent::ShowNormal );
    QApplication::sendEvent( this, &e );
    clearWState( WState_Maximized | WState_Minimized );
}


void QWidget::raise()
{
    QWidget *p = parentWidget();
    if ( p && p->d->children.indexOf(this) >= 0 ) {
	p->d->children.remove( this );
	p->d->children.append( this );
    }
    uint f = ( isPopup() || testWFlags(WStyle_Tool) ) ? SWP_NOACTIVATE : 0;
    SetWindowPos( winId(), HWND_TOP, 0, 0, 0, 0, f | SWP_NOMOVE | SWP_NOSIZE );
}

void QWidget::lower()
{
    QWidget *p = parentWidget();
    if ( p && p->d->children.indexOf(this) >= 0 ) {
	p->d->children.remove( this );
	p->d->children.prepend( this );
    }
    uint f = ( isPopup() || testWFlags(WStyle_Tool) ) ? SWP_NOACTIVATE : 0;
    SetWindowPos( winId(), HWND_BOTTOM, 0, 0, 0, 0, f | SWP_NOMOVE |
		  SWP_NOSIZE );
}

void QWidget::stackUnder( QWidget* w)
{
    QWidget *p = parentWidget();
    if ( !w || isTopLevel() || p != w->parentWidget() || this == w )
	return;
    if ( p && p->d->children.indexOf(w) >= 0 && p->d->children.indexOf(this) >= 0 ) {
	p->d->children.remove(this);
	p->d->children.insert( p->d->children.indexOf(w), this );
    }
    SetWindowPos( winId(), w->winId() , 0, 0, 0, 0, SWP_NOMOVE |
		  SWP_NOSIZE );
}


//
// The internal qWinRequestConfig, defined in qapplication_win.cpp, stores move,
// resize and setGeometry requests for a widget that is already
// processing a config event. The purpose is to avoid recursion.
//
void qWinRequestConfig( WId, int, int, int, int, int );

void QWidget::setGeometry_helper( int x, int y, int w, int h, bool isMove )
{
    if ( d->extra ) {				// any size restrictions?
	w = QMIN(w,d->extra->maxw);
	h = QMIN(h,d->extra->maxh);
	w = QMAX(w,d->extra->minw);
	h = QMAX(h,d->extra->minh);
    }
    if ( w < 1 )				// invalid size
	w = 1;
    if ( h < 1 )
	h = 1;
    QSize  oldSize( size() );
    QPoint oldPos( pos() );
    if ( isMove == FALSE && oldSize.width()==w && oldSize.height()==h )
	return;
    clearWState(WState_Maximized);
    if ( testWState(WState_ConfigPending) ) {	// processing config event
	qWinRequestConfig( winId(), isMove ? 2 : 1, x, y, w, h );
    } else {
	setWState( WState_ConfigPending );
	if ( isTopLevel() ) {
	    QRect fr( frameGeometry() );
	    if ( d->extra ) {
		fr.setLeft( fr.left() + x - crect.left() );
		fr.setTop( fr.top() + y - crect.top() );
		fr.setRight( fr.right() + ( x + w - 1 ) - crect.right() );
		fr.setBottom( fr.bottom() + ( y + h - 1 ) - crect.bottom() );
	    }
	    MoveWindow( winId(), fr.x(), fr.y(), fr.width(), fr.height(), TRUE );
	} else {
	    crect.setRect( x, y, w, h );
	    MoveWindow( winId(), x, y, w, h, TRUE );
	    crect.setRect( x, y, w, h );
	}
	clearWState( WState_ConfigPending );
    }

    bool isResize = w != oldSize.width() || h != oldSize.height();
    if ( isVisible() ) {
	if ( isMove && pos() != oldPos ) {
	    QMoveEvent e( pos(), oldPos );
	    QApplication::sendEvent( this, &e );
	}
	if ( isResize ) {
	    QResizeEvent e( size(), oldSize );
	    QApplication::sendEvent( this, &e );
	    if ( !testWFlags( WStaticContents ) )
		repaint( visibleRect(), !testWFlags(WResizeNoErase) );
	}
    } else {
	if (isMove && pos() != oldPos)
	    d->setAttribute(WA_PendingMoveEvent, true);
	if (isResize)
	    d->setAttribute(WA_PendingResizeEvent, true);
    }
}


void QWidget::setMinimumSize( int minw, int minh )
{
    if ( minw < 0 || minh < 0 )
	qWarning("QWidget::setMinimumSize: The smallest allowed size is (0,0)");
    d->createExtra();
    if ( d->extra->minw == minw && d->extra->minh == minh )
	return;
    d->extra->minw = minw;
    d->extra->minh = minh;
    if ( minw > width() || minh > height() ) {
	bool resized = testWState( WState_Resized );
	resize( QMAX(minw,width()), QMAX(minh,height()) );
	if ( !resized )
	    clearWState( WState_Resized ); //not a user resize
    }
    updateGeometry();
}

void QWidget::setMaximumSize( int maxw, int maxh )
{
    if ( maxw > QWIDGETSIZE_MAX || maxh > QWIDGETSIZE_MAX ) {
	qWarning("QWidget::setMaximumSize: The largest allowed size is (%d,%d)",
		 QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
	maxw = QMIN( maxw, QWIDGETSIZE_MAX );
	maxh = QMIN( maxh, QWIDGETSIZE_MAX );
    }
    if ( maxw < 0 || maxh < 0 ) {
	qWarning("QWidget::setMaximumSize: (%s/%s) Negative sizes (%d,%d) "
		"are not possible",
		name( "unnamed" ), className(), maxw, maxh );
	maxw = QMAX( maxw, 0 );
	maxh = QMAX( maxh, 0 );
    }
    d->createExtra();
    if ( d->extra->maxw == maxw && d->extra->maxh == maxh )
	return;
    d->extra->maxw = maxw;
    d->extra->maxh = maxh;
    if ( maxw < width() || maxh < height() ) {
	bool resized = testWState( WState_Resized );
	resize( QMIN(maxw,width()), QMIN(maxh,height()) );
	if ( !resized )
	    clearWState( WState_Resized ); //not a user resize
    }
    updateGeometry();
}

void QWidget::setSizeIncrement( int w, int h )
{
    d->createTLExtra();
    d->extra->topextra->incw = w;
    d->extra->topextra->inch = h;
}

void QWidget::setBaseSize( int w, int h )
{
    d->createTLExtra();
    d->extra->topextra->basew = w;
    d->extra->topextra->baseh = h;
}


extern void qt_erase_background( HDC, int, int, int, int,
			 const QColor &, const QPixmap *, int, int );

void QWidget::erase( int x, int y, int w, int h )
{
    // SIMILAR TO region ERASE BELOW

    if ( backgroundMode()==NoBackground )
	return;
    if ( w < 0 )
	w = crect.width() - x;
    if ( h < 0 )
	h = crect.height() - y;

    HDC lhdc;
    bool tmphdc;

    if( QPainter::redirect( this ) ) {
	tmphdc = FALSE;
	lhdc = QPainter::redirect( this )->handle();
	Q_ASSERT( lhdc );
    } else if ( !hdc ) {
	tmphdc = TRUE;
	lhdc = GetDC( winId() );
    } else {
	tmphdc = FALSE;
	lhdc = hdc;
    }

    QPoint offset = backgroundOffset();
    int ox = offset.x();
    int oy = offset.y();

    qt_erase_background( lhdc, x, y, w, h, bg_col, backgroundPixmap(), ox, oy );

    if ( tmphdc ) {
	ReleaseDC( winId(), lhdc );
	hdc = 0;
    }
}

void QWidget::erase( const QRegion& rgn )
{
    // SIMILAR TO rect ERASE ABOVE

    if ( backgroundMode()==NoBackground )
	return;

    HDC lhdc;
    bool tmphdc;

    if( QPainter::redirect( this ) ) {
	tmphdc = FALSE;
	lhdc = QPainter::redirect( this )->handle();
	Q_ASSERT( lhdc );
    } else if ( !hdc ) {
	tmphdc = TRUE;
	lhdc = GetDC( winId() );
    } else {
	tmphdc = FALSE;
	lhdc = hdc;
    }

    HRGN oldRegion = CreateRectRgn( 0, 0, 0, 0 );
    bool hasRegion = GetClipRgn( lhdc, oldRegion ) != 0;
    HRGN newRegion = 0;
    if ( hasRegion ) {
	newRegion = CreateRectRgn( 0, 0, 0, 0 );
	CombineRgn(newRegion, oldRegion, rgn.handle(), RGN_AND );
    } else {
	newRegion = rgn.handle();
    }
    SelectClipRgn( lhdc, newRegion );

    QPoint offset = backgroundOffset();
    int ox = offset.x();
    int oy = offset.y();

    qt_erase_background( lhdc, 0, 0, crect.width(), crect.height(), bg_col,
			 backgroundPixmap(), ox, oy );
    SelectClipRgn( lhdc, hasRegion ? oldRegion : 0 );
    DeleteObject( oldRegion );
    if ( hasRegion )
	DeleteObject( newRegion );
    if ( tmphdc ) {
	ReleaseDC( winId(), lhdc );
	hdc = 0;
    }
}


void QWidget::scroll( int dx, int dy )
{
    if ( testWState( WState_BlockUpdates ) && !children() )
	return;
    UINT flags = SW_INVALIDATE | SW_SCROLLCHILDREN;
    if ( backgroundMode() != NoBackground )
	flags |= SW_ERASE;

    ScrollWindowEx( winId(), dx, dy, 0, 0, 0, 0, flags );
    UpdateWindow( winId() );
}

void QWidget::scroll( int dx, int dy, const QRect& r )
{
    if ( testWState( WState_BlockUpdates ) )
	return;
    UINT flags = SW_INVALIDATE;
    if ( backgroundMode() != NoBackground )
	flags |= SW_ERASE;

    RECT wr;
    wr.top = r.top();
    wr.left = r.left();
    wr.bottom = r.bottom()+1;
    wr.right = r.right()+1;
    ScrollWindowEx( winId(), dx, dy, &wr, &wr, 0, 0, flags );
    UpdateWindow( winId() );
}


int QWidget::metric( int m ) const
{
    int val;
    if ( m == QPaintDeviceMetrics::PdmWidth ) {
	val = crect.width();
    } else if ( m == QPaintDeviceMetrics::PdmHeight ) {
	val = crect.height();
    } else {
	HDC gdc = GetDC( 0 );
	switch ( m ) {
	case QPaintDeviceMetrics::PdmDpiX:
	case QPaintDeviceMetrics::PdmPhysicalDpiX:
	    val = GetDeviceCaps( gdc, LOGPIXELSX );
	    break;
	case QPaintDeviceMetrics::PdmDpiY:
	case QPaintDeviceMetrics::PdmPhysicalDpiY:
	    val = GetDeviceCaps( gdc, LOGPIXELSY );
	    break;
	case QPaintDeviceMetrics::PdmWidthMM:
	    val = crect.width()
		    * GetDeviceCaps( gdc, HORZSIZE )
		    / GetDeviceCaps( gdc, HORZRES );
	    break;
	case QPaintDeviceMetrics::PdmHeightMM:
	    val = crect.height()
		    * GetDeviceCaps( gdc, VERTSIZE )
		    / GetDeviceCaps( gdc, VERTRES );
	    break;
	case QPaintDeviceMetrics::PdmNumColors:
	    if ( GetDeviceCaps(gdc, RASTERCAPS) & RC_PALETTE )
		val = GetDeviceCaps( gdc, SIZEPALETTE );
	    else {
		int bpp = GetDeviceCaps( hdc, BITSPIXEL );
		if( bpp==32 )
		    val = INT_MAX;
		else if( bpp<=8 )
		    val = GetDeviceCaps( hdc, NUMCOLORS );
		else
		    val = 1 << ( bpp * GetDeviceCaps( hdc, PLANES ) );
	    }
	    break;
	case QPaintDeviceMetrics::PdmDepth:
	    val = GetDeviceCaps( gdc, BITSPIXEL );
	    break;
	default:
	    val = 0;
	    qWarning( "QWidget::metric: Invalid metric command" );
	}
	ReleaseDC( 0, gdc );
    }
    return val;
}

void QWidgetPrivate::createSysExtra()
{
    extra->dropTarget = 0;
}

void QWidgetPrivate::deleteSysExtra()
{
    q->setAcceptDrops( FALSE );
}

void QWidgetPrivate::createTLSysExtra()
{
    extra->topextra->winIcon = 0;
}

void QWidgetPrivate::deleteTLSysExtra()
{
    if ( extra->topextra->winIcon )
	DestroyIcon( extra->topextra->winIcon );
}


bool QWidget::acceptDrops() const
{
    return ( d->extra && d->extra->dropTarget );
}

void QWidget::setAcceptDrops( bool on )
{
    // Enablement is defined by d->extra->dropTarget != 0.

    if ( on ) {
	// Turn on.
	d->createExtra();
	QWExtra *extra = d->extraData();
	if ( !extra->dropTarget )
	    extra->dropTarget = qt_olednd_register( this );
    } else {
	// Turn off.
	QWExtra *extra = d->extraData();
	if ( extra && extra->dropTarget ) {
	    qt_olednd_unregister(this, extra->dropTarget);
	    extra->dropTarget = 0;
	}
    }
}

void QWidget::setMask( const QRegion &region )
{
    // Since SetWindowRegion takes ownership, and we need to translate,
    // we take a copy.
    HRGN wr = CreateRectRgn(0,0,1,1);
    CombineRgn(wr, region.handle(), 0, RGN_COPY);

    int fleft = 0, ftop = 0;
    if (isTopLevel()) {
	ftop = d->topData()->ftop;
	fleft = d->topData()->fleft;
    }
    OffsetRgn(wr, fleft, ftop );
    SetWindowRgn( winId(), wr, TRUE );
}

void QWidget::setMask( const QBitmap &bitmap )
{
    HRGN wr = qt_win_bitmapToRegion(bitmap);

    int fleft = 0, ftop = 0;
    if (isTopLevel()) {
	ftop = d->topData()->ftop;
	fleft = d->topData()->fleft;
    }
    OffsetRgn(wr, fleft, ftop );
    SetWindowRgn( winId(), wr, TRUE );
}

void QWidget::clearMask()
{
    SetWindowRgn( winId(), 0, TRUE );
}

void QWidget::setName( const char *name )
{
    QObject::setName( name );
}

void QWidget::updateFrameStrut() const
{
    QWidget *that = (QWidget *) this;

    if ( !isVisible() || isDesktop() ) {
	that->fstrut_dirty = isVisible();
	return;
    }

    RECT  fr, cr;
    GetWindowRect( winId(), &fr );
    GetClientRect( winId(), &cr );

    POINT pt;
    pt.x = 0;
    pt.y = 0;

    ClientToScreen( winId(), &pt );
    that->crect = QRect( QPoint( pt.x, pt.y ),
 			 QPoint( pt.x + cr.right, pt.y + cr.bottom ) );

    QTLWExtra *top = that->d->topData();
    top->ftop = crect.top() - fr.top;
    top->fleft = crect.left() - fr.left;
    top->fbottom = fr.bottom - crect.bottom();
    top->fright = fr.right - crect.right();

    that->fstrut_dirty = FALSE;
}

void QWidget::setMouseTracking( bool enable )
{
    if ( enable )
        setWState( WState_MouseTracking );
    else
        clearWState( WState_MouseTracking );
#if defined(QT_TABLET_SUPPORT)
    if ( !qt_tablet_widget )
	qt_tablet_init();
#endif
}

#if defined(QT_TABLET_SUPPORT)
extern bool qt_is_gui_used;
static void init_wintab_functions()
{
    if ( !qt_is_gui_used )
        return;
    QLibrary library( "wintab32" );
    library.setAutoUnload( FALSE );
    QT_WA( {
        ptrWTOpen = (PtrWTOpen)library.resolve( "WTOpenW" );
        ptrWTInfo = (PtrWTInfo)library.resolve( "WTInfoW" );
    } , {
        ptrWTOpen = (PtrWTOpen)library.resolve( "WTOpenA" );
        ptrWTInfo = (PtrWTInfo)library.resolve( "WTInfoA" );
    } );

    ptrWTClose = (PtrWTClose)library.resolve( "WTClose" );
    ptrWTQueueSizeGet = (PtrWTQueueSizeGet)library.resolve( "WTQueueSizeGet" );
    ptrWTQueueSizeSet = (PtrWTQueueSizeSet)library.resolve( "WTQueueSizeSet" );
}

static void qt_tablet_init()
{
    if ( qt_tablet_widget )
	return;
    LOGCONTEXT lcMine;
    qAddPostRoutine( qt_tablet_cleanup );
    struct tagAXIS tpOri[3];
    init_wintab_functions();
    if ( ptrWTInfo && ptrWTOpen && ptrWTQueueSizeGet && ptrWTQueueSizeSet ) {
        // make sure we have WinTab
        if ( !ptrWTInfo( 0, 0, NULL ) ) {
            qWarning( "Wintab services not available" );
            return;
        }

        // some tablets don't support tilt, check if it is possible,
        qt_tablet_tilt_support = ptrWTInfo( WTI_DEVICES, DVC_ORIENTATION, &tpOri );
        if ( qt_tablet_tilt_support ) {
            // check for azimuth and altitude
            qt_tablet_tilt_support = tpOri[0].axResolution && tpOri[1].axResolution;
        }
        // build our context from the default context
        ptrWTInfo( WTI_DEFSYSCTX, 0, &lcMine );
        lcMine.lcOptions |= CXO_MESSAGES | CXO_CSRMESSAGES;
        lcMine.lcPktData = PACKETDATA;
        lcMine.lcPktMode = PACKETMODE;
        lcMine.lcMoveMask = PACKETDATA;
        lcMine.lcOutOrgX = 0;
        lcMine.lcOutExtX = GetSystemMetrics( SM_CXSCREEN );
        lcMine.lcOutOrgY = 0;
        lcMine.lcOutExtY = -GetSystemMetrics( SM_CYSCREEN );
        qt_tablet_widget = new QWidget( 0, "Qt internal tablet widget" );
        qt_tablet_context = ptrWTOpen( qt_tablet_widget->winId(), &lcMine, TRUE);
	if ( qt_tablet_context == NULL ) {
	    qWarning( "Failed to open the tablet" );
	    return;
	}
	// Set the size of the Packet Queue to the correct size...
	int currSize = ptrWTQueueSizeGet( qt_tablet_context );
	if ( !ptrWTQueueSizeSet(qt_tablet_context, QT_TABLET_NPACKETQSIZE) ) {
	    // Ideally one might want to make use a smaller
	    // multiple, but for now, since we managed to destroy
	    // the existing Q with the previous call, set it back
	    // to the other size, which should work.  If not,
	    // we had best die.
	    if ( !ptrWTQueueSizeSet(qt_tablet_context, currSize) )
		qWarning( "There is no packet queue for the tablet.\n"
		"The tablet will not work" );
	}

    }
}

static void qt_tablet_cleanup()
{
    if ( ptrWTClose )
        ptrWTClose( qt_tablet_context );
    delete qt_tablet_widget;
    qt_tablet_widget = 0;
}
#endif
