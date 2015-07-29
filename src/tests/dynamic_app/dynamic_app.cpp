// dynamic_app.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <assert.h>
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
};


TEST_P(DynamicLoader, LoaderTests)
{
    VLDMarkAllLeaksAsReported();
    int prevleaks = (int)VLDGetLeaksCount();
    HMODULE hmfcLib = RunLoaderTests(GetParam());    // leaks 18
    int totalleaks = (int)VLDGetLeaksCount();
    int leaks = totalleaks - prevleaks;
    VLDReportLeaks();
    ASSERT_EQ(18, leaks);
}

TEST_P(DynamicLoader, MFCLoaderTests)
{
    int prevleaks = (int)VLDGetLeaksCount();
    HMODULE hmfcLib = RunMFCLoaderTests(GetParam()); // leaks 11
#ifndef STATIC_CRT
    FreeLibrary(hmfcLib);
#endif
    int totalleaks = (int)VLDGetLeaksCount();
    int leaks = totalleaks - prevleaks;
    ASSERT_EQ(11, leaks);
#ifdef STATIC_CRT
    FreeLibrary(hmfcLib);
#endif
}

TEST_P(DynamicLoader, Thread)
{
    // Creates 64 threads that each leaks 11 allocations
    int prevleaks = (int)VLDGetLeaksCount();
    RunLoaderLockTests(GetParam());
    int totalleaks = (int)VLDGetLeaksCount();
    int leaks = totalleaks - prevleaks;
    ASSERT_EQ(leaks, 64 * 11);
}

INSTANTIATE_TEST_CASE_P(ResolveVal,
    DynamicLoader,
    ::testing::Bool());

TEST(Dynamic, DuplicateLeaks)
{
    int prevleaks = (int)VLDGetLeaksCount();
    LeakDuplicateLeaks();       // leaks 6
    int totalleaks = (int)VLDGetLeaksCount();
    int leaks = totalleaks - prevleaks;
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
