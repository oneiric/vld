#include "stdafx.h"
#include "LoadTests.h"
// Hook in Visual Leak Detector to this application
#include "../../vld.h"
#include <assert.h>

#ifdef _WIN64
	static const TCHAR* sVld_dll = _T("vld_x64.dll");
#else
	static const TCHAR* sVld_dll = _T("vld_x86.dll");
#endif

void CallVLDExportedMethod(const CHAR* function)
{
	HMODULE vld_module =  GetModuleHandle(sVld_dll);
	assert(vld_module);
	typedef void (*VLDAPI_func)();
	if (vld_module != NULL)
	{
		VLDAPI_func func = (VLDAPI_func)GetProcAddress(vld_module, function);
		assert(func);
		if (func)
		{
			func();
		}
	}
}

void CallDynamicMethods(const CHAR* function) 
{
	HMODULE dynamic_module =  GetModuleHandle(_T("dynamic.dll"));
	assert(dynamic_module);
	typedef int (__cdecl *DYNAPI_FNC)();
	if (dynamic_module != NULL)
	{
		DYNAPI_FNC func = (DYNAPI_FNC)GetProcAddress(dynamic_module, function );
		//GetFormattedMessage(GetLastError());
		assert(func);
		int result = -1;
		if (func)
		{
			result = func();
		}
		assert(42 == result);
	}
}

void RunLoaderTests( bool resolve ) 
{
	HMODULE hdyn = LoadLibrary(_T("dynamic.dll"));
	assert(hdyn);
	if (hdyn)
	{
		// Now leak some memory
		CallDynamicMethods("SimpleLeak_New"); // This requires ansi, not Unicode strings

		if (resolve)
		{
			CallVLDExportedMethod("VLDResolveCallstacks"); // This requires ansi, not Unicode strings
		}

		FreeLibrary(hdyn);
	}
}

void CallLibraryMethods( HMODULE hmfcLib, LPCSTR function ) 
{
	HMODULE dynamic_module = hmfcLib;
	assert(dynamic_module);
	typedef void (__cdecl *DYNAPI_FNC)();
	if (dynamic_module != NULL)
	{
		DYNAPI_FNC func = (DYNAPI_FNC)GetProcAddress(dynamic_module, function );
		//GetFormattedMessage(GetLastError());
		assert(func);
		if (func)
		{
			func();
		}
	}
}

void RunMFCLoaderTests()
{
	HMODULE hmfcLib = LoadLibrary(_T("test_mfc.dll"));
	assert(hmfcLib);
	if (hmfcLib)
	{
		// Now leak some memory
		CallLibraryMethods(hmfcLib, "MFC_LeakSimple"); // This requires ansi, not Unicode strings
		CallLibraryMethods(hmfcLib, "MFC_LeakArray");

		//CallVLDExportedMethod("VLDResolveCallstacks"); // This requires ansi, not Unicode strings
		
		FreeLibrary(hmfcLib);
	}
}

