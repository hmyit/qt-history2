/****************************************************************************
**
** Implementation of the QAccessibleWidget class.
**
** Copyright (C) 1992-2003 Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qaccessiblewidget.h"

#if defined(QT_ACCESSIBILITY_SUPPORT)

#include "qapplication.h"
#include "qtooltip.h"
#include "qwhatsthis.h"
#include "qwidget.h"

class QAccessibleWidgetPrivate : public QAccessible
{
public:
    QAccessibleWidgetPrivate()
	:role(Client), state(Normal)
    {
    }

    Role role;
    State state;
    QString name;
    QString description;
    QString value;
    QString help;
    QString defAction;
    QString accelerator;
};

/*!
  \class QAccessibleWidget qaccessiblewidget.h
  \brief The QAccessibleWidget class implements the QAccessibleInterface for QWidgets.
*/

/*!
  Creates a QAccessibleWidget object for \a o.
  \a role, \a name, \a description, \a value, \a help, \a defAction,
  \a accelerator and \a state are optional parameters for static values
  of the object's property.
*/
QAccessibleWidget::QAccessibleWidget( QObject *o, Role role, QString name,
    QString description, QString value, QString help, QString defAction, QString accelerator, State state )
    : QAccessibleObject( o )
{
    d = new QAccessibleWidgetPrivate();
    d->role = role;
    d->state = state;
    d->name = name;
    d->description = description;
    d->value = value;
    d->help = help;
    d->defAction = defAction;
    d->accelerator = accelerator;
}

QAccessibleWidget::~QAccessibleWidget()
{
    delete d;
}

/*! Returns the widget. */
QWidget *QAccessibleWidget::widget() const
{
    Q_ASSERT(object()->isWidgetType());
    if ( !object()->isWidgetType() )
	return 0;
    return (QWidget*)object();
}

/*! \reimp */
int QAccessibleWidget::controlAt( int x, int y ) const
{
    QWidget *w = widget();
    QPoint gp = w->mapToGlobal( QPoint( 0, 0 ) );
    if ( !QRect( gp.x(), gp.y(), w->width(), w->height() ).contains( x, y ) )
	return -1;

    QPoint rp = w->mapFromGlobal( QPoint( x, y ) );

    QObjectList list = w->queryList( "QWidget", 0, FALSE, FALSE );

    if ( list.isEmpty() )
	return 0;

    QList<QObject*>::Iterator it = list.begin();
    QWidget *child = 0;
    int index = 1;
    while ( it != list.end() ) {
	child = (QWidget*)*it;
	if ( !child->isTopLevel() && !child->isHidden() && child->geometry().contains( rp ) ) {
	    return index;
	}
	++it;
	++index;
    }
    return index;
}

/*! \reimp */
QRect	QAccessibleWidget::rect( int control ) const
{
#if defined(QT_DEBUG)
    if ( control )
	qWarning( "QAccessibleWidget::rect: This implementation does not support subelements! (ID %d unknown for %s)", control, widget()->className() );
#else
    Q_UNUSED(control)
#endif
    QWidget *w = widget();
    QPoint wpos = w->mapToGlobal( QPoint( 0, 0 ) );

    return QRect( wpos.x(), wpos.y(), w->width(), w->height() );
}

/*! \reimp */
int QAccessibleWidget::navigate( NavDirection dir, int startControl ) const
{
#if defined(QT_DEBUG)
    if ( startControl )
	qWarning( "QAccessibleWidget::navigate: This implementation does not support subelements! (ID %d unknown for %s)", startControl, widget()->className() );
#else
    Q_UNUSED(startControl);
#endif
    QWidget *w = widget();
    switch ( dir ) {
    case NavFirstChild:
	{
	    QObjectList list = w->queryList( "QWidget", 0, FALSE, FALSE );
	    return list.isEmpty() ? -1 : 1;
	}
    case NavLastChild:
	{
	    QObjectList list = w->queryList( "QWidget", 0, FALSE, FALSE );
	    return list.isEmpty() ? -1 : list.count();
	}
    case NavNext:
    case NavPrevious:
	{
	    QAccessibleInterface *parent = 0;
	    queryParent(&parent);
	    if (!parent)
		return -1;

	    int ourIndex = parent->indexOfChild(this);
	    int siblings = parent->childCount();
	    parent->release();
	    if (dir == NavNext) {
		if (ourIndex < siblings)
		    return ourIndex + 1;
	    } else {
		if (ourIndex > 1)
		    return ourIndex - 1;
	    }
	    return -1;
	}
	break;
    case NavFocusChild:
	{
	    if ( w->hasFocus() )
		return 0;

	    QWidget *fw = w->focusWidget();
	    if ( !fw )
		return -1;

	    QObjectList list = w->queryList( "QWidget", 0, FALSE, FALSE );
	    int index = list.indexOf(fw);
	    if (index != -1)
		++index;
	    return index;
	}
    default:
	qWarning( "QAccessibleWidget::navigate: unhandled request" );
	break;
    };
    return -1;
}

/*! \reimp */
int QAccessibleWidget::childCount() const
{
    QObjectList cl = widget()->queryList( "QWidget", 0, FALSE, FALSE );
    return cl.count();
}

/*! \reimp */
int QAccessibleWidget::indexOfChild(const QAccessibleInterface *child) const
{
    QObjectList cl = widget()->queryList( "QWidget", 0, FALSE, FALSE );
    int index = cl.indexOf(child->object());
    if (index != -1)
	++index;
    return index;
}

/*! \reimp */
bool QAccessibleWidget::queryChild( int control, QAccessibleInterface **iface ) const
{
    *iface = 0;
    QObjectList cl = widget()->queryList( "QWidget", 0, FALSE, FALSE );
    if ( cl.isEmpty() )
	return FALSE;

    QObject *o = 0;
    if ( cl.count() >= control )
	o = cl.at( control-1 );

    if ( !o )
	return FALSE;

    return QAccessible::queryAccessibleInterface( o, iface );
}

/*! \reimp */
bool QAccessibleWidget::queryParent( QAccessibleInterface **iface ) const
{
#ifndef Q_WS_WIN32
    if (!widget()->parentWidget(TRUE))
	return QAccessible::queryAccessibleInterface( qApp, iface );
#endif
    return QAccessible::queryAccessibleInterface( widget()->parentWidget(), iface );
}

/*! \reimp */
bool QAccessibleWidget::doDefaultAction( int control )
{
#if defined(QT_DEBUG)
    if ( control )
	qWarning( "QAccessibleWidget::doDefaultAction: This implementation does not support subelements! (ID %d unknown for %s)", control, widget()->className() );
#else
    Q_UNUSED(control)
#endif
    return FALSE;
}

/*! \reimp */
QString QAccessibleWidget::text( Text t, int control ) const
{
    switch ( t ) {
    case DefaultAction:
	return d->defAction;
    case Description:
	if ( !control && d->description.isEmpty() ) {
	    QString desc = QToolTip::textFor(widget());
	    return desc;
	}
	return d->description;
    case Help:
	if ( !control && d->help.isEmpty() ) {
	    QString help = QWhatsThis::textFor( widget() );
	    return help;
	}
	return d->help;
    case Accelerator:
	return d->accelerator;
    case Name:
	{
	    if ( !control && d->name.isEmpty() && widget()->isTopLevel() )
		return widget()->caption();
	    return d->name;
	}
    case Value:
	return d->value;
    default:
	break;
    }
    return QString();
}

/*! \reimp */
void QAccessibleWidget::setText( Text t, int /*control*/, const QString &text )
{
    switch ( t ) {
    case DefaultAction:
	d->defAction = text;
	break;
    case Description:
	d->description = text;
	break;
    case Help:
	d->help = text;
	break;
    case Accelerator:
	d->accelerator = text;
	break;
    case Name:
	d->name = text;
	break;
    case Value:
	d->value = text;
	break;
    default:
	break;
    }
}

/*! \reimp */
QAccessible::Role QAccessibleWidget::role( int control ) const
{
    if ( !control )
	return d->role;
    return NoRole;
}

/*! \reimp */
QAccessible::State QAccessibleWidget::state( int control ) const
{
    if ( control )
	return Normal;

    if ( d->state != Normal )
	return d->state;

    int state = Normal;

    QWidget *w = widget();
    if ( w->isHidden() )
	state |= Invisible;
    if ( w->focusPolicy() != QWidget::NoFocus && w->isActiveWindow() )
	state |= Focusable;
    if ( w->hasFocus() )
	state |= Focused;
    if ( !w->isEnabled() )
	state |= Unavailable;
    if ( w->isTopLevel() ) {
	state |= Moveable;
	if ( w->minimumSize() != w->maximumSize() )
	    state |= Sizeable;
    }

    return (State)state;
}

/*! \reimp */
bool QAccessibleWidget::setFocus( int control )
{
#if defined(QT_DEBUG)
    if ( control )
	qWarning( "QAccessibleWidget::setFocus: This implementation does not support subelements! (ID %d unknown for %s)", control, widget()->className() );
#else
    Q_UNUSED(control)
#endif
    if ( widget()->focusPolicy() != QWidget::NoFocus ) {
	widget()->setFocus();
	return TRUE;
    }
    return FALSE;
}

/*! \reimp */
bool QAccessibleWidget::setSelected( int, bool, bool )
{
#if defined(QT_DEBUG)
    qWarning( "QAccessibleWidget::setSelected: This function not supported for simple widgets." );
#endif
    return FALSE;
}

/*! \reimp */
void QAccessibleWidget::clearSelection()
{
#if defined(QT_DEBUG)
    qWarning( "QAccessibleWidget::clearSelection: This function not supported for simple widgets." );
#endif
}

/*! \reimp */
QMemArray<int> QAccessibleWidget::selection() const
{
    return QMemArray<int>();
}

#endif //QT_ACCESSIBILITY_SUPPORT
