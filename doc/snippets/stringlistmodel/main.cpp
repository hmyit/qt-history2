/****************************************************************************
**
** Copyright (C) 2004-$THISYEAR$ Trolltech AS. All rights reserved.
**
** This file is part of an example program for Qt.
** EDITIONS: NOLIMITS
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/*!
    The main function for the string list model example. This creates and
    populates a model with values from a string list then displays the
    contents of the model using a QGenericListView widget.
*/

#include <qapplication.h>
#include <qgenericlistview.h>

#include "model.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QStringList numbers;
    numbers << "One" << "Two" << "Three" << "Nine" << "Ten" << "Ten" << "Ten";

    QAbstractItemModel *model = new StringListModel(numbers);
    QAbstractItemView *view = new QGenericListView();
    view->setWindowTitle("View onto a string list model");
    view->setModel(model);

    model->insertRows(3, QModelIndex(), 5);
    model->removeRows(10, QModelIndex(), 2);

    view->show();
    app.setMainWidget(view);

    return app.exec();
}
