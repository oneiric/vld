////////////////////////////////////////////////////////////////////////////////
//  $Id: vld.cpp,v 1.34 2006/01/22 17:28:45 db Exp $
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
#define DBGHELP_TRANSLATE_TCHAR
#include <dbghelp.h>    // Provides stack walking and symbol handling services.
#define _CRTBLD         // Force dbgint.h to allow us to include it.
#include <dbgint.h>     // Provides access to the heap internals, specifically the memory block header structure.
#undef _CRTBLD
#define VLDBUILD        // Declares that we are building Visual Leak Detector.
#include "callstack.h"  // Provides a custom class for handling call stacks
#include "map.h"        // Provides a custom STL-like map template
#include "utility.h"    // Provides various utility functions.
#include "vldheap.h"    // Provides internal new and delete operators.
#include "vldint.h"     // Provides access to the Visual Leak Detector internals.

#define HEAPMAPRESERVE  2  // Usually there won't be more than a few heaps in the process, so this should be small
#define BLOCKMAPRESERVE 64 // This should strike a balance between memory use and a desire to minimize heap hits

// Imported global variables.
extern vldblockheader_t *vldblocklist;
extern HANDLE            vldheap;

// Global variables.
static HANDLE crtheap = NULL;                  // Handle to the static CRT heap.
HANDLE        currentprocess;                  // Pseudo-handle for the current process.
HANDLE        currentthread;                   // Pseudo-handle for the current thread.
static HANDLE dllcrtheap = (HANDLE)0xffffffff; // Internal pseudo-handle for the shared DLL CRT heap.

// The import patch table: lists the heap API imports that VLD patches through
// to replacement functions provided by VLD. Having this table simply makes it
// more convenient to add/remove API patches.
static importpatchentry_t importpatchtable [] = {
    "kernel32.dll", "HeapAlloc",         VisualLeakDetector::_HeapAlloc,
    "kernel32.dll", "HeapCreate",        VisualLeakDetector::_HeapCreate,
    "kernel32.dll", "HeapDestroy",       VisualLeakDetector::_HeapDestroy,
    "kernel32.dll", "HeapFree",          VisualLeakDetector::_HeapFree,
    "kernel32.dll", "HeapReAlloc",       VisualLeakDetector::_HeapReAlloc,
    /*"kernel32.dll", "LoadLibraryExW",    VisualLeakDetector::_LoadLibraryExW,*/
    "msvcrtd.dll",  "??2@YAPAXI@Z",      VisualLeakDetector::_malloc,      // operator new
    "msvcrtd.dll",  "??2@YAPAXIHPBDH@Z", VisualLeakDetector::__malloc_dbg, // operator new (debug)
    "msvcrtd.dll",  "_free_dbg",         VisualLeakDetector::__free_dbg,
    "msvcrtd.dll",  "_malloc_dbg",       VisualLeakDetector::__malloc_dbg,
    "msvcrtd.dll",  "free",              VisualLeakDetector::_free,
    "msvcrtd.dll",  "malloc",            VisualLeakDetector::_malloc
};

// The one and only VisualLeakDetector object instance.
__declspec(dllexport) VisualLeakDetector visualleakdetector;

// Constructor - Initializes private data, loads configuration options, and
//   patches Windows heap APIs to redirect to VLD's heap handlers.
//
VisualLeakDetector::VisualLeakDetector ()
{
    WCHAR bom = BOM; // Unicode byte-order mark.

    currentprocess = GetCurrentProcess();
    currentthread = GetCurrentThread();
    vldheap = HeapCreate(0x0, 0, 0);

    // Initialize private data.
    InitializeCriticalSection(&m_heaplock);
    m_heapmap        = new HeapMap;
    m_maxdatadump    = 0xffffffff;
    m_maxtraceframes = 0xffffffff;
    wcsnset(m_modulelist, L'\0', MAXMODULELISTLENGTH);
    m_modulespatched = 0;
    m_optflags       = 0x0;
    m_selftestfile   = __FILE__;
    m_status         = 0x0;
    m_tlsindex       = TlsAlloc();

    m_heapmap->reserve(HEAPMAPRESERVE);

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

    if (m_tlsindex == TLS_OUT_OF_INDEXES) {
        report(L"ERROR: Visual Leak Detector: Couldn't allocate thread local storage.\n"
               L"Visual Leak Detector is NOT installed!\n");
    }
    else {
        // For each loaded module, patch the Windows heap APIs.
        EnumerateLoadedModules64(currentprocess, patchapis, NULL);

        report(L"Visual Leak Detector Version " VLDVERSION L" installed.\n");
        if (m_status & VLD_STATUS_FORCE_REPORT_TO_FILE) {
            // The report is being forced to a file. Let the human know why.
            report(L"NOTE: Visual Leak Detector: Unicode-encoded reporting has been enabled, but the\n"
                   L"  debugger is the only selected report destination. The debugger cannot display\n"
                   L"  Unicode characters, so the report will also be sent to a file. If no file has\n"
                   L"  been specified, the default file name is \"" VLD_DEFAULT_REPORT_FILE_NAME L"\".\n");

        }
        reportconfig();
        m_status |= VLD_STATUS_INSTALLED;
    }
}

// Destructor - Restores Windows heap APIs, frees internally allocated
//   resources, and generates the memory leak report.
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

    if (m_status & VLD_STATUS_INSTALLED) {
        // For each loaded module, restore the Windows heap APIs.
        EnumerateLoadedModules64(currentprocess, restoreapis, NULL);
        if (m_status & VLD_STATUS_NEVER_ENABLED) {
            // Visual Leak Detector started with leak detection disabled and
            // it was never enabled at runtime. A lot of good that does.
            report(L"WARNING: Visual Leak Detector: Memory leak detection was never enabled.\n");
        }
        else if (m_modulespatched == 0) {
            // No modules had any of their API's patched.
            report(L"WARNING: Visual Leak Detector did not attach to any loaded modules.\n"
                   L"  No memory leak detection was actually done. Please check the \"ModuleList\"\n"
                   L"  option in vld.ini and be sure that at least one module is being included,\n"
                   L"  and that no module names have been misspelt.\n");
        }
        else {
            // Generate a memory leak report.
            reportleaks();
        }
    }

    EnterCriticalSection(&m_heaplock);

    // Free internally allocated resources.
    for (heapit = m_heapmap->begin(); heapit != m_heapmap->end(); ++heapit) {
        blockmap = (*heapit).second;
        for (blockit = blockmap->begin(); blockit != blockmap->end(); ++blockit) {
            delete (*blockit).second;
        }
        delete blockmap;
    }
    delete m_heapmap;

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

    LeaveCriticalSection(&m_heaplock);
    DeleteCriticalSection(&m_heaplock);

    report(L"Visual Leak Detector is now exiting.\n");

    if (m_reportfile != NULL) {
        fclose(m_reportfile);
    }
}

// __free_dbg - Calls to _free_dbg are patched through to this function. This
//   function calls VLD's free tracking function and then invokes the real
//   _free_dbg. All arguments passed to this function are passed on to the real
//   _free_dbg without modification.
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
    EnterCriticalSection(&visualleakdetector.m_heaplock);

    visualleakdetector.unmapblock(dllcrtheap, (LPVOID)mem);

    LeaveCriticalSection(&visualleakdetector.m_heaplock);

    _free_dbg(mem, type);
}

// __malloc_dbg - Calls to _malloc_dbg are patched through to this function.
//   This function invokes the real _malloc_dbg and then calls VLD's allocation
//   tracking function. All arguments passed to this function are passed on to
//   the real _malloc_dbg without modification.
//
//   Note: Only modules dynamically linked to the CRT will import _malloc_dbg,
//     so this function only allocates blocks from the shared DLL CRT heap.
//
//  - size (IN): The size of the memory block to be allocated.
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
    void *block;

    EnterCriticalSection(&visualleakdetector.m_heaplock);

    block = _malloc_dbg(size, type, file, line);
    if (block != NULL) {
        visualleakdetector.mapblock(dllcrtheap, (LPVOID)block, (DWORD)size);
    }

    LeaveCriticalSection(&visualleakdetector.m_heaplock);

    return block;
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
    EnterCriticalSection(&visualleakdetector.m_heaplock);

    visualleakdetector.unmapblock(dllcrtheap, (LPVOID)mem);

    LeaveCriticalSection(&visualleakdetector.m_heaplock);

    free(mem);
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
//  - bytes (IN): Size, in bytes, of the block to allocate.
//
//  Return Value:
//
//    Returns the return value from HeapAlloc.
//
LPVOID VisualLeakDetector::_HeapAlloc (HANDLE heap, DWORD flags, SIZE_T bytes)
{
    LPVOID block;

    EnterCriticalSection(&visualleakdetector.m_heaplock);
    
    block = HeapAlloc(heap, flags, bytes);
    if (block != NULL) {
        visualleakdetector.mapblock(heap, block, bytes);
    }

    LeaveCriticalSection(&visualleakdetector.m_heaplock);

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
    HANDLE heap = HeapCreate(options, initsize, maxsize);

    EnterCriticalSection(&visualleakdetector.m_heaplock);

    if (crtheap == NULL) {
        // The static CRT heap is always the first one created. XXX is this reliable?
        crtheap = heap;
    }

    visualleakdetector.mapheap(heap);

    LeaveCriticalSection(&visualleakdetector.m_heaplock);

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
    EnterCriticalSection(&visualleakdetector.m_heaplock);

    visualleakdetector.unmapheap(heap);

    LeaveCriticalSection(&visualleakdetector.m_heaplock);

    return HeapDestroy(heap);
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
    EnterCriticalSection(&visualleakdetector.m_heaplock);

    visualleakdetector.unmapblock(heap, mem);

    LeaveCriticalSection(&visualleakdetector.m_heaplock);

    return HeapFree(heap, flags, mem);
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
//  - bytes (IN): Size, in bytes, of the block to reallocate.
//
//  Return Value:
//
//    Returns the value returned by HeapReAlloc.
//
LPVOID VisualLeakDetector::_HeapReAlloc (HANDLE heap, DWORD flags, LPVOID mem, SIZE_T bytes)
{
    LPVOID newmem;
    
    EnterCriticalSection(&visualleakdetector.m_heaplock);

    newmem = HeapReAlloc(heap, flags, mem, bytes);

    visualleakdetector.remapblock(heap, mem, newmem, bytes);

    LeaveCriticalSection(&visualleakdetector.m_heaplock);

    return newmem;
}

/*// _LoadLibraryExW - Calls to LoadLibraryExW are patched through to this
//   function. This function invokes the real LoadLibraryExW and then patches
//   the APIs imported by the target modules and listed in the import patch
//   table to their respective VLD replacement functions. All arguments passed
//   to this function are passed on to the real LoadLibraryExW without
//   modification.
//
//  - modulename (IN): Unicode string containing the name of the module to load.
//
//  - file (IN): Reserved. Must be NULL.
//
//  - flags (IN): Load action flags.
//
//  Return Value:
//
//    Returns the value returned by LoadLibraryExW.
//
HINSTANCE VisualLeakDetector::_LoadLibraryExW (LPCWSTR modulename, HANDLE file, DWORD flags)
{
    HINSTANCE handle;
    
    handle = LoadLibraryExW(modulename, file, flags);
    if (handle != NULL) {
        // Patch the newly loaded module's APIs.
        visualleakdetector.patchapis(modulename, (DWORD64)handle, 0, NULL);
    }

    return handle;
}*/

// _malloc - Calls to malloc are patched through to this function. This function
//   invokes the real malloc and then calls VLD's allocation tracking function.
//   All arguments passed to this function are passed on to the real malloc
//   without modification.
//
//   Note: Only modules dynamically linked to the CRT will import malloc, so
//     this function only allocates blocks from the shared DLL CRT heap.
//
//  - size (IN): Size of the memory block to allocate.
//
//  Return Value:
//
//    Returns the valued returned from malloc.
//
void* VisualLeakDetector::_malloc (size_t size)
{
    void *block;

    EnterCriticalSection(&visualleakdetector.m_heaplock);
    
    block = malloc(size);
    if (block != NULL) {
        visualleakdetector.mapblock(dllcrtheap, (LPVOID)block, (SIZE_T)size);
    }

    LeaveCriticalSection(&visualleakdetector.m_heaplock);

    return block;
}

// buildsymbolsearchpath - Builds the symbol search path for the symbol handler.
//   This helps the symbol handler find the symbols for the application being
//   debugged. The default behavior of the search path doesn't appear to work
//   as documented (at least not under Visual C++ 6.0) so we need to augment the
//   default search path in order for the symbols to be found if they're in a
//   program database (PDB) file.
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

    // The documentation says that executables with associated program database
    // (PDB) files have the absolute path to the PDB embedded in them and that,
    // by default, that path is used to find the PDB. That appears to not be the
    // case (at least not with Visual C++ 6.0). So we'll manually add the
    // location of the executable (which is where the PDB is located by default)
    // to the symbol search path.
    path[0] = L'\0';
    module = GetModuleHandle(NULL);
    GetModuleFileName(module, path, MAX_PATH);
    _wsplitpath(path, drive, directory, NULL, NULL);
    strapp(&path, drive);
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
    LPCWSTR defstate;
    WCHAR   filename [MAX_PATH];
    WCHAR   inipath [MAX_PATH];

    _wfullpath(inipath, L".\\vld.ini", MAX_PATH);

    // Read the boolean options.
    defstate = booltostr(VLD_DEFAULT_OPT_FLAGS & VLD_OPT_AGGREGATE_DUPLICATES);
    GetPrivateProfileString(L"Options", L"AggregateDuplicates", defstate, buffer, BSIZE, inipath);
    if (strtobool(buffer) == TRUE) {
        m_optflags |= VLD_OPT_AGGREGATE_DUPLICATES;
    }
    defstate = booltostr(VLD_DEFAULT_OPT_FLAGS & VLD_OPT_SELF_TEST);
    GetPrivateProfileString(L"Options", L"SelfTest", defstate, buffer, BSIZE, inipath);
    if (strtobool(buffer) == TRUE) {
        m_optflags |= VLD_OPT_SELF_TEST;
    }
    defstate = booltostr(VLD_DEFAULT_OPT_FLAGS & VLD_OPT_SHOW_USELESS_FRAMES);
    GetPrivateProfileString(L"Options", L"ShowUselessFrames", defstate, buffer, BSIZE, inipath);
    if (strtobool(buffer) == TRUE) {
        m_optflags |= VLD_OPT_SHOW_USELESS_FRAMES;
    }
    defstate = booltostr(VLD_DEFAULT_OPT_FLAGS & VLD_OPT_START_DISABLED);
    GetPrivateProfileString(L"Options", L"StartDisabled", defstate, buffer, BSIZE, inipath);
    if (strtobool(buffer) == TRUE) {
        m_optflags |= VLD_OPT_START_DISABLED;
    }

    // Read the integer configuration options.
    m_maxdatadump = GetPrivateProfileInt(L"Options", L"MaxDataDump", VLD_DEFAULT_MAX_DATA_DUMP, inipath);
    m_maxtraceframes = GetPrivateProfileInt(L"Options", L"MaxTraceFrames", VLD_DEFAULT_MAX_TRACE_FRAMES, inipath);

    // Read the module list and the module list mode (include/exclude).
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

    // Read the report file encoding
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
    UINT32 status;

    status = (UINT32)TlsGetValue(m_tlsindex);
    if (status == VLD_TLS_UNINITIALIZED) {
        // TLS is uninitialized for the current thread. Use the initial state.
        if (m_optflags & VLD_OPT_START_DISABLED) {
            status = VLD_TLS_DISABLED;
        }
        else {
            status = VLD_TLS_ENABLED;
        }
        // Initialize TLS for this thread.
        TlsSetValue(m_tlsindex, (LPVOID)status);
    }

    return ((status & VLD_TLS_ENABLED) != 0);
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

// mapblock - Tracks memory allocations. Information about allocated blocks is
//   collected and then the block is mapped to this information.
//
//  - heap (IN): Handle to the heap from which the block has been allocated.
//
//  - mem (IN): Pointer to the memory block being allocated.
//
//  - size (IN): Size of the memory block being allocated.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::mapblock (HANDLE heap, LPCVOID mem, SIZE_T size)
{
    BlockMap::Iterator  blockit;
    BlockMap           *blockmap;
    _CrtMemBlockHeader *crtheader;
    HeapMap::Iterator   heapit;
    blockinfo_t        *info;
    static SIZE_T       serialnumber = 0;

    if (!enabled()) {
        // Memory leak detection is disabled. Don't track allocations.
        return;
    }

    // Record the block's information.
    info = new blockinfo_t;
    info->callstack.getstacktrace(m_maxtraceframes);
    info->serialnumber = serialnumber++;
    if (heap == crtheap) {
        // For CRT blocks, the address and size of the user-data section of the
        // block are stored in the map.
        crtheader = (_CrtMemBlockHeader*)mem;
        mem = pbData(crtheader);
        info->size = (SIZE_T)crtheader->nDataSize;
    }
    else {
        info->size = size;
    }

    // Insert the block's information into the block map.
    heapit = m_heapmap->find(heap);
    if (heapit == m_heapmap->end()) {
        // We haven't mapped this heap to a block map yet. Do it now.
        mapheap(heap);
        heapit = m_heapmap->find(heap);
    }
    blockmap = (*heapit).second;
    blockit = blockmap->insert(mem, info);
    if (blockit == blockmap->end()) {
        // A block with this address has already been allocated. The
        // previously allocated block must have been freed (probably by some
        // mechanism unknown to VLD), or the heap wouldn't have allocated it
        // again. Replace the previously allocated info with the new info.
        blockit = blockmap->find(mem);
        delete (*blockit).second;
        blockmap->erase(blockit);
        blockmap->insert(mem, info);
    }
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
}

// patchapis - Callback function for EnumerateLoadedModules64 that patches
//   certain APIs for all modules loaded in the process (unless otherwise
//   excepted). The import patch table is consulted for determining which APIs
//   exported by which modules should be patched and to which replacement
//   functions provided by VLD they should be patched through to.
//
//  - modulename (IN): Name of a module loaded in the current process.
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
BOOL VisualLeakDetector::patchapis (PCWSTR modulepath, DWORD64 modulebase, ULONG modulesize, PVOID context)
{
    LPCSTR  exportmodulename;
    WCHAR   extension [_MAX_EXT];
    WCHAR   filename [_MAX_FNAME];
    LPCSTR  importname;
    UINT    index;
#define MAXMODULENAME (_MAX_FNAME + _MAX_EXT)
    WCHAR   modulename [MAXMODULENAME + 1];
    UINT    numpatchentries = sizeof(importpatchtable) / sizeof(importpatchentry_t);
    LPCVOID replacement;

    _wsplitpath(modulepath, NULL, NULL, filename, extension);
    wcsncpy(modulename, filename, MAXMODULENAME);
    wcsncat(modulename, extension, MAXMODULENAME - wcslen(modulename));
    wcslwr(modulename);
    if (wcsstr(L"vld.dll dbghelp.dll msvcrt.dll msvcrtd.dll", modulename)) {
        // The above modules are hard-excluded from memory leak detection. VLD
        // itself must be excluded (though it has a separate self-checker that
        // does check VLD for memory leaks) to avoid infinite recursion.
        // Similarly, because VLD's allocation tracking functions depend on
        // dbghelp.dll, msvcrt.dll, and msvcrtd.dll, those too must be excluded
        // to avoid unwanted recursion.
        if ((visualleakdetector.m_optflags & VLD_OPT_MODULE_LIST_INCLUDE) &&
            (wcsstr(visualleakdetector.m_modulelist, modulename) != NULL)) {
            // This hard-excluded module was specified on the list of modules to be included.
            report(L"WARNING: Visual Leak Detector: A module, %s, specified to be included by the\n"
                   L"  \"ModuleList\" vld.ini option is hard-excluded by Visual Leak Detector.\n"
                   L"  The module will not be included in memory leak detection.\n", modulename);
        }
        return TRUE;
    }
    if (visualleakdetector.m_optflags & VLD_OPT_MODULE_LIST_INCLUDE) {
        // Only patch this module if it is in the module list.
        if (wcsstr(visualleakdetector.m_modulelist, modulename) == NULL) {
            return TRUE;
        }
    }
    else {
        // Do not patch this module if it is in the module list.
        if (wcsstr(visualleakdetector.m_modulelist, modulename) != NULL) {
            return TRUE;
        }
    }

    // Patch each API listed in the import patch table to its respective VLD
    // replacement function.
    visualleakdetector.m_modulespatched++;
    for (index = 0; index < numpatchentries; ++index) {
        exportmodulename = importpatchtable[index].exportmodulename;
        importname       = importpatchtable[index].importname;
        replacement      = importpatchtable[index].replacement;
        patchimport((HMODULE)modulebase, exportmodulename, importname, replacement);
    }

    return TRUE;
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
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::remapblock (HANDLE heap, LPCVOID mem, LPCVOID newmem, SIZE_T size)
{
    BlockMap::Iterator  blockit;
    BlockMap           *blockmap;
    HeapMap::Iterator   heapit;
    blockinfo_t        *info;

    if (newmem != mem) {
        // The block was not reallocated in-place. Instead the old block was
        // freed and a new block allocated to satisfy the new size.
        unmapblock(heap, mem);
        mapblock(heap, newmem, size);
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
            mapblock(heap, newmem, size);
        }
        else {
            // Find the block's blockinfo_t structure so that we can update it.
            blockmap = (*heapit).second;
            blockit = blockmap->find(mem);
            if (blockit == blockmap->end()) {
                // The block hasn't been mapped to a blockinfo_t entry yet.
                // Treat this reallocation as a new allocation.
                mapblock(heap, newmem, size);
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
    BlockMap::Iterator  blockit;
    BlockMap           *blockmap;
    _CrtMemBlockHeader *crtheader;
    SIZE_T              duplicates;
    HANDLE              heap;
    HeapMap::Iterator   heapit;
    blockinfo_t        *info;
    SIZE_T              leaksfound = 0;
    LPCVOID             mem;
    LPWSTR              symbolpath;

    // Initialize the symbol handler. We use it for obtaining source file/line
    // number information and function names for the memory leak report.
    symbolpath = buildsymbolsearchpath();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
    if (!SymInitialize(currentprocess, symbolpath, TRUE)) {
        report(L"WARNING: Visual Leak Detector: The symbol handler failed to initialize (error=%lu).\n"
               L"    File and function names will probably not be available in call stacks.\n", GetLastError());
    }
    delete [] symbolpath;

    EnterCriticalSection(&m_heaplock);

    // Iterate through all blocks in all heaps.
    for (heapit = m_heapmap->begin(); heapit != m_heapmap->end(); ++heapit) {
        heap = (*heapit).first;
        blockmap = (*heapit).second;
        for (blockit = blockmap->begin(); blockit != blockmap->end(); ++blockit) {
            // Found a block which is still in the BlockMap. We've identified a
            // potential memory leak.
            mem = (*blockit).first;
            info = (*blockit).second;
            if ((heap == crtheap) || (heap == dllcrtheap)) {
                // This block is allocated to a CRT heap, so the block has a CRT
                // memory block header prepended to it.
                crtheader = pHdr(mem);
                if (_BLOCK_TYPE(crtheader->nBlockUse) == _CRT_BLOCK) {
                    // This block is marked as being used internally by the CRT.
                    // The CRT will free the block after VLD is destroyed.
                    continue;
                }
            }
            // It looks like a real memory leak.
            if (leaksfound == 0) {
                report(L"WARNING: Visual Leak Detector detected memory leaks!\n");
            }
            leaksfound++;
            report(L"---------- Block %ld at " ADDRESSFORMAT L": %u bytes ----------\n", info->serialnumber, mem, info->size);
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
                    dumpmemoryw(mem, (m_maxdatadump < info->size) ? m_maxdatadump : info->size);
                }
                else {
                    dumpmemorya(mem, (m_maxdatadump < info->size) ? m_maxdatadump : info->size);
                }
            }
            report(L"\n");
        }
    }

    LeaveCriticalSection(&m_heaplock);

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

// restoreapis - Callback function for EnumerateLoadedModules64 that restores
//   all APIs previously patched through to VLD's replacement functions.
//
//  - modulename (IN): Name of a module loaded in the current process.
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
BOOL VisualLeakDetector::restoreapis (PCWSTR modulename, DWORD64 modulebase, ULONG modulesize, PVOID context)
{
    LPCSTR  exportmodulename;
    LPCSTR  importname;
    UINT    index;
    UINT    numpatchentries = sizeof(importpatchtable) / sizeof(importpatchentry_t);
    LPCVOID replacement;

    // Restore the previously patched APIs.
    for (index = 0; index < numpatchentries; ++index) {
        exportmodulename = importpatchtable[index].exportmodulename;
        importname       = importpatchtable[index].importname;
        replacement      = importpatchtable[index].replacement;
        restoreimport((HMODULE)modulebase, exportmodulename, importname, replacement);
    }

    return TRUE;
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

    // Find this heap's block map.
    heapit = m_heapmap->find(heap);
    if (heapit == m_heapmap->end()) {
        // We don't have a block map for this heap. We must not have monitored
        // this allocation (probably happened before VLD was initialized).
        return;
    }
    // Find this block in the block map.
    blockmap = (*heapit).second;
    blockit = blockmap->find(mem);
    if (blockit == blockmap->end()) {
        // This block is not in the block map. We must not have monitored this
        // allocation (probably happened before VLD was initialized).
        return;
    }
    // Free the blockinfo_t structure and erase it from the block map.
    info = (*blockit).second;
    delete info;
    blockmap->erase(blockit);
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

    // Find this heap's block map.
    heapit = m_heapmap->find(heap);
    if (heapit != m_heapmap->end()) {
        blockmap = (*heapit).second;
        // Free all of the blockinfo_t structures stored in the block map.
        for (blockit = blockmap->begin(); blockit != blockmap->end(); ++blockit) {
            delete (*blockit).second;
        }
        delete blockmap;
    }
    // Remove this heap's block map from the heap map.
    m_heapmap->erase(heapit);
}
