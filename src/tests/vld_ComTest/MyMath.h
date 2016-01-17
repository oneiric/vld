#pragma once
#include "resource.h"

#include "ComTest_i.h"

using namespace ATL;

// CMyMath

class ATL_NO_VTABLE CMyMath :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMyMath, &CLSID_MyMath>,
	public IMyMath
{
public:
	CMyMath()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_MYMATH)

BEGIN_COM_MAP(CMyMath)
	COM_INTERFACE_ENTRY(IMyMath)
END_COM_MAP()

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

public:
	STDMETHOD(Test)(void);
};

OBJECT_ENTRY_AUTO(__uuidof(MyMath), CMyMath)
