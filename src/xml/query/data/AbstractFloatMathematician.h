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

#ifndef Patternist_AbstractFloatMathematician_H
#define Patternist_AbstractFloatMathematician_H

#include "AbstractFloat.h"
#include "AtomicMathematician.h"
#include "Integer.h"
#include "Numeric.h"
#include "PatternistLocale.h"
#include "SourceLocationReflection.h"

/**
 * @file
 * @short Contains classes performing arithemetic operations between atomic values, such as
 * substracting two dates.
 */

QT_BEGIN_HEADER 

namespace Patternist
{
    /**
     * @short Performs arithmetics between AbstractFloat values (Float and Double classes).
     *
     * @ingroup Patternist_xdm
     * @author Vincent Ricard <magic@magicninja.org>
     */
    template <const bool isDouble>
    class AbstractFloatMathematician : public AtomicMathematician
                                     , public DelegatingSourceLocationReflection
    {
    public:
        inline AbstractFloatMathematician(const SourceLocationReflection *const r) : DelegatingSourceLocationReflection(r)
        {
        }

        virtual Item calculate(const Item &o1,
                                    const Operator op,
                                    const Item &o2,
                                    const PlainSharedPtr<DynamicContext> &context) const;
    };

#include "AbstractFloatMathematician.cpp"

    /**
     * An instantiation of AbstractFloatMathematician that handles @c xs:double.
     */
    typedef AbstractFloatMathematician<true> DoubleMathematician;

    /**
     * An instantiation of AbstractFloatMathematician that handles @c xs:float.
     */
    typedef AbstractFloatMathematician<false> FloatMathematician;
}

QT_END_HEADER 

#endif
// vim: et:ts=4:sw=4:sts=4