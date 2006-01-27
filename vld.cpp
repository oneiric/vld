////////////////////////////////////////////////////////////////////////////////
//  $Id: vld.cpp,v 1.35 2006/01/27 23:02:37 dmouldin Exp $
//
//  Visual Leak Detector (Version 1.0)
//  Copyright (c) 2005 Dan Moulding
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation; either version 2.1 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  See COPYING.txt for the full terms of the GNU Lesser General Public License.
//
////////////////////////////////////////////////////////////////////////////////

#if !defined(_DEBUG) || !defined(_DLL)
#error "Visual Leak Detector must be dynamically linked with the debug C runtime library (compiler option /MDd)"
#endif // _DEBUG

#include <cstdio>
#include <windows.h>
#define __out_xcount(x) // Workaround for the specstrings.h bug in the Platform SDK.
#define DBGHELP_TRANSLATE_TCHAR
#include <dbghelp.h>    // Provides stack walking and symbol handling services.
#define _CRTBLD         // Force dbgint.h and winheap.h to allow us to include them.
#include <dbgint.h>     // Provides access to the CRT heap internals, specifically the memory block header structure.
#include <winheap.h>    // Provides access to more heap internals, specifically the "paragraph" size.
#undef _CRTBLD
#define VLDBUILD        // Declares that we are building Visual Leak Detector.
#include "callstack.h"  // Provides a custom class for handling call stacks
#include "map.h"        // Provides a custom STL-like map template
#include "utility.h"    // Provides various utility functions.
#include "vldheap.h"    // Provides internal new and delete operators.
#include "vldint.h"     // Provides access to the Visual Leak Detector internals.

#define BLOCKMAPRESERVE  64  // This should strike a balance between memory use and a desire to minimize heap hits.
#define HEAPMAPRESERVE   2   // Usually there won't be more than a few heaps in the process, so this should be small.
#define MODULESETRESERVE 16  // There are likely to be several modules loaded in the process.

// Imported global variables.
extern vldblockheader_t *vldblocklist;
extern HANDLE            vldheap;

// Global variables.
HANDLE crtheap = NULL;    // Handle to the static CRT heap.
HANDLE dllcrtheap = NULL; // Handle to the shared DLL CRT heap.
HANDLE currentprocess;    // Pseudo-handle for the current process.
HANDLE currentthread;     // Pseudo-handle for the current thread.
HANDLE processheap;       // Handle to the process's heap (COM allocations come from here).

// Function pointer types for explicit dynamic linking with functions that can't
// be load-time linked.
typedef void* (__cdecl *crtnewdbg_t)(unsigned int size, int type, const char *file, int line);
typedef void (__cdecl *delete_t)(void *);
typedef NTSTATUS (__stdcall *LdrLoadDll_t)(LPWSTR, PDWORD, unicodestring_t *, PHANDLE);
typedef void* (__cdecl *mfc42newdbg_t)(unsigned int size, const char *file, int line);
typedef void* (__cdecl *new_t)(unsigned int);

// Global function pointers for explicit dynamic linking with functions that
// can't be load-time linked.
static delete_t      crtdelete = NULL;
static new_t         crtnew = NULL;
static crtnewdbg_t   crtnewdbg = NULL;
static LdrLoadDll_t  LdrLoadDll = NULL;
static delete_t      mfc42delete = NULL;
static new_t         mfc42new = NULL;
static mfc42newdbg_t mfc42newdbg = NULL;

// The one and only VisualLeakDetector object instance.
__declspec(dllexport) VisualLeakDetector vld;

// Thread-local storage
__declspec(thread) static tls_t vldtls;

// The import patch table: lists the heap-related API imports that VLD patches
// through to replacement functions provided by VLD. Having this table simply
// makes it more convenient to add additional IAT patches.
patchentry_t VisualLeakDetector::m_patchtable [] = {
    // Win32 heap APIs
    "kernel32.dll", "HeapAlloc",         _HeapAlloc,
    "kernel32.dll", "HeapCreate",        _HeapCreate,
    "kernel32.dll", "HeapDestroy",       _HeapDestroy,
    "kernel32.dll", "HeapFree",          _HeapFree,
    "kernel32.dll", "HeapReAlloc",       _HeapReAlloc,

    // MFC new and delete operators (exported by ordinal)
    "mfc42d.dll",   (const char*)721,    _mfc42_delete,
    "mfc42d.dll",   (const char*)711,    _mfc42_new,
    "mfc42d.dll",   (const char*)714,    _mfc42_new_dbg,

    // CRT new and delete operators
    "msvcrtd.dll",  "??2@YAPAXI@Z",      _crt_new,
    "msvcrtd.dll",  "??2@YAPAXIHPBDH@Z", _crt_new_dbg,
    "msvcrtd.dll",  "??3@YAXPAX@Z",      _crt_delete,

    // CRT heap APIs
    "msvcrtd.dll",  "_free_dbg",         __free_dbg,
    "msvcrtd.dll",  "_malloc_dbg",       __malloc_dbg,
    "msvcrtd.dll",  "_realloc_dbg",      __realloc_dbg,
    "msvcrtd.dll",  "free",              _free,
    "msvcrtd.dll",  "malloc",            _malloc,
    "/msvcrtd.dll",  "realloc",           _realloc,

    // COM heap APIs
    "ole32.dll",    "CoGetMalloc",       _CoGetMalloc,
    "ole32.dll",    "CoTaskMemAlloc",    _CoTaskMemAlloc,
    "ole32.dll",    "CoTaskMemFree",     _CoTaskMemFree,
    "ole32.dll",    "CoTaskMemRealloc",  _CoTaskMemRealloc
};

// Constructor - Initializes private data, loads configuration options, and
//   attaches Visual Leak Detector to (almost) all modules loaded into the
//   current process.
//
VisualLeakDetector::VisualLeakDetector ()
{
    WCHAR   bom = BOM; // Unicode byte-order mark.
    HMODULE kernel32 = GetModuleHandle(L"kernel32.dll");
    LPWSTR  symbolpath;

    // Initialize global variables.
    currentprocess = GetCurrentProcess();
    currentthread = GetCurrentThread();
    processheap = GetProcessHeap();
    vldheap = HeapCreate(0x0, 0, 0);

    // Initialize private data.
    m_heapmap         = new HeapMap;
    CoGetMalloc(1, &m_imalloc);
    m_heapmap->reserve(HEAPMAPRESERVE);
    InitializeCriticalSection(&m_maplock);
    m_maxdatadump     = 0xffffffff;
    m_maxtraceframes  = 0xffffffff;
    wcsnset(m_modulelist, L'\0', MAXMODULELISTLENGTH);
    m_modulesincluded = 0;
    m_moduleset       = new ModuleSet;
    m_moduleset->reserve(MODULESETRESERVE);
    m_optflags        = 0x0;
    m_reportfile      = NULL;
    wcsncpy(m_reportfilepath, VLD_DEFAULT_REPORT_FILE_NAME, MAX_PATH);
    m_selftestfile    = __FILE__;
    m_selftestline    = 0;
    m_status          = 0x0;

    // Load configuration options.
    configure();
    if (m_optflags & VLD_OPT_SELF_TEST) {
        // Self-test mode has been enabled. Intentionally leak a small amount of
        // memory so that memory leak self-checking can be verified.
        if (m_optflags & VLD_OPT_UNICODE_REPORT) {
            wcsncpy(new WCHAR [21], L"Memory Leak Self-Test", 21); m_selftestline = __LINE__;
        }
        else {
            strncpy(new CHAR [21], "Memory Leak Self-Test", 21); m_selftestline = __LINE__;
        }
    }
    if (m_optflags & VLD_OPT_START_DISABLED) {
        // Memory leak detection will initially be disabled.
        m_status |= VLD_STATUS_NEVER_ENABLED;
    }
    if (m_optflags & VLD_OPT_REPORT_TO_FILE) {
        // Reporting to file enabled.
        if (m_optflags & VLD_OPT_UNICODE_REPORT) {
            // Unicode data encoding has been enabled. Write the byte-order
            // mark before anything else gets written to the file. Open the
            // file for binary writing.
            m_reportfile = _wfopen(m_reportfilepath, L"wb");
            if (m_reportfile != NULL) {
                fwrite(&bom, sizeof(WCHAR), 1, m_reportfile);
            }
            setreportencoding(unicode);
        }
        else {
            // Open the file in text mode for ASCII output.
            m_reportfile = _wfopen(m_reportfilepath, L"w");
            setreportencoding(ascii);
        }
        if (m_reportfile == NULL) {
            report(L"WARNING: Visual Leak Detector: Couldn't open report file for writing: %s\n"
                   L"  The report will be sent to the debugger instead.\n", m_reportfilepath);
        }
        else {
            // Set the "report" function to write to the file.
            setreportfile(m_reportfile, m_optflags & VLD_OPT_REPORT_TO_DEBUGGER);
        }
    }

    // Initialize the symbol handler. We use it for obtaining source
    // file/line number information and function names for the memory leak
    // report, and for identifying modules with good debug information
    // (suitable for memory leak debugging).
    symbolpath = buildsymbolsearchpath();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    if (!SymInitialize(currentprocess, symbolpath, FALSE)) {
        report(L"WARNING: Visual Leak Detector: The symbol handler failed to initialize (error=%lu).\n"
               L"    File and function names will probably not be available in call stacks.\n", GetLastError());
    }
    delete [] symbolpath;

    // Patch into kernel32.dll's calls to LdrLoadDll so that VLD can
    // dynamically attach to new modules loaded during runtime.
    patchimport(kernel32, "ntdll.dll", "LdrLoadDll", _LdrLoadDll);

    // Attach Visual Leak Detector to each module loaded in the process. Any
    // modules which are to be excluded will not be attached to.
    EnumerateLoadedModules64(currentprocess, attachtomodule, NULL);

    report(L"Visual Leak Detector Version " VLDVERSION L" installed.\n");
    if (m_status & VLD_STATUS_FORCE_REPORT_TO_FILE) {
        // The report is being forced to a file. Let the human know why.
        report(L"NOTE: Visual Leak Detector: Unicode-encoded reporting has been enabled, but the\n"
               L"  debugger is the only selected report destination. The debugger cannot display\n"
               L"  Unicode characters, so the report will also be sent to a file. If no file has\n"
               L"  been specified, the default file name is \"" VLD_DEFAULT_REPORT_FILE_NAME L"\".\n");

    }
    reportconfig();
}

// Destructor - Detaches Visual Leak Detector from all previously attached
//   modules., frees internally allocated resources, and generates the memory
//   leak report.
//
VisualLeakDetector::~VisualLeakDetector ()
{
    BlockMap::Iterator  blockit;
    BlockMap           *blockmap;
    vldblockheader_t   *header;
    HeapMap::Iterator   heapit;
    SIZE_T              internalleaks = 0;
    const char         *leakfile;
    WCHAR               leakfilew [MAX_PATH];
    int                 leakline;

    // Detach Visual Leak Detector from previously attached modules.
    EnumerateLoadedModules64(currentprocess, detachfrommodule, NULL);

    if (m_status & VLD_STATUS_NEVER_ENABLED) {
        // Visual Leak Detector started with leak detection disabled and
        // it was never enabled at runtime. A lot of good that does.
        report(L"WARNING: Visual Leak Detector: Memory leak detection was never enabled.\n");
    }
    else if (m_modulesincluded == 0) {
        // No modules were included in leak detection.
        report(L"WARNING: No modules were ever included in memory leak detection.\n"
               L"  No memory leak detection was actually done. Please check the \"ModuleList\"\n"
               L"  option in vld.ini and be sure that at least one module is being included,\n"
               L"  and that no module names have been misspelt.\n");
    }
    else {
        // Generate a memory leak report.
        reportleaks();
    }

    EnterCriticalSection(&m_maplock);

    // Free internally allocated resources.
    for (heapit = m_heapmap->begin(); heapit != m_heapmap->end(); ++heapit) {
        blockmap = (*heapit).second;
        for (blockit = blockmap->begin(); blockit != blockmap->end(); ++blockit) {
            delete (*blockit).second;
        }
        delete blockmap;
    }
    delete m_heapmap;
    delete m_moduleset;

    // Do a memory leak self-check.
    header = vldblocklist;
    while (header) {
        // Doh! VLD still has an internally allocated block!
        // This won't ever actually happen, right guys?... guys?
        internalleaks++;
        leakfile = header->file;
        leakline = header->line;
        mbstowcs(leakfilew, leakfile, MAX_PATH);
        report(L"ERROR: Visual Leak Detector: Detected a memory leak internal to Visual Leak Detector!!\n");
        report(L"---------- Block %ld at " ADDRESSFORMAT L": %u bytes ----------\n", header->serialnumber, BLOCKDATA(header), header->size);
        report(L"  Call Stack:\n");
        report(L"    %s (%d): Full call stack not available.\n", leakfilew, leakline);
        report(L"  Data:\n");
        if (m_optflags & VLD_OPT_UNICODE_REPORT) {
            dumpmemoryw(BLOCKDATA(header), (m_maxdatadump < header->size) ? m_maxdatadump : header->size);
        }
        else {
            dumpmemorya(BLOCKDATA(header), (m_maxdatadump < header->size) ? m_maxdatadump : header->size);
        }
        report(L"\n");
        header = header->next;
    }
    if (m_optflags & VLD_OPT_SELF_TEST) {
        if ((internalleaks == 1) && (strcmp(leakfile, m_selftestfile) == 0) && (leakline == m_selftestline)) {
            report(L"Visual Leak Detector passed the memory leak self-test.\n");
        }
        else {
            report(L"ERROR: Visual Leak Detector: Failed the memory leak self-test.\n");
        }
    }

    LeaveCriticalSection(&m_maplock);
    DeleteCriticalSection(&m_maplock);

    report(L"Visual Leak Detector is now exiting.\n");

    if (m_reportfile != NULL) {
        fclose(m_reportfile);
    }
}

// __free_dbg - Calls to _free_dbg are patched through to this function. This
//   function calls VLD's free tracking function and then invokes the the real
//   _free_dbg. All arguments passed to this function are passed on to _free_dbg
//   without modification.
//
//   Note: Only modules dynamically linked to the CRT will import _free_dbg, so
//     this function only frees blocks allocated to the shared DLL CRT heap.
//
//  - mem (IN): Pointer to the memory block to be freed.
//
//  - type (IN): CRT block "use type" of the block being freed.
//
//  Return Value:
//
//    None.
//
void VisualLeakDetector::__free_dbg (void *mem, int type)
{
    vld.enter();
    vld.unmapblock(dllcrtheap, (LPVOID)pHdr(mem));
    _free_dbg(mem, type);
    vld.leave();
}

// __malloc_dbg - Calls to _malloc_dbg are patched through to this function.
//   This function invokes the real _malloc_dbg and then calls VLD's allocation
//   tracking function. All arguments passed to this function are passed on to
//   the real _malloc_dbg without modification.
//
//   Note: Only modules dynamically linked to the CRT will import _malloc_dbg,
//     so this function only allocates blocks from the shared DLL CRT heap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _malloc_dbg.
//
void* VisualLeakDetector::__malloc_dbg (size_t size, int type, const char *file, int line)
{
    void   *block;
    SIZE_T  ra;
    
    vld.enter();
    if (dllcrtheap == NULL) {
        // This is the first time that a CRT allocator has been invoked. Set
        // a flag that _HeapAlloc will use to know that the current allocation
        // is from the CRT heap so that the CRT heap handle can be initialized.
        vldtls.status |= VLD_TLS_INITDLLCRTHEAP;
    }
    block = _malloc_dbg(size, type, file, line);
    if (block != NULL) {
        // The *real* memory block includes the CRT header, the user-data, an
        // additional "no man's land" boundary, and any padding required to make
        // the overall size a multiple of BYTES_PER_PARA.
        size = sizeof(_CrtMemBlockHeader) + size + nNoMansLandSize + ((size + BYTES_PER_PARA - 1) & ~(BYTES_PER_PARA - 1));
        RETURNADDRESS(ra);
        vld.mapblock(ra, dllcrtheap, (LPVOID)pHdr(block), (DWORD)size);
    }
    vld.leave();

    return block;
}

// __realloc_dbg - Calls to _realloc_dbg are patched through to this function.
//   This function invokes the real _realloc_dbg and then calls VLD's
//   reallocation tracking function. All areguments passed to this function are
//   passed on to the real _realloc_dbg without modification.
//
//   Note: Only modules dynamically linked to the CRT will import _realloc_dbg,
//     so this function only reallocates blocks from the shared DLL CRT heap.
//
//  - mem (IN): Pointer to the memory block to be reallocated.
//
//  - size (IN): The size of the memory block to reallocate.
//
//  - type (IN): The CRT "use type" of the block to be reallocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above filel, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _realloc_dbg.
//
void* VisualLeakDetector::__realloc_dbg (void *mem, size_t size, int type, const char *file, int line)
{
    void   *newmem;
    SIZE_T  ra;
    
    vld.enter();
    if (dllcrtheap == NULL) {
        // This is the first time that a CRT allocator has been invoked. Set
        // a flag that _HeapAlloc will use to know that the current allocation
        // is from the CRT heap so that the CRT heap handle can be initialized.
        vldtls.status |= VLD_TLS_INITDLLCRTHEAP;
    }
    newmem = _realloc_dbg(mem, size, type, file, line);
    if (newmem != NULL) {
        // The *real* memory block includes the CRT header, the user-data, an
        // additional "no man's land" boundary, and any padding required to make
        // the overall size a multiple of BYTES_PER_PARA.
        size = sizeof(_CrtMemBlockHeader) + size + nNoMansLandSize + ((size + BYTES_PER_PARA - 1) & ~(BYTES_PER_PARA - 1));
        RETURNADDRESS(ra);
        vld.remapblock(ra, dllcrtheap, (LPVOID)pHdr(mem), (LPVOID)pHdr(newmem), (SIZE_T)size);
    }
    vld.leave();

    return newmem;
}

// _crt_delete - Calls to the CRT's delete operator are patched through to this
//   function. This function calls VLD's free tracking function and then invokes
//   the real CRT delete operator. All arguments passed to this function are
//   passed on to the CRT delete operator without modification.
//
//   Note: Only modules dynamically linked to the CRT will import the CRT's
//     delete operator, so this function only frees blocks allocated to the
//     shared DLL CRT heap.
//
//  - mem (IN): Pointer to the memory block to be freed.
//
//  Return Value:
//
//    None.
//
void VisualLeakDetector::_crt_delete (void *mem)
{
    HMODULE msvcrtd;
    
    vld.enter();
    vld.unmapblock(dllcrtheap, (LPVOID)pHdr(mem));

    // "delete" is ambiguous. To call the specific delete operator exported by
    // msvcrtd.dll, we need to make an explicit dynamic link.
    if (crtdelete == NULL) {
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        crtdelete = (delete_t)GetProcAddress(msvcrtd, "??3@YAXPAX@Z");
    }
    crtdelete(mem);
    vld.leave();
}

// _crt_new - Calls to the CRT's new operator are patched through to this
//  function. This function invokes the real CRT new operator and then calls
//  VLD's allocation tracking function. All arguments passed to this function
//  are passed on to the CRT new operator without modification.
//
//  Note: Only modules dynamically linked to the CRT will import the CRT's new
//    operator, so this function only allocates blocks from the shared DLL CRT
//    heap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the CRT new operator.
//
void* VisualLeakDetector::_crt_new (unsigned int size)
{
    void    *block;
    HMODULE  msvcrtd;
    SIZE_T   ra;

    vld.enter();

    // "new" is ambiguous. To call the specific new operator exported by
    // msvcrtd.dll, we need to make an explicit dynamic link.
    if (crtnew == NULL) {
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        crtnew = (new_t)GetProcAddress(msvcrtd, "??2@YAPAXI@Z");
    }
    if (dllcrtheap == NULL) {
        // This is the first time that a CRT allocator has been invoked. Set
        // a flag that _HeapAlloc will use to know that the current allocation
        // is from the CRT heap so that the CRT heap handle can be initialized.
        vldtls.status |= VLD_TLS_INITDLLCRTHEAP;
    }
    block = crtnew(size);
    if (block != NULL) {
        // The *real* memory block includes the CRT header, the user-data, an
        // additional "no man's land" boundary, and any padding required to make
        // the overall size a multiple of BYTES_PER_PARA.
        size = sizeof(_CrtMemBlockHeader) + size + nNoMansLandSize + ((size + BYTES_PER_PARA - 1) & ~(BYTES_PER_PARA - 1));
        RETURNADDRESS(ra);
        vld.mapblock(ra, dllcrtheap, (LPVOID)pHdr(block), (SIZE_T)size);
    }
    vld.leave();

    return block;
}

// _crt_new_dbg - Calls to the CRT's debug new operator are patched through to
//  this function. This function invokes the real CRT debug new operator and
//  then calls VLD's allocation tracking function. All arguments passed to this
//  function are passed on to the CRT debug new operator without modification.
//
//  Note: Only modules dynamically linked to the CRT will import the CRT's debug
//    new operator, so this function only allocates blocks from the shared DLL
//    CRT heap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the CRT debug new operator.
//
void* VisualLeakDetector::_crt_new_dbg (unsigned int size, int type, const char *file, int line)
{
    void    *block;
    HMODULE  msvcrtd;
    SIZE_T   ra;

    vld.enter();

    // "new" is ambiguous. To call the specific new operator exported by
    // msvcrtd.dll, we need to make an explicit dynamic link.
    if (crtnewdbg == NULL) {
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        crtnewdbg = (crtnewdbg_t)GetProcAddress(msvcrtd, "??2@YAPAXIHPBDH@Z");
    }
    if (dllcrtheap == NULL) {
        // This is the first time that a CRT allocator has been invoked. Set
        // a flag that _HeapAlloc will use to know that the current allocation
        // is from the CRT heap so that the CRT heap handle can be initialized.
        vldtls.status |= VLD_TLS_INITDLLCRTHEAP;
    }
    block = crtnewdbg(size, type, file, line);
    if (block != NULL) {
        // The *real* memory block includes the CRT header, the user-data, an
        // additional "no man's land" boundary, and any padding required to make
        // the overall size a multiple of BYTES_PER_PARA.
        size = sizeof(_CrtMemBlockHeader) + size + nNoMansLandSize + ((size + BYTES_PER_PARA - 1) & ~(BYTES_PER_PARA - 1));
        RETURNADDRESS(ra);
        vld.mapblock(ra, dllcrtheap, (LPVOID)pHdr(block), (SIZE_T)size);
    }
    vld.leave();

    return block;
}

// _CoGetMalloc - Calls to CoGetMalloc are patched through to this function.
//   This function returns a pointer to Visual Leak Detector's implementation
//   of the IMalloc interface, instead of returning a pointer to the system
//   implementation. This allows VLD's implementation of the IMalloc interface
//   (which is basically a thin wrapper around the system implementation) to be
//   invoked in place of the system implementation.
//
//  - context (IN): Reserved; value must be 1.
//
//  - imalloc (IN): Address of a pointer to receive the address of VLD's
//      implementation of the IMalloc interface.
//
//  Return Value:
//
//    Always returns S_OK.
//
HRESULT VisualLeakDetector::_CoGetMalloc (DWORD context, LPMALLOC *imalloc)
{
    *imalloc = (LPMALLOC)&vld;

    return S_OK;
}

// _CoTaskMemAlloc - Calls to CoTaskMemAlloc are patched through to this
//   function. This function invokes the real CoTaskMemAlloc and then calls
//   VLD's allocation tracking function. All parameters passed to this function
//   are passed on to CoTaskMemAlloc without modification.
//
//  - size (IN): Size of the memory block to allocate.
//
//  Return Value:
//
//    Returns the value returned from CoTaskMemAlloc.
//
LPVOID VisualLeakDetector::_CoTaskMemAlloc (ULONG size)
{
    LPVOID block;
    SIZE_T ra;
    
    vld.enter();
    block = CoTaskMemAlloc(size);
    if (block != NULL) {
        RETURNADDRESS(ra);
        vld.mapblock(ra, processheap, block, (SIZE_T)size);
    }
    vld.leave();

    return block;
}

// _CoTaskMemFree - Calls to CoTaskMemFree are patched through to this function.
//   This function calls VLD's free tracking function and then invokes the real
//   CoTaskMemFree. All arguments passed to this function are passed on to
//   CoTaskMemFree without modification.
//
//  - mem (IN): Pointer to the memory block to be freed.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::_CoTaskMemFree (LPVOID mem)
{
    vld.enter();
    vld.unmapblock(processheap, mem);
    CoTaskMemFree(mem);
    vld.leave();
}

// _CoTaskMemRealloc - Calls to CoTaskMemRealloc are patched through to this
//   function. This function invokes the real CoTaskMemRealloc and then calls
//   VLD's reallocation tracking function. All arguments passed to this function
//   are passed on to CoTaskMemRealloc without modification.
//
//  - mem (IN): Pointer to the memory block to reallocate.
//
//  - size (IN): Size, in bytes, of the block to reallocate.
//
//  Return Value:
//
//    Returns the value returned from CoTaskMemRealloc.
//
LPVOID VisualLeakDetector::_CoTaskMemRealloc (LPVOID mem, ULONG size)
{
    LPVOID newmem;
    SIZE_T ra;

    vld.enter();
    newmem = CoTaskMemRealloc(mem, size);
    if (newmem != NULL) {
        RETURNADDRESS(ra);
        vld.remapblock(ra, processheap, mem, newmem, (SIZE_T)size);
    }
    vld.leave();

    return newmem;
}

// _free - Calls to free are patched through to this function. This function
//   calls VLD's free tracking function and then invokes the real "free". All
//   arguments passed to this function are passed on to the real "free" without
//   modification.
//
//   Note: Only modules dynamically linked to the CRT will import free, so this
//     function only frees blocks allocated to the shared DLL CRT heap.
//
//  - mem (IN): Pointer to the memory block to be freed. Passed on to "free"
//      without modification.
//
//  Return Value:
//
//    None.
//
void VisualLeakDetector::_free (void *mem)
{
    vld.enter();
    vld.unmapblock(dllcrtheap, (LPVOID)pHdr(mem));
    free(mem);
    vld.leave();
}

// _HeapAlloc - Calls to HeapAlloc are patched through to this function. This
//   function invokes the real HeapAlloc and then calls VLD's allocation
//   tracking function. All arguments passed to this function are passed on to
//   the real HeapAlloc without modification.
//
//  - heap (IN): Handle to the heap from which to allocate memory.
//
//  - flags (IN): Heap allocation control flags.
//
//  - size (IN): Size, in bytes, of the block to allocate.
//
//  Return Value:
//
//    Returns the return value from HeapAlloc.
//
LPVOID VisualLeakDetector::_HeapAlloc (HANDLE heap, DWORD flags, SIZE_T size)
{
    LPVOID block;
    SIZE_T ra;

    vld.enter();
    if (vldtls.status & VLD_TLS_INITDLLCRTHEAP) {
        // The current allocation is from the shared DLL CRT heap, and the
        // caller at the CRT level requested that we initialize the handle.
        dllcrtheap = heap;
        vldtls.status &= ~VLD_TLS_INITDLLCRTHEAP;
    }
    block = HeapAlloc(heap, flags, size);
    if (block != NULL) {
        RETURNADDRESS(ra);
        vld.mapblock(ra, heap, block, size);
    }
    vld.leave();

    return block;
}

// _HeapCreate - Calls to HeapCreate are patched through to this function. This
//   function invokes the real HeapCreate and then calls VLD's heap creation
//   tracking function. All arguments passed to this function are passed on to
//   the real HeapCreate without modification.
//
//  - options (IN): Heap options.
//
//  - initsize (IN): Initial size of the heap.
//
//  - maxsize (IN): Maximum size of the heap.
//
//  Return Value:
//
//    Returns the value returned by HeapCreate.
//
HANDLE VisualLeakDetector::_HeapCreate (DWORD options, SIZE_T initsize, SIZE_T maxsize)
{
    HANDLE heap;

    heap = HeapCreate(options, initsize, maxsize);
    vld.mapheap(heap);

    return heap;
}

// _HeapDestroy - Calls to HeapDestroy are patched through to this function.
//   This function calls VLD's heap destruction tracking function and then
//   invokes the real HeapDestroy. All arguments passed to this function are
//   passed on to the real HeapDestroy without modification.
//
//  - heap (IN): Handle to the heap to be destroyed.
//
//  Return Value:
//
//    Returns the valued returned by HeapDestroy.
//
BOOL VisualLeakDetector::_HeapDestroy (HANDLE heap)
{
    BOOL ret;

    vld.unmapheap(heap);
    ret = HeapDestroy(heap);

    return ret;
}

// _HeapFree - Calls to HeapFree are patched through to this function. This
//   function calls VLD's free tracking function and then invokes the real
//   HeapFree. All arguments passed to this function are passed on to the real
//   HeapFree without modification.
//
//  - heap (IN): Handle to the heap to which the block being freed belongs.
//
//  - flags (IN): Heap control flags.
//
//  - mem (IN): Pointer to the memory block being freed.
//
//  Return Value:
//
//    Returns the value returned by HeapFree.
//
BOOL VisualLeakDetector::_HeapFree (HANDLE heap, DWORD flags, LPVOID mem)
{
    BOOL ret;

    vld.enter();
    vld.unmapblock(heap, mem);
    ret = HeapFree(heap, flags, mem);
    vld.leave();

    return ret;
}

// _HeapReAlloc - Calls to HeapReAlloc are patched through to this function.
//   This function invokes the real HeapReAlloc and then calls VLD's
//   reallocation tracking function. All arguments passed to this function are
//   passed on to the real HeapReAlloc without modification.
//
//  - heap (IN): Handle to the heap to reallocate memory from.
//
//  - flags (IN): Heap control flags.
//
//  - size (IN): Size, in bytes, of the block to reallocate.
//
//  Return Value:
//
//    Returns the value returned by HeapReAlloc.
//
LPVOID VisualLeakDetector::_HeapReAlloc (HANDLE heap, DWORD flags, LPVOID mem, SIZE_T size)
{
    LPVOID newmem;
    SIZE_T ra;

    vld.enter();
    newmem = HeapReAlloc(heap, flags, mem, size);
    if (newmem != NULL) {
        RETURNADDRESS(ra);
        vld.remapblock(ra, heap, mem, newmem, size);
    }
    vld.leave();

    return newmem;
}

// _LdrLoadDll - Calls to LdrLoadDll are patched through to this function. This
//   function invokes the real LdrLoadDll and then re-attaches VLD to all
//   modules loaded in the process after loading of the new DLL is complete.
//   All modules must be re-enumerated because the explicit load of the
//   specified module may result in the implicit load of one or more additional
//   modules which are dependencies of the specified module.
//
//  - searchpath (IN): The patch to use for searching for the specified module
//      to be loaded.
//
//  - flags (IN): Pointer to action flags.
//
//  - modulename (IN): Pointer to a unicodestring_t structure specifying the
//      name of the module to be loaded.
//
//  - modulehandle (OUT): Address of a HANDLE to receive the newly loaded
//      module's handle.
//
//  Return Value:
//
//    Returns the value returned by LdrLoadDll.
//
NTSTATUS VisualLeakDetector::_LdrLoadDll (LPWSTR searchpath, PDWORD flags, unicodestring_t *modulename, PHANDLE modulehandle)
{
    HMODULE  ntdll;
    NTSTATUS status;

    if (LdrLoadDll == NULL) {
        ntdll = GetModuleHandle(L"ntdll.dll");
        LdrLoadDll = (LdrLoadDll_t)GetProcAddress(ntdll, "LdrLoadDll");
    }
    status = LdrLoadDll(searchpath, flags, modulename, modulehandle);

    // Reset the information we have stored about all loaded modules.
    vld.m_modulesincluded = 0;
    vld.m_moduleset->clear();
    
    // Re-attach to all loaded modules.
    EnumerateLoadedModules64(currentprocess, attachtomodule, NULL);

    return status;
}

// _malloc - Calls to malloc and operator new are patched through to this
//   function. This function invokes the real malloc and then calls VLD's
//   allocation tracking function. All arguments passed to this function are
//   passed on to the real malloc without modification.
//
//   Note: Only modules dynamically linked to the CRT will import malloc, so
//     this function only allocates blocks from the shared DLL CRT heap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the valued returned from malloc.
//
void* VisualLeakDetector::_malloc (size_t size)
{
    void   *block;
    SIZE_T  ra;

    vld.enter();
    if (dllcrtheap == NULL) {
        // This is the first time that a CRT allocator has been invoked. Set
        // a flag that _HeapAlloc will use to know that the current allocation
        // is from the CRT heap so that the CRT heap handle can be initialized.
        vldtls.status |= VLD_TLS_INITDLLCRTHEAP;
    }
    block = malloc(size);
    if (block != NULL) {
        // The *real* memory block includes the CRT header, the user-data, an
        // additional "no man's land" boundary, and any padding required to make
        // the overall size a multiple of BYTES_PER_PARA.
        size = sizeof(_CrtMemBlockHeader) + size + nNoMansLandSize + ((size + BYTES_PER_PARA - 1) & ~(BYTES_PER_PARA - 1));
        RETURNADDRESS(ra);
        vld.mapblock(ra, dllcrtheap, (LPVOID)pHdr(block), (SIZE_T)size);
    }
    vld.leave();

    return block;
}

// _mfc42_delete - Calls to the MFC 4.2 delete operator are patched through to
//   this function. This function calls VLD's free tracking function and then
//   invokes the real MFC 4.2 delete operator. All arguments passed to this
//   function are passed on to the MFC 4.2 delete operator without modification.
//
//   Note: Only modules dynamically linked to the MFC 4.2 libraries will import
//     the MFC 4.2 delete operator, so this function only frees blocks allocated
//     to the shared DLL CRT heap (MFC uses the CRT heap).
//
//  - mem (IN): Pointer to the memory block to be freed.
//
//  Return Value:
//
//    None.
//
void VisualLeakDetector::_mfc42_delete (void *mem)
{
    HMODULE mfc42d;

    vld.enter();
    vld.unmapblock(dllcrtheap, (LPVOID)pHdr(mem));

    // "delete" is ambiguous. To call the specific delete operator exported by
    // mfc42d.dll, we need to make an explicit dynamic link.
    if (mfc42delete == NULL) {
        mfc42d = GetModuleHandle(L"mfc42d.dll");
        mfc42delete = (delete_t)GetProcAddress(mfc42d, (const char*)721);
    }
    mfc42delete(mem);
    vld.leave();
}

// _mfc42_new - Calls to the MFC 4.2 new operator are patched through to this
//  function. This function invokes the real MFC 4.2 new operator and then calls
//  VLD's allocation tracking function. All arguments passed to this function
//  are passed on to the MFC 4.2 new operator without modification.
//
//  Note: Only modules dynamically linked to the MFC 4.2 libraries will import
//    the MFC 4.2 new operator, so this function only allocates blocks from the
//    shared DLL CRT heap (MFC uses the CRT heap).
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the MFC 4.2 new operator.
//
void* VisualLeakDetector::_mfc42_new (unsigned int size)
{
    void    *block;
    HMODULE  mfc42d;
    SIZE_T   ra;

    vld.enter();

    // "new" is ambiguous. To call the specific new operator exported by
    // mfc42d.dll, we need to make an explicit dynamic link.
    if (mfc42new == NULL) {
        mfc42d = GetModuleHandle(L"mfc42d.dll");
        mfc42new = (new_t)GetProcAddress(mfc42d, (const char*)711);
    }
    if (dllcrtheap == NULL) {
        // This is the first time that a CRT allocator has been invoked. Set
        // a flag that _HeapAlloc will use to know that the current allocation
        // is from the CRT heap so that the CRT heap handle can be initialized.
        vldtls.status |= VLD_TLS_INITDLLCRTHEAP;
    }
    block = mfc42new(size);
    if (block != NULL) {
        // The *real* memory block includes the CRT header, the user-data, an
        // additional "no man's land" boundary, and any padding required to make
        // the overall size a multiple of BYTES_PER_PARA.
        size = sizeof(_CrtMemBlockHeader) + size + nNoMansLandSize + ((size + BYTES_PER_PARA - 1) & ~(BYTES_PER_PARA - 1));
        RETURNADDRESS(ra);
        vld.mapblock(ra, dllcrtheap, (LPVOID)pHdr(block), (SIZE_T)size);
    }
    vld.leave();

    return block;
}

// _mfc42_new_dbg - Calls to the MFC 4.2 debug new operator are patched through
//  to this function. This function invokes the real MFC 4.2 debug new operator
//  and then calls VLD's allocation tracking function. All arguments passed to
//  this  function are passed on to the MFC 4.2 debug new operator without
//  modification.
//
//  Note: Only modules dynamically linked to the MFC 4.2 libraries will import
//    the MFC 4.2 debug new operator, so this function only allocates blocks
//    from the shared DLL CRT heap (MFC uses the CRT heap).
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the MFC 4.2 debug new operator.
//
void* VisualLeakDetector::_mfc42_new_dbg (unsigned int size, const char *file, int line)
{
    void    *block;
    HMODULE  mfc42d;
    SIZE_T   ra;

    vld.enter();

    // "new" is ambiguous. To call the specific new operator exported by
    // mfc42d.dll, we need to make an explicit dynamic link.
    if (mfc42newdbg == NULL) {
        mfc42d = GetModuleHandle(L"mfc42d.dll");
        mfc42newdbg = (mfc42newdbg_t)GetProcAddress(mfc42d, (const char*)714);
    }
    if (dllcrtheap == NULL) {
        // This is the first time that a CRT allocator has been invoked. Set
        // a flag that _HeapAlloc will use to know that the current allocation
        // is from the CRT heap so that the CRT heap handle can be initialized.
        vldtls.status |= VLD_TLS_INITDLLCRTHEAP;
    }
    block = mfc42newdbg(size, file, line);
    if (block != NULL) {
        // The *real* memory block includes the CRT header, the user-data, an
        // additional "no man's land" boundary, and any padding required to make
        // the overall size a multiple of BYTES_PER_PARA.
        size = sizeof(_CrtMemBlockHeader) + size + nNoMansLandSize + ((size + BYTES_PER_PARA - 1) & ~(BYTES_PER_PARA - 1));
        RETURNADDRESS(ra);
        vld.mapblock(ra, dllcrtheap, (LPVOID)pHdr(block), (SIZE_T)size);
    }
    vld.leave();

    return block;
}

// _realloc - Calls to realloc are patched through to this function. This
//   function invokes the real realloc and then calls VLD's reallocation
//   tracking function. All arguments passed to this function are passed on to
//   the real realloc without modification.
//
//   Note: Only modules dynamically linked to the CRT will import realloc, so
//     this function only reallocates blocks from the shared DLL CRT heap.
//
//  - mem (IN): Pointer to the memory block to reallocate.
//
//  - size (IN): Size of the memory block to reallocate.
//
//  Return Value:
//
//    Returns the value returned from realloc.
//
void* VisualLeakDetector::_realloc (void *mem, size_t size)
{
    void   *newmem;
    SIZE_T  ra;

    vld.enter();
    if (dllcrtheap == NULL) {
        // This is the first time that a CRT allocator has been invoked. Set
        // a flag that _HeapAlloc will use to know that the current allocation
        // is from the CRT heap so that the CRT heap handle can be initialized.
        vldtls.status |= VLD_TLS_INITDLLCRTHEAP;
    }
    newmem = realloc(mem, size);
    if (newmem != NULL) {
        // The *real* memory block includes the CRT header, the user-data, an
        // additional "no man's land" boundary, and any padding required to make
        // the overall size a multiple of BYTES_PER_PARA.
        size = sizeof(_CrtMemBlockHeader) + size + nNoMansLandSize + ((size + BYTES_PER_PARA - 1) & ~(BYTES_PER_PARA - 1));
        RETURNADDRESS(ra);
        vld.remapblock(ra, dllcrtheap, (LPVOID)pHdr(mem), (LPVOID)pHdr(newmem), (SIZE_T)size);
    }
    vld.leave();

    return newmem;
}

// AddRef - Calls to IMalloc::AddRef end up here. This function is just a
//   wrapper around the system implementation of IMalloc::AddRef.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::AddRef.
//
ULONG VisualLeakDetector::AddRef ()
{
    return m_imalloc->AddRef();
}

// Alloc - Calls to IMalloc::Alloc end up here. This function invokes the
//   system's implementation of IMalloc::Alloc and then calls VLD's allocation
//   tracking function. All arguments passed to this function are passed on to
//   the system's IMalloc::Alloc implementation without modification.
//
//  - size (IN): The size of the memory block to allocate.
//
//  Return Value:
//
//    Returns the value returned by the system's IMalloc::Alloc implementation.
//
LPVOID VisualLeakDetector::Alloc (ULONG size)
{
    LPVOID block;
    SIZE_T ra;

    vld.enter();
    block = m_imalloc->Alloc(size);
    if (block != NULL) {
        RETURNADDRESS(ra);
        mapblock(ra, processheap, block, (SIZE_T)size);
    }
    vld.leave();

    return block;
}

// attachtomodule - Callback function for EnumerateLoadedModules64 that attaches
//   Visual Leak Detector to the specified module. Certain modules may be
//   excluded from attachment because they are either hard-excluded or because
//   they have been listed (by the user) on the list of modules to exclude.
//   If the specified module is excluded, then Visual Leak Detector will not
//   attach to the module and will return without changing anything.
//
//  - modulepath (IN): String containing the name, which may include a path, of
//      the module to attach to.
//
//  - modulebase (IN): Base address of the module.
//
//  - modulesize (IN): Total size of the module.
//
//  - context (IN): User-supplied context (ignored).
//
//  Return Value:
//
//    Always returns TRUE.
//
BOOL VisualLeakDetector::attachtomodule (PCWSTR modulepath, DWORD64 modulebase, ULONG modulesize, PVOID context)
{
    WCHAR             extension [_MAX_EXT];
    WCHAR             filename [_MAX_FNAME];
    IMAGEHLP_MODULE64 moduleimageinfo;
    moduleinfo_t      moduleinfo;
#define MAXMODULENAME (_MAX_FNAME + _MAX_EXT)
    WCHAR             modulename [MAXMODULENAME + 1];
    CHAR              modulepatha [MAX_PATH];
    BOOL              symbolsloaded = FALSE;
    UINT              tablesize = sizeof(m_patchtable) / sizeof(patchentry_t);

    // Extract just the filename and extension from the module path.
    _wsplitpath(modulepath, NULL, NULL, filename, extension);
    wcsncpy(modulename, filename, MAXMODULENAME);
    wcsncat(modulename, extension, MAXMODULENAME - wcslen(modulename));
    wcslwr(modulename);

    // Try to load the module's symbols. This ensures that we have loaded the
    // symbols for every module that has ever been loaded into the process,
    // guaranteeing the symbols' availability when generating the leak report.
    moduleimageinfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
    wcstombs(modulepatha, modulepath, MAX_PATH);
    if ((SymGetModuleInfo64(currentprocess, modulebase, &moduleimageinfo) == TRUE) ||
        ((SymLoadModule64(currentprocess, NULL, modulepatha, NULL, modulebase, modulesize) == modulebase) &&
        (SymGetModuleInfo64(currentprocess, modulebase, &moduleimageinfo) == TRUE))) {
        symbolsloaded = TRUE;
    }

    // Record the address range where the module is loaded.
    moduleinfo.addrlow = (SIZE_T)modulebase;
    moduleinfo.addrhigh = (SIZE_T)modulebase + modulesize - 1;
    moduleinfo.flags = 0x0;

    if (wcsstr(L"vld.dll dbghelp.dll msvcrt.dll", modulename)) {
        // VLD should not attach to the above hard-coded list of modules. If VLD
        // attaches to itself it will result in infinite recurstion. VLD's
        // allocation tracking functions depend on dbghelp.dll and msvcrt.dll,
        // so those too must not be attached for the same reason.
        if (wcsicmp(L"vld.dll", modulename) == 0) {
            // Flag VLD's own module, so that VLD functions can easily be
            // identified and excluded from the memory leak report.
            moduleinfo.flags |= VLD_MODULE_VLDDLL;
        }
        if ((vld.m_optflags & VLD_OPT_MODULE_LIST_INCLUDE) &&
            (wcsstr(vld.m_modulelist, modulename) != NULL)) {
            // This hard-excluded module was specified on the list of modules to be included.
            report(L"WARNING: Visual Leak Detector: A module, %s, specified to be included by the\n"
                   L"  \"ModuleList\" vld.ini option is hard-excluded by Visual Leak Detector.\n"
                   L"  The module will NOT be included in memory leak detection.\n", modulename);
        }

        // Insert the module's information into the module set.
        moduleinfo.flags |= VLD_MODULE_NOTATTACHED;
        vld.m_moduleset->insert(moduleinfo);
        return TRUE;
    }

    if (!(vld.m_optflags & VLD_OPT_MODULE_LIST_INCLUDE) &&
        (wcsstr(vld.m_modulelist, modulename) != NULL)) {
        // The module should be excluded from leak detection because it is
        // listed on the "ModuleList" line in vld.ini and "ModuleListMode" is
        // not set to "include".
        moduleinfo.flags |= VLD_MODULE_EXCLUDED;
    }

    // Is the module specifically included in memory leak detection?
    if (!(vld.m_optflags & VLD_OPT_MODULE_LIST_INCLUDE) ||
        ((vld.m_optflags & VLD_OPT_MODULE_LIST_INCLUDE) && (wcsstr(vld.m_modulelist, modulename) == NULL))) {
        // The module is neither spcifically excluded nor is it specifcally
        // included. 
        if ((symbolsloaded == TRUE) && (moduleimageinfo.SymType == SymExport)) {
            // The only debugging information in the module was generated from a
            // DLL export table. This is a good indication that the module is a
            // system DLL and it is not likely that the user is interested in
            // checking system DLLs for memory leaks.
            moduleinfo.flags |= VLD_MODULE_EXCLUDED;
        }

    }


    // Insert the module's information into the module set and attach to the
    // module.
    vld.m_moduleset->insert(moduleinfo);
    patchmodule((HMODULE)modulebase, m_patchtable, tablesize);

    if (!(moduleinfo.flags & VLD_MODULE_EXCLUDED)) {
        // Keep a total of how many modules are actually being checked for
        // leaks.
        vld.m_modulesincluded++;
    }

    return TRUE;
}

// buildsymbolsearchpath - Builds the symbol search path for the symbol handler.
//   This helps the symbol handler find the symbols for the application being
//   debugged.
//
//  Return Value:
//
//    Returns a string containing the search path. The caller is responsible for
//    freeing the string.
//
LPWSTR VisualLeakDetector::buildsymbolsearchpath ()
{
    WCHAR   directory [_MAX_DIR];
    WCHAR   drive [_MAX_DRIVE];
    LPWSTR  env;
    DWORD   envlen;
    size_t  index;
    size_t  length;
    HMODULE module;
    LPWSTR  path = new WCHAR [MAX_PATH];
    size_t  pos = 0;
    WCHAR   system [MAX_PATH];
    WCHAR   windows [MAX_PATH];

    // Oddly, the symbol handler ignores the link to the PDB embedded in the
    // executable image. So, we'll manually add the location of the executable
    // to the search path since that is often where the PDB will be located.
    path[0] = L'\0';
    module = GetModuleHandle(NULL);
    GetModuleFileName(module, path, MAX_PATH);
    _wsplitpath(path, drive, directory, NULL, NULL);
    wcsncpy(path, drive, MAX_PATH);
    strapp(&path, directory);

    // When the symbol handler is given a custom symbol search path, it will no
    // longer search the default directories (working directory, system root,
    // etc). But we'd like it to still search those directories, so we'll add
    // them to our custom search path.
    //
    // Append the working directory.
    strapp(&path, L";.\\");

    // Append the Windows directory.
    if (GetWindowsDirectory(windows, MAX_PATH) != 0) {
        strapp(&path, L";");
        strapp(&path, windows);
    }

    // Append the system directory.
    if (GetSystemDirectory(system, MAX_PATH) != 0) {
        strapp(&path, L";");
        strapp(&path, system);
    }

    // Append %_NT_SYMBOL_PATH%.
    envlen = GetEnvironmentVariable(L"_NT_SYMBOL_PATH", NULL, 0);
    if (envlen != 0) {
        env = new WCHAR [envlen];
        if (GetEnvironmentVariable(L"_NT_SYMBOL_PATH", env, envlen) != 0) {
            strapp(&path, L";");
            strapp(&path, env);
        }
        delete [] env;
    }

    //  Append %_NT_ALT_SYMBOL_PATH%.
    envlen = GetEnvironmentVariable(L"_NT_ALT_SYMBOL_PATH", NULL, 0);
    if (envlen != 0) {
        env = new WCHAR [envlen];
        if (GetEnvironmentVariable(L"_NT_ALT_SYMBOL_PATH", env, envlen) != 0) {
            strapp(&path, L";");
            strapp(&path, env);
        }
        delete [] env;
    }

    // Remove any quotes from the path. The symbol handler doesn't like them.
    pos = 0;
    length = wcslen(path);
    while (pos < length) {
        if (path[pos] == L'\"') {
            for (index = pos; index < length; index++) {
                path[index] = path[index + 1];
            }
        }
        pos++;
    }

    return path;
}

// configure - Configures VLD using values read from the vld.ini file.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::configure ()
{
#define BSIZE 64
    WCHAR   buffer [BSIZE];
    WCHAR   filename [MAX_PATH];
    WCHAR   inipath [MAX_PATH];

    _wfullpath(inipath, L".\\vld.ini", MAX_PATH);

    // Read the boolean options.
    GetPrivateProfileString(L"Options", L"AggregateDuplicates", L"", buffer, BSIZE, inipath);
    if (wcslen(buffer) == 0) {
        m_optflags |= (VLD_DEFAULT_OPT_FLAGS & VLD_OPT_AGGREGATE_DUPLICATES);
    }
    else if (strtobool(buffer) == TRUE) {
        m_optflags |= VLD_OPT_AGGREGATE_DUPLICATES;
    }

    GetPrivateProfileString(L"Options", L"SelfTest", L"", buffer, BSIZE, inipath);
    if (wcslen(buffer) == 0) {
        m_optflags |= (VLD_DEFAULT_OPT_FLAGS & VLD_OPT_SELF_TEST);
    }
    else if (strtobool(buffer) == TRUE) {
        m_optflags |= VLD_OPT_SELF_TEST;
    }

    GetPrivateProfileString(L"Options", L"ShowUselessFrames", L"", buffer, BSIZE, inipath);
    if (wcslen(buffer) == 0) {
        m_optflags |= (VLD_DEFAULT_OPT_FLAGS & VLD_OPT_SHOW_USELESS_FRAMES);
    }
    else if (strtobool(buffer) == TRUE) {
        m_optflags |= VLD_OPT_SHOW_USELESS_FRAMES;
    }

    GetPrivateProfileString(L"Options", L"StartDisabled", L"", buffer, BSIZE, inipath);
    if (wcslen(buffer) == 0) {
        m_optflags |= (VLD_DEFAULT_OPT_FLAGS & VLD_OPT_START_DISABLED);
    }
    else if (strtobool(buffer) == TRUE) {
        m_optflags |= VLD_OPT_START_DISABLED;
    }

    // Read the integer configuration options.
    m_maxdatadump = GetPrivateProfileInt(L"Options", L"MaxDataDump", VLD_DEFAULT_MAX_DATA_DUMP, inipath);
    m_maxtraceframes = GetPrivateProfileInt(L"Options", L"MaxTraceFrames", VLD_DEFAULT_MAX_TRACE_FRAMES, inipath);

    // Read the module list and the module list mode (include or exclude).
    GetPrivateProfileString(L"Options", L"ModuleList", L"", m_modulelist, MAXMODULELISTLENGTH, inipath);
    wcslwr(m_modulelist);
    GetPrivateProfileString(L"Options", L"ModuleListMode", L"", buffer, BSIZE, inipath);
    if (wcslen(buffer) == 0) {
        m_optflags |= (VLD_DEFAULT_OPT_FLAGS & VLD_OPT_MODULE_LIST_INCLUDE);
    }
    else if (wcsicmp(buffer, L"include") == 0) {
        m_optflags |= VLD_OPT_MODULE_LIST_INCLUDE;
    }

    // Read the report destination (debugger, file, or both).
    GetPrivateProfileString(L"Options", L"ReportFile", VLD_DEFAULT_REPORT_FILE_NAME, filename, MAX_PATH, inipath);
    if (wcslen(filename) == 0) {
        wcsncpy(filename, VLD_DEFAULT_REPORT_FILE_NAME, MAX_PATH);
    }
    _wfullpath(m_reportfilepath, filename, MAX_PATH);
    GetPrivateProfileString(L"Options", L"ReportTo", L"", buffer, BSIZE, inipath);
    if (wcsicmp(buffer, L"both") == 0) {
        m_optflags |= (VLD_OPT_REPORT_TO_DEBUGGER | VLD_OPT_REPORT_TO_FILE);
    }
    else if (wcsicmp(buffer, L"debugger") == 0) {
        m_optflags |= VLD_OPT_REPORT_TO_DEBUGGER;
    }
    else if (wcsicmp(buffer, L"file") == 0) {
        m_optflags |= VLD_OPT_REPORT_TO_FILE;
    }
    else {
        m_optflags |= (VLD_DEFAULT_OPT_FLAGS & (VLD_OPT_REPORT_TO_DEBUGGER | VLD_OPT_REPORT_TO_FILE));
    }

    // Read the report file encoding (ascii or unicode).
    GetPrivateProfileString(L"Options", L"ReportEncoding", L"", buffer, BSIZE, inipath);
    if (wcsicmp(buffer, L"unicode") == 0) {
        m_optflags |= VLD_OPT_UNICODE_REPORT;
    }
    else {
        m_optflags |= (VLD_DEFAULT_OPT_FLAGS & VLD_OPT_UNICODE_REPORT);
    }
    if ((m_optflags & VLD_OPT_UNICODE_REPORT) && !(m_optflags & VLD_OPT_REPORT_TO_FILE)) {
        // If Unicode report encoding is enabled, then the report needs to be
        // sent to a file because the debugger will not display Unicode
        // characters, it will display question marks in their place instead.
        m_optflags |= VLD_OPT_REPORT_TO_FILE;
        m_status |= VLD_STATUS_FORCE_REPORT_TO_FILE;
    }
}

// detachfrommodule - Callback function for EnumerateLoadedModules64 that
//   detaches Visual Leak Detector from the specified module. If the specified
//   module has not previously been attached to, then calling this function will
//   not actually result in any changes.
//
//  - modulepath (IN): String containing the name, which may inlcude a path, of
//      the module to detach from (ignored).
//
//  - modulebase (IN): Base address of the module.
//
//  - modulesize (IN): Total size of the module (ignored).
//
//  - context (IN): User-supplied context (ignored).
//
//  Return Value:
//
//    Always returns TRUE.
//
BOOL VisualLeakDetector::detachfrommodule (PCWSTR modulepath, DWORD64 modulebase, ULONG modulesize, PVOID context)
{
    UINT tablesize = sizeof(m_patchtable) / sizeof(patchentry_t);

    restoremodule((HMODULE)modulebase, m_patchtable, tablesize);

    return TRUE;
}

// DidAlloc - Calls to IMalloc::DidAlloc will end up here. This function is just
//   a wrapper around the system implementation of IMalloc::DidAlloc. All
//   arguments passed to this function are passed on to the system
//   implementation of IMalloc::DidAlloc without modification.
//
//  - mem (IN): Pointer to a memory block to inquire about.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::DidAlloc.
//
INT VisualLeakDetector::DidAlloc (LPVOID mem)
{
    return m_imalloc->DidAlloc(mem);
}

// enabled - Determines if memory leak detection is enabled for the current
//   thread.
//
//  Return Value:
//
//    Returns true if Visual Leak Detector is enabled for the current thread.
//    Otherwise, returns false.
//
BOOL VisualLeakDetector::enabled ()
{
    UINT32 threadstatus;

    threadstatus = vldtls.status;
    if (threadstatus == VLD_TLS_UNINITIALIZED) {
        // TLS is uninitialized for the current thread. Use the default state.
        if (m_optflags & VLD_OPT_START_DISABLED) {
            threadstatus = VLD_TLS_DISABLED;
        }
        else {
            threadstatus = VLD_TLS_ENABLED;
        }
        // Initialize TLS for this thread.
        vldtls.status = threadstatus;
    }

    return ((threadstatus & VLD_TLS_ENABLED) != 0);
}

VOID VisualLeakDetector::enter ()
{    
    vldtls.level++;
}

// eraseduplicates - Erases, from the block maps, blocks that appear to be
//   duplicate leaks of an already identified leak.
//
//  - element (IN): Iterator referencing the block of which to search for
//      duplicates.
//
//  Return Value:
//
//    Returns the number of duplicate blocks erased from the block map.
//
SIZE_T VisualLeakDetector::eraseduplicates (const BlockMap::Iterator &element)
{
    BlockMap::Iterator  blockit;
    BlockMap           *blockmap;
    blockinfo_t        *elementinfo;
    SIZE_T              erased = 0;
    HeapMap::Iterator   heapit;
    blockinfo_t        *info;
    BlockMap::Iterator  previt;

    elementinfo = (*element).second;

    // Iteratate through all block maps, looking for blocks with the same size
    // and callstack as the specified element.
    for (heapit = m_heapmap->begin(); heapit != m_heapmap->end(); ++heapit) {
        blockmap = (*heapit).second;
        for (blockit = blockmap->begin(); blockit != blockmap->end(); ++blockit) {
            if (blockit == element) {
                // Don't delete the element of which we are searching for
                // duplicates.
                continue;
            }
            info = (*blockit).second;
            if ((info->size == elementinfo->size) && (info->callstack == elementinfo->callstack)) {
                // Found a duplicate. Erase it.
                delete info;
                previt = blockit - 1;
                blockmap->erase(blockit);
                blockit = previt;
                erased++;
            }
        }
    }

    return erased;
}

// Free - Calls to IMalloc::Free will end up here. This function calls VLD's
//   free tracking function and then invokes the system implementation of
//   IMalloc::Free. All arguments passed to this function are passed on to the
//   system implementation of IMalloc::Free without modification.
//
//  - mem (IN): Pointer to the memory block to be freed.
//
//  Return Value:
//
//    Returns the return value from the system implementation of IMalloc::Free.
//
VOID VisualLeakDetector::Free (LPVOID mem)
{
    vld.enter();
    unmapblock(processheap, mem);
    m_imalloc->Free(mem);
    vld.leave();
}

// GetSize - Calls to IMalloc::GetSize will end up here. This function is just a
//   wrapper around the system implementation of IMalloc::GetSize. All arguments
//   passed to this function are passed on to the system implementation of
//   IMalloc::GetSize without modification.
//
//  - mem (IN): Pointer to the memory block to inquire about.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::GetSize.
//
ULONG VisualLeakDetector::GetSize (LPVOID mem)
{
    return m_imalloc->GetSize(mem);
}

// HeapMinimize - Calls to IMalloc::HeapMinimize will end up here. This function
//   is just a wrapper around the system implementation of
//   IMalloc::HeapMinimize.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::HeapMinimize ()
{
    m_imalloc->HeapMinimize();
}

BOOL VisualLeakDetector::isvldaddress (SIZE_T address)
{
    moduleinfo_t        moduleinfo;
    ModuleSet::Iterator moduleit;

    moduleinfo.addrhigh = address;
    moduleinfo.addrlow = address;
    moduleit = m_moduleset->find(moduleinfo);
    if ((moduleit != m_moduleset->end()) && ((*moduleit).flags & VLD_MODULE_VLDDLL)) {
        // The address resides in VLD's virtual address space.
        return TRUE;
    }
    return FALSE;
}

VOID VisualLeakDetector::leave ()
{
    vldtls.level--;
}

// mapblock - Tracks memory allocations. Information about allocated blocks is
//   collected and then the block is mapped to this information.
//
//  - address (IN): Address from the module which initiated this allocation
//      request (e.g. return address from the original call). This parameter is
//      used to identify the module allocating the block. Some modules may not
//      be included in leak detection.
//
//  - heap (IN): Handle to the heap from which the block has been allocated.
//
//  - mem (IN): Pointer to the memory block being allocated.
//
//  - size (IN): Size, in bytes, of the memory block being allocated.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::mapblock (SIZE_T address, HANDLE heap, LPCVOID mem, SIZE_T size)
{
    blockinfo_t         *blockinfo;
    BlockMap::Iterator   blockit;
    BlockMap            *blockmap;
    HeapMap::Iterator    heapit;
    moduleinfo_t         moduleinfo;
    ModuleSet::Iterator  moduleit;
    static SIZE_T        serialnumber = 0;

    if (!enabled()) {
        // Memory leak detection is disabled. Don't track allocations.
        return;
    }

    if (vldtls.level > 1) {
        // Don't count nested calls as allocations, otherwise it leads to bogus
        // double (or even more) allocations.
        return;
    }

    moduleinfo.addrlow = address;
    moduleinfo.addrhigh = address;
    moduleit = m_moduleset->find(moduleinfo);
    if ((moduleit != m_moduleset->end()) && ((*moduleit).flags & VLD_MODULE_EXCLUDED)) {
        // The module that initiated this request is excluded from leak
        // detection. Don't track this block.
        return;
    }

    EnterCriticalSection(&m_maplock);

    // Record the block's information.
    blockinfo = new blockinfo_t;
    blockinfo->callstack.getstacktrace(m_maxtraceframes);
    blockinfo->serialnumber = serialnumber++;
    blockinfo->size = size;

    // Insert the block's information into the block map.
    heapit = m_heapmap->find(heap);
    if (heapit == m_heapmap->end()) {
        // We haven't mapped this heap to a block map yet. Do it now.
        mapheap(heap);
        heapit = m_heapmap->find(heap);
    }
    blockmap = (*heapit).second;
    blockit = blockmap->insert(mem, blockinfo);
    if (blockit == blockmap->end()) {
        // A block with this address has already been allocated. The
        // previously allocated block must have been freed (probably by some
        // mechanism unknown to VLD), or the heap wouldn't have allocated it
        // again. Replace the previously allocated info with the new info.
        blockit = blockmap->find(mem);
        delete (*blockit).second;
        blockmap->erase(blockit);
        blockmap->insert(mem, blockinfo);
    }

    LeaveCriticalSection(&m_maplock);
}

// mapheap - Tracks heap creation. Creates a block map for tracking individual
//   allocations from the newly created heap and then maps the heap to this
//   block map.
//
//  - heap (IN): Handle to the newly created heap.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::mapheap (HANDLE heap)
{
    BlockMap          *blockmap;
    HeapMap::Iterator  heapit;

    EnterCriticalSection(&m_maplock);

    if ((crtheap == NULL) && (heap != processheap)) {
        // The static CRT heap is always the first one created. XXX is this reliable?
        crtheap = heap;
    }

    // Create a new block map for this heap and insert it into the heap map.
    blockmap = new BlockMap;
    blockmap->reserve(BLOCKMAPRESERVE);
    heapit = m_heapmap->insert(heap, blockmap);
    if (heapit == m_heapmap->end()) {
        // Somehow this heap has been created twice without being destroyed,
        // or at least it was destroyed without VLD's knowledge. Unmap the heap
        // from the existing block map, and remap it to the new one.
        report(L"WARNING: Visual Leak Detector detected a duplicate heap.\n");
        heapit = m_heapmap->find(heap);
        unmapheap((*heapit).first);
        m_heapmap->insert(heap, blockmap);
    }

    LeaveCriticalSection(&m_maplock);
}

// QueryInterface - Calls to IMalloc::QueryInterface will end up here. This
//   function is just a wrapper around the system implementation of
//   IMalloc::QueryInterface. All arguments passed to this function are
//   passed on to IMalloc::QueryInterface without modification.
//
//  - iid (IN): COM interface ID to query about.
//
//  - object (IN): Address of a pointer to receive the requested interface
//      pointer.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::QueryInterface.
//
HRESULT VisualLeakDetector::QueryInterface (REFIID iid, LPVOID *object)
{
    return m_imalloc->QueryInterface(iid, object);
}

// Realloc - Calls to IMalloc::Realloc will end up here. This function invokes
//   the system implementation of IMalloc::Realloc and then calls VLD's
//   reallocation tracking function. All arguments passed to this function are
//   passed on to the system implementation of IMalloc::Realloc without
//   modification.
//
//  - mem (IN): Pointer to the memory block to reallocate.
//
//  - size (IN): Size, in bytes, of the memory block to reallocate.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::Realloc.
//
LPVOID VisualLeakDetector::Realloc (LPVOID mem, ULONG size)
{
    LPVOID newmem;
    SIZE_T ra;

    vld.enter();
    newmem = m_imalloc->Realloc(mem, size);
    if (newmem != NULL) {
        RETURNADDRESS(ra);
        remapblock(ra, processheap, mem, newmem, (SIZE_T)size);
    }
    vld.leave();

    return newmem;
}

// Release - Calls to IMalloc::Release will end up here. This function is just
//   a wrapper around the system implementation of IMalloc::Release.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::Release.
//
ULONG VisualLeakDetector::Release ()
{
    return m_imalloc->Release();
}

// remapblock - Tracks reallocations. Unmaps a block from its previously
//   collected information and remaps it to updated information.
//
//  Note:If the block itself remains at the same address, then the block's
//   information can simply be updated rather than having to actually erase and
//   reinsert the block.
//
//  - heap (IN): Handle to the heap from which the memory is being reallocated.
//
//  - mem (IN): Pointer to the memory block being reallocated.
//
//  - newmem (IN): Pointer to the memory block being returned to the caller
//      that requested the reallocation. This pointer may or may not be the same
//      as the original memory block (as pointed to by "mem").
//
//  - size (IN): Size, in bytes, of the new memory block.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::remapblock (SIZE_T address, HANDLE heap, LPCVOID mem, LPCVOID newmem, SIZE_T size)
{
    BlockMap::Iterator  blockit;
    BlockMap           *blockmap;
    HeapMap::Iterator   heapit;
    blockinfo_t        *info;

    if (!enabled()) {
        // Memory leak detection is disabled. Don't track allocations.
        return;
    }

    if (vldtls.level > 1) {
        // Don't count nested calls as allocations, otherwise it leads to bogus
        // double (or even more) allocations.
        return;
    }

    EnterCriticalSection(&m_maplock);

    if (newmem != mem) {
        // The block was not reallocated in-place. Instead the old block was
        // freed and a new block allocated to satisfy the new size.
        unmapblock(heap, mem);
        mapblock(address, heap, newmem, size);
    }
    else {
        // The block was reallocated in-place. Find the existing blockinfo_t
        // entry in the block map and update it with the new callstack and size.
        heapit = m_heapmap->find(heap);
        if (heapit == m_heapmap->end()) {
            // We haven't mapped this heap to a block map yet. Obviously the
            // block has also not been mapped to a blockinfo_t entry yet either,
            // so treat this reallocation as a brand-new allocation (this will
            // also map the heap to a new block map).
            mapblock(address, heap, newmem, size);
        }
        else {
            // Find the block's blockinfo_t structure so that we can update it.
            blockmap = (*heapit).second;
            blockit = blockmap->find(mem);
            if (blockit == blockmap->end()) {
                // The block hasn't been mapped to a blockinfo_t entry yet.
                // Treat this reallocation as a new allocation.
                mapblock(address, heap, newmem, size);
            }
            else {
                // Found the blockinfo_t entry for this block. Update it with
                // a new callstack and new size.
                info = (*blockit).second;
                info->callstack.clear();
                info->callstack.getstacktrace(m_maxtraceframes);
                info->size = size;
            }
        }
    }

    LeaveCriticalSection(&m_maplock);
}

// reportconfig - Generates a brief report summarizing Visual Leak Detector's
//   configuration, as loaded from the vld.ini file.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::reportconfig ()
{
    if (m_optflags & VLD_OPT_AGGREGATE_DUPLICATES) {
        report(L"    Aggregating duplicate leaks.\n");
    }
    if (m_maxdatadump != VLD_DEFAULT_MAX_DATA_DUMP) {
        if (m_maxdatadump == 0) {
            report(L"    Suppressing data dumps.\n");
        }
        else {
            report(L"    Limiting data dumps to %lu bytes.\n", m_maxdatadump);
        }
    }
    if (m_maxtraceframes != VLD_DEFAULT_MAX_TRACE_FRAMES) {
        report(L"    Limiting stack traces to %u frames.\n", m_maxtraceframes);
    }
    if (m_optflags & VLD_OPT_SELF_TEST) {
        report(L"    Perfoming a memory leak self-test.\n");
    }
    if (m_optflags & VLD_OPT_SHOW_USELESS_FRAMES) {
        report(L"    Showing useless frames.\n");
    }
    if (m_optflags & VLD_OPT_START_DISABLED) {
        report(L"    Starting with memory leak detection disabled.\n");
    }
}

// reportleaks - Generates a memory leak report when the process terminates.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::reportleaks ()
{
    LPCVOID             address;
    LPCVOID             block;
    BlockMap::Iterator  blockit;
    BlockMap           *blockmap;
    _CrtMemBlockHeader *crtheader;
    SIZE_T              duplicates;
    HANDLE              heap;
    HeapMap::Iterator   heapit;
    blockinfo_t        *info;
    SIZE_T              leaksfound = 0;
    SIZE_T              size;

    EnterCriticalSection(&m_maplock);

    // Iterate through all blocks in all heaps.
    for (heapit = m_heapmap->begin(); heapit != m_heapmap->end(); ++heapit) {
        heap = (*heapit).first;
        blockmap = (*heapit).second;
        for (blockit = blockmap->begin(); blockit != blockmap->end(); ++blockit) {
            // Found a block which is still in the BlockMap. We've identified a
            // potential memory leak.
            block = (*blockit).first;
            info = (*blockit).second;
            address = block;
            size = info->size;
            if ((heap == crtheap) || (heap == dllcrtheap)) {
                // This block is allocated to a CRT heap, so the block has a CRT
                // memory block header prepended to it.
                crtheader = (_CrtMemBlockHeader*)block;
                if (_BLOCK_TYPE(crtheader->nBlockUse) == _CRT_BLOCK) {
                    // This block is marked as being used internally by the CRT.
                    // The CRT will free the block after VLD is destroyed.
                    continue;
                }
                // The CRT header is more or less transparent to the user, so
                // the information about the contained block will probably be
                // more useful to the user. Accordingly, that's the information
                // we'll include in the report.
                address = pbData(block);
                size = crtheader->nDataSize;
            }
            // It looks like a real memory leak.
            if (leaksfound == 0) {
                report(L"WARNING: Visual Leak Detector detected memory leaks!\n");
            }
            leaksfound++;
            report(L"---------- Block %ld at " ADDRESSFORMAT L": %u bytes ----------\n", info->serialnumber, address, size);
            if (m_optflags & VLD_OPT_AGGREGATE_DUPLICATES) {
                // Aggregate all other leaks which are duplicates of this one
                // under this same heading, to cut down on clutter.
                duplicates = eraseduplicates(blockit);
                if (duplicates) {
                    report(L"A total of %lu leaks match this size and call stack. Showing only the first one.\n", duplicates + 1);
                    leaksfound += duplicates;
                }
            }
            // Dump the call stack.
            report(L"  Call Stack:\n");
            info->callstack.dump((m_optflags & VLD_OPT_SHOW_USELESS_FRAMES) != 0);
            // Dump the data in the user data section of the memory block.
            if (m_maxdatadump != 0) {
                report(L"  Data:\n");
                if (m_optflags & VLD_OPT_UNICODE_REPORT) {
                    dumpmemoryw(address, (m_maxdatadump < size) ? m_maxdatadump : size);
                }
                else {
                    dumpmemorya(address, (m_maxdatadump < size) ? m_maxdatadump : size);
                }
            }
            report(L"\n");
        }
    }

    LeaveCriticalSection(&m_maplock);

    // Show a summary.
    if (!leaksfound) {
        report(L"No memory leaks detected.\n");
    }
    else {
        report(L"Visual Leak Detector detected %lu memory leak", leaksfound);
        report((leaksfound > 1) ? L"s.\n" : L".\n");
    }

    // Free resources used by the symbol handler.
    if (!SymCleanup(currentprocess)) {
        report(L"WARNING: Visual Leak Detector: The symbol handler failed to deallocate resources (error=%lu).\n", GetLastError());
    }
}

// unmapblock - Tracks memory blocks that are freed. Unmaps the specified block
//   from the block's information, relinquishing internally allocated resources.
//
//  - heap (IN): Handle to the heap to which this block is being freed.
//
//  - mem (IN): Pointer to the memory block being freed.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::unmapblock (HANDLE heap, LPCVOID mem)
{
    BlockMap::Iterator  blockit;
    BlockMap           *blockmap;
    HeapMap::Iterator   heapit;
    blockinfo_t        *info;

    if (vldtls.level > 1) {
        // Don't count nested calls as frees, otherwise it may lead to bogus
        // double (or even more) frees.
        return;
    }

    EnterCriticalSection(&m_maplock);

    // Find this heap's block map.
    heapit = m_heapmap->find(heap);
    if (heapit == m_heapmap->end()) {
        // We don't have a block map for this heap. We must not have monitored
        // this allocation (probably happened before VLD was initialized).
        LeaveCriticalSection(&m_maplock);
        return;
    }
    // Find this block in the block map.
    blockmap = (*heapit).second;
    blockit = blockmap->find(mem);
    if (blockit == blockmap->end()) {
        // This block is not in the block map. We must not have monitored this
        // allocation (probably happened before VLD was initialized).
        LeaveCriticalSection(&m_maplock);
        return;
    }
    // Free the blockinfo_t structure and erase it from the block map.
    info = (*blockit).second;
    delete info;
    blockmap->erase(blockit);

    LeaveCriticalSection(&m_maplock);
}

// unmapheap - Tracks heap destruction. Unmaps the specified heap from its block
//   map. The block map is cleared and deleted, relinquishing internally
//   allocated resources.
//
//  - heap (IN): Handle to the heap which is being destroyed.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::unmapheap (HANDLE heap)
{
    BlockMap::Iterator  blockit;
    BlockMap           *blockmap;
    HeapMap::Iterator   heapit;

    EnterCriticalSection(&m_maplock);

    // Find this heap's block map.
    heapit = m_heapmap->find(heap);
    if (heapit == m_heapmap->end()) {
        // This heap hasn't been mapped. We must not have monitored this heap's
        // creation (probably happened before VLD was initialized).
        LeaveCriticalSection(&m_maplock);
        return;
    }

    blockmap = (*heapit).second;
    // Free all of the blockinfo_t structures stored in the block map.
    for (blockit = blockmap->begin(); blockit != blockmap->end(); ++blockit) {
        delete (*blockit).second;
    }
    delete blockmap;

    // Remove this heap's block map from the heap map.
    m_heapmap->erase(heapit);

    LeaveCriticalSection(&m_maplock);
}
