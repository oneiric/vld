// dynamic_app.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "LoadTests.h"
#include "ThreadTests.h"
#include "vld.h"

#include <gtest/gtest.h>

// Leaks 6 memory allocations
void LeakDuplicateLeaks()
{
    // For testing aggregation
    for (int i = 0; i < 3; ++i)
    {
        int* tmp = new int(0x63);
        tmp;
    }
    for (int i = 0; i < 3; ++i)
    {
        int* tmp = new int(0x63);
        tmp;
    }
    // Should report 6 memory leaks
}

class DynamicLoader : public ::testing::TestWithParam<bool> {
    virtual void SetUp()
    {
        VLDMarkAllLeaksAsReported();
    }
    virtual void TearDown()
    {
        // Check that callstack resolved without unresolved functions (required symbols for all dll's)
        //EXPECT_EQ(0, VLDResolveCallstacks());
    }
};


TEST_P(DynamicLoader, LoaderTests)
{
    HMODULE hdynLib = LoadDynamicTests();
    ASSERT_NE(0u, reinterpret_cast<UINT_PTR>(hdynLib));
#ifdef STATIC_CRT // Toolset <= v100 use own heap
    VLDMarkAllLeaksAsReported();
    RunLoaderTests(hdynLib, GetParam());    // leaks 18
    int leaks = static_cast<int>(VLDGetLeaksCount());
    FreeLibrary(hdynLib);
#else
    //VLDMarkAllLeaksAsReported();
    RunLoaderTests(hdynLib, GetParam());    // leaks 18
    FreeLibrary(hdynLib);
    int leaks = static_cast<int>(VLDGetLeaksCount());
#endif
    if (18 != leaks) VLDReportLeaks();
    ASSERT_EQ(18, leaks);
}

TEST_P(DynamicLoader, MultithreadLoadingTests)
{
    // Creates NUMTHREADS threads that each leaks 18 allocations
    DWORD start = GetTickCount();
    RunLoaderLockTests(GetParam(), false);
    DWORD duration = GetTickCount() - start;
    _tprintf(_T("Thread Test took: %u ms\n"), duration);
    start = GetTickCount();
    int leaks = (int)VLDGetLeaksCount();
    duration = GetTickCount() - start;
    _tprintf(_T("VLDGetLeaksCount took: %u ms\n"), duration);
    int correctLeaks = NUMTHREADS * 18;
    if (correctLeaks != leaks) VLDReportLeaks();
#ifdef STATIC_CRT // One leak from dll static allocation
    HMODULE hlib = LoadDynamicTests();
    for (int i = 0; i < NUMTHREADS + 1; i++)
        FreeLibrary(hlib);
#endif
    ASSERT_EQ(correctLeaks, leaks);
}

TEST_P(DynamicLoader, MfcLoaderTests)
{
    HMODULE hmfcLib = LoadMFCTests();
    ASSERT_NE(0u, reinterpret_cast<UINT_PTR>(hmfcLib));
#ifdef STATIC_CRT // Toolset <= v100 use own heap
    VLDMarkAllLeaksAsReported();
    RunMFCLoaderTests(hmfcLib, GetParam()); // leaks 11
    int leaks = (int)VLDGetLeaksCount();
    FreeLibrary(hmfcLib);
#else
    //VLDMarkAllLeaksAsReported();
    RunMFCLoaderTests(hmfcLib, GetParam()); // leaks 11
    FreeLibrary(hmfcLib);
    int leaks = (int)VLDGetLeaksCount();
#endif
    if (11 != leaks) VLDReportLeaks();
    ASSERT_EQ(11, leaks);
}

TEST_P(DynamicLoader, MfcMultithreadLoadingTests)
{
    // Creates NUMTHREADS threads that each leaks 11 allocations
    DWORD start = GetTickCount();
    RunLoaderLockTests(GetParam(), true);
    DWORD duration = GetTickCount() - start;
    _tprintf(_T("Thread Test took: %u ms\n"), duration);
    start = GetTickCount();
    int leaks = (int)VLDGetLeaksCount();
    duration = GetTickCount() - start;
    _tprintf(_T("VLDGetLeaksCount took: %u ms\n"), duration);
    if (NUMTHREADS * 11 != leaks) VLDReportLeaks();
#ifdef STATIC_CRT // One leak from dll static allocation
    HMODULE hlib = LoadMFCTests();
    for (int i = 0; i < NUMTHREADS + 1; i++)
        FreeLibrary(hlib);
#endif
    ASSERT_EQ(NUMTHREADS * 11, leaks);
}

INSTANTIATE_TEST_CASE_P(ResolveVal,
    DynamicLoader,
    ::testing::Bool());

TEST(Dynamic, DuplicateLeaks)
{
    VLDMarkAllLeaksAsReported();
    LeakDuplicateLeaks();       // leaks 6
    int leaks = (int)VLDGetLeaksCount();
    ASSERT_EQ(6, leaks);
}

int __cdecl ReportHook(int /*reportHook*/, wchar_t *message, int* /*returnValue*/)
{
    OutputDebugString(message);
    return 0;
}

int main(int argc, char **argv) {
    VLDSetReportHook(VLD_RPTHOOK_INSTALL, ReportHook);
    ::testing::InitGoogleTest(&argc, argv);
    int res = RUN_ALL_TESTS();
    VLDMarkAllLeaksAsReported();
    VLDSetReportHook(VLD_RPTHOOK_REMOVE, ReportHook);
    return res;
}
