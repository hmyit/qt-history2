/****************************************************************************
**
** Copyright (C) 2006-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_DECLARE_CLASS(QAbstractItemModel)
QT_DECLARE_CLASS(QComboBox)
QT_DECLARE_CLASS(QCompleter)
QT_DECLARE_CLASS(QLabel)
QT_DECLARE_CLASS(QLineEdit)
QT_DECLARE_CLASS(QProgressBar)
class TextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);

private slots:
    void about();

private:
    void createMenu();
    QAbstractItemModel *modelFromFile(const QString& fileName);

    QCompleter *completer;
    TextEdit *completingTextEdit;
};

#endif // MAINWINDOW_H
