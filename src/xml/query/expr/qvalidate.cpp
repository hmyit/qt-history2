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

#include "qbuiltintypes_p.h"
#include "qgenericsequencetype_p.h"
#include "qmultiitemtype_p.h"
#include "qtypechecker_p.h"

#include "qvalidate_p.h"

QT_BEGIN_NAMESPACE

using namespace Patternist;

Expression::Ptr Validate::create(const Expression::Ptr &operandNode,
                                 const Mode validationMode,
                                 const StaticContext::Ptr &context)
{
    Q_ASSERT(operandNode);
    Q_ASSERT(validationMode == Lax || validationMode == Strict);
    Q_ASSERT(context);

    static SequenceType::Ptr elementOrDocument; // STATIC DATA

    if(elementOrDocument)
    {
        return TypeChecker::applyFunctionConversion(operandNode,
                                                    elementOrDocument,
                                                    context,
                                                    ReportContext::XQTY0030);
    }
    else
    {
        ItemType::List tList;
        tList.append(BuiltinTypes::element);
        tList.append(BuiltinTypes::document);
        elementOrDocument = makeGenericSequenceType(ItemType::Ptr(new MultiItemType(tList)),
                                                    Cardinality::exactlyOne());

        return create(operandNode, validationMode, context);
    }
}

// vim: et:ts=4:sw=4:sts=4

QT_END_NAMESPACE
