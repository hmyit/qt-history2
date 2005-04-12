#include <QtGui>

#include "mainwindow.h"

static const char * const listEntries[] = {
    QT_TRANSLATE_NOOP("MainWindow", "First"),
    QT_TRANSLATE_NOOP("MainWindow", "Second"),
    QT_TRANSLATE_NOOP("MainWindow", "Third"),
    0
}; 

MainWindow::MainWindow()
{
    centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    createGroupBox();

    listWidget = new QListWidget;
    for (int i = 0; listEntries[i]; ++i)
        listWidget->addItem(tr(listEntries[i]));

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(groupBox);
    mainLayout->addWidget(listWidget);

    exitAction = new QAction(tr("E&xit"), this);
    exitAction->setShortcut(tr("Ctrl+Q"));
    connect(exitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(exitAction);

    setWindowTitle(tr("Language: %1").arg(tr("English")));
    statusBar()->showMessage(tr("Internationalization Example"));
}

void MainWindow::createGroupBox()
{
    groupBox = new QGroupBox(tr("View"));
    perspectiveRadioButton = new QRadioButton(tr("Perspective"));
    isometricRadioButton = new QRadioButton(tr("Isometric"));
    obliqueRadioButton = new QRadioButton(tr("Oblique"));
    perspectiveRadioButton->setChecked(true);

    QVBoxLayout *groupBoxLayout = new QVBoxLayout(groupBox);
    groupBoxLayout->addWidget(perspectiveRadioButton);
    groupBoxLayout->addWidget(isometricRadioButton);
    groupBoxLayout->addWidget(obliqueRadioButton);
}
