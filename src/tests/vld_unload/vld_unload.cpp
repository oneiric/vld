// vld_unload.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <Windows.h>
#include <tchar.h>
#include <assert.h>

#ifdef _WIN64
static const TCHAR* sVld_dll = _T("vld_x64.dll");
#else
static const TCHAR* sVld_dll = _T("vld_x86.dll");
#endif

UINT VLDGetLeaksCount()
{
    HMODULE vld_module = GetModuleHandle(sVld_dll);
    if (vld_module != NULL) {
        typedef UINT(*VLDAPI_func)();
        VLDAPI_func func = (VLDAPI_func)GetProcAddress(vld_module, "VLDGetLeaksCount");
        assert(func);
        if (func) {
            return func();
        }
    }
    return -1;
}

HMODULE GetModuleFromAddress(LPCVOID pAddress)
{
    HMODULE hModule = NULL;
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery((LPCVOID)pAddress, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) == sizeof(MEMORY_BASIC_INFORMATION)) {
        // the allocation base is the beginning of a PE file
        hModule = (HMODULE)mbi.AllocationBase;
    }
    return hModule;
}


int _tmain(int argc, _TCHAR* argv[])
{
    int PASSED = 0;
    {
        HMODULE hModule1 = ::LoadLibrary(_T("vld_dll1.dll"));
        int w = VLDGetLeaksCount(); // vld is loaded and counts 1 memory leak
        ::FreeLibrary(hModule1);    // vld is unloaded here and reports the memory leak
        int x = VLDGetLeaksCount(); // vld is unloaded and cannot count any memory leaks

        HMODULE hModule2 = ::LoadLibrary(_T("vld_dll2.dll"));
        int y = VLDGetLeaksCount(); // vld is loaded and counts 1 memory leak
        ::FreeLibrary(hModule2);    // vld is unloaded here and reports the memory leak
        int z = VLDGetLeaksCount(); // vld is unloaded and cannot count any memory leaks
        assert(w == 1 && x == -1 && y == 1 && z == -1);
        if (w == 1 && x == -1 && y == 1 && z == -1) {
            ++PASSED;
        }
    }

    {
        HMODULE hModule3 = ::LoadLibrary(_T("vld_dll1.dll"));
        int w = VLDGetLeaksCount(); // vld is loaded and counts 1 memory leak
        HMODULE hModule4 = ::LoadLibrary(_T("vld_dll2.dll"));
        int x = VLDGetLeaksCount(); // vld is still loaded and counts 2 memory leaks
        ::FreeLibrary(hModule4);    // vld is *not* unloaded here
        int y = VLDGetLeaksCount(); // vld is still loaded and counts 2 memory leaks
        ::FreeLibrary(hModule3);    // vld is unloaded here and reports 2 memory leaks
        int z = VLDGetLeaksCount(); // vld is unloaded and cannot count any memory leaks
        assert(w == 1 && x == 2 && y == 2 && z == -1);
        if (w == 1 && x == 2 && y == 2 && z == -1) {
            ++PASSED;
        }
    }

    {
        HMODULE hModule5 = ::LoadLibrary(_T("vld_dll1.dll"));
        int w = VLDGetLeaksCount(); // vld is loaded and counts 1 memory leak
        HMODULE hModule6 = ::LoadLibrary(_T("vld_dll2.dll"));
        int x = VLDGetLeaksCount(); // vld is still loaded and counts 2 memory leaks
        ::FreeLibrary(hModule5);    // vld is *not* unloaded here
        int y = VLDGetLeaksCount(); // vld is still loaded and counts 2 memory leaks
        ::FreeLibrary(hModule6);    // vld is unloaded here and reports 2 memory leaks
        int z = VLDGetLeaksCount(); // vld is unloaded and cannot count any memory leaks
        assert(w == 1 && x == 2 && y == 2 && z == -1);
        if (w == 1 && x == 2 && y == 2 && z == -1) {
            ++PASSED;
        }
    }

    {
        typedef FARPROC(__stdcall *GetProcAddress_t) (HMODULE, LPCSTR);

        HMODULE kernel32 = GetModuleHandleW(L"KernelBase.dll");
        if (!kernel32) {
            kernel32 = GetModuleHandleW(L"kernel32.dll");
        }

        // pGetProcAddress1 resolves to kernel32!GetProcAddress()
        GetProcAddress_t pGetProcAddress1 = GetProcAddress;

        HMODULE hModule7 = ::LoadLibrary(_T("vld_dll1.dll"));
        int w = VLDGetLeaksCount(); // vld is loaded and counts 1 memory leak

        // pGetProcAddress2 resolves to vld_xXX.dll!VisualLeakDetector::_GetProcAddress()
        GetProcAddress_t pGetProcAddress2 = GetProcAddress;

        ::FreeLibrary(hModule7);    // vld is unloaded here and reports the memory leak
        int x = VLDGetLeaksCount(); // vld is unloaded and cannot count any memory leaks

        //assert(pGetProcAddress1 == pGetProcAddress2);
        GetProcAddress_t pGetProcAddress3 = (GetProcAddress_t)pGetProcAddress1(kernel32, "GetProcAddress");
        
        // Following line raises 0xC0000005: Access violation exception
        //GetProcAddress_t pGetProcAddress4 = (GetProcAddress_t)pGetProcAddress2(kernel32, "GetProcAddress");
    }

    // if PASSED == 3 return exit code 0;
    return !(PASSED == 3);
}