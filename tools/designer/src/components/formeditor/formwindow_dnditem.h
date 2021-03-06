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

#ifndef FORMWINDOW_DNDITEM_H
#define FORMWINDOW_DNDITEM_H

#include <qdesigner_dnditem_p.h>

QT_BEGIN_NAMESPACE

namespace qdesigner_internal {

class FormWindow;

class FormWindowDnDItem : public QDesignerDnDItem
{
public:
    FormWindowDnDItem(QDesignerDnDItemInterface::DropType type, FormWindow *form,
                        QWidget *widget, const QPoint &global_mouse_pos);
    virtual DomUI *domUi() const;
};

}  // namespace qdesigner_internal

QT_END_NAMESPACE

#endif // FORMWINDOW_DNDITEM_H
