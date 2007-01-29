/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <qmenubar.h>

#include <qstyle.h>
#include <qlayout.h>
#include <qapplication.h>
#include <qdesktopwidget.h>
#ifndef QT_NO_ACCESSIBILITY
# include <qaccessible.h>
#endif
#include <qpainter.h>
#include <qevent.h>
#include <qmainwindow.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qwhatsthis.h>

#ifndef QT_NO_MENUBAR

#ifdef QT3_SUPPORT
#include <private/qaction_p.h>
#include <qmenudata.h>
#endif

#include "qmenu_p.h"
#include "qmenubar_p.h"
#include "qdebug.h"


class QMenuBarExtension : public QToolButton
{
public:
    explicit QMenuBarExtension(QWidget *parent);

    QSize sizeHint() const;
};

QMenuBarExtension::QMenuBarExtension(QWidget *parent)
    : QToolButton(parent)
{
    setObjectName(QLatin1String("qt_menubar_ext_button"));
    setAutoRaise(true);
#ifndef QT_NO_MENU
    setPopupMode(QToolButton::InstantPopup);
#endif
    setIcon(style()->standardIcon(QStyle::SP_ToolBarHorizontalExtensionButton));
}

QSize QMenuBarExtension::sizeHint() const
{
    int ext = style()->pixelMetric(QStyle::PM_ToolBarExtensionExtent);
    return QSize(ext, ext);
}

QAction *QMenuBarPrivate::actionAt(QPoint p) const
{
    Q_Q(const QMenuBar);
    QList<QAction*> items = q->actions();
    for(int i = 0; i < items.size(); ++i) {
        if(actionRect(items.at(i)).contains(p))
            return items.at(i);
    }
    return 0;
}

bool QMenuBarPrivate::isVisible(QAction *action)
{
    Q_Q(QMenuBar);

    if (!extension->isHidden()) {
        int hmargin = q->style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, q);
        QRect menuRect = q->rect();
        if (QApplication::layoutDirection() == Qt::RightToLeft)
            menuRect.setLeft(menuRect.left() + extension->width() + hmargin);
        else
            menuRect.setWidth(menuRect.width() - extension->width() - hmargin);
        if (menuRect.contains(actionRect(action)))
            return true;
        else
            return false;
     }
     return true;
}

void QMenuBarPrivate::updateGeometries()
{
    Q_Q(QMenuBar);
    if(!itemsDirty)
        return;
    int q_width = q->width()-(q->style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, q)*2);
    int q_start = -1;
    if(leftWidget || rightWidget) {
        int vmargin = q->style()->pixelMetric(QStyle::PM_MenuBarVMargin, 0, q)
                      + q->style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, q);
        int hmargin = q->style()->pixelMetric(QStyle::PM_MenuBarHMargin, 0, q)
                      + q->style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, q);
        if (leftWidget && leftWidget->isVisible()) {
            QSize sz = leftWidget->sizeHint();
            q_width -= sz.width();
            q_start = sz.width();
            QPoint pos(hmargin, vmargin);
            QRect vRect = QStyle::visualRect(q->layoutDirection(), q->rect(), QRect(pos, sz));
            leftWidget->setGeometry(vRect);
        }
        if (rightWidget && rightWidget->isVisible()) {
            QSize sz = rightWidget->sizeHint();
            q_width -= sz.width();
            QPoint pos(q->width() - sz.width() - hmargin, vmargin);
            QRect vRect = QStyle::visualRect(q->layoutDirection(), q->rect(), QRect(pos, sz));
            rightWidget->setGeometry(vRect);
        }
    }

#ifdef Q_WS_MAC
    if(mac_menubar) {//nothing to see here folks, move along..
        itemsDirty = false;
        return;
    }
#endif
    calcActionRects(q_width, q_start, actionRects, actionList);
    itemsWidth = q_width;
    itemsStart = q_start;
    currentAction = 0;
#ifndef QT_NO_SHORTCUT
    if(itemsDirty) {
        for(int j = 0; j < shortcutIndexMap.size(); ++j)
            q->releaseShortcut(shortcutIndexMap.value(j));
        shortcutIndexMap.resize(0); // faster than clear
        for(int i = 0; i < actionList.count(); i++)
            shortcutIndexMap.append(q->grabShortcut(QKeySequence::mnemonic(actionList.at(i)->text())));
    }
#endif
    itemsDirty = false;

    QList<QAction *> hiddenActions;
    int hmargin = q->style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, q);
    QRect menuRect = q->rect();
    if (!extension->isHidden() && (menuRect.width() < q->sizeHint().width())) {
        if (QApplication::layoutDirection() == Qt::RightToLeft)
            menuRect.setLeft(menuRect.left() + extension->width() + hmargin);
        else
            menuRect.setWidth(menuRect.width() - extension->width() - hmargin);
    }
    for (int i = 0; i < actionList.count(); ++i) {
        if (!menuRect.contains(actionRect(actionList.at(i))))
            hiddenActions.append(actionList.at(i));
    }

    if (hiddenActions.count() > 0) {
        QMenu *pop = extension->menu();
        if (!pop) {
            pop = new QMenu(q);
            extension->setMenu(pop);
        }
        pop->clear();
        pop->addActions(hiddenActions);

        int vmargin = q->style()->pixelMetric(QStyle::PM_MenuBarVMargin, 0, q);
        int x = QApplication::layoutDirection() == Qt::RightToLeft
                ? hmargin
                : q->width() - extension->sizeHint().width() - hmargin;
        extension->setGeometry(x, vmargin, extension->sizeHint().width(), menuRect.height() - vmargin*2);
        extension->show();
    } else {
        extension->hide();
    }
    q->updateGeometry();
#ifdef QT3_SUPPORT
    if (q->parentWidget() != 0) {
        QMenubarUpdatedEvent menubarUpdated(q);
        QApplication::sendEvent(q->parentWidget(), &menubarUpdated);
    }
#endif
}

QRect QMenuBarPrivate::actionRect(QAction *act) const
{
    Q_Q(const QMenuBar);
    QRect ret = actionRects.value(act);
    const int fw = q->style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, q);
    ret.translate(fw, fw);
    return QStyle::visualRect(q->layoutDirection(), q->rect(), ret);
}

void QMenuBarPrivate::setKeyboardMode(bool b)
{
    Q_Q(QMenuBar);
    if (b && !q->style()->styleHint(QStyle::SH_MenuBar_AltKeyNavigation, 0, q)) {
        setCurrentAction(0);
        return;
    }
    keyboardState = b;
    if(b) {
        QWidget *fw = qApp->focusWidget();
        if (fw != q)
            keyboardFocusWidget = qApp->focusWidget();
        if(!currentAction && !actionList.isEmpty())
            setCurrentAction(actionList.first());
        q->setFocus();
    } else {
        if(!popupState)
            setCurrentAction(0);
        if(keyboardFocusWidget) {
            keyboardFocusWidget->setFocus();
            keyboardFocusWidget = 0;
        }
    }
    q->update();
}

void QMenuBarPrivate::popupAction(QAction *action, bool activateFirst)
{
    Q_Q(QMenuBar);
    if(!action || !action->menu())
        return;
    popupState = true;
    if (action->isEnabled() && action->menu()->isEnabled()) {
        closePopupMode = 0;
        activeMenu = action->menu();
        activeMenu->d_func()->causedPopup.widget = q;
        activeMenu->d_func()->causedPopup.action = action;

        QRect adjustedActionRect = actionRect(action);
        QPoint pos(q->mapToGlobal(QPoint(adjustedActionRect.left(), adjustedActionRect.bottom() + 1)));
        QSize popup_size = activeMenu->sizeHint();
        if(q->isRightToLeft())
            pos.setX(pos.x()-(popup_size.width()-adjustedActionRect.width()));

        QRect screenRect = QApplication::desktop()->screenGeometry(pos);
        if(pos.x() < screenRect.x()) {
            pos.setX(screenRect.x());
        } else {
            const int off = pos.x()+popup_size.width() - screenRect.right();
            if(off > 0)
                pos.setX(qMax(screenRect.x(), pos.x()-off));

        }

        if(!defaultPopDown || (pos.y() + popup_size.height() > screenRect.bottom()))
            pos.setY(qMax(screenRect.y(), q->mapToGlobal(QPoint(0, adjustedActionRect.top()-popup_size.height())).y()));
        activeMenu->popup(pos);
        if(activateFirst)
            activeMenu->d_func()->setFirstActionActive();
    }
    q->update(actionRect(action));
}

void QMenuBarPrivate::setCurrentAction(QAction *action, bool popup, bool activateFirst)
{
    if(currentAction == action && popup == popupState)
        return;

    doChildEffects = (popup && !activeMenu);
    Q_Q(QMenuBar);
    QWidget *fw = 0;
    if(activeMenu) {
        QMenu *menu = activeMenu;
        activeMenu = NULL;
        if (popup) {
            fw = q->window()->focusWidget();
            q->setFocus(Qt::NoFocusReason);
        }
        menu->hide();
    }
    if(currentAction)
        q->update(actionRect(currentAction));

    popupState = popup;
#ifndef QT_NO_STATUSTIP
    QAction *previousAction = currentAction;
#endif
    currentAction = action;
    if(action) {
        activateAction(action, QAction::Hover);
        if(popup)
            popupAction(action, activateFirst);
        q->update(actionRect(action));
#ifndef QT_NO_STATUSTIP
    }  else if (previousAction) {
        QString empty;
        QStatusTipEvent tip(empty);
        QApplication::sendEvent(q, &tip);
#endif
    }


    if (fw)
        fw->setFocus(Qt::NoFocusReason);
}

void QMenuBarPrivate::calcActionRects(int max_width, int start, QMap<QAction*, QRect> &actionRects, QList<QAction*> &actionList) const
{
    Q_Q(const QMenuBar);

    if(!itemsDirty && itemsWidth == max_width && itemsStart == start) {
        actionRects = actionRects;
        actionList = actionList;
        return;
    }
    actionRects.clear();
    actionList.clear();
    const int itemSpacing = q->style()->pixelMetric(QStyle::PM_MenuBarItemSpacing, 0, q);
    int max_item_height = 0, separator = -1, separator_start = 0, separator_len = 0;
    QList<QAction*> items = q->actions();

    //calculate size
    const QFontMetrics fm = q->fontMetrics();
    const int hmargin = q->style()->pixelMetric(QStyle::PM_MenuBarHMargin, 0, q),
              vmargin = q->style()->pixelMetric(QStyle::PM_MenuBarVMargin, 0, q),
                icone = q->style()->pixelMetric(QStyle::PM_SmallIconSize, 0, q);
    for(int i = 0; i < items.count(); i++) {
        QAction *action = items.at(i);
        if(!action->isVisible())
            continue;

        QSize sz;

        //calc what I think the size is..
        if(action->isSeparator()) {
            if (q->style()->styleHint(QStyle::SH_DrawMenuBarSeparator, 0, q))
                separator = actionRects.count();
            continue; //we don't really position these!
        } else {
            QString s = action->text();
            if(!s.isEmpty()) {
                int w = fm.width(s);
                w -= s.count(QLatin1Char('&')) * fm.width(QLatin1Char('&'));
                w += s.count(QLatin1String("&&")) * fm.width(QLatin1Char('&'));
                sz = QSize(w, fm.height());
            }

            QIcon is = action->icon();
            if (!is.isNull()) {
                QSize is_sz = QSize(icone, icone);
                if (is_sz.height() > sz.height())
                    sz.setHeight(is_sz.height());
                if (is_sz.width() > sz.width())
                    sz.setWidth(is_sz.width());
            }
        }

        //let the style modify the above size..
        QStyleOptionMenuItem opt;
        q->initStyleOption(&opt, action);
        sz = q->style()->sizeFromContents(QStyle::CT_MenuBarItem, &opt, sz, q);

        if(!sz.isEmpty()) {
            { //update the separator state
                int iWidth = sz.width();
                iWidth += itemSpacing;
                if(separator == -1)
                    separator_start += iWidth;
                else
                    separator_len += iWidth;
            }
            //maximum height
            max_item_height = qMax(max_item_height, sz.height());
            //append
            actionRects.insert(action, QRect(0, 0, sz.width(), sz.height()));
            actionList.append(action);
        }
    }

    //calculate position
    int x = ((start == -1) ? hmargin : start) + itemSpacing;
    int y = vmargin;
    for(int i = 0; i < actionList.count(); i++) {
        QAction *action = actionList.at(i);
        QRect &rect = actionRects[action];
        //resize
        rect.setHeight(max_item_height);

        //move
        if(separator != -1 && i >= separator) { //after the separator
            int left = (max_width - separator_len - hmargin - itemSpacing) + (x - separator_start - hmargin);
            if(left < separator_start) { //wrap
                separator_start = x = hmargin;
                y += max_item_height;
            }
            rect.moveLeft(left);
        } else {
            rect.moveLeft(x);
        }
        rect.moveTop(y);

        //keep moving along..
        x += rect.width() + itemSpacing;
    }
}

void QMenuBarPrivate::activateAction(QAction *action, QAction::ActionEvent action_e)
{
    Q_Q(QMenuBar);
    if (!action || !action->isEnabled())
        return;
    action->activate(action_e);
    if (action_e == QAction::Hover)
        action->showStatusText(q);

//     if(action_e == QAction::Trigger)
//         emit q->activated(action);
//     else if(action_e == QAction::Hover)
//         emit q->highlighted(action);
}


void QMenuBarPrivate::_q_actionTriggered()
{
    Q_Q(QMenuBar);
    if (QAction *action = qobject_cast<QAction *>(q->sender())) {
        emit q->triggered(action);
#ifdef QT3_SUPPORT
        emit q->activated(q->findIdForAction(action));
#endif
    }
}

void QMenuBarPrivate::_q_actionHovered()
{
    Q_Q(QMenuBar);
    if (QAction *action = qobject_cast<QAction *>(q->sender())) {
        emit q->hovered(action);
#ifdef QT3_SUPPORT
        emit q->highlighted(q->findIdForAction(action));
#endif
    }
}

/*!
    Initialize \a option with the values from the menu bar and information from \a action. This method
    is useful for subclasses when they need a QStyleOptionMenuItem, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom() QMenu::initStyleOption()
*/
void QMenuBar::initStyleOption(QStyleOptionMenuItem *option, const QAction *action) const
{
    if (!option || !action)
        return;
    Q_D(const QMenuBar);
    option->palette = palette();
    option->state = QStyle::State_None;
    if (isEnabled() && action->isEnabled())
        option->state |= QStyle::State_Enabled;
    else
        option->palette.setCurrentColorGroup(QPalette::Disabled);
    option->fontMetrics = fontMetrics();
    if (d->currentAction && d->currentAction == action) {
        option->state |= QStyle::State_Selected;
        if (d->popupState && !d->closePopupMode)
            option->state |= QStyle::State_Sunken;
    }
    if (hasFocus() || d->currentAction)
        option->state |= QStyle::State_HasFocus;
    option->menuRect = rect();
    option->menuItemType = QStyleOptionMenuItem::Normal;
    option->checkType = QStyleOptionMenuItem::NotCheckable;
    option->text = action->text();
    option->icon = action->icon();
}

/*!
    \class QMenuBar
    \brief The QMenuBar class provides a horizontal menu bar.

    \ingroup application
    \mainclass

    A menu bar consists of a list of pull-down menu items. You add
    menu items with addMenu(). For example, asuming that \c menubar
    is a pointer to a QMenuBar and \c fileMenu is a pointer to a
    QMenu, the following statement inserts the menu into the menu bar:
    \code
      menubar->addMenu(fileMenu);
    \endcode

    The ampersand in the menu item's text sets Alt+F as a shortcut for
    this menu. (You can use "\&\&" to get a real ampersand in the menu
    bar.)

    There is no need to lay out a menu bar. It automatically sets its
    own geometry to the top of the parent widget and changes it
    appropriately whenever the parent is resized.

    \omit
    Example of creating a menu bar with menu items (from \l menu/menu.cpp):
    \quotefile menu/menu.cpp
    \skipto file = new QMenu
    \printline
    \skipto Qt::Key_O
    \printline
    \printline
    \skipto new QMenuBar
    \printline
    \skipto addMenu
    \printline
    \endomit

    In most main window style applications you would use the menuBar()
    provided in QMainWindow, adding \l{QMenu}s to the menu bar and
    adding \l{QAction}s to the popup menus.

    Example (from the \l{mainwindows/menus}{Menus} example):

    \quotefile mainwindows/menus/mainwindow.cpp
    \skipto fileMenu =
    \printuntil fileMenu->addAction(

    Menu items may be removed with removeAction().

    \section1 Platform Dependent Look and Feel

    Different platforms have different requirements for the appearance
    of menu bars and their behavior when the user interacts with them.
    For example, Windows systems are often configured so that the
    underlined character mnemonics that indicate keyboard shortcuts for
    items in the menu bar are only shown when the \gui{Alt} key is
    pressed.

    \table
    \row \o \inlineimage plastique-menubar.png A menubar shown in the Plastique widget style.
    \o The \l{QPlastiqueStyle}{Plastique widget style}, like most other styles,
    handles the \gui{Help} menu in the same way as it handles any other menu.
    \row \o \inlineimage motif-menubar.png A menubar shown in the Motif widget style.
    \o The \l{QMotifStyle}{Motif widget style} treats \gui{Help} menus in a
    special way, placing them at right-hand end of the menu bar.
    \endtable

    \section1 QMenuBar on Qt/Mac

    QMenuBar on Qt/Mac is a wrapper for using the system-wide menubar.
    If you have multiple menubars in one dialog the outermost menubar
    (normally inside a widget with widget flag Qt::Window) will
    be used for the system-wide menubar.

    Qt/Mac also provides a menubar merging feature to make QMenuBar
    conform more closely to accepted Mac OS X menubar layout. The
    merging functionality is based on string matching the title of a
    QMenu entry. These strings are translated (using QObject::tr()) in
    the "QMenuBar" context. If an entry is moved its slots will still
    fire as if it was in the original place. The table below outlines
    the strings looked for and where the entry is placed if matched:

    \table
    \header \i String matches \i Placement \i Notes
    \row \i about.*
         \i Application Menu | About <application name>
         \i If this entry is not found no About item will appear in
            the Application Menu
    \row \i config, options, setup, settings or preferences
         \i Application Menu | Preferences
         \i If this entry is not found the Settings item will be disabled
    \row \i quit or exit
         \i Application Menu | Quit <application name>
         \i If this entry is not found a default Quit item will be
            created to call QApplication::quit()
    \endtable

    You can override this behavior by using the QAction::menuRole() property.

    If you wish to make all windows in a Mac application share the
    same menubar, you need to create a menu bar that does not have a
    parent. The menubar is created like this:

    \code
	QMenuBar *menuBar = new QMenuBar(0);
    \endcode

    \bold{Note:} The text used for the application name in the menu bar is
    obtained from the value set in the \c{Info.plist} file in the application's
    bundle. See \l{Deploying an Application on Qt/Mac} for more information.

    \section1 Examples

    The \l{mainwindows/menus}{Menus} example shows how to use QMenuBar and QMenu.
    The other \l{Qt Examples#Main Window Examples}{main window application examples}
    also provide menus using these classes.

    \sa QMenu, QShortcut, QAction,
        {http://developer.apple.com/documentation/UserExperience/Conceptual/OSXHIGuidelines/index.html}{Introduction to Apple Human Interface Guidelines},
        {fowler}{GUI Design Handbook: Menu Bar}, {Menus Example}
*/


void QMenuBarPrivate::init()
{
    Q_Q(QMenuBar);
    q->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    q->setAttribute(Qt::WA_CustomWhatsThis);
#ifdef Q_WS_MAC
    macCreateMenuBar(q->parentWidget());
    if(mac_menubar)
        q->hide();
#endif
    q->setBackgroundRole(QPalette::Button);
    q->setAutoFillBackground(true);
    oldWindow = oldParent = 0;
#ifdef QT3_SUPPORT
    doAutoResize = false;
#endif
    handleReparent();
    q->setMouseTracking(q->style()->styleHint(QStyle::SH_MenuBar_MouseTracking, 0, q));

    extension = new QMenuBarExtension(q);
    extension->setFocusPolicy(Qt::NoFocus);
    extension->hide();
}

/*!
    Constructs a menu bar with parent \a parent.
*/
QMenuBar::QMenuBar(QWidget *parent) : QWidget(*new QMenuBarPrivate, parent, 0)
{
    Q_D(QMenuBar);
    d->init();
}

#ifdef QT3_SUPPORT
/*!
    Use one of the constructors that doesn't take the \a name
    argument and then use setObjectName() instead.
*/
QMenuBar::QMenuBar(QWidget *parent, const char *name) : QWidget(*new QMenuBarPrivate, parent, 0)
{
    Q_D(QMenuBar);
    d->init();
    setObjectName(QString::fromAscii(name));
}
#endif

/*!
    Destroys the menu bar.
*/
QMenuBar::~QMenuBar()
{
#ifdef Q_WS_MAC
    Q_D(QMenuBar);
    d->macDestroyMenuBar();
#endif
}

/*!
    \overload

    This convenience function creates a new action with \a text.
    The function adds the newly created action to the menu's
    list of actions, and returns it.

    \sa QWidget::addAction()
*/
QAction *QMenuBar::addAction(const QString &text)
{
    QAction *ret = new QAction(text, this);
    addAction(ret);
    return ret;
}

/*!
    \overload

    This convenience function creates a new action with the given \a
    text. The action's triggered() signal is connected to the \a
    receiver's \a member slot. The function adds the newly created
    action to the menu's list of actions and returns it.

    \sa QWidget::addAction()
*/
QAction *QMenuBar::addAction(const QString &text, const QObject *receiver, const char* member)
{
    QAction *ret = new QAction(text, this);
    QObject::connect(ret, SIGNAL(triggered(bool)), receiver, member);
    addAction(ret);
    return ret;
}

/*!
  Appends a new QMenu with \a title to the menubar. The menubar
  takes ownership of the menu. Returns the new menu.

  \sa QWidget::addAction() QMenu::menuAction()
*/
QMenu *QMenuBar::addMenu(const QString &title)
{
    QMenu *menu = new QMenu(title, this);
    addAction(menu->menuAction());
    return menu;
}

/*!
  Appends a new QMenu with \a icon and \a title to the menubar. The menubar
  takes ownership of the menu. Returns the new menu.

  \sa QWidget::addAction() QMenu::menuAction()
*/
QMenu *QMenuBar::addMenu(const QIcon &icon, const QString &title)
{
    QMenu *menu = new QMenu(title, this);
    menu->setIcon(icon);
    addAction(menu->menuAction());
    return menu;
}

/*!
  Appends \a menu to the menubar. Returns the menu's menuAction().

  \sa QWidget::addAction() QMenu::menuAction()
*/
QAction *QMenuBar::addMenu(QMenu *menu)
{
    QAction *action = menu->menuAction();
    addAction(action);
    return action;
}

/*!
  Appends a separator to the menu.
*/
QAction *QMenuBar::addSeparator()
{
    QAction *ret = new QAction(this);
    ret->setSeparator(true);
    addAction(ret);
    return ret;
}

/*!
    This convenience function creates a new separator action, i.e. an
    action with QAction::isSeparator() returning true. The function inserts
    the newly created action into this menubar's list of actions before
    action \a before and returns it.

    \sa QWidget::insertAction(), addSeparator()
*/
QAction *QMenuBar::insertSeparator(QAction *before)
{
    QAction *action = new QAction(this);
    action->setSeparator(true);
    insertAction(before, action);
    return action;
}

/*!
  This convenience function inserts \a menu before action \a before
  and returns the menus menuAction().

  \sa QWidget::insertAction() addMenu()
*/
QAction *QMenuBar::insertMenu(QAction *before, QMenu *menu)
{
    QAction *action = menu->menuAction();
    insertAction(before, action);
    return action;
}

/*!
  Returns the QAction that is currently highlighted. A null pointer
  will be returned if no action is currently selected.
*/
QAction *QMenuBar::activeAction() const
{
    Q_D(const QMenuBar);
    return d->currentAction;
}

/*!
    \since 4.1

    Sets the currently highlighted action to \a act.
*/
void QMenuBar::setActiveAction(QAction *act)
{
    Q_D(QMenuBar);
    d->setCurrentAction(act, true, false);
}


/*!
    Removes all the actions from the menu bar.

    \sa removeAction()
*/
void QMenuBar::clear()
{
    QList<QAction*> acts = actions();
    for(int i = 0; i < acts.size(); i++)
        removeAction(acts[i]);
}

/*!
    \property QMenuBar::defaultUp
    \brief the popup orientation

    The default popup orientation. By default, menus pop "down" the
    screen. By setting the property to true, the menu will pop "up".
    You might call this for menus that are \e below the document to
    which they refer.

    If the menu would not fit on the screen, the other direction is
    used automatically.
*/
void QMenuBar::setDefaultUp(bool b)
{
    Q_D(QMenuBar);
    d->defaultPopDown = !b;
}

bool QMenuBar::isDefaultUp() const
{
    Q_D(const QMenuBar);
    return !d->defaultPopDown;
}

/*!
  \reimp
*/
void QMenuBar::resizeEvent(QResizeEvent *)
{
    Q_D(QMenuBar);
    d->itemsDirty = true;
    d->updateGeometries();
}

/*!
  \reimp
*/
void QMenuBar::paintEvent(QPaintEvent *e)
{
    Q_D(QMenuBar);
    QPainter p(this);
    QRegion emptyArea(rect());

    //draw the items
    for (int i = 0; i < d->actionList.count(); ++i) {
        QAction *action = d->actionList.at(i);
        QRect adjustedActionRect = d->actionRect(action);
        if (adjustedActionRect.isEmpty() || !d->isVisible(action))
            continue;
        if(!e->rect().intersects(adjustedActionRect))
            continue;

        emptyArea -= adjustedActionRect;
        QStyleOptionMenuItem opt;
        initStyleOption(&opt, action);
        opt.rect = adjustedActionRect;
        p.setClipRect(adjustedActionRect);
        style()->drawControl(QStyle::CE_MenuBarItem, &opt, &p, this);
    }
     //draw border
    if(int fw = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, this)) {
        QRegion borderReg;
        borderReg += QRect(0, 0, fw, height()); //left
        borderReg += QRect(width()-fw, 0, fw, height()); //right
        borderReg += QRect(0, 0, width(), fw); //top
        borderReg += QRect(0, height()-fw, width(), fw); //bottom
        p.setClipRegion(borderReg);
        emptyArea -= borderReg;
        QStyleOptionFrame frame;
        frame.rect = rect();
        frame.palette = palette();
        frame.state = QStyle::State_None;
        frame.lineWidth = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth);
        frame.midLineWidth = 0;
        style()->drawPrimitive(QStyle::PE_PanelMenuBar, &frame, &p, this);
    }
    p.setClipRegion(emptyArea);
    QStyleOptionMenuItem menuOpt;
    menuOpt.palette = palette();
    menuOpt.state = QStyle::State_None;
    menuOpt.menuItemType = QStyleOptionMenuItem::EmptyArea;
    menuOpt.checkType = QStyleOptionMenuItem::NotCheckable;
    menuOpt.rect = rect();
    menuOpt.menuRect = rect();
    style()->drawControl(QStyle::CE_MenuBarEmptyArea, &menuOpt, &p, this);
}

/*!
  \reimp
*/
void QMenuBar::setVisible(bool visible)
{
#ifdef Q_WS_MAC
    Q_D(QMenuBar);
    if(d->mac_menubar)
        return;
#endif
    QWidget::setVisible(visible);
}

/*!
  \reimp
*/
void QMenuBar::mousePressEvent(QMouseEvent *e)
{
    Q_D(QMenuBar);
    if(e->button() != Qt::LeftButton)
        return;

    QAction *action = d->actionAt(e->pos());
    if (!action || !d->isVisible(action)) {
        d->setCurrentAction(0);
#ifndef QT_NO_WHATSTHIS
        if (QWhatsThis::inWhatsThisMode())
            QWhatsThis::showText(e->globalPos(), d->whatsThis, this);
#endif
        return;
    }

    d->mouseDown = true;

    if(d->currentAction == action && d->popupState) {
        if((d->closePopupMode = style()->styleHint(QStyle::SH_MenuBar_DismissOnSecondClick)))
            update(d->actionRect(action));
    } else {
        d->setCurrentAction(action, true);
    }
}

/*!
  \reimp
*/
void QMenuBar::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QMenuBar);
    if(e->button() != Qt::LeftButton || !d->mouseDown)
        return;

    d->mouseDown = false;
    QAction *action = d->actionAt(e->pos());
    if((d->closePopupMode && action == d->currentAction) || !action || !action->menu()) {
        if(action)
            d->activateAction(action, QAction::Trigger);
        d->setCurrentAction(action, false);
    }
    d->closePopupMode = 0;
}

/*!
  \reimp
*/
void QMenuBar::keyPressEvent(QKeyEvent *e)
{
    Q_D(QMenuBar);
    int key = e->key();
    if(isRightToLeft()) {  // in reverse mode open/close key for submenues are reversed
        if(key == Qt::Key_Left)
            key = Qt::Key_Right;
        else if(key == Qt::Key_Right)
            key = Qt::Key_Left;
    }
    if(key == Qt::Key_Tab) //means right
        key = Qt::Key_Right;
    else if(key == Qt::Key_Backtab) //means left
        key = Qt::Key_Left;

    bool key_consumed = false;
    switch(key) {
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Enter:
    case Qt::Key_Space:
    case Qt::Key_Return: {
        if(!style()->styleHint(QStyle::SH_MenuBar_AltKeyNavigation, 0, this) || !d->currentAction)
           break;
        if(d->currentAction->menu()) {
            d->popupAction(d->currentAction, true);
        } else if(key == Qt::Key_Enter || key == Qt::Key_Return || key == Qt::Key_Space) {
            d->activateAction(d->currentAction, QAction::Trigger);
            d->setCurrentAction(d->currentAction, false);
        }
        key_consumed = true;
        break; }

    case Qt::Key_Right:
    case Qt::Key_Left: {
        if(d->currentAction) {
            QAction *nextAction = 0;
            for(int i=0; i<(int)d->actionList.count(); i++) {
                if(d->actionList.at(i) == (QAction*)d->currentAction) {
                    if(key == Qt::Key_Left) {
                        if(i > 0)
                            nextAction = d->actionList.at(i-1);
                    } else {
                        if(i < d->actionList.count()-1)
                            nextAction = d->actionList.at(i+1);
                    }
                    break;
                }
            }
            if(!nextAction) {
                if(key == Qt::Key_Left)
                    nextAction = d->actionList.last();
                else
                    nextAction = d->actionList.first();
            }
            if(nextAction) {
                d->setCurrentAction(nextAction, d->popupState, true);
                key_consumed = true;
            }
        }
        break; }

    case Qt::Key_Escape:
        d->setCurrentAction(0);
        d->setKeyboardMode(false);
        key_consumed = true;
        break;

    default:
        key_consumed = false;
    }

    if(!key_consumed &&
       (!e->modifiers() ||
        (e->modifiers()&(Qt::MetaModifier|Qt::AltModifier))) && e->text().length()==1 && !d->popupState) {
        int clashCount = 0;
        QAction *first = 0, *currentSelected = 0, *firstAfterCurrent = 0;
        {
            QChar c = e->text()[0].toUpper();
            for(int i = 0; i < d->actionList.size(); ++i) {
                register QAction *act = d->actionList.at(i);
                QString s = act->text();
                if(!s.isEmpty()) {
                    int ampersand = s.indexOf(QLatin1Char('&'));
                    if(ampersand >= 0) {
                        if(s[ampersand+1].toUpper() == c) {
                            clashCount++;
                            if(!first)
                                first = act;
                            if(act == d->currentAction)
                                currentSelected = act;
                            else if (!firstAfterCurrent && currentSelected)
                                firstAfterCurrent = act;
                        }
                    }
                }
            }
        }
        QAction *next_action = 0;
        if(clashCount >= 1) {
            if(clashCount == 1 || !d->currentAction || (currentSelected && !firstAfterCurrent))
                next_action = first;
            else
                next_action = firstAfterCurrent;
        }
        if(next_action) {
            key_consumed = true;
            d->setCurrentAction(next_action, true, true);
        }
    }
    if(key_consumed)
        e->accept();
    else
        e->ignore();
}

/*!
  \reimp
*/
void QMenuBar::mouseMoveEvent(QMouseEvent *e)
{
    Q_D(QMenuBar);
    d->mouseDown = e->buttons() & Qt::LeftButton;
    QAction *action = d->actionAt(e->pos());
    bool popupState = d->popupState || d->mouseDown;
    if(action && d->isVisible(action) || !popupState)
        d->setCurrentAction(action, popupState);
}

/*!
  \reimp
*/
void QMenuBar::leaveEvent(QEvent *)
{
    Q_D(QMenuBar);
    if(!hasFocus() && !d->popupState)
        d->setCurrentAction(0);
}

/*!
  \reimp
*/
void QMenuBar::actionEvent(QActionEvent *e)
{
    Q_D(QMenuBar);
    d->itemsDirty = true;
#ifdef Q_WS_MAC
    if(d->mac_menubar) {
        if(e->type() == QEvent::ActionAdded)
            d->mac_menubar->addAction(e->action(), d->mac_menubar->findAction(e->before()));
        else if(e->type() == QEvent::ActionRemoved)
            d->mac_menubar->removeAction(e->action());
        else if(e->type() == QEvent::ActionChanged)
            d->mac_menubar->syncAction(e->action());
    }
#endif
    if(e->type() == QEvent::ActionAdded) {
        connect(e->action(), SIGNAL(triggered()), this, SLOT(_q_actionTriggered()));
        connect(e->action(), SIGNAL(hovered()), this, SLOT(_q_actionHovered()));
    } else if(e->type() == QEvent::ActionRemoved) {
        e->action()->disconnect(this);
    }
    if (isVisible()) {
        d->updateGeometries();
        update();
    }
}

/*!
  \reimp
*/
void QMenuBar::focusInEvent(QFocusEvent *)
{
    Q_D(QMenuBar);
    if(d->keyboardState && !d->currentAction && !d->actionList.isEmpty())
        d->setCurrentAction(d->actionList.first());
}

/*!
  \reimp
*/
void QMenuBar::focusOutEvent(QFocusEvent *)
{
    Q_D(QMenuBar);
    if(!d->popupState) {
        d->setCurrentAction(0);
        d->setKeyboardMode(false);
    }
}



void QMenuBarPrivate::handleReparent()
{
    Q_Q(QMenuBar);
    QWidget *newParent = q->parentWidget();
    //Note: if parent is reparented, then window may change even if parent doesn't

    // we need to install an event filter on parent, and remove the old one

    if (oldParent != newParent) {
        if (oldParent)
            oldParent->removeEventFilter(q);
        if (newParent)
            newParent->installEventFilter(q);
    }

    //we also need event filter on top-level (for shortcuts)
    QWidget *newWindow = newParent ? newParent->window() : 0;

    if (oldWindow != newWindow) {
        if (oldParent && oldParent != oldWindow)
            oldWindow->removeEventFilter(q);

        if (newParent && newParent != newWindow)
            newWindow->installEventFilter(q);
    }

    oldParent = newParent;
    oldWindow = newWindow;
}

#ifdef QT3_SUPPORT
/*!
    Sets whether the menu bar should automatically resize itself
    when its parent widget is resized.

    This feature is provided to help porting to Qt 4. We recommend
    against using it in new code.

    \sa autoGeometry()
*/
void QMenuBar::setAutoGeometry(bool b)
{
    Q_D(QMenuBar);
    d->doAutoResize = b;
}

/*!
    Returns true if the menu bar automatically resizes itself
    when its parent widget is resized; otherwise returns false.

    This feature is provided to help porting to Qt 4. We recommend
    against using it in new code.

    \sa setAutoGeometry()
*/
bool QMenuBar::autoGeometry() const
{
    Q_D(const QMenuBar);
    return d->doAutoResize;
}
#endif

/*!
  \reimp
*/
void QMenuBar::changeEvent(QEvent *e)
{
    Q_D(QMenuBar);
    if(e->type() == QEvent::StyleChange) {
        d->itemsDirty = true;
        setMouseTracking(style()->styleHint(QStyle::SH_MenuBar_MouseTracking, 0, this));
        if(parentWidget())
            resize(parentWidget()->width(), heightForWidth(parentWidget()->width()));
        d->updateGeometries();
    } else if (e->type() == QEvent::ParentChange) {
        d->handleReparent();
    } else if (e->type() == QEvent::FontChange
               || e->type() == QEvent::ApplicationFontChange) {
        d->itemsDirty = true;
        d->updateGeometries();
    }
    QWidget::changeEvent(e);
}

/*!
  \reimp
*/
bool QMenuBar::event(QEvent *e)
{
    Q_D(QMenuBar);
    switch (e->type()) {
    case QEvent::KeyPress: {
        QKeyEvent *ke = (QKeyEvent*)e;
#if 0
        if(!d->keyboardState) { //all keypresses..
            d->setCurrentAction(0);
            return ;
        }
#endif
        if(ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Backtab) {
            keyPressEvent(ke);
            return true;
        }

    } break;
#ifndef QT_NO_SHORTCUT
    case QEvent::Shortcut: {
        QShortcutEvent *se = static_cast<QShortcutEvent *>(e);
        int shortcutId = se->shortcutId();
        for(int j = 0; j < d->shortcutIndexMap.size(); ++j) {
            if (shortcutId == d->shortcutIndexMap.value(j))
                d->_q_internalShortcutActivated(j);
        }
    } break;
#endif
    case QEvent::Show:
#ifdef QT3_SUPPORT
        if(QWidget *p = parentWidget()) {
            // If itemsDirty == true, updateGeometries sends the MenubarUpdated event.
            if (!d->itemsDirty) {
                QMenubarUpdatedEvent menubarUpdated(this);
                QApplication::sendEvent(p, &menubarUpdated);
            }
        }
#endif
        d->updateGeometries();
    break;

#ifdef QT3_SUPPORT
    case QEvent::Hide: {
        if(QWidget *p = parentWidget()) {
            QMenubarUpdatedEvent menubarUpdated(this);
            QApplication::sendEvent(p, &menubarUpdated);
        }
    } break;
#endif

#ifndef QT_NO_WHATSTHIS
    case QEvent::QueryWhatsThis:
        e->setAccepted(d->whatsThis.size());
        if (QAction *action = d->actionAt(static_cast<QHelpEvent*>(e)->pos())) {
            if (action->whatsThis().size() || action->menu())
                e->accept();
        }
        return true;
#endif
    default:
        break;
    }
    return QWidget::event(e);
}

/*!
  \reimp
*/
bool QMenuBar::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QMenuBar);
    if (object == parent() && object) {
#ifdef QT3_SUPPORT
        if (d->doAutoResize && event->type() == QEvent::Resize) {
            QResizeEvent *e = (QResizeEvent *)event;
            int w = e->size().width();
            setGeometry(0, y(), w, heightForWidth(w));
            return false;
        }
#endif
        if (event->type() == QEvent::ParentChange) //GrandparentChange
            d->handleReparent();
    }
    if (object == d->leftWidget || object == d->rightWidget) {
        switch (event->type()) {
        case QEvent::ShowToParent:
        case QEvent::HideToParent:
            d->_q_updateLayout();
            break;
        default:
            break;
        }
    }

    if (style()->styleHint(QStyle::SH_MenuBar_AltKeyNavigation, 0, this)) {
        if (d->altPressed) {
            switch (event->type()) {
            case QEvent::KeyPress:
            case QEvent::KeyRelease:
            {
                QKeyEvent *kev = static_cast<QKeyEvent*>(event);
                if (kev->key() == Qt::Key_Alt || kev->key() == Qt::Key_Meta) {
                    if (event->type() == QEvent::KeyPress) // Alt-press does not interest us, we have the shortcut-override event
                        break;
                    d->setKeyboardMode(!d->keyboardState);
                }
            }
            // fall through
            case QEvent::MouseButtonPress:
            case QEvent::FocusIn:
            case QEvent::FocusOut:
            case QEvent::ActivationChange:
                d->altPressed = false;
                qApp->removeEventFilter(this);
                break;
            default:
                break;
            }
        } else if (isVisible()) {
            if (event->type() == QEvent::ShortcutOverride) {
                QKeyEvent *kev = static_cast<QKeyEvent*>(event);
                if ((kev->key() == Qt::Key_Alt || kev->key() == Qt::Key_Meta)
                    && kev->modifiers() == Qt::AltModifier) {
                    d->altPressed = true;
                    qApp->installEventFilter(this);
                }
            }
        }
    }

    return false;
}

/*!
  \internal

  Return the item at \a pt, or 0 if there is no item there or if it is
  a separator item.
*/
QAction *QMenuBar::actionAt(const QPoint &pt) const
{
    Q_D(const QMenuBar);
    return d->actionAt(pt);
}

/*!
  \internal

  Returns the geometry of action \a act.
*/
QRect QMenuBar::actionGeometry(QAction *act) const
{
    Q_D(const QMenuBar);
    return d->actionRect(act);
}

/*!
  \reimp
*/
QSize QMenuBar::minimumSizeHint() const
{
    Q_D(const QMenuBar);
#ifdef Q_WS_MAC
    const bool as_gui_menubar = !d->mac_menubar;
#else
    const bool as_gui_menubar = true;
#endif

    ensurePolished();
    QSize ret(0, 0);
    const int hmargin = style()->pixelMetric(QStyle::PM_MenuBarHMargin, 0, this);
    const int vmargin = style()->pixelMetric(QStyle::PM_MenuBarVMargin, 0, this);
    int fw = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, this);
    int spaceBelowMenuBar = style()->styleHint(QStyle::SH_MainWindow_SpaceBelowMenuBar, 0, this);
    if(as_gui_menubar) {
        QMap<QAction*, QRect> actionRects;
        QList<QAction*> actionList;
        int w = parentWidget() ? parentWidget()->width() : QApplication::desktop()->width();
        d->calcActionRects(w - (2 * fw), 0, actionRects, actionList);
        if (d->actionList.count() > 0) {
            ret = d->actionRect(d->actionList.at(0)).size();
            if (!d->extension->isHidden())
                ret += QSize(d->extension->sizeHint().width(), 0);
        }
        ret += QSize(2*fw + hmargin, 2*fw + vmargin);
    }
    int margin = 2*vmargin + 2*fw + spaceBelowMenuBar;
    if(d->leftWidget) {
        QSize sz = d->leftWidget->minimumSizeHint();
        ret.setWidth(ret.width() + sz.width());
        if(sz.height() + margin > ret.height())
            ret.setHeight(sz.height() + margin);
    }
    if(d->rightWidget) {
        QSize sz = d->rightWidget->minimumSizeHint();
        ret.setWidth(ret.width() + sz.width());
        if(sz.height() + margin > ret.height())
            ret.setHeight(sz.height() + margin);
    }
    if(as_gui_menubar) {
        QStyleOptionMenuItem opt;
        opt.rect = rect();
        opt.menuRect = rect();
        opt.state = QStyle::State_None;
        opt.menuItemType = QStyleOptionMenuItem::Normal;
        opt.checkType = QStyleOptionMenuItem::NotCheckable;
        opt.palette = palette();
        return (style()->sizeFromContents(QStyle::CT_MenuBar, &opt,
                                         ret.expandedTo(QApplication::globalStrut()),
                                         this));
    }
    return ret;
}

/*!
  \reimp
*/
QSize QMenuBar::sizeHint() const
{
    Q_D(const QMenuBar);
#ifdef Q_WS_MAC
    const bool as_gui_menubar = !d->mac_menubar;
#else
    const bool as_gui_menubar = true;
#endif

    ensurePolished();
    QSize ret(0, 0);
    const int hmargin = style()->pixelMetric(QStyle::PM_MenuBarHMargin, 0, this);
    const int vmargin = style()->pixelMetric(QStyle::PM_MenuBarVMargin, 0, this);
    int fw = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, this);
    int spaceBelowMenuBar = style()->styleHint(QStyle::SH_MainWindow_SpaceBelowMenuBar, 0, this);
    if(as_gui_menubar) {
        QMap<QAction*, QRect> actionRects;
        QList<QAction*> actionList;
        const int w = parentWidget() ? parentWidget()->width() : QApplication::desktop()->width();
        d->calcActionRects(w - (2 * fw), 0, actionRects, actionList);
        for (QMap<QAction*, QRect>::const_iterator i = actionRects.constBegin();
             i != actionRects.constEnd(); ++i) {
            QRect actionRect(i.value());
            if(actionRect.x() + actionRect.width() > ret.width())
                ret.setWidth(actionRect.x() + actionRect.width());
            if(actionRect.y() + actionRect.height() > ret.height())
                ret.setHeight(actionRect.y() + actionRect.height());
        }
        ret += QSize(2*fw + 2*hmargin, 2*fw + 2*vmargin);
    }
    int margin = 2*vmargin + 2*fw + spaceBelowMenuBar;
    if(d->leftWidget) {
        QSize sz = d->leftWidget->sizeHint();
        ret.setWidth(ret.width() + sz.width());
        if(sz.height() + margin > ret.height())
            ret.setHeight(sz.height() + margin);
    }
    if(d->rightWidget) {
        QSize sz = d->rightWidget->sizeHint();
        ret.setWidth(ret.width() + sz.width());
        if(sz.height() + margin > ret.height())
            ret.setHeight(sz.height() + margin);
    }
    if(as_gui_menubar) {
        QStyleOptionMenuItem opt;
        opt.rect = rect();
        opt.menuRect = rect();
        opt.state = QStyle::State_None;
        opt.menuItemType = QStyleOptionMenuItem::Normal;
        opt.checkType = QStyleOptionMenuItem::NotCheckable;
        opt.palette = palette();
        return (style()->sizeFromContents(QStyle::CT_MenuBar, &opt,
                                         ret.expandedTo(QApplication::globalStrut()),
                                         this));
    }
    return ret;
}

/*!
  \reimp
*/
int QMenuBar::heightForWidth(int) const
{
    Q_D(const QMenuBar);
#ifdef Q_WS_MAC
    const bool as_gui_menubar = !d->mac_menubar;
#else
    const bool as_gui_menubar = true;
#endif
    int height = 0;
    const int vmargin = style()->pixelMetric(QStyle::PM_MenuBarVMargin, 0, this);
    int fw = style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, this);
    int spaceBelowMenuBar = style()->styleHint(QStyle::SH_MainWindow_SpaceBelowMenuBar, 0, this);
    if(as_gui_menubar) {
        if (d->actionList.count()) {
            // assume all actionrects have the same height
            height = d->actionRect(d->actionList.first()).height();
            height += spaceBelowMenuBar;
        }
        height += 2*fw;
        height += 2*vmargin;
    }
    int margin = 2*vmargin + 2*fw + spaceBelowMenuBar;
    if(d->leftWidget)
        height = qMax(d->leftWidget->sizeHint().height() + margin, height);
    if(d->rightWidget)
        height = qMax(d->rightWidget->sizeHint().height() + margin, height);
    if(as_gui_menubar) {
        QStyleOptionMenuItem opt;
        opt.init(this);
        opt.menuRect = rect();
        opt.state = QStyle::State_None;
        opt.menuItemType = QStyleOptionMenuItem::Normal;
        opt.checkType = QStyleOptionMenuItem::NotCheckable;
        return style()->sizeFromContents(QStyle::CT_MenuBar, &opt, QSize(0, height), this).height(); //not pretty..
    }
    return height;
}

/*!
  \internal
*/
void QMenuBarPrivate::_q_internalShortcutActivated(int id)
{
    QAction *act = actionList.at(id);
    setCurrentAction(act, true, true);
}

void QMenuBarPrivate::_q_updateLayout()
{
    Q_Q(QMenuBar);
    itemsDirty = true;
    if (q->isVisible()) {
        updateGeometries();
        q->update();
    }
}

/*!
  \internal

  This sets widget \a w to be shown directly on the left of the first or
  the right of the last menu item, depending on \a corner.
*/
void QMenuBar::setCornerWidget(QWidget *w, Qt::Corner corner)
{
    Q_D(QMenuBar);
    switch (corner) {
    case Qt::TopLeftCorner:
        if (d->leftWidget)
            d->leftWidget->removeEventFilter(this);
        d->leftWidget = w;
        break;
    case Qt::TopRightCorner:
        if (d->rightWidget)
            d->rightWidget->removeEventFilter(this);
        d->rightWidget = w;
        break;
    default:
        qWarning("QMenuBar::setCornerWidget: Only TopLeftCorner and TopRightCorner are supported");
        return;
    }

    if (w) {
        w->setParent(this);
        w->installEventFilter(this);
    }

    d->_q_updateLayout();
}

/*!
  \internal

  Returns the widget in the left of the first or the right of the last menu
  item, depending on \a corner.
*/
QWidget *QMenuBar::cornerWidget(Qt::Corner corner) const
{
    Q_D(const QMenuBar);
    QWidget *w = 0;
    switch(corner) {
    case Qt::TopLeftCorner:
        w = d->leftWidget;
        break;
    case Qt::TopRightCorner:
        w = d->rightWidget;
        break;
    default:
        qWarning("QMenuBar::cornerWidget: Only TopLeftCorner and TopRightCorner are supported");
        break;
    }

    return w;
}

/*!
    \fn void QMenuBar::triggered(QAction *action)

    This signal is emitted when a menu action is selected; \a action
    is the action that caused the event to be sent.

    Normally, you connect each menu action to a single slot using
    QAction::triggered(), but sometimes you will want to connect
    several items to a single slot (most often if the user selects
    from an array). This signal is useful in such cases.

    \sa hovered(), QAction::triggered()
*/

/*!
    \fn void QMenuBar::hovered(QAction *action)

    This signal is emitted when a menu action is highlighted; \a action
    is the action that caused the event to be sent.

    Often this is used to update status information.

    \sa triggered(), QAction::hovered()
*/


#ifdef QT3_SUPPORT
/*!
    Use style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, this)
    instead.
*/
int QMenuBar::frameWidth() const
{
    return style()->pixelMetric(QStyle::PM_MenuBarPanelWidth, 0, this);
}

int QMenuBar::insertAny(const QIcon *icon, const QString *text, const QObject *receiver, const char *member,
                        const QKeySequence *shortcut, const QMenu *popup, int id, int index)
{
    QAction *act = popup ? popup->menuAction() : new QAction(this);
    if(id != -1)
        static_cast<QMenuItem*>(act)->setId(id);
    if(icon)
        act->setIcon(*icon);
    if(text)
        act->setText(*text);
    if(shortcut)
        act->setShortcut(*shortcut);
    if(receiver && member)
        QObject::connect(act, SIGNAL(triggered(bool)), receiver, member);
    if(index == -1 || index >= actions().count())
        addAction(act);
    else
        insertAction(actions().value(index), act);
    return findIdForAction(act);
}

/*!
    \since 4.2

    Use addSeparator() or insertAction() instead.

    \oldcode
        menuBar->insertSeparator();
    \newcode
        menuBar->addSeparator();
    \endcode
*/
int QMenuBar::insertSeparator(int index)
{
    QAction *act = new QAction(this);
    act->setSeparator(true);
    if(index == -1 || index >= actions().count())
        addAction(act);
    else
        insertAction(actions().value(index), act);
    return findIdForAction(act);
}

/*!
    Use QAction::setData() instead.
*/
bool QMenuBar::setItemParameter(int id, int param)
{
    if(QAction *act = findActionForId(id)) {
        act->d_func()->param = param;
        return true;
    }
    return false;
}

/*!
    Use QAction::data() instead.
*/
int QMenuBar::itemParameter(int id) const
{
    if(QAction *act = findActionForId(id))
        return act->d_func()->param;
    return id;
}

QAction *QMenuBar::findActionForId(int id) const
{
    QList<QAction *> list = actions();
    for (int i = 0; i < list.size(); ++i) {
        QAction *act = list.at(i);
        if (findIdForAction(act) == id)
            return act;
    }
    return 0;
}

int QMenuBar::findIdForAction(QAction *act) const
{
    Q_ASSERT(act);
    return act->d_func()->id;
}
#endif

/*!
    \enum QMenuBar::Separator

    \compat

    \value Never
    \value InWindowsStyle

*/

/*!
    \fn uint QMenuBar::count() const

    Use actions().count() instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QString &text, const QObject *receiver, const char* member, const QKeySequence& shortcut, int id, int index)

    Use one of the insertAction() or addAction() overloads instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QIcon& icon, const QString &text, const QObject *receiver, const char* member, const QKeySequence& shortcut, int id, int index)

    Use one of the insertAction() or addAction() overloads instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QPixmap &pixmap, const QObject *receiver, const char* member, const QKeySequence& shortcut, int id, int index)

    Use one of the insertAction(), addAction(), insertMenu(), or
    addMenu() overloads instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QString &text, int id, int index)

    Use one of the insertAction() or addAction() overloads instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QIcon& icon, const QString &text, int id, int index)

    Use one of the insertAction(), addAction(), insertMenu(), or
    addMenu() overloads instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QString &text, QMenu *popup, int id, int index)

    Use one of the insertMenu(), or addMenu() overloads instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QIcon& icon, const QString &text, QMenu *popup, int id, int index)

    Use one of the insertMenu(), or addMenu() overloads instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QPixmap &pixmap, int id, int index)

    Use one of the insertAction(), addAction(), insertMenu(), or
    addMenu() overloads instead.
*/

/*!
    \fn int QMenuBar::insertItem(const QPixmap &pixmap, QMenu *popup, int id, int index)

    Use one of the insertMenu(), or addMenu() overloads instead.
*/

/*!
    \fn void QMenuBar::removeItem(int id)

    Use removeAction() instead.
*/

/*!
    \fn void QMenuBar::removeItemAt(int index)

    Use removeAction() instead.
*/

/*!
    \fn QKeySequence QMenuBar::accel(int id) const

    Use shortcut() on the relevant QAction instead.
*/

/*!
    \fn void QMenuBar::setAccel(const QKeySequence& key, int id)

    Use setShortcut() on the relevant QAction instead.
*/

/*!
    \fn QIcon QMenuBar::iconSet(int id) const

    Use icon() on the relevant QAction instead.
*/

/*!
    \fn QString QMenuBar::text(int id) const

    Use text() on the relevant QAction instead.
*/

/*!
    \fn QPixmap QMenuBar::pixmap(int id) const

    Use QPixmap(icon()) on the relevant QAction instead.
*/

/*!
    \fn void QMenuBar::setWhatsThis(int id, const QString &w)

    Use setWhatsThis() on the relevant QAction instead.
*/

/*!
    \fn QString QMenuBar::whatsThis(int id) const

    Use whatsThis() on the relevant QAction instead.
*/

/*!
    \fn void QMenuBar::changeItem(int id, const QString &text)

    Use setText() on the relevant QAction instead.
*/

/*!
    \fn void QMenuBar::changeItem(int id, const QPixmap &pixmap)

    Use setText() on the relevant QAction instead.
*/

/*!
    \fn void QMenuBar::changeItem(int id, const QIcon &icon, const QString &text)

    Use setIcon() and setText() on the relevant QAction instead.
*/

/*!
    \fn bool QMenuBar::isItemActive(int id) const

    Use activeAction() instead.
*/

/*!
    \fn bool QMenuBar::isItemEnabled(int id) const

    Use isEnabled() on the relevant QAction instead.
*/

/*!
    \fn void QMenuBar::setItemEnabled(int id, bool enable)

    Use setEnabled() on the relevant QAction instead.
*/

/*!
    \fn bool QMenuBar::isItemChecked(int id) const

    Use isChecked() on the relevant QAction instead.
*/

/*!
    \fn void QMenuBar::setItemChecked(int id, bool check)

    Use setChecked() on the relevant QAction instead.
*/

/*!
    \fn bool QMenuBar::isItemVisible(int id) const

    Use isVisible() on the relevant QAction instead.
*/

/*!
    \fn void QMenuBar::setItemVisible(int id, bool visible)

    Use setVisible() on the relevant QAction instead.
*/

/*!
    \fn int QMenuBar::indexOf(int id) const

    Use actions().indexOf(action) on the relevant QAction instead.
*/

/*!
    \fn int QMenuBar::idAt(int index) const

    Use actions instead.
*/

/*!
    \fn void QMenuBar::activateItemAt(int index)

    Use activate() on the relevant QAction instead.
*/

/*!
    \fn bool QMenuBar::connectItem(int id, const QObject *receiver, const char* member)

    Use connect() on the relevant QAction instead.
*/

/*!
    \fn bool QMenuBar::disconnectItem(int id,const QObject *receiver, const char* member)

    Use disconnect() on the relevant QAction instead.
*/

/*!
    \fn QMenuItem *QMenuBar::findItem(int id) const

    Use actions instead.
*/

/*!
    \fn Separator QMenuBar::separator() const

    This function is provided only to make old code compile.
*/

/*!
    \fn void QMenuBar::setSeparator(Separator sep)

    This function is provided only to make old code compile.
*/

/*!
    \fn QRect QMenuBar::itemRect(int index)

    Use actionGeometry() on the relevant QAction instead.
*/

/*!
    \fn int QMenuBar::itemAtPos(const QPoint &p)

    There is no equivalent way to achieve this in Qt 4.
*/

/*!
    \fn void QMenuBar::activated(int itemId);

    Use triggered() instead.
*/

/*!
    \fn void QMenuBar::highlighted(int itemId);

    Use hovered() instead.
*/

/*!
    \fn void QMenuBar::setFrameRect(QRect)
    \internal
*/

/*!
    \fn QRect QMenuBar::frameRect() const
    \internal
*/
/*!
    \enum QMenuBar::DummyFrame
    \internal

    \value Box
    \value Sunken
    \value Plain
    \value Raised
    \value MShadow
    \value NoFrame
    \value Panel
    \value StyledPanel
    \value HLine
    \value VLine
    \value GroupBoxPanel
    \value WinPanel
    \value ToolBarPanel
    \value MenuBarPanel
    \value PopupPanel
    \value LineEditPanel
    \value TabWidgetPanel
    \value MShape
*/

/*!
    \fn void QMenuBar::setFrameShadow(DummyFrame)
    \internal
*/

/*!
    \fn DummyFrame QMenuBar::frameShadow() const
    \internal
*/

/*!
    \fn void QMenuBar::setFrameShape(DummyFrame)
    \internal
*/

/*!
    \fn DummyFrame QMenuBar::frameShape() const
    \internal
*/

/*!
    \fn void QMenuBar::setFrameStyle(int)
    \internal
*/

/*!
    \fn int QMenuBar::frameStyle() const
    \internal
*/

/*!
    \fn void QMenuBar::setLineWidth(int)
    \internal
*/

/*!
    \fn int QMenuBar::lineWidth() const
    \internal
*/

/*!
    \fn void QMenuBar::setMargin(int margin)
    Sets the width of the margin around the contents of the widget to \a margin.

    Use QWidget::setContentsMargins() instead.
    \sa margin(), QWidget::setContentsMargins()
*/

/*!
    \fn int QMenuBar::margin() const
    Returns the with of the the margin around the contents of the widget.

    Use QWidget::getContentsMargins() instead.
    \sa setMargin(), QWidget::getContentsMargins()
*/

/*!
    \fn void QMenuBar::setMidLineWidth(int)
    \internal
*/

/*!
    \fn int QMenuBar::midLineWidth() const
    \internal
*/

// for private slots

#include <moc_qmenubar.cpp>
#endif // QT_NO_MENUBAR
