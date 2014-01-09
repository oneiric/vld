#include "stdafx.h"
#include <Windows.h>
#include <tchar.h>
#include <process.h>
#include <vld.h>

UINT WINAPI ThreadProc1(LPVOID pParam)
{
	for (int i = 0; i < 10000; i++)
	{
		HMODULE hModule = ::LoadLibrary(_T("wtsapi32.dll"));

		if (NULL != hModule)
		{
			::FreeLibrary(hModule);
			hModule = NULL;
		}
	}

	return 0;
}

UINT WINAPI ThreadProc2(LPVOID pParam)
{
	for (int i = 0; i < 10000; i++)
	{
		HMODULE hModule = ::LoadLibrary(_T("psapi.dll"));

		if (NULL != hModule)
		{
			::FreeLibrary(hModule);
			hModule = NULL;
		}
	}

	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	UINT nThreadID;
	HANDLE hThread[2];

	hThread[0] = (HANDLE) _beginthreadex(NULL, 0, ThreadProc1, NULL, 0, &nThreadID);
	hThread[1] = (HANDLE) _beginthreadex(NULL, 0, ThreadProc2, NULL, 0, &nThreadID);

	::WaitForMultipleObjects(2, hThread, TRUE, INFINITE);

	return 0;
}
