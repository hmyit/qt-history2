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

#include "abstractpropertyeditor.h"

AbstractPropertyEditor::AbstractPropertyEditor(QWidget *parent, Qt::WFlags flags)
    : QWidget(parent, flags)
{
}

AbstractPropertyEditor::~AbstractPropertyEditor()
{
}

AbstractFormEditor *AbstractPropertyEditor::core() const
{
    return 0;
}
