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

#include "q3toolbar.h"
#ifndef QT_NO_TOOLBAR

#include "q3mainwindow.h"
#include "qapplication.h"
#include "qcombobox.h"
#include "qcursor.h"
#include "qdesktopwidget.h"
#include "qdrawutil.h"
#include "qevent.h"
#include "qframe.h"
#include "qlayout.h"
#include "qmap.h"
#include "qpainter.h"
#include "qpopupmenu.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qtimer.h"
#include "qtoolbutton.h"
#include "qtooltip.h"

static const char * const arrow_v_xpm[] = {
    "7 9 3 1",
    "            c None",
    ".            c #000000",
    "+            c none",
    ".+++++.",
    "..+++..",
    "+..+..+",
    "++...++",
    ".++.++.",
    "..+++..",
    "+..+..+",
    "++...++",
    "+++.+++"};

static const char * const arrow_h_xpm[] = {
    "9 7 3 1",
    "            c None",
    ".            c #000000",
    "+            c none",
    "..++..+++",
    "+..++..++",
    "++..++..+",
    "+++..++..",
    "++..++..+",
    "+..++..++",
    "..++..+++"};

class Q3ToolBarExtensionWidget;

class Q3ToolBarPrivate
{
public:
    Q3ToolBarPrivate() : moving(false) {
    }

    bool moving;
    Q3ToolBarExtensionWidget *extension;
    QPopupMenu *extensionPopup;

    QMap<QAction *, QWidget *> actions;
};


class Q3ToolBarSeparator : public QWidget
{
    Q_OBJECT
public:
    Q3ToolBarSeparator(Qt::Orientation, Q3ToolBar *parent, const char* name=0);

    QSize sizeHint() const;
    Qt::Orientation orientation() const { return orient; }
public slots:
    void setOrientation(Qt::Orientation);
protected:
    void changeEvent(QEvent *);
    void paintEvent(QPaintEvent *);

private:
    Qt::Orientation orient;
};

class Q3ToolBarExtensionWidget : public QWidget
{
    Q_OBJECT

public:
    Q3ToolBarExtensionWidget(QWidget *w);
    void setOrientation(Qt::Orientation o);
    QToolButton *button() const { return tb; }

protected:
    void resizeEvent(QResizeEvent *e) {
        QWidget::resizeEvent(e);
        layOut();
    }

private:
    void layOut();
    QToolButton *tb;
    Qt::Orientation orient;

};

Q3ToolBarExtensionWidget::Q3ToolBarExtensionWidget(QWidget *w)
    : QWidget(w, "qt_dockwidget_internal")
{
    tb = new QToolButton(this, "qt_toolbar_ext_button");
    tb->setAutoRaise(true);
    setOrientation(Qt::Horizontal);
}

void Q3ToolBarExtensionWidget::setOrientation(Qt::Orientation o)
{
    orient = o;
    if (orient == Qt::Horizontal)
        tb->setIcon(QPixmap((const char **)arrow_h_xpm));
    else
        tb->setIcon(QPixmap((const char **)arrow_v_xpm));
    layOut();
}

void Q3ToolBarExtensionWidget::layOut()
{
    tb->setGeometry(2, 2, width() - 4, height() - 4);
}

Q3ToolBarSeparator::Q3ToolBarSeparator(Qt::Orientation o , Q3ToolBar *parent,
                                     const char* name)
    : QWidget(parent, name)
{
    connect(parent, SIGNAL(orientationChanged(Qt::Orientation)),
             this, SLOT(setOrientation(Qt::Orientation)));
    setOrientation(o);
    setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
}



void Q3ToolBarSeparator::setOrientation(Qt::Orientation o)
{
    orient = o;
}

void Q3ToolBarSeparator::changeEvent(QEvent *ev)
{
    if(ev->type() == QEvent::StyleChange)
        setOrientation(orient);
    QWidget::changeEvent(ev);
}

static QStyleOption getStyleOption(const Q3ToolBarSeparator *tbs)
{
    QStyleOption opt(0);
    opt.rect = tbs->rect();
    opt.palette = tbs->palette();
    if (tbs->orientation() == Qt::Horizontal)
        opt.state = QStyle::Style_Horizontal;
    else
        opt.state = QStyle::Style_None;
    return opt;
}

QSize Q3ToolBarSeparator::sizeHint() const
{
    QStyleOption opt = getStyleOption(this);
    int extent = style().pixelMetric(QStyle::PM_DockWindowSeparatorExtent, &opt, this);
    if (orient == Qt::Horizontal)
        return QSize(extent, 0);
    else
        return QSize(0, extent);
}

void Q3ToolBarSeparator::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QStyleOption opt = getStyleOption(this);
    style().drawPrimitive(QStyle::PE_DockWindowSeparator, &opt, &p, this);
}

#include "q3toolbar.moc"


/*!
    \class Q3ToolBar
    \brief The Q3ToolBar class provides a movable panel containing
    widgets such as tool buttons.

    \compat

    A toolbar is a panel that contains a set of controls, usually
    represented by small icons. It's purpose is to provide quick
    access to frequently used commands or options. Within a
    Q3MainWindow the user can drag toolbars within and between the
    \link QDockArea dock areas\endlink. Toolbars can also be dragged
    out of any dock area to float freely as top-level windows.

    Q3ToolBar is a specialization of QDockWindow, and so provides all
    the functionality of a QDockWindow.

    To use Q3ToolBar you simply create a Q3ToolBar as a child of a
    Q3MainWindow, create a number of QToolButton widgets (or other
    widgets) in left to right (or top to bottom) order and call
    addSeparator() when you want a separator. When a toolbar is
    floated the caption used is the label given in the constructor
    call. This can be changed with setLabel().

    You may use most widgets within a toolbar, with QToolButton and
    QComboBox being the most common.

    If you create a new widget on an already visible Q3ToolBar, this
    widget will automatically become visible without needing a show()
    call. (This differs from every other Qt widget container. We
    recommend calling show() anyway since we hope to fix this anomaly
    in a future release.)

    Q3ToolBars, like QDockWindows, are located in \l{QDockArea}s or
    float as top-level windows. Q3MainWindow provides four QDockAreas
    (top, left, right and bottom). When you create a new toolbar (as
    in the example above) as a child of a Q3MainWindow the toolbar will
    be added to the top dock area. You can move it to another dock
    area (or float it) by calling Q3MainWindow::moveDockWindow(). QDock
    areas lay out their windows in \link qdockarea.html#lines
    Lines\endlink.

    If the main window is resized so that the area occupied by the
    toolbar is too small to show all its widgets a little arrow button
    (which looks like a right-pointing chevron, '&#187;') will appear
    at the right or bottom of the toolbar depending on its
    orientation. Clicking this button pops up a menu that shows the
    'overflowing' items. QToolButtons are represented in the menu using
    their textLabel property, other QButton subclasses are represented
    using their text property, and QComboBoxes are represented as submenus,
    with the caption text being used in the submenu item.

    Usually a toolbar will get precisely the space it needs. However,
    with setHorizontalStretchable(), setVerticalStretchable() or
    setStretchableWidget() you can tell the main window to expand the
    toolbar to fill all available space in the specified orientation.

    The toolbar arranges its buttons either horizontally or vertically
    (see orientation() for details). Generally, QDockArea will set the
    orientation correctly for you, but you can set it yourself with
    setOrientation() and track any changes by connecting to the
    orientationChanged() signal.

    You can use the clear() method to remove all items from a toolbar.

    \img qdockwindow.png Toolbar (dock window)
    \caption A floating QToolbar (dock window)

    \sa QToolButton Q3MainWindow \link http://www.iarchitect.com/visual.htm Parts of Isys on Visual Design\endlink \link guibooks.html#fowler GUI Design Handbook: Tool Bar\endlink.
*/

/*!
    Constructs an empty toolbar.

    The toolbar is called \a name and is a child of \a parent and is
    managed by \a parent. It is initially located in dock area \a dock
    and is labeled \a label. If \a newLine is true the toolbar will be
    placed on a new line in the dock area.
*/

Q3ToolBar::Q3ToolBar(const QString &label,
                    Q3MainWindow * parent, Qt::ToolBarDock dock,
                    bool newLine, const char * name)
    : Q3DockWindow(InDock, parent, name, 0, true)
{
    mw = parent;
    init();

    if (parent)
        parent->addToolBar(this, label, dock, newLine);
}


/*!
    Constructs an empty horizontal toolbar.

    The toolbar is called \a name and is a child of \a parent and is
    managed by \a mainWindow. The \a label and \a newLine parameters
    are passed straight to Q3MainWindow::addDockWindow(). \a name and
    the widget flags \a f are passed on to the Q3DockWindow constructor.

    Use this constructor if you want to create torn-off (undocked,
    floating) toolbars or toolbars in the \link QStatusBar status
    bar\endlink.
*/

Q3ToolBar::Q3ToolBar(const QString &label, Q3MainWindow * mainWindow,
                    QWidget * parent, bool newLine, const char * name,
                    Qt::WFlags f)
    : Q3DockWindow(InDock, parent, name, f, true)
{
    mw = mainWindow;
    init();

    clearWFlags(Qt::WType_Dialog | Qt::WStyle_Customize | Qt::WStyle_NoBorder);
    setParent(parent);

    if (mainWindow)
        mainWindow->addToolBar(this, label, Qt::DockUnmanaged, newLine);
}


/*!
    \overload

    Constructs an empty toolbar called \a name, with parent \a parent,
    in its \a parent's top dock area, without any label and without
    requiring a newline.
*/

Q3ToolBar::Q3ToolBar(Q3MainWindow * parent, const char * name)
    : Q3DockWindow(InDock, parent, name, 0, true)
{
    mw = parent;
    init();

    if (parent)
        parent->addToolBar(this, QString::null, Qt::DockTop);
}

/*!
    \internal

  Common initialization code. Requires that \c mw and \c o are set.
  Does not call Q3MainWindow::addDockWindow().
*/
void Q3ToolBar::init()
{
    d = new Q3ToolBarPrivate;
    d->extension = 0;
    d->extensionPopup = 0;
    sw = 0;

    setBackgroundRole(QPalette::Button);
    setFocusPolicy(Qt::NoFocus);
    setFrameStyle(QFrame::ToolBarPanel | QFrame::Raised);
    boxLayout()->setSpacing(style().pixelMetric(QStyle::PM_ToolBarItemSpacing));
}

/*!
    Destructor.
*/

Q3ToolBar::~Q3ToolBar()
{
    delete d;
}

/*!
    \reimp
*/

void Q3ToolBar::setOrientation(Qt::Orientation o)
{
    Q3DockWindow::setOrientation(o);
    if (d->extension)
        d->extension->setOrientation(o);
    QObjectList childs = children();
    for (int i = 0; i < childs.size(); ++i) {
        Q3ToolBarSeparator* w = qt_cast<Q3ToolBarSeparator*>(childs.at(i));
        if (w)
            w->setOrientation(o);
    }
}

/*!
    Adds a separator to the right/bottom of the toolbar.
*/

void Q3ToolBar::addSeparator()
{
    (void) new Q3ToolBarSeparator(orientation(), this, "toolbar separator");
}

/*!
    \reimp
*/

void Q3ToolBar::styleChange(QStyle &oldStyle)
{
    boxLayout()->setSpacing(style().pixelMetric(QStyle::PM_ToolBarItemSpacing));
    Q3DockWindow::styleChange(oldStyle);
}

/*!
    \reimp.
*/

void Q3ToolBar::show()
{
    Q3DockWindow::show();
    if (mw)
        mw->triggerLayout(false);
    checkForExtension(size());
}


/*!
    \reimp
*/

void Q3ToolBar::hide()
{
    Q3DockWindow::hide();
    if (mw)
        mw->triggerLayout(false);
}

/*!
    Returns a pointer to the Q3MainWindow which manages this toolbar.
*/

Q3MainWindow * Q3ToolBar::mainWindow() const
{
    return mw;
}


/*!
    Sets the widget \a w to be expanded if this toolbar is requested
    to stretch.

    The request to stretch might occur because Q3MainWindow
    right-justifies the dock area the toolbar is in, or because this
    toolbar's isVerticalStretchable() or isHorizontalStretchable() is
    set to true.

    If you call this function and the toolbar is not yet stretchable,
    setStretchable() is called.

    \sa Q3MainWindow::setRightJustification(), setVerticalStretchable(),
    setHorizontalStretchable()
*/

void Q3ToolBar::setStretchableWidget(QWidget * w)
{
    sw = w;
    boxLayout()->setStretchFactor(w, 1);

    if (!isHorizontalStretchable() && !isVerticalStretchable()) {
        if (orientation() == Qt::Horizontal)
            setHorizontalStretchable(true);
        else
            setVerticalStretchable(true);
    }
}


/*!
    \reimp
*/

bool Q3ToolBar::event(QEvent * e)
{
    bool r =  Q3DockWindow::event(e);
    // After the event filters have dealt with it, do our stuff.
    if (e->type() == QEvent::ChildInserted) {
        QObject * child = ((QChildEvent*)e)->child();
        if (child && child->isWidgetType() && !((QWidget*)child)->isTopLevel()
             && child->parent() == this
            && QLatin1String("qt_dockwidget_internal") != child->objectName()) {
            boxLayout()->addWidget((QWidget*)child);
            if (QToolButton *button = qt_cast<QToolButton*>(child)) {
                if (mw) {
                    QObject::connect(mw, SIGNAL(pixmapSizeChanged(bool)),
                                     button, SLOT(setUsesBigPixmap(bool)));
                    button->setUsesBigPixmap(mw->usesBigPixmaps());
                    QObject::connect(mw, SIGNAL(usesTextLabelChanged(bool)),
                                     child, SLOT(setUsesTextLabel(bool)));
                    button->setUsesTextLabel(mw->usesTextLabel());
                }
                button->setAutoRaise(true);
            }
            if (isVisible()) {
                // toolbar compatibility: we auto show widgets that
                // are not explicitely hidden
                if (((QWidget*)child)->testWState(Qt::WState_Hidden|Qt::WState_ExplicitShowHide)
                     == Qt::WState_Hidden)
                    ((QWidget*)child)->show();
                checkForExtension(size());
            }
        }
        if (child && child->isWidgetType() && ((QWidget*)child) == sw)
            boxLayout()->setStretchFactor((QWidget*)child, 1);
    } else if (e->type() == QEvent::Show) {
        layout()->activate();
    } else if (e->type() == QEvent::LayoutHint && place() == OutsideDock) {
        adjustSize();
    }
    return r;
}


/*!
    \property Q3ToolBar::label
    \brief the toolbar's label.

    If the toolbar is floated the label becomes the toolbar window's
    caption. There is no default label text.
*/

void Q3ToolBar::setLabel(const QString & label)
{
    l = label;
    setWindowTitle(l);
}

QString Q3ToolBar::label() const
{
    return l;
}


/*!
    Deletes all the toolbar's child widgets.
*/

void Q3ToolBar::clear()
{
    QObjectList childs = children();
    for (int i = 0; i < childs.size(); ++i) {
        QObject *obj = childs.at(i);
        if (obj->isWidgetType() && QLatin1String("qt_dockwidget_internal") != obj->objectName())
            delete obj;
    }
}

/*!
    \internal
*/

QSize Q3ToolBar::minimumSize() const
{
    if (orientation() == Qt::Horizontal)
        return QSize(0, Q3DockWindow::minimumSize().height());
    return QSize(Q3DockWindow::minimumSize().width(), 0);
}

/*!
    \reimp
*/

QSize Q3ToolBar::minimumSizeHint() const
{
    if (orientation() == Qt::Horizontal)
        return QSize(0, Q3DockWindow::minimumSizeHint().height());
    return QSize(Q3DockWindow::minimumSizeHint().width(), 0);
}

void Q3ToolBar::createPopup()
{
    if (!d->extensionPopup) {
        d->extensionPopup = new QPopupMenu(this, "qt_dockwidget_internal");
        connect(d->extensionPopup, SIGNAL(aboutToShow()), this, SLOT(createPopup()));
    }

    if (!d->extension) {
        d->extension = new Q3ToolBarExtensionWidget(this);
        d->extension->setOrientation(orientation());
        d->extension->button()->setPopup(d->extensionPopup);
        d->extension->button()->setPopupDelay(-1);
    }

    d->extensionPopup->clear();

    // delete submenus
    QObjectList popups = d->extensionPopup->queryList("QPopupMenu", 0, false, true);
    while (!popups.isEmpty())
        delete popups.takeFirst();

    QObjectList childlist = queryList("QWidget", 0, false, true);
    bool hide = false;
    bool doHide = false;
    int id;
    for (int i = 0; i < childlist.size(); ++i) {
        QObject *obj = childlist.at(i);
        if (!obj->isWidgetType() || obj == d->extension->button() ||
            QLatin1String("qt_dockwidget_internal") == obj->objectName()) {
            continue;
        }
        int j = 2;
        QWidget *w = (QWidget*)obj;
        if (qt_cast<QComboBox*>(w))
            j = 1;
        hide = false;
        QPoint p = w->parentWidget()->mapTo(this, w->geometry().bottomRight());
        if (orientation() == Qt::Horizontal) {
            if (p.x() > (doHide ? width() - d->extension->width() / j : width()))
                hide = true;
        } else {
            if (p.y() > (doHide ? height()- d->extension->height() / j : height()))
                hide = true;
        }
        if (hide && w->isVisible()) {
            doHide = true;
            if (qt_cast<QToolButton*>(w)) {
                QToolButton *b = (QToolButton*)w;
                QString s = b->textLabel();
                if (s.isEmpty())
                    s = b->text();
                if (b->popup() && b->popupDelay() == 0)
                    id = d->extensionPopup->insertItem(b->iconSet(), s, b->popup());
                else
                    id = d->extensionPopup->insertItem(b->iconSet(), s, b, SLOT(emulateClick())) ;
                if (b->isToggleButton())
                    d->extensionPopup->setItemChecked(id, b->isOn());
                if (!b->isEnabled())
                    d->extensionPopup->setItemEnabled(id, false);
            } else if (qt_cast<QAbstractButton*>(w)) {
                QAbstractButton *b = (QAbstractButton*)w;
                QString s = b->text();
                if (s.isEmpty())
                    s = "";
                if (b->pixmap())
                    id = d->extensionPopup->insertItem(*b->pixmap(), s, b, SLOT(emulateClick()));
                else
                    id = d->extensionPopup->insertItem(s, b, SLOT(emulateClick()));
                if (b->isToggleButton())
                    d->extensionPopup->setItemChecked(id, b->isOn());
                if (!b->isEnabled())
                    d->extensionPopup->setItemEnabled(id, false);
#ifndef QT_NO_COMBOBOX
            } else if (qt_cast<QComboBox*>(w)) {
                QComboBox *c = (QComboBox*)w;
                if (c->count() != 0) {
                    QString s = c->windowTitle();
                    if (s.isEmpty())
                        s = c->currentText();
                    int maxItems = 0;
                    QPopupMenu *cp = new QPopupMenu(d->extensionPopup);
                    cp->setEnabled(c->isEnabled());
                    d->extensionPopup->insertItem(s, cp);
                    connect(cp, SIGNAL(activated(int)), c, SLOT(internalActivate(int)));
                    for (int i = 0; i < c->count(); ++i) {
                        QString tmp = c->text(i);
                        cp->insertItem(tmp, i);
                        if (c->currentText() == tmp)
                            cp->setItemChecked(i, true);
                        if (!maxItems) {
                            if (cp->actions().count() == 10) {
                                int h = cp->sizeHint().height();
                                maxItems = QApplication::desktop()->height() * 10 / h;
                            }
                        } else if (cp->actions().count() >= maxItems - 1) {
                            QPopupMenu* sp = new QPopupMenu(d->extensionPopup);
                            cp->insertItem(tr("More..."), sp);
                            cp = sp;
                            connect(cp, SIGNAL(activated(int)), c, SLOT(internalActivate(int)));
                        }
                    }
                }
#endif //QT_NO_COMBOBOX
            }
        }
    }
}

/*!
    \reimp
*/

void Q3ToolBar::resizeEvent(QResizeEvent *e)
{
    Q3DockWindow::resizeEvent(e);
    checkForExtension(e->size());
}

/*!
    \internal

    This function is called when an action is triggered. The relevant
    information is passed in the event \a e.
*/
void Q3ToolBar::actionEvent(QActionEvent *e)
{
    if (e->type() == QEvent::ActionAdded) {
        QAction *a = e->action();
        QWidget *w;
        if (a->isSeparator()) {
            w = new Q3ToolBarSeparator(orientation(), this, "toolbar separator");
        } else {
            QToolButton* btn = new QToolButton(this);
            btn->setToggleButton(a->isCheckable());
            QIcon icon = a->icon();
            if (!icon.isNull())
                btn->setIconSet(icon);
            connect(btn, SIGNAL(clicked()), a, SIGNAL(triggered()));
            connect(btn, SIGNAL(clicked()), a, SIGNAL(activated()));
//         connect(btn, SIGNAL(toggled(bool)), a, SLOT(toolButtonToggled(bool)));
// #ifndef QT_NO_TOOLTIP
//         connect(&(d->tipGroup), SIGNAL(showTip(QString)), this, SLOT(showStatusText(QString)));
//         connect(&(d->tipGroup), SIGNAL(removeTip()), this, SLOT(clearStatusText()));
// #endif

            w = btn;
        }
        d->actions.insert(a, w);
    } else if (e->type() == QEvent::ActionRemoved) {
        QAction *a = e->action();
        delete d->actions.take(a);
    } else if (e->type() == QEvent::ActionChanged) {
        QAction *a = e->action();
        QWidget *w = d->actions.value(a);
        if (w) {
            w->setShown(a->isVisible());
            w->setEnabled(a->isEnabled());
        }
    }
}


void Q3ToolBar::checkForExtension(const QSize &sz)
{
    if (!isVisible())
        return;

    bool tooSmall;
    if (orientation() == Qt::Horizontal)
        tooSmall = sz.width() < sizeHint().width();
    else
        tooSmall = sz.height() < sizeHint().height();

    if (tooSmall) {
        createPopup();
        if (d->extensionPopup->actions().count()) {
            if (orientation() == Qt::Horizontal)
                d->extension->setGeometry(width() - 20, 1, 20, height() - 2);
            else
                d->extension->setGeometry(1, height() - 20, width() - 2, 20);
            d->extension->show();
            d->extension->raise();
        } else {
            delete d->extension;
            d->extension = 0;
            delete d->extensionPopup;
            d->extensionPopup = 0;
        }
    } else {
        delete d->extension;
        d->extension = 0;
        delete d->extensionPopup;
        d->extensionPopup = 0;
    }
}


/*!
    \internal
*/

void Q3ToolBar::setMinimumSize(int, int)
{
}

/* from chaunsee:

1.  Tool Bars should contain only high-frequency functions.  Avoid putting
things like About and Exit on a tool bar unless they are frequent functions.

2.  All tool bar buttons must have some keyboard access method (it can be a
menu or shortcut key or a function in a dialog box that can be accessed
through the keyboard).

3.  Make tool bar functions as efficient as possible (the common example is to
Print in Microsoft applications, it doesn't bring up the Print dialog box, it
prints immediately to the default printer).

4.  Avoid turning tool bars into graphical menu bars.  To me, a tool bar should
be efficient. Once you make almost all the items in a tool bar into graphical
pull-down menus, you start to lose efficiency.

5.  Make sure that adjacent icons are distinctive. There are some tool bars
where you see a group of 4-5 icons that represent related functions, but they
are so similar that you can't differentiate among them.         These tool bars are
often a poor attempt at a "common visual language".

6.  Use any de facto standard icons of your platform (for windows use the
cut, copy and paste icons provided in dev kits rather than designing your
own).

7.  Avoid putting a highly destructive tool bar button (delete database) by a
safe, high-frequency button (Find) -- this will yield 1-0ff errors).

8.  Tooltips in many Microsoft products simply reiterate the menu text even
when that is not explanatory.  Consider making your tooltips slightly more
verbose and explanatory than the corresponding menu item.

9.  Keep the tool bar as stable as possible when you click on different
objects. Consider disabling tool bar buttons if they are used in most, but not
all contexts.

10.  If you have multiple tool bars (like the Microsoft MMC snap-ins have),
put the most stable tool bar to at the left with less stable ones to the
right. This arrangement (stable to less stable) makes the tool bar somewhat
more predictable.

11.  Keep a single tool bar to fewer than 20 items divided into 4-7 groups of
items.
*/
#endif
