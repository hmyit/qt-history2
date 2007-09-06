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

#include "FunctionSignature.h"

#include "FunctionFactory.h"

using namespace Patternist;

FunctionFactory::~FunctionFactory()
{
}

bool FunctionFactory::isAvailable(const NamePool::Ptr &np,
                                  const QName name, const xsInteger arity)
{
    const FunctionSignature::Ptr sign(retrieveFunctionSignature(np, name));

    if(sign)
        return arity == FunctionSignature::UnlimitedArity || sign->isArityValid(arity);
    else
        return false;
}

bool FunctionFactory::hasSignature(const FunctionSignature::Ptr &signature) const
{
    const FunctionSignature::Hash signs(functionSignatures());
    const FunctionSignature::Hash::const_iterator end(signs.constEnd());
    FunctionSignature::Hash::const_iterator it(signs.constBegin());

    for(; it != end; ++it)
    {
        if(*(*it) == *signature)
            return true;
    }

    return false;
}
// vim: et:ts=4:sw=4:sts=4