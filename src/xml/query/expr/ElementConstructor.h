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

#ifndef Patternist_ElementConstructor_H
#define Patternist_ElementConstructor_H

#include <QUrl>

#include "PairContainer.h"

QT_BEGIN_HEADER 

namespace Patternist
{
    /**
     * @short Constructs an element node. This covers both computed and directly constructed
     * element nodes.
     *
     * @see <a href="http://www.w3.org/TR/xquery/#id-constructors">XQuery
     * 1.0: An XML Query Language, 3.7 Constructors</a>
     * @author Frans Englich <fenglich@trolltech.com>
     * @ingroup Patternist_expressions
     */
    class ElementConstructor : public PairContainer
    {
    public:
        ElementConstructor(const Expression::Ptr &operand1,
                           const Expression::Ptr &operand2);

        virtual Item evaluateSingleton(const DynamicContext::Ptr &context) const;
        virtual void evaluateToSequenceReceiver(const DynamicContext::Ptr &context) const;

        virtual SequenceType::Ptr staticType() const;

        /**
         * The first operand must be exactly one @c xs:QName, and the second
         * argument can be zero or more items.
         */
        virtual SequenceType::List expectedOperandTypes() const;

        virtual ExpressionVisitorResult::Ptr accept(const ExpressionVisitor::Ptr &visitor) const;
        virtual Expression::Ptr typeCheck(const StaticContext::Ptr &context,
                                          const SequenceType::Ptr &reqType);
        virtual Properties properties() const;

    private:
        QUrl m_staticBaseURI;
    };
}

QT_END_HEADER 

#endif
// vim: et:ts=4:sw=4:sts=4