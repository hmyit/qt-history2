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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef SCRIPT_H
#define SCRIPT_H

#include <QtDesigner/sdk_global.h>
#include <QtDesigner/extension.h>
#include <QtCore/QVariant>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QString; // FIXME: fool syncqt

class QDESIGNER_SDK_EXPORT QDesignerScriptExtension
{
public:
    virtual ~QDesignerScriptExtension();

    virtual QVariantMap data() const = 0;
    virtual void setData(const QVariantMap &data) = 0;

    virtual QString script() const = 0;

};
Q_DECLARE_EXTENSION_INTERFACE(QDesignerScriptExtension, "com.trolltech.Qt.Designer.Script")

QT_END_NAMESPACE

QT_END_HEADER

#endif // SCRIPT_H
