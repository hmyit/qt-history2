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

#ifndef VALIDATOR_H
#define VALIDATOR_H

#include "treewalker.h"

class QTextStream;
class Driver;
class Uic;

struct Option;

struct Validator : public TreeWalker
{
    Validator(Uic *uic);

    void accept(DomUI *node);
    void accept(DomWidget *node);

    void accept(DomLayoutItem *node);
    void accept(DomLayout *node);

    void accept(DomActionGroup *node);
    void accept(DomAction *node);

private:
    Driver *driver;
    QTextStream &output;
    const Option &option;
    Uic *uic;
};

#endif // VALIDATOR_H
