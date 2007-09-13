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

#ifndef Patternist_SequenceMappingIterator_H
#define Patternist_SequenceMappingIterator_H

#include "qiterator_p.h"
#include "qdynamiccontext_p.h"
#include "qdebug_p.h"

QT_BEGIN_HEADER 

QT_BEGIN_NAMESPACE

namespace Patternist
{
    /**
     * @short Proxies another Iterator, and for each item, returns the
     * Sequence returned from a mapping function.
     *
     * ItemMappingIterator is practical when the items in an Iterator needs to
     * be translated to another sequence, while still doing it in a pipe-lined
     * fashion. In contrast to ItemMappingIterator, SequenceMappingIterator maps
     * each item into another Iterator, and where the SequenceMappingIterator's own
     * result is the concatenation of all those Iterators. Hence, while ItemMappingIterator
     * is better tailored for one-to-one or one-to-zero conversion, SequenceMappingIterator
     * is more suitable for one-to-many conversion.
     *
     * This is achieved by that SequenceMappingIterator's constructor takes
     * an instance of a class, that must have the following member:
     *
     * @code
     * Iterator<TResult>::Ptr mapToSequence(const TSource::Ptr &item,
     *                                      const DynamicContext::Ptr &context) const
     * @endcode
     *
     * @author Frans Englich <fenglich@trolltech.com>
     * @see ItemMappingIterator
     * @ingroup Patternist_iterators
     */
    template<typename TResult, typename TSource, typename TMapper>
    class SequenceMappingIterator : public Iterator<TResult>
    {
    public:
        /**
         * Constructs a SequenceMappingIterator.
         *
         * @param mapper the object that has the mapToItem() sequence.
         * @param sourceIterator the Iterator whose items should be mapped.
         * @param context the DynamicContext that will be passed to the map function.
         * May be null.
         */
        SequenceMappingIterator(const TMapper &mapper,
                                const typename Iterator<TSource>::Ptr &sourceIterator,
                                const DynamicContext::Ptr &context);

        virtual TResult next();
        virtual xsInteger count();
        virtual TResult current() const;
        virtual xsInteger position() const;
        virtual typename Iterator<TResult>::Ptr copy() const;

    private:
        xsInteger m_position;
        TResult m_current;
        typename Iterator<TSource>::Ptr m_mainIterator;
        typename Iterator<TResult>::Ptr m_currentIterator;
        const typename DynamicContext::Ptr m_context;
        const TMapper m_mapper;
    };

    template<typename TResult, typename TSource, typename TMapper>
    SequenceMappingIterator<TResult, TSource, TMapper>::SequenceMappingIterator(
                                        const TMapper &mapper,
                                        const typename Iterator<TSource>::Ptr &iterator,
                                        const DynamicContext::Ptr &context)
                                        : m_position(0),
                                          m_mainIterator(iterator),
                                          m_context(context),
                                          m_mapper(mapper)
    {
        qDebug() << Q_FUNC_INFO << endl;
        Q_ASSERT(mapper);
        Q_ASSERT(iterator);
    }

    template<typename TResult, typename TSource, typename TMapper>
    TResult SequenceMappingIterator<TResult, TSource, TMapper>::next()
    {
        /* This was once implemented with a recursive function, but the stack
         * got blown for some inputs by that approach. */
        while(true)
        {
            while(!m_currentIterator)
            {
                const TSource mainItem(m_mainIterator->next());

                if(isIteratorEnd(mainItem)) /* We've reached the very end. */
                {
                    m_position = -1;
                    m_current = TResult();
                    return TResult();
                }
                else
                    m_currentIterator = m_mapper->mapToSequence(mainItem, m_context);
            }

            m_current = m_currentIterator->next();

            if(isIteratorEnd(m_current))
            {
                m_currentIterator.reset();
                continue;
            }
            else
            {
                ++m_position;
                return m_current;
            }
        }
    }

    template<typename TResult, typename TSource, typename TMapper>
    xsInteger SequenceMappingIterator<TResult, TSource, TMapper>::count()
    {
        TSource unit(m_mainIterator->next());
        xsInteger c = 0;

        while(!isIteratorEnd(unit))
        {
            const typename Iterator<TResult>::Ptr sit(m_mapper->mapToSequence(unit, m_context));
            c += sit->count();
            unit = m_mainIterator->next();
        }

        return c;
    }

    template<typename TResult, typename TSource, typename TMapper>
    TResult SequenceMappingIterator<TResult, TSource, TMapper>::current() const
    {
        return m_current;
    }

    template<typename TResult, typename TSource, typename TMapper>
    xsInteger SequenceMappingIterator<TResult, TSource, TMapper>::position() const
    {
        return m_position;
    }

    template<typename TResult, typename TSource, typename TMapper>
    typename Iterator<TResult>::Ptr
    SequenceMappingIterator<TResult, TSource, TMapper>::copy() const
    {
        qDebug() << Q_FUNC_INFO << endl;
        qDebug() << endl;
        return typename Iterator<TResult>::Ptr
            (new SequenceMappingIterator<TResult, TSource, TMapper>(m_mapper,
                                                                     m_mainIterator->copy(),
                                                                     m_context));
    }

    /**
     * @short An object generator for SequenceMappingIterator.
     *
     * makeSequenceMappingIterator() is a convenience function for avoiding specifying
     * the full template instantiation for SequenceMappingIterator. Conceptually, it
     * is identical to Qt's qMakePair().
     *
     * @returns a SequenceMappingIterator wrapped in a smart pointer, that has been
     * passed the constructor arguments @p mapper, @p source, and @p context.
     * @see makeMappingCallbackPtr()
     * @relates Iterator
     */
    template<typename TResult, typename TSource, typename TMapper>
    static inline
    typename Iterator<TResult>::Ptr
    makeSequenceMappingIterator(const TMapper &mapper,
                                const PlainSharedPtr<Iterator<TSource> > &source,
                                const DynamicContext::Ptr &context)
    {
        return typename Iterator<TResult>::Ptr
            (new SequenceMappingIterator<TResult, TSource, TMapper>(mapper, source, context));
    }
}

QT_END_NAMESPACE

QT_END_HEADER 

#endif
// vim: et:ts=4:sw=4:sts=4
