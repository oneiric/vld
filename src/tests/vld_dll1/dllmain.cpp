// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <stdlib.h>
#include <assert.h>

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
    )
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        void* leak = malloc(9);
        break;
    }
    case DLL_THREAD_ATTACH: {
        break;
    }
    case DLL_THREAD_DETACH: {
        break;
    }
    case DLL_PROCESS_DETACH: {
        //assert(1 == VLDGetThreadLeaksCount(GetCurrentThreadId()));
        break;
    }
    }
    return TRUE;
}

