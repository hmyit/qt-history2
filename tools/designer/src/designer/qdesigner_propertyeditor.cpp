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

#include "qdesigner.h"
#include "qdesigner_propertyeditor.h"
#include "qdesigner_workbench.h"

#include <propertyeditor/propertyeditor.h>

#include <abstractformeditor.h>

QDesignerPropertyEditor::QDesignerPropertyEditor(QDesignerWorkbench *workbench)
    : QDesignerToolWindow(workbench)
{
    setObjectName(QLatin1String("PropertyEditor"));
    PropertyEditor *widget = new PropertyEditor(workbench->core(), this);
    workbench->core()->setPropertyEditor(widget);

    setCentralWidget(widget);

    setWindowTitle(tr("Property Editor"));
}

QDesignerPropertyEditor::~QDesignerPropertyEditor()
{
}

QRect QDesignerPropertyEditor::geometryHint() const
{
    QRect g = workbench()->availableGeometry();
    int margin = workbench()->marginHint();
    int spacing = 40;

    QSize sz(g.width() * 1/4, g.height() * 4/6);

    return QRect((g.width() - sz.width() - margin), (margin + g.height() * 1/6) + spacing,
                  sz.width(), sz.height());
}
