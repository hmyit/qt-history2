#include "qmenu.h"
#include "qstyle.h"
#include "qevent.h"
#include "qpainter.h"
#include "qtoolbar.h"
#include "qapplication.h"
#include "qdesktopwidget.h"
#if defined(QT_ACCESSIBILITY_SUPPORT)
# include "qaccessible.h"
#endif
#ifndef QT_NO_EFFECTS
# include <private/qeffects_p.h>
#endif
#ifndef QT_NO_WHATSTHIS
# include <qwhatsthis.h>
#endif

#include "qmenu_p.h"
#define d d_func()
#define q q_func()

/* Q4Menu code */

// These really are QStyle::fu, but for now I just hard code them..
int scrollSizeDistance = 15; //the distance to center within on the desktop
int scrollConsumeAllSpace = false; //toggles when the popupmenu will scroll to fill the screen

// internal class used for the torn off popup
class Q4TornOffMenu : public Q4Menu
{
    Q_OBJECT
public:
    Q4TornOffMenu(Q4Menu *p) : Q4Menu(0)
    {
	d->tornoff = 1;
	d->causedPopup = ((Q4TornOffMenu*)p)->d->causedPopup;

	setParent(p, WType_TopLevel | WStyle_Tool | WDestructiveClose | WStyle_NormalBorder);
	p->setWindowTitle(p->windowTitle());
	setEnabled(p->isEnabled());
	QObject::connect(this, SIGNAL(activated(QAction*)), p, SIGNAL(activated(QAction*)));
	QObject::connect(this, SIGNAL(highlighted(QAction*)), p, SIGNAL(highlighted(QAction*)));
	QList<QAction*> items = p->actions();
	for(int i = 0; i < items.count(); i++)
	    addAction(items.at(i));
    }
    void syncWithMenu(Q4Menu *, QActionEvent *act)
    {
	if(act->type() == QEvent::ActionAdded)
	    insertAction(act->before(), act->action());
	else if(act->type() == QEvent::ActionRemoved)
	    removeAction(act->action());
    }
};
#include "qmenu.moc"

QList<Q4MenuAction*> Q4MenuPrivate::calcActionRects() const
{
    if(!itemsDirty)
	return actionItems;

    QList<Q4MenuAction*> ret;
    QList<QAction*> items = q->actions();
    int max_column_width = 0, dh = QApplication::desktop()->height(), ncols = 1;

    //for compatability now - will have to refactor this away..
    tabWidth = maxIconWidth = 0;
    for(int i = 0; i < items.count(); i++) {
	QAction *action = items.at(i);
	if(!action->isVisible())
	    continue;
	QIconSet is = action->icon();
	if(!is.isNull())
	    maxIconWidth = qMax(maxIconWidth, (uint)is.pixmap(QIconSet::Small, QIconSet::Normal).width() + 4);
    }

    //calculate size
    const QFontMetrics fm = q->fontMetrics();
    for(int i = 0, y = 0; i < items.count(); i++) {
	QAction *action = items.at(i);
	if(!action->isVisible())
	    continue;

	QSize sz;

	//calc what I think the size is..
	if(action->isSeparator()) {
	    sz = QSize(2, 2);
	} else {
	    QString s = action->text();
	    int t = s.indexOf('\t');
	    if(t != -1) {
		tabWidth = qMax((int)tabWidth, fm.width(s.mid(t+1)));
		s = s.left(t);
	    }
	    int w = fm.width( s );
	    w -= s.count('&') * fm.width('&');
	    w += s.count("&&") * fm.width('&');
	    sz.setWidth(w);
	    sz.setHeight(fm.height());

	    QIconSet is = action->icon();
	    if(!is.isNull()) {
		QSize is_sz = is.pixmap(QIconSet::Small, QIconSet::Normal).size();
		if(is_sz.height() > sz.height())
		    sz.setHeight(is_sz.height());
	    }
	}

	//let the style modify the above size..
	sz = q->style().sizeFromContents(QStyle::CT_MenuItem, q, sz, QStyleOption(action, maxIconWidth, 0));

	if(!sz.isEmpty()) {
	    max_column_width = qMax(max_column_width, sz.width());
	    //wrapping
	    if(!scroll && y+sz.height() > dh) {
		ncols++;
		y = 0;
	    }
	    y += sz.height();
	    //append item
	    Q4MenuAction *item = new Q4MenuAction;
	    item->action = action;
	    item->rect = QRect(0, 0, sz.width(), sz.height());
	    ret.append(item);
	}
    }
    if(tabWidth) 
	max_column_width += tabWidth+20; //finally add in the tab width

    //calculate position
    int x = 0, y = 0;
    for(int i = 0; i < ret.count(); i++) {
	Q4MenuAction *action = ret.at(i);
	if(!scroll && y+action->rect.height() > dh) {
	    ncols--;
	    if(ncols < 0)
		qWarning("Q4Menu: Column mismatch calculation. %d", ncols);
	    x += max_column_width;
	    y = 0;
	}
	action->rect.moveBy(x, y);                        //move
	action->rect.setWidth(max_column_width); //uniform width
	y += action->rect.height();
    }
    return ret;
}

void Q4MenuPrivate::updateActions()
{
    if(!itemsDirty)
	return;
    for(QList<Q4MenuAction*>::Iterator it = actionItems.begin(); it != actionItems.end(); ++it)
	delete (*it);
    actionItems = calcActionRects();
    ncols = 1;
    if(!scroll) {
	for(int i = 0, last_left = 0; i < actionItems.count(); i++) {
	    int left = actionItems[i]->rect.left();
	    if(left > last_left) {
		last_left = left;
		ncols++;
	    }
	}
    }
    itemsDirty = 0;
}

QRect Q4MenuPrivate::actionRect(Q4MenuAction *act)
{
    QRect ret = act->rect;
    if(scroll)
	ret.moveBy(0, scroll->scrollOffset);
    if(tearoff)
	ret.moveBy(0, q->style().pixelMetric(QStyle::PM_MenuTearoffHeight, q));
    return ret;
}

void Q4MenuPrivate::hideUpToMenuBar()
{
    if(!tornoff) {
	QWidget *caused = causedPopup;
	q->hide(); //hide after getting causedPopup
	while(caused) {
	    if(Q4MenuBar *mb = qt_cast<Q4MenuBar*>(caused)) {
		mb->d->setCurrentAction(0);
		caused = 0;
	    } else if(Q4Menu *m = qt_cast<Q4Menu*>(caused)) {
		caused = m->d->causedPopup;
		if(!m->d->tornoff)
		    m->hide();
		m->d->setCurrentAction(0);
	    } else {
		qWarning("not possible..");
		caused = 0;
	    }
	}
    }
    setCurrentAction(0);
}

void Q4MenuPrivate::popupAction(Q4MenuAction *action, bool activateFirst)
{
    if(action && action->action->menu()) {
	activeMenu = action->action->menu();
	activeMenu->d->causedPopup = q;
	if(activeMenu->parent() != q)
	    activeMenu->setParent(q, activeMenu->getWFlags());
	QPoint pos(q->width(), actionRect(action).top());
	if(QApplication::reverseLayout())
	    pos.setX(-activeMenu->sizeHint().width());
	activeMenu->popup(q->mapToGlobal(pos));
	if(activateFirst)
	    activeMenu->d->setFirstActionActive();
    }
}

void Q4MenuPrivate::setFirstActionActive()
{
    const int scrollerHeight = q->style().pixelMetric(QStyle::PM_MenuScrollerHeight, q);
    for(int i = 0, saccum = 0; i < actionItems.count(); i++) {
	Q4MenuAction *act = actionItems[i];
	if(scroll && scroll->scrollFlags & Q4MenuScroller::ScrollUp) {
	    saccum -= act->rect.height();
	    if(saccum > scroll->scrollOffset-scrollerHeight)
		continue;
	}
	if(!act->action->isSeparator() &&
	   (q->style().styleHint(QStyle::SH_PopupMenu_AllowActiveAndDisabled, q) || act->action->isEnabled())) {
	    setCurrentAction(act);
	    break;
	}
    }
}

void Q4MenuPrivate::setCurrentAction(Q4MenuAction *action, bool popup, bool activateFirst)
{
    d->tearoffHighlighted = 0;
    if(action == currentAction)
	return;
    if(activeMenu) {
	Q4Menu *menu = activeMenu;
	activeMenu = NULL;
	menu->hide();
    }
    if(currentAction)
	q->update(actionRect(currentAction));

    currentAction = action;
    if(action) {
	action->action->activate(QAction::Hover);
	if(popup)
	    popupAction(d->currentAction, activateFirst);
	q->update(actionRect(action));
    }
}

Q4MenuAction *Q4MenuPrivate::actionAt(QPoint p)
{
    if(!q->rect().contains(p))     //sanity check
       return 0;

    for(int i = 0; i < actionItems.count(); i++) {
	Q4MenuAction *act = actionItems[i];
	if(actionRect(act).contains(p))
	    return act;
    }
    return 0;
}

//actually performs the scrolling
void Q4MenuPrivate::scrollMenu(uint dir)
{
    if(!scroll || !(scroll->scrollFlags & dir)) //not really possible...
	return;

    //figure out how much to offset..
    int soff = 0, scrollHeight = q->style().pixelMetric(QStyle::PM_MenuScrollerHeight, q);
    if(dir == Q4MenuScroller::ScrollUp) {
	for(int i = 0, saccum = 0; i < actionItems.count(); i++) {
	    Q4MenuAction *act = actionItems[i];
	    saccum -= act->rect.height();
	    if(saccum <= scroll->scrollOffset-scrollHeight) {
		soff = saccum - scroll->scrollOffset;
		break;
	    }
	}
    } else if(dir == Q4MenuScroller::ScrollDown) {
	for(int i = 0, saccum = 0; i < actionItems.count(); i++) {
	    Q4MenuAction *act = actionItems[i];
	    saccum += act->rect.height();
	    if(saccum >= -scroll->scrollOffset) {
		saccum = scroll->scrollOffset + saccum;
		int scrollerArea = q->height() - scrollHeight;
		for(i++ ; i < actionItems.count(); i++) {
	            act = actionItems[i];
		    saccum += act->rect.height();
		    if(saccum > scrollerArea) {
			soff = -(scrollerArea - saccum);
			break;
		    }
		}
		break;
	    }
	}
    }

     //we can do resizing magic (ala Panther)
    int dh = QApplication::desktop()->height();
    if(q->height() < dh-(scrollSizeDistance*2)) {
	QRect geom = q->geometry();
	if(dir == Q4MenuScroller::ScrollUp) {
	    geom.setHeight(qMin(geom.height()-soff, dh-(scrollSizeDistance*2)));
	} else if(dir == Q4MenuScroller::ScrollDown) {
	    geom.setTop(qMax(scrollSizeDistance, geom.top()-soff));
	    if(geom != q->geometry())
		soff = 0;
	}
	q->setGeometry(geom);
    }

    //actually offset some things now (if necessary)
    if(soff) {
	scroll->scrollOffset -= soff;
	if(scroll->scrollOffset > 0)
	    scroll->scrollOffset = 0;
    }

    //finally update the scroller status
    scroll->scrollFlags = Q4MenuScroller::ScrollNone;
    if(scroll->scrollOffset) //easy and cheap one
	scroll->scrollFlags |= Q4MenuScroller::ScrollUp;
    for(int i = 0; i < actionItems.count(); i++) {
	if(actionItems[i]->rect.bottom() > q->height()-scroll->scrollOffset) {
	    scroll->scrollFlags |= Q4MenuScroller::ScrollDown;
	    break;
	}
    }

    q->update();     //issue an update so we see all the new state..
}

/* This is poor-mans eventfilters. This avoids the use of
   eventFilter (which can be nasty for users of QMenuBar's). */
bool Q4MenuPrivate::mouseEventTaken(QMouseEvent *e)
{
    tearoffHighlighted = 0;
    QPoint pos = q->mapFromGlobal(e->globalPos());
    if(scroll && !activeMenu) { //let the scroller "steal" the event
	bool isScroll = false;
	if(pos.x() >= 0 && pos.x() < q->width()) {
	    const int scrollerHeight = q->style().pixelMetric(QStyle::PM_MenuScrollerHeight, q);
	    for(int dir = Q4MenuScroller::ScrollUp; dir <= Q4MenuScroller::ScrollDown; dir = dir << 1) {
		if(d->scroll->scrollFlags & dir) {
		    if(dir == Q4MenuScroller::ScrollUp)
			isScroll = (pos.y() <= scrollerHeight);
		    else if(dir == Q4MenuScroller::ScrollDown)
			isScroll = (pos.y() >= q->height()-scrollerHeight);
		    if(isScroll) {
			scroll->scrollDirection = dir;
			break;
		    }
		}
	    }
	}
	if(isScroll) {
	    if(!scroll->scrollTimer)
		scroll->scrollTimer = new QBasicTimer;
	    scroll->scrollTimer->start(50, q);
	    return true;
	} else if(scroll->scrollTimer && scroll->scrollTimer->isActive()) {
	    scroll->scrollTimer->stop();
	}
    }

    if(tearoff) { //let the tear off thingie "steal" the event..
	QRect tearRect(0, 0, q->width(), q->style().pixelMetric(QStyle::PM_MenuTearoffHeight, q));
	if(d->scroll && d->scroll->scrollFlags & Q4MenuPrivate::Q4MenuScroller::ScrollUp)
	    tearRect.moveBy(0, q->style().pixelMetric(QStyle::PM_MenuScrollerHeight, q));
	if(tearRect.contains(pos)) {
	    tearoffHighlighted = 1;
	    if(e->type() == QEvent::MouseButtonRelease) {
		if(tornPopup) {
		    tornPopup->close();
		} else {
		    tornPopup = new Q4TornOffMenu(q);
		    tornPopup->popup(q->pos());
		}
		hideUpToMenuBar();
	    }
	    return true;
	}
    }

    if(q->frameGeometry().contains(e->globalPos())) //otherwise if the event is in our rect we want it..
	return false;

    for(QWidget *caused = causedPopup; caused; ) {
	bool passOnEvent = false;
	QWidget *next_widget = 0;
	QPoint cpos = caused->mapFromGlobal(e->globalPos());
	if(Q4MenuBar *mb = qt_cast<Q4MenuBar*>(caused)) {
	    passOnEvent = mb->rect().contains(cpos);
	} else if(Q4Menu *m = qt_cast<Q4Menu*>(caused)) {
	    passOnEvent = m->d->actionAt(cpos);
	    next_widget = m->d->causedPopup;
	}
	if(passOnEvent) {
	    QMouseEvent new_e(e->type(), cpos, e->button(), e->state());
	    QApplication::sendEvent(caused, &new_e);
	    return true;
	}
	if(!next_widget)
	    break;
	caused = next_widget;
    }
    return false;
}

Q4Menu::Q4Menu(QWidget *parent) : QWidget(*new Q4MenuPrivate, parent, WType_TopLevel|WType_Popup)
{
    setFocusPolicy(StrongFocus);
    setMouseTracking(style().styleHint(QStyle::SH_PopupMenu_MouseTracking));
    if(style().styleHint(QStyle::SH_PopupMenu_Scrollable, this))
	d->scroll = new Q4MenuPrivate::Q4MenuScroller;
}

Q4Menu::~Q4Menu()
{
    if(d->sync)
	qApp->exit_loop();
    if(d->tornPopup)
	d->tornPopup->close();
}

QAction *Q4Menu::addAction(const QString &text)
{
    QAction *ret = new QAction(text);
    addAction(ret);
    return ret;
}

QAction *Q4Menu::addAction(const QIconSet &icon, const QString &text)
{
    QAction *ret = new QAction(icon, text);
    addAction(ret);
    return ret;
}

QAction *Q4Menu::addAction(const QString &text, const QObject *receiver, const char* member)
{
    QAction *ret = new QAction(text, this);
    QObject::connect(ret, SIGNAL(activated()), receiver, member);
    addAction(ret);
    return ret;
}

QAction *Q4Menu::addAction(const QIconSet &icon, const QString &text, const QObject *receiver, const char* member)
{
    QAction *ret = new QAction(icon, text, this);
    QObject::connect(ret, SIGNAL(activated()), receiver, member);
    addAction(ret);
    return ret;
}

QAction *Q4Menu::addMenu(const QString &text, Q4Menu *menu)
{
    QAction *ret = new QAction(text, menu, this);
    addAction(ret);
    return ret;
}

QAction *Q4Menu::addSeparator()
{
    QAction *ret = new QAction(this);
    ret->setSeparator(true);
    addAction(ret);
    return ret;
}

QAction *Q4Menu::insertMenu(QAction *before, const QString &text, Q4Menu *menu)
{
    QAction *ret = new QAction(text, menu, this);
    insertAction(before, ret);
    return ret;
}

QAction *Q4Menu::insertSeparator(QAction *before)
{
    QAction *ret = new QAction(this);
    ret->setSeparator(true);
    insertAction(before, ret);
    return ret;
}

void Q4Menu::setTearOffEnabled(bool b)
{
    if(!b && d->tornPopup)
	d->tornPopup->close();
    d->tearoff = b;
}

bool Q4Menu::isTearOffEnabled() const
{
    return d->tearoff;
}

void Q4Menu::setCheckable(bool b)
{
    d->checkable = b;
}

bool Q4Menu::isCheckable() const
{
    return d->checkable;
}


QSize Q4Menu::sizeHint() const
{
    QSize s(0, 0);
    QList<Q4MenuAction*> actions = d->calcActionRects();
    if(!actions.isEmpty())
	s.setWidth(s.width()+actions[0]->rect.width()); //just take the first
    for(int i = 0; i < actions.count(); ++i) {
	if(actions[i]->rect.right() > s.width())
	    s.setWidth(actions[i]->rect.right());
	s.setHeight(s.height() + actions[i]->rect.height());
    }
    if(d->tearoff)
	s.setHeight(s.height()+style().pixelMetric(QStyle::PM_MenuTearoffHeight, this));
    return (style().sizeFromContents(QStyle::CT_Menu, this, 
				     s.expandedTo(QApplication::globalStrut())));
}

void Q4Menu::popup(const QPoint &p, QAction *atAction)
{
    if(d->scroll) { //reset scroll state from last popup
	d->scroll->scrollOffset = 0;
	d->scroll->scrollFlags = Q4MenuPrivate::Q4MenuScroller::ScrollNone;
    }
    d->tearoffHighlighted = 0;

    d->updateActions();
    QPoint pos = p;
    QSize size = sizeHint();
    QRect screen = QApplication::desktop()->availableGeometry();
    if(d->ncols != 1) {
	pos.setY(0);
    } else if(atAction) {
	for(int i=0, above_height=0; i<(int)d->actionItems.count(); i++) {
	    Q4MenuAction *action = d->actionItems.at(i);
	    if(action->action == atAction) {
		int newY = pos.y()-above_height;
		if(d->scroll && newY < scrollSizeDistance) {
		    d->scroll->scrollFlags |= Q4MenuPrivate::Q4MenuScroller::ScrollUp;
		    d->scroll->scrollOffset = newY;
		    newY = scrollSizeDistance;
		}
		pos.setY(newY);

		if(!scrollConsumeAllSpace) { //make it just high enough to view the rest..
		    int below_height = above_height + d->scroll->scrollOffset;
		    for(int i2 = i; i2 < (int)d->actionItems.count(); i2++)
			below_height += d->actionItems.at(i2)->rect.height();
		    size.setHeight(below_height);
		}
		break;
	    } else {
		above_height += action->rect.height();
	    }
	}
    }
    if(d->scroll && pos.y()+size.height() > screen.height()-(scrollSizeDistance*2)) {
	d->scroll->scrollFlags |= Q4MenuPrivate::Q4MenuScroller::ScrollDown;
	size.setHeight(screen.height()-scrollSizeDistance-pos.y());
    }

    QPoint mouse = QCursor::pos();
    bool snapToMouse = (p == mouse);
    if(snapToMouse) {
	if(qApp->reverseLayout())
	    pos.setY(pos.y()-size.width());
	if(pos.x()+size.width() > screen.right())
	    pos.setX(mouse.x()-size.width());
	if(pos.y()+size.height() > screen.bottom())
	    pos.setY(mouse.y()-size.height());
	if(pos.x() < screen.left())
	    pos.setX(mouse.x());
	if(pos.y() < screen.top())
	    pos.setY(screen.top());
    }
    setGeometry(QRect(pos, size));

#ifndef QT_NO_EFFECTS
    int hGuess = qApp->reverseLayout() ? QEffects::LeftScroll : QEffects::RightScroll;
    int vGuess = QEffects::DownScroll;
    if(qApp->reverseLayout()) {
	if((snapToMouse && (pos.x() + size.width()/2 > mouse.x())) ||
	   (qt_cast<Q4Menu*>(d->causedPopup) && pos.x() + size.width()/2 > d->causedPopup->x()))
	    hGuess = QEffects::RightScroll;
    } else {
	if((snapToMouse && (pos.x() + size.width()/2 < mouse.x())) ||
	   (qt_cast<Q4Menu*>(d->causedPopup) && pos.x() + size.width()/2 < d->causedPopup->x()))
	    hGuess = QEffects::LeftScroll;
    }

#ifndef QT_NO_MENUBAR
    if((snapToMouse && (pos.y() + size.height()/2 < mouse.y())) ||
       (qt_cast<Q4MenuBar*>(d->causedPopup) && pos.y() + size.width()/2 < d->causedPopup->mapToGlobal(d->causedPopup->pos()).y()))
       vGuess = QEffects::UpScroll;
#endif

    if(QApplication::isEffectEnabled(UI_AnimateMenu)) {
	if(QApplication::isEffectEnabled(UI_FadeMenu))
	    qFadeEffect(this);
	else if(d->causedPopup)
	    qScrollEffect(this, qt_cast<Q4Menu*>(d->causedPopup) ? hGuess : vGuess);
	else
	    qScrollEffect(this, hGuess | vGuess);
    } else
#endif
    {
	show();
    }

#if defined(QT_ACCESSIBILITY_SUPPORT)
    QAccessible::updateAccessibility(this, 0, QAccessible::PopupMenuStart);
#endif
}

QAction *Q4Menu::exec(const QPoint &p, QAction *action)
{
    d->sync = 1;
    popup(p, action);
    qApp->enter_loop();

    QAction *ret = d->syncAction;
    d->syncAction = 0;
    d->sync = 0;
    return ret;
}

QAction *Q4Menu::exec(QList<QAction*> actions, const QPoint &pos, QAction *at)
{
    Q4Menu menu;
    for(QList<QAction*>::Iterator it = actions.begin(); it != actions.end(); ++it)
	menu.addAction((*it));
    return menu.exec(pos, at);
}

void Q4Menu::hideEvent(QHideEvent *)
{
    if(d->sync)
	qApp->exit_loop();
    d->setCurrentAction(0);
#if defined(QT_ACCESSIBILITY_SUPPORT)
    QAccessible::updateAccessibility(this, 0, QAccessible::PopupMenuEnd);
#endif
    if(Q4MenuBar *mb = qt_cast<Q4MenuBar*>(d->causedPopup))
	mb->d->setCurrentAction(0);
    d->causedPopup = 0;
}

void Q4Menu::paintEvent(QPaintEvent *e)
{
    d->updateActions();

    QPainter p(this);
    QRegion emptyArea = QRegion(rect());

    //draw the items that need updating..

    for(int i=0; i<(int)d->actionItems.count(); i++) {
	Q4MenuAction *action = d->actionItems.at(i);
	QRect adjustedActionRect = d->actionRect(action);
	if(!e->rect().intersects(adjustedActionRect))
	   continue;

	//set the clip region to be extra safe (and adjust for the scrollers)
	QRegion adjustedActionReg(adjustedActionRect);
	emptyArea -= adjustedActionReg;
	p.setClipRegion(adjustedActionReg);

	QPalette pal = palette();
	QStyle::SFlags flags = QStyle::Style_Default;
	if(isEnabled() && action->action->isEnabled() && (!action->action->menu() || action->action->menu()->isEnabled()))
	    flags |= QStyle::Style_Enabled;
	else
	    pal.setCurrentColorGroup(QPalette::Disabled);
	if(d->currentAction == action)
	    flags |= QStyle::Style_Active;
	if(d->mouseDown)
	    flags |= QStyle::Style_Down;
	style().drawControl(QStyle::CE_MenuItem, &p, this, adjustedActionRect, pal, flags,
			    QStyleOption(action->action, d->maxIconWidth, d->tabWidth));
    }

    //draw the scroller regions..
    if(d->scroll) {
	const int scrollerHeight = style().pixelMetric(QStyle::PM_MenuScrollerHeight, this);
	if(d->scroll->scrollFlags & Q4MenuPrivate::Q4MenuScroller::ScrollUp) {
	    QRect topScroll(0, 0, width(), scrollerHeight);
	    emptyArea -= QRegion(topScroll);
	    p.setClipRect(topScroll);
	    QStyle::SFlags flags = QStyle::Style_Default;
	    //if(d->scroll->scrollDirection & Q4MenuPrivate::Q4MenuScroller::ScrollUp)
	    //flags |= QStyle::Style_Active;
	    style().drawControl(QStyle::CE_MenuScroller, &p, this, topScroll, palette(), flags);
	}
	if(d->scroll->scrollFlags & Q4MenuPrivate::Q4MenuScroller::ScrollDown) {
	    QRect bottomScroll(0, height()-scrollerHeight, width(), scrollerHeight);
	    emptyArea -= QRegion(bottomScroll);
	    p.setClipRect(bottomScroll);
	    QStyle::SFlags flags = QStyle::Style_Down;
	    //if(d->scroll->scrollDirection & Q4MenuPrivate::Q4MenuScroller::ScrollDown)
	    //flags |= QStyle::Style_Active;
	    style().drawControl(QStyle::CE_MenuScroller, &p, this, bottomScroll, palette(), flags);
	}
    }
    //paint the tear off..
    if(d->tearoff) {
	QRect tearRect(0, 0, width(), style().pixelMetric(QStyle::PM_MenuTearoffHeight, this));
	if(d->scroll && d->scroll->scrollFlags & Q4MenuPrivate::Q4MenuScroller::ScrollUp)
	    tearRect.moveBy(0, style().pixelMetric(QStyle::PM_MenuScrollerHeight, this));
	emptyArea -= QRegion(tearRect);
	p.setClipRect(tearRect);
	QStyle::SFlags flags = QStyle::Style_Default;
	if(d->tearoffHighlighted)
	    flags |= QStyle::Style_Active;
	style().drawControl(QStyle::CE_MenuTearoff, &p, this, tearRect, palette(), flags);
    }

    //finally the rest of the space
    p.setClipRegion(emptyArea);
    style().drawControl(QStyle::CE_MenuEmptyArea, &p, this, rect(), palette());
}

void Q4Menu::mousePressEvent(QMouseEvent *e)
{
    if(d->mouseEventTaken(e))
	return;
    if(e->button() != LeftButton)
	return;
    if(!rect().contains(e->pos())) {
	d->hideUpToMenuBar();
	return;
    }
    d->mouseDown = true;

    Q4MenuAction *action = d->actionAt(e->pos());
    d->setCurrentAction(action, true);
}

void Q4Menu::mouseReleaseEvent(QMouseEvent *e)
{
    if(d->mouseEventTaken(e))
	return;
    if(e->button() != LeftButton || !d->mouseDown)
	return;
    d->mouseDown = false;
    Q4MenuAction *action = d->actionAt(e->pos());
    if(d->sync)
	d->syncAction = action ? action->action : 0;
    if(action && action->action->isEnabled()) {
	if(action->action->menu()) {
	    action->action->menu()->d->setFirstActionActive();
	} else {
	    action->action->activate(QAction::Trigger);
	    d->hideUpToMenuBar();
	}
    }
}

void Q4Menu::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::StyleChange) {
	setMouseTracking(style().styleHint(QStyle::SH_PopupMenu_MouseTracking));
	d->itemsDirty = 1;
	if(isVisible())
	    resize(sizeHint());
	if(style().styleHint(QStyle::SH_PopupMenu_Scrollable, this)) {
	    delete d->scroll;
	    d->scroll = 0;
	} else if(!d->scroll) {
	    d->scroll = new Q4MenuPrivate::Q4MenuScroller;
	}
    } else if(e->type() == QEvent::EnabledChange) {
	if (d->tornPopup) // torn-off menu
	    d->tornPopup->setEnabled(isEnabled());
    }
    QWidget::changeEvent(e);
}

bool
Q4Menu::event(QEvent *e)
{
    if(e->type() == QEvent::KeyPress) {
	QKeyEvent *ke = (QKeyEvent*)e;
	if(ke->key() == Key_Tab || ke->key() == Key_BackTab) {
	    keyPressEvent(ke);
	    return true;
	}
    }
    return QWidget::event(e);
}


void Q4Menu::keyPressEvent(QKeyEvent *e)
{
    int key = e->key();
    if(QApplication::reverseLayout()) {  // in reverse mode open/close key for submenues are reversed
	if(key == Key_Left)
	    key = Key_Right;
	else if(key == Key_Right)
	    key = Key_Left;
    }
    if(key == Key_Tab) //means down
	key = Key_Down;

    bool key_consumed = false;
    switch(key) {
    case Key_Up:
    case Key_Down: {
	Q4MenuAction *nextAction = 0;
	uint scroll_direction = Q4MenuPrivate::Q4MenuScroller::ScrollNone;
	if(!d->currentAction) {
	    nextAction = d->actionItems.first();
	} else {
	    for(int i=0, y=0; i<(int)d->actionItems.count(); i++) {
		Q4MenuAction *act = d->actionItems.at(i);
		if(act == d->currentAction) {
		    if(key == Key_Up) {
			for(int next_i = i+1; true; next_i++) {
			    if(next_i == d->actionItems.count())
				next_i = 0;
			    Q4MenuAction *next = d->actionItems.at(next_i);
			    if(next == d->currentAction)
				break;
			    if(next->action->isSeparator() ||
			       (!next->action->isEnabled() && 
				!style().styleHint(QStyle::SH_PopupMenu_AllowActiveAndDisabled, this)))
				continue;
			    nextAction = next;
			    if(d->scroll && (d->scroll->scrollFlags & Q4MenuPrivate::Q4MenuScroller::ScrollUp)) {
				int topVisible = style().pixelMetric(QStyle::PM_MenuScrollerHeight, this);
				if(d->tearoff)
				    topVisible += style().pixelMetric(QStyle::PM_MenuTearoffHeight, q);
				if(((y + d->scroll->scrollOffset) - topVisible) < act->rect.height())
				    scroll_direction = Q4MenuPrivate::Q4MenuScroller::ScrollUp;
			    }
			}
			if(!nextAction && d->tearoff)
			    d->tearoffHighlighted = 1;
		    } else {
			for(int next_i = i-1; true; next_i--) {
			    if(next_i == -1)
				next_i = d->actionItems.count()-1;
			    Q4MenuAction *next = d->actionItems.at(next_i);
			    if(next == d->currentAction)
				break;
			    if(next->action->isSeparator() ||
			       (!next->action->isEnabled() && 
				!style().styleHint(QStyle::SH_PopupMenu_AllowActiveAndDisabled, this)))
				continue;
			    nextAction = next;
			    if(d->scroll && (d->scroll->scrollFlags & Q4MenuPrivate::Q4MenuScroller::ScrollDown)) {
				const int scrollerHeight = style().pixelMetric(QStyle::PM_MenuScrollerHeight, this);
				int bottomVisible = height()-scrollerHeight;
				if(d->scroll->scrollFlags & Q4MenuPrivate::Q4MenuScroller::ScrollUp)
				    bottomVisible -= scrollerHeight;
				if(d->tearoff)
				    bottomVisible -= style().pixelMetric(QStyle::PM_MenuTearoffHeight, q);
				if((y + d->scroll->scrollOffset + act->rect.height()) > bottomVisible)
				    scroll_direction = Q4MenuPrivate::Q4MenuScroller::ScrollDown;
			    }
			}
		    }
		    break;
		}
		y += act->rect.height();
	    }
	}
	if(nextAction) {
	    if(d->scroll && scroll_direction != Q4MenuPrivate::Q4MenuScroller::ScrollNone) {
		if(d->scroll->scrollTimer)
		    d->scroll->scrollTimer->stop();
		d->scrollMenu(scroll_direction);
	    }
	    d->setCurrentAction(nextAction);
	    key_consumed = true;
	}
	break; }

    case Key_Right:
	if(d->currentAction && d->currentAction->action->isEnabled() && d->currentAction->action->menu()) {
	    d->popupAction(d->currentAction, true);
	    key_consumed = true;
	    break;
	}
	//FALL THROUGH
    case Key_Left: {
	if(key == Key_Left && d->causedPopup && qt_cast<Q4Menu*>(d->causedPopup)) {
	    hide();
	    key_consumed = true;
	} else if(d->currentAction && !d->scroll) {
	    Q4MenuAction *nextAction = 0;
	    if(key == Key_Left) {
		QRect actionR = d->actionRect(d->currentAction);
		for(int x = actionR.left()-1; !nextAction && x >= 0; x--)
		    nextAction = d->actionAt(QPoint(x, actionR.center().y()));
	    } else {
		QRect actionR = d->actionRect(d->currentAction);
		for(int x = actionR.right()+1; !nextAction && x < width(); x++)
		    nextAction = d->actionAt(QPoint(x, actionR.center().y()));
	    }
	    if(nextAction) {
		d->setCurrentAction(nextAction);
		key_consumed = true;
	    }
	}
	break; }

    case Key_Alt:
	key_consumed = true;
	break;

    case Key_Escape:
	key_consumed = true;
	if(d->tornoff) {
	    close();
	    return;
	}
	hide();
	break;

    case Key_Space:
	if(!style().styleHint(QStyle::SH_PopupMenu_SpaceActivatesItem, this))
	    break;
	// for motif, fall through
    case Key_Return:
    case Key_Enter: {
	    if(!d->currentAction)
		break;
#ifndef QT_NO_WHATSTHIS
	    bool whats_this_mode = QWhatsThis::inWhatsThisMode();
#else
	    const bool whats_this_mode = FALSE;
#endif
	    if(!d->currentAction->action->isEnabled() && !whats_this_mode)
		break;
	    if(d->currentAction->action->menu()) {
		d->popupAction(d->currentAction, true);
	    } else {
		d->currentAction->action->activate(QAction::Trigger);
		d->hideUpToMenuBar();
	    }
	    key_consumed = true;
	    break; }

#ifndef QT_NO_WHATSTHIS
    case Key_F1:
	if(!d->currentAction || d->currentAction->action->whatsThis().isNull())
	    break;
	if(!QWhatsThis::inWhatsThisMode())
	    QWhatsThis::enterWhatsThisMode();
	QWhatsThis::leaveWhatsThisMode();
	//fall-through!
#endif
    default:
	key_consumed = false;
    }

    if(!key_consumed) {				// send to menu bar
	if(QWidget *caused = d->causedPopup) {
	    while(Q4Menu *m = qt_cast<Q4Menu*>(caused))
		caused = m->d->causedPopup;
	    if(Q4MenuBar *mb = qt_cast<Q4MenuBar*>(caused)) {
		Q4MenuAction *oldAct = mb->d->currentAction;
		QApplication::sendEvent(mb, e);
		if(mb->d->currentAction != oldAct)
		    key_consumed = true;
	    }
	}


	if(!key_consumed && (!e->state() || e->state() == AltButton || e->state() == ShiftButton) && e->text().length()==1) {
	    int clashCount = 0;
	    Q4MenuAction *first = 0, *currentSelected = 0, *firstAfterCurrent = 0;
	    {
		QChar c = e->text()[0].toUpper();
		for(int i = 0; i < d->actionItems.size(); ++i) {
		    register Q4MenuAction *act = d->actionItems.at(i);
		    QString s = act->action->text();
		    if(!s.isEmpty()) {
			int ampersand = s.indexOf('&');
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
	    Q4MenuAction *next_action = 0;
	    if(clashCount >= 1) {
		if(clashCount == 1 || !d->currentAction || (currentSelected && !firstAfterCurrent))
		    next_action = first;
		else
		    next_action = firstAfterCurrent;
	    }
	    if(next_action) {
		d->setCurrentAction(next_action, true, true);
		if(!next_action->action->menu()) {
		    next_action->action->activate(QAction::Trigger);
		    d->hideUpToMenuBar();
		}
	    }
	}
#ifdef Q_OS_WIN32
	if (key_consumed && (e->key() == Key_Control || e->key() == Key_Shift || e->key() == Key_Meta))
	    qApp->beep();
#endif // Q_OS_WIN32
    }
}

void Q4Menu::mouseMoveEvent(QMouseEvent *e)
{
    if(d->mouseEventTaken(e))
	return;
    d->mouseDown = e->state() & LeftButton;

    Q4MenuAction *action = d->actionAt(e->pos());
    d->setCurrentAction(action, true);
}

void Q4Menu::leaveEvent(QEvent *)
{
#if 0
    if(d->tornoff)
	d->setCurrentAction(0);
#endif
}

void
Q4Menu::timerEvent(QTimerEvent *e)
{
    if(d->scroll && d->scroll->scrollTimer && d->scroll->scrollTimer->timerId() == e->timerId()) {
	d->scrollMenu(d->scroll->scrollDirection);
	if(d->scroll->scrollFlags == Q4MenuPrivate::Q4MenuScroller::ScrollNone)
	    d->scroll->scrollTimer->stop();
    }
}

void Q4Menu::actionEvent(QActionEvent *e)
{
    if(e->type() == QEvent::ActionAdded) {
	QObject::connect(e->action(), SIGNAL(triggered()), this, SLOT(internalActionActivated()));
	QObject::connect(e->action(), SIGNAL(hovered()), this, SLOT(internalActionHighlighted()));
    } else if(e->type() == QEvent::ActionRemoved) {
	QObject::disconnect(e->action(), SIGNAL(triggered()), this, SLOT(internalActionActivated()));
	QObject::disconnect(e->action(), SIGNAL(hovered()), this, SLOT(internalActionHighlighted()));
    }
    if(d->tornPopup)
	d->tornPopup->syncWithMenu(this, e);
    d->itemsDirty = 1;
    update();
}

void Q4Menu::internalActionActivated()
{
    QAction *action = qt_cast<QAction*>(sender());
    if(!action)
	qWarning("not possible..");
    emit activated(action);

    QWidget *caused = d->causedPopup;
    while(caused) {
	if(Q4MenuBar *mb = qt_cast<Q4MenuBar*>(caused)) {
	    emit mb->activated(action);
	    caused = 0;
	} else if(Q4Menu *m = qt_cast<Q4Menu*>(caused)) {
	    caused = m->d->causedPopup;
	    emit m->activated(action);
	} else {
	    qWarning("not possible..");
	    caused = 0;
	}
    }
}

void Q4Menu::internalActionHighlighted()
{
    QAction *action = qt_cast<QAction*>(sender());
    if(!action)
	qWarning("not possible..");
    emit highlighted(action);
#if defined(QT_ACCESSIBILITY_SUPPORT)
    {
	int actionID = d->indexOf(action);
	QAccessible::updateAccessibility(this, actionID, QAccessible::Focus);
	QAccessible::updateAccessibility(this, actionID, QAccessible::Selection);
    }
#endif

    QWidget *caused = d->causedPopup;
    while(caused) {
	if(Q4MenuBar *mb = qt_cast<Q4MenuBar*>(caused)) {
	    emit mb->highlighted(action);
	    caused = 0;
	} else if(Q4Menu *m = qt_cast<Q4Menu*>(caused)) {
	    caused = m->d->causedPopup;
	    emit m->highlighted(action);
	} else {
	    qWarning("not possible..");
	    caused = 0;
	}
    }
}

/* Q4Menubar code */
Q4MenuAction *Q4MenuBarPrivate::actionAt(QPoint p)
{
    for(int i = 0; i < actionItems.count(); i++) {
	Q4MenuAction *act = actionItems[i];
	if(act->rect.contains(p))
	    return act;
    }
    return 0;
}

void Q4MenuBarPrivate::updateActions()
{
    int q_width = q->width();
    if(!itemsDirty && itemsWidth == q_width)
	return;
    actionItems = calcActionRects(q_width);
    itemsWidth = q_width;
#ifndef QT_NO_ACCEL
    if(itemsDirty) {
	delete shortcuts;
	shortcuts = new QAccel(q);
	QObject::connect(shortcuts, SIGNAL(activated(int)), q, SLOT(internalShortcutActivated(int)));
	for(int i = 0; i < actionItems.count(); i++) {
	    QKeySequence key = QAccel::shortcutKey(actionItems.at(i)->action->text());
	    if(!key.isEmpty())
		shortcuts->insertItem(key);
	}
    }
#endif
    itemsDirty = 0;
}

/* I may not really need this function, but I've added it for now so that it
   is easy to translate code from the menu - I will probably end up removing it
   later */
QRect Q4MenuBarPrivate::actionRect(Q4MenuAction *act)
{
    return act->rect;
}

void Q4MenuBarPrivate::setKeyboardMode(bool b)
{
    d->altPressed = false;
    d->keyboardState = b;
    if(b) {
	d->keyboardFocusWidget = qApp->focusWidget();
	q->setFocus();
    } else {
	if(d->keyboardFocusWidget) {
	    d->keyboardFocusWidget->setFocus();
	    d->keyboardFocusWidget = 0;
	}
    }
}

void Q4MenuBarPrivate::popupAction(Q4MenuAction *action, bool activateFirst)
{
    popupState = true;
    if(action && action->action->menu()) {
	d->closePopupMode = 0;
	activeMenu = action->action->menu();
	activeMenu->d->causedPopup = q;
	if(activeMenu->parent() != q)
	    activeMenu->setParent(q, activeMenu->getWFlags());
	QRect adjustedActionRect = actionRect(action);
	QPoint pos(adjustedActionRect.left(), adjustedActionRect.bottom());
	if(QApplication::reverseLayout())
	    pos.setX((pos.x()+adjustedActionRect.width())-activeMenu->sizeHint().width());
	activeMenu->popup(q->mapToGlobal(pos));
	if(activateFirst)
	    activeMenu->d->setFirstActionActive();
	q->update(actionRect(action));
    }
}

void Q4MenuBarPrivate::setCurrentAction(Q4MenuAction *action, bool popup, bool activateFirst)
{
    if(currentAction == action && popup == popupState)
	return;
    if(activeMenu) {
	Q4Menu *menu = activeMenu;
	activeMenu = NULL;
	menu->hide();
    }
    if(currentAction)
	q->update(actionRect(currentAction));

    popupState = popup;
    currentAction = action;
    if(action) {
	action->action->activate(QAction::Hover);
	if(popup)
	    popupAction(action, activateFirst);
	q->update(actionRect(action));
    }
}

QList<Q4MenuAction*> Q4MenuBarPrivate::calcActionRects(int max_width) const
{
    if(!itemsDirty && itemsWidth == max_width)
	return actionItems;
    QList<Q4MenuAction*> ret;
    int max_item_height = 0, separator = -1, separator_start = 0, separator_len = 0;
    QList<QAction*> items = q->actions();

    //calculate size
    const QFontMetrics fm = q->fontMetrics();
    for(int i = 0; i < items.count(); i++) {
	QAction *action = items.at(i);
	if(!action->isVisible())
	    continue;

	QSize sz;

	//calc what I think the size is..
	if(action->isSeparator()) {
	    if(q->style().styleHint(QStyle::SH_GUIStyle, q) == MotifStyle)
		separator = ret.count();
	    continue; //we don't really position these!
	} else {
	    QString s = action->text();
	    int w = fm.width( s );
	    w -= s.count('&') * fm.width('&');
	    w += s.count("&&") * fm.width('&');
	    sz.setWidth(w);
	    sz.setHeight(fm.height());
	}

	//let the style modify the above size..
	sz = q->style().sizeFromContents(QStyle::CT_MenuBarItem, q, sz, QStyleOption(action));

	if(!sz.isEmpty()) {
	    if(separator == -1)
		separator_start += sz.width();
	    else
		separator_len += sz.width();
	    max_item_height = qMax(max_item_height, sz.height());
	    //append
	    Q4MenuAction *item = new Q4MenuAction;
	    item->action = action;
	    item->rect = QRect(0, 0, sz.width(), sz.height());
	    ret.append(item);
	}
    }

    //calculate position
    int x = 0, y = 0;
    const bool reverse = QApplication::reverseLayout();
    const int itemSpacing = q->style().pixelMetric(QStyle::PM_MenuBarItemSpacing, q);
    for(int i = 0; i != ret.count(); i++) {
	Q4MenuAction *item = ret.at(i);
	//resize
	item->rect.setHeight(max_item_height);

	//move
	if(separator != -1 && i >= separator) { //after the separator
	    int left = (max_width - separator_len) + (x - separator_start);
	    if(left < separator_start) { //wrap
		separator_start = x = 0;
		y += max_item_height;
	    }
	    item->rect.moveLeft(left);
	} else {
	    if(x+item->rect.width() >= max_width) { //wrap
		y += max_item_height;
		separator_start -= x;
		x = 0;
	    } else {
		item->rect.moveLeft(x);
	    }
	}
	if(reverse)
	    item->rect.moveLeft(max_width - item->rect.right());
	item->rect.moveTop(y);

	//keep moving along..
	x += item->rect.width() + itemSpacing; 
    }
    return ret;
}

Q4MenuBar::Q4MenuBar(QWidget *parent) : QWidget(*new Q4MenuBarPrivate, parent, 0)
{
    topLevelWidget()->installEventFilter(this); //grab accels
    setMouseTracking(style().styleHint(QStyle::SH_MenuBar_MouseTracking));
}

Q4MenuBar::~Q4MenuBar()
{
}

QAction *Q4MenuBar::addMenu(const QString &text, Q4Menu *menu)
{
    QAction *ret = new QAction(text, menu);
    addAction(ret);
    return ret;
}

QAction *Q4MenuBar::insertMenu(QAction *before, const QString &text, Q4Menu *menu)
{
    QAction *ret = new QAction(text, menu);
    insertAction(before, ret);
    return ret;
}

void Q4MenuBar::resizeEvent(QResizeEvent *)
{
    d->itemsDirty = 1;
}

void Q4MenuBar::paintEvent(QPaintEvent *e)
{
    d->updateActions();

    QPainter p(this);
    QRegion emptyArea(rect());

    //draw the items
    for(int i=0; i<(int)d->actionItems.count(); i++) {
	Q4MenuAction *action = d->actionItems.at(i);
	QRect adjustedActionRect = d->actionRect(action);
	if(!e->rect().intersects(adjustedActionRect))
	   continue;

	QPalette pal = palette();
	QStyle::SFlags flags = QStyle::Style_Default;
	if(isEnabled() && action->action->isEnabled() )
	    flags |= QStyle::Style_Enabled;
	else
	    pal.setCurrentColorGroup(QPalette::Disabled);
	if(d->currentAction == action) {
	    flags |= QStyle::Style_Active;
	    if(d->popupState && !d->closePopupMode)
		flags |= QStyle::Style_Down;
	}
	if(hasFocus() || d->currentAction)
	    flags |= QStyle::Style_HasFocus;
	emptyArea -= adjustedActionRect;
	p.setClipRect(adjustedActionRect);
	style().drawControl(QStyle::CE_MenuBarItem, &p, this,
			    adjustedActionRect, pal, flags, QStyleOption(action->action));
    }
    p.setClipRegion(emptyArea);
    style().drawControl(QStyle::CE_MenuBarEmptyArea, &p, this, rect(), palette());
}

void Q4MenuBar::mousePressEvent(QMouseEvent *e)
{
    if(e->button() != LeftButton)
	return;
    d->mouseDown = true;
    Q4MenuAction *action = d->actionAt(e->pos());
    if(d->currentAction == action && d->popupState) {
	if((d->closePopupMode = (style().styleHint(QStyle::SH_GUIStyle) == WindowsStyle)))
	    q->update(d->actionRect(action));
    } else {
	d->setCurrentAction(action, true);
    }
}

void Q4MenuBar::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() != LeftButton || !d->mouseDown)
	return;
    d->mouseDown = false;
    Q4MenuAction *action = d->actionAt(e->pos());
    if((d->closePopupMode && action == d->currentAction) || !action || !action->action->menu()) {
	if(action) 
	    action->action->activate(QAction::Trigger);
	d->setCurrentAction(action, false);
    }
    d->closePopupMode = 0;
}

void Q4MenuBar::keyPressEvent(QKeyEvent *e)
{
    int key = e->key();
    if(QApplication::reverseLayout()) {  // in reverse mode open/close key for submenues are reversed
	if(key == Key_Left)
	    key = Key_Right;
	else if(key == Key_Right)
	    key = Key_Left;
    }
    if(key == Key_Tab) //means right
	key = Key_Right;

    bool key_consumed = false;
    switch(key) {
    case Key_Up:
    case Key_Down:
    case Key_Enter:
    case Key_Space:
    case Key_Return: {
	if(!style().styleHint(QStyle::SH_MenuBar_AltKeyNavigation, this) || !d->currentAction)
	   break;
	if(d->currentAction->action->menu()) {
	    d->popupAction(d->currentAction, true);
	} else if(key == Key_Enter || key == Key_Return | key == Key_Space) {
	    d->currentAction->action->activate(QAction::Trigger);
	    d->setCurrentAction(d->currentAction, false);
	}
	key_consumed = true;
	break; }

    case Key_Right:
    case Key_Left: {
	if(d->currentAction) {
	    Q4MenuAction *nextAction = 0;
	    for(int i=0; i<(int)d->actionItems.count(); i++) {
		if(d->actionItems.at(i) == d->currentAction) {
		    if(key == Key_Left) {
			if(i > 0)
			    nextAction = d->actionItems.at(i-1);
		    } else {
			if(i < d->actionItems.count()-1)
			    nextAction = d->actionItems.at(i+1);
		    }
		    break;
		}
	    }
	    if(!nextAction) {
		if(key == Key_Left)
		    nextAction = d->actionItems.last();
		else
		    nextAction = d->actionItems.first();
	    }
	    if(nextAction) {
		d->setCurrentAction(nextAction, d->popupState, true);
		key_consumed = true;
	    }
	}
	break; }

    case Key_Escape:
	d->setCurrentAction(0);
	d->setKeyboardMode(false);
	key_consumed = true;
	break;

    default:
	key_consumed = false;
    }

    if(!key_consumed && 
       (!e->state() || (e->state()&(MetaButton|AltButton))) && e->text().length()==1 && !d->popupState) {
	int clashCount = 0;
	Q4MenuAction *first = 0, *currentSelected = 0, *firstAfterCurrent = 0;
	{
	    QChar c = e->text()[0].toUpper();
	    for(int i = 0; i < d->actionItems.size(); ++i) {
		register Q4MenuAction *act = d->actionItems.at(i);
		QString s = act->action->text();
		if(!s.isEmpty()) {
		    int ampersand = s.indexOf('&');
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
	Q4MenuAction *next_action = 0;
	if(clashCount >= 1) {
	    if(clashCount == 1 || !d->currentAction || (currentSelected && !firstAfterCurrent))
		next_action = first;
	    else
		next_action = firstAfterCurrent;
	}
	if(next_action) 
	    d->setCurrentAction(next_action, true, true);
    }
}

void Q4MenuBar::mouseMoveEvent(QMouseEvent *e)
{
    d->mouseDown = e->state() & LeftButton;
    Q4MenuAction *action = d->actionAt(e->pos());
    bool popupState = d->popupState || d->mouseDown;
    if(action || !popupState) 
	d->setCurrentAction(action, popupState);
}

void Q4MenuBar::leaveEvent(QEvent *)
{
    if(!hasFocus() && !d->popupState)
	d->setCurrentAction(0);
}

void Q4MenuBar::actionEvent(QActionEvent *e)
{
    if(e->type() == QEvent::ActionAdded) {
	QObject::connect(e->action(), SIGNAL(triggered()), this, SLOT(internalActionActivated()));
	QObject::connect(e->action(), SIGNAL(hovered()), this, SLOT(internalActionHighlighted()));
    } else if(e->type() == QEvent::ActionRemoved) {
	QObject::disconnect(e->action(), SIGNAL(triggered()), this, SLOT(internalActionActivated()));
	QObject::disconnect(e->action(), SIGNAL(hovered()), this, SLOT(internalActionHighlighted()));
    }
    d->itemsDirty = 1;
    update();
}

void
Q4MenuBar::focusInEvent(QFocusEvent *)
{
    if(!d->currentAction && !d->actionItems.isEmpty())
	d->setCurrentAction(d->actionItems.first());
}

void
Q4MenuBar::focusOutEvent(QFocusEvent *)
{
    if(!d->popupState) {
	d->setCurrentAction(0);
	d->setKeyboardMode(false);
    }
}

void Q4MenuBar::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::StyleChange) {
	setMouseTracking(style().styleHint(QStyle::SH_MenuBar_MouseTracking));
	d->itemsDirty = 1;
    }
}

bool
Q4MenuBar::event(QEvent *e)
{
    if(e->type() == QEvent::KeyPress) {
	QKeyEvent *ke = (QKeyEvent*)e;
#if 0
	if(!d->keyboardState) { //all keypresses..
	    d->setCurrentAction(0);
	    return ;
	}
#endif
	if(ke->key() == Key_Tab || ke->key() == Key_BackTab) {
	    keyPressEvent(ke);
	    return true;
	}

    }
    return QWidget::event(e);
}

bool
Q4MenuBar::eventFilter(QObject *object, QEvent *event)
{
    if(!isVisible() || !object->isWidgetType())
	return false;

    if(event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
	d->altPressed = false;
	return false;
    } else if(d->altPressed && event->type() == QEvent::FocusOut) {
	// some window systems/managers use alt/meta as accelerator keys
	// for switching between windows/desktops/etc.  If the focus
	// widget gets unfocused, then we need to stop waiting for alt release
	d->altPressed = false;
	QWidget *f = ((QWidget *)object)->focusWidget();
	if(f && !f->isTopLevel())
	    f->removeEventFilter(this);
	return false;
    } else if(!(event->type() == QEvent::AccelOverride ||
		event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) ||
	      !style().styleHint(QStyle::SH_MenuBar_AltKeyNavigation, this)) {
	return false;
    }

    QWidget *widget = (QWidget *)object;
    QKeyEvent *ke = (QKeyEvent *)event;
#ifndef QT_NO_ACCEL
    // look for Alt press and Alt-anything press
    if(event->type() == QEvent::Accel || event->type() == QEvent::KeyPress) {
	QWidget *f = widget->focusWidget();
	// ### this thinks alt and meta are the same
	if(ke->key() == Key_Alt || ke->key() == Key_Meta) {
	    if(d->altPressed) { //eat first alt
		d->altPressed = false;
		if(!widget->isTopLevel())
		    object->removeEventFilter(this);
		ke->accept();
		return true;
	    } else if(hasFocus()) { 	    // Menu has focus, send focus back
		d->setKeyboardMode(false);
		ke->accept();
		return true;
	    } else if(ke->stateAfter() == AltButton) {  // Start waiting for Alt release on focus widget
		d->altPressed = true;
		if(f && f != object)
		    f->installEventFilter(this);
	    }
	} else if(ke->key() == Key_Control || ke->key() == Key_Shift) {	// Other modifiers kills focus on menubar
	    d->setKeyboardMode(false);
	} else { 	// Got other key, no need to wait for Alt release
	    d->altPressed = false;
	}
	d->setCurrentAction(0);
	return false;
    }
#endif
    if(((QWidget*)object)->focusWidget() == object || (object->parent() == 0 && ((QWidget*)object)->focusWidget() == 0)) {
	if(d->altPressed && event->type() == QEvent::KeyRelease && (ke->key() == Key_Alt || ke->key() == Key_Meta)) {    //alt release
	    d->setKeyboardMode(true);
	    if(!widget->isTopLevel())
		object->removeEventFilter(this);
	    return true;
	} else if(!hasFocus() && (event->type() == QEvent::AccelOverride) &&
		  !(((QKeyEvent *)event)->key() == Key_Alt || ((QKeyEvent *)event)->key() == Key_Meta)) { 	// Cancel if next keypress is NOT Alt/Meta,
	    if(!widget->isTopLevel())
		object->removeEventFilter(this);
	    d->setKeyboardMode(false);
	}
    }
    return false;
}

QSize Q4MenuBar::sizeHint() const
{
    const_cast<Q4MenuBar*>(this)->d->updateActions();
    QSize s(0, height());
    for(int i = 0; i < d->actionItems.count(); ++i) {
	QRect actionRect(d->actionItems[i]->rect);
	s.setWidth(s.width() + actionRect.width() + 2);
	if(actionRect.bottom() > s.height())
	    s.setHeight(actionRect.bottom());
    }
    return (style().sizeFromContents(QStyle::CT_MenuBar, this, 
				     s.expandedTo(QApplication::globalStrut())));
}

QSize Q4MenuBar::minimumSizeHint() const
{
#ifndef QT_NO_TOOLBAR
    QToolBar *tb = qt_cast<QToolBar*>(parent());
    if(tb)
	return sizeHint();
#endif
    return QWidget::minimumSizeHint();
}

int Q4MenuBar::heightForWidth(int max_width) const
{
    QList<Q4MenuAction*> actions = d->calcActionRects(max_width);
    int height = 0;
    for(int i = 0; i < actions.count(); ++i)
	height = qMax(height, actions[i]->rect.bottom());
    return height;
}

void Q4MenuBar::internalShortcutActivated(int id)
{
#ifndef QT_NO_ACCEL
    QKeySequence key = d->shortcuts->key(id);
    for(int i = 0; i < d->actionItems.count(); i++) {
	Q4MenuAction *act = d->actionItems.at(i);
	if(QAccel::shortcutKey(act->action->text()) == key) {
	    d->setCurrentAction(act, true);
	    break;
	}
    }
#endif
}
void Q4MenuBar::internalActionActivated()
{
    QAction *action = qt_cast<QAction*>(sender());
    if(!action)
	qWarning("not possible..");
    emit activated(action);
}

void Q4MenuBar::internalActionHighlighted()
{
    QAction *action = qt_cast<QAction*>(sender());
    if(!action)
	qWarning("not possible..");
    emit highlighted(action);
}

