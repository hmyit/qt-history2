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

#include <QVariant>

#include "qatomicstring_p.h"
#include "qatomictype_p.h"
#include "qbuiltintypes_p.h"
#include "qdynamiccontext_p.h"
#include "qgenericsequencetype_p.h"
#include "qhexbinary_p.h"
#include "qlistiterator_p.h"
#include "qpatternistlocale_p.h"
#include "qqobjectnodemodel_p.h"
#include "qvalidationerror_p.h"

#include "qitem_p.h"

QT_BEGIN_NAMESPACE

/**
 * @file
 * @short Contains the implementation for AtomicValue. The defintion is in qitem_p.h.
 */

using namespace Patternist;

AtomicValue::~AtomicValue()
{
}

bool AtomicValue::evaluateEBV(const PlainSharedPtr<DynamicContext> &context) const
{
    context->error(tr("Evaluating the effective boolean value for a value of type %1 "
                      "isn't possible.").arg(formatType(context->namePool(), type())),
                      ReportContext::FORG0006,
                      QSourceLocation());
    return false; /* Silence GCC warning. */
}

bool AtomicValue::hasError() const
{
    return false;
}

QVariant AtomicValue::toQt(const AtomicValue *const value)
{
    Q_ASSERT_X(value, Q_FUNC_INFO,
               "Internal error, a null pointer cannot be passed.");

    const ItemType::Ptr t(value->type());

    if(BuiltinTypes::xsString->xdtTypeMatches(t))
        return value->stringValue();
    else
    {
        Q_ASSERT_X(false, Q_FUNC_INFO, "Internal error, an unknown AtomicValue was encountered.");
        return QVariant();
    }
}

Item AtomicValue::toXDM(const QVariant &value,
                        const QObjectNodeModel *const nm)
{
    Q_ASSERT_X(value.isValid(), Q_FUNC_INFO,
               "QVariants sent to Patternist must be valid.");

    switch(value.type())
    {
        case QVariant::Url:
        /* Fallthrough. QUrl doesn't follow the spec properly, so we
         * have to let it be an xs:string. */
        case QVariant::String:
        case QVariant::Char:
            return AtomicString::fromValue(value.toString());
        case QVariant::ByteArray:
            return HexBinary::fromValue(value.toByteArray());
        case QMetaType::QObjectStar:
            return nm->createNode(qVariantValue<QObject *>(value));
        default:
        {
            Q_ASSERT_X(false, Q_FUNC_INFO,
                       qPrintable(QString::fromLatin1("QVariants of type %1 are not supported in Patternist, see the documentation").arg(QLatin1String(value.typeName()))));
            return AtomicValue::Ptr();
        }
    }
}

// vim: et:ts=4:sw=4:sts=4

QT_END_NAMESPACE
