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

#ifndef Patternist_DeepEqualFN_H
#define Patternist_DeepEqualFN_H

#include "qatomiccomparator_p.h"
#include "qcomparisonplatform_p.h"
#include "qfunctioncall_p.h"

QT_BEGIN_HEADER 

QT_BEGIN_NAMESPACE

namespace Patternist
{
    /**
     * @short Implements the function <tt>fn:deep-equal()</tt>.
     *
     * @ingroup Patternist_functions
     * @author Frans Englich <fenglich@trolltech.com>
     */
    class DeepEqualFN : public FunctionCall,
                        public ComparisonPlatform<DeepEqualFN, false>
    {
    public:
        inline DeepEqualFN() : ComparisonPlatform<DeepEqualFN, false>()
        {
        }

        virtual bool evaluateEBV(const DynamicContext::Ptr &context) const;
        virtual Expression::Ptr typeCheck(const StaticContext::Ptr &context,
                                          const SequenceType::Ptr &reqType);

        inline AtomicComparator::Operator operatorID() const
        {
            return AtomicComparator::OperatorEqual;
        }
    };
}

QT_END_NAMESPACE

QT_END_HEADER 

#endif
// vim: et:ts=4:sw=4:sts=4
