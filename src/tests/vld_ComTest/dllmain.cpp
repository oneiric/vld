#include "stdafx.h"
#include "resource.h"
#include "ComTest_i.h"
#include "dllmain.h"
#include "xdlldata.h"

CComTestModule _AtlModule;

class CComTestApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CComTestApp, CWinApp)
END_MESSAGE_MAP()

CComTestApp theApp;

BOOL CComTestApp::InitInstance()
{
#ifdef _MERGE_PROXYSTUB
	if (!PrxDllMain(m_hInstance, DLL_PROCESS_ATTACH, NULL))
		return FALSE;
#endif
	return CWinApp::InitInstance();
}

int CComTestApp::ExitInstance()
{
	return CWinApp::ExitInstance();
}
