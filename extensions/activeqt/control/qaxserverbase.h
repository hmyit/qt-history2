/****************************************************************************
** $Id: $
**
** Declaration of the QAxServerBase class
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

#ifndef QAXSERVERBASE_H
#define QAXSERVERBASE_H

#include <qmetaobject.h>
#include <qmap.h>
#include <quuid.h>
#include <qobject.h>
#include <qintdict.h>
#include "qaxfactory.h"

#include <atlbase.h>
class CExeModule : public CComModule
{
public:
    LONG Unlock();
    DWORD dwThreadID;
    HANDLE hEventShutdown;
    void MonitorShutdown();
    bool StartMonitor();
    bool bActivity;

    static QAxFactoryInterface *factory();
    static QInterfacePtr<QAxFactoryInterface> _factory;
};

extern CExeModule _Module;
#include <atlcom.h>
#include <atlctl.h>

extern GUID IID_IAxServerBase;

struct IAxServerBase : public IUnknown
{
    virtual QObject *qObject() = 0;
    virtual QWidget *widget() = 0;
};

class QWidget;
class QAxPropertyPage;

/////////////////////////////////////////////////////////////////////////////
// QAxServerBase
class QAxServerBase : 
    public QObject,
    public IAxServerBase,
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatch,
    public CComControl<QAxServerBase>,
    public IOleControlImpl<QAxServerBase>,
    public IOleObjectImpl<QAxServerBase>,
    public IViewObjectExImpl<QAxServerBase>,
    public IOleInPlaceObjectWindowlessImpl<QAxServerBase>,
    public IOleInPlaceActiveObjectImpl<QAxServerBase>,
    public IProvideClassInfo2,
    public IConnectionPointContainer,
    public IPersistPropertyBag,
    public ISpecifyPropertyPages,
    public IPropertyPage2
{
public:
    typedef QMap<QUuid,IConnectionPoint*> ConnectionPoints;
    typedef QMap<QUuid,IConnectionPoint*>::Iterator ConnectionPointsIterator;

    QAxServerBase( const QString &classname );

    ~QAxServerBase();

BEGIN_COM_MAP(QAxServerBase)
    COM_INTERFACE_ENTRY(IDispatch)
    {&IID_IAxServerBase,
    offsetofclass(IAxServerBase, _ComMapClass),
    _ATL_SIMPLEMAPENTRY},
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY_IMPL(IOleControl)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY(IPersistPropertyBag)
    COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
    COM_INTERFACE_ENTRY(IPropertyPage)
    COM_INTERFACE_ENTRY(IPropertyPage2)
{NULL, 0, 0}}; return _entries;}


    unsigned long WINAPI AddRef()
    {
	return ++ref;
    }
    unsigned long WINAPI Release()
    {
	if ( !--ref ) {
	    delete this;
	    return 0;
	}
	return ref;
    }
    HRESULT WINAPI QueryInterface( REFIID iid, void **iface );

BEGIN_MSG_MAP(QAxServerBase)
    CHAIN_MSG_MAP(CComControl<QAxServerBase>)
    DEFAULT_REFLECTION_HANDLER()
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_SHOWWINDOW, ForwardMessage )
    MESSAGE_HANDLER(WM_PAINT, ForwardMessage )
    MESSAGE_HANDLER(WM_SIZE, ForwardMessage )
    MESSAGE_HANDLER(WM_ACTIVATE, ForwardMessage)
    MESSAGE_HANDLER(WM_KEYUP, ForwardMessage)
    MESSAGE_HANDLER(WM_KEYDOWN, ForwardMessage)
    MESSAGE_HANDLER(WM_CHAR, ForwardMessage)
    MESSAGE_HANDLER(WM_SETFOCUS, ForwardMessage )
    MESSAGE_HANDLER(WM_KILLFOCUS, ForwardMessage )
    MESSAGE_HANDLER(WM_ACTIVATE, ForwardMessage )
END_MSG_MAP()

// IAxServerBase
    QObject *qObject()
    {
        return this;
    }
    QWidget *widget()
    {
	return activeqt;
    }

// IViewObjectEx
    DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// IDispatch
    CComTypeInfoHolder *_tih;

    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
    {
	    *pctinfo = 1;
	    return S_OK;
    }
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
    {
	    return _tih->GetTypeInfo(itinfo, lcid, pptinfo);
    }
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
	    LCID lcid, DISPID* rgdispid)
    {
	    return _tih->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    }

    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
		LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
		EXCEPINFO* pexcepinfo, UINT* puArgErr);

// IProvideClassInfo2
    CComTypeInfoHolder *_tih2;

    STDMETHOD(GetClassInfo)(ITypeInfo** pptinfo)
    {
	return _tih2->GetTypeInfo(0, LANG_NEUTRAL, pptinfo);
    }

    STDMETHOD(GetGUID)(DWORD dwGuidKind, GUID* pGUID)
    {
	if (pGUID == NULL)
	    return E_POINTER;
	
	if ( dwGuidKind == GUIDKIND_DEFAULT_SOURCE_DISP_IID )
	{
	    *pGUID = _Module.factory()->eventsID( class_name );
	    return S_OK;
	}
	*pGUID = GUID_NULL;
	return E_FAIL;
    }


// IOleControl
    STDMETHOD(OnAmbientPropertyChange)(DISPID);

// IOleObject
    STDMETHOD(GetUserType)(DWORD dwFormOfType, LPOLESTR *pszUserType);
    STDMETHOD(GetMiscStatus)(DWORD dwAspect, DWORD *pdwStatus);

// IConnectionPointContainer
    STDMETHOD(EnumConnectionPoints)(IEnumConnectionPoints**);
    STDMETHOD(FindConnectionPoint)(REFIID, IConnectionPoint**);

// IPersist
    STDMETHOD(GetClassID)(GUID*clsid) 
    {
	*clsid = _Module.factory()->classID( class_name );
	return S_OK;
    }

// IPersistPropertyBag
    STDMETHOD(InitNew)(VOID);
    STDMETHOD(Load)(IPropertyBag *, IErrorLog *);
    STDMETHOD(Save)(IPropertyBag *, BOOL, BOOL);

// IPersistStorage
    STDMETHOD(IsDirty)(VOID);

// ISpecifyPropertyPages
    STDMETHOD(GetPages)( CAUUID *pPages );

// IPropertyPage
    STDMETHOD(SetPageSite)( IPropertyPageSite *pPageSite );
    STDMETHOD(Activate)( HWND hWndParent, LPCRECT pRect, BOOL bModal );
    STDMETHOD(Deactivate)();
    STDMETHOD(GetPageInfo)( PROPPAGEINFO *pPageInfo );
    STDMETHOD(SetObjects)( ULONG cObjects, IUnknown **ppUnk );
    STDMETHOD(Show)( UINT nCmdShow );
    STDMETHOD(Move)( LPCRECT pRect );
    STDMETHOD(IsPageDirty)();
    STDMETHOD(Apply)();
    STDMETHOD(Help)( LPCOLESTR pszHelpDir );
    STDMETHOD(TranslateAccelerator)( MSG *pMsg );

// IPropertyPage2
    STDMETHOD(EditProperty)( DISPID prop );

/* IPersistStorage
    STDMETHOD(InitNew)(IStorage *pStg ) { return E_NOTIMPL; }
    STDMETHOD(Load)(IStorage *pStg ) { return E_NOTIMPL; }
    STDMETHOD(Save)(IStorage *pStg, BOOL fSameAsLoad ) { return E_NOTIMPL; }
    STDMETHOD(SaveCompleted)( IStorage *pStgNew ) { return E_NOTIMPL; }
    STDMETHOD(HandsOffStorage)() { return E_NOTIMPL; }
*/

    bool qt_emit( int, QUObject* );
    void emitPropertyChanged( DISPID dispId );
    bool emitRequestPropertyChange( DISPID dispId );

    void readMetaData();

    static QPtrList<CComTypeInfoHolder> *typeInfoHolderList;

    // required for IOleObjectImpl or whatever to compile
    static const CLSID& WINAPI GetObjectCLSID()
    {
	return CLSID_NULL;
    }

    bool eventFilter( QObject *o, QEvent *e );
private:
    bool internalCreate();

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT ForwardMessage( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );

    friend class QAxBindable;
    friend class QAxPropertyPage;

    QWidget* activeqt;
    ConnectionPoints points;

    uint initNewCalled :1;
    uint dirtyflag :1;
    uint hasStockEvents :1;

    unsigned long ref;

    QString class_name;

    QIntDict<QMetaData>* slotlist;
    QMap<int,DISPID>* signallist;
    QIntDict<QMetaProperty>* proplist;
    QMap<int, DISPID>* proplist2;

    IPropertyPageSite *propPageSite;
    QAxPropertyPage *propPage;
    QPtrList<IAxServerBase> propObjects;
};

#endif //QAXSERVERBASE_H
