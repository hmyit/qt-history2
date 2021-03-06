/****************************************************************************
**
** Copyright (C) 2001-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QApplication>
#include "hello.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    MyPushButton helloButton("Hello world!");
    helloButton.resize(100, 30);

    helloButton.show();
    return app.exec();
}
