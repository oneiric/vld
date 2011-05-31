// dynamic_app.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <assert.h>
#include "LoadTests.h"
#include "ThreadTests.h"

void PrintUsage() 
{
    wprintf(_T("Usage:\n"));
    wprintf(_T("\tdynamic [resolve:[true|false]]\n"));
    wprintf(_T("\t<resolve> - [OPTIONAL] Resolves callstacks before unloading the dynamic DLL.\n"));
}

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

// VLD internal API
#if defined(_DEBUG) || defined(VLD_FORCE_ENABLE)
extern "C" {
    __declspec(dllimport) SIZE_T VLDGetLeaksCount (BOOL includingInternal = FALSE);
}
#else
#define VLDGetLeaksCount() 0
#endif

int _tmain(int argc, _TCHAR* argv[])
{
    wprintf(_T("======================================\n"));
    wprintf(_T("==\n"));
    wprintf(_T("==    VLD Tests: Dynamic DLL Loading  \n"));
    wprintf(_T("==\n"));
    wprintf(_T("======================================\n"));

    bool resolve = true;
    bool doThreadTests = false;
    if (argc == 2)
    {
        resolve = _tcsicmp(_T("true"), argv[1]) == 0;
    }
    else if (argc == 3)
    {
        resolve = _tcsicmp(_T("true"), argv[1]) == 0;
        doThreadTests = _tcsicmp(_T("thread"), argv[2]) == 0;
    }

    RunLoaderTests(resolve);    // leaks 18
    int totalleaks = (int)VLDGetLeaksCount();
    int leaks1 = totalleaks;
    int prevleaks = totalleaks;
    assert(leaks1 == 18);
 
    RunMFCLoaderTests(resolve); // leaks 7
    totalleaks = (int)VLDGetLeaksCount();
    int leaks2 = totalleaks - prevleaks;
    prevleaks = totalleaks;
    assert(leaks2 == 7);
 
    LeakDuplicateLeaks();       // leaks 6
    totalleaks = (int)VLDGetLeaksCount();
    int leaks3 = totalleaks - prevleaks;
    prevleaks = totalleaks;
    assert(leaks3 == 6);

    if (doThreadTests)
    {
        // Creates 64 threads that each leaks 7 allocations. This equals 448
        RunLoaderLockTests(resolve);
        totalleaks = (int)VLDGetLeaksCount();
        int leaks4 = totalleaks - prevleaks;
        assert(leaks4 == 448);

        // ..................Total:    479 leaks total
        totalleaks = (int)VLDGetLeaksCount();
        int diff = 479 - totalleaks;
        return diff;
    }

    // ..................Total:    31 leaks total
    totalleaks = (int)VLDGetLeaksCount();
    int diff = 31 - totalleaks;
    return diff;
}

