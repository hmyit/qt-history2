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

#ifndef Patternist_GYear_H
#define Patternist_GYear_H

#include "qabstractdatetime_p.h"

QT_BEGIN_HEADER 

QT_BEGIN_NAMESPACE

namespace Patternist
{
    /**
     * @short Implements the value instance of the @c xs:gYear type.
     *
     * @author Frans Englich <fenglich@trolltech.com>
     * @ingroup Patternist_xdm
     */
    class GYear : public AbstractDateTime
    {
    public:
        typedef AtomicValue::Ptr Ptr;

        /**
         * Creates an instance from the lexical representation @p string.
         */
        static GYear::Ptr fromLexical(const QString &string);
        static GYear::Ptr fromDateTime(const QDateTime &dt);

        virtual ItemType::Ptr type() const;
        virtual QString stringValue() const;

    protected:
        friend class CommonValues;

        GYear(const QDateTime &dateTime);
    };
}

QT_END_NAMESPACE

QT_END_HEADER 

#endif
// vim: et:ts=4:sw=4:sts=4
