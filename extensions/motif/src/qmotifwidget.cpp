/****************************************************************************
**
** Implementation of Qt extension classes for Xt/Motif support.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the Qt extension for Xt/Motif support.
** EDITIONS: ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <qapplication.h>
#include <qevent.h>
#include <qwidget.h>
#include <private/qwidget_p.h>

#include <qgc_x11.h>

#include "qmotifwidget.h"
#include "qmotif.h"

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <X11/Xatom.h>

const int XFocusOut = FocusOut;
const int XFocusIn = FocusIn;
#undef FocusOut
#undef FocusIn

const int XKeyPress = KeyPress;
const int XKeyRelease = KeyRelease;
#undef KeyPress
#undef KeyRelease

static void qmotif_widget_shell_destroy(Widget w);
void qmotif_widget_shell_realize( Widget w, XtValueMask *mask,
				  XSetWindowAttributes *attr );
void qmotif_widget_shell_change_managed( Widget w );

// TopLevelShell subclass to wrap toplevel motif widgets into QWidgets

typedef struct {
    QMotifWidget *widget;
} QTopLevelShellPart;

typedef struct _QTopLevelShellRec
{
    // full instance record declaration
    CorePart			core;
    CompositePart		composite;
    ShellPart			shell;
    WMShellPart			wmshell;
    VendorShellPart		vendorshell;
    TopLevelShellPart		toplevelshell;
    QTopLevelShellPart		qtoplevelshell;
} QTopLevelShellRec;

typedef struct
{
    // extension record
    XtPointer extension;
} QTopLevelShellClassPart;

typedef struct _QTopLevelShellClassRec {
    CoreClassPart		core_class;
    CompositeClassPart		composite_class;
    ShellClassPart		shell_class;
    WMShellClassPart		wmshell_class;
    VendorShellClassPart	vendorshell_class;
    TopLevelShellClassPart	toplevelshell_class;
    QTopLevelShellClassPart	qtoplevelshell_class;
} QTopLevelShellClassRec;

externalref QTopLevelShellClassRec 	qtoplevelShellClassRec;
externalref WidgetClass 		qtoplevelShellWidgetClass;
typedef struct _QTopLevelShellClassRec	*QTopLevelShellWidgetClass;
typedef struct _QTopLevelShellRec 	*QTopLevelShellWidget;

externaldef(qtoplevelshellclassrec)
    QTopLevelShellClassRec qtoplevelShellClassRec = {
	// core
	{
	    (WidgetClass) &topLevelShellClassRec,	// superclass
	    "QTopLevelShell",			// class name
	    sizeof(QTopLevelShellRec),		// widget size
	    NULL,					/* class_initialize proc */
	    NULL,					/* class_part_initialize proc */
	    FALSE,					/* class_inited flag */
	    NULL,					/* instance initialize proc */
	    NULL,					/* init_hook proc */
	    qmotif_widget_shell_realize,		/* realize widget proc */
	    NULL,					/* action table for class */
	    0,						/* num_actions */
	    NULL,					/* resource list of class */
	    0,						/* num_resources in list */
	    NULLQUARK,					/* xrm_class ? */
	    FALSE,					/* don't compress_motion */
	    XtExposeCompressSeries,			/* compressed exposure */
	    FALSE,					/* do compress enter-leave */
	    FALSE,					/* do have visible_interest */
	    NULL,					/* destroy widget proc */
	    XtInheritResize,				/* resize widget proc */
	    NULL,					/* expose proc */
	    NULL,					/* set_values proc */
	    NULL,					/* set_values_hook proc */
	    XtInheritSetValuesAlmost,			/* set_values_almost proc */
	    NULL,					/* get_values_hook */
	    NULL,					/* accept_focus proc */
	    XtVersion,					/* current version */
	    NULL,					/* callback offset    */
	    XtInheritTranslations,			/* default translation table */
	    XtInheritQueryGeometry,			/* query geometry widget proc */
	    NULL,					/* display accelerator	  */
	    NULL					/* extension record	 */
	},
	// composite
	{
	    XtInheritGeometryManager,			/* geometry_manager */
	    qmotif_widget_shell_change_managed,		// change managed
	    XtInheritInsertChild,			// insert_child
	    XtInheritDeleteChild,			// delete_child
	    NULL					// extension record
	},
	// shell extension record
	{ NULL },
	// wmshell extension record
	{ NULL },
	// vendorshell extension record
	{ NULL },
	// toplevelshell extension record
	{ NULL },
	// qtoplevelshell extension record
	{ NULL }
    };

externaldef(qtoplevelshellwidgetclass)
    WidgetClass qtoplevelShellWidgetClass = (WidgetClass)&qtoplevelShellClassRec;

// ApplicationShell subclass to wrap toplevel motif widgets into QWidgets

typedef struct {
    QMotifWidget *widget;
} QApplicationShellPart;

typedef struct _QApplicationShellRec
{
    // full instance record declaration
    CorePart			core;
    CompositePart		composite;
    ShellPart			shell;
    WMShellPart			wmshell;
    VendorShellPart		vendorshell;
    TopLevelShellPart		toplevelshell;
    ApplicationShellPart   	applicationshell;
    QApplicationShellPart	qapplicationshell;
} QApplicationShellRec;

typedef struct
{
    // extension record
    XtPointer extension;
} QApplicationShellClassPart;

typedef struct _QApplicationShellClassRec {
    CoreClassPart		core_class;
    CompositeClassPart		composite_class;
    ShellClassPart		shell_class;
    WMShellClassPart		wmshell_class;
    VendorShellClassPart	vendorshell_class;
    TopLevelShellClassPart	toplevelshell_class;
    ApplicationShellClassPart   applicationshell_class;
    QApplicationShellClassPart	qapplicationshell_class;
} QApplicationShellClassRec;

externalref QApplicationShellClassRec		qapplicationShellClassRec;
externalref WidgetClass				qapplicationShellWidgetClass;
typedef struct _QApplicationShellClassRec	*QApplicationShellWidgetClass;
typedef struct _QApplicationShellRec		*QApplicationShellWidget;

externaldef(qapplicationshellclassrec)
    QApplicationShellClassRec qapplicationShellClassRec = {
	// core
	{
	    (WidgetClass) &applicationShellClassRec,	// superclass
	    "QApplicationShell",			// class name
	    sizeof(QApplicationShellRec),		// widget size
	    NULL,					/* class_initialize proc */
	    NULL,					/* class_part_initialize proc */
	    FALSE,					/* class_inited flag */
	    NULL,					/* instance initialize proc */
	    NULL,					/* init_hook proc */
	    qmotif_widget_shell_realize,		/* realize widget proc */
	    NULL,					/* action table for class */
	    0,						/* num_actions */
	    NULL,					/* resource list of class */
	    0,						/* num_resources in list */
	    NULLQUARK,					/* xrm_class ? */
	    FALSE,					/* don't compress_motion */
	    XtExposeCompressSeries,			/* compressed exposure */
	    FALSE,					/* do compress enter-leave */
	    FALSE,					/* do have visible_interest */
	    qmotif_widget_shell_destroy,		/* destroy widget proc */
	    XtInheritResize,				/* resize widget proc */
	    NULL,					/* expose proc */
	    NULL,					/* set_values proc */
	    NULL,					/* set_values_hook proc */
	    XtInheritSetValuesAlmost,			/* set_values_almost proc */
	    NULL,					/* get_values_hook */
	    NULL,					/* accept_focus proc */
	    XtVersion,					/* current version */
	    NULL,					/* callback offset    */
	    XtInheritTranslations,			/* default translation table */
	    XtInheritQueryGeometry,			/* query geometry widget proc */
	    NULL,					/* display accelerator	  */
	    NULL					/* extension record	 */
	},
	// composite
	{
	    XtInheritGeometryManager,			/* geometry_manager */
	    qmotif_widget_shell_change_managed,		// change managed
	    XtInheritInsertChild,			// insert_child
	    XtInheritDeleteChild,			// delete_child
	    NULL					// extension record
	},
	// shell extension record
	{ NULL },
	// wmshell extension record
	{ NULL },
	// vendorshell extension record
	{ NULL },
	// toplevelshell extension record
	{ NULL },
	// applicationshell extension record
	{ NULL },
	// qapplicationshell extension record
	{ NULL }
    };

externaldef(qapplicationshellwidgetclass)
    WidgetClass qapplicationShellWidgetClass = (WidgetClass)&qapplicationShellClassRec;


class QMotifWidgetPrivate
{
public:
    QMotifWidgetPrivate() : widget( NULL ), shell( NULL ) { }

    Widget widget;
    Widget shell;
};

/*!
    \class QMotifWidget
    \brief The QMotifWidget class provides the QWidget API for Xt/Motif widgets.
\if defined(commercial)
    It is part of the <a href="commercialeditions.html">Qt Enterprise Edition</a>.
\endif

    \extension Motif

    QMotifWidget exists to provide a QWidget that can act as a parent
    for any Xt/Motif widget. Since the QMotifWidget is a proper
    QWidget, it can be used as a top-level widget (e.g. 0 parent) or
    as a child of any other QWidget. Note: Since QMotifWidget acts as
    a parent for Xt/Motif widgets, you should not create QWidgets with
    a QMotifWidget parent.

    An Xt/Motif widget with a top-level QMotifWidget parent can begin
    using the standard Qt dialogs and custom QDialogs while keeping
    the main Xt/Motif interface of the application. Using a
    QMotifWidget as the parent for the various QDialogs will ensure
    that modality and stacking works properly throughout the entire
    application.

    Applications moving to Qt may have custom Xt/Motif widgets that
    will take time to rewrite with Qt. Such applications can use
    these custom widgets as QMotifWidget with QWidget parents. This
    allows the application's interface to be replaced gradually.

    \warning QMotifWidget uses the X11 window ID of the Motif widget
    directly, instead of creating its own.  Because ot this,
    QWidget::reparent() will not work.  This includes the functions
    QWidget::showFullScreen() and QWidget::showNormal(), which use
    QWidget::reparent().
*/

/*!
    Creates a QMotifWidget of the given \a widgetclass as a child of
    \a parent, with the name \a name and widget flags \a flags.

    The \a args and \a argcount arguments are passed on to
    XtCreateWidget.

    The motifWidget() function returns the resulting Xt/Motif widget.
    This widget can be used as a parent for any other Xt/Motif widget.

    If \a parent is a QMotifWidget, the Xt/Motif widget is created as
    a child of the parent's motifWidget(). If \ parent is 0 or a
    normal QWidget, the Xt/Motif widget is created as a child of a
    special TopLevelShell widget. Xt/Motif widgets can use this
    special TopLevelShell parent as the parent for existing
    Xt/Motif dialogs or QMotifDialogs.
*/
QMotifWidget::QMotifWidget( QWidget *parent, WidgetClass widgetclass,
			    ArgList args, Cardinal argcount,
			    const char *name, WFlags flags )
    : QWidget( parent, name, flags )
{
    setFocusPolicy( StrongFocus );
    d = new QMotifWidgetPrivate;

    Widget motifparent = NULL;
    if ( parent ) {
	if ( parent->inherits( "QMotifWidget" ) ) {
	    // not really supported, but might be possible
	    motifparent = ( (QMotifWidget *) parent )->motifWidget();
	} else {
	    // keep an eye on the position of the toplevel widget, so that we
	    // can tell our hidden shell its real on-screen position
	    parent->topLevelWidget()->installEventFilter( this );
	}
    }

    bool isShell = widgetclass == applicationShellWidgetClass || widgetclass == topLevelShellWidgetClass;
    if (!motifparent || isShell) {
	ArgList realargs = new Arg[argcount + 3];
	Cardinal nargs = argcount;
	memcpy( realargs, args, sizeof( Arg ) * argcount );

	if ( ! QX11GC::x11AppDefaultVisual() ) {
	    // make Motif use the same visual/colormap/depth as Qt (if
	    // Qt is not using the default)
	    XtSetArg(realargs[nargs], XtNvisual, QX11GC::x11AppVisual());
	    ++nargs;
	    XtSetArg(realargs[nargs], XtNcolormap, QX11GC::x11AppColormap());
	    ++nargs;
	    XtSetArg(realargs[nargs], XtNdepth, QX11GC::x11AppDepth());
	    ++nargs;
	}

	if (widgetclass == applicationShellWidgetClass) {
	    d->shell = XtAppCreateShell( name, name, qapplicationShellWidgetClass,
					 QMotif::x11Display(), realargs, nargs );
	    ( (QApplicationShellWidget) d->shell )->qapplicationshell.widget = this;
	} else {
	    d->shell = XtAppCreateShell( name, name, qtoplevelShellWidgetClass,
					 QMotif::x11Display(), realargs, nargs );
	    ( (QTopLevelShellWidget) d->shell )->qtoplevelshell.widget = this;
	}
	motifparent = d->shell;

	delete [] realargs;
	realargs = 0;
    }

    if (isShell)
	d->widget = d->shell;
    else
	d->widget = XtCreateWidget( name, widgetclass, motifparent, args, argcount );

    QWidgetPrivate *wp = static_cast<QWidgetPrivate*>(QObject::d_ptr);
    if (! wp->extra) {
        // createExtra() is private, so use topData() to ensure that
        // the extra data is created
	wp->createExtra();
    }
    wp->extra->compress_events = FALSE;
}

/*!
    Destroys the QMotifWidget. The special TopLevelShell is also
    destroyed, if it was created during construction.
*/
QMotifWidget::~QMotifWidget()
{
    QMotif::unregisterWidget( this );
    XtDestroyWidget( d->widget );
    if ( d->shell && d->widget != d->shell ) {
	if (XtIsSubclass(d->shell, qapplicationShellWidgetClass)) {
	    ( (QApplicationShellWidget) d->shell )->qapplicationshell.widget = 0;
	} else {
	    Q_ASSERT(XtIsSubclass(d->shell, qtoplevelShellWidgetClass));
	    ( (QTopLevelShellWidget) d->shell )->qtoplevelshell.widget = 0;
	}
	XtDestroyWidget( d->shell );
    }
    delete d;

    // make sure we don't have any pending requests for the window we
    // are about to destroy
    XSync(x11Display(), FALSE);
    destroy( FALSE );
}

/*!
    Returns the embedded Xt/Motif widget. If a Shell widget was
    created by the constructor, you can access it with XtParent().
*/
Widget QMotifWidget::motifWidget() const
{
    return d->widget;
}

/*!
    \reimp

    Manages the embedded Xt/Motif widget and shows the widget.
*/
void QMotifWidget::show()
{
    if ( d->shell ) {
	// since we have a shell, we need to manage it's children
	XtManageChildren( ((CompositeWidget) d->shell)->composite.children,
			  ((CompositeWidget) d->shell)->composite.num_children );
	if ( ! XtIsRealized( d->shell ) )
	    XtRealizeWidget( d->shell );

	XSync(x11Display(), FALSE);
	XSync(QMotif::x11Display(), FALSE);

	XtMapWidget(d->shell);
    }

    QWidget::show();
}

/*!
    \reimp

    Unmanages the embedded Xt/Motif widget and hides the widget.
*/
void QMotifWidget::hide()
{
    if ( d->shell ) {
	// since we have a shell, we need to manage it's children
	XtUnmanageChildren( ((CompositeWidget) d->shell)->composite.children,
			    ((CompositeWidget) d->shell)->composite.num_children );
    }

    QWidget::hide();
}


/*! \internal
    Wraps the Motif widget by setting the X window for the
    QMotifWidget to the X window id of the widget shell.
*/
void QMotifWidget::realize( Widget w )
{
    // use the winid of the dialog shell, reparent any children we
    // have
    if ( XtWindow( w ) != winId() ) {
	XSync(QMotif::x11Display(), FALSE);

	// save the geometry of the motif widget, since it has the
	// geometry we want
	QRect save( w->core.x, w->core.y, w->core.width, w->core.height );

	// save the wi
	QString wtitle = windowTitle();
	if (wtitle.isEmpty()) {
	    char *t;
	    XtVaGetValues(w, XtNtitle, &t, NULL);
	    wtitle = QString::fromLocal8Bit(t);
	}

	Window newid = XtWindow( w );
	QObjectList list = children();
	for (int i = 0; i < list.size(); ++i) {
	    QWidget *widget = qt_cast<QWidget*>(list.at(i));
	    if (!widget) continue;

	    XReparentWindow(x11Display(), widget->winId(), newid,
			    widget->x(), widget->y());
	}
	QApplication::syncX();

	// re-create this QWidget with the winid from the motif
	// widget... the geometry will be reset to roughly 1/4 of the
	// screen, so we need to restore it below
	create( newid, TRUE, TRUE );

	// restore the caption
	if (!wtitle.isEmpty())
	    setWindowTitle(wtitle);

	// restore geometry of the shell
	XMoveResizeWindow( x11Display(), winId(),
			   save.x(), save.y(), save.width(), save.height() );

	// if this QMotifWidget has a parent widget, we should
	// reparent the shell into that parent
	if ( parentWidget() ) {
	    XReparentWindow( x11Display(), winId(),
			     parentWidget()->winId(), x(), y() );
	}
    }
    QMotif::registerWidget( this );
}

/*! \internal
    Motif callback to send a close event to a QMotifWidget
*/
void qmotif_widget_shell_destroy(Widget w)
{
    XtWidgetProc destroy =
	((CoreWidgetClass)topLevelShellClassRec.core_class.
	 superclass)->core_class.destroy;
    (*destroy)(w);

    QMotifWidget *widget = 0;
    if (XtIsSubclass(w, qapplicationShellWidgetClass)) {
	widget = ((QApplicationShellWidget) w)->qapplicationshell.widget;
    } else {
	Q_ASSERT(XtIsSubclass(w, qtoplevelShellWidgetClass));
	widget = ((QTopLevelShellWidget) w)->qtoplevelshell.widget;
    }
    if (!widget)
	return;
    widget->close();
}

/*! \internal
    Motif callback to resolve a QMotifWidget and call
    QMotifWidget::realize().
*/
void qmotif_widget_shell_realize( Widget w, XtValueMask *mask, XSetWindowAttributes *attr )
{
    XtRealizeProc realize =
	((CoreWidgetClass)topLevelShellClassRec.core_class.
	 superclass)->core_class.realize;
    (*realize)( w, mask, attr );

    QMotifWidget *widget = 0;
    if (XtIsSubclass(w, qapplicationShellWidgetClass)) {
	widget = ((QApplicationShellWidget) w)->qapplicationshell.widget;
    } else {
	Q_ASSERT(XtIsSubclass(w, qtoplevelShellWidgetClass));
	widget = ((QTopLevelShellWidget) w)->qtoplevelshell.widget;
    }
    if ( ! widget )
	return;
    widget->realize( w );
}

/*! \internal
    Motif callback to resolve a QMotifWidget and set the initial
    geometry of the widget.
*/
void qmotif_widget_shell_change_managed( Widget w )
{
    XtWidgetProc change_managed =
	((CompositeWidgetClass)topLevelShellClassRec.core_class.
	 superclass)->composite_class.change_managed;
    (*change_managed)( w );

    QMotifWidget *widget = 0;
    if (XtIsSubclass(w, qapplicationShellWidgetClass)) {
	widget = ((QApplicationShellWidget) w)->qapplicationshell.widget;
    } else {
	Q_ASSERT(XtIsSubclass(w, qtoplevelShellWidgetClass));
	widget = ((QTopLevelShellWidget) w)->qtoplevelshell.widget;
    }
    if ( ! widget )
	return;
    QRect r( widget->d->shell->core.x,
	     widget->d->shell->core.y,
	     widget->d->shell->core.width,
	     widget->d->shell->core.height ),
	d = widget->geometry();
    if ( d != r ) {
	// ### perhaps this should be a property that says "the
	// ### initial size of the QMotifWidget should be taken from
	// ### the motif widget, otherwise use the size from the
	// ### parent widget (i.e. we are in a layout)"
	if ((! widget->isTopLevel() && widget->parentWidget() && widget->parentWidget()->layout())
	    || widget->testAttribute(QWidget::WA_Resized)) {
	    // the widget is most likely resized a) by a layout or b) explicitly
	    XtMoveWidget( w, d.x(), d.y() );
	    XtResizeWidget( w, d.width(), d.height(), 0 );
	    widget->setGeometry( d );
	} else {
	    // take the size from the motif widget
	    widget->setGeometry( r );
	}
    }
}

/*!\reimp
 */
bool QMotifWidget::event( QEvent* e )
{
    if ( dispatchQEvent( e, this ) )
	return TRUE;
    return QWidget::event( e );
}

/*!\reimp
 */
bool QMotifWidget::eventFilter( QObject *object, QEvent *event )
{
    if (object != topLevelWidget() || event->type() != QEvent::Move || !d->shell)
	return FALSE;

    // the motif widget is embedded in our special shell, so when the
    // top-level widget moves, we need to inform the special shell
    // about our new position
    QPoint p = topLevelWidget()->geometry().topLeft() +
	       mapTo( topLevelWidget(), QPoint( 0, 0 ) );
    d->shell->core.x = p.x();
    d->shell->core.y = p.y();

    return FALSE;
}

bool QMotifWidget::dispatchQEvent( QEvent* e, QWidget* w)
{
    switch ( e->type() ) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
	if ( QMotif::lastEvent() ) {
	    QMotif::lastEvent()->xany.window = w->winId();
	    QMotif::redeliverEvent( QMotif::lastEvent() );
	}
	break;
    case QEvent::FocusIn:
    {
	XFocusInEvent ev = { XFocusIn, 0, TRUE, QMotif::x11Display(), w->winId(),
			     NotifyNormal, NotifyPointer  };
	QMotif::redeliverEvent( (XEvent*)&ev );
	break;
    }
    case QEvent::FocusOut:
    {
	XFocusOutEvent ev = { XFocusOut, 0, TRUE, QMotif::x11Display(), w->winId(),
			      NotifyNormal, NotifyPointer  };
	QMotif::redeliverEvent( (XEvent*)&ev );
	break;
    }
    default:
	break;
    }
    return FALSE;
}
