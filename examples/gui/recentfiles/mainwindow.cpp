#include <QtCore> // ### for QSettings
#include <QtGui>

#include "mainwindow.h"

QList<MainWindow *> MainWindow::windowList;

MainWindow::MainWindow()
{
    setAttribute(Qt::WA_DeleteOnClose);

    textEdit = new QTextEdit(this);
    setCentralWidget(textEdit);

    createActions();
    createMenus();
    (void)statusBar();
    windowList.append(this);

    setWindowTitle(tr("Recent Files"));
}

MainWindow::~MainWindow()
{
    windowList.removeAll(this);
}

void MainWindow::newFile()
{
    MainWindow *other = new MainWindow;
    other->show();
}

void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
        loadFile(fileName);
}

void MainWindow::save()
{
    if (curFile.isEmpty())
        saveAs();
    else
        saveFile(curFile);
}

void MainWindow::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this);
    if (fileName.isEmpty())
        return;

    if (QFile::exists(fileName)) {
        int ret = QMessageBox::warning(this, tr("Recent Files"),
                     tr("File %1 already exists.\n"
                        "Do you want to overwrite it?")
                     .arg(QDir::convertSeparators(fileName)),
                     QMessageBox::Yes | QMessageBox::Default,
                     QMessageBox::No | QMessageBox::Escape);
        if (ret == QMessageBox::No)
            return;
    }
    if (!fileName.isEmpty())
        saveFile(fileName);
}

void MainWindow::openRecentFile()
{
    QAction *action = qt_cast<QAction *>(sender());
    if (action)
        loadFile(action->iconText());
}

void MainWindow::about()
{
   QMessageBox::about(this, tr("About Recent Files"),
            tr("The <b>Recent Files</b> example demonstrates how to provide a "
               "recently used file menu in a Qt application."));
}

void MainWindow::createActions()
{
    newAct = new QAction(tr("&New"), this);
    newAct->setShortcut(tr("Ctrl+N"));
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, SIGNAL(activated()), this, SLOT(newFile()));

    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcut(tr("Ctrl+O"));
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(activated()), this, SLOT(open()));

    saveAct = new QAction(tr("&Save"), this);
    saveAct->setShortcut(tr("Ctrl+S"));
    saveAct->setStatusTip(tr("Save the spreadsheet to disk"));
    connect(saveAct, SIGNAL(activated()), this, SLOT(save()));

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setStatusTip(tr("Save the spreadsheet under a new name"));
    connect(saveAsAct, SIGNAL(activated()), this, SLOT(saveAs()));

    for (int i = 0; i < MaxRecentFiles; ++i) {
        recentFileActs[i] = new QAction(this);
        recentFileActs[i]->setVisible(false);
        connect(recentFileActs[i], SIGNAL(activated()),
                this, SLOT(openRecentFile()));
    }

    exitAct = new QAction(tr("&Close"), this);
    exitAct->setShortcut(tr("Ctrl+W"));
    exitAct->setStatusTip(tr("Close this window"));
    connect(exitAct, SIGNAL(activated()), this, SLOT(close()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(activated()), qApp, SLOT(closeAllWindows()));

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(activated()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, SIGNAL(activated()), qApp, SLOT(aboutQt()));
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    separatorAct = fileMenu->addSeparator();
    for (int i = 0; i < MaxRecentFiles; ++i)
        fileMenu->addAction(recentFileActs[i]);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);
    updateRecentFileActions();

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

void MainWindow::loadFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::warning(this, tr("Recent Files"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    QTextStream in(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    textEdit->setPlainText(in.read());
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    statusBar()->message(tr("File loaded"), 2000);
}

void MainWindow::saveFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox::warning(this, tr("Recent Files"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    out << textEdit->plainText();
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    statusBar()->message(tr("File saved"), 2000);
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    curFile = fileName;
    if (curFile.isEmpty())
        setWindowTitle(tr("Recent Files"));
    else
        setWindowTitle(tr("%1 - %2").arg(strippedName(curFile))
                                    .arg(tr("Recent Files")));

#if 0
    QSettings settings("doc.trolltech.com", "Recent Files");
    QStringList files = settings.value("recentFileList");
#else
    QSettings settings;
    settings.setPath("doc.trolltech.com", "Recent Files");
    settings.beginGroup("Recent Files");
    QStringList files = settings.readListEntry("recentFileList");
#endif
    files.removeAll(fileName);
    files.prepend(fileName);

#if 0
    settings.setValue("recentFileList", files);
#else
    settings.writeEntry("recentFileList", files);
    settings.sync();
#endif

    foreach (MainWindow *win, windowList)
        win->updateRecentFileActions();
}

void MainWindow::updateRecentFileActions()
{
#if 0
    QSettings settings("doc.trolltech.com", "Recent Files");
    QStringList files = settings.value("recentFileList");
#else
    QSettings settings;
    settings.setPath("doc.trolltech.com", "Recent Files");
    settings.sync();
    settings.beginGroup("Recent Files");
    QStringList files = settings.readListEntry("recentFileList");
#endif

    int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
        recentFileActs[i]->setText(text);
        recentFileActs[i]->setIconText(files[i]);
        recentFileActs[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
        recentFileActs[j]->setVisible(false);

    separatorAct->setVisible(numRecentFiles > 0);
}

QString MainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}
