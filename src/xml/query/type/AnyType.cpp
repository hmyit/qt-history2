/****************************************************************************
**
** Copyright (C) 2006-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the Patternist project on Trolltech Labs.
**
** $TROLLTECH_GPL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
***************************************************************************
*/

#include "AtomicType.h"

#include "AnyType.h"

using namespace Patternist;

AnyType::AnyType()
{
}

AnyType::~AnyType()
{
}

bool AnyType::wxsTypeMatches(const SchemaType::Ptr &other) const
{
    if(other)
        return this == other.get() ? true : wxsTypeMatches(other->wxsSuperType());
    else
        return false;
}

bool AnyType::isAbstract() const
{
    return false;
}

QName AnyType::name(const NamePool::Ptr &np) const
{
    return np->allocateQName(StandardNamespaces::xs, QLatin1String("anyType"));
}

QString AnyType::displayName(const NamePool::Ptr &) const
{
    /* A bit faster than calling name()->displayName() */
    return QLatin1String("xs:anyType");
}

SchemaType::Ptr AnyType::wxsSuperType() const
{
    return SchemaType::Ptr();
}

SchemaType::TypeCategory AnyType::category() const
{
    return None;
}

SchemaType::DerivationMethod AnyType::derivationMethod() const
{
    return NoDerivation;
}

// vim: et:ts=4:sw=4:sts=4