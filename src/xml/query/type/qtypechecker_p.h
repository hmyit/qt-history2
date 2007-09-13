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

#ifndef Patternist_TypeChecker_H
#define Patternist_TypeChecker_H

#include "qstaticcontext_p.h"
#include "qexpression_p.h"

QT_BEGIN_HEADER 

QT_BEGIN_NAMESPACE

namespace Patternist
{
    /**
     * @short Contains functions that applies Function Conversion Rules and other
     * kinds of compile-time type checking tasks.
     *
     * @ingroup Patternist_types
     * @author Frans Englich <fenglich@trolltech.com>
     */
    class TypeChecker
    {
    public:
        enum Option
        {
            /**
             * @short When set, the function conversion rules are applied.
             *
             * For instance, this is type promotion and conversions from @c
             * xs:untypedAtomic to @c xs:date. This is done for function calls,
             * but not when binding an expression to a variable.
             */
            AutomaticallyConvert = 1,

            /**
             * @short Whether the focus should be checked or not.
             *
             * Sometimes the focus is unknown at the time
             * applyFunctionConversion() is called, and therefore it is
             * of interest to post pone the check to later on.
             */
            CheckFocus = 2
        };
        typedef QFlags<Option> Options;

        /**
         * @short Builds a pipeline of artificial AST nodes that ensures @p operand
         * conforms to the type @p reqType by applying the Function
         * Conversion Rules.
         *
         * This new Expression is returned, or, if no conversions were necessary,
         * @p operand as it is.
         *
         * applyFunctionConversion() also performs various checks, such as if
         * @p operand needs the focus and that the focus is defined in the
         * @p context. These checks are largely guided by @p operand's
         * Expression::properties().
         *
         * @see <a href="http://www.w3.org/TR/xpath20/\#id-function-calls">XML Path
         * Language (XPath) 2.0, 3.1.5 Function Calls</a>, more specifically the
         * Function Conversion Rules
         */
        static Expression::Ptr
        applyFunctionConversion(const Expression::Ptr &operand,
                                const SequenceType::Ptr &reqType,
                                const StaticContext::Ptr &context,
                                const ReportContext::ErrorCode code = ReportContext::XPTY0004,
                                const Options = Options(AutomaticallyConvert | CheckFocus));
    private:

        static inline Expression::Ptr typeCheck(Expression *const op,
                                                const StaticContext::Ptr &context,
                                                const SequenceType::Ptr &reqType);
        /**
         * @short Implements the type checking and promotion part of the Function Conversion Rules.
         */
        static Expression::Ptr verifyType(const Expression::Ptr &operand,
                                          const SequenceType::Ptr &reqSeqType,
                                          const StaticContext::Ptr &context,
                                          const ReportContext::ErrorCode code,
                                          const Options options);

        /**
         * Determines whether type promotion is possible from one type to another. False
         * is returned when a promotion is not possible or if a promotion is not needed(as when
         * the types are identical), since that can be considered to not be type promotion.
         *
         * @returns @c true if @p fromType can be promoted to @p toType.
         * @see <a href="http://www.w3.org/TR/xpath20/#promotion">XML Path Language
         * (XPath) 2.0, B.1 Type Promotion</a>
         */
        static bool promotionPossible(const ItemType::Ptr &fromType,
                                      const ItemType::Ptr &toType,
                                      const StaticContext::Ptr &context);

        /**
         * @short Centralizes a message-string to reduce work for translators
         * and increase consistency.
         */
        static inline QString wrongType(const NamePool::Ptr &np,
                                        const ItemType::Ptr &reqType,
                                        const ItemType::Ptr &opType);

        /**
         * No implementation is provided for this constructor. This class
         * is not supposed to be instantiated.
         */
        inline TypeChecker();

        Q_DISABLE_COPY(TypeChecker)
    };
}

QT_END_NAMESPACE

QT_END_HEADER 

#endif
// vim: et:ts=4:sw=4:sts=4
