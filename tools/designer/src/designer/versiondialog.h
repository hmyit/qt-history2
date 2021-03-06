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

#ifndef VERSIONDIALOG_H
#define VERSIONDIALOG_H

#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE

class VersionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit VersionDialog(QWidget *parent);
};

QT_END_NAMESPACE

#endif
