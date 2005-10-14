/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qdesigner_menubar_p.h"
#include "qdesigner_toolbar_p.h"
#include "actionrepository_p.h"
#include "actionprovider_p.h"

#include <QtDesigner/QtDesigner>

#include <QtCore/QTimer>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QMenu>
#include <QtGui/QToolButton>
#include <QtGui/QDrag>
#include <QtGui/qevent.h>

Q_DECLARE_METATYPE(QAction*)
Q_DECLARE_METATYPE(QListWidgetItem*)

using namespace qdesigner_internal;

namespace qdesigner_internal
{

MenuToolBox::MenuToolBox(QDesignerMenuBar *menuBar)
    : QWidget(menuBar)
{
    QHBoxLayout *lay = new QHBoxLayout(this);
    lay->setMargin(0);
    lay->setSpacing(2);;

    m_createMenuButton = new QToolButton(this);
    m_createMenuButton->setObjectName("__qt__passive_create_menu");
    lay->addWidget(m_createMenuButton);
    connect(m_createMenuButton, SIGNAL(pressed()), this, SLOT(slotCreateMenu()));
}

MenuToolBox::~MenuToolBox()
{
}

QDesignerMenuBar *MenuToolBox::menuBar() const
{
    return qobject_cast<QDesignerMenuBar*>(parentWidget());
}

void MenuToolBox::slotCreateMenu()
{
    m_createMenuButton->setDown(false);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(new MenuMimeData()); // ### set a nice pixmap
    drag->start();
}


QDesignerMenuBar::QDesignerMenuBar(QWidget *parent)
    : QMenuBar(parent)
{
    m_blockSentinelChecker = false;
    m_sentinel = 0;

    setContextMenuPolicy(Qt::DefaultContextMenu);

    setAcceptDrops(true); // ### fake

    m_sentinel = new SentinelAction(this);       // ### use a special widget as indicator
    addAction(m_sentinel);

    m_sentinelChecker = new QTimer(this);
    connect(m_sentinelChecker, SIGNAL(timeout()), this, SLOT(slotCheckSentinel()));

    m_toolBox = new MenuToolBox(this);
    setCornerWidget(m_toolBox);

    qApp->installEventFilter(this);
}

QDesignerMenuBar::~QDesignerMenuBar()
{
}

bool QDesignerMenuBar::handleEvent(QWidget *widget, QEvent *event)
{
    if (!formWindow())
        return false;

    switch (event->type()) {
        default: break;

        case QEvent::MouseButtonPress:
            return handleMousePressEvent(widget, static_cast<QMouseEvent*>(event));
        case QEvent::MouseButtonRelease:
            return handleMouseReleaseEvent(widget, static_cast<QMouseEvent*>(event));
        case QEvent::MouseMove:
            return handleMouseMoveEvent(widget, static_cast<QMouseEvent*>(event));
        case QEvent::ContextMenu:
            return handleContextMenuEvent(widget, static_cast<QContextMenuEvent*>(event));
    }

    return true;
}

void QDesignerMenuBar::startDrag(const QPoint &pos)
{
    int index = findAction(pos);
    if (index >= actions().count() - 1)
        return;

    QAction *action = actions().at(index);
    removeAction(action);
    adjustSize();

    adjustIndicator(pos);

    QDrag *drag = new QDrag(this);
    drag->setPixmap(action->icon().pixmap(QSize(22, 22)));

    ActionRepositoryMimeData *data = new ActionRepositoryMimeData();
    data->items.append(action);
    drag->setMimeData(data);

    if (drag->start() == Qt::IgnoreAction) {
        QAction *previous = actions().at(index);
        insertAction(previous, action);
        adjustSize();
    }
}

bool QDesignerMenuBar::handleMousePressEvent(QWidget *, QMouseEvent *event)
{
    m_startPosition = QPoint();
    event->accept();

    if (event->button() != Qt::LeftButton)
        return true;

    if (QDesignerFormWindowInterface *fw = formWindow()) {
        if (QDesignerPropertyEditorInterface *pe = fw->core()->propertyEditor()) {
            pe->setObject(this);
        }
    }

    m_startPosition = mapFromGlobal(event->globalPos());

    int index = findAction(m_startPosition);
    if (index >= actions().count() - 1)
        return false;

    if (QAction *action = actions().at(index)) {
        setActiveAction(action);

        if (QMenu *menu = action->menu()) {
            menu->setActiveAction(0);
        }
    }

    return true;
}

bool QDesignerMenuBar::handleMouseReleaseEvent(QWidget *, QMouseEvent *event)
{
    event->accept();

    m_startPosition = QPoint();

    return false;
}

bool QDesignerMenuBar::handleMouseMoveEvent(QWidget *, QMouseEvent *event)
{
    event->accept();

    if (m_startPosition.isNull())
        return false;

    QPoint pos = mapFromGlobal(event->globalPos());

    if ((pos - m_startPosition).manhattanLength() < qApp->startDragDistance())
        return false;

    startDrag(pos);
    m_startPosition = QPoint();

    return true;
}

bool QDesignerMenuBar::handleContextMenuEvent(QWidget *, QContextMenuEvent *event)
{
    event->accept();

    int index = findAction(mapFromGlobal(event->globalPos()));
    QAction *action = actions().at(index);
    if (action == actions().last())
        return true;

    QMenu menu(0);
    QAction *a = menu.addAction(tr("Remove action '%1'").arg(action->objectName()));
    QVariant itemData;
    qVariantSetValue(itemData, action);
    a->setData(itemData);

    connect(&menu, SIGNAL(triggered(QAction*)), this, SLOT(slotRemoveSelectedAction(QAction*)));
    menu.exec(event->globalPos());

    return true;
}

void QDesignerMenuBar::slotRemoveSelectedAction(QAction *action)
{
    QAction *a = qvariant_cast<QAction*>(action->data());
    Q_ASSERT(a != 0);
    removeAction(a);
}

bool QDesignerMenuBar::eventFilter(QObject *object, QEvent *event)
{
    if (object == qApp->activePopupWidget())
        return false;

    switch (event->type()) {
        default: break;

        case QEvent::ContextMenu:
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::Enter:
        case QEvent::Leave:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        {
            QWidget *widget = qobject_cast<QWidget*>(object);

            if (widget && (widget == this || isAncestorOf(widget)))
                return handleEvent(widget, event);
        } break;
    }

    return false;
};

int QDesignerMenuBar::findAction(const QPoint &pos) const
{
    QList<QAction*> lst = actions();
    int index = 0;
    for (; index<lst.size() - 1; ++index) {
        QRect g = actionGeometry(lst.at(index));
        g.setTopLeft(QPoint(0, 0));

        if (g.contains(pos))
            break;
    }

    return index;
}

void QDesignerMenuBar::adjustIndicator(const QPoint &pos)
{
    if (QAction *action = actions().at(findAction(pos))) {
        if (action->menu()) {
            setActiveAction(action);
            action->menu()->setActiveAction(0);
        }
    }

    if (QDesignerActionProviderExtension *a = actionProvider()) {
        a->adjustIndicator(pos);
    }
}

void QDesignerMenuBar::dragEnterEvent(QDragEnterEvent *event)
{
    if (const ActionRepositoryMimeData *d = qobject_cast<const ActionRepositoryMimeData*>(event->mimeData())) {
        Q_ASSERT(!d->items.isEmpty());

        QAction *action = d->items.first();
        if (action && !actions().contains(action)) {
            event->acceptProposedAction();
            adjustIndicator(event->pos());
        }
    } else if (qobject_cast<const MenuMimeData*>(event->mimeData())) {
        event->acceptProposedAction();
        adjustIndicator(event->pos());
    }
}

void QDesignerMenuBar::dragMoveEvent(QDragMoveEvent *event)
{
    if (const ActionRepositoryMimeData *d = qobject_cast<const ActionRepositoryMimeData*>(event->mimeData())) {
        Q_ASSERT(!d->items.isEmpty());

        QAction *action = d->items.first();
        if (action && !actions().contains(action)) {
            event->acceptProposedAction();
            adjustIndicator(event->pos());
        }
    } else if (qobject_cast<const MenuMimeData*>(event->mimeData())) {
        event->acceptProposedAction();
        adjustIndicator(event->pos());
    }
}

void QDesignerMenuBar::dragLeaveEvent(QDragLeaveEvent *)
{
    adjustIndicator(QPoint(-1, -1));
}

void QDesignerMenuBar::dropEvent(QDropEvent *event)
{
    if (const ActionRepositoryMimeData *d = qobject_cast<const ActionRepositoryMimeData*>(event->mimeData())) {
        event->acceptProposedAction();

        QAction *action = d->items.first();
        if (action && action->menu() && !actions().contains(action)) {
            int index = findAction(event->pos());
            index = qMin(index, actions().count() - 1);
            insertAction(actions().at(index), action);
            adjustIndicator(QPoint(-1, -1));
        }
    } else if (qobject_cast<const MenuMimeData*>(event->mimeData())) {
        if (QDesignerFormWindowInterface *fw = formWindow()) {
            QDesignerFormEditorInterface *core = fw->core();
            event->acceptProposedAction();

            QMenu *menu = qobject_cast<QMenu*>(core->widgetFactory()->createWidget("QMenu", this)); // ### use undo/redo stack
            menu->setObjectName("menu");
            core->metaDataBase()->add(menu);
            fw->ensureUniqueObjectName(menu);

            QAction *menuAction = menu->menuAction();
            core->metaDataBase()->add(menuAction);
            // fw->ensureUniqueObjectName(menuAction);
            menu->setTitle(tr("Menu"));
            addAction(menu->menuAction());
            core->actionEditor()->setFormWindow(fw);
            core->propertyEditor()->setObject(menu);

            if (QDesignerActionProviderExtension *a = actionProvider()) {
                a->adjustIndicator(QPoint(-1, -1));
            }
        }
    }
}

void QDesignerMenuBar::actionEvent(QActionEvent *event)
{
    QMenuBar::actionEvent(event);

    if (!m_blockSentinelChecker && event->type() == QEvent::ActionAdded
            && m_sentinel && event->action() != m_sentinel)
        m_sentinelChecker->start(0);
}

QDesignerFormWindowInterface *QDesignerMenuBar::formWindow() const
{
    return QDesignerFormWindowInterface::findFormWindow(const_cast<QDesignerMenuBar*>(this));
}

void QDesignerMenuBar::slotCheckSentinel()
{
    bool blocked = blockSentinelChecker(true);
    m_sentinelChecker->stop();
    removeAction(m_sentinel);
    addAction(m_sentinel);
    blockSentinelChecker(blocked);
}

bool QDesignerMenuBar::blockSentinelChecker(bool b)
{
    bool old = m_blockSentinelChecker;
    m_blockSentinelChecker = b;
    return old;
}

QDesignerActionProviderExtension *QDesignerMenuBar::actionProvider()
{
    if (QDesignerFormWindowInterface *fw = formWindow()) {
        QDesignerFormEditorInterface *core = fw->core();
        return qt_extension<QDesignerActionProviderExtension*>(core->extensionManager(), this);
    }

    return 0;
}

} // namespace qdesigner_internal
