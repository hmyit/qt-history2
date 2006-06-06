/****************************************************************************
**
** Copyright (C) 2006-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QtGui>
#include "dirmodel.h"
#include "historymodel.h"
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), completer(0)
{
    createMenu();

    QWidget *centralWidget = new QWidget;
    lineEdit = new QLineEdit;
    
    QLabel *modelLabel = new QLabel;
    modelLabel->setText(tr("Model"));

    modelCombo = new QComboBox;
    modelCombo->addItem(tr("QDirModel"));
    modelCombo->addItem(tr("QDirModel that shows full path"));
    modelCombo->addItem(tr("QDirModel with history"));
    modelCombo->addItem(tr("Country list"));
    modelCombo->addItem(tr("Word list"));
    modelCombo->addItem(tr("Very big Word list"));
    modelCombo->setCurrentIndex(0);

    QLabel *modeLabel = new QLabel;
    modeLabel->setText(tr("Completion Mode"));
    QComboBox *modeCombo = new QComboBox;
    modeCombo->addItem(tr("Inline"));
    modeCombo->addItem(tr("Filtered Popup"));
    modeCombo->addItem(tr("Unfiltered Popup"));
    modeCombo->setCurrentIndex(1);

    QLabel *caseLabel = new QLabel;
    caseLabel->setText(tr("Case Sensitivity"));
    caseCombo = new QComboBox;
    caseCombo->addItem(tr("Case Insensitive"));
    caseCombo->addItem(tr("Case Sensitive"));
    caseCombo->setCurrentIndex(0);

    contentsLabel = new QLabel;
    contentsLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    progressLabel = new QLabel;

    progressBar = new QProgressBar;
    progressBar->setMinimum(0);
    progressBar->hide();

    connect(modelCombo, SIGNAL(activated(int)), this, SLOT(updateCompletionModel()));
    connect(modeCombo, SIGNAL(activated(int)), this, SLOT(changeMode(int)));
    connect(caseCombo, SIGNAL(activated(int)), this, SLOT(changeCase(int)));

    QGridLayout *layout = new QGridLayout; 
    layout->addWidget(modelLabel, 0, 0); layout->addWidget(modelCombo, 0, 1);
    layout->addWidget(modeLabel, 1, 0);  layout->addWidget(modeCombo, 1, 1);
    layout->addWidget(caseLabel, 2, 0);  layout->addWidget(caseCombo, 2, 1);
    layout->addWidget(contentsLabel, 3, 0, 1, 2);
    layout->addWidget(lineEdit, 4, 0, 1, 2);
    layout->addWidget(progressLabel, 5, 0, 1, 2);
    layout->addWidget(progressBar, 6, 0, 1, 2);

    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    updateCompletionModel();
    changeCase(caseCombo->currentIndex());
    changeMode(modeCombo->currentIndex());

    setWindowTitle(tr("Completer"));
}

void MainWindow::createMenu()
{
    QAction *exitAction = new QAction(tr("Exit"), this);
    QAction *aboutAct = new QAction(tr("About"), this);
    QAction *aboutQtAct = new QAction(tr("About Qt"), this);

    connect(exitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    QMenu* fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu->addAction(exitAction);

    QMenu* helpMenu = menuBar()->addMenu(tr("About"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

void MainWindow::changeMode(int index)
{
    QCompleter::CompletionMode mode;
    if (index == 0)
        mode = QCompleter::InlineCompletion;
    else if (index == 1)
        mode = QCompleter::PopupCompletion;
    else
        mode = QCompleter::UnfilteredPopupCompletion;

    completer->setCompletionMode(mode);
}

QAbstractItemModel *MainWindow::modelFromFile(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly))
        return new QStringListModel(completer);

    int sz = file.size();
    int est_words = sz/10; 
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    progressLabel->setText(tr("Reading word list"));
    progressBar->setMinimum(0);
    progressBar->setMaximum(est_words);
    progressBar->setValue(0);
    progressLabel->show();
    progressBar->show();
    qApp->processEvents(0);

    QStringList words;
    
    for(int num_words = 0;;++num_words) {
        QByteArray line = file.readLine();
        if (line.isEmpty())
            break;
        words << line.trimmed();
        if (num_words % 300) {
            progressBar->setValue(num_words);
        }
    }

    progressLabel->hide();
    progressBar->hide();
    QApplication::restoreOverrideCursor();

    if (fileName.compare(QLatin1String("countries.txt")) != 0)
        return new QStringListModel(words, completer);

    // The last two chars of the countries.txt file indicate the country
    // symbol. We put that in column 2 of a standard item model
    // To make this model really cool, you can add the country flag
    // as the decoration role of column 0 ;)
    QStandardItemModel *m = new QStandardItemModel(words.count(), 2, completer);
    for (int i = 0; i < words.count(); ++i) {
        QModelIndex countryIdx = m->index(i, 0);
        QModelIndex symbolIdx = m->index(i, 1);
        QString country = words[i].mid(0, words[i].length() - 2).trimmed();
        QString symbol = words[i].right(2);
        m->setData(countryIdx, country);
        m->setData(symbolIdx, symbol);
    }
    
    return m;
}

void MainWindow::updateCompletionModel()
{
    if (modelCombo->currentIndex() == 2)
        completer = new HistoryCompleter(lineEdit);
    else
        completer = new QCompleter(lineEdit);

    lineEdit->setCompleter(completer);
    lineEdit->setFocus();

    switch (modelCombo->currentIndex()) {
    default:
    case 0:
        { // Unsorted QDirModel
            QDirModel *dirModel = new QDirModel(completer);
            completer->setModel(dirModel);
            completer->setModelSorting(QCompleter::UnsortedModel);
            contentsLabel->setText(tr("Enter file path"));
        }
        break;
    case 1:
        { // DirModel that shows full paths
            DirModel *dirModel = new DirModel(completer);
            completer->setModel(dirModel);
            completer->setModelSorting(QCompleter::UnsortedModel);
            contentsLabel->setText(tr("Enter file path"));
        }
        break;
    case 2:
        { // DirModel that stores previous paths (history model)
            HistoryModel *historyModel = new HistoryModel(completer);
            completer->setModel(historyModel);
            lineEdit->setCompleter(completer);
            QObject::connect(lineEdit, SIGNAL(returnPressed()), historyModel, SLOT(addToHistory()));
            completer->setModelSorting(QCompleter::UnsortedModel);
            contentsLabel->setText(tr("Enter file path or url or previously typed path"));
        }
        break;

    case 3:
        { // Country List
            completer->setModel(modelFromFile(":/resources/countries.txt"));
            completer->setModelSorting(QCompleter::UnsortedModel);
            contentsLabel->setText(tr("Enter name of your country"));
        }
        break;
    case 4:
        { // Word list
            completer->setModel(modelFromFile(":/resources/wordlist.txt"));
            completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
            contentsLabel->setText(tr("Enter a word"));
        }
        break;
    }
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About"), tr("This example demonstrates the "
        "different features of the QCompleter class."));
}

void MainWindow::changeCase(int cs)
{
    completer->setCaseSensitivity(cs ? Qt::CaseSensitive : Qt::CaseInsensitive);
}
