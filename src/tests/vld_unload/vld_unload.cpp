// vld_unload.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <Windows.h>
#include <tchar.h>
#include <assert.h>

#include <gtest/gtest.h>

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


TEST(TestUnloadDlls, Sequence1)
{
    ASSERT_EQ(NULL, GetModuleHandle(sVld_dll));
    HMODULE hModule1 = ::LoadLibrary(_T("vld_dll1.dll"));
    int w = VLDGetLeaksCount(); // vld is loaded and counts 1 memory leak
    EXPECT_EQ(1, w);
    ::FreeLibrary(hModule1);    // vld is unloaded here and reports the memory leak
    int x = VLDGetLeaksCount(); // vld is unloaded and cannot count any memory leaks
    EXPECT_EQ(-1, x);

    HMODULE hModule2 = ::LoadLibrary(_T("vld_dll2.dll"));
    int y = VLDGetLeaksCount(); // vld is loaded and counts 1 memory leak
    EXPECT_EQ(1, y);
    ::FreeLibrary(hModule2);    // vld is unloaded here and reports the memory leak
    int z = VLDGetLeaksCount(); // vld is unloaded and cannot count any memory leaks
    EXPECT_EQ(-1, z);
}

TEST(TestUnloadDlls, Sequence2)
{
    ASSERT_EQ(NULL, GetModuleHandle(sVld_dll));
    HMODULE hModule3 = ::LoadLibrary(_T("vld_dll1.dll"));
    int w = VLDGetLeaksCount(); // vld is loaded and counts 1 memory leak
    EXPECT_EQ(1, w);
    HMODULE hModule4 = ::LoadLibrary(_T("vld_dll2.dll"));
    int x = VLDGetLeaksCount(); // vld is still loaded and counts 2 memory leaks
    EXPECT_EQ(2, x);
    ::FreeLibrary(hModule4);    // vld is *not* unloaded here
    int y = VLDGetLeaksCount();
#if _MSC_VER <= 1600 && !defined(_DLL) // VS 2010 and bellow
    // The reason for reporting 1 leak at this point is that the vld_dll1 module build with <VS2013 calls internally
    // HeapDestroy when it unloads and the leak is either
    // - reported automatically by VLD when the Heap is destroyed if VLD_OPT_SKIP_HEAPFREE_LEAKS was specified or
    // - ignored since the destroyed Heap was removed from VLD heap map.
    EXPECT_EQ(1, y); // vld is still loaded and counts 1 memory leaks
#else
    EXPECT_EQ(2, y); // vld is still loaded and counts 2 memory leaks
#endif
    ::FreeLibrary(hModule3);    // vld is unloaded here and reports 2 memory leaks
    int z = VLDGetLeaksCount(); // vld is unloaded and cannot count any memory leaks
    EXPECT_EQ(-1, z);
}

TEST(TestUnloadDlls, Sequence3)
{
    ASSERT_EQ(NULL, GetModuleHandle(sVld_dll));
    HMODULE hModule5 = ::LoadLibrary(_T("vld_dll1.dll"));
    int w = VLDGetLeaksCount(); // vld is loaded and counts 1 memory leak
    EXPECT_EQ(1, w);
    HMODULE hModule6 = ::LoadLibrary(_T("vld_dll2.dll"));
    int x = VLDGetLeaksCount(); // vld is still loaded and counts 2 memory leaks
    EXPECT_EQ(2, x);
    ::FreeLibrary(hModule5);    // vld is *not* unloaded here
    int y = VLDGetLeaksCount(); // vld is still loaded and counts 2 memory leaks
#if _MSC_VER <= 1600 && !defined(_DLL) // VS 2010 and bellow
    // The reason for reporting 1 leak at this point is that the vld_dll1 module build with <VS2013 calls internally
    // HeapDestroy when it unloads and the leak is either
    // - reported automatically by VLD when the Heap is destroyed if VLD_OPT_SKIP_HEAPFREE_LEAKS was specified or
    // - ignored since the destroyed Heap was removed from VLD heap map.
    EXPECT_EQ(1, y); // vld is still loaded and counts 1 memory leaks
#else
    EXPECT_EQ(2, y); // vld is still loaded and counts 2 memory leaks
#endif
    ::FreeLibrary(hModule6);    // vld is unloaded here and reports 2 memory leaks
    int z = VLDGetLeaksCount(); // vld is unloaded and cannot count any memory leaks
    EXPECT_EQ(-1, z);
}

TEST(TestUnloadDlls, Sequence4)
{
    ASSERT_EQ(NULL, GetModuleHandle(sVld_dll));
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
    //GetProcAddress_t pGetProcAddress3 = (GetProcAddress_t)pGetProcAddress1(kernel32, "GetProcAddress");

    // Following line raises 0xC0000005: Access violation exception
    //GetProcAddress_t pGetProcAddress4 = (GetProcAddress_t)pGetProcAddress2(kernel32, "GetProcAddress");
}

int _tmain(int argc, _TCHAR* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}