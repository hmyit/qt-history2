
#include "formwindowmanager.h"
#include "widgetdatabase.h"
#include "iconloader.h"
#include "sizehandle.h"
#include "connectionedit.h"

#include <abstractwidgetfactory.h>
#include <abstractformeditor.h>
#include <abstractmetadatabase.h>
#include <abstractformwindow.h>
#include <layoutinfo.h>
#include <qtundo.h>

#include <QAction>
#include <QLayout>
#include <QMouseEvent>
#include <QApplication>
#include <QIcon>
#include <QBitmap>
#include <QPainter>
#include <QSizeGrip>
#include <QAbstractButton>
#include <QToolBox>
#include <QMainWindow>
#include <QMenuBar>

#include <qdebug.h>

static QString whatsThisFrom(const QString &str)
{
    Q_UNUSED(str); /// ### implement me!
    return str;
}

FormWindowManager::FormWindowManager(AbstractFormEditor *core, QObject *parent)
    : AbstractFormWindowManager(parent),
      m_core(core),
      m_activeFormWindow(0)
{
    lastWasAPassiveInteractor = false;
    
    m_layoutChilds = false;
    m_layoutSelected = false;
    m_breakLayout = false;

    setupActions();
    qApp->installEventFilter(this);

    // DnD stuff
    m_last_widget_under_mouse = 0;
    m_last_form_under_mouse = 0;
    m_source_form = 0;
}

FormWindowManager::~FormWindowManager()
{
    qApp->removeEventFilter(this);

    qDeleteAll(m_formWindows);
}

AbstractFormEditor *FormWindowManager::core() const
{
    return m_core;
}

AbstractFormWindow *FormWindowManager::activeFormWindow() const
{
    return m_activeFormWindow;
}

int FormWindowManager::formWindowCount() const
{
    return m_formWindows.size();
}

AbstractFormWindow *FormWindowManager::formWindow(int index) const
{
    return m_formWindows.at(index);
}

static bool isMouseMoveOrRelease(QEvent *e)
{
    return e->type() == QEvent::MouseButtonRelease
            || e->type() == QEvent::MouseMove;
}

bool FormWindowManager::eventFilter(QObject *o, QEvent *e)
{
    if (!m_drag_item_list.isEmpty() 
            && isMouseMoveOrRelease(e)
            && o == m_core->topLevel()) {
        // We're dragging
        QMouseEvent *me = static_cast<QMouseEvent*>(e);
        me->accept();
        
        if (me->type() == QEvent::MouseButtonRelease)
            endDrag(me->globalPos());
        else
            setItemsPos(me->globalPos());
        return true;
    }

    QWidget *w = static_cast<QWidget*>(o);
    if (qt_cast<SizeHandle*>(w))
        return false;

    if (!o->isWidgetType())
        return false;
    else if (isPassiveInteractor(w))
        return false;

    FormWindow *fw = FormWindow::findFormWindow(w);
    if (!fw)
        return false;

    w = findManagedWidget(fw, w);
    if (!w)
        return false;

    switch (e->type()) {
        case QEvent::Close: {
            if (o != fw)
                break;

            bool accept = true;
            emit formWindowClosing(fw, &accept);
            if (accept)
                static_cast<QCloseEvent*>(e)->accept();
            else
                static_cast<QCloseEvent*>(e)->ignore();
        } break;
        
        case QEvent::Hide:
            if (fw->isWidgetSelected(w))
                fw->hideSelection(w);
            break;
        
        case QEvent::WindowActivate: {
            if (fw->mainContainer() == static_cast<QWidget*>(o)) {
                setActiveFormWindow(fw);
            }
        } break;

        case QEvent::WindowDeactivate: {
            fw->repaintSelection();
        } break;

        case QEvent::Enter:
        case QEvent::Leave:
            return true;

        case QEvent::Resize:
        case QEvent::Move:
            if (fw->editMode() != AbstractFormWindow::WidgetEditMode)
                break;
            if (LayoutInfo::layoutType(m_core, w->parentWidget()) != LayoutInfo::NoLayout) {
                fw->updateSelection(w);
                if (e->type() != QEvent::Resize)
                    fw->updateChildSelections(w);
            }
            break;

        case QEvent::FocusOut:
        case QEvent::FocusIn:
            if (o == fw)
                break;
            return true;

        case QEvent::KeyPress:
            if (fw->editMode() != AbstractFormWindow::WidgetEditMode)
                break;
            fw->handleKeyPressEvent(w, static_cast<QKeyEvent*>(e));
            if (static_cast<QKeyEvent*>(e)->isAccepted())
                return true;
            break;

        case QEvent::KeyRelease:
            if (fw->editMode() != AbstractFormWindow::WidgetEditMode)
                break;
            fw->handleKeyReleaseEvent(w, static_cast<QKeyEvent*>(e));
            if (static_cast<QKeyEvent*>(e)->isAccepted())
                return true;
            break;

        case QEvent::MouseMove:
            if (fw->editMode() != AbstractFormWindow::WidgetEditMode)
                break;
            fw->handleMouseMoveEvent(w, static_cast<QMouseEvent*>(e));
            return true;

        case QEvent::MouseButtonPress:
            if (fw->editMode() != AbstractFormWindow::WidgetEditMode)
                break;
            fw->handleMousePressEvent(w, static_cast<QMouseEvent*>(e));
            return true;

        case QEvent::MouseButtonRelease:
            if (fw->editMode() != AbstractFormWindow::WidgetEditMode)
                break;
            fw->handleMouseReleaseEvent(w, static_cast<QMouseEvent*>(e));
            return true;

        case QEvent::MouseButtonDblClick:
            if (fw->editMode() != AbstractFormWindow::WidgetEditMode)
                break;
            fw->handleMouseButtonDblClickEvent(w, static_cast<QMouseEvent*>(e));
            return true;

        case QEvent::ContextMenu:
            if (fw->editMode() != AbstractFormWindow::WidgetEditMode)
                break;
            fw->handleContextMenu(w, static_cast<QContextMenuEvent*>(e));
            return true;

#if 0 // ### fix me
        case QEvent::Paint:
            fw->handlePaintEvent(static_cast<QWidget*>(o), static_cast<QPaintEvent*>(e));
            return true;
#endif

        default:
            break;
    }

    return false;
}

void FormWindowManager::addFormWindow(AbstractFormWindow *w)
{
    FormWindow *formWindow = qt_cast<FormWindow*>(w);
    if (!formWindow || m_formWindows.contains(formWindow))
        return;

    connect(formWindow, SIGNAL(selectionChanged()), this, SLOT(slotUpdateActions()));
    connect(formWindow, SIGNAL(editModeChanged(AbstractFormWindow::EditMode)), this, SLOT(slotUpdateActions()));

    m_formWindows.append(formWindow);
    emit formWindowAdded(formWindow);
}

void FormWindowManager::removeFormWindow(AbstractFormWindow *w)
{
    FormWindow *formWindow = qt_cast<FormWindow*>(w);

    int idx = m_formWindows.indexOf(formWindow);
    if (!formWindow || idx == -1)
        return;

    formWindow->disconnect(this);
    m_formWindows.removeAt(idx);
    emit formWindowRemoved(formWindow);

    if (formWindow == m_activeFormWindow)
        setActiveFormWindow(0);
}

void FormWindowManager::setActiveFormWindow(AbstractFormWindow *w)
{
    FormWindow *formWindow = qt_cast<FormWindow*>(w);

    if (formWindow == m_activeFormWindow)
        return;

    FormWindow *old = m_activeFormWindow;

    m_activeFormWindow = formWindow;

    slotUpdateActions();

    if (m_activeFormWindow) {
        m_activeFormWindow->repaintSelection();
        if (old)
            old->repaintSelection();
    }

    emit activeFormWindowChanged(m_activeFormWindow);

    if (m_activeFormWindow) {
        m_activeFormWindow->emitSelectionChanged();
    }
}

QWidget *FormWindowManager::findManagedWidget(FormWindow *fw, QWidget *w)
{
    while (w && w != fw) {
        if (fw->isManaged(w))
            return w;
        w = w->parentWidget();
    }

    return w;
}

void FormWindowManager::setupActions()
{
    m_actionCut = new QAction(createIconSet("designer_editcut.png"), tr("Cu&t"));
    m_actionCut->setShortcut(Qt::CTRL + Qt::Key_X);
    m_actionCut->setStatusTip(tr("Cuts the selected widgets and puts them on the clipboard"));
    m_actionCut->setWhatsThis(whatsThisFrom("Edit|Cut"));
    connect(m_actionCut, SIGNAL(triggered()), this, SLOT(slotActionCutActivated()));
    m_actionCut->setEnabled(false);

    m_actionCopy = new QAction(createIconSet("designer_editcopy.png"), tr("&Copy"));
    m_actionCopy->setShortcut(Qt::CTRL + Qt::Key_C);
    m_actionCopy->setStatusTip(tr("Copies the selected widgets to the clipboard"));
    m_actionCopy->setWhatsThis(whatsThisFrom("Edit|Copy"));
    connect(m_actionCopy, SIGNAL(triggered()), this, SLOT(slotActionCopyActivated()));
    m_actionCopy->setEnabled(false);

    m_actionPaste = new QAction(createIconSet("designer_editpaste.png"), tr("&Paste"));
    m_actionPaste->setShortcut(Qt::CTRL + Qt::Key_V);
    m_actionPaste->setStatusTip(tr("Pastes the clipboard's contents"));
    m_actionPaste->setWhatsThis(whatsThisFrom("Edit|Paste"));
    connect(m_actionPaste, SIGNAL(triggered()), this, SLOT(slotActionPasteActivated()));
    m_actionPaste->setEnabled(false);

    m_actionDelete = new QAction(tr("&Delete"));
    m_actionDelete->setShortcut(Qt::Key_Delete);
    m_actionDelete->setStatusTip(tr("Deletes the selected widgets"));
    m_actionDelete->setWhatsThis(whatsThisFrom("Edit|Delete"));
    connect(m_actionDelete, SIGNAL(triggered()), this, SLOT(slotActionDeleteActivated()));
    m_actionDelete->setEnabled(false);

    m_actionSelectAll = new QAction(tr("Select &All"));
    m_actionSelectAll->setShortcut(Qt::CTRL + Qt::Key_A);
    m_actionSelectAll->setStatusTip(tr("Selects all widgets"));
    m_actionSelectAll->setWhatsThis(whatsThisFrom("Edit|Select All"));
    connect(m_actionSelectAll, SIGNAL(triggered()), this, SLOT(slotActionSelectAllActivated()));
    m_actionSelectAll->setEnabled(false);

    m_actionRaise = new QAction(createIconSet("designer_editraise.png"), tr("Bring to &Front"));
    m_actionRaise->setStatusTip(tr("Raises the selected widgets"));
    m_actionRaise->setWhatsThis(tr("Raises the selected widgets"));
    connect(m_actionRaise, SIGNAL(triggered()), this, SLOT(slotActionRaiseActivated()));
    m_actionRaise->setEnabled(false);

    m_actionLower = new QAction(createIconSet("designer_editlower.png"), tr("Send to &Back"));
    m_actionLower->setStatusTip(tr("Lowers the selected widgets"));
    m_actionLower->setWhatsThis(tr("Lowers the selected widgets"));
    connect(m_actionLower, SIGNAL(triggered()), this, SLOT(slotActionLowerActivated()));
    m_actionLower->setEnabled(false);


    m_actionAdjustSize = new QAction(createIconSet("designer_adjustsize.png"), tr("Adjust &Size"));
    m_actionAdjustSize->setShortcut(Qt::CTRL + Qt::Key_J);
    m_actionAdjustSize->setStatusTip(tr("Adjusts the size of the selected widget"));
    m_actionAdjustSize->setWhatsThis(whatsThisFrom("Layout|Adjust Size"));
    connect(m_actionAdjustSize, SIGNAL(triggered()), this, SLOT(slotActionAdjustSizeActivated()));
    m_actionAdjustSize->setEnabled(false);

    m_actionHorizontalLayout = new QAction(createIconSet("designer_edithlayout.png"), tr("Lay Out &Horizontally"));
    m_actionHorizontalLayout->setShortcut(Qt::CTRL + Qt::Key_H);
    m_actionHorizontalLayout->setStatusTip(tr("Lays out the selected widgets horizontally"));
    m_actionHorizontalLayout->setWhatsThis(whatsThisFrom("Layout|Lay Out Horizontally"));
    connect(m_actionHorizontalLayout, SIGNAL(triggered()), this, SLOT(slotActionHorizontalLayoutActivated()));
    m_actionHorizontalLayout->setEnabled(false);

    m_actionVerticalLayout = new QAction(createIconSet("designer_editvlayout.png"), tr("Lay Out &Vertically"));
    m_actionVerticalLayout->setShortcut(Qt::CTRL + Qt::Key_L);
    m_actionVerticalLayout->setStatusTip(tr("Lays out the selected widgets vertically"));
    m_actionVerticalLayout->setWhatsThis(whatsThisFrom("Layout|Lay Out Vertically"));
    connect(m_actionVerticalLayout, SIGNAL(triggered()), this, SLOT(slotActionVerticalLayoutActivated()));
    m_actionVerticalLayout->setEnabled(false);

    m_actionGridLayout = new QAction(createIconSet("designer_editgrid.png"), tr("Lay Out in a &Grid"));
    m_actionGridLayout->setShortcut(Qt::CTRL + Qt::Key_G);
    m_actionGridLayout->setStatusTip(tr("Lays out the selected widgets in a grid"));
    m_actionGridLayout->setWhatsThis(whatsThisFrom("Layout|Lay Out in a Grid"));
    connect(m_actionGridLayout, SIGNAL(triggered()), this, SLOT(slotActionGridLayoutActivated()));
    m_actionGridLayout->setEnabled(false);

    m_actionSplitHorizontal = new QAction(createIconSet("designer_editvlayoutsplit.png"),
                                             tr("Lay Out Horizontally in S&plitter"));
    m_actionSplitHorizontal->setStatusTip(tr("Lays out the selected widgets horizontally in a splitter"));
    m_actionSplitHorizontal->setWhatsThis(whatsThisFrom("Layout|Lay Out Horizontally in Splitter"));
    connect(m_actionSplitHorizontal, SIGNAL(triggered()), this, SLOT(slotActionSplitHorizontalActivated()));
    m_actionSplitHorizontal->setEnabled(false);

    m_actionSplitVertical = new QAction(createIconSet("designer_edithlayoutsplit.png"),
                                             tr("Lay Out Vertically in Sp&litter"));
    m_actionSplitVertical->setStatusTip(tr("Lays out the selected widgets vertically in a splitter"));
    m_actionSplitVertical->setWhatsThis(whatsThisFrom("Layout|Lay Out Vertically in Splitter"));
    connect(m_actionSplitVertical, SIGNAL(triggered()), this, SLOT(slotActionSplitVerticalActivated()));
    m_actionSplitVertical->setEnabled(false);

    m_actionBreakLayout = new QAction(createIconSet("designer_editbreaklayout.png"), tr("&Break Layout"));
    m_actionBreakLayout->setShortcut(Qt::CTRL + Qt::Key_B);
    m_actionBreakLayout->setStatusTip(tr("Breaks the selected layout"));
    m_actionBreakLayout->setWhatsThis(whatsThisFrom("Layout|Break Layout"));
    connect(m_actionBreakLayout, SIGNAL(triggered()), this, SLOT(slotActionBreakLayoutActivated()));
    m_actionBreakLayout->setEnabled(false);

}

void FormWindowManager::slotActionCutActivated()
{
    m_activeFormWindow->cut();
}

void FormWindowManager::slotActionCopyActivated()
{
    m_activeFormWindow->copy();
}

void FormWindowManager::slotActionPasteActivated()
{
    m_activeFormWindow->paste();
}

void FormWindowManager::slotActionDeleteActivated()
{
    m_activeFormWindow->deleteWidgets();
}

void FormWindowManager::slotActionLowerActivated()
{
    m_activeFormWindow->lowerWidgets();
}

void FormWindowManager::slotActionRaiseActivated()
{
    m_activeFormWindow->lowerWidgets();
}

void FormWindowManager::slotActionHorizontalLayoutActivated()
{
    if (m_layoutChilds)
        layoutContainerHorizontal();
    else if (m_layoutSelected)
        m_activeFormWindow->layoutHorizontal();
}

void FormWindowManager::slotActionVerticalLayoutActivated()
{
    if (m_layoutChilds)
        layoutContainerVertical();
    else if (m_layoutSelected)
        m_activeFormWindow->layoutVertical();
}

void FormWindowManager::slotActionGridLayoutActivated()
{
    if (m_layoutChilds)
        layoutContainerGrid();
    else if (m_layoutSelected)
        m_activeFormWindow->layoutGrid();
}

void FormWindowManager::slotActionSplitHorizontalActivated()
{
    if (m_layoutChilds)
        ; // no way to do that
    else if (m_layoutSelected)
        m_activeFormWindow->layoutHorizontalSplit();
}

void FormWindowManager::slotActionSplitVerticalActivated()
{
    if (m_layoutChilds)
        ; // no way to do that
    else if (m_layoutSelected)
        m_activeFormWindow->layoutVerticalSplit();
}

void FormWindowManager::slotActionBreakLayoutActivated()
{
    if (!m_breakLayout)
        return;
    QWidget *w = m_activeFormWindow->mainContainer();
    if (m_activeFormWindow->currentWidget())
        w = m_activeFormWindow->currentWidget();
    if (LayoutInfo::layoutType(m_core, w) != LayoutInfo::NoLayout ||
         w->parentWidget() && LayoutInfo::layoutType(m_core, w->parentWidget()) != LayoutInfo::NoLayout) {
        m_activeFormWindow->breakLayout(w);
        return;
    } else {
        QList<QWidget*> widgets = m_activeFormWindow->selectedWidgets();
        for (int i = 0; i < widgets.size(); ++i) {
            QWidget *w = widgets.at(i);
            if (LayoutInfo::layoutType(m_core, w) != LayoutInfo::NoLayout ||
                 w->parentWidget() && LayoutInfo::layoutType(m_core, w->parentWidget()) != LayoutInfo::NoLayout)
                break;
        }
        if (w) {
            m_activeFormWindow->breakLayout(w);
            return;
        }
    }

    w = m_activeFormWindow->mainContainer();
    if (LayoutInfo::layoutType(m_core, w) != LayoutInfo::NoLayout ||
         w->parentWidget() && LayoutInfo::layoutType(m_core, w->parentWidget()) != LayoutInfo::NoLayout)
        m_activeFormWindow->breakLayout(w);
}

void FormWindowManager::slotActionAdjustSizeActivated()
{
    m_activeFormWindow->adjustSize();
}

void FormWindowManager::slotActionSelectAllActivated()
{
    m_activeFormWindow->selectAll();
}

void FormWindowManager::slotUpdateActions()
{
    m_layoutChilds = false;
    m_layoutSelected = false;
    m_breakLayout = false;

    if (!m_activeFormWindow 
            || m_activeFormWindow->editMode() != AbstractFormWindow::WidgetEditMode) {
        m_actionCut->setEnabled(false);
        m_actionCopy->setEnabled(false);
        m_actionPaste->setEnabled(false);
        m_actionDelete->setEnabled(false);
        m_actionSelectAll->setEnabled(false);
        m_actionAdjustSize->setEnabled(false);
        m_actionHorizontalLayout->setEnabled(false);
        m_actionVerticalLayout->setEnabled(false);
        m_actionSplitHorizontal->setEnabled(false);
        m_actionSplitVertical->setEnabled(false);
        m_actionGridLayout->setEnabled(false);
        m_actionBreakLayout->setEnabled(false);
        m_actionLower->setEnabled(false);
        m_actionRaise->setEnabled(false);
        m_actionAdjustSize->setEnabled(false);
        return;
    }

    QList<QWidget*> widgets = m_activeFormWindow->selectedWidgets();
    int selectedWidgets = widgets.size();

    bool enable = selectedWidgets > 0;

    m_actionCut->setEnabled(enable);
    m_actionCopy->setEnabled(enable);
    m_actionPaste->setEnabled(true);
    m_actionDelete->setEnabled(enable);
    m_actionLower->setEnabled(enable);
    m_actionRaise->setEnabled(enable);
    m_actionSelectAll->setEnabled(true);

    m_actionAdjustSize->setEnabled(false);
    m_actionSplitHorizontal->setEnabled(false);
    m_actionSplitVertical->setEnabled(false);

    enable = false;
    if (selectedWidgets > 1) {
        int unlaidout = 0;
        int laidout = 0;
        for (int i = 0; i < widgets.size(); ++i) {
            QWidget *w = widgets.at(i);
            if (!w->parentWidget() || LayoutInfo::layoutType(m_core, w->parentWidget()) == LayoutInfo::NoLayout)
                unlaidout++;
            else
                laidout++;
        }

        m_actionHorizontalLayout->setEnabled(unlaidout > 1);
        m_actionVerticalLayout->setEnabled(unlaidout > 1);
        m_actionSplitHorizontal->setEnabled(unlaidout > 1);
        m_actionSplitVertical->setEnabled(unlaidout > 1);
        m_actionGridLayout->setEnabled(unlaidout > 1);
        m_actionBreakLayout->setEnabled(laidout > 0);
        m_actionAdjustSize->setEnabled(laidout > 0);
        m_layoutSelected = unlaidout > 1;
        m_breakLayout = laidout > 0;
    } else if (selectedWidgets == 1) {
        QWidget *w = widgets.first();
        bool isContainer = core()->widgetDataBase()->isContainer(w)
            || w == m_activeFormWindow->mainContainer();

        m_actionAdjustSize->setEnabled(!w->parentWidget()
            || LayoutInfo::layoutType(m_core, w->parentWidget()) == LayoutInfo::NoLayout);

        if (!isContainer) {
            m_actionHorizontalLayout->setEnabled(false);
            m_actionVerticalLayout->setEnabled(false);
            m_actionGridLayout->setEnabled(false);
            if (w->parentWidget() && LayoutInfo::layoutType(m_core, w->parentWidget()) != LayoutInfo::NoLayout) {
                m_actionBreakLayout->setEnabled(true);
                m_breakLayout = true;
            } else {
                m_actionBreakLayout->setEnabled(false);
            }
        } else {
            if (LayoutInfo::layoutType(m_core, w) == LayoutInfo::NoLayout) {
                if (!m_activeFormWindow->hasInsertedChildren(w)) {
                    m_actionHorizontalLayout->setEnabled(false);
                    m_actionVerticalLayout->setEnabled(false);
                    m_actionGridLayout->setEnabled(false);
                    m_actionBreakLayout->setEnabled(false);
                } else {
                    m_actionHorizontalLayout->setEnabled(true);
                    m_actionVerticalLayout->setEnabled(true);
                    m_actionGridLayout->setEnabled(true);
                    m_actionBreakLayout->setEnabled(false);
                    m_layoutChilds = true;
                }
                if (w->parentWidget() && LayoutInfo::layoutType(m_core, w->parentWidget()) != LayoutInfo::NoLayout) {
                    m_actionBreakLayout->setEnabled(true);
                    m_breakLayout = true;
                }
            } else {
                m_actionHorizontalLayout->setEnabled(false);
                m_actionVerticalLayout->setEnabled(false);
                m_actionGridLayout->setEnabled(false);
                m_actionBreakLayout->setEnabled(true);
                m_breakLayout = true;
            }
        }
    } else if (selectedWidgets == 0) {
        m_actionAdjustSize->setEnabled(true);
        QWidget *w = m_activeFormWindow->mainContainer();
        if (LayoutInfo::layoutType(m_core, w) == LayoutInfo::NoLayout) {
            if (!m_activeFormWindow->hasInsertedChildren(w)) {
                m_actionHorizontalLayout->setEnabled(false);
                m_actionVerticalLayout->setEnabled(false);
                m_actionGridLayout->setEnabled(false);
                m_actionBreakLayout->setEnabled(false);
            } else {
                m_actionHorizontalLayout->setEnabled(true);
                m_actionVerticalLayout->setEnabled(true);
                m_actionGridLayout->setEnabled(true);
                m_actionBreakLayout->setEnabled(false);
                m_layoutChilds = true;
            }
        } else {
            m_actionHorizontalLayout->setEnabled(false);
            m_actionVerticalLayout->setEnabled(false);
            m_actionGridLayout->setEnabled(false);
            m_actionBreakLayout->setEnabled(true);
            m_breakLayout = true;
        }
    } else {
        m_actionHorizontalLayout->setEnabled(false);
        m_actionVerticalLayout->setEnabled(false);
        m_actionGridLayout->setEnabled(false);
        m_actionBreakLayout->setEnabled(false);
    }
}

void FormWindowManager::layoutContainerHorizontal()
{
    QWidget *w = m_activeFormWindow->mainContainer();
    QList<QWidget*> l(m_activeFormWindow->selectedWidgets());
    if (l.count() == 1)
        w = l.first();

    if (w)
        m_activeFormWindow->layoutHorizontalContainer(w);
}

void FormWindowManager::layoutContainerVertical()
{
    QWidget *w = m_activeFormWindow->mainContainer();
    QList<QWidget*> l(m_activeFormWindow->selectedWidgets());
    if (l.count() == 1)
        w = l.first();

    if (w)
        m_activeFormWindow->layoutVerticalContainer(w);
}

void FormWindowManager::layoutContainerGrid()
{
    QWidget *w = m_activeFormWindow->mainContainer();
    QList<QWidget*> l(m_activeFormWindow->selectedWidgets());
    if (l.count() == 1)
        w = l.first();

    if (w)
        m_activeFormWindow->layoutGridContainer(w);
}

AbstractFormWindow *FormWindowManager::createFormWindow(QWidget *parentWidget, Qt::WFlags flags)
{
    FormWindow *formWindow = new FormWindow(qt_cast<FormEditor*>(core()), parentWidget, flags);
    addFormWindow(formWindow);
    return formWindow;
}

QAction *FormWindowManager::actionUndo() const
{
    return QtUndoManager::manager()->undoAction();
}

QAction *FormWindowManager::actionRedo() const
{
    return QtUndoManager::manager()->redoAction();
}

// DnD stuff

void FormWindowManager::dragItems(const QList<AbstractDnDItem*> &item_list, AbstractFormWindow *source_form)
{
    if (!m_drag_item_list.isEmpty()) {
        qWarning("FormWindowManager::dragItem(): called while already dragging");
        return;
    }
    
    m_source_form = qt_cast<FormWindow*>(source_form);
    
    beginDrag(item_list);
}

void FormWindowManager::beginDrag(const QList<AbstractDnDItem*> &item_list)
{
    Q_ASSERT(m_drag_item_list.isEmpty());
    
    m_drag_item_list = item_list;
    
    QPoint pos = QCursor::pos();

    setItemsPos(pos);    

    foreach(AbstractDnDItem *item, m_drag_item_list) {
        QWidget *deco = item->decoration();
        QBitmap bitmap(deco->size());
        QPainter p(&bitmap);
        p.fillRect(bitmap.rect(), Qt::color1);
        p.setPen(Qt::color0);
        p.drawPoint(deco->mapFromGlobal(pos));
        deco->setMask(bitmap);
        deco->show();
    }

    m_core->topLevel()->installEventFilter(this);
    m_core->topLevel()->grabMouse();
}
    
void FormWindowManager::setItemsPos(const QPoint &globalPos)
{
    foreach(AbstractDnDItem *item, m_drag_item_list)
        item->decoration()->move(globalPos - item->hotSpot());

    QWidget *widget_under_mouse = qApp->widgetAt(globalPos);
    int max_try = 3;
    while (max_try && widget_under_mouse && isDecoration(widget_under_mouse)) {
        --max_try;
        widget_under_mouse = qApp->widgetAt(widget_under_mouse->pos() - QPoint(1,1));
        Q_ASSERT(!qt_cast<ConnectionEdit*>(widget_under_mouse));
    }
    
    FormWindow *form_under_mouse
            = qt_cast<FormWindow*>(AbstractFormWindow::findFormWindow(widget_under_mouse));
    if (form_under_mouse != 0 && !form_under_mouse->hasFeature(AbstractFormWindow::EditFeature))
        form_under_mouse = 0;
    if (form_under_mouse != 0) {
        // widget_under_mouse might be some temporary thing like the dropLine. We need
        // the actual widget that's part of the edited GUI.
        widget_under_mouse
            = form_under_mouse->widgetAt(form_under_mouse->mapFromGlobal(globalPos));
            
        Q_ASSERT(!qt_cast<ConnectionEdit*>(widget_under_mouse));
    }
    
    if (m_last_form_under_mouse != 0 && widget_under_mouse != m_last_widget_under_mouse) {
        m_last_form_under_mouse->highlightWidget(m_last_widget_under_mouse,
                                    m_last_widget_under_mouse->mapFromGlobal(globalPos),
                                    FormWindow::Restore);
    }

    if (form_under_mouse != 0 
        && (m_source_form == 0 || widget_under_mouse != m_source_form->mainContainer())) {
    
        form_under_mouse->highlightWidget(widget_under_mouse,
                                    widget_under_mouse->mapFromGlobal(globalPos),
                                    FormWindow::Highlight);
    }

    m_last_widget_under_mouse = widget_under_mouse;
    m_last_form_under_mouse = form_under_mouse;
}

void FormWindowManager::endDrag(const QPoint &pos)
{
    m_core->topLevel()->removeEventFilter(this);
    m_core->topLevel()->releaseMouse();

    Q_ASSERT(!m_drag_item_list.isEmpty());
    
    if (m_last_form_under_mouse != 0 && 
            m_last_form_under_mouse->hasFeature(AbstractFormWindow::EditFeature)) {
        FormWindow *form = qt_cast<FormWindow*>(m_last_form_under_mouse);
        
        form->beginCommand(tr("Drop widget"));
                                    
        QWidget *parent = m_last_widget_under_mouse;
        if (parent == 0)
            parent = form->mainContainer();
        
        form->mainContainer()->setActiveWindow();
        form->clearSelection(false);

        form->highlightWidget(m_last_widget_under_mouse,
                            m_last_widget_under_mouse->mapFromGlobal(pos),
                            FormWindow::Restore);
        
        if (m_drag_item_list.first()->domUi() != 0) {
            foreach (AbstractDnDItem *item, m_drag_item_list) {
                DomUI *dom_ui = item->domUi();
                Q_ASSERT(dom_ui != 0);
                                
                QRect geometry = item->decoration()->geometry();
                if (LayoutInfo::layoutType(core(), parent) != LayoutInfo::NoLayout
                        && core()->metaDataBase()->item(parent->layout()))
                    geometry.moveTopLeft(pos);
                    
                QWidget *widget = form->createWidget(dom_ui, geometry, parent);                
                form->selectWidget(widget, true);
                emit itemDragFinished();
            }
        } else if (qt_cast<FormWindowDnDItem*>(m_drag_item_list.first()) != 0) {
            foreach (AbstractDnDItem *item, m_drag_item_list) {
                FormWindowDnDItem *form_item = qt_cast<FormWindowDnDItem*>(item);
                Q_ASSERT(form_item != 0);
                QWidget *widget = form_item->widget();
                Q_ASSERT(widget != 0);
                QRect geometry = item->decoration()->geometry();
                                
                if (parent == widget->parent()) {
                    geometry.moveTopLeft(parent->mapFromGlobal(geometry.topLeft()));
                    form->resizeWidget(widget, geometry);
                    form->selectWidget(widget, true);
                    widget->show();
                } else {
                    if (m_source_form != 0)
                        m_source_form->deleteWidgets(QList<QWidget*>() << widget);
                        
                    if (LayoutInfo::layoutType(core(), parent) != LayoutInfo::NoLayout
                            && core()->metaDataBase()->item(parent->layout()))
                        geometry.moveTopLeft(pos);
                        
                    form->insertWidget(widget, geometry, parent);
                }           
            }
            
        }

        form->endCommand();
    } else {
        foreach (AbstractDnDItem *item, m_drag_item_list) {
            FormWindowDnDItem *form_item = qt_cast<FormWindowDnDItem*>(item);
            if (form_item != 0 && form_item->widget() != 0)
                form_item->widget()->show();
        }
    }

    foreach (AbstractDnDItem *item, m_drag_item_list)
        delete item;
    m_drag_item_list.clear();
    m_last_widget_under_mouse = 0;
    m_last_form_under_mouse = 0;
}

bool FormWindowManager::isDecoration(QWidget *widget) const
{
    foreach (AbstractDnDItem *item, m_drag_item_list) {
        if (item->decoration() == widget)
            return true;
    }
    
    return false;
}

bool FormWindowManager::isPassiveInteractor(QWidget *o) const
{
    if (lastPassiveInteractor && lastPassiveInteractor == o)
        return lastWasAPassiveInteractor;
    lastWasAPassiveInteractor = false;
    lastPassiveInteractor = o;
    if (QApplication::activePopupWidget()) // if a popup is open, we have to make sure that this one is closed, else X might do funny things
        return (lastWasAPassiveInteractor = true);

    if (qt_cast<QTabBar*>(o))
        return (lastWasAPassiveInteractor = true);
    else if (qt_cast<QSizeGrip*>(o))
        return (lastWasAPassiveInteractor = true);
    else if (qt_cast<QAbstractButton*>(o) 
            && (qt_cast<QTabBar*>(o->parent()) || qt_cast<QToolBox*>(o->parent())))
        return (lastWasAPassiveInteractor = true);
    else if (qt_cast<QMenuBar*>(o) && qt_cast<QMainWindow*>(o->parent()))
        return (lastWasAPassiveInteractor = true);
    else if (o->objectName() == QLatin1String("designer_wizardstack_button"))
        return (lastWasAPassiveInteractor = true);
    return lastWasAPassiveInteractor;
}
