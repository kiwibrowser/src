// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// MeasurePageLoadTimeBHO.h : Declaration of the CMeasurePageLoadTimeBHO

#include "resource.h"       // main symbols

#include <shlguid.h>     // IID_IWebBrowser2, DIID_DWebBrowserEvents2, et
#include <exdispid.h>    // DISPID_DOCUMENTCOMPLETE, etc.

#include <string>

#include "MeasurePageLoadTime.h"


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM objects and allow use of its single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif



// CMeasurePageLoadTimeBHO

class ATL_NO_VTABLE CMeasurePageLoadTimeBHO :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMeasurePageLoadTimeBHO, &CLSID_MeasurePageLoadTimeBHO>,
	public IObjectWithSiteImpl<CMeasurePageLoadTimeBHO>,
	public IDispatchImpl<IMeasurePageLoadTimeBHO, &IID_IMeasurePageLoadTimeBHO, &LIBID_MeasurePageLoadTimeLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
  public IDispEventImpl<1, CMeasurePageLoadTimeBHO, &DIID_DWebBrowserEvents2, &LIBID_SHDocVw, 1, 1>
{
public:
	CMeasurePageLoadTimeBHO()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_MEASUREPAGELOADTIMEBHO)

DECLARE_NOT_AGGREGATABLE(CMeasurePageLoadTimeBHO)

BEGIN_COM_MAP(CMeasurePageLoadTimeBHO)
	COM_INTERFACE_ENTRY(IMeasurePageLoadTimeBHO)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP()

BEGIN_SINK_MAP(CMeasurePageLoadTimeBHO)
    SINK_ENTRY_EX(1, DIID_DWebBrowserEvents2, DISPID_DOCUMENTCOMPLETE, OnDocumentComplete)
END_SINK_MAP()

    // DWebBrowserEvents2
  void STDMETHODCALLTYPE OnDocumentComplete(IDispatch *pDisp, VARIANT *pvarURL);
  STDMETHOD(SetSite)(IUnknown *pUnkSite);

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

  void ProcessPageTimeRequests(void);
  void VisitNextURL(void);
  void ErrorExit(void);

private:
    CComPtr<IWebBrowser2>  m_spWebBrowser;
    BOOL m_fAdvised;

    // Handle to global interface table
    DWORD m_dwCookie;

    // Handle to event to signal when navigation completes
    HANDLE m_hEvent;

    // Socket for accepting incoming connections
    SOCKET m_sockListen;

    // Socket for communicating with remote peers
    SOCKET m_sockTransport;
};

OBJECT_ENTRY_AUTO(__uuidof(MeasurePageLoadTimeBHO), CMeasurePageLoadTimeBHO)
