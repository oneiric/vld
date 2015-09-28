// vld_main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <malloc.h>

#define VLD_FORCE_ENABLE
#include <vld.h>


class MemoryLeak {
public:
    MemoryLeak(size_t n) { l = malloc(n); }
    ~MemoryLeak() { free(l); }
private:
    void* l;
};

static void* s_m = malloc(10);
static char* s_n = new char[20];

static MemoryLeak* pml = new MemoryLeak(70); // leaks a new pointer and malloc(70)
static MemoryLeak ml{ 80 }; // *should* be freed and not report as a memory leak

void* g_m = malloc(30);
char* g_n = new char[40];


int _tmain(int argc, _TCHAR* argv[])
{
    void* m = malloc(50);
    char* n = new char[60];

    // At this point VLDGetLeaksCount() and VLDReportLeaks() should report 9 leaks
    // including a leak for ml which has not been freed yet. ml will be freed after
    // _tmain exits but before VLDReportLeaks() is called internally by VLD and
    // therefore correctly report 8 leaks.
    // VLDReportLeaks(); // at this point should report 9 leaks;
    return VLDGetLeaksCount() == 9 ? 0 : -1;
}


int WINAPI _tWinMain(__in HINSTANCE hInstance,
    __in_opt HINSTANCE hPrevInstance,
    __in LPWSTR lpCmdLine,
    __in int nShowCmd)
{
    void* m = malloc(50);
    char* n = new char[60];

    // At this point VLDGetLeaksCount() and VLDReportLeaks() should report 9 leaks
    // including a leak for ml which has not been freed yet. ml will be freed after
    // _tWinMain exits but before VLDReportLeaks() is called internally by VLD and
    // therefore correctly report 8 leaks.
    // VLDReportLeaks(); // at this point should report 9 leaks;
    return VLDGetLeaksCount() == 9 ? 0 : -1;
}
