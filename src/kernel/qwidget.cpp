/****************************************************************************
** $Id: //depot/qt/main/src/kernel/qwidget.cpp#347 $
**
** Implementation of QWidget class
**
** Created : 931031
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

#include "qobjectlist.h"
#include "qobjectdict.h"
#include "qwidget.h"
#include "qwidgetlist.h"
#include "qwidgetintdict.h"
#include "qptrdict.h"
#include "qfocusdata.h"
#include "qpixmap.h"
#include "qkeycode.h"
#include "qapplication.h"
#include "qbrush.h"
#include "qlayout.h"
#if defined(_WS_WIN_)
#include "qt_windows.h"
#endif


/*!
  \class QWidget qwidget.h
  \brief The QWidget class is the base class of all user interface objects.

  \ingroup abstractwidgets

  The widget is the atom of the user interface: It receives mouse,
  keyboard and other events from the window system, and paints a
  representation of itself on the screen.  Every widget is
  rectangular, and they are sorted in a Z-order.  A widget is clipped
  by its parent and by the widgets in front of it.

  A widget without a parent, called a top-level widget, is a window
  with a frame and a title bar (though it is also possible to create
  top level widgets without such decoration by the use of <a
  href="#widgetflags">widget flags</a>).  In Qt, QMainWindow and the
  various subclasses of QDialog are the most common top-level windows.

  A widget with a parent is a child window in its parent.  You usually
  cannot distinguish a child widget from its parent visually.  Most
  other widgets in Qt are useful only as child widgets.  (You \e can
  make a e.g. button into a top-level widget, but most people prefer
  to put their buttons in e.g. dialogs.)

  QWidget has many member functions, but some of them have little
  direct functionality - for example it has a font but never uses it
  itself. There are many subclasses which provide real functionality,
  as diverse as QPushButton, QListBox and QTabDialog.

  <strong>Groups of functions:</strong>
  <ul>

  <li> Window functions:
	show(),
	hide(),
	raise(),
	lower(),
	close().

  <li> Top level windows:
	caption(),
	setCaption(),
	icon(),
	setIcon(),
	iconText(),
	setIconText(),
	isActiveWindow(),
	setActiveWindow(),
	showMinimized().

  <li> Window contents:
	update(),
	repaint(),
	erase(),
	scroll().

  <li> Geometry:
	pos(),
	size(),
	rect(),
	x(),
	y(),
	width(),
	height(),
	sizePolicy(),
	sizeHint(),
	layout(),
	setLayout()
	move(),
	resize(),
	setGeometry(),
	frameGeometry(),
	geometry(),
	childrenRect(),
	adjustSize(),
	mapFromGlobal(),
	mapFromParent()
	mapToGlobal(),
	mapToParent(),
	maximumSize(),
	minimumSize(),
	sizeIncrement(),
	setMaximumSize(),
	setMinimumSize(),
	setSizeIncrement(),
	setFixedSize()

  <li> Mode:
	isVisible(),
	isVisibleTo(),
	isVisibleToTLW(),
	isDesktop(),
	isEnabled(),
	isEnabledTo(),
	isEnabledToTLW(),
	isModal(),
	isPopup(),
	isTopLevel(),
	setEnabled(),
	hasMouseTracking(),
	setMouseTracking(),
	isUpdatesEnabled(),
	setUpdatesEnabled(),
	setFontPropagation(),
	fontPropagation(),
	setPalettePropagation(),
	palettePropagation().

  <li> Look and feel:
	style(),
	cursor(),
	setCursor()
	font(),
	setFont(),
	palette(),
	setPalette(),
	backgroundMode(),
	setBackgroundMode(),
	backgroundPixmap(),
	setBackgroundPixmap(),
	backgroundBrush(),
	colorGroup(),
	fontMetrics(),
	fontInfo().

  <li> Keyboard focus functions:
	isFocusEnabled(),
	setFocusPolicy(),
	focusPolicy(),
	hasFocus(),
	setFocus(),
	clearFocus(),
	setTabOrder(),
	setFocusProxy().

  <li> Mouse and keyboard grabbing:
	grabMouse(),
	releaseMouse(),
	grabKeyboard(),
	releaseKeyboard(),
	mouseGrabber(),
	keyboardGrabber().

  <li> Event handlers:
	event(),
	mousePressEvent(),
	mouseReleaseEvent(),
	mouseDoubleClickEvent(),
	mouseMoveEvent(),
	keyPressEvent(),
	keyReleaseEvent(),
	focusInEvent(),
	focusOutEvent(),
	wheelEvent(),
	enterEvent(),
	leaveEvent(),
	paintEvent(),
	moveEvent(),
	resizeEvent(),
	closeEvent(),
	dragEnterEvent(),
	dragMoveEvent(),
	dragLeaveEvent(),
	dropEvent(),
	childEvent(),
	showEvent(),
	hideEvent(),
	customEvent().

  <li> Change handlers:
	backgroundColorChange(),
	backgroundPixmapChange(),
	enabledChange(),
	fontChange(),
	paletteChange(),
	styleChange().

  <li> System functions:
	parentWidget(),
	topLevelWidget(),
	reparent(),
	winId(),
	find(),
	metric().

  <li> Internal kernel functions:
	setFRect(),
	setCRect(),
	focusNextPrevChild(),
	wmapper(),
	clearWFlags(),
	getWFlags(),
	setWFlags(),
	testWFlags().
  </ul>

  Every widget's constructor accepts two or three standard arguments:
  <ul>
  <li><code>QWidget *parent = 0</code> is the parent of the new widget.
  If it is 0 (the default), the new widget will be a top-level window.
  If not, it will be a child of \e parent, and be constrained by \e
  parent's geometry.
  <li><code>const char * name = 0</code> is the widget name of the new
  widget.  The widget name is little used at the moment - the
  dumpObjectTree() debugging function uses it, and you can access it using
  name().  It will become more important when our visual GUI builder is
  ready (you can name a a widget in the builder, and connect() to it by
  name in your code).
  <li><code>WFlags f = 0</code> (where available) sets the <a
  href="#widgetflags">widget flags;</a> the default is good for almost
  all widgets, but to get e.g. top-level widgets without a window
  system frame you must use special flags.
  </ul>

  The tictac/tictac.cpp example program is good example of a simple
  widget.  It contains a few event handlers (as all widgets must), a
  few custom routines that are peculiar to it (as all useful widgets
  must), and has a few children and connections.  Everything it does
  is done in response to an event: This is by far the most common way
  to design GUI applications.

  You will need to supply the content for your widgets yourself, but
  here is a brief run-down of the events, starting with the most common
  ones: <ul>

  <li> paintEvent() - called whenever the widget needs to be
  repainted.  Every widget which displays output must implement it,
  and it is sensible to \e never paint on the screen outside
  paintEvent().  The widget is guaranteed to receive a paint event
  when the widget is first shown.  Unless the widget was created with
  the WResizeNoErase flag, the widget will also receive a paint event
  immediately after every resize events.

  <li> resizeEvent() - normally called when the widget has been
  resized.  For widgets created with the WResizeNoErase flag, see the
  full documentation for resizeEvent() for details.

  <li> mousePressEvent() - called when a mouse button is pressed.
  There are six mouse-related events, mouse press and mouse release
  events are by far the most important.  A widget receives mouse press
  events when the widget is inside it, or when it has grabbed the
  mouse using grabMouse().

  <li> mouseReleaseEvent() - called when a mouse button is released.
  A widget receives mouse release events when it has received the
  corresponding mouse press event.  This means that if the user
  presses the mouse inside \e your widget, then drags the mouse to
  somewhere else, then releases, \e your widget receives the release
  event.  There is one exception, however: If a popup menu appears
  while the mouse button is held down, that popup steals the mouse
  events at once.

  <li> mouseDoubleClickEvent() - not quite as obvious as it might seem.
  If the user double-clicks, the widget receives a mouse press event
  (perhaps a mouse move event or two if he/she does not hold the mouse
  quite steady), a mouse release event and finally this event.  It is \e
  not \e possible to distinguish a click from a double click until you've
  seen whether the second click arrives.  (This is one reason why most GUI
  books recommend that double clicks be an extension of single clicks,
  rather than trigger a different action.)
  </ul>

  If your widget only contains child widgets, you probably do not need to
  implement any event handlers.

  Widgets that accept keyboard input need to reimplement a few more
  event handlers: <ul>

  <li> keyPressEvent() - called whenever a key is pressed, and again
  when a key has been held down long enough for it to auto-repeat.
  Note that the Tab and shift-Tab keys are only passed to the widget
  if they are not used by the focus-change mechanisms.  To force those
  keys to be processed by your widget, you must override QWidget::event().

  <li> focusInEvent() - called when the widget gains keyboard focus
  (assuming you have called setFocusPolicy(), of course). Well
  written widgets indicate that they own the keyboard focus in a clear
  but discreet way.

  <li> focusOutEvent() - called when the widget loses keyboard
  focus.
  </ul>

  Some widgets will need to reimplement some more obscure event
  handlers, too: <ul>

  <li> mouseMoveEvent() - called whenever the mouse moves while a
  button is held down.  This is useful for e.g. dragging.  If you call
  setMouseTracking(TRUE), you get mouse move events even when no
  buttons are held down.  (Note that applications which make use of
  mouse tracking are often not very useful on low-bandwidth X
  connections.)

  <li> keyReleaseEvent() - called whenever a key is released, and also
  while it is held down if the key is auto-repeating.  In that case
  the widget receives a key release event and immediately a key press
  event for every repeat.  Note that the Tab and shift-Tab keys are
  only passed to the widget if they are not used by the focus-change
  mechanisms.  To force those keys to be processed by your widget, you
  must override QWidget::event().

  <li> wheelEvent() -- called whenever the user turns the mouse wheel
  while the widget has the focus.

  <li> enterEvent() - called when the mouse enters the widget's screen
  space.  (This excludes screen space owned by any children of the
  widget.)

  <li> leaveEvent() - called when the mouse leaves the widget's screen
  space.

  <li> moveEvent() - called when the widget has been moved relative to its
  parent.

  <li> closeEvent() - called when the user closes the widget (or when
  close() is called).
 </ul>

  There are also some \e really obscure events.  They are listed in
  qevent.h and you need to reimplement event() to handle them.  The
  default implementation of event() handles Tab and shift-Tab (to move
  the keyboard focus), and passes on every other event to one of the
  more specialized handlers above.

  When writing a widget, there are a few more things to look out
  for. <ul>

  <li> In the constructor, be sure to set up your member variables
  early on, before there's any chance that you might receive an event.

  <li>It is almost always useful to reimplement sizePolicy or at least
  sizeHint(), so users of your class can set up layout management more
  easily.

  sizePolicy() lets you supply good defaults for the layout management
  handling, so that other widgets can contain yours easily.
  sizeHint() indicates a "good" size for the widget.

  <li>If your widget is a top-level window, setCaption() and setIcon() set
  the title bar and icon respectively.

  </ul>

  \sa QEvent, QPainter, QGridLayout, QBoxLayout
*/


/*****************************************************************************
  Internal QWidgetMapper class

  The purpose of this class is to map widget identifiers to QWidget objects.
  All QWidget objects register themselves in the QWidgetMapper when they
  get an identifier. Widgets unregister themselves when they change ident-
  ifier or when they are destroyed. A widget identifier is really a window
  handle.

  The widget mapper is created and destroyed by the main application routines
  in the file qapp_xxx.cpp.
 *****************************************************************************/

static const int WDictSize = 101;

class QWidgetMapper : public QWidgetIntDict
{						// maps ids -> widgets
public:
    QWidgetMapper();
   ~QWidgetMapper();
    QWidget *find( WId id );			// find widget
    void     insert( const QWidget * );		// insert widget
    bool     remove( WId id );			// remove widget
private:
    WId	     cur_id;
    QWidget *cur_widget;
};

QWidgetMapper *QWidget::mapper = 0;		// app global widget mapper


QWidgetMapper::QWidgetMapper() : QWidgetIntDict(WDictSize)
{
    cur_id = 0;
    cur_widget = 0;
}

QWidgetMapper::~QWidgetMapper()
{
    clear();
}

inline QWidget *QWidgetMapper::find( WId id )
{
    if ( id != cur_id ) {			// need to lookup
	cur_widget = QWidgetIntDict::find((long)id);
	if ( cur_widget )
	    cur_id = id;
	else
	    cur_id = 0;
    }
    return cur_widget;
}

inline void QWidgetMapper::insert( const QWidget *widget )
{
    QWidgetIntDict::insert((long)widget->winId(),widget);
}

inline bool QWidgetMapper::remove( WId id )
{
    if ( cur_id == id ) {			// reset current widget
	cur_id = 0;
	cur_widget = 0;
    }
    return QWidgetIntDict::remove((long)id);
}


/*****************************************************************************
  QWidget member functions
 *****************************************************************************/


/*!
  Constructs a widget which is a child of \e parent, with the name \e name and
  widget flags set to \e f.

  If \e parent is 0, the new widget becomes a \link isTopLevel() top-level
  window\endlink. If \e parent is another widget, this widget becomes a child
  window inside \e parent.  Unless the newly created widget is
  \link QWidget::reparent() reparented\endlink, it will be deleted when
  the parent is deleted.

  The \e name is sent to the QObject constructor.

  <a name="widgetflags"></a>

  The widget flags argument \e f is normally 0, but it can be set to
  customize the window frame of a top-level widget (i.e. \e parent must be
  zero). To customize the frame, set the \c WStyle_Customize flag OR'ed with
  any of these flags:

  <ul>
  <li> \c WStyle_NormalBorder gives the window a normal border. Cannot
    be combined with \c WStyle_DialogBorder or \c WStyle_NoBorder.
  <li> \c WStyle_DialogBorder gives the window a thin dialog border.
    Cannot be combined with \c WStyle_NormalBorder or \c WStyle_NoBorder.
  <li> \c WStyle_NoBorder gives a borderless window. Note that the
    user cannot move or resize a borderless window via the window system.
    Cannot be combined with \c WStyle_NormalBorder or \c WStyle_DialogBorder.
  <li> \c WStyle_Title gives the window a title bar.
  <li> \c WStyle_SysMenu adds a window system menu.
  <li> \c WStyle_Minimize adds a minimize button.
  <li> \c WStyle_Maximize adds a maximize button.
  <li> \c WStyle_MinMax is equal to <code>(WStyle_Minimize | WStyle_Maximize)
  </code>.
  <li> \c WStyle_Tool makes the window a tool window, usually
    combined with \c WStyle_NoBorder. A tool window is a small window that
    lives for a short time and it is typically used for creating popup
    windows.
  </ul>

  Note that X11 does not necessarily support all style flag combinations. X11
  window managers live their own lives and can only take hints. Win32
  supports all style flags.

  Example:
  \code
    QLabel *toolTip = new QLabel( 0, "myToolTip",
				  WStyle_Customize | WStyle_NoBorder |
				  WStyle_Tool );
  \endcode

  The widget flags are defined in qwindowdefs.h (which is included by
  qwidget.h).
*/

QWidget::QWidget( QWidget *parent, const char *name, WFlags f )
    : QObject( parent, name ), QPaintDevice( QInternal::Widget ),
      pal( parent ? parent->palette()		// use parent's palette
           : *qApp->palette() )			// use application palette
{
    isWidget = TRUE;				// is a widget
    winid = 0;					// default attributes
    widget_state = 0;
    widget_flags = f;
    propagate_font = 0;
    propagate_palette = 0;
    focus_policy = 0;
    lay_out = 0;
    extra = 0;					// no extra widget info
    create();					// platform-dependent init
    // make sure move/resize events are sent to all widgets
    QApplication::postEvent( this, new QMoveEvent( fpos, fpos ) );
    QApplication::postEvent( this, new QResizeEvent(crect.size(),
						    crect.size()) );
#if defined(_OS_LINUX_)
#warning "Arnt, can you have a look at this - tabtofocus no longer widget flag"
#endif
#if 0
    if ( isTopLevel() ||			// kludge alert
	 testWState(QWS_TabToFocus) ) {		// focus was set using WFlags
	QFocusData *fd = focusData( TRUE );
	if ( fd->focusWidgets.findRef(this) < 0 )
 	    fd->focusWidgets.append( this );
	if ( isTopLevel() ) {
	    // kludge details: set focus to this widget, whether it
	    // accepss focus or not.  this makes buggy old qt programs
	    // which assume that the tlw has focus even though it
	    // doesn't accept focus work.
	    if ( fd->it.current() != this ) {
		fd->it.toFirst();
		while ( fd->it.current() != this && !fd->it.atLast() )
		    ++fd->it;
	    }
	}
    }
#endif
}


/*!
  Destroys the widget.

  All children of this widget are deleted first.
  The application exits if this widget is (was) the main widget.
*/

QWidget::~QWidget()
{
#if defined (CHECK_STATE)
    if ( paintingActive() )
	warning( "%s (%s): deleted while being painted", className(), name() );
#endif

    // remove myself from the can-take-focus list
    QFocusData *f = focusData( FALSE );
    if ( f )
	f->focusWidgets.removeRef( this );

    if ( QApplication::main_widget == this ) {	// reset main widget
	QApplication::main_widget = 0;
	if (qApp)
	    qApp->quit();
    }
    if ( focusWidget() == this )
	clearFocus();
    if ( QApplication::focus_widget == this )
	QApplication::focus_widget = 0;

    if ( isTopLevel() && isVisible() && winId() )
	hide();					// emit lastWindowClosed?

    // A parent widget must destroy all its children before destroying itself
    if ( childObjects ) {			// delete children objects
	QObjectListIt it(*childObjects);
	QObject *obj;
	while ( (obj=it.current()) ) {
	    ++it;
	    delete obj;
	    if ( !childObjects )		// removeChild resets it
		break;
	}
	delete childObjects;
	childObjects = 0;
    }

    if ( extra )
	deleteExtra();
    destroy();					// platform-dependent cleanup
}


/*!
  \internal
  Creates the global widget mapper.
  The widget mapper converts window handles to widget pointers.
  \sa destroyMapper()
*/

void QWidget::createMapper()
{
    mapper = new QWidgetMapper;
    CHECK_PTR( mapper );
}

/*!
  \internal
  Destroys the global widget mapper.
  \sa createMapper()
*/

void QWidget::destroyMapper()
{
    if ( !mapper )				// already gone
	return;
    QWidgetIntDictIt it( *((QWidgetIntDict*)mapper) );
    QWidgetMapper * myMapper = mapper;
    mapper = 0;
    register QWidget *w;
    while ( (w=it.current()) ) {		// remove parents widgets
	++it;
	if ( !w->parentObj )			// widget is a parent
	    w->destroy( TRUE, TRUE );
    }
    delete myMapper;
}


static QWidgetList *wListInternal( QWidgetMapper *mapper, bool onlyTopLevel )
{
    QWidgetList *list = new QWidgetList;
    CHECK_PTR( list );
    if ( mapper ) {
	QWidget *w;
	QWidgetIntDictIt it( *((QWidgetIntDict*)mapper) );
	while ( (w=it.current()) ) {
	    ++it;
	    if ( !onlyTopLevel || w->isTopLevel() )
		list->append( w );
	}
    }
    return list;
}

/*!
  \internal
  Returns a list of all widgets.
  \sa tlwList(), QApplication::allWidgets()
*/

QWidgetList *QWidget::wList()
{
    return wListInternal( mapper, FALSE );
}

/*!
  \internal
  Returns a list of all top level widgets.
  \sa wList(), QApplication::topLevelWidgets()
*/

QWidgetList *QWidget::tlwList()
{
    return wListInternal( mapper, TRUE );
}


void QWidget::setWinId( WId id )		// set widget identifier
{
    if ( !mapper )				// mapper destroyed
	return;
    if ( winid )
	mapper->remove( winid );
    winid = id;
#if defined(_WS_X11_)
    hd = id;					// X11: hd == ident
#endif
    if ( id )
	mapper->insert( this );
}


/*!
  \internal
  Returns a pointer to the block of extra widget data.
*/

QWExtra *QWidget::extraData()
{
    return extra;
}

void QWidget::createTLExtra()
{
    if ( !extra )
	createExtra();
    if ( !extra->topextra ) {
	extra->topextra = new QTLWExtra;
	extra->topextra->icon = 0;
	extra->topextra->focusData = 0;
	extra->topextra->fsize = crect.size();
	extra->topextra->incw = extra->topextra->inch = 0;
	extra->topextra->iconic = 0;
#if defined(_WS_X11_)
	extra->topextra->normalGeometry = QRect(0,0,-1,-1);
	extra->topextra->embedded = 0;
	extra->topextra->parentWinId = 0;
#endif
	createTLSysExtra();
    }
}

/*!
  \internal
  Creates the widget extra data.
*/

void QWidget::createExtra()
{
    if ( !extra ) {				// if not exists
	extra = new QWExtra;
	CHECK_PTR( extra );
	extra->minw = extra->minh = 0;
	extra->maxw = extra->maxh = QCOORD_MAX;
	extra->bg_pix = 0;
	extra->focus_proxy = 0;
	extra->curs = 0;
	extra->topextra = 0;
	extra->bg_mode = PaletteBackground;
	extra->sizegrip = 0;
	createSysExtra();
    }
}


/*!
  \internal
  Deletes the widget extra data.
*/

void QWidget::deleteExtra()
{
    if ( extra ) {				// if exists
	delete extra->bg_pix;
	delete extra->curs;
	deleteSysExtra();
	if ( extra->topextra ) {
	    deleteTLSysExtra();
	    delete extra->topextra->icon;
	    delete extra->topextra->focusData;
	}
	delete extra;
	// extra->xic destroyed in QWidget::destroy()
	extra = 0;
    }
}


/*!
  Returns a pointer to the widget with window identifer/handle \e id.

  The window identifier type depends by the underlying window system,
  see qwindowdefs.h for the actual definition.
  If there is no widget with this identifier, a null pointer is returned.

  \sa wmapper(), id()
*/

QWidget *QWidget::find( WId id )
{
    return mapper ? mapper->find( id ) : 0;
}

/*!
  \fn QWidgetMapper *QWidget::wmapper()
  \internal
  Returns a pointer to the widget mapper.

  The widget mapper is an internal dictionary that is used to map from
  window identifiers/handles to widget pointers.
  \sa find(), id()
*/


/*!
  \fn WFlags QWidget::getWFlags() const
  \internal
  Returns the widget flags for this this widget.

  Widget flags are internal, not meant for public use.
  \sa testWFlags(), setWFlags(), clearWFlags()
*/

/*!
  \fn void QWidget::setWFlags( WFlags f )
  \internal
  Sets the widget flags \e f.

  Widget flags are internal, not meant for public use.
  \sa testWFlags(), getWFlags(), clearWFlags()
*/

/*!
  \fn void QWidget::clearWFlags( WFlags f )
  \internal
  Clears the widget flags \e f.

  Widget flags are internal, not meant for public use.
  \sa testWFlags(), getWFlags(), setWFlags()
*/


/*!
  \fn WId QWidget::winId() const
  Returns the window system identifier of the widget.

  Portable in principle, but if you use it you are probably about to do
  something non-portable. Be careful.

  \sa find()
*/


/*!
  Returns the GUI style for this widget.

  \sa setStyle(), QApplication::style()
*/

QStyle& QWidget::style() const
{
    return qApp->style();
}


/*!
  \fn void QWidget::styleChange( GUIStyle oldStyle )

  This virtual function is called from setStyle().  \e oldStyle is the
  previous style; you can get the new style from style().

  Reimplement this function if your widget needs to know when its GUI
  style changes.  You will almost certainly need to update the widget
  using either repaint(TRUE) or update().

  The default implementation calls update().

  \sa setStyle(), style(), repaint(), update()
*/

void QWidget::styleChange( GUIStyle )
{
    update();
}


/*!
  \fn bool QWidget::isTopLevel() const
  Returns TRUE if the widget is a top-level widget, otherwise FALSE.

  A top-level widget is a widget which usually has a frame and a \link
  setCaption() caption\endlink (title bar). \link isPopup() Popup\endlink
  and \link isDesktop() desktop\endlink widgets are also top-level
  widgets. Modal \link QDialog dialog\endlink widgets are the only
  top-level widgets that can have \link parentWidget() parent
  widgets\endlink; all other top-level widgets have null parents.  Child
  widgets are the opposite of top-level widgets.

  \sa topLevelWidget(), isModal(), isPopup(), isDesktop(), parentWidget()
*/

/*!
  \fn bool QWidget::isModal() const
  Returns TRUE if the widget is a modal widget, otherwise FALSE.

  A modal widget is also a top-level widget.

  \sa isTopLevel(), QDialog
*/

/*!
  \fn bool QWidget::isPopup() const
  Returns TRUE if the widget is a popup widget, otherwise FALSE.

  A popup widget is created by specifying the widget flag \c WType_Popup
  to the widget constructor.

  A popup widget is also a top-level widget.

  \sa isTopLevel()
*/


/*!
  \fn bool QWidget::isDesktop() const
  Returns TRUE if the widget is a desktop widget, otherwise FALSE.

  A desktop widget is also a top-level widget.

  \sa isTopLevel(), QApplication::desktop()
*/


/*!
  \fn bool QWidget::isEnabled() const
  Returns TRUE if the widget is enabled, or FALSE if it is disabled.
  \sa setEnabled()
*/


/*!
  Returns TRUE if this widget and every parent up to but excluding
  \a ancestor is enabled, otherwise returns FALSE.

  \sa setEnabled() isEnabled()
*/

bool QWidget::isEnabledTo(QWidget* ancestor) const
{
    const QWidget * w = this;
    while ( w && w->isEnabled()
	    && !w->isTopLevel()
	    && w->parentWidget()
	    && w->parentWidget()!=ancestor
	)
	w = w->parentWidget();
    return w->isEnabled();
}


/*!
  Returns TRUE if this widget and every parent up to the \link
  topLevelWidget() top level widget \endlink is enabled, otherwise
  returns FALSE.

  This is equivalent to isEnabledTo(0).

  \sa setEnabled() isEnabled()
*/

bool QWidget::isEnabledToTLW() const
{
    return isEnabledTo(0);
}


/*!
  Enables widget input events if \e enable is TRUE, otherwise disables
  input events.

  An enabled widget receives keyboard and mouse events; a disabled
  widget does not.  Note that an enabled widget receives keyboard
  events only when it is in focus.

  Some widgets display themselves differently when they are disabled.
  For example a button might draw its label grayed out.

  \sa isEnabled(), QKeyEvent, QMouseEvent
*/

void QWidget::setEnabled( bool enable )
{
    if ( isEnabled() == enable) // nothing to do
	return;
    if ( enable ) {
	if ( testWState(QWS_Disabled) ) {
	    clearWState( QWS_Disabled );
	    setBackgroundFromMode();
	    enabledChange( TRUE );
	}
    } else {
	if ( !testWState(QWS_Disabled) ) {
	    if ( focusWidget() == this )
		focusNextPrevChild( TRUE );
	    setWState( QWS_Disabled );
	    setBackgroundFromMode();
	    enabledChange( FALSE );
	}
    }
}

/*!
  \fn void QWidget::enabledChange( bool oldEnabled )

  This virtual function is called from setEnabled(). \e oldEnabled is the
  previous setting; you can get the new setting from isEnabled().

  Reimplement this function if your widget needs to know when it becomes
  enabled or disabled. You will almost certainly need to update the widget
  using either repaint(TRUE) or update().

  The default implementation calls repaint(TRUE).

  \sa setEnabled(), isEnabled(), repaint(), update()
*/

void QWidget::enabledChange( bool )
{
    repaint();
}


/*!
  \fn QRect QWidget::frameGeometry() const
  Returns the geometry of the widget, relative to its parent and
  including the window frame.
  \sa geometry(), x(), y(), pos()
*/

/*!
  \fn const QRect &QWidget::geometry() const
  Returns the geometry of the widget, relative to its parent widget
  and excluding the window frame.
  \sa frameGeometry(), size(), rect()
*/

/*!
  \fn int QWidget::x() const
  Returns the x coordinate of the widget, relative to its parent
  widget and including the window frame.
  \sa frameGeometry(), y(), pos()
*/

/*!
  \fn int QWidget::y() const
  Returns the y coordinate of the widget, relative to its parent
  widget and including the window frame.
  \sa frameGeometry(), x(), pos()
*/

/*!
  \fn QPoint QWidget::pos() const
  Returns the position of the widget in its parent widget, including
  the window frame.
  \sa frameGeometry(), x(), y()
*/

/*!
  \fn QSize QWidget::size() const
  Returns the size of the widget, excluding the window frame.
  \sa geometry(), width(), height()
*/

/*!
  \fn int QWidget::width() const
  Returns the width of the widget, excluding the window frame.
  \sa geometry(), height(), size()
*/

/*!
  \fn int QWidget::height() const
  Returns the height of the widget, excluding the window frame.
  \sa geometry(), width(), size()
*/

/*!
  \fn QRect QWidget::rect() const
  Returns the the internal geometry of the widget, excluding the window frame.
  rect() equals QRect(0,0,width(),height()).
  \sa size()
*/


/*!
  Returns the bounding rectangle of the widget's children.
*/

QRect QWidget::childrenRect() const
{
    QRect r( 0, 0, 0, 0 );
    if ( !children() )
	return r;
    QObjectListIt it( *children() );		// iterate over all children
    QObject *obj;
    while ( (obj=it.current()) ) {
	++it;
	if ( obj->isWidgetType() )
	    r = r.unite( ((QWidget*)obj)->geometry() );
    }
    return r;
}

/*!
  Returns the combined region of the widget's children geometry().
*/

QRegion QWidget::childrenRegion() const
{
    QRegion r;
    if ( !children() )
	return r;
    QObjectListIt it( *children() );		// iterate over all children
    QObject *obj;
    while ( (obj=it.current()) ) {
	++it;
	if ( obj->isWidgetType() )
	    r = r.unite( ((QWidget*)obj)->geometry() );
    }
    return r;
}


/*!
  Returns the minimum widget size.

  The widget cannot be resized to a smaller size than the minimum widget
  size.

  \sa maximumWidth(), maximumHeight(), setMinimumSize(),
  maximumSize(), sizeIncrement()
*/

QSize QWidget::minimumSize() const
{
    return extra ? QSize(extra->minw,extra->minh) : QSize(0,0);
}

/*!
  Returns the maximum widget size.

  The widget cannot be resized to a larger size than the maximum widget
  size.

  \sa maximumWidth(), maximumHeight(), setMaximumSize(),
  minimumSize(), sizeIncrement()
*/

QSize QWidget::maximumSize() const
{
    return extra ? QSize(extra->maxw,extra->maxh)
		 : QSize(QCOORD_MAX,QCOORD_MAX);
}


/*!
  \fn int QWidget::minimumWidth() const

  Returns the widget's minimum width.

  \sa minimumSize(), minimumHeight()
*/

/*!
  \fn int QWidget::minimumHeight() const

  Returns the widget's minimum height.

  \sa minimumSize(), minimumWidth()
*/

/*!
  \fn int QWidget::maximumWidth() const

  Returns the widget's maximum width.

  \sa maximumSize(), maximumHeight()
*/

/*!
  \fn int QWidget::maximumHeight() const

  Returns the widget's maximum height.

  \sa maximumSize(), maximumWidth()
*/


/*!
  Returns the widget size increment.

  \sa setSizeIncrement(), minimumSize(), maximumSize()
*/

QSize QWidget::sizeIncrement() const
{
    return extra && extra->topextra
	? QSize(extra->topextra->incw,extra->topextra->inch)
	: QSize(0,0);
}


/*!
  Sets both the minimum and maximum sizes of the widget to \e s,
  thereby preventing it from ever growing or shrinking.

  \sa setMaximumSize() setMinimumSize()
*/

void QWidget::setFixedSize( const QSize & s)
{
    setMinimumSize( s );
    setMaximumSize( s );
    resize( s );
}


/*!
  \overload void QWidget::setFixedSize( int w, int h )
*/

void QWidget::setFixedSize( int w, int h )
{
    setMinimumSize( w, h );
    setMaximumSize( w, h );
    resize( w, h );
}


/*!
  Sets the minimum width of the widget to \a w without changing the
  height.  Provided for convenience.

  \sa sizeHint() minimumSize() maximumSize()
  setFixedSize() and more
*/

void QWidget::setMinimumWidth( int w )
{
    setMinimumSize( w, minimumSize().height() );
}


/*!
  Sets the minimum height of the widget to \a h without changing the
  width.  Provided for convenience.

  \sa sizeHint() minimumSize() maximumSize() setFixedSize() and more
*/

void QWidget::setMinimumHeight( int h )
{
    setMinimumSize( minimumSize().width(), h );
}


/*!
  Sets the maximum width of the widget to \a w without changing the
  height.  Provided for convenience.

  \sa sizeHint() minimumSize() maximumSize() setFixedSize() and more
*/

void QWidget::setMaximumWidth( int w )
{
    setMaximumSize( w, maximumSize().height() );
}


/*!
  Sets the maximum height of the widget to \a h without changing the
  width.  Provided for convenience.

  \sa sizeHint() minimumSize() maximumSize() setFixedSize() and more
*/

void QWidget::setMaximumHeight( int h )
{
    setMaximumSize( maximumSize().width(), h );
}


/*!
  Sets both the minimum and maximum width of the widget to \a w
  without changing the heights.  Provided for convenience.

  \sa sizeHint() minimumSize() maximumSize() setFixedSize() and more
*/

void QWidget::setFixedWidth( int w )
{
    setMinimumSize( w, minimumSize().height() );
    setMaximumSize( w, maximumSize().height() );
}


/*!
  Sets both the minimum and maximum heights of the widget to \a h
  without changing the widths.  Provided for convenience.

  \sa sizeHint() minimumSize() maximumSize() setFixedSize() and more
*/

void QWidget::setFixedHeight( int h )
{
    setMinimumSize( minimumSize().width(), h );
    setMaximumSize( maximumSize().width(), h );
}


/*!
  Translates the widget coordinate \e pos to a coordinate in the parent widget.

  Same as mapToGlobal() if the widget has no parent.
  \sa mapFromParent()
*/

QPoint QWidget::mapToParent( const QPoint &p ) const
{
    return p + crect.topLeft();
}

/*!
  Translates the parent widget coordinate \e pos to widget coordinates.

  Same as mapFromGlobal() if the widget has no parent.

  \sa mapToParent()
*/

QPoint QWidget::mapFromParent( const QPoint &p ) const
{
    return p - crect.topLeft();
}


/*!
  Returns the top-level widget for this widget.

  A top-level widget is an overlapping widget. It usually has no parent.
  Modal \link QDialog dialog widgets\endlink are the only top-level
  widgets that can have parent widgets.

  \sa isTopLevel()
*/

QWidget *QWidget::topLevelWidget() const
{
    QWidget *w = (QWidget *)this;
    QWidget *p = w->parentWidget();
    while ( !w->testWFlags(WType_TopLevel) && p ) {
	w = p;
	p = p->parentWidget();
    }
    return w;
}


// Please do NOT remove the FAQ answer from this doc again.  It's a
// FAQ, it remains a FAQ, and people apparently will not follow three
// links to find the right answer.

/*!
  This function is deprecated.  Use setBackgroundMode() or setPalette(),
  as they ensure the appropriate clearing color is used when the widget
  is in the Active, Normal, or Disabled state.

  If you want to change the color scheme of a widget, the setPalette()
  function is better suited.  Here is how to set \e thatWidget to use a
  light green (RGB value 80, 255, 80) as background color, with shades
  of green used for all the 3D effects:

  \code
    thatWidget->setPalette( QPalette( QColor(80, 255, 80) ) );
  \endcode

  \sa setPalette(), QApplication::setPalette(), backgroundColor(),
      setBackgroundPixmap(), setBackgroundMode()
*/

void QWidget::setBackgroundColor( const QColor &color )
{
    setBackgroundModeDirect( FixedColor );
    setBackgroundColorDirect( color );
}


/*!
  Sets the background pixmap of the widget to \e pixmap.

  This function is deprecated.  Use setBackgroundMode() or
  setPalette(), as they ensure the appropriate clearing pixmap or
  color is used when the widget is in the Active, Normal, or Disabled
  state.

  The background pixmap is tiled.  Some widgets (e.g. QLineEdit) do
  not work well with a background pixmap.

  \sa backgroundPixmap(), backgroundPixmapChange(), setBackgroundColor()

  \internal
  This function is call with a null pixmap by setBackgroundEmpty().
*/

void QWidget::setBackgroundPixmap( const QPixmap &pixmap )
{
    setBackgroundPixmapDirect( pixmap );
    setBackgroundModeDirect( FixedPixmap );

}

void QWidget::setBackgroundFromMode()
{
    switch (extra ? (BackgroundMode)extra->bg_mode : PaletteBackground) {
      case FixedColor:
      case FixedPixmap:
      case NoBackground:
	break;
      case PaletteForeground:
	  if ( colorGroup().fillForeground().pixmap() )
	      setBackgroundPixmapDirect( *colorGroup().fillForeground().pixmap() );
	  else
	      setBackgroundColorDirect( colorGroup().foreground() );
	break;
      case PaletteButton:
	  if ( colorGroup().fillButton().pixmap() )
	      setBackgroundPixmapDirect( *colorGroup().fillButton().pixmap() );
	  else
	      setBackgroundColorDirect( colorGroup().button() );
	break;
      case PaletteLight:
	  if ( colorGroup().fillLight().pixmap() )
	      setBackgroundPixmapDirect( *colorGroup().fillLight().pixmap() );
	  else
	      setBackgroundColorDirect( colorGroup().light() );
	break;
      case PaletteMidlight:
	  if ( colorGroup().fillMidlight().pixmap() )
	      setBackgroundPixmapDirect( *colorGroup().fillMidlight().pixmap() );
	  else
	      setBackgroundColorDirect( colorGroup().midlight() );
	break;
      case PaletteDark:
	  if ( colorGroup().fillDark().pixmap() )
	      setBackgroundPixmapDirect( *colorGroup().fillDark().pixmap() );
	  else
	      setBackgroundColorDirect( colorGroup().dark() );
	break;
      case PaletteMid:
	  if ( colorGroup().fillMid().pixmap() )
	      setBackgroundPixmapDirect( *colorGroup().fillMid().pixmap() );
	  else
	      setBackgroundColorDirect( colorGroup().mid() );
	break;
      case PaletteText:
	  if ( colorGroup().fillText().pixmap() )
	      setBackgroundPixmapDirect( *colorGroup().fillText().pixmap() );
	  else
	      setBackgroundColorDirect( colorGroup().text() );
	break;
      case PaletteBrightText:
	  if ( colorGroup().fillBrightText().pixmap() )
	      setBackgroundPixmapDirect( *colorGroup().fillBrightText().pixmap() );
	  else
	      setBackgroundColorDirect( colorGroup().brightText() );
	break;
      case PaletteBase:
	  if ( colorGroup().fillBase().pixmap() )
	      setBackgroundPixmapDirect( *colorGroup().fillBase().pixmap() );
	  else
	      setBackgroundColorDirect( colorGroup().base() );
	break;
      case PaletteBackground:
	  if ( colorGroup().fillBackground().pixmap() )
	      setBackgroundPixmapDirect( *colorGroup().fillBackground().pixmap() );
	  else
	      setBackgroundColorDirect( colorGroup().background() );
	break;
      case PaletteShadow:
	  if ( colorGroup().fillShadow().pixmap() )
	      setBackgroundPixmapDirect( *colorGroup().fillShadow().pixmap() );
	  else
	      setBackgroundColorDirect( colorGroup().shadow() );
	break;
      case PaletteHighlight:
	  if ( colorGroup().fillHighlight().pixmap() )
	      setBackgroundPixmapDirect( *colorGroup().fillHighlight().pixmap() );
	  else
	      setBackgroundColorDirect( colorGroup().highlight() );
	break;
      case PaletteHighlightedText:
	  if ( colorGroup().fillHighlightedText().pixmap() )
	      setBackgroundPixmapDirect( *colorGroup().fillHighlightedText().pixmap() );
	  else
	      setBackgroundColorDirect( colorGroup().highlightedText() );
	break;
    }
}

/*!
  Returns the mode most recently set by setBackgroundMode().
  The default is \link QWidget::BackgroundMode PaletteBackground\endlink.
*/
QWidget::BackgroundMode QWidget::backgroundMode() const
{
    return extra ? (BackgroundMode)extra->bg_mode : PaletteBackground;
}

/*!
  Tells the window system which color to clear this widget to when
  sending a paint event.

  In other words, this color is the color of the widget when
  paintEvent() is called.  To minimize flicker, this should be the
  most common color in the widget.

  The following values are valid:

  <ul>
    <li> \c PaletteForeground
	- use palette() . \link QColorGroup::fillForeground() fillForeground()\endlink
    <li> \c PaletteBackground
	- use palette() . \link QColorGroup::fillBackground() fillBackground()\endlink
    <li> \c PaletteButton
	- use palette() . \link QColorGroup::fillButton() fillButton()\endlink
    <li> \c PaletteLight
	- use palette() . \link QColorGroup::fillLight() fillLight()\endlink
    <li> \c PaletteMidlight
	- use palette() . \link QColorGroup::fillMidlight() fillMidlight()\endlink
    <li> \c PaletteDark
	- use palette() . \link QColorGroup::fillDark() fillDark()\endlink
    <li> \c PaletteMid
	- use palette() . \link QColorGroup::fillMid() fillMid()\endlink
    <li> \c PaletteText
	- use palette() . \link QColorGroup::fillText() fillText()\endlink
    <li> \c PaletteBrightText
	- use palette() . \link QColorGroup::fillBrightText() fillBrightText()\endlink
    <li> \c PaletteButtonText
	- use palette() . \link QColorGroup::fillButtonText() fillButtonText()\endlink
    <li> \c PaletteBase
	- use palette() . \link QColorGroup::fillBase() fillBase()\endlink
    <li> \c PaletteShadow
	- use palette() . \link QColorGroup::fillShadow() fillShadow()\endlink
    <li> \c PaletteHighlight
	- use palette() . \link QColorGroup::fillHighlight() fillHighlight()\endlink
    <li> \c PaletteHighlightedText
	- use palette() . \link QColorGroup::fillHighlightedText() fillHighlightedText()\endlink
    <li> \c NoBackground
	- no color or pixmap is used - the paintEvent() must completely
	    cover the drawing area.  This can help avoid flicker.
  </ul>

  The fill functions of the colorgroup returns \link QBrush
  Brushes\endlink, which may be either a plain color or a pixmap.

  If setBackgroundPixmap() or setBackgroundColor() is called, the
  mode will be one of:
  <ul>
    <li> \c FixedPixmap - the pixmap set by setBackgroundPixmap()
    <li> \c FixedColor - the color set by setBackgroundColor()
  </ul>

  These values may not be used as parameters to setBackgroundMode().
  \define QWidget::BackgroundMode
  For most widgets the default (PaletteBackground, normally
  gray) suffices, but some need to use PaletteBase (the
  background color for text output, normally white) and a few need
  other colors.

  QListBox, which is "sunken" and uses the base color to contrast with
  its environment, does this:

  \code
    setBackgroundMode( PaletteBase );
  \endcode

  If you want to change the color scheme of a widget, the setPalette()
  function is better suited.  Here is how to set \e thatWidget to use a
  light green (RGB value 80, 255, 80) as background color, with shades
  of green used for all the 3D effects:

  \code
    thatWidget->setPalette( QPalette( QColor(80, 255, 80) ) );
  \endcode

  You can also use QApplication::setPalette() if you want to change
  the color scheme of your entire application, or of all new widgets.
*/

void QWidget::setBackgroundMode( BackgroundMode m )
{
    if ( m==NoBackground )
	setBackgroundEmpty();
    else if ( m==FixedColor || m==FixedPixmap ) {
	warning("May not pass FixedColor or FixedPixmap to setBackgroundMode()");
	return;
    }
    setBackgroundModeDirect(m);
}

/*!
  \internal
*/
void QWidget::setBackgroundModeDirect( BackgroundMode m )
{
    if (m==PaletteBackground && !extra) return;

    createExtra();
    if (extra->bg_mode != m) {
	extra->bg_mode = m;
	setBackgroundFromMode();
    }
}


/*!
  \fn const QColor &QWidget::backgroundColor() const

  Returns the background color of this widget.

  The background color is independent of the color group.

  Setting a new palette overwrites the background color.

  \sa setBackgroundColor(), foregroundColor(), colorGroup(), palette()
*/

/*!
  Returns the foreground color of this widget.

  The foreground color equals <code>colorGroup().foreground()</code>.

  \sa backgroundColor(), colorGroup()
*/

const QColor &QWidget::foregroundColor() const
{
    return colorGroup().foreground();
}


/*!
  \fn void QWidget::backgroundColorChange( const QColor &oldBackgroundColor )

  This virtual function is called from setBackgroundColor().
  \e oldBackgroundColor is the previous background color; you can get the new
  background color from backgroundColor().

  Reimplement this function if your widget needs to know when its
  background color changes.  You will almost certainly need to update the
  widget using either repaint(TRUE) or update().

  The default implementation calls update().

  \sa setBackgroundColor(), backgroundColor(), setPalette(), repaint(),
  update()
*/

void QWidget::backgroundColorChange( const QColor & )
{
    update();
}


/*!
  Returns the background pixmap, or null if no background pixmap has not
  been set.  If the widget has been made empty, this function will return
  a pixmap which isNull() rather than a null pointer.

  \sa setBackgroundPixmap(), setBackgroundMode()
*/

const QPixmap *QWidget::backgroundPixmap() const
{
    return (extra && extra->bg_pix) ? extra->bg_pix : 0;
}


/*!
  \fn void QWidget::backgroundPixmapChange( const QPixmap & oldBackgroundPixmap )

  This virtual function is called from setBackgroundPixmap().
  \e oldBackgroundPixmap is the previous background pixmap; you can get the
  new background pixmap from backgroundPixmap().

  Reimplement this function if your widget needs to know when its
  background pixmap changes.  You will almost certainly need to update the
  widget using either repaint(TRUE) or update().

  The default implementation calls update().

  \sa setBackgroundPixmap(), backgroundPixmap(), repaint(), update()
*/

void QWidget::backgroundPixmapChange( const QPixmap & )
{
    update();
}


/*!
  Returns the current color group of the widget palette.

  The color group is determined by the state of the widget.

  A disabled widget returns the QPalette::disabled() color group, a
  widget in focus returns the QPalette::active() color group and a
  normal widget returns the QPalette::normal() color group.

  \sa palette(), setPalette()
*/

const QColorGroup &QWidget::colorGroup() const
{
    if ( !testWState(QWS_PaletteSet) ){
	palette();				// initialize palette
    }
    if ( testWState(QWS_Disabled) )
	return pal.disabled();
    else if ( qApp->focus_widget == this && focusPolicy() != NoFocus )
	return pal.active();
    else
	return pal.normal();
}

/*!
  \fn const QPalette &QWidget::palette() const
  Returns the widget palette.

  As long as no special palette has been set, this is either a special
  palette for the widget class, the palette of the parent widget or
  the default application palette.

  \sa setPalette(), colorGroup(), QApplication::palette()
*/

const QPalette &QWidget::palette() const
{
    if ( !testWState(QWS_PaletteSet) ){
	QWidget* that = (QWidget*)this;
	that->pal = *QApplication::palette( that );
	that->setWState(QWS_PaletteSet);
	if (that->pal == *QApplication::palette() && parentWidget() )
	    that->pal = parentWidget()->palette();
    }
    return pal;
}


/*!
  Sets the widget palette to \e p. The widget background color is set to
  <code>colorGroup().background()</code>.

  If \a palettePropagation() is \c AllChildren or \c SamePalette,
  setPalette() calls setPalette() for children of the object, or those
  with whom the object shares the palette, respectively.  The default
  for QWidget is \a NoChildren, so setPalette() will not change the children's
  palettes.

  \sa QApplication::setPalette(), palette(), paletteChange(),
  colorGroup(), setBackgroundColor(), setPalettePropagation()
*/

void QWidget::setPalette( const QPalette &p )
{
    QPalette old = palette();
    pal = p;
    setBackgroundFromMode();
    paletteChange( old );
    PropagationMode m = palettePropagation();
    if ( m != NoChildren && children() ) {
	QObjectListIt it( *children() );
	QWidget *w;
	while( (w=(QWidget *)it.current()) != 0 ) {
	    ++it;
	    if ( w->isWidgetType() &&
		 ( m == AllChildren ||
		   old.isCopyOf( w->pal ) ) )
		w->setPalette( pal );
	}
    }
}


/*!
  Like setPalette(const QPalette&) but has an additional flag to
  indicate whether the palette should be fix for the widget. Fixed
  means that QApplication::setPalette() will not touch the current
  setting. This Function calls setPalette(const QPalette&).
*/

void QWidget::setPalette( const QPalette &p, bool fixed )
{
    if ( fixed ) {
	setWState( QWS_PaletteSet|QWS_PaletteFixed );
    } else {
	clearWState( QWS_PaletteFixed );
	setWState( QWS_PaletteSet );
    }
    setPalette( p );
}

/*!
  \fn void QWidget::paletteChange( const QPalette &oldPalette )

  This virtual function is called from setPalette().  \e oldPalette is the
  previous palette; you can get the new palette from palette().

  Reimplement this function if your widget needs to know when its palette
  changes.  You will almost certainly need to update the widget using
  either repaint(TRUE) or update().

  The default implementation calls update().

  \sa setPalette(), palette(), repaint(), update()
*/

void QWidget::paletteChange( const QPalette & )
{
    update();
}


/*!
  \fn const QFont &QWidget::font() const

  Returns the font currently set for the widget.

  fontInfo() tells you what font is actually being used.

  As long as no special font has been set, this is either a special
  font for the widget class, the font of the parent widget or
  the default application font.

  \sa setFont(), fontInfo(), fontMetrics(), QApplication::font()
*/

const QFont &QWidget::font() const
{
    if ( !testWState(QWS_FontSet) ) {
	QWidget* that = (QWidget*)this;
	that->fnt = *QApplication::font( that );
	that->setWState( QWS_FontSet );
	if (that->fnt == *QApplication::font() && that->parentWidget() )
	    that->fnt = that->parentWidget()->font();
    }
	
    return fnt;
}



/*!
  Sets the font for the widget.

  The fontInfo() function reports the actual font that is being used by the
  widget.

  This code fragment sets a 12 point helvetica bold font:
  \code
    QFont f("Helvetica", 12, QFont::Bold);
    setFont( f );
  \endcode

  If \a fontPropagation() is \c AllChildren or \c SameFont, setFont()
  calls setFont() for children of the object, or those with whom the
  object shares the font, respectively.  The default for QWidget
  is \a NoChildren, so setFont() will not change the children's fonts.

  \sa font(), fontChange(), fontInfo(), fontMetrics(), setFontPropagation()
*/

void QWidget::setFont( const QFont &font )
{
    QFont old = QWidget::font();
    fnt = font;
    fnt.handle();				// force load font
    if ( !testWState(QWS_FontSet) )
	setWState(QWS_FontSet);			// indicate initialized
    fontChange( old );
    PropagationMode m = fontPropagation();
    if ( m != NoChildren && children() ) {
	QObjectListIt it( *children() );
	QWidget *w;
	while( (w=(QWidget *)it.current()) != 0 ) {
	    ++it;
	    if ( w->isWidgetType() &&
		 ( m == AllChildren ||
		   old.isCopyOf( w->fnt ) ) )
		 w->setFont( fnt );
	}
    }
}

/*!
  Like setFont(const QFont&) but has an additional flag to
  indicate whether the font should be fix for the widget. Fixed
  means that QApplication::setFont() will not touch the current
  setting. This Function calls setFont(const QFont&).
*/

void QWidget::setFont( const QFont &font, bool fixed )
{
    if ( fixed ) {
	setWState( QWS_FontSet|QWS_FontFixed );
    } else {
	clearWState( QWS_FontFixed );
	setWState( QWS_FontSet );
    }
    setFont( font );
}

/*!
  \fn void QWidget::fontChange( const QFont &oldFont )

  This virtual function is called from setFont().  \e oldFont is the
  previous font; you can get the new font from font().

  Reimplement this function if your widget needs to know when its font
  changes.  You will almost certainly need to update the widget using
  either repaint(TRUE) or update().

  The default implementation calls update().

  \sa setFont(), font(), repaint(), update()
*/

void QWidget::fontChange( const QFont & )
{
    update();
}


/*!
  \fn QFontMetrics QWidget::fontMetrics() const

  Returns the font metrics for the widget's current font.
  Equivalent to QFontMetrics(widget->font()).

  \sa font(), fontInfo(), setFont()
*/

/*!
  \fn QFontInfo QWidget::fontInfo() const

  Returns the font info for the widget's current font.
  Equivalent to QFontInto(widget->font()).

  \sa font(), fontMetrics(), setFont()
*/


/*!
  Returns the widget cursor. If no cursor has been set the parent
  widget's cursor is returned.
  \sa setCursor(), unsetCursor();
*/

const QCursor &QWidget::cursor() const
{
    if ( testWState(QWS_OwnCursor) )
	return (extra && extra->curs)
	    ? *extra->curs
	    : arrowCursor;
    else
	return isTopLevel() ? arrowCursor : parentWidget()->cursor();
}


/*!
  Returns the widget caption, or null if no caption has been set.
  \sa setCaption(), icon(), iconText()
*/

QString QWidget::caption() const
{
    return extra && extra->topextra
	? extra->topextra->caption
	: QString::null;
}

/*!
  Returns the widget icon pixmap, or null if no icon has been set.
  \sa setIcon(), iconText(), caption()
*/

const QPixmap *QWidget::icon() const
{
    return extra && extra->topextra
	? extra->topextra->icon
	: 0;
}

/*!
  Returns the widget icon text, or null if no icon text has been set.
  \sa setIconText(), icon(), caption()
*/

QString QWidget::iconText() const
{
    return extra && extra->topextra
	? extra->topextra->iconText
	: QString::null;
}


/*!
  \fn bool QWidget::hasMouseTracking() const
  Returns TRUE if mouse tracking is enabled for this widget, or FALSE
  if mouse tracking is disabled.
  \sa setMouseTracking()
*/

/*!
  \fn void QWidget::setMouseTracking( bool enable )
  Enables mouse tracking if \e enable is TRUE, or disables it if \e enable
  is FALSE.

  If mouse tracking is disabled (default), this widget only receives
  mouse move events when at least one mouse button is pressed down while
  the mouse is being moved.

  If mouse tracking is enabled, this widget receives mouse move events
  even if no buttons are pressed down.

  \sa hasMouseTracking(), mouseMoveEvent(),
    QApplication::setGlobalMouseTracking()
*/

#if !defined(_WS_X11_)
void QWidget::setMouseTracking( bool enable )
{
    if ( enable )
	setWState( QWS_MouseTracking );
    else
	clearWState( QWS_MouseTracking );
    return;
}
#endif // _WS_X11_


/*!  Sets this widget's focus proxy to \a w.

  Some widgets, such as QComboBox, can "have focus," but create a
  child widget to actually handle the focus.  QComboBox, for example,
  creates a QLineEdit.

  setFocusProxy() sets the widget which will actually get focus when
  "this widget" gets it.  If there is a focus proxy, focusPolicy(),
  setFocusPolicy(), setFocus() and hasFocus() all operate on the focus
  proxy.
*/

void QWidget::setFocusProxy( QWidget * w )
{
    if ( !w && !extra )
	return;

    createExtra();

    if ( extra->focus_proxy ) {
	disconnect( extra->focus_proxy, SIGNAL(destroyed()),
		    this, SLOT(focusProxyDestroyed()) );
	extra->focus_proxy = 0;
    }

    if ( w ) {
	w->setFocusPolicy( focusPolicy() );
	setFocusPolicy( NoFocus );
	extra->focus_proxy = w;
	connect( extra->focus_proxy, SIGNAL(destroyed()),
		 this, SLOT(focusProxyDestroyed()) );
    } else {
	QWidget * pfc = extra->focus_proxy;
	extra->focus_proxy = 0;
	if ( pfc )
	    setFocusPolicy( pfc->focusPolicy() );
    }
}


/*!  Returns a pointer to the focus proxy, or 0 if there is no focus
  proxy.
  \sa setFocusProxy()
*/

QWidget * QWidget::focusProxy() const
{
    return extra ? extra->focus_proxy : 0; // ### watch out for deletes
}


/*!  Internal slot used to clean up if the focus proxy is destroyed.
  \sa setFocusProxy()
*/

void QWidget::focusProxyDestroyed()
{
    if ( extra )
	extra->focus_proxy = 0;
    setFocusPolicy( NoFocus );
}


/*!
  Returns TRUE if this widget (or its focus proxy) has the keyboard
  input focus, otherwise FALSE.

  Equivalent to <code>qApp->focusWidget() == this</code>.

  \sa setFocus(), clearFocus(), setFocusPolicy(), QApplication::focusWidget()
*/

bool QWidget::hasFocus() const
{
    const QWidget* w = this;
    while ( w->focusProxy() )
	w = w->focusProxy();
    return qApp->focusWidget() == w;
}

/*!
  Gives the keyboard input focus to the widget (or its focus proxy).

  First, a \link focusOutEvent() focus out event\endlink is sent to the
  focus widget (if any) to tell it that it is about to loose the
  focus. Then a \link focusInEvent() focus in event\endlink is sent to
  this widget to tell it that it just received the focus.

  setFocus() gives focus to a widget regardless of its focus policy.
  However, QWidget::focusWidget() (which determines where
  Tab/shift-Tab) moves from) is changed only if the widget accepts
  focus.  This can be used to implement "hidden focus"; see
  focusNextPrevChild() for details.

  \warning If you call setFocus() in a function which may itself be
  called from focusOutEvent() or focusInEvent(), you may see infinite
  recursion.

  \sa hasFocus(), clearFocus(), focusInEvent(), focusOutEvent(),
  setFocusPolicy(), QApplication::focusWidget()
*/

void QWidget::setFocus()
{
    if ( !isEnabled() )
	return;

    if ( focusProxy() ) {
	focusProxy()->setFocus();
	return;
    }

    QFocusData * f = focusData(TRUE);
    if ( f->it.current() == this && qApp->focusWidget() == this )
	return;

    if ( isFocusEnabled() ) {
	if ( focusPolicy() == TabFocus || focusPolicy() == StrongFocus ) {
	    // move the tab focus pointer only if this widget can be
	    // tabbed to or from.
	    f->it.toFirst();
	    while ( f->it.current() != this && !f->it.atLast() )
		++f->it;
	    // at this point, the iterator should point to 'this'.  if it
	    // does not, 'this' must not be in the list - an error, but
	    // perhaps possible.  fix it.
	    if ( f->it.current() != this ) {
		f->focusWidgets.append( this );
		f->it.toLast();
	    }
	}
    }

    if ( isActiveWindow() ) {
	QWidget * prev = qApp->focus_widget;
	qApp->focus_widget = this;
	if ( prev && prev != this ) {
	
	    if (!prev->isPopup() && topLevelWidget()->isPopup()
		&& topLevelWidget() != prev->topLevelWidget()) {
		// imitate other toolkits by not sending a focus out event in that case
	    }
	    else {
		QFocusEvent out( QEvent::FocusOut );
		QApplication::sendEvent( prev, &out );
	    }
	}

	QFocusEvent in( QEvent::FocusIn );
	QApplication::sendEvent( this, &in );
    }
}


/*!
  Takes keyboard input focus from the widget.

  If the widget has active focus, a \link focusOutEvent() focus out
  event\endlink is sent to this widget to tell it that it is about to
  loose the focus.

  This widget must enable focus setting in order to get the keyboard input
  focus, i.e. it must call setFocusPolicy().

  \sa hasFocus(), setFocus(), focusInEvent(), focusOutEvent(),
  setFocusPolicy(), QApplication::focusWidget()
*/

void QWidget::clearFocus()
{
    if ( focusProxy() ) {
	focusProxy()->clearFocus();
	return;
    } else {
	QWidget* w = qApp->focusWidget();
	if ( w && w->focusWidget() == this ) {
	    // clear active focus
	    qApp->focus_widget = 0;
	    QFocusEvent out( QEvent::FocusOut );
	    QApplication::sendEvent( w, &out );
	}
    }
}


/*!
  Finds a new widget to give the keyboard focus to, as appropriate for
  Tab/shift-Tab, and returns TRUE if is can find a new widget and
  FALSE if it can't,

  If \a next is true, this function searches "forwards", if \a next is
  FALSE, "backwards".

  Sometimes, you will want to reimplement this function.  For example,
  a web browser might reimplement it to move its "current active link"
  forwards or backwards, and call QWidget::focusNextPrevChild() only
  when it reaches the last/first.

  Child widgets call focusNextPrevChild() on their parent widgets, and
  only the top-level widget will thus make the choice of where to redirect
  focus.  By overriding this method for an object, you thus gain control
  of focus traversal for all child widgets.

  \sa focusData()
*/

bool QWidget::focusNextPrevChild( bool next )
{
    QWidget* p = parentWidget();
    if ( !testWFlags(WType_TopLevel) && p )
	return p->focusNextPrevChild(next);

    QFocusData *f = focusData( TRUE );

    QWidget *startingPoint = f->it.current();
    QWidget *candidate = 0;
    QWidget *w = next ? f->focusWidgets.last() : f->focusWidgets.first();

    do {
	if ( w && w != startingPoint &&
	     (w->focusPolicy() == TabFocus || w->focusPolicy() == StrongFocus)
	     && !w->focusProxy() && w->isVisibleToTLW() && w->isEnabledToTLW())
	    candidate = w;
	w = next ? f->focusWidgets.prev() : f->focusWidgets.next();
    } while( w && !(candidate && w==startingPoint) );

    if ( !candidate )
	return FALSE;

    candidate->setFocus();
    return TRUE;
}

/*!
  Returns the focus widget in this widget's window.  This
  is not the same as QApplication::focusWidget(), which returns the
  focus widget in the currently active window.
*/

QWidget *QWidget::focusWidget() const
{
    QWidget *that = (QWidget *)this;		// mutable
    QFocusData *f = that->focusData( FALSE );
    if ( f && f->focusWidgets.count() && f->it.current() == 0 )
	f->it.toFirst();
    return f ? f->it.current() : 0;
}


/*!
  Returns a pointer to the focus data for this widget's top-level
  widget.

  Focus data always belongs to the top-level widget.  The focus data
  list contains all the widgets in this top-level widget that can
  accept focus, in tab order.  An iterator points to the current focus
  widget (focusWidget() returns a pointer to this widget).

  This information is useful for implementing advanced versions
  of focusNextPrevChild().
*/
QFocusData * QWidget::focusData()
{
    return focusData(TRUE);
}

/*!
  Internal function which lets us not create it too.
*/
QFocusData * QWidget::focusData( bool create )
{
    QWidget * tlw = topLevelWidget();
    QWExtra * ed = tlw->extraData();
    if ( !ed || !ed->topextra ) {
	if ( !create )
	    return 0;
	tlw->createTLExtra();
	ed = tlw->extraData();
    }
    if ( create && !ed->topextra->focusData ) {
	ed->topextra->focusData = new QFocusData;
    }
    return ed->topextra->focusData;
}

/*!
  \fn void QWidget::setSizeGrip(bool sizegrip)

  Informs the underlying window system that this widget is a size grip
  (if sizegrip is TRUE). An example is the nifty decoration in the
  bottom right corner of a QStatusBar.

  This function does yet nothing under Windows. Under X11, the window
  manager has to support the QT_SIZEGRIP protocol.
*/

/*!
  Enables key event compression. Per default, the compression is
  turned off.  If you enable it, the widget receives compressed key
  press events whenever the user types too fast for your program. That
  means you may receive an entire word in the QKeyEvent::text() field
  of they QKeyEvent, instead of one event for each character. This
  makes sense for a word processor, for example, since it takes almost
  as much time to insert a single character as it takes to insert a
  full word, because the required recalculation of the layout of the
  paragraph is roughly the same.

  If a widgets supports multiple byte unicode input, it is always safe
  (and also recommended!) to turn the compression on.

  \sa QKeyEvent::text();
*/

void QWidget::setKeyCompression(bool compress)
{
    if ( compress )
	setWState( QWS_CompressKeys );
    else
	clearWState( QWS_CompressKeys );
}


/*!
  Moves the \a second widget around the ring of focus widgets
  so that keyboard focus moves from \a first widget to \a second
  widget when Tab is pressed.

  Note that since the tab order of the \e second widget is changed,
  you should order a chain like this:

  \code
    setTabOrder(a, b ); // a to b
    setTabOrder(b, c ); // a to b to c
    setTabOrder(c, d ); // a to b to c to d
  \endcode

  not like this:

  \code
    setTabOrder(c, d); // c to d
    setTabOrder(a, b); // a to b AND c to d
    setTabOrder(b, c); // a to b to c, but not c to d
  \endcode

  If either \a first or \a second has a focus proxy, setTabOrder()
  substitutes its/their proxies.

  \sa setFocusPolicy(), setFocusProxy()
*/
void QWidget::setTabOrder( QWidget* first, QWidget *second )
{
    if ( !first || !second )
	return;

    while ( first->focusProxy() )
	first = first->focusProxy();
    while ( second->focusProxy() )
	second = second->focusProxy();

    QFocusData *f = first->focusData( TRUE );
    bool focusThere = (f->it.current() == second );
    f->focusWidgets.removeRef( second );
    if ( f->focusWidgets.findRef( first ) >= 0 )
	f->focusWidgets.insert( f->focusWidgets.at() + 1, second );
    else
	f->focusWidgets.append( second );
    if ( focusThere ) { // reset iterator so tab will work appropriately
	f->it.toFirst();
	while( f->it.current() && f->it.current() != second )
	    ++f->it;
    }
}

/*!
  Moves the relevant widgets from the this window's tab chain to
  that of \a parent, if there's anything to move and we're really
  moving

  \sa reparent()
*/

void QWidget::reparentFocusWidgets( QWidget * parent )
{
    if ( focusData() &&
	 ( parent == 0 ? parentObj != 0
		       : parent->topLevelWidget() != topLevelWidget() ) )
    {
	QFocusData * from = focusData();
	from->focusWidgets.first();
	QFocusData * to;
	if ( parent ) {
	    to = parent->focusData( TRUE );
	} else {
	    // ################ NOT CURRECT - need focusdata of a TLW!
	    createTLExtra();
	    to = extra->topextra->focusData = new QFocusData;
	}

	do {
	    QWidget * pw = from->focusWidgets.current();
	    while( pw && pw != this )
		pw = pw->parentWidget();
	    if ( pw == this ) {
		QWidget * w = from->focusWidgets.take();
		if ( w == from->it.current() )
		    // probably best to clear keyboard focus, or
		    // the user might become rather confused
		    w->clearFocus();
		to->focusWidgets.append( w );
	    } else {
		from->focusWidgets.next();
	    }
	} while( from->focusWidgets.current() );

	if ( parentObj == 0 ) {
	    // this widget is no longer a top-level widget, so get rid
	    // of old focus data
	    delete extra->topextra->focusData;
	    extra->topextra->focusData = 0;
	}
    }
}

/*!
  \fn void recreate( QWidget *parent, WFlags f, const QPoint & p, bool showIt )

  Obsolete.  This method is provided to aide porting to Qt 2.0.

  The function recreate is renamed to reparent in Qt 2.0.
*/

/*!
  Returns the size of the window system frame (for top level widgets).
*/
QSize QWidget::frameSize() const
{
    return extra && extra->topextra
	? extra->topextra->fsize
	: crect.size();
}

/*!
  \internal
  Sets the frame rectangle and recomputes the client rectangle.

  The frame rectangle is the geometry of this widget including any
  decorative borders, in its parent's coordinate system.

  The client rectangle is the geometry of just this widget in its
  parent's coordinate system.
*/

void QWidget::setFRect( const QRect &r )
{
    if ( extra && extra->topextra ) {
	QRect frect = frameGeometry();
	crect.setLeft( crect.left() + r.left() - frect.left() );
	crect.setTop( crect.top() + r.top() - frect.top() );
	crect.setRight( crect.right() + r.right() - frect.right() );
	crect.setBottom( crect.bottom() + r.bottom() - frect.bottom() );
	fpos = r.topLeft();
	extra->topextra->fsize = r.size();
    } else {
	// One rect is both the same.
	fpos = r.topLeft();
	crect = r;
    }
}

/*!
  \internal
  Sets the client rectangle and recomputes the frame rectangle.

  The client rectangle is the geometry of just this widget in its
  parent's coordinate system.

  The frame rectangle is the geometry of this widget including any
  decorative borders, in its parent's coordinate system.
*/

void QWidget::setCRect( const QRect &r )
{
    if ( extra && extra->topextra ) {
	QRect frect = frameGeometry();
	frect.setLeft( frect.left() + r.left() - crect.left() );
	frect.setTop( frect.top() + r.top() - crect.top() );
	frect.setRight( frect.right() + r.right() - crect.right() );
	frect.setBottom( frect.bottom() + r.bottom() - crect.bottom() );
	fpos = frect.topLeft();
	extra->topextra->fsize = frect.size();
    } else {
	// One rect is both the same.
	fpos = r.topLeft();
    }
    crect = r;
}


/*!
  \overload void QWidget::move( const QPoint & )
*/

/*!
  Moves the widget to the position \e (x,y) relative to the parent widget.

  A \link moveEvent() move event\endlink is generated immediately if
  the widget is visible. If the widget is invisible, the move event
  is generated when show() is called.

  This function is virtual, and all other overloaded move()
  implementations call it.

  \warning If you call move() or setGeometry() from moveEvent(), you
  may see infinite recursion.

  \sa pos(), resize(), setGeometry(), moveEvent()
*/

void QWidget::move( int x, int y )
{
    internalSetGeometry( x, y, width(), height(), TRUE );
}



/*!
  \overload void QWidget::resize( const QSize & )
*/

/*!
  Resizes the widget to size \e w by \e h pixels.

  A \link resizeEvent() resize event\endlink is generated immediately if
  the widget is visible. If the widget is invisible, the resize event
  is generated when show() is called.

  The size is adjusted if it is outside the \link setMinimumSize()
  minimum\endlink or \link setMaximumSize() maximum\endlink widget size.

  This function is virtual, and all other overloaded resize()
  implementations call it.

  \warning If you call resize() or setGeometry() from resizeEvent(),
  you may see infinite recursion.

  \sa size(), move(), setGeometry(), resizeEvent(),
  minimumSize(),  maximumSize()
*/
void QWidget::resize( int w, int h )
{
    internalSetGeometry( x(), y(), w, h, FALSE );
}


/*!
  \overload void QWidget::setGeometry( const QRect & )
*/

/*!
  Sets the widget geometry to \e w by \e h, positioned at \e x,y in its
  parent widget.

  A \link resizeEvent() resize event\endlink and a \link moveEvent() move
  event\endlink are generated immediately if the widget is visible. If the
  widget is invisible, the events are generated when show() is called.

  The size is adjusted if it is outside the \link setMinimumSize()
  minimum\endlink or \link setMaximumSize() maximum\endlink widget size.

  This function is virtual, and all other overloaded setGeometry()
  implementations call it.

  \warning If you call setGeometry() from resizeEvent() or moveEvent(),
  you may see infinite recursion.

  \sa geometry(), move(), resize(), moveEvent(), resizeEvent(),
  minimumSize(), maximumSize()
*/

void QWidget::setGeometry( int x, int y, int w, int h )
{
    internalSetGeometry( x, y, w, h, TRUE );
}


/*!
  \fn bool QWidget::isFocusEnabled() const

  Returns TRUE if the widget accepts keyboard focus, or FALSE if it does
  not.

  Keyboard focus is initially disabled (i.e. focusPolicy() ==
  \c QWidget::NoFocus).

  You must enable keyboard focus for a widget if it processes keyboard
  events. This is normally done from the widget's constructor.  For
  instance, the QLineEdit constructor calls
  setFocusPolicy(\c QWidget::StrongFocus).

  \sa setFocusPolicy(), focusInEvent(), focusOutEvent(), keyPressEvent(),
  keyReleaseEvent(), isEnabled()
*/

/*!
  \fn QWidget::FocusPolicy QWidget::focusPolicy() const

  Returns \c QWidget::TabFocus if the widget accepts focus by tabbing, \c
  QWidget::ClickFocus if the widget accepts focus by clicking, \c
  QWidget::StrongFocus if it accepts both and \c QWidget::NoFocus if it
  does not accept focus at all.

  \sa isFocusEnabled(), setFocusPolicy(), focusInEvent(), focusOutEvent(),
  keyPressEvent(), keyReleaseEvent(), isEnabled()
*/

/*!
  Enables or disables the keyboard focus for the widget.

  The keyboard focus is initially disabled (i.e. \e policy ==
  \c QWidget::NoFocus).

  You must enable keyboard focus for a widget if it processes keyboard
  events. This is normally done from the widget's constructor.  For
  instance, the QLineEdit constructor calls
  setFocusPolicy(\c QWidget::StrongFocus).

  The \e policy can be:
  <ul>
  <li> \c QWidget::TabFocus, the widget accepts focus by tabbing.
  <li> \c QWidget::ClickFocus, the widget accepts focus by clicking.
  <li> \c QWidget::StrongFocus, the widget accepts focus by both tabbing
  and clicking.
  <li> \c QWidget::NoFocus, the widget does not accept focus
  </ul>

  As a special case to support applications not utilizing focus,
  \link isTopLevel() Top-level widgets \endlink that have
  NoFocus policy will receive focus events and
  gain keyboard events.

  \sa isFocusEnabled(), focusInEvent(), focusOutEvent(), keyPressEvent(),
  keyReleaseEvent(), isEnabled()
*/

void QWidget::setFocusPolicy( FocusPolicy policy )
{
    if ( focusProxy() )
	focusProxy()->setFocusPolicy( policy );
    if ( policy ) {
	QFocusData * f = focusData( TRUE );
	if ( f->focusWidgets.findRef( this ) < 0 )
 	    f->focusWidgets.append( this );
    }
    focus_policy = (uint)policy;
}


/*!
  \fn bool QWidget::isUpdatesEnabled() const
  Returns TRUE if updates are enabled, otherwise FALSE.
  \sa setUpdatesEnabled()
*/

/*!
  Enables widget updates if \e enable is TRUE, or disables widget updates
  if \e enable is FALSE.

  Calling update() and repaint() has no effect if updates are disabled.
  Paint events from the window system are processed as normally even if
  updates are disabled.

  This function is normally used to disable updates for a short period of
  time, for instance to avoid screen flicker during large changes.

  Example:
  \code
    setUpdatesEnabled( FALSE );
    bigVisualChanges();
    setUpdatesEnabled( TRUE );
    repaint();
  \endcode

  \sa isUpdatesEnabled(), update(), repaint(), paintEvent()
*/

void QWidget::setUpdatesEnabled( bool enable )
{
    if ( enable )
	clearWState( QWS_BlockUpdates );
    else
	setWState( QWS_BlockUpdates );
}


/*
  Returns TRUE if there's no visible top level window (except the desktop).
  This is an internal function used by QWidget::hide().
*/

static bool noVisibleTLW()
{
    QWidgetList *list   = qApp->topLevelWidgets();
    QWidget     *widget = list->first();
    while ( widget ) {
	if ( widget->isVisible() && !widget->isDesktop() )
	    break;
	widget = list->next();
    }
    delete list;
    return widget == 0;
}


void qt_enter_modal( QWidget * );		// defined in qapp_xxx.cpp
void qt_leave_modal( QWidget * );		// --- "" ---
bool qt_modal_state();				// --- "" ---


/*!
  Shows the widget and its child widgets.

  If its size or position has changed, Qt guarantees that a widget gets
  move and resize events just before the widget is shown.

  You almost never have to reimplement this function. If you need to
  change some settings before a widget is shown, use showEvent()
  instead. If you need to do some delayed initialization use polish().

  \sa showEvent, hide(), showMinimized(), isVisible(), polish()
*/

void QWidget::show()
{
    if ( testWState(QWS_Visible) )
	return;
    if ( extra ) {
	int w = crect.width();
	int h = crect.height();
	if ( w < extra->minw || h < extra->minh ||
	     w > extra->maxw || h > extra->maxh ) {
	    w = QMAX( extra->minw, QMIN( w, extra->maxw ));
	    h = QMAX( extra->minh, QMIN( h, extra->maxh ));
	    resize( w, h );			// deferred resize
	}
    }
    QApplication::sendPostedEvents( this, QEvent::ChildInserted );
    if ( parentWidget() )
      QApplication::sendPostedEvents( parentWidget(), QEvent::ChildInserted );
    QApplication::sendPostedEvents( this, QEvent::Move );
    QApplication::sendPostedEvents( this, QEvent::Resize );
    if ( children() ) {
	QObjectListIt it(*children());
	register QObject *object;
	QWidget *widget;
	while ( it ) {				// show all widget children
	    object = it.current();		//   (except popups and other toplevels)
	    ++it;
	    if ( object->isWidgetType() ) {
		widget = (QWidget*)object;
		if ( !widget->testWState(QWS_ForceHide) && !widget->isTopLevel() )
		    widget->show();
	    }
	}
    }
    if ( testWFlags(WStyle_Tool) ) {
	raise();
    } else if ( testWFlags(WType_TopLevel) && !isPopup() ) {
	while ( QApplication::activePopupWidget() )
	    QApplication::activePopupWidget()->hide();
    }

    if ( !testWState(QWS_Polished) ) {
	polish();
	setWState(QWS_Polished);
	setBackgroundFromMode();
    }

    if ( testWFlags(WType_Modal) ) {
	// qt_enter_modal *before* show, otherwise the initial
	// stacking might be wrong
	qt_enter_modal( this );
	showWindow();
    }
    else {
	showWindow();
	if ( testWFlags(WType_Popup) )
	    qApp->openPopup( this );
    }
}


/*!
  Hides the widget.

  The QApplication::lastWindowClosed() signal is emitted when the last
  visible top level widget is hidden,

  You almost never have to reimplement this function. If you need to
  do something after a widget is hidden, use \link hideEvent()
  instead.

  \sa \hideEvent(), show(), showMinimized(), isVisible()
*/

void QWidget::hide()
{
    setWState( QWS_ForceHide );
    if ( !testWState(QWS_Visible) )
	return;

    if ( testWFlags(WType_Modal) )
	qt_leave_modal( this );
    else if ( testWFlags(WType_Popup) )
	qApp->closePopup( this );

    hideWindow();

    clearWState( QWS_Visible );

    // next bit tries to move the focus if the focus widget is now
    // hidden.
    if ( qApp && qApp->focusWidget() == this )
	focusNextPrevChild( TRUE );

    if ( isTopLevel() ) {			// last TLW hidden?
	if ( qApp->receivers(SIGNAL(lastWindowClosed())) && noVisibleTLW() )
	    emit qApp->lastWindowClosed();
    }

    QHideEvent e(FALSE);
    QApplication::sendEvent( this, &e );
}


/*!
  Delayed initialization of a widget.

  This function will be called \e after a widget has been fully created
  just \e before it is shown the very first time.

  Polishing is useful for final initialization depending on an
  instantiated widget. This is something a constructor cannot
  guarantee since the initialization of the subclasses might not be
  finished.

  The default implementation calls QApplication::polish() and some
   internal stuff Warwick is currently hacking on.

  \sa QApplication::polish()
*/

void QWidget::polish()
{
    if ( !parentObj )
	qApp->noteTopLevel(this);
    qApp->polish( this );
}


/*!
  Closes this widget. Returns TRUE if the widget was closed, otherwise
  FALSE.

  First it sends the widget a QCloseEvent. The widget is \link hide()
  hidden\endlink if it \link QCloseEvent::accept() accepts\endlink the
  close event. The default implementation of QWidget::closeEvent()
  accepts the close event.

  If \e forceKill is TRUE, the widget is deleted whether it accepts
  the close event or not.

  The application is \link QApplication::quit() terminated\endlink when
  the \link QApplication::setMainWidget() main widget\endlink is closed.

  The QApplication::lastWindowClosed() signal is emitted when the last
  visible top level widget is closed.

  \sa closeEvent(), QCloseEvent, hide(), QApplication::quit(),
  QApplication::setMainWidget()
*/

bool QWidget::close( bool forceKill )
{
    WId	 id	= winId();
    bool isMain = qApp->mainWidget() == this;
    QCloseEvent e;
    bool accept = QApplication::sendEvent( this, &e );
    if ( !QWidget::find(id) ) {			// widget was deleted
	if ( isMain )
	    qApp->quit();
	return TRUE;
    }
    if ( forceKill )
	accept = TRUE;
    if ( accept ) {
	hide();
	if ( isMain )
	    qApp->quit();
	else if ( forceKill || testWFlags(WDestructiveClose) )
	    delete this;
    }
    return accept;
}


/*!
  \fn bool QWidget::close()
  Closes the widget.

  This version of close is usable as a slot. It calls close( FALSE )

  \sa close(bool)
*/

/*!
  \fn bool QWidget::isVisible() const

  Returns TRUE if the widget itself is set to visible status, or else
  FALSE.  Calling show() sets the widget to visible status; calling
  hide() sets it to hidden status. Iconified top-level widgets also
  have hidden status.

  If a widget is set to visible status, but its parent widget is set
  to hidden status, this function returns TRUE.  isVisibleToTLW()
  looks at the visibility status of the parent widgets up to the
  top level widget.

  This function returns TRUE if the widget currently is obscured by
  other windows on the screen, but would be visible if moved.

  \sa show(), hide(), isVisibleToTLW()
*/


/*!
  Returns TRUE if this widget and every parent up to but excluding
  \a ancestor is visible, otherwise returns FALSE.

  This function returns TRUE if the widget it is obscured by other
  windows on the screen, but would be visible if moved.

  \sa show() hide() isVisible()
*/

bool QWidget::isVisibleTo(QWidget* ancestor) const
{
    const QWidget * w = this;
    while ( w
	    && w->isVisible()
	    && !w->isTopLevel()
	    && w->parentWidget()
	    && w->parentWidget()!=ancestor )
	w = w->parentWidget();
    return w->isVisible();
}


/*!
  Returns TRUE if this widget and every parent up to the \link
  topLevelWidget() top level widget \endlink is visible, otherwise
  returns FALSE.

  This function returns TRUE if the widget it is obscured by other
  windows on the screen, but would be visible if moved.

  This is equivalent to isVisibleTo(0).

  \sa show(), hide(), isVisible()
*/

bool QWidget::isVisibleToTLW() const
{
    return isVisibleTo( 0 );
}


/*!
  Adjusts the size of the widget to fit the contents.

  Uses sizeHint() if valid (i.e if the size hint's width and height are
  equal to or greater than 0), otherwise sets the size to the children
  rectangle (the union of all child widget geometries).

  \sa sizeHint(), childrenRect()
*/

void QWidget::adjustSize()
{
    QSize s = sizeHint();
    if ( s.isValid() ) {
	resize( s );
	return;
    }
    QRect r = childrenRect();			// get children rectangle
    if ( r.isNull() )				// probably no widgets
	return;
    resize( r.width()+2*r.x(), r.height()+2*r.y() );
}


/*!
  Returns a recommended size for the widget, or an invalid size if
  no size is recommended.

  The default implementation returns an invalid size if there is no layout
  for this widget, the layout's preferred size otherwise.

  \sa QSize::isValid(), resize(), setMinimumSize()
*/

QSize QWidget::sizeHint() const
{
    if ( layout() )
	return layout()->sizeHint();
    return QSize( -1, -1 );
}

/*!
  \fn QWidget *QWidget::parentWidget() const
  Returns a pointer to the parent of this widget, or a null pointer if
  it does not have any parent widget.
*/

/*!
  \fn bool QWidget::testWFlags( WFlags n ) const

  Returns non-zero if any of the widget flags in \e n are set. The
  widget flags are listed in qwindowdefs.h, and are strictly for
  internal use.

  \internal

  Widget state flags:
  <dl compact>
  <dt>QWS_Created<dd> The widget has a valid winId().
  <dt>QWS_Disabled<dd> Disables mouse and keyboard events.
  <dt>QWS_Visible<dd> show() has been called.
  <dt>QWS_ForceHide<dd> hide() has been called before first show().
  <dt>QWS_OwnCursor<dd> A cursor has been set for this widget.
  <dt>QWS_MouseTracking<dd> Mouse tracking is enabled.
  <dt>QWS_CompressKeys<dd> Compress keyboard events.
  <dt>QWS_BlockUpdates<dd> Repaints and updates are disabled.
  <dt>QWS_InPaintEvent<dd> Currently processing a paint event.
  <dt>QWS_Reparented<dd> The widget has been reparented.
  <dt>QWS_ConfigPending<dd> A config (resize/move) event is pending.
  <dt>QWS_Resized<dd> The widget has been resized.
  <dt>QWS_AutoMask<dd> The widget has an automatic mask, see setAutoMask().
  <dt>QWS_Polished<dd> The widget has an auomatic mask, see setAutoMask().
  <dt>QWS_DND<dd> The widget supports drag and drop, see setAcceptDrops().
  <dt>QWS_USPositionX<dd> X11 only: Set the USPosition size hint.
  <dt>QWS_PaletteSet<dd> The palette has been set.
  <dt>QWS_PaletteFixed<dd> The widget has a fixed palette.
  <dt>QWS_FontSet<dd> The font has been set.
  <dt>QWS_FontFixed<dd> The widget has a fixed font.
  </dl>

  Widget type flags:
  <dl compact>
  <dt>WType_TopLevel<dd> Top-level widget (not a child).
  <dt>WType_Modal<dd> Modal widget, implies \c WType_TopLevel.
  <dt>WType_Popup<dd> Popup widget, implies \c WType_TopLevel.
  <dt>WType_Desktop<dd> Desktop widget (root window), implies
	\c WType_TopLevel.
  </dl>

  Window style flags (for top-level widgets):
  <dl compact>
  <dt>WStyle_Customize<dd> Custom window style.
  <dt>WStyle_NormalBorder<dd> Normal window border.
  <dt>WStyle_DialogBorder<dd> Thin dialog border.
  <dt>WStyle_NoBorder<dd> No window border.
  <dt>WStyle_Title<dd> The window has a title.
  <dt>WStyle_SysMenu<dd> The window has a system menu
  <dt>WStyle_Minimize<dd> The window has a minimize box.
  <dt>WStyle_Maximize<dd> The window has a maximize box.
  <dt>WStyle_MinMax<dd> Equals (\c WStyle_Minimize | \c WStyle_Maximize).
  <dt>WStyle_Tool<dd> The window is a tool window.
  </dl>

  Misc. flags:
  <dl compact>
  <dt>WDestructiveClose<dd> The widget is deleted when its closed.
  <dt>WPaintDesktop<dd> The widget wants desktop paint events.
  <dt>WPaintUnclipped<dd> Paint without clipping child widgets.
  <dt>WPaintClever<dd> The widget wants every update rectangle.
  <dt>WResizeNoErase<dd> Widget resizing should not erase the widget.
			This allows smart-repainting to avoid flicker.
  <dt>WMouseNoMask<dd> Even if the widget has a mask, mouse events
			are delivered for the entire rectangle.
  </dl>
*/


/*****************************************************************************
  QWidget event handling
 *****************************************************************************/


/*!
  This is the main event handler. You may reimplement this function
  in a subclass, but we recommend using one of the specialized event
  handlers instead.

  The main event handler first passes an event through all \link
  QObject::installEventFilter() event filters\endlink that have been
  installed.  If none of the filters intercept the event, it calls one
  of the specialized event handlers.

  Key press/release events are treated differently from other events.
  event() checks for Tab and shift-Tab and tries to move the focus
  appropriately.  If there is no widget to move the focus to (or the
  key press is not Tab or shift-Tab), event() calls keyPressEvent().

  This function returns TRUE if it is able to pass the event over to
  someone, or FALSE if nobody wanted the event.

  \sa closeEvent(), focusInEvent(), focusOutEvent(), enterEvent(),
  keyPressEvent(), keyReleaseEvent(), leaveEvent(),
  mouseDoubleClickEvent(), mouseMoveEvent(), mousePressEvent(),
  mouseReleaseEvent(), moveEvent(), paintEvent(),
  resizeEvent(), QObject::event(), QObject::timerEvent()
*/

bool QWidget::event( QEvent *e )
{
    if ( eventFilters ) {			// try filters
	if ( activate_filters(e) )		// stopped by a filter
	    return TRUE;
    }

    switch ( e->type() ) {

	case QEvent::Timer:
	    timerEvent( (QTimerEvent*)e );
	    break;

	case QEvent::MouseMove:
	    mouseMoveEvent( (QMouseEvent*)e );
	    break;

	case QEvent::MouseButtonPress:
	    mousePressEvent( (QMouseEvent*)e );
	    break;

	case QEvent::MouseButtonRelease:
	    mouseReleaseEvent( (QMouseEvent*)e );
	    break;

	case QEvent::MouseButtonDblClick:
	    mouseDoubleClickEvent( (QMouseEvent*)e );
	    break;

	case QEvent::Wheel:
	    wheelEvent( (QWheelEvent*)e );
	    if ( ! ((QWheelEvent*)e)->isAccepted() )
		return FALSE;
	    break;
	case QEvent::KeyPress: {
	    QKeyEvent *k = (QKeyEvent *)e;
	    bool res = FALSE;
	    if ( k->key() == Key_Backtab ||
		 (k->key() == Key_Tab &&
		  (k->state() & ShiftButton)) )
		res = focusNextPrevChild( FALSE );
	    else if ( k->key() == Key_Tab )
		res = focusNextPrevChild( TRUE );
	    if ( res )
		break;
	    QWidget *w = this;
	    while ( w ) {
		w->keyPressEvent( k );
		if ( k->isAccepted() || w->isTopLevel() )
		    break;
		w = w->parentWidget();
		k->accept();
	    }
	    }
	    break;

	case QEvent::KeyRelease: {
	    QKeyEvent *k = (QKeyEvent *)e;
	    QWidget *w = this;
	    while ( w ) {
		k->accept();
		w->keyReleaseEvent( k );
		if ( k->isAccepted() || w->isTopLevel() )
		    break;
		w = w->parentWidget();
	    }
	    }
	    break;

	case QEvent::FocusIn:
	    focusInEvent( (QFocusEvent*)e );
	    break;

	case QEvent::FocusOut:
	    focusOutEvent( (QFocusEvent*)e );
	    break;

	case QEvent::Enter:
	    enterEvent( e );
	    break;

	case QEvent::Leave:
	     leaveEvent( e );
	    break;

	case QEvent::Paint:
	    paintEvent( (QPaintEvent*)e );
	    break;

	case QEvent::Move:
	    moveEvent( (QMoveEvent*)e );
	    break;

	case QEvent::Resize:
	    resizeEvent( (QResizeEvent*)e );
	    break;

	case QEvent::Close: {
	    QCloseEvent *c = (QCloseEvent *)e;
	    closeEvent( c );
	    if ( !c->isAccepted() )
		return FALSE;
	    }
	    break;
	case QEvent::Drop:
	    dropEvent( (QDropEvent*) e);
	    break;	
	case QEvent::DragEnter:
	    dragEnterEvent( (QDragEnterEvent*) e);
	    break;	
	case QEvent::DragMove:
	    dragMoveEvent( (QDragMoveEvent*) e);
	    break;	
	case QEvent::DragLeave:
	    dragLeaveEvent( (QDragLeaveEvent*) e);
	    break;	
	case QEvent::Show:
	    showEvent( (QShowEvent*) e);
	    break;
	case QEvent::Hide:
	    hideEvent( (QHideEvent*) e);
	    break;
	case QEvent::ChildInserted:
	case QEvent::ChildRemoved:
	    childEvent( (QChildEvent*) e);
	    break;
	default:
	    return FALSE;
    }
    return TRUE;
}


/*!
  This event handler can be reimplemented in a subclass to receive
  mouse move events for the widget.

  If mouse tracking is switched off, mouse move events only occur if a
  mouse button is down while the mouse is being moved.	If mouse
  tracking is switched on, mouse move events occur even if no mouse
  button is down.

  QMouseEvent::pos() reports the position of the mouse cursor, relative to
  this widget.  For press and release events, the position is usually
  the same as the position of the last mouse move event, but it might be
  different if the user moves and clicks the mouse fast.  This is
  a feature of the underlying window system, not Qt.

  The default implementation does nothing.

  \sa setMouseTracking(), mousePressEvent(), mouseReleaseEvent(),
  mouseDoubleClickEvent(), event(), QMouseEvent
*/

void QWidget::mouseMoveEvent( QMouseEvent * )
{
}

/*!
  This event handler can be reimplemented in a subclass to receive
  mouse press events for the widget.

  If you create new widgets in the mousePressEvent() the
  mouseReleaseEvent() may not end up where you expect, depending on the
  underlying window system (or X11 window manager), the widgets'
  location and maybe more.

  The default implementation implements the closing of popup widgets
  when you click outside the window. For other widget types it does
  nothing.

  \sa mouseReleaseEvent(), mouseDoubleClickEvent(),
  mouseMoveEvent(), event(),  QMouseEvent
*/

void QWidget::mousePressEvent( QMouseEvent *e )
{
    if ( isPopup() ) {
	QWidget* w;
	while ( (w = qApp->activePopupWidget() ) && w != this ){
	    w->close();
	    if (qApp->activePopupWidget() == w) // widget does not want to dissappear
		w->close(TRUE); // no chance
	}
	if (!rect().contains(e->pos()) ){	
	    close();
	}
    }
}

/*!
  This event handler can be reimplemented in a subclass to receive
  mouse release events for the widget.

  The default implementation does nothing.

  \sa mouseReleaseEvent(), mouseDoubleClickEvent(),
  mouseMoveEvent(), event(),  QMouseEvent
*/

void QWidget::mouseReleaseEvent( QMouseEvent * )
{
}

/*!
  This event handler can be reimplemented in a subclass to receive
  mouse double click events for the widget.

  The default implementation generates a normal mouse press event.

  Note that the widgets gets a mousePressEvent() and a mouseReleaseEvent()
  before the mouseDoubleClickEvent().

  \sa mousePressEvent(), mouseReleaseEvent()
  mouseMoveEvent(), event(),  QMouseEvent
*/

void QWidget::mouseDoubleClickEvent( QMouseEvent *e )
{
    mousePressEvent( e );			// try mouse press event
}


/*!
  This event handler can be reimplemented in a subclass to receive
  wheel events for the widget.

  If you reimplement this handler, it is very important that you \link
  QWheelEvent ignore()\endlink the event if you do not handle it, so
  that the widget's parent can interpret it.

  The default implementation ignores the event.

  \sa QWheelEvent::ignore(), QWheelEvent::accept(),
  event(), QWheelEvent
*/

void QWidget::wheelEvent( QWheelEvent *e )
{
    e->ignore();
}


/*!
  This event handler can be reimplemented in a subclass to receive
  key press events for the widget.

  A widget must \link setFocusPolicy() accept focus\endlink initially
  and \link hasFocus() have focus\endlink in order to receive a key press
  event.

  If you reimplement this handler, it is very important that you \link
  QKeyEvent ignore()\endlink the press if you do not understand it, so
  that the widget's parent can interpret it.

  The default implementation closes popup widgets if you hit
  escape. Otherwise the event is ignored.

  As a special case to support applications not utilizing focus,
  \link isTopLevel() Top-level widgets \endlink that have
  NoFocus policy will receive keyboard events.

  \sa keyReleaseEvent(), QKeyEvent::ignore(), setFocusPolicy(),
  focusInEvent(), focusOutEvent(), event(), QKeyEvent
*/

void QWidget::keyPressEvent( QKeyEvent *e )
{
    if ( isPopup() && e->key() == Key_Escape ) {
	e->accept();
	close();
    } else {
	e->ignore();
    }
}

/*!
  This event handler can be reimplemented in a subclass to receive
  key release events for the widget.

  A widget must \link setFocusPolicy() accept focus\endlink initially
  and \link hasFocus() have focus\endlink in order to receive a key
  release event.

  If you reimplement this handler, it is very important that you \link
  QKeyEvent ignore()\endlink the release if you do not understand it,
  so that the widget's parent can interpret it.

  The default implementation ignores the event.

  \sa keyPressEvent(), QKeyEvent::ignore(), setFocusPolicy(),
  focusInEvent(), focusOutEvent(), event(), QKeyEvent
*/

void QWidget::keyReleaseEvent( QKeyEvent *e )
{
    e->ignore();
}

/*!
  This event handler can be reimplemented in a subclass to receive
  keyboard focus events (focus received) for the widget.

  A widget must \link setFocusPolicy() accept focus\endlink initially in
  order to receive focus events.

  The default implementation calls repaint() since the widget's \link
  QColorGroup color group\endlink changes from normal to active.  You
  may want to call repaint(FALSE) to reduce flicker in any reimplementation.

  As a special case to support applications not utilizing focus,
  \link isTopLevel() Top-level widgets \endlink that have
  \link focusPolicy() NoFocus policy\endlink will receive focus events and
  gain keyboard events, but the repaint is not done by default.

  \sa focusOutEvent(), setFocusPolicy(),
  keyPressEvent(), keyReleaseEvent(), event(), QFocusEvent
*/

void QWidget::focusInEvent( QFocusEvent * )
{
    if ( focusPolicy() != NoFocus || !isTopLevel() ) {
	repaint();
	if ( testWState(QWS_AutoMask) )
	    updateMask();
    }
}

/*!
  This event handler can be reimplemented in a subclass to receive
  keyboard focus events (focus lost) for the widget.

  A widget must \link setFocusPolicy() accept focus\endlink initially in
  order to receive focus events.

  The default implementation calls repaint() since the widget's \link
  QColorGroup color group\endlink changes from active to normal.  You
  may want to call repaint(FALSE) to reduce flicker in any reimplementation.

  \sa focusInEvent(), setFocusPolicy(),
  keyPressEvent(), keyReleaseEvent(), event(), QFocusEvent
*/

void QWidget::focusOutEvent( QFocusEvent * )
{
    if ( focusPolicy() != NoFocus || !isTopLevel() ){
	repaint();
	if ( testWState(QWS_AutoMask) )
	    updateMask();
    }
}

/*!
  This event handler can be reimplemented in a subclass to receive
  widget enter events.

  An event is sent to the widget when the mouse cursor enters the widget.

  The default implementation does nothing.

  \sa leaveEvent(), mouseMoveEvent(), event()
*/

void QWidget::enterEvent( QEvent * )
{
}

/*!
  This event handler can be reimplemented in a subclass to receive
  widget leave events.

  A leave event is sent to the widget when the mouse cursor leaves
  the widget.

  The default implementation does nothing.

  \sa enterEvent(), mouseMoveEvent(), event()
*/

void QWidget::leaveEvent( QEvent * )
{
}

/*!
  This event handler can be reimplemented in a subclass to receive
  widget paint events.	Actually, it more or less \e must be
  reimplemented.

  The default implementation does nothing.

  When the paint event occurs, the update rectangle QPaintEvent::rect()
  normally has been cleared to the background color or pixmap. An
  exception is repaint() with erase=FALSE.

  For many widgets it is sufficient to redraw the entire widget each time,
  but some need to consider the update rectangle to avoid flicker or slow
  update.

  Pixmaps can also be used to implement flicker-free update.

  update() and repaint() can be used to force a paint event.

  \sa event(), repaint(), update(), QPainter, QPixmap, QPaintEvent
*/

void QWidget::paintEvent( QPaintEvent * )
{
}


/*!
  This event handler can be reimplemented in a subclass to receive
  widget move events.  When the widget receives this event, it is
  already at the new position.

  The old position is accessible through QMoveEvent::oldPos().

  The default implementation does nothing.

  \sa resizeEvent(), event(), move(), QMoveEvent
*/

void QWidget::moveEvent( QMoveEvent * )
{
}


/*!
  This event handler can be reimplemented in a subclass to receive
  widget resize events.	 When resizeEvent() is called, the widget
  already has its new geometry.

  If the widget was created with the WResizeNoErase flag, this handler
  is responsible for calling update() for all areas in the \link
  QResizeEvent::oldSize() previous area\endlink of the widget which
  look different with the new size.  For example, for a simple label,
  no areas need to be updated, while for a button with centered text,
  the area covered by the old and new text needs to be updated and
  possibly also the right-edge and bottom-edge of the button frame
  (depending on the actual resize) You should also use
  QPaintEvent::region() rather than QPaintEvent::rect() when you \link
  QPainter::setClipRegion() set clipping\endlink in the paintEvent().

  The old size is accessible through QResizeEvent::oldSize().

  The default implementation calls updateMask() if the widget
  has \link QWidget::setAutoMask() automatic masking\endlink
  enabled.

  \sa moveEvent(), event(), resize(), QResizeEvent
*/

void QWidget::resizeEvent( QResizeEvent * )
{
    if ( testWState(QWS_AutoMask) )
	updateMask();
}

/*!
  This event handler can be reimplemented in a subclass to receive
  widget close events.

  The default implementation calls e->accept(), which hides this widget.
  See the QCloseEvent documentation for more details.

  \sa event(), hide(), close(), QCloseEvent
*/

void QWidget::closeEvent( QCloseEvent *e )
{
    e->accept();
}


/*!
  This event handler is called when a drag is in progress and the
  mouse enters this widget.

  The default implementation does nothing.

  \sa QTextDrag, QImageDrag, QDragEnterEvent
*/
void QWidget::dragEnterEvent( QDragEnterEvent * )
{
}

/*!
  This event handler is called when a drag is in progress and the
  mouse enters this widget, and whenever it moves within
  the widget.

  The default implementation does nothing.

  \sa QTextDrag, QImageDrag, QDragMoveEvent
*/
void QWidget::dragMoveEvent( QDragMoveEvent * )
{
}

/*!
  This event handler is called when a drag is in progress and the
  mouse leaves this widget.

  The default implementation does nothing.

  \sa QTextDrag, QImageDrag, QDragLeaveEvent
*/
void QWidget::dragLeaveEvent( QDragLeaveEvent * )
{
}

/*!
  This event handler is called when the drag is dropped on this
  widget.

  The default implementation does nothing.

  \sa QTextDrag, QImageDrag, QDropEvent
*/
void QWidget::dropEvent( QDropEvent * )
{
}


/*!
  This event handler can be reimplemented in a subclass to receive
  widget show events.

  Non-sponaneous show events are sent to widgets right before they are
  shown. Spontaneous show events of toplevel widgets are delivered
  afterwards, naturally.

  The default implementation does nothing.

  \sa event(), QShowEvent
  */
void QWidget::showEvent( QShowEvent * )
{
}

/*!
  This event handler can be reimplemented in a subclass to receive
  widget hide events.

  Hide events are sent to widgets right after they have been hidden.

  The default implementation does nothing.

  \sa event(), QHideEvent
  */
void QWidget::hideEvent( QHideEvent * )
{
}

/*!
  This event handler can be reimplemented in a subclass to receive
  custom events.

  QCustomEvent is a user-defined event type which contains a \c void*.

  \warning
  This event class is internally used to implement Qt enhancements.  It is
  not advisable to use QCustomEvent in normal applications, where other
  event types and the signal/slot mechanism can do the job.

  The default implementation does nothing.

  \sa event(), QCustomEvent
*/
void QWidget::customEvent( QCustomEvent * )
{
}


#if defined(_WS_MAC_)

/*!
  This special event handler can be reimplemented in a subclass to receive
  native Macintosh events.

  If the event handler returns FALSE, this native event is passed back to
  Qt, which translates the event into a Qt event and sends it to the
  widget.  If the event handler returns TRUE, the event is stopped.

  \warning This function is not portable.

  QApplication::macEventFilter()
*/

bool QWidget::macEvent( MSG * )
{
    return FALSE;
}

#elif defined(_WS_WIN_)

/*!
  This special event handler can be reimplemented in a subclass to receive
  native Windows events.

  If the event handler returns FALSE, this native event is passed back to
  Qt, which translates the event into a Qt event and sends it to the
  widget.  If the event handler returns TRUE, the event is stopped.

  \warning This function is not portable.

  QApplication::winEventFilter()
*/

bool QWidget::winEvent( MSG * )
{
    return FALSE;
}

#elif defined(_WS_X11_)

/*!
  This special event handler can be reimplemented in a subclass to receive
  native X11 events.

  If the event handler returns FALSE, this native event is passed back to
  Qt, which translates the event into a Qt event and sends it to the
  widget.  If the event handler returns TRUE, the event is stopped.

  \warning This function is not portable.

  QApplication::x11EventFilter()
*/

bool QWidget::x11Event( XEvent * )
{
    return FALSE;
}

#endif


/*!
  Returns the font propagation mode of this widget.  The default
  font propagation mode is \c NoChildren, but you can set it to \a
  SameFont or \a AllChildren.

  \sa setFontPropagation()
*/

QWidget::PropagationMode QWidget::fontPropagation() const
{
    return (PropagationMode)propagate_font;
}


/*!
  Sets the font propagation mode to \a m.

  if \a m is \c NoChildren (the default), setFont() does not change
  any children's fonts.  If it is \c SameFont, setFont() changes the
  font of the children that have the exact same font as this widget
  (see \l QFont::isCopyOf() for details).  If it is \c AllChildren,
  setFont() changes the font of all children.

  \sa fontPropagation() setFont() setPalettePropagation()
*/

void QWidget::setFontPropagation( PropagationMode m )
{
    propagate_font = (int)m;
}


/*!  Returns the palette propagation mode of this widget.  The default
  palette propagation mode is \c NoChildren, but you can set it to \a
  SamePalette or \a AllChildren.

  \sa setPalettePropagation()
*/

QWidget::PropagationMode QWidget::palettePropagation() const
{
    return (PropagationMode)propagate_palette;
}


/*!  Sets the palette propagation mode to \a m.

  if \a m is \c NoChildren (the default), setPalette() does not change
  any children's palettes.  If it is \c SamePalette, setPalette()
  changes the palette of the children that have the exact same palette
  as this widget (see \l QPalette::isCopyOf() for details).  If it is
  \c AllChildren, setPalette() changes the palette of all children.

  \sa palettePropagation() setPalette() setFontPropagation()
*/

void QWidget::setPalettePropagation( PropagationMode m )
{
    propagate_palette = (int)m;
}

/*!
  Transparent widgets use a \link setMask() mask \endlink to define
  their visible region. QWidget has some built-in support to make the
  task of recalculating the mask easier. When setting auto mask to
  TRUE, updateMask() will be called whenever the widget is resized or
  changes its focus state.

  Note: When you re-implement resizeEvent(), focusInEvent() or
  focusOutEvent() in your custom widgets and still want to ensure that
  the auto mask calculation works, you will have to add

    \code
    if ( autoMask() )
          updateMask();
    \endcode

    at the end of your event handlers. Same holds for all member
    functions that change the appearance of the widget in a way that a
    recalculation of the mask is necessary.

  \sa autoMask(), updateMask(), setMask(), clearMask()
*/

void QWidget::setAutoMask( bool enable )
{
    if ( enable ) {
	setWState(QWS_AutoMask);
	updateMask();
    } else {
	clearWState(QWS_AutoMask);
	clearMask();
    }
}

/*!
  Returns whether or not a widget has the auto mask feature enabled.

  \sa setAutoMask(), updateMask(), setMask(), clearMask()
*/

bool QWidget::autoMask() const
{
    return testWState(QWS_AutoMask);
}

/*!
  This function can be reimplemented in a subclass to support
  transparent widgets. It is supposed to be called whenever a widget
  changes state in a way that the shape mask has to be recalculated.

  The default implementation does nothing.

  \sa setAutoMask(), updateMask(), setMask(), clearMask()
  */
void QWidget::updateMask()
{
}


/*!
  \fn QLayout* QWidget::layout () const

  Returns a pointer to the layout engine that manages the geometry of
  this widget's children.

  If the widget does not have a layout, layout() returns a null pointer.

  \sa setLayout() sizePolicy()
*/


/*!  Sets this widget to use \a l to manage the geometry of its
  children.

  If there already was a layout for this widget, the old layout is
  forgotten.  (Note that it is not deleted.)

  \sa layout() QLayout sizePolicy()
*/

void QWidget::setLayout( QLayout *l )
{
    lay_out = l;
}


/*!  This function can be reimplemented in subclasses to specify the
  default layout behaviour of that subclass.

  The default implementation returns a value which means <ul>

  <li> If there is a QLayout that manages this widget's children, the
  size policy specified by that layout is used.

  <li> If there is no such QLayout, the widget can be freely resized,
  but prefers to be the size sizeHint() returns.

  </ul>

  \sa sizeHint() QLayout QSizePolicy
*/

QSizePolicy QWidget::sizePolicy() const
{
    return QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
}
