#include "stdafx.h"
#include "Allocs.h"

#include <new> // for placement new
#include <crtdbg.h>
#include <ObjBase.h>
#include <assert.h>
#include <tchar.h>

// Name of the debug C Runtime Library DLL on this system
#ifdef _DEBUG
#if _MSC_VER == 1400	// VS 2005
#ifdef _DLL
#define CRTDLLNAME   _T("msvcr80d.dll")
#else
#define CRTDLLNAME   _T("msvcrt.dll")
#endif
#elif _MSC_VER == 1500	// VS 2008
#ifdef _DLL
#define CRTDLLNAME   _T("msvcr90d.dll")
#else
#define CRTDLLNAME   _T("msvcrt.dll")
#endif
#elif _MSC_VER == 1600	// VS 2010
#define CRTDLLNAME   _T("msvcr100d.dll")
#elif _MSC_VER == 1700	// VS 2012
#define CRTDLLNAME   _T("msvcr110d.dll")
#elif _MSC_VER == 1800	// VS 2013
#define CRTDLLNAME   _T("msvcr120d.dll")
#elif _MSC_VER == 1900	// VS 2015
#define CRTDLLNAME   _T("ucrtbased.dll")
#else
#error Unsupported compiler
#endif
#else
#if _MSC_VER == 1400	// VS 2005
#ifdef _DLL
#define CRTDLLNAME   _T("msvcr80.dll")
#else
#define CRTDLLNAME   _T("msvcrt.dll")
#endif
#elif _MSC_VER == 1500	// VS 2008
#ifdef _DLL
#define CRTDLLNAME   _T("msvcr90.dll")
#else
#define CRTDLLNAME   _T("msvcrt.dll")
#endif
#elif _MSC_VER == 1600	// VS 2010
#define CRTDLLNAME   _T("msvcr100.dll")
#elif _MSC_VER == 1700	// VS 2012
#define CRTDLLNAME   _T("msvcr110.dll")
#elif _MSC_VER == 1800	// VS 2013
#define CRTDLLNAME   _T("msvcr120.dll")
#elif _MSC_VER == 1900	// VS 2015
#define CRTDLLNAME   _T("ucrtbase.dll")
#else
#error Unsupported compiler
#endif
#endif

// Vld shouldn't report this leak because we enable Vld after executing DllMain
class MemoryLeak {
public:
    MemoryLeak() { l = malloc(7); memset(l, 0x30 + ((int)7 / 10), 7); }
    ~MemoryLeak() { free(l); }
private:
    void* l;
};

static MemoryLeak ml; 

typedef void* (__cdecl *free_t) (void* mem);
typedef void* (__cdecl *malloc_t) (size_t size);
typedef void (*allocFunc_t) (bool bFree);

static const int recursion = 3;

__declspec(noinline) void allocMalloc(bool bFree)
{
    int* leaked_memory = (int*)malloc(78);
    int* leaked_memory_dbg = (int*)_malloc_dbg(80, _NORMAL_BLOCK, __FILE__, __LINE__);
    if (bFree)
    {
        free(leaked_memory);
        _free_dbg(leaked_memory_dbg, _NORMAL_BLOCK);
    }
}

void allocNew(bool bFree)
{
    int* leaked_memory = (int*)malloc(78);
    int* leaked_memory_dbg = (int*)_malloc_dbg(80, _NORMAL_BLOCK, __FILE__, __LINE__);
    if (bFree)
    {
        free(leaked_memory);
        _free_dbg(leaked_memory_dbg, _NORMAL_BLOCK);
    }
}

void allocNewArray(bool bFree)
{
    int* leaked_memory = new int[3];
    int* leaked_memory_dbg = new (_NORMAL_BLOCK, __FILE__, __LINE__) int[4];

    // placement new operator
    int temp[3];
    void* place = temp;
    float* placed_mem = new (place) float[3]; // doesn't work. Nothing gets patched by vld
    if (bFree)
    {
        delete[] leaked_memory;
        delete[] leaked_memory_dbg;
    }
}

void allocCalloc(bool bFree)
{
    int* leaked_memory = (int*)calloc(47, sizeof(int));
    int* leaked_memory_dbg = (int*)_calloc_dbg(39, sizeof(int), _NORMAL_BLOCK, __FILE__, __LINE__);
    if (bFree)
    {
        free(leaked_memory);
        _free_dbg(leaked_memory_dbg, _NORMAL_BLOCK);
    }
}

void allocRealloc(bool bFree)
{
    int* temp = (int*)malloc(17);
    int* old_leaked_memory = (int*)realloc(temp, 23);
    int* leaked_memory = (int*)_recalloc(old_leaked_memory, 1, 31);
    int* old_leaked_memory_dbg = (int*)malloc(9);
    int* leaked_memory_dbg = (int*)_realloc_dbg(old_leaked_memory_dbg, 21, _NORMAL_BLOCK, __FILE__, __LINE__);
    if (bFree)
    {
        free(leaked_memory);
        _free_dbg(leaked_memory_dbg, _NORMAL_BLOCK);
    }
}

void allocCoTaskMem(bool bFree)
{
    void* leaked = CoTaskMemAlloc(7);
    if (bFree)
    {
        CoTaskMemFree(leaked);
    }
    void* leaked2 = CoTaskMemAlloc(7);
    void* realloced = CoTaskMemRealloc(leaked2, 29);
    if (bFree)
    {
        CoTaskMemFree(realloced);
    }
}

void allocAlignedMalloc(bool bFree)
{
    void* leaked = _aligned_offset_malloc(64, 16, 1);
    int* leaked_memory = (int*)_aligned_malloc(64, 16);
    int* leaked_memory_dbg = (int*)_aligned_malloc_dbg(32, 16, __FILE__, __LINE__);
    if (bFree)
    {
        _aligned_free(leaked);
        _aligned_free(leaked_memory);
        _aligned_free_dbg(leaked_memory_dbg);
    }
}

void allocAlignedRealloc(bool bFree)
{
    void* leaked = _aligned_offset_malloc(64, 16, 1);
    int* leaked_memory = (int*)_aligned_malloc(64, 16);
    int* leaked_memory_dbg = (int*)_aligned_malloc_dbg(32, 16, __FILE__, __LINE__);
    leaked = (int*)_aligned_offset_realloc(leaked, 48, 16, 2);
    leaked_memory = (int*)_aligned_realloc(leaked_memory, 128, 16);
    leaked_memory_dbg = (int*)_aligned_realloc_dbg(leaked_memory_dbg, 48, 16, __FILE__, __LINE__);
    leaked = (int*)_aligned_offset_recalloc(leaked, 1, 52, 16, 2);
    leaked_memory = (int*)_aligned_recalloc(leaked_memory, 1, 132, 16);
    leaked_memory_dbg = (int*)_aligned_recalloc_dbg(leaked_memory_dbg, 1, 64, 16, __FILE__, __LINE__);
    if (bFree)
    {
        _aligned_free(leaked);
        _aligned_free(leaked_memory);
        _aligned_free_dbg(leaked_memory_dbg);
    }
}

void allocStrdup(bool bFree)
{
    int* leaked_memory = (int*)_strdup("_strdup() leaks!");
    int* leaked_memory_dbg = (int*)_strdup_dbg("_strdup_dbg() leaks!", _NORMAL_BLOCK, __FILE__, __LINE__);
    void* leaked_wmemory = (int*)_wcsdup(L"_wcsdup() leaks!");
    void* leaked_wmemory_dbg = (int*)_wcsdup_dbg(L"_wcsdup_dbg() leaks!", _NORMAL_BLOCK, __FILE__, __LINE__);
    if (bFree)
    {
        free(leaked_memory);
        _free_dbg(leaked_memory_dbg, _NORMAL_BLOCK);
        free(leaked_wmemory);
        _free_dbg(leaked_wmemory_dbg, _NORMAL_BLOCK);
    }
}

void allocHeapAlloc(bool bFree)
{
    HANDLE heap = HeapCreate(0x0, 0, 0);
    int* leaked_memory = (int*)HeapAlloc(heap, 0x0, 15);
    if (bFree)
    {
        HeapFree(heap, 0, leaked_memory);
        HeapDestroy(heap);
    }
}

void allocIMalloc(bool bFree)
{
    IMalloc *imalloc = NULL;
    HRESULT hr = CoGetMalloc(1, &imalloc);
    assert(SUCCEEDED(hr));
    int* leaked_memory = static_cast<int*>(imalloc->Alloc(34));
    if (bFree)
    {
        imalloc->Free(leaked_memory);
    }
}

void allocGetProcMalloc(bool bFree)
{
    malloc_t pmalloc = NULL;
    free_t pfree = NULL;
    HMODULE crt = LoadLibrary(CRTDLLNAME);
    assert(crt != NULL);
    pmalloc = reinterpret_cast<malloc_t>(GetProcAddress(crt, "malloc"));
    assert(pmalloc != NULL);
    int* leaked_memory = static_cast<int*>(pmalloc(64));
    if (bFree)
    {
        pfree = reinterpret_cast<free_t>(GetProcAddress(crt, "free"));
        pfree(leaked_memory);
        FreeLibrary(crt);
    }
}

#pragma optimize( "", off )

template<size_t i>
struct allocator;

template<>
struct allocator<0> {

__declspec(noinline) static void alloc(allocFunc_t func, bool bFree)
{
    func(bFree);
}

};

template<size_t i>
struct allocator {

__declspec(noinline) static void alloc(allocFunc_t func, bool bFree)
{
    allocator<i - 1>::alloc(func, bFree);
}

};

#pragma optimize( "", on )

__declspec(noinline) void alloc(allocFunc_t func, bool bFree)
{
    allocator<recursion>::alloc(func, bFree);
}

const int repeats = 1;

void LeakMemoryMalloc(int repeat, bool bFree)
{
    for (int i = 0; i < repeat; i++)
    {
        alloc(allocMalloc, bFree);
    }
}

void LeakMemoryNew(int repeat, bool bFree)
{
    for (int i = 0; i < repeat; i++)
    {
        alloc(allocNew, bFree);
    }
}

void LeakMemoryNewArray(int repeat, bool bFree)
{
    for (int i = 0; i < repeat; i++)
    {
        alloc(allocNewArray, bFree);
    }
}

void LeakMemoryCalloc(int repeat, bool bFree)
{
    for (int i = 0; i < repeat; i++)
    {
        alloc(allocCalloc, bFree);
    }
}

void LeakMemoryRealloc(int repeat, bool bFree)
{
    for (int i = 0; i < repeat; i++)
    {
        alloc(allocRealloc, bFree);
    }
}

void LeakMemoryCoTaskMem(int repeat, bool bFree)
{
    for (int i = 0; i < repeat; i++)
    {
        alloc(allocCoTaskMem, bFree);
    }
}

void LeakMemoryAlignedMalloc(int repeat, bool bFree)
{
    for (int i = 0; i < repeat; i++)
    {
        alloc(allocAlignedMalloc, bFree);
    }
}

void LeakMemoryAlignedRealloc(int repeat, bool bFree)
{
    for (int i = 0; i < repeat; i++)
    {
        alloc(allocAlignedRealloc, bFree);
    }
}

void LeakMemoryStrdup(int repeat, bool bFree)
{
    for (int i = 0; i < repeat; i++)
    {
        alloc(allocStrdup, bFree);
    }
}

void LeakMemoryHeapAlloc(int repeat, bool bFree)
{
    for (int i = 0; i < repeat; i++)
    {
        alloc(allocHeapAlloc, bFree);
    }
}

void LeakMemoryIMalloc(int repeat, bool bFree)
{
    for (int i = 0; i < repeat; i++)
    {
        alloc(allocIMalloc, bFree);
    }
}

void LeakMemoryGetProcMalloc(int repeat, bool bFree)
{
    for (int i = 0; i < repeat; i++)
    {
        alloc(allocGetProcMalloc, bFree);
    }
}
