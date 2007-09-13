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

#include "qcommonsequencetypes_p.h"
#include "qoptimizerblocks_p.h"

#include "qoptimizationpasses_p.h"

QT_BEGIN_NAMESPACE

using namespace Patternist;

OptimizationPass::List OptimizationPasses::comparisonPasses;
OptimizationPass::List OptimizationPasses::forPasses;
OptimizationPass::List OptimizationPasses::ifThenPasses;
OptimizationPass::List OptimizationPasses::notFN;

void OptimizationPasses::Coordinator::init()
{
    static bool isInitialized = false; // STATIC DATA

    if(isInitialized)
        return;

    isInitialized = true;

    /* Note, below is many of the building blocks re-used in several passes
     * in order to reduce memory use. Thus, when changing one building block
     * it potentially affects many passes. */

    /* ****************************************************** */
    /* Rewrite "count(<expr>) ge 1" into "exists(<expr>)" */
    OptimizationPass::ExpressionMarker firstFirstChild;
    firstFirstChild.append(0);
    firstFirstChild.append(0);

    ExpressionIdentifier::List geOpIDs;
    const ExpressionIdentifier::Ptr countFN(new ByIDIdentifier(Expression::IDCountFN));
    geOpIDs.append(countFN);
    geOpIDs.append(ExpressionIdentifier::Ptr(new IntegerIdentifier(1)));

    const ExpressionIdentifier::Ptr ge(new ComparisonIdentifier(Expression::IDValueComparison |
                                                                Expression::IDGeneralComparison,
                                                                AtomicComparator::OperatorGreaterOrEqual));

    const ExpressionCreator::Ptr existsFN(new ByIDCreator(Expression::IDExistsFN));
    const OptimizationPass::Ptr geToExists(new OptimizationPass(ge, geOpIDs, firstFirstChild, existsFN));
    comparisonPasses.append(geToExists);
    /* ****************************************************** */

    /* ****************************************************** */
    /* Rewrite "count(<expr>) gt 0" into "exists(<expr>)" */
    ExpressionIdentifier::List countAndIntZero;
    countAndIntZero.append(countFN);
    const ExpressionIdentifier::Ptr zeroInteger(new IntegerIdentifier(0));
    countAndIntZero.append(zeroInteger);

    const ExpressionIdentifier::Ptr gt(new ComparisonIdentifier(Expression::IDValueComparison |
                                                                Expression::IDGeneralComparison,
                                                                AtomicComparator::OperatorGreaterThan));

    const OptimizationPass::Ptr gtToExists(new OptimizationPass(gt, countAndIntZero,
                                                                firstFirstChild, existsFN));
    comparisonPasses.append(gtToExists);
    /* ****************************************************** */

    /* ****************************************************** */
    /* Rewrite "count(<expr>) ne 0" into "exists(<expr>)" */

    const ExpressionIdentifier::Ptr ne(new ComparisonIdentifier(Expression::IDValueComparison |
                                                                Expression::IDGeneralComparison,
                                                                AtomicComparator::OperatorNotEqual));
    const OptimizationPass::Ptr neToExists(new OptimizationPass(ne, countAndIntZero, firstFirstChild,
                                                                existsFN,
                                                                OptimizationPass::AnyOrder));
    comparisonPasses.append(neToExists);
    /* ****************************************************** */

    /* ****************************************************** */
    /* Rewrite "count(<expr>) eq 0" into "empty(<expr>)" */
    ExpressionIdentifier::List eqOpIDs;
    eqOpIDs.append(countFN);
    eqOpIDs.append(zeroInteger);
    const ExpressionCreator::Ptr emptyFN(new ByIDCreator(Expression::IDEmptyFN));
    const ExpressionIdentifier::Ptr eq(new ComparisonIdentifier(Expression::IDValueComparison |
                                                                Expression::IDGeneralComparison,
                                                                AtomicComparator::OperatorEqual));
    const OptimizationPass::Ptr eqToEmpty(new OptimizationPass(eq, eqOpIDs, firstFirstChild,
                                                               emptyFN,
                                                               OptimizationPass::AnyOrder));
    comparisonPasses.append(eqToEmpty);

    /* ****************************************************** */

    /* ****************************************************** */
    /* Rewrite "for $var in <expr> return $var" into "<expr>" */
    ExpressionIdentifier::List forOps;
    OptimizationPass::ExpressionMarker firstChild;
    firstChild.append(0);

    forOps.append(ExpressionIdentifier::Ptr());
    forOps.append(ExpressionIdentifier::Ptr(new ByIDIdentifier(Expression::IDRangeVariableReference)));
    const OptimizationPass::Ptr simplifyFor(new OptimizationPass(ExpressionIdentifier::Ptr(), forOps,
                                                                 firstChild, ExpressionCreator::Ptr()));
    forPasses.append(simplifyFor);
    /* ****************************************************** */

    /* ****************************************************** */
    /* Rewrite "if(<expr>) then true() else false()" to "<expr>" */
    OptimizationPass::ExpressionMarker marker;
    marker.append(0);

    ExpressionIdentifier::List opIDs;
    opIDs.append(ExpressionIdentifier::Ptr(new BySequenceTypeIdentifier(
                        CommonSequenceTypes::ExactlyOneBoolean)));
    opIDs.append(ExpressionIdentifier::Ptr(new BooleanIdentifier(true)));
    opIDs.append(ExpressionIdentifier::Ptr(new BooleanIdentifier(false)));

    const OptimizationPass::Ptr pass(new OptimizationPass(ExpressionIdentifier::Ptr(), opIDs, marker));
    ifThenPasses.append(pass);
    /* ****************************************************** */

    /* ****************************************************** */
    /* Rewrite "not(exists(X))" into "empty(X)" */
    ExpressionIdentifier::List idExistsFN;
    idExistsFN.append(ExpressionIdentifier::Ptr(new ByIDIdentifier(Expression::IDExistsFN)));

    notFN.append(OptimizationPass::Ptr(new OptimizationPass(ExpressionIdentifier::Ptr(),
                                                            idExistsFN,
                                                            firstFirstChild,
                                                            emptyFN)));

    /* Rewrite "not(empty(X))" into "exists(X)" */
    ExpressionIdentifier::List idEmptyFN;
    idEmptyFN.append(ExpressionIdentifier::Ptr(new ByIDIdentifier(Expression::IDEmptyFN)));

    notFN.append(OptimizationPass::Ptr(new OptimizationPass(ExpressionIdentifier::Ptr(),
                                                            idEmptyFN,
                                                            firstFirstChild,
                                                            existsFN)));
    /* ****************************************************** */
}

// vim: et:ts=4:sw=4:sts=4

QT_END_NAMESPACE
