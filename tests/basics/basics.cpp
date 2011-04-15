// basics.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../../vld.h"
#include "Allocs.h"
#include <vector>


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
	wprintf(_T("VLD Tests: basics\n"));
	wprintf(_T("======================================\n"));
	if (argc == 3)
	{
		LeakOption leak_type = eMalloc;
		wprintf(_T("Memory leaks will be created \n"));
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

		int repeat = _tstoi(argv[2]);
		LeakMemory(leak_type,repeat);
	} 
	else
	{
		wprintf(_T("No memory leaks will be created \n"));
	}
	return 0;
}

