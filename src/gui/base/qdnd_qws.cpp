/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
** EDITIONS: FREE, PROFESSIONAL, ENTERPRISE
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qapplication.h"

#ifndef QT_NO_DRAGANDDROP

#include "qwidget.h"
#include "qintdict.h"
#include "qdatetime.h"
#include "qdict.h"
#include "qdragobject.h"
#include "qbitmap.h"
#include "qcursor.h"
#include "qevent.h"

static QPixmap *defaultPm = 0;
static const int default_pm_hotx = -2;
static const int default_pm_hoty = -16;
static const char* default_pm[] = {
"13 9 3 1",
".      c None",
"       c #000000",
"X      c #FFFFFF",
"X X X X X X X",
" X X X X X X ",
"X ......... X",
" X.........X ",
"X ......... X",
" X.........X ",
"X ......... X",
" X X X X X X ",
"X X X X X X X",
};

// Shift/Ctrl handling, and final drop status
static QDragObject::DragMode drag_mode;
static QDropEvent::Action global_requested_action = QDropEvent::Copy;
static QDropEvent::Action global_accepted_action = QDropEvent::Copy;
static QDragObject *drag_object;

static Qt::ButtonState oldstate;

class QShapedPixmapWidget : public QWidget {
    QPixmap pixmap;
public:
    QShapedPixmapWidget() :
        QWidget(0,0,WStyle_Customize | WStyle_Tool | WStyle_NoBorder | WX11BypassWM)
    {
    }

    void setPixmap(QPixmap pm)
    {
        pixmap = pm;
        if (pixmap.mask()) {
            setMask(*pixmap.mask());
        } else {
            clearMask();
        }
        resize(pm.width(),pm.height());
    }

    void paintEvent(QPaintEvent*)
    {
        bitBlt(this,0,0,&pixmap);
    }
};

QShapedPixmapWidget *qt_qws_dnd_deco = 0;

void QDragManager::updatePixmap()
{
    if (qt_qws_dnd_deco) {
        QPixmap pm;
        QPoint pm_hot(default_pm_hotx,default_pm_hoty);
        if (drag_object) {
            pm = drag_object->pixmap();
            if (!pm.isNull())
                pm_hot = drag_object->pixmapHotSpot();
        }
        if (pm.isNull()) {
            if (!defaultPm)
                defaultPm = new QPixmap(default_pm);
            pm = *defaultPm;
        }
        qt_qws_dnd_deco->setPixmap(pm);
        qt_qws_dnd_deco->move(QCursor::pos()-pm_hot);
        if (willDrop) {
            qt_qws_dnd_deco->show();
        } else {
            qt_qws_dnd_deco->hide();
        }
    }
}

void QDragManager::timerEvent(QTimerEvent *) { }

void QDragManager::move(const QPoint &) { }

bool QDropEvent::provides(const char *mimeType) const
{
    int n = 0;
    const char* f;
    do {
        f = format(n);
        if (!f)
            return false;
        n++;
    } while(qstricmp(mimeType, f));
    return true;
}

QByteArray QDropEvent::encodedData(const char *format) const
{
    // ### multi-process drag'n'drop support is the next step
    if (drag_object)
        return drag_object->encodedData(format);
    return QByteArray();
}

const char* QDropEvent::format(int n) const
{
    // ### multi-process drag'n'drop support is the next step
    if (drag_object)
        return drag_object->format(n);
    return 0;
}

void myOverrideCursor(QCursor cursor, bool replace) {
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(cursor, replace);
#endif
}

void myRestoreOverrideCursor() {
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
}

void QDragManager::updateCursor()
{
#ifndef QT_NO_CURSOR
    if (willDrop) {
        int cursorIndex = 0; // default is copy_cursor
        if (global_accepted_action == QDropEvent::Copy) {
            if (global_requested_action != QDropEvent::Move)
                cursorIndex = 1; // move_cursor
        } else if (global_accepted_action == QDropEvent::Link)
            cursorIndex = 2; // link_cursor
        if (qt_qws_dnd_deco)
            qt_qws_dnd_deco->show();
        myOverrideCursor(QCursor(pm_cursor[cursorIndex], 0, 0), true);
    } else {
        myOverrideCursor(QCursor(ForbiddenCursor), true);
        if (qt_qws_dnd_deco)
            qt_qws_dnd_deco->hide();
    }
#endif
}


bool QDragManager::eventFilter(QObject *o, QEvent *e)
{
    if (!o->isWidgetType())
        return false;

    switch(e->type()) {

        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        {
            QKeyEvent *ke = ((QKeyEvent*)e);
            if (ke->key() == Key_Escape && e->type() == QEvent::KeyPress) {
                cancel();
                qApp->removeEventFilter(this);
                dragSource = 0;
            } else {
                updateMode(ke->stateAfter());
                updateCursor();
            }
            return true; // Eat all key events
        }

        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
        {
            QMouseEvent *me = (QMouseEvent *)e;
            if (me->state() & (QMouseEvent::LeftButton | QMouseEvent::MidButton | QMouseEvent::RightButton)) {

                QWidget *cw = QApplication::widgetAt(me->globalPos(), true);

                // Fix for when we move mouse on to the deco widget
                if (qt_qws_dnd_deco && cw == qt_qws_dnd_deco)
                    cw = dropWidget;

                if (dropWidget != cw) {
                    if (dropWidget) {
                        QDragLeaveEvent dle;
                        QApplication::sendEvent(dropWidget, &dle);
                        willDrop = false;
                        updateCursor();
                        restoreCursor = true;
                        dropWidget = NULL;
                    }
                    if (cw && cw->acceptDrops()) {
                        dropWidget = cw;
                        QDragEnterEvent dee(me->pos());
                        QApplication::sendEvent(dropWidget, &dee);
                        willDrop = dee.isAccepted();
                        updateCursor();
                        restoreCursor = true;
                    }
                } else if (cw) {
                    QDragMoveEvent dme(me->pos());
                    QApplication::sendEvent(cw, &dme);
                    updatePixmap();
                }
            }
            return true; // Eat all mouse events
        }

        case QEvent::MouseButtonRelease:
        {
            qApp->removeEventFilter(this);
            if (qt_qws_dnd_deco)
                delete qt_qws_dnd_deco;
            qt_qws_dnd_deco = 0;
            if (restoreCursor) {
                willDrop = false;
                myRestoreOverrideCursor();
                restoreCursor = false;
            }
            if (dropWidget) {
                QMouseEvent *me = (QMouseEvent *)e;
                QDropEvent de(me->pos());
                QApplication::sendEvent(dropWidget, &de);
                dropWidget = NULL;
            }
            return true; // Eat all mouse events
        }

        default:
             break;
    }

    return false;
}

bool QDragManager::drag(QDragObject *o, QDragObject::DragMode mode)
{
    object = drag_object = o;
    qt_qws_dnd_deco = new QShapedPixmapWidget();
    dragSource = (QWidget *)(drag_object->parent());
    oldstate = ButtonState(-1); // #### Should use state that caused the drag
    drag_mode = mode;
    global_accepted_action = QDropEvent::Copy; // #####
    willDrop = false;
    updateMode(ButtonState(0));
    updatePixmap();
    updateCursor();
    restoreCursor = true;
    dropWidget = NULL;
    qApp->installEventFilter(this);
    return true;
}

void QDragManager::updateMode(ButtonState newstate)
{
    if (newstate == oldstate)
        return;
    const int both = ShiftButton|ControlButton;
    if ((newstate & both) == both) {
        global_requested_action = QDropEvent::Link;
    } else {
        bool local = drag_object != 0;
        if (drag_mode == QDragObject::DragMove)
            global_requested_action = QDropEvent::Move;
        else if (drag_mode == QDragObject::DragCopy)
            global_requested_action = QDropEvent::Copy;
        else {
            if (drag_mode == QDragObject::DragDefault && local)
                global_requested_action = QDropEvent::Move;
            else
                global_requested_action = QDropEvent::Copy;
            if (newstate & ShiftButton)
                global_requested_action = QDropEvent::Move;
            else if (newstate & ControlButton)
                global_requested_action = QDropEvent::Copy;
        }
    }
    oldstate = newstate;
}

void QDragManager::cancel(bool deleteSource)
{
    if (dropWidget) {
        QDragLeaveEvent dle;
        QApplication::sendEvent(dropWidget, &dle);
    }

#ifndef QT_NO_CURSOR
    if (restoreCursor) {
        myRestoreOverrideCursor();
        restoreCursor = false;
    }
#endif

    if (drag_object) {
        if (deleteSource)
            delete object;
        drag_object = object = 0;
    }

    delete qt_qws_dnd_deco;
    qt_qws_dnd_deco = 0;
}


void QDragManager::drop()
{
    if (!dropWidget)
        return;

    delete qt_qws_dnd_deco;
    qt_qws_dnd_deco = 0;

    QDropEvent de(QCursor::pos());
    QApplication::sendEvent(dropWidget, &de);

#ifndef QT_NO_CURSOR
    if (restoreCursor) {
        myRestoreOverrideCursor();
        restoreCursor = false;
    }
#endif
}

#endif // QT_NO_DRAGANDDROP

