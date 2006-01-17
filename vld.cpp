////////////////////////////////////////////////////////////////////////////////
//  $Id: vld.cpp,v 1.31 2006/01/17 23:14:46 dmouldin Exp $
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

#define _CRTBLD
#include <dbgint.h>  // Provides access to the heap internals, specifically the memory block header structure.
#undef _CRTBLD

#define VLDBUILD     // Declares that we are building Visual Leak Detector.
#include "utility.h" // Provides various utility functions.
#include "vldheap.h" // Provides internal new and delete operators.
#include "vldint.h"  // Provides access to the Visual Leak Detector internals.

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
    currentprocess = GetCurrentProcess();
    currentthread = GetCurrentThread();
    vldheap = HeapCreate(0x0, 0, 0);

    // Initialize private data.
    InitializeCriticalSection(&m_heaplock);
    m_heapmap        = new HeapMap;
    m_maxdatadump    = 0xffffffff;
    m_maxtraceframes = 0xffffffff;
    m_optflags       = 0x0;
    m_selftestfile   = __FILE__;
    m_status         = 0x0;
    m_tlsindex       = TlsAlloc();

    m_heapmap->reserve(HEAPMAPRESERVE);
    if (m_tlsindex == TLS_OUT_OF_INDEXES) {
        report("ERROR: Visual Leak Detector: Couldn't allocate thread local storage.\n"
               "Visual Leak Detector is NOT installed!\n");
    }
    else {
        // Load configuration options.
        configure();
        if (m_optflags & VLD_OPT_SELF_TEST) {
            // Self-test mode has been enabled. Intentionally leak a small amount of
            // memory so that memory leak self-checking can be verified.
            strncpy(new char [21], "Memory Leak Self-Test", 21); m_selftestline = __LINE__;
        }
        if (m_optflags & VLD_OPT_START_DISABLED) {
            // Memory leak detection will initially be disabled.
            m_status |= VLD_STATUS_NEVER_ENABLED;
        }
        m_status |= VLD_STATUS_INSTALLED;

        // For each loaded module, patch the Windows heap APIs.
        EnumerateLoadedModules64(currentprocess, patchheapapis, NULL);
        report("Visual Leak Detector Version "VLDVERSION" installed.\n");
        reportconfig();
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
    unsigned int        internalleaks = 0;
    const char         *leakfile;
    int                 leakline;

    if (m_status & VLD_STATUS_INSTALLED) {
        // For each loaded module, restore the Windows heap APIs.
        EnumerateLoadedModules64(currentprocess, restoreheapapis, NULL);
        if (m_status & VLD_STATUS_NEVER_ENABLED) {
            // Visual Leak Detector started with leak detection disabled and
            // it was never enabled at runtime. A lot of good that does.
            report("WARNING: Visual Leak Detector: Memory leak detection was never enabled.\n");
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
        report("ERROR: Visual Leak Detector: Detected a memory leak internal to Visual Leak Detector!!\n");
        report("---------- Block %ld at "ADDRESSFORMAT": %u bytes ----------\n", header->serialnumber, BLOCKDATA(header), header->size);
        report("  Call Stack:\n");
        report("    %s (%d): Full call stack not available.\n", header->file, header->line);
        report("  Data:\n");
        dumpmemory(BLOCKDATA(header), (m_maxdatadump < header->size) ? m_maxdatadump : header->size);
        report("\n");
        header = header->next;
    }
    if (m_optflags & VLD_OPT_SELF_TEST) {
        if ((internalleaks == 1) && (strcmp(leakfile, m_selftestfile) == 0) && (leakline == m_selftestline)) {
            report("Visual Leak Detector passed the memory leak self-test.\n");
        }
        else {
            report("ERROR: Visual Leak Detector: Failed the memory leak self-test.\n");
        }
    }

    LeaveCriticalSection(&m_heaplock);
    DeleteCriticalSection(&m_heaplock);

    report("Visual Leak Detector is now exiting.\n");
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

    visualleakdetector.mapfree(dllcrtheap, mem);

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
        visualleakdetector.mapalloc(dllcrtheap, block, size);
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

    visualleakdetector.mapfree(dllcrtheap, mem);

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
LPVOID VisualLeakDetector::_HeapAlloc (HANDLE heap, DWORD flags, DWORD bytes)
{
    LPVOID block;

    EnterCriticalSection(&visualleakdetector.m_heaplock);
    
    block = HeapAlloc(heap, flags, bytes);
    if (block != NULL) {
        visualleakdetector.mapalloc(heap, block, bytes);
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

    visualleakdetector.mapcreate(heap);

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

    visualleakdetector.mapdestroy(heap);

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

    visualleakdetector.mapfree(heap, mem);

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

    visualleakdetector.maprealloc(heap, mem, newmem, bytes);

    LeaveCriticalSection(&visualleakdetector.m_heaplock);

    return newmem;
}

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
        visualleakdetector.mapalloc(dllcrtheap, block, size);
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
char* VisualLeakDetector::buildsymbolsearchpath ()
{
    char    *env;
    size_t   index;
    size_t   length;
    HMODULE  module;
    char    *path = new char [MAX_PATH];
    size_t   pos = 0;

    // The documentation says that executables with associated program database
    // (PDB) files have the absolute path to the PDB embedded in them and that,
    // by default, that path is used to find the PDB. That appears to not be the
    // case (at least not with Visual C++ 6.0). So we'll manually add the
    // location of the executable (which is where the PDB is located by default)
    // to the symbol search path.
    module = GetModuleHandle(NULL);
    GetModuleFileNameA(module, path, MAX_PATH);
    PathRemoveFileSpec(path);

    // When the symbol handler is given a custom symbol search path, it will no
    // longer search the default directories (working directory, system root,
    // etc). But we'd like it to still search those directories, so we'll add
    // them to our custom search path.
    //
    // Append the working directory.
    strapp(&path, ";.\\");

    // Append %SYSTEMROOT% and %SYSTEMROOT%\system32.
    env = getenv("SYSTEMROOT");
    if (env) {
        strapp(&path, ";");
        strapp(&path, env);
        strapp(&path, ";");
        strapp(&path, env);
        strapp(&path, "\\system32");
    }

    // Append %_NT_SYMBOL_PATH% and %_NT_ALT_SYMBOL_PATH%.
    env = getenv("_NT_SYMBOL_PATH");
    if (env) {
        strapp(&path, ";");
        strapp(&path, env);
    }
    env = getenv("_NT_ALT_SYMBOL_PATH");
    if (env) {
        strapp(&path, ";");
        strapp(&path, env);
    }

    // Remove any quotes from the path. The symbol handler doesn't like them.
    pos = 0;
    length = strlen(path);
    while (pos < length) {
        if (path[pos] == '\"') {
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
void VisualLeakDetector::configure ()
{
    const char *defstate;
    char        inipath [MAX_PATH];
#define BSIZE 16
    char        state [BSIZE];

    _fullpath(inipath, ".\\vld.ini", MAX_PATH);

    // Read the boolean options.
    defstate = booltostr(VLD_DEFAULT_OPT_FLAGS & VLD_OPT_AGGREGATE_DUPLICATES);
    GetPrivateProfileString("Options", "AggregateDuplicates", defstate, state, BSIZE, inipath);
    if (strtobool(state) == true) {
        m_optflags |= VLD_OPT_AGGREGATE_DUPLICATES;
    }
    defstate = booltostr(VLD_DEFAULT_OPT_FLAGS & VLD_OPT_SELF_TEST);
    GetPrivateProfileString("Options", "SelfTest", defstate, state, BSIZE, inipath);
    if (strtobool(state) == true) {
        m_optflags |= VLD_OPT_SELF_TEST;
    }
    defstate = booltostr(VLD_DEFAULT_OPT_FLAGS & VLD_OPT_SHOW_USELESS_FRAMES);
    GetPrivateProfileString("Options", "ShowUselessFrames", defstate, state, BSIZE, inipath);
    if (strtobool(state) == true) {
        m_optflags |= VLD_OPT_SHOW_USELESS_FRAMES;
    }
    defstate = booltostr(VLD_DEFAULT_OPT_FLAGS & VLD_OPT_START_DISABLED);
    GetPrivateProfileString("Options", "StartDisabled", defstate, state, BSIZE, inipath);
    if (strtobool(state) == true) {
        m_optflags |= VLD_OPT_START_DISABLED;
    }

    // Read the integer configuration options.
    m_maxdatadump = GetPrivateProfileInt("Options", "MaxDataDump", VLD_DEFAULT_MAX_DATA_DUMP, inipath);
    m_maxtraceframes = GetPrivateProfileInt("Options", "MaxTraceFrames", VLD_DEFAULT_MAX_TRACE_FRAMES, inipath);

    // Read string options
    GetPrivateProfileString("Options", "ModuleList", "", m_modulelist, MODULELISTLENGTH, inipath);
    _strlwr(m_modulelist);
    GetPrivateProfileString("Options", "ModuleListMode", "exclude", state, BSIZE, inipath);
    if (_stricmp(state, "include") == 0) {
        // The default behavior is to include all modules, except those listed
        // in the module list, in memory leak detection. However, the "include"
        // ModuleListMode has been specified. So, all modules will be excluded
        // except those listed in the module list.
        m_optflags |= VLD_OPT_MODULE_LIST_INCLUDE;
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
bool VisualLeakDetector::enabled ()
{
    unsigned long status;

    status = (unsigned long)TlsGetValue(m_tlsindex);
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

    return (status & VLD_TLS_ENABLED) ? true : false;
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
unsigned long VisualLeakDetector::eraseduplicates (const BlockMap::Iterator &element)
{
    BlockMap::Iterator  blockit;
    BlockMap           *blockmap;
    blockinfo_t        *elementinfo;
    unsigned long       erased = 0;
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

// mapalloc - Tracks memory allocations. Information about allocated blocks is
//   collected and then stored in the block map.
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
void VisualLeakDetector::mapalloc (HANDLE heap, LPVOID mem, SIZE_T size)
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
        info->size = crtheader->nDataSize;
    }
    else {
        info->size = size;
    }

    // Insert the block's information into the block map.
    heapit = m_heapmap->find(heap);
    if (heapit == m_heapmap->end()) {
        // We haven't created a block map for this heap yet, do it now.
        mapcreate(heap);
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

// mapcreate - Tracks heap creation. Creates a block map for tracking
//   individual allocations from the newly created heap.
//
//  - heap (IN): Handle to the newly created heap.
//
//  Return Value:
//
//    None.
//
void VisualLeakDetector::mapcreate (HANDLE heap)
{
    BlockMap          *blockmap;
    HeapMap::Iterator  heapit;

    // Create a new block map for this heap and insert it into the heap map.
    blockmap = new BlockMap;
    blockmap->reserve(BLOCKMAPRESERVE);
    heapit = m_heapmap->insert(heap, blockmap);
    if (heapit == m_heapmap->end()) {
        // Somehow this heap has been created twice without being destroyed,
        // or at least it was destroyed without VLD's knowledge. Destroy the
        // old heap and replace it with the new one.
        report("WARNING: Visual Leak Detector detected a duplicate heap.\n");
        heapit = m_heapmap->find(heap);
        mapdestroy((*heapit).first);
        m_heapmap->insert(heap, blockmap);
    }
}

// mapdestroy - Tracks heap destruction. Clears the block map associated with
//   the specified heap and relinquishes internal resources.
//
//  - heap (IN): Handle to the heap which is being destroyed.
//
//  Return Value:
//
//    None.
//
void VisualLeakDetector::mapdestroy (HANDLE heap)
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

// mapfree - Tracks blocks that are freed. Removes the information about the
//   block being freed from the block map.
//
//  - heap (IN): Handle to the heap to which this block is being freed.
//
//  - mem (IN): Pointer to the memory block being freed.
//
//  Return Value:
//
//    None.
//
void VisualLeakDetector::mapfree (HANDLE heap, LPVOID mem)
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

// maprealloc - Tracks reallocations. Updates the information stored in the
//   block map about the reallocated block, or erases the old information and
//   reinserts the new information, if needed.
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
void VisualLeakDetector::maprealloc (HANDLE heap, LPVOID mem, LPVOID newmem, SIZE_T size)
{
    BlockMap::Iterator  blockit;
    BlockMap           *blockmap;
    HeapMap::Iterator   heapit;
    blockinfo_t        *info;

    if (newmem != mem) {
        // The block was not reallocated in-place. Instead the old block was
        // freed and a new block allocated to satisfy the new size.
        mapfree(heap, mem);
        mapalloc(heap, newmem, size);
    }
    else {
        // The block was reallocated in-place. Find the existing blockinfo_t
        // entry in the block map and update it with the new callstack and size.
        heapit = m_heapmap->find(heap);
        if (heapit == m_heapmap->end()) {
            // We haven't created this heap's block map yet. Obviously the block
            // doesn't have a blockinfo_t entry yet, so treat this reallocation
            // as a brand-new allocation (this will also create a new block map
            // for the heap).
            mapalloc(heap, newmem, size);
        }
        else {
            // Find the block's blockinfo_t structure so that we can update it.
            blockmap = (*heapit).second;
            blockit = blockmap->find(mem);
            if (blockit == blockmap->end()) {
                // The block doesn't have a blockinfo_t entry yet. Treat this
                // reallocation as a new allocation.
                mapalloc(heap, newmem, size);
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

// patchheapapis - Callback function for EnumerateLoadedModules64 that patches
//   heap APIs for all modules loaded in the process (unless otherwise
//   excepted). The import patch table is consulted for determining which APIs
//   exported by which modules should be patched and to which replacement
//   functions provided by VLD they should be patched through to.
//
//  - modulename (IN): Name of a module loaded in the current process.
//
//  - modulebase (IN): Base address of the module.
//
//  - modulesize (IN): Total size of the module.
//
//  - context (IN): User-supplied context (not currently used by VLD).
//
//  Return Value:
//
//    Always returns TRUE.
//
BOOL VisualLeakDetector::patchheapapis (PCTSTR modulename, DWORD64 modulebase, ULONG modulesize, PVOID context)
{
    char   *exportmodulename;
    char   *importname;
    SIZE_T  index;
    char   *lowername;
    size_t  length = strlen(modulename) + 1;
    SIZE_T  numpatchentries = sizeof(importpatchtable) / sizeof(importpatchentry_t);
    void   *replacement;

    lowername = new char [length];
    strncpy(lowername, modulename, length);
    _strlwr(lowername);
    if (strstr("vld.dll dbghelp.dll msvcrt.dll msvcrtd.dll", lowername) != NULL) {
        // Don't patch vld.dll's, dbghelp.dll's, or msvcrt.dll's IATs. We want
        // VLD's calls to the heap APIs to go directly to the real APIs. Also,
        // VLD's replacement heap APIs depend on certain Debug Help Library and
        // CRT routines which may need to allocate memory so patching
        // dbghelp.dll's or msvcrt.dll's IATs can result in infinite recursion.
        delete lowername;
        return TRUE;
    }
    if (visualleakdetector.m_optflags & VLD_OPT_MODULE_LIST_INCLUDE) {
        // Only patch this module if it is in the module list.
        if (strstr(visualleakdetector.m_modulelist, lowername) == NULL) {
            delete lowername;
            return TRUE;
        }
    }
    else {
        // Do not patch this module if it is in the module list.
        if (strstr(visualleakdetector.m_modulelist, lowername) != NULL) {
            delete lowername;
            return TRUE;
        }
    }
    delete lowername;

    // Patch each API listed in the import patch table to its respective VLD
    // replacement function.
    for (index = 0; index < numpatchentries; ++index) {
        exportmodulename = importpatchtable[index].exportmodulename;
        importname       = importpatchtable[index].importname;
        replacement      = importpatchtable[index].replacement;
        patchimport((HMODULE)modulebase, exportmodulename, importname, replacement);
    }

    return TRUE;
}

// reportconfig - Generates a brief report summarizing Visual Leak Detector's
//   configuration, as loaded from the vld.ini file.
//
//  Return Value:
//
//    None.
//
void VisualLeakDetector::reportconfig ()
{
    if (m_optflags & VLD_OPT_AGGREGATE_DUPLICATES) {
        report("    Aggregating duplicate leaks.\n");
    }
    if (m_maxdatadump != VLD_DEFAULT_MAX_DATA_DUMP) {
        if (m_maxdatadump == 0) {
            report("    Suppressing data dumps.\n");
        }
        else {
            report("    Limiting data dumps to %lu bytes.\n", m_maxdatadump);
        }
    }
    if (m_maxtraceframes != VLD_DEFAULT_MAX_TRACE_FRAMES) {
        report("    Limiting stack traces to %lu frames.\n", m_maxtraceframes);
    }
    if (m_optflags & VLD_OPT_SELF_TEST) {
        report("    Perfoming a memory leak self-test.\n");
    }
    if (m_optflags & VLD_OPT_SHOW_USELESS_FRAMES) {
        report("    Showing useless frames.\n");
    }
    if (m_optflags & VLD_OPT_START_DISABLED) {
        report("    Starting with memory leak detection disabled.\n");
    }
}

// reportleaks - Generates a memory leak report when the process terminates.
//
//  Return Value:
//
//    None.
//
void VisualLeakDetector::reportleaks ()
{
    BlockMap::Iterator  blockit;
    BlockMap           *blockmap;
    _CrtMemBlockHeader *crtheader;
    unsigned long       duplicates;
    HANDLE              heap;
    HeapMap::Iterator   heapit;
    blockinfo_t        *info;
    unsigned long       leaksfound = 0;
    LPVOID              mem;
    char               *symbolpath;

    // Initialize the symbol handler. We use it for obtaining source file/line
    // number information and function names for the memory leak report.
    symbolpath = buildsymbolsearchpath();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
    if (!SymInitialize(currentprocess, symbolpath, TRUE)) {
        report("WARNING: Visual Leak Detector: The symbol handler failed to initialize (error=%lu).\n"
               "    File and function names will probably not be available in call stacks.\n", GetLastError());
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
                report("WARNING: Visual Leak Detector detected memory leaks!\n");
            }
            leaksfound++;
            report("---------- Block %ld at "ADDRESSFORMAT": %u bytes ----------\n", info->serialnumber, mem, info->size);
            if (m_optflags & VLD_OPT_AGGREGATE_DUPLICATES) {
                // Aggregate all other leaks which are duplicates of this one
                // under this same heading, to cut down on clutter.
                duplicates = eraseduplicates(blockit);
                if (duplicates) {
                    report("A total of %lu leaks match this size and call stack. Showing only the first one.\n", duplicates + 1);
                    leaksfound += duplicates;
                }
            }
            // Dump the call stack.
            report("  Call Stack:\n");
            info->callstack.dump((m_optflags & VLD_OPT_SHOW_USELESS_FRAMES) != 0);
            // Dump the data in the user data section of the memory block.
            if (m_maxdatadump != 0) {
                report("  Data:\n");
                dumpmemory(mem, (m_maxdatadump < info->size) ? m_maxdatadump : info->size);
            }
            report("\n");
        }
    }

    LeaveCriticalSection(&m_heaplock);

    // Show a summary.
    if (!leaksfound) {
        report("No memory leaks detected.\n");
    }
    else {
        report("Visual Leak Detector detected %lu memory leak", leaksfound);
        report((leaksfound > 1) ? "s.\n" : ".\n");
    }

    // Free resources used by the symbol handler.
    if (!SymCleanup(currentprocess)) {
        report("WARNING: Visual Leak Detector: The symbol handler failed to deallocate resources (error=%lu).\n", GetLastError());
    }
}

// restoreheapapis - Callback function for EnumerateLoadedModules64 that
//   restores all heap APIs previously patched through to VLD's replacement
//   functions.
//
//  - modulename (IN): Name of a module loaded in the current process.
//
//  - modulebase (IN): Base address of the module.
//
//  - modulesize (IN): Total size of the module.
//
//  - context (IN): User-supplied context (not currently used by VLD).
//
//  Return Value:
//
//    Always returns TRUE.
//
BOOL VisualLeakDetector::restoreheapapis (PCTSTR modulename, DWORD64 modulebase, ULONG modulesize, PVOID context)
{
    char   *exportmodulename;
    char   *importname;
    SIZE_T  index;
    SIZE_T  numpatchentries = sizeof(importpatchtable) / sizeof(importpatchentry_t);
    void   *replacement;

    // Restore the previously patched APIs.
    for (index = 0; index < numpatchentries; ++index) {
        exportmodulename = importpatchtable[index].exportmodulename;
        importname       = importpatchtable[index].importname;
        replacement      = importpatchtable[index].replacement;
        restoreimport((HMODULE)modulebase, exportmodulename, importname, replacement);
    }

    return TRUE;
}
