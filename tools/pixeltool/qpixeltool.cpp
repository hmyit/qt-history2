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

#include "qpixeltool.h"

#include <QtAssistant/QAssistantClient>

#include <qapplication.h>
#include <qdesktopwidget.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qpainter.h>
#include <qevent.h>
#include <qfiledialog.h>
#include <qsettings.h>
#include <qmenu.h>
#include <qactiongroup.h>
#include <QtCore/QLibraryInfo>

#include <qdebug.h>

QPixelTool::QPixelTool(QWidget *parent)
    : QWidget(parent),
      m_assistantClient(0)
{
    QSettings settings("Trolltech", "QPixelTool");

    m_freeze = false;

    m_autoUpdate = settings.value("autoUpdate", 0).toBool();

    m_gridSize = settings.value("gridSize", 1).toInt();
    m_gridActive = settings.value("gridActive", 1).toInt();
    m_displayGridSize = false;
    m_displayGridSizeId = 0;

    m_zoom = settings.value("zoom", 4).toInt();
    m_displayZoom = false;
    m_displayZoomId = 0;

    m_currentColor = 0;

    m_mouseDown = false;

    m_initialSize = settings.value("initialSize", QSize(250, 200)).toSize();

    move(settings.value("position").toPoint());

    setMouseTracking(true);
    setAttribute(Qt::WA_NoBackground);
    m_updateId = startTimer(30);
}

QPixelTool::~QPixelTool()
{
    QSettings settings("Trolltech", "QPixelTool");
    settings.setValue("autoUpdate", int(m_autoUpdate));
    settings.setValue("gridSize", m_gridSize);
    settings.setValue("gridActive", m_gridActive);
    settings.setValue("zoom", m_zoom);
    settings.setValue("initialSize", size());
    settings.setValue("position", pos());
}

void QPixelTool::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_updateId && !m_freeze) {
        grabScreen();
    } else if (event->timerId() == m_displayZoomId) {
        killTimer(m_displayZoomId);
        setZoomVisible(false);
    } else if (event->timerId() == m_displayGridSizeId) {
        killTimer(m_displayGridSizeId);
        m_displayGridSize = false;
    }
}

void render_string(QPainter *p, int w, int h, const QString &text, int flags)
{
    p->setBrush(QColor(255, 255, 255, 191));
    p->setPen(Qt::black);
    QRect bounds;
    p->drawText(0, 0, w, h, Qt::TextDontPrint | flags, text, &bounds);

    if (bounds.x() == 0) bounds.adjust(0, 0, 10, 0);
    else bounds.adjust(-10, 0, 0, 0);

    if (bounds.y() == 0) bounds.adjust(0, 0, 0, 10);
    else bounds.adjust(0, -10, 0, 0);

    p->drawRect(bounds);
    p->drawText(bounds, flags, text);
}

void QPixelTool::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    int w = width();
    int h = height();

    if (m_zoom <= 4) {
        int wext = width() % m_zoom;
        int hext = height() % m_zoom;
        p.drawPixmap(0, 0, width() + wext, height() + hext, m_buffer);
    } else {
        p.setPen(Qt::NoPen);
        QImage im = m_buffer.toImage().convertToFormat(QImage::Format_RGB32);
        for (int y=0; y<h; y+=m_zoom) {
            int y_px = qMin(im.height()-1, y/m_zoom);
            for (int x=0; x<w; x+=m_zoom) {
                int x_px = qMin(im.width()-1, x/m_zoom);
                p.setBrush(QColor(im.pixel(x_px, y_px)));
                p.drawRect(x, y, m_zoom, m_zoom);
            }
        }
    }

    // Draw the grid on top.
    if (m_gridActive) {
        p.setPen(m_gridActive == 1 ? Qt::black : Qt::white);
        int incr = m_gridSize * m_zoom;
        for (int x=0; x<w; x+=incr)
            p.drawLine(x, 0, x, h);
        for (int y=0; y<h; y+=incr)
            p.drawLine(0, y, w, y);
    }

    QFont f("courier");
    f.setBold(true);
    p.setFont(f);

    if (m_displayZoom) {
        render_string(&p, w, h,
                      QString("Zoom: x%1").arg(m_zoom),
                      Qt::AlignBottom | Qt::AlignRight);
    }

    if (m_displayGridSize) {
        render_string(&p, w, h,
                      QString("Grid size: %1").arg(m_gridSize),
                      Qt::AlignBottom | Qt::AlignLeft);
    }

    if (m_freeze) {
        QString str;
        str.sprintf("Pixel: %6X\nRed:   %3d\nGreen: %3d\nBlue:  %3d",
                    0x00ffffff & m_currentColor,
                    (0x00ff0000 & m_currentColor) >> 16,
                    (0x0000ff00 & m_currentColor) >> 8,
                    (0x000000ff & m_currentColor));
        render_string(&p, w, h,
                      str,
                      Qt::AlignTop | Qt::AlignLeft);
    }

    if (m_mouseDown && m_dragStart != m_dragCurrent) {
        int x1 = (m_dragStart.x() / m_zoom) * m_zoom;
        int y1 = (m_dragStart.y() / m_zoom) * m_zoom;
        int x2 = (m_dragCurrent.x() / m_zoom) * m_zoom;
        int y2 = (m_dragCurrent.y() / m_zoom) * m_zoom;
        QRect r = QRect(x1, y1, x2 - x1, y2 - y1).normalized();
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(Qt::red, 3, Qt::SolidLine));
        p.drawRect(r);
        p.setPen(QPen(Qt::black, 1, Qt::SolidLine));
        p.drawRect(r);

        QString str;
        str.sprintf("Rect: x=%d, y=%d, w=%d, h=%d",
                    r.x() / m_zoom,
                    r.y() / m_zoom,
                    r.width() / m_zoom,
                    r.height() / m_zoom);
        render_string(&p, w, h, str, Qt::AlignBottom | Qt::AlignLeft);
    }


}

void QPixelTool::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Space:
        toggleFreeze();
        break;
    case Qt::Key_Plus:
        setZoom(m_zoom + 1);
        break;
    case Qt::Key_Minus:
        setZoom(m_zoom - 1);
        break;
    case Qt::Key_PageUp:
        setGridSize(m_gridSize + 1);
        break;
    case Qt::Key_PageDown:
        setGridSize(m_gridSize - 1);
        break;
    case Qt::Key_G:
        toggleGrid();
        break;
    case Qt::Key_A:
        m_autoUpdate = !m_autoUpdate;
        break;
    case Qt::Key_C:
        if (e->modifiers() & Qt::ControlModifier)
            copyToClipboard();
        break;
    case Qt::Key_S:
        if (e->modifiers() & Qt::ControlModifier) {
            releaseKeyboard();
            saveToFile();
        }
        break;
    case Qt::Key_Control:
        grabKeyboard();
        break;
    case Qt::Key_F1:
        showHelp();
        break;
    }
}

void QPixelTool::keyReleaseEvent(QKeyEvent *e)
{
    switch(e->key()) {
    case Qt::Key_Control:
        releaseKeyboard();
        break;
    default:
        break;
    }
}

void QPixelTool::resizeEvent(QResizeEvent *)
{
    grabScreen();
}

void QPixelTool::mouseMoveEvent(QMouseEvent *e)
{
    if (m_mouseDown)
        m_dragCurrent = e->pos();

    int x = e->x() / m_zoom;
    int y = e->y() / m_zoom;
    QImage im = m_buffer.toImage().convertToFormat(QImage::Format_RGB32);
    m_currentColor = im.pixel(x, y);
    update();
}

void QPixelTool::mousePressEvent(QMouseEvent *e)
{
    if (!m_freeze)
        return;
    m_mouseDown = true;
    m_dragStart = e->pos();
}

void QPixelTool::mouseReleaseEvent(QMouseEvent *)
{
    m_mouseDown = false;
}

void QPixelTool::contextMenuEvent(QContextMenuEvent *e)
{
    bool tmpFreeze = m_freeze;
    m_freeze = true;

    QMenu menu;

    QAction title("Qt Pixel Zooming Tool", &menu);
    title.setEnabled(false);

    // Grid color options...
    QActionGroup gridGroup(this);
    QAction whiteGrid("White grid", &gridGroup);
    whiteGrid.setCheckable(true);
    whiteGrid.setChecked(m_gridActive == 2);
    whiteGrid.setShortcut(QKeySequence(Qt::Key_G));
    QAction blackGrid("Black grid", &gridGroup);
    blackGrid.setCheckable(true);
    blackGrid.setChecked(m_gridActive == 1);
    blackGrid.setShortcut(QKeySequence(Qt::Key_G));
    QAction noGrid("No grid", &gridGroup);
    noGrid.setCheckable(true);
    noGrid.setChecked(m_gridActive == 0);
    noGrid.setShortcut(QKeySequence(Qt::Key_G));

    // Grid size options
    QAction incrGrid("Increase grid size", &menu);
    incrGrid.setShortcut(QKeySequence(Qt::Key_PageUp));
    connect(&incrGrid, SIGNAL(triggered()), this, SLOT(increaseGridSize()));
    QAction decrGrid("Decrease grid size", &menu);
    decrGrid.setShortcut(QKeySequence(Qt::Key_PageDown));
    connect(&decrGrid, SIGNAL(triggered()), this, SLOT(decreaseGridSize()));

    // Zoom options
    QAction incrZoom("Zoom in", &menu);
    incrZoom.setShortcut(QKeySequence(Qt::Key_Plus));
    connect(&incrZoom, SIGNAL(triggered()), this, SLOT(increaseZoom()));
    QAction decrZoom("Zoom out", &menu);
    decrZoom.setShortcut(QKeySequence(Qt::Key_Minus));
    connect(&decrZoom, SIGNAL(triggered()), this, SLOT(decreaseZoom()));

    // Freeze / Autoupdate
    QAction freeze("Frozen", &menu);
    freeze.setCheckable(true);
    freeze.setChecked(tmpFreeze);
    freeze.setShortcut(QKeySequence(Qt::Key_Space));
    QAction autoUpdate("Continous update", &menu);
    autoUpdate.setCheckable(true);
    autoUpdate.setChecked(m_autoUpdate);
    autoUpdate.setShortcut(QKeySequence(Qt::Key_A));

    // Copy to clipboard / save
    QAction save("Save as image", &menu);
    save.setShortcut(QKeySequence("Ctrl+S"));
    connect(&save, SIGNAL(triggered()), this, SLOT(saveToFile()));
    QAction copy("Copy to clipboard", &menu);
    copy.setShortcut(QKeySequence("Ctrl+C"));
    connect(&copy, SIGNAL(triggered()), this, SLOT(copyToClipboard()));

    menu.addAction(&title);
    menu.addSeparator();
    menu.addAction(&whiteGrid);
    menu.addAction(&blackGrid);
    menu.addAction(&noGrid);
    menu.addSeparator();
    menu.addAction(&incrGrid);
    menu.addAction(&decrGrid);
    menu.addSeparator();
    menu.addAction(&incrZoom);
    menu.addAction(&decrZoom);
    menu.addSeparator();
    menu.addAction(&freeze);
    menu.addAction(&autoUpdate);
    menu.addSeparator();
    menu.addAction(&save);
    menu.addAction(&copy);

    menu.exec(mapToGlobal(e->pos()));

    // Read out grid settings
    if (noGrid.isChecked()) m_gridActive = 0;
    else if (blackGrid.isChecked()) m_gridActive = 1;
    else m_gridActive = 2;

    m_autoUpdate = autoUpdate.isChecked();
    tmpFreeze = freeze.isChecked();


    m_freeze = tmpFreeze;
}

QSize QPixelTool::sizeHint() const
{
    return m_initialSize;
}

void QPixelTool::grabScreen()
{
    QPoint mousePos = QCursor::pos();
    if (mousePos == m_lastMousePos && !m_autoUpdate || rect().contains(mapToGlobal(mousePos)))
        return;

    int w = int(width() / float(m_zoom));
    int h = int(height() / float(m_zoom));

    if (width() % m_zoom > 0)
        ++w;
    if (height() % m_zoom > 0)
        ++h;

    int x = mousePos.x() - w/2;
    int y = mousePos.y() - h/2;

    m_buffer = QPixmap::grabWindow(qApp->desktop()->winId(), x, y, w, h);

    update();

    m_lastMousePos = mousePos;
}

void QPixelTool::startZoomVisibleTimer()
{
    if (m_displayZoomId > 0) {
        killTimer(m_displayZoomId);
    }
    m_displayZoomId = startTimer(5000);
    setZoomVisible(true);
}

void QPixelTool::startGridSizeVisibleTimer()
{
    if (m_gridActive) {
        if (m_displayGridSizeId > 0)
            killTimer(m_displayGridSizeId);
        m_displayGridSizeId = startTimer(5000);
        m_displayGridSize = true;
        update();
    }
}

void QPixelTool::setZoomVisible(bool visible)
{
    m_displayZoom = visible;
    update();
}

void QPixelTool::toggleFreeze()
{
    m_freeze = !m_freeze;
    if (!m_freeze)
        m_dragStart = m_dragCurrent = QPoint();
}

void QPixelTool::setZoom(int zoom)
{
    if (zoom > 0) {
        QPoint pos = m_lastMousePos;
        m_lastMousePos = QPoint();
        m_zoom = zoom;
        grabScreen();
        m_lastMousePos = pos;
        m_dragStart = m_dragCurrent = QPoint();
        startZoomVisibleTimer();
    }
}

void QPixelTool::toggleGrid()
{
    if (++m_gridActive > 2)
        m_gridActive = 0;
    update();
}

void QPixelTool::setGridSize(int gridSize)
{
    if (m_gridActive && gridSize > 0) {
        m_gridSize = gridSize;
        startGridSizeVisibleTimer();
        update();
    }
}

void QPixelTool::copyToClipboard()
{
    QClipboard *cb = QApplication::clipboard();
    cb->setPixmap(m_buffer);
}

void QPixelTool::saveToFile()
{
    bool oldFreeze = m_freeze;
    m_freeze = true;
    QString name = QFileDialog::getSaveFileName(this, "Save as image", QString(), "*.png");
    if (!name.endsWith(".png"))
        name.append(".png");
    m_buffer.save(name, "PNG");
    m_freeze = oldFreeze;
}

void QPixelTool::showHelp()
{
    if (!m_assistantClient)
        m_assistantClient
            = new QAssistantClient(
                QLibraryInfo::location(QLibraryInfo::BinariesPath), this);
    QString filePath = QLibraryInfo::location(QLibraryInfo::DocumentationPath)
                       + QLatin1String("/html/pixeltool-manual.html");
    
    m_assistantClient->showPage(filePath);
}
