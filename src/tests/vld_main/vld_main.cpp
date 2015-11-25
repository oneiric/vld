// vld_main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <malloc.h>
#include <iostream>

#define VLD_FORCE_ENABLE
#include <vld.h>


class MemoryLeak {
public:
    MemoryLeak(size_t n) { l = malloc(n); memset(l, 0x30 + ((int)n / 10), n); } // 4,5
    ~MemoryLeak() { free(l); }
private:
    void* l;
};

static void* s_m = malloc(10); // 1
static char* s_n = new char[20]; // 2

static MemoryLeak* pml = new MemoryLeak(70); // 3: leaks a new pointer and malloc(70)
#if _MSC_VER > 1700
static MemoryLeak ml{ 80 }; // *should* be freed and not report as a memory leak
#endif

void* g_m = malloc(30); // 6
char* g_n = new char[40]; // 7


int Test()
{
    VLDSetOptions(VLD_OPT_TRACE_INTERNAL_FRAMES | VLD_OPT_SKIP_CRTSTARTUP_LEAKS, 256, 64);

    void* m = malloc(50); // 8
    char* n = new char[60]; // 9

    memset(s_m, '1', 10);
    memset(s_n, '2', 20);
    memset(g_m, '3', 30);
    memset(g_n, '4', 40);
    memset(m,   '5', 50);
    memset(n,   '6', 60);

    // std libary dynamically initializes the objects "cout" and "cerr", which
    // produce false positive leaks in Release_StaticCrt because we doesn't have
    // debug CRT allocation header.
    std::cout << "Test: cout";
    //std::cerr << "Test: cerr";

    // At this point VLDGetLeaksCount() and VLDReportLeaks() should report 9 leaks
    // including a leak for ml which has not been freed yet. ml will be freed after
    // _tmain exits but before VLDReportLeaks() is called internally by VLD and
    // therefore correctly report 8 leaks.
    int leaks = VLDGetLeaksCount();
    VLDReportLeaks(); // at this point should report 9 leaks;
    return leaks;
}

int _tmain(int argc, _TCHAR* argv[])
{
    return Test();
}


int WINAPI _tWinMain(__in HINSTANCE hInstance,
    __in_opt HINSTANCE hPrevInstance,
    __in LPWSTR lpCmdLine,
    __in int nShowCmd)
{
    return Test();
}