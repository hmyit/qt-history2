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

#include "dirmodel.h"

DirModel::DirModel(QObject *parent)
    : QDirModel(parent)
{
}

QVariant DirModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole && index.column() == 0) {
        QString path  = QDir::toNativeSeparators(filePath(index));
        if (path.endsWith(QDir::separator()))
            path.chop(1);
        return path;
    }

    return QDirModel::data(index, role);
}
