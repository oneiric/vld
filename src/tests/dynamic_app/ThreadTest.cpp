#include "stdafx.h"
#include "ThreadTests.h"
#include "LoadTests.h"
#include <process.h>

struct ThreadData
{
	bool resolve;
	bool mfc;
};

void RunPSApiLoaderTests()
{
    HMODULE hModule = ::LoadLibrary(_T("psapi.dll"));

    if (NULL != hModule)
    {
        ::FreeLibrary(hModule);
    }
}

void Call_LoaderLocks(const ThreadData& threadData)
{
	RunPSApiLoaderTests(); // will not crash
	HMODULE hlib;
    if (threadData.mfc)
    {
        hlib = LoadMFCTests();
        RunMFCLoaderTests(hlib, threadData.resolve); // Leaks 11 allocs
    }
    else
    {
        hlib = LoadDynamicTests();
        RunLoaderTests(hlib, threadData.resolve); // Leaks 18 allocs
    }
#ifndef STATIC_CRT
    FreeLibrary(hlib);
#else
    UNREFERENCED_PARAMETER(hlib);
#endif

    HMODULE this_app = NULL;
    WCHAR path_name[MAX_PATH] = {0};
    // This also acquires the loader lock
    GetModuleFileName(this_app, path_name, MAX_PATH);
}

void Call_Three(const ThreadData& threadData)
{
    Call_LoaderLocks(threadData);
}

void Call_Two(const ThreadData& threadData)
{
    Call_Three(threadData);
}

void Call_One(const ThreadData& threadData)
{
    Call_Two(threadData);
}

unsigned __stdcall Dynamic_Thread_Procedure(LPVOID foo)
{
	ThreadData& threadData = *(ThreadData*)(foo);
	Call_One(threadData);
    return 0;
}

void RunLoaderLockTests(bool resolve, bool mfc)
{
    HANDLE threads[NUMTHREADS] = {0};
    unsigned thread_id = NULL;
	ThreadData threadData;
	threadData.resolve = resolve;
	threadData.mfc = mfc;

    for (UINT i = 0; i < NUMTHREADS; ++i)
    {
        threads[i] = (HANDLE)_beginthreadex(NULL, // security attribute
            0,                          // stack size
            Dynamic_Thread_Procedure,   // start function
            &threadData,                // thread parameters
            0,                          // creation flags
            &thread_id);                // thread id
    }

    BOOL wait_for_all = TRUE;
    DWORD result = WaitForMultipleObjects(NUMTHREADS, threads, wait_for_all, INFINITE);
    switch (result)
    {
    case WAIT_OBJECT_0:
        _tprintf(_T("All threads finished correctly.\n"));
        break;
    case WAIT_ABANDONED_0:
        _tprintf(_T("Abandoned mutex.\n"));
        break;
    case WAIT_TIMEOUT:
        _tprintf(_T("All threads timed out\n"));
        break;
    case WAIT_FAILED:
        {
            _tprintf(_T("Function call to Wait failed with unknown error\n"));
            TCHAR lpMsgBuf[MAX_PATH] = {0};
            FormatMessage(
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                lpMsgBuf,
                MAX_PATH,
                NULL );

            _tprintf(_T("%s"), lpMsgBuf);
        }

        break;
    default:
        _tprintf(_T("Some other return value\n"));
        break;
    }

}
