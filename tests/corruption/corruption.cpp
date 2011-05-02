// corruption.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Tests.h"
// This hooks vld into this app
#include "../../vld.h"

int _tmain(int argc, _TCHAR* argv[])
{
	wprintf(_T("======================================\n"));
	wprintf(_T("==\n"));
	wprintf(_T("==    VLD Tests: Memory Corruption\n"));
	wprintf(_T("==\n"));
	wprintf(_T("======================================\n"));

	UINT vld_options = VLDGetOptions();
	vld_options |= VLD_OPT_VALIDATE_HEAPFREE;
	VLDSetOptions(vld_options, 15, 25);

	if (argc == 2)
	{
		// Pick up options to determine which type of test to execute
		if (_tcsicmp(_T("allocmismatch"), argv[1]) == 0)
		{
			TestAllocationMismatch_malloc_delete();
			TestAllocationMismatch_malloc_deleteVec();
			TestAllocationMismatch_new_free();
			TestAllocationMismatch_newVec_free();
		}
		else if (_tcsicmp(_T("heapmismatch"), argv[1]) == 0)
		{
			TestHeapMismatch();
		}
	}

	return 0;
}

