/****************************************************************************
** $Id: $
**
** Implementation of the QAxBase class
**
** Copyright (C) 2001-2002 Trolltech AS.  All rights reserved.
**
** This file is part of the Active Qt integration.
**
** Licensees holding valid Qt Enterprise Edition
** licenses for Windows may use this file in accordance with the Qt Commercial
** License Agreement provided with the Software.
**
** This file is not available for use under any other license without
** express written permission from the copyright holder.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

//#define QAX_NO_CLASSINFO

#include "qaxobject.h"

#include <quuid.h>
#include <qdict.h>
#include <qptrdict.h>
#include <qsettings.h>
#include <qmetaobject.h>
#include <qcache.h>

#include <ctype.h>

#include <atlbase.h>
CComModule _Module;
int moduleLockCount = 0;
void moduleLock()
{
    if ( !moduleLockCount ) {
	CoInitialize(0);
	_Module.Init( 0, GetModuleHandle(0) );	
    }
    ++moduleLockCount;
}
void moduleUnlock()
{
    if ( !moduleLockCount ) {
#ifndef QT_NO_DEBUG
	qWarning( "Unbalanced module count!" );
#endif
	return;
    }
    if ( !--moduleLockCount ) {
	_Module.Term();
	CoUninitialize();
    }
}

static QMetaObject *tempMetaObj = 0;

#define PropDesignable  0x00001000
#define PropScriptable	0x00002000
#define PropStored	0x00004000
#define PropBindable	0x00008000
#define PropRequesting	0x00010000



#include "../shared/types.h"

static QMetaObjectCleanUp cleanUp_QAxBase;


/*
    \internal
    \class QAxEventSink qaxbase.cpp

    \brief The QAxEventSink class implements the event sink for all
	   IConnectionPoints implemented in the COM object.
*/

class QAxEventSink : public IDispatch,
		     public IPropertyNotifySink
{
public:
    QAxEventSink( QAxBase *com ) : combase( com ), ref( 1 ) {}
    virtual ~QAxEventSink() 
    {
	Q_ASSERT( !connection.cpoint );
    }

    // add a connection
    void advise( IConnectionPoint *cpoint, IID iid ) 
    {
	connection = Connection( cpoint, iid, 0 );
	cpoint->Advise( (IUnknown*)(IDispatch*)this, &connection.cookie );
    }
    
    // disconnect from all connection points
    void unadvise()
    {
	if ( connection.cpoint ) {
	    connection.cpoint->Unadvise( connection.cookie );
	    connection.cpoint->Release();
	    connection.cpoint = 0;
	}
    }

    void addSignal( DISPID memid, const QString &name )
    {
	sigs.insert( memid, name );
	QMap<DISPID,QString>::Iterator it;
	DISPID id = -1;
	for ( it = propsigs.begin(); it!= propsigs.end(); ++it ) {
	    if ( it.data() == name ) {
		id = it.key();
		break;
	    }
	}
	if ( id != -1 )
	    propsigs.remove( id );
    }
    void addProperty( DISPID propid, const QString &name, const QString &signal )
    {
	props.insert( propid, name );
	propsigs.insert( propid, signal );
    }

    // IUnknown
    unsigned long __stdcall AddRef()
    {
	return ref++;
    }
    unsigned long __stdcall Release()
    {
	if ( !--ref ) {
	    delete this;
	    return 0;
	}
	return ref;
    }
    HRESULT __stdcall QueryInterface( REFIID riid, void **ppvObject )
    {
	*ppvObject = 0;
	if ( riid == IID_IUnknown)
	    *ppvObject = (IUnknown*)(IDispatch*)this;
	else if ( riid == IID_IDispatch )
	    *ppvObject = (IDispatch*)this;
	else if ( riid == IID_IPropertyNotifySink )
	    *ppvObject = (IPropertyNotifySink*)this;
	else if ( connection.iid == riid )
	    *ppvObject = (IDispatch*)this;
	else
	    return E_NOINTERFACE;

	AddRef();
	return S_OK;
    }

    // IDispatch
    HRESULT __stdcall GetTypeInfoCount( unsigned int *count ) { return E_NOTIMPL; }
    HRESULT __stdcall GetTypeInfo( UINT, LCID, ITypeInfo **info ) { return E_NOTIMPL; }
    HRESULT __stdcall GetIDsOfNames( const _GUID &, wchar_t **, unsigned int, unsigned long, long * ) { return E_NOTIMPL; }

    HRESULT __stdcall Invoke( DISPID dispIdMember, 
			      REFIID riid, 
			      LCID lcid, 
			      WORD wFlags, 
			      DISPPARAMS *pDispParams, 
			      VARIANT *pVarResult, 
			      EXCEPINFO *pExcepInfo, 
			      UINT *puArgErr )
    {
	// verify input
	if ( riid != IID_NULL )
	    return DISP_E_UNKNOWNINTERFACE;
	if ( wFlags != DISPATCH_METHOD )
	    return DISP_E_MEMBERNOTFOUND;

	QMetaObject *meta = combase->metaObject();
	QString signame = sigs[dispIdMember];
	if ( !meta || signame.isEmpty() )
	    return DISP_E_MEMBERNOTFOUND;

	QObject *qobject = combase->qObject();
	if ( qobject->signalsBlocked() )
	    return S_OK;

	// emit the signal "as is"
	int index = meta->findSignal( "signal(const QString&,int,void*)" );
	if ( index != -1 && ((QAxObject*)qobject)->receivers( index ) ) {
	    QUObject o[4];
	    static_QUType_QString.set(o+1,signame);
	    static_QUType_int.set(o+2,pDispParams->cArgs);
	    static_QUType_ptr.set(o+3,pDispParams->rgvarg);
	    combase->qt_emit( index, o );
	    static_QUType_ptr.clear(o+3);
	    static_QUType_int.clear(o+2);
	    static_QUType_QString.clear(o+1);
	}

	// get the signal information from the metaobject
	index = meta->findSignal( signame );
	if ( index != -1 && ((QAxObject*)qobject)->receivers( index ) ) {
	    const QMetaData *signal = meta->signal( index - meta->signalOffset() );
	    if ( !signal )
		return DISP_E_MEMBERNOTFOUND;

	    // verify parameter count
	    int pcount = signal->method->count;
	    int argcount = pDispParams->cArgs;
	    if ( pcount > argcount )
		return DISP_E_PARAMNOTOPTIONAL;
	    else if ( pcount < argcount )
		return DISP_E_BADPARAMCOUNT;

	    // setup parameters
	    const QUParameter *params = signal->method->parameters;
	    QUObject *objects = pcount ? new QUObject[pcount+1] : 0;
	    int p;
	    for ( p = 0; p < pcount; ++p ) // map the VARIANT to the QUObject
		VARIANTToQUObject( pDispParams->rgvarg[ pcount-p-1 ], objects + p + 1 );

	    // emit the generated signal
	    bool ret = combase->qt_emit( index, objects );

	    for ( p = 0; p < pcount; ++p ) { // update the VARIANT for references and free memory
		VARIANT *arg = &(pDispParams->rgvarg[ pcount-p-1 ]);
		VARTYPE vt = arg->vt;
		QUObject *obj = objects + p + 1;
		switch ( vt )
		{
		case VT_DATE:
		case VT_DATE|VT_BYREF:
		    {
			QDateTime *dt = (QDateTime*)static_QUType_ptr.get( obj );
			if ( vt & VT_BYREF )
			    *arg->pdate = QDateTimeToDATE( *dt );
			delete dt;
		    }
		    break;
		case VT_BSTR|VT_BYREF:
		    {
			BSTR *bstr = arg->pbstrVal;
			QString *str = (QString*)static_QUType_ptr.get( obj );
			*bstr = QStringToBSTR( *str );
			delete str;
		    }
		case VT_UNKNOWN|VT_BYREF:
		    {
			IUnknown *iface = (IUnknown*)static_QUType_ptr.get( obj );
			*arg->ppunkVal = iface;
		    }
		    break;

		case VT_DISPATCH|VT_BYREF:
		    {
			IDispatch *iface = (IDispatch*)static_QUType_ptr.get( obj );
			*arg->ppdispVal = iface;
		    }
		    break;

		default:
		    break;
		}
	    }
	    // cleanup
	    for ( p = 0; p < pcount; ++p ) {
		objects[p].type->clear(objects+p);
	    }
	    delete [] objects;
	    return ret ? S_OK : DISP_E_MEMBERNOTFOUND;
	} else {
	    return S_OK;
	}
    }

    // IPropertyNotifySink
    HRESULT __stdcall OnChanged( DISPID dispID )
    {
	// verify input
	if ( dispID == DISPID_UNKNOWN ) {
	    return S_OK;
	}

	QMetaObject *meta = combase->metaObject();
	QString propname = props[dispID];
	if ( !meta || propname.isEmpty() )
	    return S_OK;

	QObject *qobject = combase->qObject();
	if ( qobject->signalsBlocked() )
	    return S_OK;

	// emit the generic signal
	int index = meta->findSignal( "propertyChanged(const QString&)" );
	if ( index != -1 && ((QAxObject*)qobject)->receivers( index ) ) {
	    QUObject o[2];
	    static_QUType_QString.set(o+1,propname);
	    combase->qt_emit( index, o );
	    static_QUType_QString.clear(o+1);
	}

	QString signame = propsigs[dispID];
	if ( signame.isEmpty() )
	    return S_OK;
	// get the signal information from the metaobject
	index = meta->findSignal( signame );
	if ( index != -1 && ((QAxObject*)qobject)->receivers( index ) ) {
	    const QMetaData *signal = meta->signal( index - meta->signalOffset() );
	    if ( !signal || signal->method->count != 1 )
		return S_OK;

	    // setup parameters
	    int pindex = meta->findProperty( propname );
	    QUObject o[2];
	    QVariant var;
	    combase->qt_property( pindex + meta->propertyOffset(), 1, &var );
	    if ( !var.isValid() )
		return S_OK;
	    if ( QUType::isEqual( signal->method->parameters->type, &static_QUType_QVariant ) )
		static_QUType_QVariant.set( o+1, var );
	    else
		QVariantToQUObject( var, o[1] );

	    // emit the "changed" signal
	    combase->qt_emit( index, o );
	    o[1].type->clear(o+1);
	}
	return S_OK;
    }
    HRESULT __stdcall OnRequestEdit( DISPID dispID )
    {
	if ( dispID == DISPID_UNKNOWN ) {
	    return S_OK;
	}
	QString propname = props[dispID];
	return combase->propertyWritable( propname ) ? S_OK : S_FALSE;
    }

private:
    struct Connection
    {
	Connection() 
	    : cpoint( 0 ), iid( IID_NULL ), cookie( 0 )
	{
	}
	Connection( IConnectionPoint *point, IID i, ULONG c ) 
	    : cpoint( point ), iid(i), cookie(c) 
	{
	    cpoint->AddRef();
	}

	IConnectionPoint *cpoint;
	IID iid;
	ULONG cookie;

	bool operator==( const Connection &other ) const { return iid == other.iid; }
    };
    Connection connection;
    QMap<DISPID, QString> sigs;
    QMap<DISPID, QString> propsigs;
    QMap<DISPID, QString> props;

    QAxBase *combase;
    long ref;
};

class QAxMetaObject
{
public:
    QAxMetaObject()
	: metaObject( 0 ), enums( 0 ), numEnums( 0 )
    {
    }

    ~QAxMetaObject()
    {
	if ( !metaObject )
	    return;

	QMetaObject *metaobj = metaObject;

	int i;
	// clean up class info
	for ( i = 0; i < metaobj->numClassInfo(); ++i ) {
	    QClassInfo *info = (QClassInfo*)metaobj->classInfo( i );
	    delete [] (char*)info->name;
	    delete [] (char*)info->value;
	}
	if ( metaobj->numClassInfo() )
	    delete [] (QClassInfo*)metaobj->classInfo( 0 );

	// clean up slot info
	for ( i = 0; i < metaobj->numSlots(); ++i ) {
	    const QMetaData *slot_data = metaobj->slot( i );
	    QUMethod *slot = (QUMethod*)slot_data->method;
	    if ( slot ) {
		delete [] (char*)slot->name;
		for ( int p = 0; p < slot->count; ++p ) {
		    const QUParameter *param = &(slot->parameters[p]);
		    delete [] (char*)param->name;
		    if ( QUType::isEqual( param->type, &static_QUType_enum ) ) {
			// delete uEnum arrays, but not the strings (shared with the QMetaEnum)
			QUEnum *uEnum = (QUEnum*)param->typeExtra;
			delete [] (QUEnumItem*)uEnum->items;
			delete uEnum;
		    } else if ( QUType::isEqual( param->type, &static_QUType_ptr ) ) {
			const char *type = (const char*)param->typeExtra;
			delete [] (char*)type;
		    }
		}
		delete [] (QUParameter*)slot->parameters;
		delete slot;
	    }
	    delete [] (char*)slot_data->name;
	}
	if ( metaobj->numSlots() )
	    delete [] (QMetaData*)metaobj->slot( 0 );

	// clean up signal info
	for ( i = 2; i < metaobj->numSignals(); ++i ) { // 0 and 1 are static signals
	    const QMetaData *signal_data = metaobj->signal( i );
	    QUMethod *signal = (QUMethod*)signal_data->method;
	    if ( signal ) {
		delete [] (char*)signal->name;
		for ( int p = 0; p < signal->count; ++p ) {
		    const QUParameter *param = &(signal->parameters[p]);
		    delete [] (char*)param->name;
		    if ( QUType::isEqual( param->type, &static_QUType_enum ) ) {
			// delete uEnum arrays, but not the strings (shared with the QMetaEnum)
			QUEnum *uEnum = (QUEnum*)param->typeExtra;
			delete [] (QUEnumItem*)uEnum->items;
			delete uEnum;
		    } else if ( QUType::isEqual( param->type, &static_QUType_ptr ) ) {
			const char *type = (const char*)param->typeExtra;
			delete [] (char*)type;
		    }
		}
		delete [] (QUParameter*)signal->parameters;
		delete signal;
	    }
	    delete [] (char*)signal_data->name;
	}
	if ( metaobj->numSignals() )
	    delete [] (QMetaData*)metaobj->signal( 0 );

	// clean up properties
	for ( i = 0; i < metaobj->numProperties(); ++i ) {
	    const QMetaProperty *property = metaobj->property( i );
	    delete [] (char*)property->n;
	    delete [] (char*)property->t;
	}
	if ( metaobj->numProperties() )
	    delete [] (QMetaProperty*)metaobj->property( 0 );

	// clean up enums
	if ( enums ) {
	    for ( i = 0; i < numEnums; ++i ) {
		for ( uint j = 0; j < enums[i].count; ++j ) {
		    QMetaEnum::Item item = enums[i].items[j];
		    delete[] (char*)item.key;
		}
		delete [] (char*)enums[i].name;
		delete [] (QMetaEnum::Item*)enums[i].items;
	    }
	    delete [] (QMetaEnum*)enums;
	}
	delete metaobj;
	metaObject = 0;
    }

    QMetaObject *metaObject;

    QMetaEnum *enums;
    int numEnums;
};

static QCache<QAxMetaObject> *mo_cache = 0;
static int mo_cache_ref = 0;

static QCache<QAxMetaObject> *metaObjectCache()
{
    if ( !mo_cache ) {
	mo_cache = new QCache<QAxMetaObject>;
	mo_cache->setAutoDelete( TRUE );
	mo_cache->setMaxCost( 5 );
    }
    return mo_cache;
}

/*
    \internal
    \class QAxBasePrivate qaxbase.cpp
*/

class QAxBasePrivate
{
public:
    QAxBasePrivate()
	: useEventSink( TRUE ), useMetaObject( TRUE ), useClassInfo( TRUE ),
	  cachedMetaObject( FALSE ), ptr( 0 ), disp( 0 ), propWritable( 0 ),
	  metaobj( 0 )
    {
	mo_cache_ref++;
    }

    ~QAxBasePrivate()
    {
	Q_ASSERT( !ptr );
	Q_ASSERT( !disp );

	delete propWritable;
	propWritable = 0;

	if ( !--mo_cache_ref ) {
	    delete mo_cache;
	    mo_cache = 0;
	}
    }

    IDispatch *dispatch()
    {
	if ( disp )
	    return disp;

	if ( ptr )
	    ptr->QueryInterface( IID_IDispatch, (void**)&disp );
	return disp;
    }

    QDict<QAxEventSink> eventSink;
    bool useEventSink	    :1;
    bool useMetaObject	    :1;
    bool useClassInfo	    :1;
    bool cachedMetaObject   :1;

    IUnknown *ptr;
    IDispatch *disp;

    QMap<QCString, bool> *propWritable;
    QAxMetaObject *metaobj;
};


/*!
    \class QAxBase qaxbase.h

    \brief The QAxBase class is an abstract class that provides an API to initalize and access a COM object.

    \module QAxContainer
    \extension ActiveQt

    QAxBase is an abstract class that cannot be used directly, and is instantiated through the subclasses 
    QAxObject and QAxWidget. This class however provides the API to access the COM object directly through 
    its IUnknown implementation. If the COM object implements the IDispatch interface, the properties and
    methods of that object become available as Qt properties and slots.

    \code
    connect( buttonBack, SIGNAL(clicked()), webBrowser, SLOT(GoBack()) );
    \endcode

    Properties exposed by the object's IDispatch implementation can be read and written through the property
    system provided by the Qt Object Model (both subclasses are QObjects, so you can use 
    \link QObject::setProperty \endlink setProperty and \link QObject::property \endlink property as with
    QObject).

    \code
    activeX->setProperty( "text", "some text" );
    int value = activeX->property( "value" );
    \endcode

    Write-functions for properties and other methods exposed by the object's IDispatch implementation can be 
    called directly using dynamicCall(), or indirectly as slots connected to a signal.

    \code
    webBrowser->dynamicCall( "GoHome()" );
    \endcode

    Outgoing events supported by the COM object are emitted as standard Qt signals.

    \code
    connect( webBrowser, SIGNAL(TitleChanged(const QString&)), this, SLOT(setCaption(const QString&)) );
    \endcode

    QAxBase transparently converts between Qt data type and the equivalent COM data types. Some COM types,
    for example the VARIANT type VT_CY, have no equivalent Qt data structure.

    Supported OLE datatypes are
    \list
    \i char, unsigned char
    \i short, unsigned short
    \i int, unsigned int
    \i long, unsigned long
    \i float, double
    \i bool
    \i const char*
    \i BSTR
    \i DATE
    \i SCODE and 
    \i VARIANT
    \i OLE_COLOR
    \i IFont*
    \endlist

    Unsupported OLE datatypes are
    \list
    \i IUnknown*
    \i IDispatch* and
    \i CY 
    \endlist 

    Pointer types and references can additionally not be provided by the Qt property system, or 
    passed as parameters to dynamicCall().
   
    If you need to access properties or pass parameters of unsupported datatypes you have to access the COM 
    object directly through its IDispatch or other interfaces implemented. Those interface can be retrieved
    through queryInterface().

    \code
    IUnknown *iface = 0;
    activeX->queryInterface( IID_IUnknown, (void**)&iface );
    if ( iface ) {
        // use the interface
	iface->Release();
    }
    \endcode

    If you need to react on events that pass parameters of unsupported datatypes you can use the generic
    signal that delivers the event data as provided by the COM event.
*/

/*! 
    \enum QAxBase::PropertyBag

    A QMap<QString,QVariant> that can store name - value pairs of properties.
*/

/*!
    Creates a QAxBase object that wraps the COM object \a iface. If \a iface is null (the default),
    use setControl() to instantiate a COM object.
*/
QAxBase::QAxBase( IUnknown *iface )
{
    d = new QAxBasePrivate();
    d->ptr = iface;

    if ( d->ptr ) {
	moduleLock();
	d->ptr->AddRef();
    }
}

/*!
    Shuts down the COM object and destroys the QAxBase object.

    \sa clear()
*/
QAxBase::~QAxBase()
{
    clear();

    delete d;
    d = 0;
}

/*!
    \property QAxBase::control
    \brief the name of the COM object wrapped by this QAxBase object.

    Setting this property initilializes the COM object. Any COM object previously set is shut down.

    The most efficient way to set this property is by using the UUID of the registered component, e.g.
    \code
    ctrl->setControl( "{8E27C92B-1264-101C-8A2F-040224009C02}" );
    \endcode
    The second fastest way is to use the class name of the registered control (with or without version number), e.g.
    \code
    ctrl->setControl( "MSCal.Calendar" );
    \endcode
    The slowest, but easiest way to use is to use the full name of the control, e.g.
    \code
    ctrl->setControl( "Calendar Control 9.0" );
    \endcode
    
    The read function of the control always returns the UUID of the control.
*/
bool QAxBase::setControl( const QString &c )
{
    if ( c == ctrl )
	return !ctrl.isEmpty();

    clear();
    ctrl = c;
    QUuid uuid( ctrl );
    if ( uuid.isNull() ) {
	QSettings controls;
	ctrl = controls.readEntry( "/Classes/" + c + "/CLSID/Default" );
	if ( ctrl.isEmpty() ) {
	    QStringList clsids = controls.subkeyList( "/Classes/CLSID" );
	    for ( QStringList::Iterator it = clsids.begin(); it != clsids.end(); ++it ) {
		QString clsid = *it;
		QStringList subkeys = controls.subkeyList( "/Classes/CLSID/" + clsid );
		if ( subkeys.contains( "Control" ) ) {
		    QString name = controls.readEntry( "/Classes/CLSID/" + clsid + "/Default" );
		    if ( name == c ) {
			ctrl = clsid;
			break;
		    }
		}
	    }
	}
    }
    if ( ctrl.isEmpty() )
	ctrl = c;
    initialize( &d->ptr );
    if ( isNull() ) {
#ifndef QT_NO_DEBUG
	qWarning( "QAxBase::setControl: requested control %s could not be instantiated.", c.latin1() );
#endif
	clear();
	return FALSE;
    }
    return TRUE;
}

QString QAxBase::control() const
{
    return ctrl;
}

/*!
    Disables the event sink implementation for this ActiveX container.
    If you don't intend to listen to events of the ActiveX control use this
    function to speed up the meta object generation.

    Some ActiveX controls might run unstable when connected with an event sink. 
    To get OLE events you will have to use standard COM methods to register your 
    own event sink. Use queryInterface to get access to the raw COM object.

    Note that this function should be called immediately after construction of the object (without
    passing an object identifier), before calling QAxWidget->setControl().
*/
void QAxBase::disableEventSink()
{
    d->useEventSink = FALSE;
}

/*!
    Disables the meta object generation for this ActiveX container. This also disables the
    event sink and class info generation. If you don't intend to use the Qt meta object 
    implementation call this function to speed up the meta object generation.

    Some ActiveX controls might run unstable when used with OLE automation. 
    Use standard COM methods to use those controls through the COM interfaces provided by 
    queryInterface.

    Note that this function has to be called immediately after construction of the object (without
    passing an object identifier), before calling QAxWidget->setControl().
*/
void QAxBase::disableMetaObject()
{
    d->useMetaObject	= FALSE;
    d->useEventSink	= FALSE;
    d->useClassInfo	= FALSE;
}

/*!
    Disables the class info generation for this ActiveX container. If you don't require any
    class information about the ActiveX control use this function to speed up the meta object
    generation.

    Note that this function has to be called immediately after construction of the object (without
    passing an object identifier), before calling QAxWidget->setControl().
*/
void QAxBase::disableClassInfo()
{
    d->useClassInfo = FALSE;
}

/*!
    Disconnects and destroys the COM object.

    If you reimplement this function you will also have to reimplement
    the destructor to call clear(), and call this implementation at the
    end of your clear() function.
*/
void QAxBase::clear()
{
    QDictIterator<QAxEventSink> it( d->eventSink );
    while ( it.current() ) {
	QAxEventSink *eventSink = it.current();
	++it;
	eventSink->unadvise();
	eventSink->Release();
    }
    d->eventSink.clear();
    if ( d->disp ) {
	d->disp->Release();
	d->disp = 0;
    }
    if ( d->ptr ) {
	d->ptr->Release();
	d->ptr = 0;
	moduleUnlock();
    }

    ctrl = QString::null;

    if ( d->metaobj && !d->cachedMetaObject ) {
	delete d->metaobj;
	d->metaobj = 0;
    }
}

/*!
    \fn bool QAxBase::initialize( IUnknown **ptr )

    This virtual function is called by setControl. Reimplement this function to return
    the IUnknown pointer of the COM object in \a ptr, and return TRUE if the object
    initialization succeeded. Otherwise, return FALSE.

    The interface returned in \a ptr should be referenced exactly once when this function
    returns. The interface provided by e.g. CoCreateInstance is already referenced, and
    there is no need to reference it again.
*/


/*!
    Requests the interface \a uuid from the COM object and sets the value of \a iface to the
    provided interface, or to null if the requested interface could not be provided.
    
    Returns the result of the QueryInterface implementation of the COM object.

    \sa control
*/
long QAxBase::queryInterface( const QUuid &uuid, void **iface ) const
{
    *iface = 0;
    if ( d->ptr && !uuid.isNull() )
	return d->ptr->QueryInterface( uuid, iface );
    
    return E_NOTIMPL;
}

static QString guessTypes( const TYPEDESC &tdesc, ITypeInfo *info, const QDict<QMetaEnum>& enumlist, const QString &function );

static inline QString usertypeToQString( const TYPEDESC &tdesc, ITypeInfo *info, const QDict<QMetaEnum>& enumlist, const QString &function )
{
    HREFTYPE usertype = tdesc.hreftype;
    if ( tdesc.vt != VT_USERDEFINED || !usertype )
	return QString::null;

    CComPtr<ITypeInfo> usertypeinfo;
    info->GetRefTypeInfo( usertype, &usertypeinfo );
    if ( usertypeinfo ) {
	CComPtr<ITypeLib> usertypelib;
	UINT index;
	usertypeinfo->GetContainingTypeLib( &usertypelib, &index );
	if ( usertypelib ) {
	    // get type name
	    BSTR usertypename;
	    usertypelib->GetDocumentation( index, &usertypename, 0, 0, 0 );
	    QString userTypeName = BSTRToQString( usertypename );
	    SysFreeString( usertypename );
	    // known enum?
	    QMetaEnum *metaEnum = enumlist.find( userTypeName );
	    if ( metaEnum )
		return userTypeName;
	    if ( userTypeName == "OLE_COLOR" || userTypeName == "VB_OLE_COLOR" )
		return "QColor";
	    if ( userTypeName == "IFontDisp" || userTypeName == "IFontDisp*" )
		return "QFont";
	    TYPEATTR *typeattr = 0;
	    usertypeinfo->GetTypeAttr( &typeattr );
	    if ( typeattr && typeattr->typekind == TKIND_ALIAS )
		userTypeName = guessTypes( typeattr->tdescAlias, info, enumlist, function );

	    usertypeinfo->ReleaseTypeAttr( typeattr );
	    return userTypeName;	    
	}
    }
    return QString::null;
}

static QString guessTypes( const TYPEDESC &tdesc, ITypeInfo *info, const QDict<QMetaEnum>& enumlist, const QString &function )
{
    QString str;
    switch ( tdesc.vt ) {
    case VT_I2:
    case VT_I4:
	str = "int";
	break;
    case VT_R4:
    case VT_R8:
	str = "double";
	break;
    case VT_CY:
	str = "long long"; // ### 64bit struct CY { ulong lo, long hi };
	break;
    case VT_DATE:
	str = "QDateTime";
	break;
    case VT_BSTR:
	str = "QString";
	break;
    case VT_DISPATCH:
	str = "IDispatch*";
	break;
    case VT_ERROR:
	str = "long";
	break;
    case VT_BOOL:
	str = "bool";
	break;
    case VT_VARIANT:
	str = "QVariant";
	break;
    case VT_UNKNOWN:
	str = "IUnknown*";
	break;
    case VT_I1:
	str = "char";
	break;
    case VT_UI1:
	str = "unsigned char";
	break;
    case VT_UI2:
	str = "unsigned short";
	break;
    case VT_UI4:
	str = "unsigned int";
	break;
    case VT_INT:
	str = "int";
	break;
    case VT_UINT:
	str = "unsigned int";
	break;
    case VT_VOID:
	str = "void";
	break;
    case VT_HRESULT:
	str = "long";
	break;
    case VT_LPSTR:
	str = "const char*";
	break;
    case VT_LPWSTR:
	str = "const unsigned short*";
	break;
    case VT_PTR:
	str = guessTypes( *tdesc.lptdesc, info, enumlist, function );
	if ( !str.isEmpty() && str != "QFont" ) 
	    str += "*";
	break;
    case VT_SAFEARRAY:
	str = guessTypes( tdesc.lpadesc->tdescElem, info, enumlist, function );
	if ( !str.isEmpty() ) 
	    str = str + "[" + QString::number( tdesc.lpadesc->cDims ) + "]";
	break;
    case VT_USERDEFINED:
	str = usertypeToQString( tdesc, info, enumlist, function );
	break;

    default:
	break;
    }

    if ( tdesc.vt & VT_BYREF )
	str += "&";

    return str;
}

static inline QString constRefify( const QString& type )
{
    QString crtype;

    if ( type == "QString" )
	crtype = "const QString&";
    else if ( type == "QDateTime" )
	crtype = "const QDateTime&";
    else if ( type == "QVariant" )
	crtype = "const QVariant&";
    else if ( type == "QColor" )
	crtype = "const QColor&";
    else if ( type == "QFont" )
	crtype = "const QFont&";
    else 
	crtype = type;

    return crtype;
}

static inline void QStringToQUType( const QString& type, QUParameter *param, const QDict<QMetaEnum> &enumDict )
{
    param->typeExtra = 0;
    QMetaEnum *enumData = 0;
    if ( type == "int" || type == "long" ) {
	param->type = &static_QUType_int;
    } else if ( type == "bool" ) {
	param->type = &static_QUType_bool;
    } else if ( type == "QString" || type == "const QString&" ) {
	param->type = &static_QUType_QString;
    } else if ( type == "double" ) {
	param->type = &static_QUType_double;
    } else if ( type == "QVariant" || type == "const QVariant&" ) {
	param->type = &static_QUType_QVariant;
    } else if ( (enumData = enumDict.find( type )) != 0 ) {
	param->type = &static_QUType_enum;
	QUEnum *uEnum = new QUEnum;
	uEnum->count = enumData->count;
	uEnum->name = enumData->name;
	uEnum->set = enumData->set;
	uEnum->items = new QUEnumItem[uEnum->count];
	for ( uint item = 0; item < uEnum->count; ++item ) {
	    QUEnumItem *uEnumItem = (QUEnumItem *)uEnum->items+item;
	    uEnumItem->key = enumData->items[item].key;
	    uEnumItem->value = enumData->items[item].value;
	}
	param->typeExtra = uEnum;
    } else {
	param->type = &static_QUType_ptr;
	QString ptype = type;
	if ( ptype.right(1) == "*" )
	    ptype.remove( ptype.length()-1, 1 );
	param->typeExtra = new char[ ptype.length() + 1 ];
	param->typeExtra = qstrcpy( (char*)param->typeExtra, ptype );
    }
}

/*!
    \reimp

    This is where all the magic happens.
    The metaobject is generated on the fly from the information provided by the 
    IDispatch and ITypeInfo interface implementations in the COM object.

    Yes, this is spaghetti code...
*/
QMetaObject *QAxBase::metaObject() const
{
    if ( d->metaobj )
	return d->metaobj->metaObject;
    QMetaObject* parentObject = parentMetaObject();

    // some signals and properties are always there
    static const QUParameter param_signal_0[] = {
	{ "name", &static_QUType_QString, 0, QUParameter::In },
	{ "argc", &static_QUType_int, 0, QUParameter::In },
	{ "argv", &static_QUType_ptr, "void", QUParameter::In }
    };
    static const QUMethod signal_0 = {"signal", 3, param_signal_0 };

    static const QUParameter param_signal_1[] = {
	{ "name", &static_QUType_QString, 0, QUParameter::In }
    };
    static const QUMethod signal_1 = {"propertyChanged", 1, param_signal_1 };
    static const QMetaData signal_tbl[] = {
	{ "signal(const QString&,int,void*)", &signal_0, QMetaData::Public },
	{ "propertyChanged(const QString&)", &signal_1, QMetaData::Public }
    };
    static const QMetaProperty props_tbl[1] = {
 	{ "QString","control", 259, (QMetaObject**)&tempMetaObj, 0, -1 }
    };

    // return the default meta object if not yet initialized
    if ( !d->ptr || !d->useMetaObject ) {
	if ( tempMetaObj )
	    return tempMetaObj;

	tempMetaObj = QMetaObject::new_metaobject(
	    "QAxBase", parentObject,
	    0, 0,
	    signal_tbl, 2,
#ifndef QT_NO_PROPERTIES
	    props_tbl, 1,
	    0, 0,
#endif // QT_NO_PROPERTIES
	    0, 0 );

	cleanUp_QAxBase.setMetaObject( tempMetaObj );
	return tempMetaObj;
    }

    // the rest is generated from the IDispatch implementation
    QAxBase* that = (QAxBase*)this; // mutable
#ifndef QAX_NO_CLASSINFO
    QSettings iidnames;
    if ( d->useClassInfo )
	iidnames.insertSearchPath( QSettings::Windows, "/Classes" );
#endif
    QUuid iid_propNotifySink( IID_IPropertyNotifySink );

    QDict<QUMethod> slotlist; // QUMethods deleted
    QDict<QUMethod> signallist; // QUMethods deleted
    QDict<QMetaProperty> proplist;
    proplist.setAutoDelete( TRUE ); // deep copied when creating metaobject
#ifndef QAX_NO_CLASSINFO
    QDict<QString> infolist;
    infolist.setAutoDelete( TRUE ); // deep copied when creating metaobject
#endif
    QDict<QMetaEnum> enumlist;
    enumlist.setAutoDelete( TRUE ); // deep copied when creating metaobject
    QDict<QMetaEnum> enumDict;	    // dict for fast lookup when generation property/parameter information
    UINT index = 0;
    QMetaEnum *enum_data = 0;

    // this string gets all the warnings, so that a client application
    // can use the classInfo "debugInfo" to get debug information about
    // what went wrong...
    QCString debugInfo;

    // create class information
    CComPtr<IProvideClassInfo> classinfo;
    CComPtr<ITypeLib> typelib;
    d->ptr->QueryInterface( IID_IProvideClassInfo, (void**)&classinfo );
    QString coClassID;
    if ( classinfo ) {
	CComPtr<ITypeInfo> info;
	classinfo->GetClassInfo( &info );
	TYPEATTR *typeattr = 0;
	if ( info )
	    info->GetTypeAttr( &typeattr );

	if ( typeattr ) {
	    QUuid clsid( typeattr->guid );
	    coClassID = clsid.toString().upper();
#ifndef QAX_NO_CLASSINFO
	// UUID
	    if ( d->useClassInfo && !infolist.find( "CoClass" ) ) {
		QString coClassIDstr = iidnames.readEntry( "/CLSID/" + coClassID + "/Default", coClassID );
		infolist.insert( "CoClass", new QString( coClassIDstr ) );
		QString version( "%1.%1" );
		version = version.arg( typeattr->wMajorVerNum ).arg( typeattr->wMinorVerNum );
		infolist.insert( "Version", new QString( version ) );
	    }
#endif
	    info->ReleaseTypeAttr( typeattr );
	}
    }

    if ( mo_cache && !coClassID.isEmpty() ) {
	d->metaobj = metaObjectCache()->find( coClassID );
	if ( d->metaobj ) {
	    d->cachedMetaObject = TRUE;
	    return d->metaobj->metaObject;
	}
    }
    d->metaobj = new QAxMetaObject;

    IDispatch *disp = d->dispatch();
    if ( disp ) {
	CComPtr<ITypeInfo> info;
	disp->GetTypeInfo( 0, LOCALE_USER_DEFAULT, &info );
	info->GetContainingTypeLib( &typelib, &index );
	// Read enum information from type library
	if ( typelib ) {
	    index = typelib->GetTypeInfoCount();
	    for ( UINT i = 0; i < index; ++i ) {
		TYPEKIND typekind;
		typelib->GetTypeInfoType( i, &typekind );
		if ( typekind == TKIND_ENUM ) {
		    // Get the type information for the enum
		    CComPtr<ITypeInfo> enuminfo;
		    typelib->GetTypeInfo( i, &enuminfo );
		    if ( !enuminfo )
			continue;
		    
		    // Get the name of the enumeration
		    BSTR enumname;
		    QString enumName;
		    if ( typelib->GetDocumentation( i, &enumname, 0, 0, 0 ) == S_OK ) {
			enumName = BSTRToQString( enumname );
			SysFreeString( enumname );
		    } else {
			enumName = QString( "enum%1" ).arg( enumlist.count() );
		    }
		    
		    // Get the attributes of the enum type
		    TYPEATTR *typeattr = 0;
		    enuminfo->GetTypeAttr( &typeattr );
		    if ( typeattr ) {
			// Create the QMetaEnum
			QMetaEnum * metaEnum = new QMetaEnum;
			metaEnum->name = new char[enumName.length() + 1];
			metaEnum->name = qstrcpy( (char*)metaEnum->name, enumName );
			metaEnum->items = typeattr->cVars ? new QMetaEnum::Item[typeattr->cVars] : 0;
			metaEnum->count = typeattr->cVars;
			metaEnum->set = FALSE;
			
			// Get all values of the enumeration
			for ( UINT vd = 0; vd < typeattr->cVars; ++vd ) {
			    VARDESC *vardesc = 0;
			    enuminfo->GetVarDesc( vd, &vardesc );
			    if ( vardesc && vardesc->varkind == VAR_CONST ) {
				int value = vardesc->lpvarValue->lVal;
				int memid = vardesc->memid;
				// Get the name of the value
				BSTR valuename;
				QString valueName;
				UINT maxNamesOut;
				enuminfo->GetNames( memid, &valuename, 1, &maxNamesOut );
				if ( maxNamesOut ) {
				    valueName = BSTRToQString( valuename );
				    SysFreeString( valuename );
				} else {
				    valueName = QString( "value%1" ).arg( vd );
				}
				// Add to QMetaEnum
				QMetaEnum::Item *items = (QMetaEnum::Item*)metaEnum->items;
				items[vd].value = value;
				items[vd].key = new char[valueName.length()+1];
				items[vd].key = qstrcpy( (char*)metaEnum->items[vd].key, valueName );
			    }
			    enuminfo->ReleaseVarDesc( vardesc );
			}
			// Add QMetaEnum to dict
			enumlist.insert( enumName, metaEnum );
		    }
		    enuminfo->ReleaseTypeAttr( typeattr );
		}
	    }
	}
        
	// setup enum data array
	index = 0;
	enum_data = enumlist.count() ? new QMetaEnum[enumlist.count()] : 0;
	QDictIterator<QMetaEnum> enum_it( enumlist );
	while ( enum_it.current() ) {
	    QMetaEnum metaEnum = *enum_it.current();
	    enum_data[index].name = metaEnum.name;
	    enum_data[index].count = metaEnum.count;
	    enum_data[index].items = metaEnum.items;
	    enum_data[index].set = metaEnum.set;
	    enumDict.insert( enum_it.currentKey(), enum_data+index );
	    ++index;
	    ++enum_it;
	}
	d->metaobj->enums = enum_data;
	d->metaobj->numEnums = enumlist.count();

	// read type information
	while ( info ) {
	    ushort nFuncs = 0;
	    ushort nVars = 0;
	    ushort nImpl = 0;
	    // get information about type
	    TYPEATTR *typeattr;
	    info->GetTypeAttr( &typeattr );
	    bool interesting = TRUE;
	    if ( typeattr ) {
		// get number of functions, variables, and implemented interfaces
		nFuncs = typeattr->cFuncs;
		nVars = typeattr->cVars;
		nImpl = typeattr->cImplTypes;

		if ( ( typeattr->typekind == TKIND_DISPATCH || typeattr->typekind == TKIND_INTERFACE ) &&
		    ( typeattr->guid != IID_IDispatch && typeattr->guid != IID_IUnknown ) ) {
#ifndef QAX_NO_CLASSINFO
		    if ( d->useClassInfo ) {
			// UUID
			QUuid uuid( typeattr->guid );
			QString uuidstr = uuid.toString().upper();
			uuidstr = iidnames.readEntry( "/Interface/" + uuidstr + "/Default", uuidstr );
			static interfacecount = 0;
			infolist.insert( QString("Interface %1").arg(++interfacecount), new QString( uuidstr ) );
		    }
#endif
		    info->ReleaseTypeAttr( typeattr );
		} else {
		    interesting = FALSE;
		}
	    }

	    // get information about all functions
	    if ( interesting ) for ( ushort fd = 0; fd < nFuncs ; ++fd ) {
		FUNCDESC *funcdesc;
		info->GetFuncDesc( fd, &funcdesc );
		if ( !funcdesc )
		    break;

		// get function prototype
		TYPEDESC typedesc = funcdesc->elemdescFunc.tdesc;

		QString function;
		QString returnType;
		QString prototype;
		QStringList parameters;
		QStringList paramTypes;

		// parse function description
		BSTR bstrNames[256];
		UINT maxNames = 255;
		UINT maxNamesOut;
		info->GetNames( funcdesc->memid, (BSTR*)&bstrNames, maxNames, &maxNamesOut );
		for ( int p = 0; p < (int)maxNamesOut; ++ p ) {
		    QString paramName = BSTRToQString( bstrNames[p] );
		    SysFreeString( bstrNames[p] );

		    // function name
		    if ( !p ) {
			function = paramName;
			prototype = function + "(";

			// get return value
			returnType = guessTypes( typedesc, info, enumDict, function );

			if ( funcdesc->invkind == INVOKE_FUNC && returnType != "void" ) {
			    parameters << "return";
			    paramTypes << returnType;
			}

			continue;
		    }

		    // parameter
		    bool optional = p > funcdesc->cParams - funcdesc->cParamsOpt;
		    TYPEDESC tdesc = funcdesc->lprgelemdescParam[p - ( (funcdesc->invkind == INVOKE_FUNC) ? 1 : 0 ) ].tdesc;
		    QString ptype = guessTypes( tdesc, info, enumDict, function );

		    if ( funcdesc->invkind == INVOKE_FUNC )
			ptype = constRefify( ptype );

		    prototype += ptype;
		    if ( optional )
			ptype += "=0";
		    paramTypes << ptype;
		    parameters << paramName;
		    if ( p < funcdesc->cParams )
			prototype += ",";
		}
		if ( !!prototype )
		    prototype += ")";

		QMetaProperty *prop = 0;

		// get type of function
		if ( !(funcdesc->wFuncFlags & FUNCFLAG_FHIDDEN) ) switch( funcdesc->invkind ) {			
		case INVOKE_PROPERTYGET: // property
		case INVOKE_PROPERTYPUT:
		    {
			prop = proplist[function];
			if ( !prop ) {
			    if ( funcdesc->cParams > 1 ) {
#ifndef QT_NO_DEBUG
				qWarning( "%s: Too many parameters in property.", function.latin1() );
#endif
				debugInfo += QString( "%1: Too many parameters in property.\n").arg( function );
				break;
			    }

			    QString ptype = paramTypes[0];
			    if ( ptype.isEmpty() )
				ptype = returnType;

			    prop = new QMetaProperty;
			    proplist.insert( function, prop );
			    prop->meta = (QMetaObject**)&(d->metaobj->metaObject);
			    prop->_id = -1;
			    if ( !ptype.isEmpty() )
				prop->enumData = enumDict.find( ptype );
			    else
				prop->enumData = 0;
			    prop->flags = PropStored;
			    if ( !(funcdesc->wFuncFlags & FUNCFLAG_FNONBROWSABLE) )
				prop->flags |= PropDesignable;
			    if ( !(funcdesc->wFuncFlags & FUNCFLAG_FRESTRICTED) )
				prop->flags |= PropScriptable;
			    if ( funcdesc->wFuncFlags & FUNCFLAG_FREQUESTEDIT )
				prop->flags |= PropRequesting;
			    if ( prop->enumData )
				prop->flags |= QMetaProperty::EnumOrSet;

			    if ( ptype != "void" ) {
				prop->t = new char[ptype.length()+1];
				prop->t = qstrcpy( (char*)prop->t, ptype );
			    } else {
				prop->t = 0;
			    }
			    prop->n = new char[function.length()+1];
			    prop->n = qstrcpy( (char*)prop->n, function );

			    if ( funcdesc->wFuncFlags & FUNCFLAG_FBINDABLE ) {
				prop->flags |= PropBindable;
				QAxEventSink *eventSink = d->eventSink.find( iid_propNotifySink );
				if ( !eventSink && d->useEventSink ) {
				    eventSink = new QAxEventSink( that );
				    d->eventSink.insert( iid_propNotifySink, eventSink );
				}
				// generate changed signal
				QString signalName = function + "Changed";
				QString signalParam = constRefify( ptype );
				QString signalProto = signalName + "(" + signalParam + ")";
				QString paramName = "value";
				if ( !signallist.find( signalProto ) ) {
				    QUMethod *signal = new QUMethod;
				    signal->name = new char[signalName.length()+1];
				    signal->name = qstrcpy( (char*)signal->name, signalName );
				    signal->count = 1;
				    QUParameter *param = new QUParameter;
				    param->name = new char[paramName.length()+1];
				    param->name = qstrcpy( (char*)param->name, paramName );
				    param->inOut = QUParameter::In;
				    QStringToQUType( signalParam, param, enumDict );
				    signal->parameters = param;

				    signallist.insert( signalProto, signal );
				}
				if ( eventSink )
				    eventSink->addProperty( funcdesc->memid, function, signalProto );
			    }
			} else if ( !prop->t ) {
			    QString ptype = paramTypes[0];
			    if ( ptype.isEmpty() )
				ptype = returnType;
			    if ( paramTypes.isEmpty() )
				paramTypes.append( ptype );
			    else
				paramTypes[0] = ptype;
			    prop->t = new char[ptype.length()+1];
			    prop->t = qstrcpy( (char*)prop->t, ptype );
			}
			if ( funcdesc->invkind == INVOKE_PROPERTYGET ) {
			    prop->flags |= QMetaProperty::Readable;
			} else {
			    prop->flags |= QMetaProperty::Writable;
			}
			if ( !prop->t )
			    break;
			// fall through to generate put function as slot
		    }
		    if ( funcdesc->invkind != INVOKE_PROPERTYPUT )
			break;

		case INVOKE_FUNC: // method
		    {
			if ( funcdesc->invkind == INVOKE_PROPERTYPUT && prop ) {
			    QString set;
			    QString pname = function;
			    QString firstletter = function.left(1);
			    if ( firstletter == firstletter.upper() ) {
				set = "Set";
			    } else {
				set = "set";
				function = firstletter.upper() + function.mid(1);
			    }
			    function = set + function;
			    QString ptype = prop->type();
			    ptype = prop->type();
			    if ( ptype.isEmpty() )
				ptype = returnType;
			    ptype = constRefify( ptype );
			    prototype = function + "(" + ptype + ")";

			    if ( !!ptype )
				paramTypes = ptype;
			    parameters = pname;
			    if ( slotlist.find( prototype ) )
				break;
			}
			bool defargs;
			QString defprototype = prototype;
			do {
			    defargs = FALSE;
			    QUMethod *slot = new QUMethod;
			    slot->name = new char[function.length()+1];
			    slot->name = qstrcpy( (char*)slot->name, function );
			    slot->count = parameters.count();
			    QUParameter *params = slot->count ? new QUParameter[slot->count] : 0;
			    int offset = parameters[0] == "return" ? 1 : 0;
			    for ( int p = 0; p< slot->count; ++p ) {
				QString paramName = parameters[p];
				QString paramType = paramTypes[p];
				if ( paramType.right( 2 ) == "=0" ) {
				    paramType.truncate( paramType.length()-2 );
				    paramTypes[p] = paramType;
				    defargs = TRUE;
				    slot->count = p;
				    prototype = function + "(";
				    for ( int pp = offset; pp < p; ++pp ) {
					prototype += paramTypes[pp];
					if ( pp < p-1 )
					    prototype += ",";
				    }
				    prototype += ")";
				    break;
				}
				params[p].name = new char[paramName.length()+1];
				params[p].name = qstrcpy( (char*)params[p].name, paramName );
				params[p].inOut = 0;
				if ( !p && paramName == "return" ) {
				    params[p].inOut = QUParameter::Out;
				} else if ( funcdesc->lprgelemdescParam + p - offset ) {
				    ushort inout = funcdesc->lprgelemdescParam[p-offset].paramdesc.wParamFlags;
				    if ( inout & PARAMFLAG_FIN )
					params[p].inOut |= QUParameter::In;
				    if ( inout & PARAMFLAG_FOUT )
					params[p].inOut |= QUParameter::Out;
				}
				QStringToQUType( paramType, params + p, enumDict );
			    }

			    slot->parameters = params;
			    slotlist.insert( prototype, slot );
			    prototype = defprototype;
			} while ( defargs );
		    }
		    break;

		default:
		    break;
		}

#if 0 // documentation in metaobject would be cool?
		// get function documentation
		BSTR bstrDocu;
		info->GetDocumentation( funcdesc->memid, 0, &bstrDocu, 0, 0 );
		QString strDocu = BSTRToQString( bstrDocu );
		if ( !!strDocu )
		    desc += "[" + strDocu + "]";
		desc += "\n";
		SysFreeString( bstrDocu );
#endif
		info->ReleaseFuncDesc( funcdesc );
	    }
	    
	    // get information about all variables
	    if ( interesting ) for ( ushort vd = 0; vd < nVars; ++vd ) {
		VARDESC *vardesc;
		info->GetVarDesc( vd, &vardesc );
		if ( !vardesc )
		    break;

		// no use if it's not a dispatched variable
		if ( vardesc->varkind != VAR_DISPATCH ) {
		    info->ReleaseVarDesc( vardesc );
		    continue;
		}

		// get variable name
		QString variableName;

		BSTR bstrNames[256];
		UINT maxNames = 255;
		UINT maxNamesOut;
		info->GetNames( vardesc->memid, (BSTR*)&bstrNames, maxNames, &maxNamesOut );
		for ( int v = 0; v < (int)maxNamesOut; ++v ) {
		    QString varName = BSTRToQString( bstrNames[v] );
		    SysFreeString( bstrNames[v] );

		    if ( !v ) {
			variableName = varName;
			continue;
		    }
		}
		// get variable type
		TYPEDESC typedesc = vardesc->elemdescVar.tdesc;
		QString variableType = guessTypes( typedesc, info, enumDict, variableName );


		if ( !(vardesc->wVarFlags & VARFLAG_FHIDDEN) ) {
		    // generate meta property
		    QMetaProperty *prop = proplist[variableName];
		    if ( !prop ) {
			prop = new QMetaProperty;
			proplist.insert( variableName, prop );
			prop->meta = (QMetaObject**)&d->metaobj->metaObject;
			prop->_id = -1;
			if ( !variableType.isEmpty() )
			    prop->enumData = enumDict.find( variableType );
			else
			    prop->enumData = 0;
			prop->flags = QMetaProperty::Readable | PropStored;
			if ( !(vardesc->wVarFlags & VARFLAG_FREADONLY) )
			    prop->flags |= QMetaProperty::Writable;;
			if ( !(vardesc->wVarFlags & VARFLAG_FNONBROWSABLE) )
			    prop->flags |= PropDesignable;
			if ( !(vardesc->wVarFlags & VARFLAG_FRESTRICTED) )
			    prop->flags |= PropScriptable;
			if ( vardesc->wVarFlags & VARFLAG_FREQUESTEDIT )
			    prop->flags |= PropRequesting;
			if ( prop->enumData != 0 )
			    prop->flags |= QMetaProperty::EnumOrSet;

			prop->t = new char[variableType.length()+1];
			prop->t = qstrcpy( (char*)prop->t, variableType );
			prop->n = new char[variableName.length()+1];
			prop->n = qstrcpy( (char*)prop->n, variableName );

			if ( vardesc->wVarFlags & VARFLAG_FBINDABLE ) {
			    prop->flags |= PropBindable;
			    QAxEventSink *eventSink = d->eventSink.find( iid_propNotifySink );
			    if ( !eventSink && d->useEventSink ) {
				eventSink = new QAxEventSink( that );
				d->eventSink.insert( iid_propNotifySink, eventSink );
			    }
			    // generate changed signal
			    QString signalName = variableName + "Changed";
			    QString signalParam = constRefify( variableType );
			    QString signalProto = signalName + "(" + signalParam + ")";
			    QString paramName = "value";
			    if ( !signallist.find( signalProto ) ) {
				QUMethod *signal = new QUMethod;
				signal->name = new char[signalName.length()+1];
				signal->name = qstrcpy( (char*)signal->name, signalName );
				signal->count = 1;
				QUParameter *param = new QUParameter;
				param->name = new char[paramName.length()+1];
				param->name = qstrcpy( (char*)param->name, paramName );
				param->inOut = QUParameter::In;
				QStringToQUType( signalParam, param, enumDict );
				signal->parameters = param;

				signallist.insert( signalProto, signal );
			    }
			    if ( eventSink )
				eventSink->addProperty( vardesc->memid, variableName, signalProto );
			}
		    }

		    // generate a set slot
		    if ( !(vardesc->wVarFlags & VARFLAG_FREADONLY) ) {
			QString firstletter = variableName.left(1);
			QString set;
			if ( firstletter == firstletter.upper() ) {
			    set = "Set";
			} else {
			    set = "set";
			}

			variableType = constRefify( variableType );
			QString function = set + variableName;
			QString prototype = function + "(" + variableType + ")";

			if ( !slotlist.find( prototype ) ) {
			    QUMethod *slot = new QUMethod;
			    slot->name = new char[ function.length() + 1 ];
			    slot->name = qstrcpy( (char*)slot->name, function );
			    slot->count = 1;
			    QUParameter *params = new QUParameter;
			    params->inOut = QUParameter::In;
			    params->name = new char[ variableName.length() + 1 ];
			    params->name = qstrcpy( (char*)params->name, variableName );

			    QStringToQUType( variableType, params, enumDict );

			    slot->parameters = params;
			    slotlist.insert( prototype, slot );
			}
		    }

#if 0 // documentation in metaobject would be cool?
		    // get function documentation
		    BSTR bstrDocu;
		    info->GetDocumentation( vardesc->memid, 0, &bstrDocu, 0, 0 );
		    QString strDocu = BSTRToQString( bstrDocu );
		    if ( !!strDocu )
			desc += "[" + strDocu + "]";
		    desc += "\n";
		    SysFreeString( bstrDocu );
#endif
		}
		info->ReleaseVarDesc( vardesc );
	    }

	    if ( !nImpl )
		break;

	    // go up one base class
	    HREFTYPE pRefType;
	    info->GetRefTypeOfImplType( 0, &pRefType );
	    CComPtr<ITypeInfo> baseInfo;
	    info->GetRefTypeInfo( pRefType, &baseInfo );
	    if ( info == baseInfo ) // IUnknown inherits IUnknown ???
		break;
	    info = baseInfo;
	}
    }

    CComPtr<IConnectionPointContainer> cpoints;
    d->ptr->QueryInterface( IID_IConnectionPointContainer, (void**)&cpoints );
    if ( cpoints && d->useEventSink ) {
	// Get connection point enumerator
	CComPtr<IEnumConnectionPoints> epoints;
	cpoints->EnumConnectionPoints( &epoints );
	if ( epoints ) {
	    ULONG c = 1;
	    epoints->Reset();
	    do {
		CComPtr<IConnectionPoint> cpoint;
		epoints->Next( c, &cpoint, &c );
		if ( !c )
		    break;

		IID conniid;
		cpoint->GetConnectionInterface( &conniid );
#ifndef QAX_NO_CLASSINFO
		if ( d->useClassInfo ) {
  		    QUuid uuid( conniid );
  		    QString uuidstr = uuid.toString().upper();
  		    uuidstr = iidnames.readEntry( "/Interface/" + uuidstr + "/Default", uuidstr );
  		    static eventcount = 0;
  		    infolist.insert( QString("Event Interface %1").arg(++eventcount), new QString( uuidstr ) );
		}
#endif

		// get information about type
		CComPtr<ITypeInfo> eventinfo;
		if ( conniid == IID_IPropertyNotifySink ) {
		    // test whether property notify sink has been created already, and advise on it
		    QAxEventSink *eventSink = d->eventSink.find( iid_propNotifySink );
		    if ( eventSink )
			eventSink->advise( cpoint, conniid );
		    continue;
		} else if ( typelib ) {
		    typelib->GetTypeInfoOfGuid( conniid, &eventinfo );
		}

		// what about other event interfaces?
		if ( eventinfo ) {
		    TYPEATTR *eventattr;
		    eventinfo->GetTypeAttr( &eventattr );
		    if ( !eventattr )
			continue;
		    if ( eventattr->typekind != TKIND_DISPATCH )
			continue;

		    QAxEventSink *eventSink = d->eventSink.find( QUuid(conniid) );
		    if ( !eventSink ) {
			eventSink = new QAxEventSink( that );
			d->eventSink.insert( QUuid(conniid), eventSink );
			eventSink->advise( cpoint, conniid );
		    }

		    // get information about all event functions
		    for ( UINT fd = 0; fd < eventattr->cFuncs; ++fd ) {
			FUNCDESC *funcdesc;
			eventinfo->GetFuncDesc( fd, &funcdesc );
			if ( !funcdesc )
			    break;
			if ( funcdesc->invkind != INVOKE_FUNC ||
			     funcdesc->funckind != FUNC_DISPATCH ) {
			    eventinfo->ReleaseFuncDesc( funcdesc );
			    continue;
			}

			// get return value
			TYPEDESC typedesc = funcdesc->elemdescFunc.tdesc;
			QString returnType;

			// get event function prototype
			QString function;
			QString prototype;
			QStringList parameters;
			QStringList paramTypes;

			BSTR bstrNames[256];
			UINT maxNames = 255;
			UINT maxNamesOut;
			eventinfo->GetNames( funcdesc->memid, (BSTR*)&bstrNames, maxNames, &maxNamesOut );
			int p;
			for ( p = 0; p < (int)maxNamesOut; ++ p ) {
			    QString paramName = BSTRToQString( bstrNames[p] );
			    SysFreeString( bstrNames[p] );

			    // function name
			    if ( !p ) {
				function = paramName;
				returnType = guessTypes( typedesc, eventinfo, enumDict, function );
				prototype = function + "(";
				continue;
			    }

			    // parameter
			    bool optional = p > funcdesc->cParams - funcdesc->cParamsOpt;
			    
			    TYPEDESC tdesc = funcdesc->lprgelemdescParam[p - ( (funcdesc->invkind == INVOKE_FUNC) ? 1 : 0 ) ].tdesc;
			    QString ptype = guessTypes( tdesc, eventinfo, enumDict, function );

			    if ( funcdesc->invkind == INVOKE_FUNC )
				ptype = constRefify( ptype );

			    paramTypes << ptype;
			    parameters << paramName;
			    prototype += ptype;
			    if ( optional )
				prototype += "=0";
			    if ( p < funcdesc->cParams )
				prototype += ",";
			}
			if ( !!prototype )
			    prototype += ")";

			if ( !signallist.find( prototype ) ) {
			    QUMethod *signal = new QUMethod;
			    signal->name = new char[function.length()+1];
			    signal->name = qstrcpy( (char*)signal->name, function );
			    signal->count = parameters.count();
			    QUParameter *params = signal->count ? new QUParameter[signal->count] : 0;
			    for ( p = 0; p< signal->count; ++p ) {
				QString paramName = parameters[p];
				QString paramType = paramTypes[p];
				params[p].name = new char[paramName.length()+1];
				params[p].name = qstrcpy( (char*)params[p].name, paramName );
				params[p].inOut = 0;
				if ( funcdesc->lprgelemdescParam + p ) {
				    ushort inout = funcdesc->lprgelemdescParam[p].paramdesc.wParamFlags;
				    if ( inout & PARAMFLAG_FIN )
					params[p].inOut |= QUParameter::In;
				    if ( inout & PARAMFLAG_FOUT )
					params[p].inOut |= QUParameter::Out;
				}

				QStringToQUType( paramType, params + p, enumDict );
			    }
			    signal->parameters = params;
			    signallist.insert( prototype, signal );
			}
			if ( eventSink )
			    eventSink->addSignal( funcdesc->memid, prototype );

#if 0 // documentation in metaobject would be cool?
			// get function documentation
			BSTR bstrDocu;
			eventinfo->GetDocumentation( funcdesc->memid, 0, &bstrDocu, 0, 0 );
			QString strDocu = BSTRToQString( bstrDocu );
			if ( !!strDocu )
			    desc += "[" + strDocu + "]";
			desc += "\n";
			SysFreeString( bstrDocu );
#endif
			eventinfo->ReleaseFuncDesc( funcdesc );
		    }
		}
	    } while ( c );
	}
    }

#ifndef QAX_NO_CLASSINFO
    if ( !!debugInfo && d->useClassInfo )
	infolist.insert( "debugInfo", new QString( debugInfo ) );
#endif

    // setup slot data
    index = 0;
    QMetaData *const slot_data = slotlist.count() ? new QMetaData[slotlist.count()] : 0;
    QDictIterator<QUMethod> slot_it( slotlist );
    while ( slot_it.current() ) {
	QUMethod *slot = slot_it.current();
	slot_data[index].name = new char[slot_it.currentKey().length()+1];
	slot_data[index].name = qstrcpy( (char*)slot_data[index].name, slot_it.currentKey() );
	slot_data[index].method = slot;
	slot_data[index].access = QMetaData::Public;

	++index;
	++slot_it;
    }

    // setup signal data
    index = 0;
    QMetaData *const signal_data = new QMetaData[signallist.count()+2];
    if ( signal_data ) {
	signal_data[index] = signal_tbl[index];
	++index;
	signal_data[index] = signal_tbl[index];
	++index;
    }

    QDictIterator<QUMethod> signal_it( signallist );
    while ( signal_it.current() ) {
	QUMethod *signal = signal_it.current();
	signal_data[index].name = new char[signal_it.currentKey().length()+1];
	signal_data[index].name = qstrcpy( (char*)signal_data[index].name, signal_it.currentKey() );
	signal_data[index].method = signal;
	signal_data[index].access = QMetaData::Public;

	++index;
	++signal_it;
    }

    // setup property data
    index = 0;
    QMetaProperty *const prop_data = new QMetaProperty[proplist.count()+1];
    if ( prop_data ) {
	prop_data[index].n = new char[8];
	prop_data[index].n = qstrcpy( (char*)prop_data[index].n, "control" );
	prop_data[index].t = new char[8];
	prop_data[index].t = qstrcpy( (char*)prop_data[index].t, "QString" );
	prop_data[index].flags = 259;
	prop_data[index]._id = -1;
	prop_data[index].enumData = 0;
	prop_data[index].meta = (QMetaObject**)&d->metaobj->metaObject;

	++index;
    }
    QDictIterator<QMetaProperty> prop_it( proplist );
    while ( prop_it.current() ) {
	QMetaProperty *prop = prop_it.current();
	prop_data[index].t = prop->t;
	prop_data[index].n = prop->n;
	prop_data[index].flags = prop->flags;
	prop_data[index].meta = prop->meta;
	prop_data[index].enumData = prop->enumData;
	prop_data[index]._id = prop->_id;

	++index;
	++prop_it;
    }

#ifndef QAX_NO_CLASSINFO
    // setup class info data
    index = 0;
    QClassInfo *const class_info = infolist.count() ? new QClassInfo[infolist.count()] : 0;
    QDictIterator<QString> info_it( infolist );
    while ( info_it.current() ) {
	QString info = *info_it.current();
	QString key = info_it.currentKey();
	class_info[index].name = new char[key.length()+1];
	class_info[index].name = qstrcpy( (char*)class_info[index].name, key );
	class_info[index].value = new char[info.length()+1];
	class_info[index].value = qstrcpy( (char*)class_info[index].value, info );

	++index;
	++info_it;
    }
#endif

    // put the metaobject together
    d->metaobj->metaObject = QMetaObject::new_metaobject( 
	className(), parentObject, 
	slot_data, slotlist.count(),
	signal_data, signallist.count()+2,
	prop_data, proplist.count()+1,
	enum_data, enumlist.count(),
#ifndef QAX_NO_CLASSINFO
	class_info, infolist.count() );
#else
	0, 0 );
#endif

    if ( !coClassID.isEmpty() ) {
	metaObjectCache()->insert( coClassID, d->metaobj );
	d->cachedMetaObject = TRUE;
    }

    return d->metaobj->metaObject;
}

static inline bool checkHRESULT( HRESULT hres )
{
    switch( hres ) {
    case S_OK:
	return TRUE;
    case DISP_E_BADPARAMCOUNT:
	return FALSE;
    case DISP_E_BADVARTYPE:
	return FALSE;
    case DISP_E_EXCEPTION:
	return FALSE;
    case DISP_E_MEMBERNOTFOUND:
	return FALSE;
    case DISP_E_NONAMEDARGS:
	return FALSE;
    case DISP_E_OVERFLOW:
	return FALSE;
    case DISP_E_PARAMNOTFOUND:
	return FALSE;
    case DISP_E_TYPEMISMATCH:
	return FALSE;
    case DISP_E_UNKNOWNINTERFACE:
	return FALSE;
    case DISP_E_UNKNOWNLCID:
	return FALSE;
    case DISP_E_PARAMNOTOPTIONAL:
	return FALSE;
    default:
	return FALSE;
    }
}

/*!
    \internal
*/
bool QAxBase::qt_invoke( int _id, QUObject* _o )
{
    const int index = _id - metaObject()->slotOffset();
    if ( !d->ptr || index < 0 )
	return FALSE;

    // get the IDispatch
    IDispatch *disp = d->dispatch();
    if ( !disp )
	return FALSE;

    // get the slot information
    const QMetaData *slot_data = metaObject()->slot( index );
    if ( !slot_data )
	return FALSE;
    const QUMethod *slot = slot_data->method;
    if ( !slot )
	return FALSE;

    // Get the Dispatch ID of the method to be called
    bool fakedslot = FALSE;
    DISPID dispid;
    OLECHAR *names = (TCHAR*)qt_winTchar(slot->name, TRUE );
    disp->GetIDsOfNames( IID_NULL, &names, 1, LOCALE_USER_DEFAULT, &dispid );
    if ( dispid == DISPID_UNKNOWN ) {
	// see if we are calling a property set function as a slot
	if ( QString( slot->name ).left( 3 ) != "set" &&
	     QString( slot->name ).left( 3 ) != "Set" )
	    return FALSE;
	QString realname = slot->name;
	realname = realname.right( realname.length() - 3 );
	OLECHAR *realnames = (TCHAR*)qt_winTchar(realname, TRUE );
	disp->GetIDsOfNames( IID_NULL, &realnames, 1, LOCALE_USER_DEFAULT, &dispid );
	if ( dispid == DISPID_UNKNOWN )
	    return FALSE;

	fakedslot = TRUE;
    }

    // setup the parameters
    VARIANT ret; // Invoke initializes it
    VARIANT *pret = 0;
    if ( slot->parameters && ( slot->parameters[0].inOut & QUParameter::Out ) ) // slot has return value
	pret = &ret;
    int slotcount = slot->count - ( pret ? 1 : 0 );

    VARIANT arg;
    DISPPARAMS params;
    DISPID dispidNamed = DISPID_PROPERTYPUT;
    params.cArgs = slotcount;
    params.cNamedArgs = fakedslot ? 1 : 0;
    params.rgdispidNamedArgs = fakedslot ? &dispidNamed : 0;
    params.rgvarg = slot->count ? new VARIANTARG[slotcount] : 0;
    int p;
    for ( p = 0; p < slotcount; ++p ) {
	QUObject *obj = _o + p + 1;
	QUObjectToVARIANT( obj, arg, slot->parameters + p + ( pret ? 1 : 0 ) );
	params.rgvarg[ slotcount - p - 1 ] = arg;
    }
    // call the method
    UINT argerr = 0;
    HRESULT hres;
    if ( fakedslot && slotcount == 1 ) {
	hres = disp->Invoke( dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &params, pret, 0, &argerr );
    } else {
	hres = disp->Invoke( dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, pret, 0, &argerr );	    
    }

    // get return value
    if ( pret )
	VARIANTToQUObject( ret, _o );

    // update out parameters
    for ( p = 0; p < slotcount; ++p ) {
	if ( slot->parameters && slot->parameters[p+1].inOut & QUParameter::Out ) {
	    QUObject *obj = _o + p+1;
	    arg = params.rgvarg[ slotcount - p-1 ];
	    VARIANTToQUObject( arg, obj );
	}
    }

    // clean up
    for ( p = 0; p < slotcount; ++p ) {
	if ( params.rgvarg[p].vt == VT_BSTR )
	    SysFreeString( params.rgvarg[p].bstrVal );
    }
    delete [] params.rgvarg;

    return checkHRESULT( hres );
}

/*!
    \reimp
*/
bool QAxBase::qt_property( int _id, int _f, QVariant* _v )
{
    const int index = _id - metaObject()->propertyOffset();
    if ( index == 0 ) { // control property
	switch( _f ) {
	case 0: setControl( _v->toString() ); break;
	case 1: *_v = control(); break;
	default: if ( _f > 5 ) return FALSE;
	}
	return TRUE;
    } else if ( d->ptr && index >= 0 ) {
	// get the IDispatch
	IDispatch *disp = d->dispatch();
	if ( !disp )
	    return FALSE;

	// get the property information
	const QMetaProperty *prop = metaObject()->property( index );
	if ( !prop )
	    return FALSE;

	//  get the Dispatch ID of the function to be called
	QString pname( prop->n );

	DISPID dispid;
	OLECHAR *names = (TCHAR*)qt_winTchar(prop->n, TRUE );
	disp->GetIDsOfNames( IID_NULL, &names, 1, LOCALE_USER_DEFAULT, &dispid );
	if ( dispid == DISPID_UNKNOWN )
	    return FALSE;

	switch ( _f ) {
	case 0: // Set
	    {
		VARIANTARG arg;
		arg.vt = VT_ERROR;
		arg.scode = DISP_E_TYPEMISMATCH;

		// map QVariant to VARIANTARG. ### Maybe it would be better 
		// to convert the QVariant to what the VARIANT is supposed to be,
		// but we don't know that from the QMetaProperty of course.
		if ( qstrcmp( prop->type(), _v->typeName() ) ) {
		    QVariant::Type type = QVariant::nameToType( prop->type() );
		    if ( type != QVariant::Invalid )
			_v->cast( type );
		}
		arg = QVariantToVARIANT( *_v, prop->type() );

		if ( arg.vt == VT_EMPTY ) {
		    qDebug( "QAxBase::setProperty(): Unhandled property type" );
		    return FALSE;
		}
		DISPPARAMS params;
		DISPID dispidNamed = DISPID_PROPERTYPUT;
		params.rgvarg = &arg;
		params.cArgs = 1;
		params.rgdispidNamedArgs = &dispidNamed;
		params.cNamedArgs = 1;

		UINT argerr = 0;
		HRESULT hres = disp->Invoke( dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &params, 0, 0, &argerr );
		switch ( arg.vt )
		{
		case VT_BSTR:
		    SysFreeString( arg.bstrVal );
		    break;
		case VT_DISPATCH:
		    if ( arg.pdispVal && _v->type() == QVariant::Font )
			arg.pdispVal->Release();
		    break;
		}
		
		return checkHRESULT( hres );
	    }
	case 1: // Get
	    {
		VARIANTARG arg;
		DISPPARAMS params;
		params.cArgs = 0;
		params.cNamedArgs = 0;
		params.rgdispidNamedArgs = 0;
		params.rgvarg = 0;

		HRESULT hres = disp->Invoke( dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &params, &arg, 0, 0 );
		if ( !checkHRESULT( hres ) )
		    return FALSE;

		if ( arg.vt == VT_UI4 && pname.endsWith( "Color" ) ) {
		    arg.vt = VT_USERDEFINED;
		}

		// map result VARIANTARG to QVariant
		*_v = VARIANTToQVariant( arg );
		return ( _v->isValid() );
	    }
	case 2: // Reset
	    return TRUE;
	case 3: // Designable
	    return prop->flags & PropDesignable;
	case 4: // Scriptable
	    return prop->flags & PropScriptable;
	case 5: // Stored
	    return prop->flags & PropStored;
	default:
	    break;
	}
    }
    return FALSE;
}

// this is copied from qobject.cpp
static inline bool isIdentChar( char x )
{						// Avoid bug in isalnum
    return x == '_' || (x >= '0' && x <= '9') ||
	 (x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z');
}

static inline bool isSpace( char x )
{
#if defined(Q_CC_BOR)
  /*
    Borland C++ 4.5 has a weird isspace() bug.
    isspace() usually works, but not here.
    This implementation is sufficient for our internal use: rmWS()
  */
    return (uchar) x <= 32;
#else
    return isspace( (uchar) x );
#endif
}

static inline QCString qt_rmWS( const char *s )
{
    QCString result( qstrlen(s)+1 );
    char *d = result.data();
    char last = 0;
    while( *s && isSpace(*s) )			// skip leading space
	s++;
    while ( *s ) {
	while ( *s && !isSpace(*s) )
	    last = *d++ = *s++;
	while ( *s && isSpace(*s) )
	    s++;
	if ( *s && isIdentChar(*s) && isIdentChar(last) )
	    last = *d++ = ' ';
    }
    result.truncate( (int)(d - result.data()) );
    int void_pos = result.find("(void)");
    if ( void_pos >= 0 )
	result.remove( void_pos+1, (uint)strlen("void") );
    return result;
}


/*!
    \internal
*/
bool QAxBase::internalInvoke( const QCString &name, void *inout, const QVariant &var )
{
    QVariant realVar = var;
    QCString function = name;
    int id = metaObject()->findSlot( qt_rmWS(function), TRUE );
    if ( id >= 0 ) {
	const QMetaData *slot = metaObject()->slot( id, TRUE );
	function = slot->method->name;
	if ( ( realVar.type() == QVariant::String || realVar.type() == QVariant::CString )
	    && QUType::isEqual( slot->method->parameters[1].type, &static_QUType_enum ) ) {
	    QUEnum *uEnum = (QUEnum *)slot->method->parameters[1].typeExtra;
	    for ( uint eItem = 0; eItem < uEnum->count; eItem++ ) {
		if ( uEnum->items[eItem].key == var.toString() ) {
		    realVar = uEnum->items[eItem].value;
		    break;
		}
	    }
	}
    }

    IDispatch *disp = d->dispatch();
    if ( !disp || !inout )
	return FALSE;

    DISPID dispid;
    OLECHAR *names = (TCHAR*)qt_winTchar(function, TRUE );
    disp->GetIDsOfNames( IID_NULL, &names, 1, LOCALE_USER_DEFAULT, &dispid );
    if ( dispid == DISPID_UNKNOWN ) {
#ifdef QT_CHECK_STATE
	const char *coclass = metaObject()->classInfo( "CoClass" );
	qWarning( "QAxBase::internalInvoke: %s: No such method or property in %s [%s]", (const char*)name, control().latin1(), 
	    coclass ? coclass: "unknown" );
#endif
	return FALSE;
    }

    VARIANTARG *res = (VARIANTARG*)inout;
    VARIANT arg;
    DISPPARAMS params;
    if ( !var.isValid() ) {
	params.cArgs = 0;
	params.cNamedArgs = 0;
	params.rgdispidNamedArgs = 0;
	params.rgvarg = 0;
    } else {
	arg = QVariantToVARIANT( realVar );
	params.cArgs = 1;
	params.cNamedArgs = 0;
	params.rgdispidNamedArgs = 0;
	params.rgvarg = &arg;
    }

    HRESULT hres = disp->Invoke( dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &params, res, 0, 0 );

    return checkHRESULT( hres );
}


/*!
    Calls the function \a function of the COM object passing the parameters \a var1 ... \a var8, 
    and returns the value, or an invalid QVariant if the function does not return a value.

    \a function has to be provided as the full prototype, like e.g. in QObject::connect().
    \code
    activeX->dynamicCall( "Navigate(const QString&)", "www.trolltech.com" );
    \endcode

    You can also use this method to set and get property values:
    \code
    activeX->dynamicCall( "Value", 5 );
    QString text = activeX->dynamicCall( "Text" ).toString();
    \endcode
    Using QObject::setProperty and QObject::property however is faster.

    It is only possible to call functions through dynamicCall that have parameters or return
    values of datatypes supported in QVariant. See the QAxBase class documentation for a list of 
    supported and unsupported datatypes. If you want to call functions that have
    unsupported datatypes in the parameter list, use queryInterface to retrieve the appropriate 
    COM interface, and use the function directly.

    \code
    IWebBrowser2 *webBrowser = 0;
    activeX->queryInterface( IID_IWebBrowser2, (void**)&webBrowser );
    if ( webBrowser ) {
        webBrowser->Navigate2( pvarURL );
	webBrowser->Release();
    }
    \endcode

    This is also more efficient.
*/
QVariant QAxBase::dynamicCall( const QCString &function, const QVariant &var1, 
							 const QVariant &var2, 
							 const QVariant &var3, 
							 const QVariant &var4, 
							 const QVariant &var5, 
							 const QVariant &var6, 
							 const QVariant &var7, 
							 const QVariant &var8 )
{
    bool ok = FALSE;
    QVariant result;

    int id = metaObject()->findSlot( qt_rmWS(function), TRUE );
    if ( id < 0 ) {
	// it's not a slot; try a property
	if ( var1.isValid() ) {
	    ok = qObject()->setProperty( function, var1 );
	} else {
	    result = qObject()->property( function );
	    ok = result.isValid();
	}
	if ( ok )
	    return result;
    } else {
	ok = TRUE;
    }
    if ( !ok ) {
#if defined(QT_CHECK_RANGE)
	const char *coclass = metaObject()->classInfo( "CoClass" );
	qWarning( "QAxBase::dynamicCall: %s: No such method or property in %s [%s]"
	    , (const char*)function, control().latin1(), coclass ? coclass: "unknown" );
#endif
	return QVariant();
    }
    const QMetaData *slot_data = 0;
    slot_data = metaObject()->slot( id, TRUE );

    if ( slot_data ) {
	const QUMethod *slot = 0;
	slot = slot_data->method;

	QUObject obj[9];
	const void *typeExtra[9];
	// obj[0] is the result
	int o;
	int ret = slot->count && slot->parameters[0].inOut == QUParameter::Out;
	for ( o = 0; o < slot->count-ret; ++o ) {
	    obj[o+1].type = slot->parameters[o+ret].type;
	    typeExtra[o+1] = slot->parameters[o+ret].typeExtra;
	}
	QVariantToQUObject( var1, obj[1], typeExtra[1] );
	QVariantToQUObject( var2, obj[2], typeExtra[2] );
	QVariantToQUObject( var3, obj[3], typeExtra[3] );
	QVariantToQUObject( var4, obj[4], typeExtra[4] );
	QVariantToQUObject( var5, obj[5], typeExtra[5] );
	QVariantToQUObject( var6, obj[6], typeExtra[6] );
	QVariantToQUObject( var7, obj[7], typeExtra[7] );
	QVariantToQUObject( var8, obj[8], typeExtra[8] );

	qt_invoke( id, obj );

	if ( !QUType::isEqual( obj->type, &static_QUType_Null ) ) {
	    if ( QUType::isEqual( obj->type, &static_QUType_int ) ) {
		result = static_QUType_int.get( obj );
	    } else if ( QUType::isEqual( obj->type, &static_QUType_QString ) ) {
		result = static_QUType_QString.get( obj );
	    } else if ( QUType::isEqual( obj->type, &static_QUType_charstar ) ) {
		result = static_QUType_charstar.get( obj );
	    } else if ( QUType::isEqual( obj->type, &static_QUType_bool ) ) {
		result = QVariant( static_QUType_bool.get( obj ), 0 );
	    } else if ( QUType::isEqual( obj->type, &static_QUType_double ) ) {
		result = static_QUType_double.get( obj );
	    } else if ( QUType::isEqual( obj->type, &static_QUType_enum ) ) {
		result = static_QUType_enum.get( obj );
	    } else if ( QUType::isEqual( obj->type, &static_QUType_QVariant ) ) {
		result = static_QUType_QVariant.get( obj );
	    } else if ( QUType::isEqual( obj->type, &static_QUType_idisp ) ) {
		//###
	    } else if ( QUType::isEqual( obj->type, &static_QUType_iface ) ) {
		//###
	    } else if ( QUType::isEqual( obj->type, &static_QUType_ptr ) && slot->parameters ) {
		const QUParameter *param = slot->parameters;
		const char *type = (const char*)param->typeExtra;
		if ( !qstrcmp( type, "int" ) ) {
		    result = *(int*)static_QUType_ptr.get( obj );
		} else if ( !qstrcmp( type, "QString" ) || !qstrcmp( type, "const QString&" ) ) {
		    result = *(QString*)static_QUType_ptr.get( obj );
		} else if ( !qstrcmp( type, "QDateTime" ) || !qstrcmp( type, "const QDateTime&" ) ) {
		    result = *(QDateTime*)static_QUType_ptr.get( obj );
		}
		//###
	    }
	}

	for ( o = 0; o < 9; ++o )
	    obj[o].type->clear( obj+o );
    }
    return result;
}

/*!
    Returns a pointer to a QAxObject wrapping the COM object provided by the method or 
    property \a name, passing \a var as an optional parameter. If \a name is provided by
    a method the string has to include the full function prototype.

    The returned object is a child of this object (which is either of type QAxObject or
    QAxWidget), and is deleted when this object is being  deleted. It is however safe to 
    delete the returned object.

    COM enabled applications usually have an object model publishing certain elements of
    the application as dispatch interfaces. Use this method to navigate the hierarchy of
    the object model, e.g.

    \code
    QAxWidget outlook( "Outlook.Application" );
    QAxObject *session = outlook.createSubObject( "Session" );
    if ( session ) {
	QAxObject *defFolder = session->createSubObject( "GetDefaultFolder(OlDefaultFolders)", "olFolderContacts" );
	//...
    }
    \endcode
*/
QAxObject *QAxBase::querySubObject( const QCString &name, const QVariant &var )
{
    QAxObject *object = 0;

    VARIANTARG res;

    internalInvoke( name, &res, var );
    switch ( res.vt ) {
    case VT_DISPATCH:
	if ( res.pdispVal )
	    object = new QAxObject( res.pdispVal, qObject(), QCString( qObject()->name() ) + "/" + name );
	break;
    case VT_UNKNOWN:
	if ( res.punkVal )
	    object = new QAxObject( res.punkVal, qObject(), QCString( qObject()->name() ) + "/" + name  );
	break;
    case VT_EMPTY:
#ifdef QT_CHECK_STATE
	{
	    const char *coclass = metaObject()->classInfo( "CoClass" );
	    qWarning( "QAxBase::querySubObject: %s: error calling function or property in %s (%s)"
		, (const char*)name, control().latin1(), coclass ? coclass: "unknown" );
	}
#endif
	break;
    default:
#ifdef QT_CHECK_STATE
	{
	    const char *coclass = metaObject()->classInfo( "CoClass" );
	    qWarning( "QAxBase::querySubObject: %s: method or property is not of interface type in %s (%s)"
		, (const char*)name, control().latin1(), coclass ? coclass: "unknown" );
	}
#endif
	break;
    }

    return object;
}



class QtPropertyBag : public IPropertyBag
{
public:
    QtPropertyBag() :ref( 0 ) {}

    HRESULT __stdcall QueryInterface( REFIID iid, LPVOID *iface )
    {
	*iface = 0;
	if ( iid == IID_IUnknown )
	    *iface = this;
	else if ( iid == IID_IPropertyBag )
	    *iface = this;
	else
	    return E_NOINTERFACE;

	AddRef();
	return S_OK;
    }
    unsigned long __stdcall AddRef() { return ++ref; }
    unsigned long __stdcall Release() 
    {
	if ( !--ref ) {
	    delete this;
	    return 0;
	}
	return ref;
    }

    HRESULT __stdcall Read( LPCOLESTR name, VARIANT *var, IErrorLog * )
    {
	if ( !var )
	    return E_POINTER;

	QCString property = BSTRToQString((TCHAR*)name).local8Bit();
	QVariant qvar = map[property];
	*var = QVariantToVARIANT( qvar );
	return S_OK;
    }
    HRESULT __stdcall Write( LPCOLESTR name, VARIANT *var )
    {
	if ( !var )
	    return E_POINTER;
	QCString property = BSTRToQString((TCHAR*)name).local8Bit();
	QVariant qvar = VARIANTToQVariant( *var );
	map[property] = qvar;

	return S_OK;
    }

    QAxBase::PropertyBag map;

private:
    unsigned long ref;
};

/*!
    Returns the values of all properties exposed by the COM object.
    
    This is more efficient than getting multiple properties individually if the COM object
    supports property bags. 
    
    \warning
    It is not guaranteed that the property bag implementation of the COM object returns all 
    properties, or that the properties returned are the same as available through the 
    IDispatch interface.
*/
QAxBase::PropertyBag QAxBase::propertyBag() const
{
    PropertyBag result;
    if ( isNull() )
	return result;
    CComPtr<IPersistPropertyBag> persist;
    d->ptr->QueryInterface( IID_IPersistPropertyBag, (void**)&persist );
    if ( persist ) {
	QtPropertyBag *pbag = new QtPropertyBag();
	pbag->AddRef();
	persist->Save( pbag, FALSE, TRUE );
	result = pbag->map;
	pbag->Release();
	return result;
    } else {
	QAxBase *that = (QAxBase*)this;
	for ( int p = 1; p < metaObject()->numProperties( FALSE ); ++p ) {
	    QVariant var;
	    that->qt_property( p+metaObject()->propertyOffset(), 1, &var );
	    result.insert( metaObject()->property( p, FALSE )->name(), var );
	}
    }
    return result;
}

/*!
    Sets the properties of the COM object to the matching values in \a bag.

    \warning
    You should only set property bags that have been returned before by the
    propertyBag function, as it cannot be guaranteed that the property bag
    implementation of the COM object supports the same properties as available 
    through the IDispatch interface.

    \sa propertyBag()
*/
void QAxBase::setPropertyBag( const PropertyBag &bag )
{
    if ( isNull() )
	return;
    CComPtr<IPersistPropertyBag> persist;
    d->ptr->QueryInterface( IID_IPersistPropertyBag, (void**)&persist );
    if ( persist ) {
	QtPropertyBag *pbag = new QtPropertyBag();
	pbag->map = bag;
	pbag->AddRef();
	persist->Load( pbag, 0 );
	pbag->Release();
    } else {
	QAxBase *that = (QAxBase*)this;
	for ( int p = 1; p < metaObject()->numProperties( FALSE ); ++p ) {
	    QVariant var = bag[metaObject()->property( p, FALSE )->name()];
	    that->qt_property( p+metaObject()->propertyOffset(), 0, &var );
	}
    }
}

/*!
    Returns TRUE if the property \a prop is allowed to change,
    otherwise returns FALSE.
    By default, all properties are allowed to change. 
    
    \warning
    Depending on the control implementation this setting might be ignored
    for some properties.

    \sa setPropertyWritable(), propertyChanged()
*/
bool QAxBase::propertyWritable( const char *prop ) const
{
    if ( !d->propWritable )
	return TRUE;

    if ( !d->propWritable->contains( prop ) )
	return TRUE;
    else
	return (*d->propWritable)[prop];
}

/*!
    Sets the property \a prop to writable if \a ok is TRUE,
    otherwise sets \a prop to be read-only.
    By default, all properties are allowed to change. 
    
    \warning
    Depending on the control implementation this setting might be ignored
    for some properties.

    \sa propertyWritable(), propertyChanged()
*/
void QAxBase::setPropertyWritable( const char *prop, bool ok )
{
    if ( !d->propWritable )
	d->propWritable = new QMap<QCString, bool>;

    (*d->propWritable)[prop] = ok;
}

/*!
    Returns TRUE if there is no COM object loaded by this wrapper, otherwise return FALSE.

    \sa control
*/
bool QAxBase::isNull() const
{ 
    return !d->ptr; 
}


/*!
    \fn bool QAxBase::qt_emit( int, QUObject* );
    \internal 
*/

/*!
    \fn const char *QAxBase::className() const
    \internal
*/

/*!
    \fn QObject *QAxBase::qObject()
    \internal
*/

/*!
    \fn void QAxBase::signal( const QString &name, int argc, void *argv )

    This generic signal gets emitted when the COM object issues the event \a name. \a argc
    is the number of parameters provided by the event (DISPPARAMS.cArgs), and \a argv is 
    the pointer to the parameter values (DISPPARAMS.rgvarg).
    
    Use this signal if the event has parameters of unsupported data types. Otherwise, connect
    directly to the signal \a name.
*/

/*!
    \fn void QAxBase::propertyChanged( const QString &name )

    If the COM object supports property notification, this signal gets emitted when the property
    \a name is changed.
*/
