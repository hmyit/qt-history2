/****************************************************************************
**
** Copyright (C) 1992-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of an example program for Qt.
** EDITIONS: NOLIMITS
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "mainwindow.h"

#include "colorswatch.h"
#include "toolbar.h"

#include <qaction.h>
#include <qlayout.h>
#include <qmenu.h>
#include <qmenubar.h>
#include <qstatusbar.h>
#include <qtextedit.h>

static const char * const message =
    "<p><b>Qt Main Window Demo</b></p>"

    "<p>This is a demonstration of the QMainWindow, QToolBar and "
    "QDockWindow classes.</p>"

    "<p>The tool bar and dock windows can be dragged around and rearranged "
    "using the mouse or via the menu.</p>"

    "<p>The tool bar contains three different types of buttons:"
    "<ul><li>Normal button with menu (button 1).</li>"
    "<li>Normal buttons (buttons 2, 3, 4 and 5).</li>"
    "<li>Checkable buttons (buttons 6, 7 and 8).</li>"
    "</ul></p>"

    "<p>Each dock window contains a colored frame and a context "
    "(right-click) menu.</p>"

#ifdef Q_WS_MAC
    "<p>On Mac OS X, the \"Black\" dock window has been created as a "
    "<em>Drawer</em>, which is a special kind of QDockWindow.</p>"
#endif
    ;

MainWindow::MainWindow(QWidget *parent, Qt::WFlags flags)
    : QMainWindow(parent, flags)
{
    setObjectName("MainWindow");
    setWindowTitle("Qt Main Window Demo");

    setupActions();
    setupToolBar();
    setupMenuBar();
    setupDockWindows();

    QTextEdit *center = new QTextEdit(this);
    center->setReadOnly(true);
    center->setHtml(tr(message));
    center->setMinimumSize(400, 300);
    setCenterWidget(center);

    statusBar()->message(tr("Status Bar"));
}

void MainWindow::actionTriggered(QAction *action)
{
    qDebug("action '%s' triggered", action->text().local8Bit());
}


void MainWindow::setupActions()
{
    dockWindowActions = new QAction(tr("Dock Windows"), this);
}

void MainWindow::setupToolBar()
{
    toolbar = new ToolBar(this);
    toolbar->setAllowedAreas(Qt::ToolBarAreaTop | Qt::ToolBarAreaBottom);
}

void MainWindow::setupMenuBar()
{
    QMenu *menu = new QMenu(this);
    menu->addAction(tr("Close"), this, SLOT(close()));

    QMenu *dockWindowMenu = new QMenu(this);
    dockWindowActions->setMenu(dockWindowMenu);

    menuBar()->addMenu(tr("File"), menu);
    menuBar()->addMenu(tr("Tool Bar"), toolbar->menu);
    menuBar()->addAction(dockWindowActions);
}

void MainWindow::setupDockWindows()
{
    static const struct Set {
        const char * name;
        uint flags;
        Qt::DockWindowArea area;
        uint allowedAreas;
        uint features;
    } sets [] = {
        { "Black", Qt::WMacDrawer, Qt::DockWindowAreaLeft,
          Qt::DockWindowAreaLeft | Qt::DockWindowAreaRight,
          QDockWindow::DockWindowClosable },

        { "White", 0, Qt::DockWindowAreaRight,
          Qt::DockWindowAreaLeft | Qt::DockWindowAreaRight,
          QDockWindow::AllDockWindowFeatures },

        { "Red", 0, Qt::DockWindowAreaTop,
          Qt::AllDockWindowAreas,
          QDockWindow::DockWindowClosable | QDockWindow::DockWindowMovable },
        { "Green", 0, Qt::DockWindowAreaTop,
          Qt::AllDockWindowAreas,
          QDockWindow::DockWindowClosable | QDockWindow::DockWindowMovable },

        { "Blue", 0, Qt::DockWindowAreaBottom,
          Qt::AllDockWindowAreas,
          QDockWindow::DockWindowClosable | QDockWindow::DockWindowMovable },
        { "Yellow", 0, Qt::DockWindowAreaBottom,
          Qt::AllDockWindowAreas,
          QDockWindow::DockWindowClosable | QDockWindow::DockWindowMovable }
    };
    const int setCount = sizeof(sets) / sizeof(Set);

    for (int i = 0; i < setCount; ++i) {
        ColorSwatch *swatch = new ColorSwatch(tr(sets[i].name), this, Qt::WFlags(sets[i].flags));
        swatch->setAllowedAreas(Qt::DockWindowAreas(sets[i].allowedAreas));
        swatch->setFeatures(QDockWindow::DockWindowFeatures(sets[i].features));
        swatch->setArea(sets[i].area);

        dockWindowActions->menu()->addMenu(tr(sets[i].name), swatch->menu);
    }
}
