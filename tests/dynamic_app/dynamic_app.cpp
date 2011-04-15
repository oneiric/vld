// dynamic_app.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "LoadTests.h"

void PrintUsage() 
{
	wprintf(_T("Usage:\n"));
	wprintf(_T("\tdynamic [resolve:[true|false]]\n"));
	wprintf(_T("\t<resolve> - [OPTIONAL] Resolves callstacks before unloading the dynamic DLL.\n"));
}

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

int _tmain(int argc, _TCHAR* argv[])
{
	wprintf(_T("======================================\n"));
	wprintf(_T("==\n"));
	wprintf(_T("==    VLD Tests: Dynamic DLL Loading  \n"));
	wprintf(_T("==\n"));
	wprintf(_T("======================================\n"));

	bool resolve = true;
	if (argc == 2)
	{
		resolve = _tcsicmp(_T("true"), argv[1]) == 0;
	} 

	RunLoaderTests(resolve);
	LeakDuplicateLeaks();

	return 0;
}

