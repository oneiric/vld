// basics.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../../vld.h"
#include "Allocs.h"

void PrintUsage() 
{
	wprintf(_T("Usage:\n"));
	wprintf(_T("\ttest_basics <type> <repeat>\n"));
	wprintf(_T("\t<type>   - The type of memory allocation to test with. This can be one of the following:\n"));
	wprintf(_T("\t           [malloc,new,new_array,calloc,realloc]\n"));
	wprintf(_T("\t<repeat> - The number of times to repeat each unique memory leak.\n\n"));
}

void LeakMemory(LeakOption type, int repeat)
{
	for (int i = 0; i < repeat; i++)
	{
		Alloc(type);
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	wprintf(_T("======================================\n"));
	wprintf(_T("==\n"));
	wprintf(_T("==    VLD Tests: basics\n"));
	wprintf(_T("==\n"));
	wprintf(_T("======================================\n"));
	if (argc == 3)
	{
		LeakOption leak_type = eMalloc; // default
		
		// Pick up options to determine which type of memory allocator
		// to test with
		if (_tcsicmp(_T("malloc"), argv[1]) == 0)
		{
			leak_type = eMalloc;
		}
		else if (_tcsicmp(_T("new"), argv[1]) == 0)
		{
			leak_type = eNew;
		}
		else if (_tcsicmp(_T("new_array"), argv[1]) == 0)
		{
			leak_type = eNewArray;
		}
		else if (_tcsicmp(_T("calloc"), argv[1]) == 0)
		{
			leak_type = eCalloc;
		}
		else if (_tcsicmp(_T("realloc"), argv[1]) == 0)
		{
			leak_type = eRealloc;
		}
		else if (_tcsicmp(_T("CoTaskMem"), argv[1]) == 0)
		{
			leak_type = eCoTaskMem;
		}
		else
		{
			wprintf(_T("Error!: Invalid arguments\n"));
			PrintUsage();
			return -1;
		}

		wprintf(_T("Options: %s \nNumber of Leaks: %s\n"), argv[1], argv[2]);
		// Convert the string into it's integer equivalent
		int repeat = _tstoi(argv[2]);
		LeakMemory(leak_type,repeat);
	} 
	else
	{
		wprintf(_T("Error!: Invalid arguments\n"));
		PrintUsage();
	}
	wprintf(_T("End of test app...\n\n"));
	return 0;
}

