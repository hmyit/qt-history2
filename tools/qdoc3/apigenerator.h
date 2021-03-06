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

#ifndef APIGENERATOR_H
#define APIGENERATOR_H

#include <QTextStream>

#include "generator.h"

QT_BEGIN_NAMESPACE

class ApiGenerator : public Generator
{
public:
    QString format();
    void generateTree(const Tree *tree, CodeMarker *marker);

private:
    void generateNode(const Node *node, CodeMarker *marker, int indent = 0);

    QTextStream out;
};

QT_END_NAMESPACE

#endif
