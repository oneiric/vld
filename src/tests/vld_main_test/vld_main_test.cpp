// vld_main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <gtest/gtest.h>
#include <string>

std::wstring dir;

TEST(TestWinMain, RunExe)
{
    PROCESS_INFORMATION processInformation = { 0 };
    STARTUPINFO startupInfo = { 0 };
    startupInfo.cb = sizeof(startupInfo);

    std::wstring exe = dir + _T("vld_main.exe");

    // Create the process
    BOOL result = CreateProcess(exe.c_str(), NULL,
        NULL, NULL, FALSE,
        NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
        NULL, NULL, &startupInfo, &processInformation);
    EXPECT_NE(0, result);

    // Successfully created the process.  Wait for it to finish.
    EXPECT_EQ(WAIT_OBJECT_0, WaitForSingleObject(processInformation.hProcess, INFINITE));

    // Get the exit code.
    DWORD exitCode = 0;
    result = GetExitCodeProcess(processInformation.hProcess, &exitCode);
    EXPECT_NE(0, result);

    // Close the handles.
    CloseHandle(processInformation.hProcess);
    CloseHandle(processInformation.hThread);
#if _MSC_VER == 1800 //VC2013

#if !defined(_WIN64) || defined(_DEBUG) || defined(_DLL)
    ASSERT_EQ(9, exitCode);
#else
    // Test doesn't work
#endif

#elif _MSC_VER == 1900 //VC2015

#if !defined(_WIN64) || defined(_DEBUG)
    ASSERT_EQ(9, exitCode);
#else
    // Test doesn't work
#endif

#else
    ASSERT_EQ(9, exitCode);
#endif
}

int _tmain(int argc, _TCHAR* argv[])
{
    dir = argv[0];
    dir.resize(dir.find_last_of(_T("\\")) + 1);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
