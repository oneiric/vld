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
    HMODULE hmfcLib = RunLoaderTests(GetParam());    // leaks 18
    ASSERT_NE(0u, (UINT_PTR)hmfcLib);
    int leaks = (int)VLDGetLeaksCount();
    if (18 != leaks) VLDReportLeaks();
    ASSERT_EQ(18, leaks);
}

TEST_P(DynamicLoader, MultithreadLoadingTests)
{
    // Creates 64 threads that each leaks 18 allocations
    DWORD start = GetTickCount();
    RunLoaderLockTests(GetParam(), false);
    DWORD duration = GetTickCount() - start;
    _tprintf(_T("Thread Test took: %u ms\n"), duration);
    start = GetTickCount();
    int leaks = (int)VLDGetLeaksCount();
    duration = GetTickCount() - start;
    _tprintf(_T("VLDGetLeaksCount took: %u ms\n"), duration);
    if (64 * 18 != leaks) VLDReportLeaks();
    ASSERT_EQ(64 * 18, leaks);
}

TEST_P(DynamicLoader, MfcLoaderTests)
{
    HMODULE hmfcLib = RunMFCLoaderTests(GetParam()); // leaks 11
    ASSERT_NE(0u, (UINT_PTR)hmfcLib);
    FreeLibrary(hmfcLib);
    int leaks = (int)VLDGetLeaksCount();
    if (11 != leaks) VLDReportLeaks();
    ASSERT_EQ(11, leaks);
}

TEST_P(DynamicLoader, MfcMultithreadLoadingTests)
{
    // Creates 64 threads that each leaks 11 allocations
    DWORD start = GetTickCount();
    RunLoaderLockTests(GetParam(), true);
    DWORD duration = GetTickCount() - start;
    _tprintf(_T("Thread Test took: %u ms\n"), duration);
    start = GetTickCount();
    int leaks = (int)VLDGetLeaksCount();
    duration = GetTickCount() - start;
    _tprintf(_T("VLDGetLeaksCount took: %u ms\n"), duration);
    if (64 * 11 != leaks) VLDReportLeaks();
    ASSERT_EQ(64 * 11, leaks);
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
    return 1;
}

int main(int argc, char **argv) {
    VLDSetReportHook(VLD_RPTHOOK_INSTALL, ReportHook);
    ::testing::InitGoogleTest(&argc, argv);
    int res = RUN_ALL_TESTS();
    VLDMarkAllLeaksAsReported();
    VLDSetReportHook(VLD_RPTHOOK_REMOVE, ReportHook);
    return res;
}
