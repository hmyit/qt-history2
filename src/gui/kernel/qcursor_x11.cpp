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

#include <qdatastream.h>
#include <private/qcursor_p.h>
#include <private/qt_x11_p.h>
#include <qbitmap.h>
#include <qcursor.h>
#include <X11/cursorfont.h>

#ifndef QT_NO_XCURSOR
#  include <X11/Xcursor/Xcursor.h>
#endif // QT_NO_XCURSOR

#include "qx11info_x11.h"

// Define QT_USE_APPROXIMATE_CURSORS when compiling if you REALLY want to
// use the ugly X11 cursors.

extern QCursorData *qt_cursorTable[Qt::LastCursor + 1]; // qcursor.cpp

/*****************************************************************************
  Internal QCursorData class
 *****************************************************************************/

QCursorData::QCursorData(Qt::CursorShape s)
    : cshape(s), bm(0), bmm(0), hx(0), hy(0), hcurs(0), pm(0), pmm(0)
{
    ref = 1;
}

QCursorData::~QCursorData()
{
    Display *dpy = X11->display;

    // Add in checking for the display too as on HP-UX
    // we seem to get a core dump as the cursor data is
    // deleted again from main() on exit...
    if (hcurs && dpy)
        XFreeCursor(dpy, hcurs);
    if (pm && dpy)
        XFreePixmap(dpy, pm);
    if (pmm && dpy)
        XFreePixmap(dpy, pmm);
    delete bm;
    delete bmm;
}


/*!
    Constructs a cursor from the window system cursor \a cursor.

    \warning Using this function is not portable. This function is only
    available on X11 and Windows.
*/
QCursor::QCursor(Qt::HANDLE cursor)
{
    if (!QCursorData::initialized)
        QCursorData::initialize();
    d = new QCursorData;
    d->hcurs = cursor;
}



QCursorData *QCursorData::setBitmap(const QBitmap &bitmap, const QBitmap &mask, int hotX, int hotY)
{
    if (!QCursorData::initialized)
        QCursorData::initialize();
    if (bitmap.depth() != 1 || mask.depth() != 1 || bitmap.size() != mask.size()) {
        qWarning("QCursor: Cannot create bitmap cursor; invalid bitmap(s)");
        QCursorData *c = qt_cursorTable[0];
        c->ref.ref();
        return c;
    }
    QCursorData *d = new QCursorData;
    d->ref = 1;
    d->bm  = new QBitmap(bitmap);
    d->bmm = new QBitmap(mask);
    d->hcurs = 0;
    d->cshape = Qt::BitmapCursor;
    d->hx = hotX >= 0 ? hotX : bitmap.width() / 2;
    d->hy = hotY >= 0 ? hotY : bitmap.height() / 2;
    d->fg.red   = 0x0000;
    d->fg.green = 0x0000;
    d->fg.blue  = 0x0000;
    d->bg.red   = 0xffff;
    d->bg.green = 0xffff;
    d->bg.blue  = 0xffff;
    return d;
}



Qt::HANDLE QCursor::handle() const
{
    if (!QCursorData::initialized)
        QCursorData::initialize();
    if (!d->hcurs)
        d->update();
    return d->hcurs;
}

/*!
    \fn QCursor::QCursor(HCURSOR handle)

    Creates a cursor with the specified window system handle \a
    handle.

    This function is Windows-specific.
*/

/*!
    Returns the position of the cursor (hot spot) in global screen
    coordinates.

    You can call QWidget::mapFromGlobal() to translate it to widget
    coordinates.

    \sa setPos(), QWidget::mapFromGlobal(), QWidget::mapToGlobal()
*/
QPoint QCursor::pos()
{
    Window root;
    Window child;
    int root_x, root_y, win_x, win_y;
    uint buttons;
    Display* dpy = X11->display;
    for (int i = 0; i < ScreenCount(dpy); ++i) {
        if (XQueryPointer(dpy, QX11Info::appRootWindow(i), &root, &child, &root_x, &root_y,
                          &win_x, &win_y, &buttons))

            return QPoint(root_x, root_y);
    }
    return QPoint();
}

/*! \internal
*/
int QCursor::x11Screen()
{
    Window root;
    Window child;
    int root_x, root_y, win_x, win_y;
    uint buttons;
    Display* dpy = X11->display;
    for (int i = 0; i < ScreenCount(dpy); ++i) {
        if (XQueryPointer(dpy, QX11Info::appRootWindow(i), &root, &child, &root_x, &root_y,
                          &win_x, &win_y, &buttons))
            return i;
    }
    return -1;
}

/*!
    Moves the cursor (hot spot) to the global screen position (\a x,
    \a y).

    You can call QWidget::mapToGlobal() to translate widget
    coordinates to global screen coordinates.

    \sa pos(), QWidget::mapFromGlobal(), QWidget::mapToGlobal()
*/

void QCursor::setPos(int x, int y)
{
    QPoint current, target(x, y);

    // this is copied from pos(), since we need the screen number for the correct
    // root window in the XWarpPointer call
    Window root;
    Window child;
    int root_x, root_y, win_x, win_y;
    uint buttons;
    Display* dpy = X11->display;
    int screen;
    for (screen = 0; screen < ScreenCount(dpy); ++screen) {
        if (XQueryPointer(dpy, QX11Info::appRootWindow(screen), &root, &child, &root_x, &root_y,
                          &win_x, &win_y, &buttons)) {
            current = QPoint(root_x, root_y);
            break;
        }
    }

    if (screen >= ScreenCount(dpy))
        return;

    // Need to check, since some X servers generate null mouse move
    // events, causing looping in applications which call setPos() on
    // every mouse move event.
    //
    if (current == target)
        return;

    XWarpPointer(X11->display, XNone, QX11Info::appRootWindow(screen), 0, 0, 0, 0, x, y);
}

/*!
    \fn void QCursor::setPos (const QPoint &p)

    \overload

    Moves the cursor (hot spot) to the global screen position at point
    \a p.
*/


/*!
    \internal

    Creates the cursor.
*/

void QCursorData::update()
{
    if (!QCursorData::initialized)
        QCursorData::initialize();
    if (hcurs)
        return;

    Display *dpy = X11->display;
    Window rootwin = QX11Info::appRootWindow();

    if (cshape == Qt::BitmapCursor) {
#ifndef QT_NO_XFT
        if (!pixmap.isNull() && X11->use_xrender) {
            hcurs = XRenderCreateCursor (X11->display, pixmap.xftPictureHandle(), hx, hy);
        } else
#endif
        {
            hcurs = XCreatePixmapCursor(dpy, bm->handle(), bmm->handle(), &fg, &bg, hx, hy);
        }
        return;
    }

#ifndef QT_NO_XCURSOR
    static const char *cursorNames[] = {
        "left_ptr",
        "up_arrow",
        "cross",
        "wait",
        "ibeam",
        "size_ver",
        "size_hor",
        "size_bdiag",
        "size_fdiag",
        "size_all",
        "blank",
        "split_v",
        "split_h",
        "pointing_hand",
        "forbidden",
        "whats_this",
        "left_ptr_watch"
    };

    hcurs = XcursorLibraryLoadCursor(dpy, cursorNames[cshape]);
    if (hcurs)
        return;
#endif // QT_NO_XCURSOR

    static const char cur_blank_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    // Non-standard X11 cursors are created from bitmaps

#ifndef QT_USE_APPROXIMATE_CURSORS
    static const char cur_ver_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0xc0, 0x03, 0xe0, 0x07, 0xf0, 0x0f,
        0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0xf0, 0x0f,
        0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00 };
    static const char mcur_ver_bits[] = {
        0x00, 0x00, 0x80, 0x03, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x1f, 0xf8, 0x3f,
        0xfc, 0x7f, 0xc0, 0x07, 0xc0, 0x07, 0xc0, 0x07, 0xfc, 0x7f, 0xf8, 0x3f,
        0xf0, 0x1f, 0xe0, 0x0f, 0xc0, 0x07, 0x80, 0x03 };
    static const char cur_hor_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x08, 0x30, 0x18,
        0x38, 0x38, 0xfc, 0x7f, 0xfc, 0x7f, 0x38, 0x38, 0x30, 0x18, 0x20, 0x08,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const char mcur_hor_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x40, 0x04, 0x60, 0x0c, 0x70, 0x1c, 0x78, 0x3c,
        0xfc, 0x7f, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfc, 0x7f, 0x78, 0x3c,
        0x70, 0x1c, 0x60, 0x0c, 0x40, 0x04, 0x00, 0x00 };
    static const char cur_bdiag_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x3e, 0x00, 0x3c, 0x00, 0x3e,
        0x00, 0x37, 0x88, 0x23, 0xd8, 0x01, 0xf8, 0x00, 0x78, 0x00, 0xf8, 0x00,
        0xf8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const char mcur_bdiag_bits[] = {
        0x00, 0x00, 0xc0, 0x7f, 0x80, 0x7f, 0x00, 0x7f, 0x00, 0x7e, 0x04, 0x7f,
        0x8c, 0x7f, 0xdc, 0x77, 0xfc, 0x63, 0xfc, 0x41, 0xfc, 0x00, 0xfc, 0x01,
        0xfc, 0x03, 0xfc, 0x07, 0x00, 0x00, 0x00, 0x00 };
    static const char cur_fdiag_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x01, 0xf8, 0x00, 0x78, 0x00,
        0xf8, 0x00, 0xd8, 0x01, 0x88, 0x23, 0x00, 0x37, 0x00, 0x3e, 0x00, 0x3c,
        0x00, 0x3e, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00 };
    static const char mcur_fdiag_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0xfc, 0x07, 0xfc, 0x03, 0xfc, 0x01, 0xfc, 0x00,
        0xfc, 0x41, 0xfc, 0x63, 0xdc, 0x77, 0x8c, 0x7f, 0x04, 0x7f, 0x00, 0x7e,
        0x00, 0x7f, 0x80, 0x7f, 0xc0, 0x7f, 0x00, 0x00 };
    static const char *cursor_bits16[] = {
        cur_ver_bits, mcur_ver_bits, cur_hor_bits, mcur_hor_bits,
        cur_bdiag_bits, mcur_bdiag_bits, cur_fdiag_bits, mcur_fdiag_bits,
        0, 0, cur_blank_bits, cur_blank_bits };

    static const char vsplit_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x80, 0x00, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00,
        0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
        0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xff, 0x7f, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x7f, 0x00,
        0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
        0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x00,
        0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const char vsplitm_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
        0x00, 0xc0, 0x01, 0x00, 0x00, 0xe0, 0x03, 0x00, 0x00, 0xf0, 0x07, 0x00,
        0x00, 0xf8, 0x0f, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
        0x00, 0xc0, 0x01, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00,
        0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00, 0x80, 0xff, 0xff, 0x00,
        0x80, 0xff, 0xff, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0xc0, 0x01, 0x00,
        0x00, 0xc0, 0x01, 0x00, 0x00, 0xf8, 0x0f, 0x00, 0x00, 0xf0, 0x07, 0x00,
        0x00, 0xe0, 0x03, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const char hsplit_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
        0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
        0x00, 0x41, 0x82, 0x00, 0x80, 0x41, 0x82, 0x01, 0xc0, 0x7f, 0xfe, 0x03,
        0x80, 0x41, 0x82, 0x01, 0x00, 0x41, 0x82, 0x00, 0x00, 0x40, 0x02, 0x00,
        0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00,
        0x00, 0x40, 0x02, 0x00, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const char hsplitm_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00,
        0x00, 0xe0, 0x07, 0x00, 0x00, 0xe2, 0x47, 0x00, 0x00, 0xe3, 0xc7, 0x00,
        0x80, 0xe3, 0xc7, 0x01, 0xc0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x07,
        0xc0, 0xff, 0xff, 0x03, 0x80, 0xe3, 0xc7, 0x01, 0x00, 0xe3, 0xc7, 0x00,
        0x00, 0xe2, 0x47, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00,
        0x00, 0xe0, 0x07, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const char whatsthis_bits[] = {
        0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x05, 0xf0, 0x07, 0x00,
        0x09, 0x18, 0x0e, 0x00, 0x11, 0x1c, 0x0e, 0x00, 0x21, 0x1c, 0x0e, 0x00,
        0x41, 0x1c, 0x0e, 0x00, 0x81, 0x1c, 0x0e, 0x00, 0x01, 0x01, 0x07, 0x00,
        0x01, 0x82, 0x03, 0x00, 0xc1, 0xc7, 0x01, 0x00, 0x49, 0xc0, 0x01, 0x00,
        0x95, 0xc0, 0x01, 0x00, 0x93, 0xc0, 0x01, 0x00, 0x21, 0x01, 0x00, 0x00,
        0x20, 0xc1, 0x01, 0x00, 0x40, 0xc2, 0x01, 0x00, 0x40, 0x02, 0x00, 0x00,
        0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
    static const char whatsthism_bits[] = {
        0x01, 0x00, 0x00, 0x00, 0x03, 0xf0, 0x07, 0x00, 0x07, 0xf8, 0x0f, 0x00,
        0x0f, 0xfc, 0x1f, 0x00, 0x1f, 0x3e, 0x1f, 0x00, 0x3f, 0x3e, 0x1f, 0x00,
        0x7f, 0x3e, 0x1f, 0x00, 0xff, 0x3e, 0x1f, 0x00, 0xff, 0x9d, 0x0f, 0x00,
        0xff, 0xc3, 0x07, 0x00, 0xff, 0xe7, 0x03, 0x00, 0x7f, 0xe0, 0x03, 0x00,
        0xf7, 0xe0, 0x03, 0x00, 0xf3, 0xe0, 0x03, 0x00, 0xe1, 0xe1, 0x03, 0x00,
        0xe0, 0xe1, 0x03, 0x00, 0xc0, 0xe3, 0x03, 0x00, 0xc0, 0xe3, 0x03, 0x00,
        0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
    static const char busy_bits[] = {
        0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
        0x09, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00,
        0x41, 0xe0, 0xff, 0x00, 0x81, 0x20, 0x80, 0x00, 0x01, 0xe1, 0xff, 0x00,
        0x01, 0x42, 0x40, 0x00, 0xc1, 0x47, 0x40, 0x00, 0x49, 0x40, 0x55, 0x00,
        0x95, 0x80, 0x2a, 0x00, 0x93, 0x00, 0x15, 0x00, 0x21, 0x01, 0x0a, 0x00,
        0x20, 0x01, 0x11, 0x00, 0x40, 0x82, 0x20, 0x00, 0x40, 0x42, 0x44, 0x00,
        0x80, 0x41, 0x4a, 0x00, 0x00, 0x40, 0x55, 0x00, 0x00, 0xe0, 0xff, 0x00,
        0x00, 0x20, 0x80, 0x00, 0x00, 0xe0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    static const char busym_bits[] = {
        0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
        0x0f, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00,
        0x7f, 0xe0, 0xff, 0x00, 0xff, 0xe0, 0xff, 0x00, 0xff, 0xe1, 0xff, 0x00,
        0xff, 0xc3, 0x7f, 0x00, 0xff, 0xc7, 0x7f, 0x00, 0x7f, 0xc0, 0x7f, 0x00,
        0xf7, 0x80, 0x3f, 0x00, 0xf3, 0x00, 0x1f, 0x00, 0xe1, 0x01, 0x0e, 0x00,
        0xe0, 0x01, 0x1f, 0x00, 0xc0, 0x83, 0x3f, 0x00, 0xc0, 0xc3, 0x7f, 0x00,
        0x80, 0xc1, 0x7f, 0x00, 0x00, 0xc0, 0x7f, 0x00, 0x00, 0xe0, 0xff, 0x00,
        0x00, 0xe0, 0xff, 0x00, 0x00, 0xe0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    static const char * const cursor_bits32[] = {
        vsplit_bits, vsplitm_bits, hsplit_bits, hsplitm_bits,
        0, 0, 0, 0, whatsthis_bits, whatsthism_bits, busy_bits, busym_bits
    };

    static const char forbidden_bits[] = {
        0x00,0x00,0x00,0x80,0x1f,0x00,0xe0,0x7f,0x00,0xf0,0xf0,0x00,0x38,0xc0,0x01,
        0x7c,0x80,0x03,0xec,0x00,0x03,0xce,0x01,0x07,0x86,0x03,0x06,0x06,0x07,0x06,
        0x06,0x0e,0x06,0x06,0x1c,0x06,0x0e,0x38,0x07,0x0c,0x70,0x03,0x1c,0xe0,0x03,
        0x38,0xc0,0x01,0xf0,0xe0,0x00,0xe0,0x7f,0x00,0x80,0x1f,0x00,0x00,0x00,0x00 };

    static const char forbiddenm_bits[] = {
        0x80,0x1f,0x00,0xe0,0x7f,0x00,0xf0,0xff,0x00,0xf8,0xff,0x01,0xfc,0xf0,0x03,
        0xfe,0xc0,0x07,0xfe,0x81,0x07,0xff,0x83,0x0f,0xcf,0x07,0x0f,0x8f,0x0f,0x0f,
        0x0f,0x1f,0x0f,0x0f,0x3e,0x0f,0x1f,0xfc,0x0f,0x1e,0xf8,0x07,0x3e,0xf0,0x07,
        0xfc,0xe0,0x03,0xf8,0xff,0x01,0xf0,0xff,0x00,0xe0,0x7f,0x00,0x80,0x1f,0x00};

    static const char * const cursor_bits20[] = {
        forbidden_bits, forbiddenm_bits
    };

    if (cshape >= Qt::SizeVerCursor && cshape < Qt::SizeAllCursor
            || cshape == Qt::BlankCursor) {
        XColor bg, fg;
        bg.red   = 255 << 8;
        bg.green = 255 << 8;
        bg.blue  = 255 << 8;
        fg.red   = 0;
        fg.green = 0;
        fg.blue  = 0;
        int i = (cshape - Qt::SizeVerCursor) * 2;
        pm  = XCreateBitmapFromData(dpy, rootwin, cursor_bits16[i], 16, 16);
        pmm = XCreateBitmapFromData(dpy, rootwin, cursor_bits16[i + 1], 16, 16);
        hcurs = XCreatePixmapCursor(dpy, pm, pmm, &fg, &bg, 8, 8);
        return;
    }
    if ((cshape >= Qt::SplitVCursor && cshape <= Qt::SplitHCursor)
            || cshape == Qt::WhatsThisCursor || cshape == Qt::BusyCursor) {
        XColor bg, fg;
        bg.red   = 255 << 8;
        bg.green = 255 << 8;
        bg.blue  = 255 << 8;
        fg.red   = 0;
        fg.green = 0;
        fg.blue  = 0;
        int i = (cshape - Qt::SplitVCursor) * 2;
        pm  = XCreateBitmapFromData(dpy, rootwin, cursor_bits32[i], 32, 32);
        pmm = XCreateBitmapFromData(dpy, rootwin, cursor_bits32[i + 1], 32, 32);
        int hs = (cshape == Qt::PointingHandCursor || cshape == Qt::WhatsThisCursor
                  || cshape == Qt::BusyCursor) ? 0 : 16;
        hcurs = XCreatePixmapCursor(dpy, pm, pmm, &fg, &bg, hs, hs);
        return;
    }
    if (cshape == Qt::ForbiddenCursor) {
        XColor bg, fg;
        bg.red   = 255 << 8;
        bg.green = 255 << 8;
        bg.blue  = 255 << 8;
        fg.red   = 0;
        fg.green = 0;
        fg.blue  = 0;
        int i = (cshape - Qt::ForbiddenCursor) * 2;
        pm  = XCreateBitmapFromData(dpy, rootwin, cursor_bits20[i], 20, 20);
        pmm = XCreateBitmapFromData(dpy, rootwin, cursor_bits20[i + 1], 20, 20);
        hcurs = XCreatePixmapCursor(dpy, pm, pmm, &fg, &bg, 10, 10);
        return;
    }
#endif /* ! QT_USE_APPROXIMATE_CURSORS */

    uint sh;
    switch (cshape) {                        // map Q cursor to X cursor
    case Qt::ArrowCursor:
        sh = XC_left_ptr;
        break;
    case Qt::UpArrowCursor:
        sh = XC_center_ptr;
        break;
    case Qt::CrossCursor:
        sh = XC_crosshair;
        break;
    case Qt::WaitCursor:
        sh = XC_watch;
        break;
    case Qt::IBeamCursor:
        sh = XC_xterm;
        break;
    case Qt::SizeAllCursor:
        sh = XC_fleur;
        break;
    case Qt::PointingHandCursor:
        sh = XC_hand2;
        break;
#ifdef QT_USE_APPROXIMATE_CURSORS
    case Qt::SizeBDiagCursor:
        sh = XC_top_right_corner;
        break;
    case Qt::SizeFDiagCursor:
        sh = XC_bottom_right_corner;
        break;
    case Qt::BlankCursor:
        XColor bg, fg;
        bg.red   = 255 << 8;
        bg.green = 255 << 8;
        bg.blue  = 255 << 8;
        fg.red   = 0;
        fg.green = 0;
        fg.blue  = 0;
        pm  = XCreateBitmapFromData(dpy, rootwin, cur_blank_bits, 16, 16);
        pmm = XCreateBitmapFromData(dpy, rootwin, cur_blank_bits, 16, 16);
        hcurs = XCreatePixmapCursor(dpy, pm, pmm, &fg, &bg, 8, 8);
        return;
        break;
    case Qt::SizeVerCursor:
    case Qt::SplitVCursor:
        sh = XC_sb_v_double_arrow;
        break;
    case Qt::SizeHorCursor:
    case Qt::SplitHCursor:
        sh = XC_sb_h_double_arrow;
        break;
    case Qt::WhatsThisCursor:
        sh = XC_question_arrow;
        break;
    case Qt::ForbiddenCursor:
        sh = XC_circle;
        break;
    case Qt::BusyCursor:
        sh = XC_watch;
        break;
#endif /* QT_USE_APPROXIMATE_CURSORS */
    default:
        qWarning("QCursor::update: Invalid cursor shape %d", cshape);
        return;
    }
    hcurs = XCreateFontCursor(dpy, sh);
}
