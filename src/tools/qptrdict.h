/****************************************************************************
** $Id: //depot/qt/main/src/tools/qptrdict.h#1 $
**
** Definition of QPtrDict template/macro class
**
** Created : 940624
**
** Copyright (C) 1994-1996 by Troll Tech AS.  All rights reserved.
**
*****************************************************************************/

#ifndef QPTRDICT_H
#define QPTRDICT_H

#include "qgdict.h"


#if defined(USE_MACROCLASS)

#include "qgeneric.h"

#if !defined(name2)
#define name2(a,b)    name2_xx(a,b)
#define name2_xx(a,b) a##b
#endif

#if defined(DEFAULT_MACROCLASS)
#define QPtrDictdeclare QPtrDictMdeclare
#define QPtrDict QPtrDictM
#endif
#define QPtrDictM(type) name2(QPtrDictM_,type)

#define QPtrDictMdeclare(type)						      \
class QPtrDictM(type) : public QGDict					      \
{									      \
public:									      \
    QPtrDictM(type)(int size=17):QGDict(size,0,0,TRUE) {}		      \
    QPtrDictM(type)( const QPtrDictM(type) &d ) : QGDict(d) {}		      \
   ~QPtrDictM(type)()		{ clear(); }				      \
    QPtrDictM(type) &operator=(const QPtrDictM(type) &d)		      \
			{ return (QPtrDictM(type)&)QGDict::operator=(d); }    \
    uint  count()   const	{ return QGDict::count(); }		      \
    uint  size()    const	{ return QGDict::size(); }		      \
    bool  isEmpty() const	{ return QGDict::count() == 0; }	      \
    void  insert( void *k, const type *d )				      \
				{ QGDict::look((const char*)k,(GCI)d,1); }    \
    void  replace( void *k, const type *d )				      \
				{ QGDict::look((const char*)k,(GCI)d,2); }    \
    bool  remove( void *k )	{ return QGDict::remove((const char*)k); }    \
    type *take( void *k )	{ return (type*)QGDict::take((const char*)k);}\
    void  clear()		{ QGDict::clear(); }			      \
    type *find( void *k )	const					      \
	{ return (type *)((QGDict*)this)->QGDict::look((const char*)k,0,0);}  \
    type *operator[]( void *k ) const					      \
	{ return (type *)((QGDict*)this)->QGDict::look((const char*)k,0,0);}  \
    void  statistics() const	{ QGDict::statistics(); }		      \
private:								      \
    void  deleteItem( GCI d )	{ if ( del_item ) delete (type *)d; }	      \
}


#if defined(DEFAULT_MACROCLASS)
#define QPtrDictIteratordeclare QPtrDictIteratorMdeclare
#define QPtrDictIterator QPtrDictIteratorM
#endif
#define QPtrDictIteratorM(type) name2(QPtrDictIteratorM_,type)

#define QPtrDictIteratorMdeclare(type)					      \
class QPtrDictIteratorM(type) : public QGDictIterator			      \
{									      \
public:									      \
    QPtrDictIteratorM(type)(const QPtrDictM(type) &d) :			      \
	QGDictIterator((QGDict &)d){}					      \
   ~QPtrDictIteratorM(type)()	 {}					      \
    uint  count()   const     { return dict->count(); }			      \
    bool  isEmpty() const     { return dict->count() == 0; }		      \
    type *toFirst()	      { return (type *)QGDictIterator::toFirst(); }   \
    operator type *()  const  { return (type *)QGDictIterator::get(); }	      \
    type *current()    const  { return (type *)QGDictIterator::get(); }	      \
    void *currentKey() const  { return (void *)QGDictIterator::getKey(); }    \
    type *operator()()	      { return (type *)QGDictIterator::operator()();} \
    type *operator++()	      { return (type *)QGDictIterator::operator++(); }\
    type *operator+=(uint j)  { return (type *)QGDictIterator::operator+=(j);}\
}

#endif // USE_MACROCLASS


#if defined(USE_TEMPLATECLASS)

#if defined(DEFAULT_TEMPLATECLASS)
#undef	QPtrDict
#define QPtrDict QPtrDictT
#endif

template<class type> class QPtrDictT : public QGDict
{
public:
    QPtrDictT(int size=17) : QGDict(size,0,0,TRUE) {}
    QPtrDictT( const QPtrDictT<type> &d ) : QGDict(d) {}
   ~QPtrDictT()			{ clear(); }
    QPtrDictT<type> &operator=(const QPtrDictT<type> &d)
			{ return (QPtrDictT<type>&)QGDict::operator=(d); }
    uint  count()   const	{ return QGDict::count(); }
    uint  size()    const	{ return QGDict::size(); }
    bool  isEmpty() const	{ return QGDict::count() == 0; }
    void  insert( void *k, const type *d )
				{ QGDict::look((const char*)k,(GCI)d,1); }
    void  replace( void *k, const type *d )
				{ QGDict::look((const char*)k,(GCI)d,2); }
    bool  remove( void *k )	{ return QGDict::remove((const char*)k); }
    type *take( void *k )	{ return (type*)QGDict::take((const char*)k); }
    void  clear()		{ QGDict::clear(); }
    type *find( void *k )	const
	{ return (type *)((QGDict*)this)->QGDict::look((const char*)k,0,0); }
    type *operator[]( void *k ) const
	{ return (type *)((QGDict*)this)->QGDict::look((const char*)k,0,0); }
    void  statistics() const	{ QGDict::statistics(); }
private:
    void  deleteItem( GCI d )	{ if ( del_item ) delete (type *)d; }
};


#if defined(DEFAULT_TEMPLATECLASS)
#undef	QPtrDictIterator
#define QPtrDictIterator QPtrDictIteratorT
#endif

template<class type> class QPtrDictIteratorT : public QGDictIterator
{
public:
    QPtrDictIteratorT(const QPtrDictT<type> &d) :QGDictIterator((QGDict &)d) {}
   ~QPtrDictIteratorT()	      {}
    uint  count()   const     { return dict->count(); }
    bool  isEmpty() const     { return dict->count() == 0; }
    type *toFirst()	      { return (type *)QGDictIterator::toFirst(); }
    operator type *()  const  { return (type *)QGDictIterator::get(); }
    type *current()    const  { return (type *)QGDictIterator::get(); }
    void *currentKey() const  { return (void *)QGDictIterator::getKey(); }
    type *operator()()	      { return (type *)QGDictIterator::operator()();}
    type *operator++()	      { return (type *)QGDictIterator::operator++(); }
    type *operator+=(uint j)  { return (type *)QGDictIterator::operator+=(j);}
};

#endif // USE_TEMPLATECLASS


#endif // QPTRDICT_H
