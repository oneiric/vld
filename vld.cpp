////////////////////////////////////////////////////////////////////////////////
//  $Id: vld.cpp,v 1.29 2006/01/16 03:52:45 db Exp $
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

// Frame pointer omission (FPO) optimization should be turned off for this
// entire file. The release VLD libs don't include FPO debug information, so FPO
// optimization will interfere with stack walking.
#pragma optimize ("y", off)

#define _CRTBLD
#ifdef NDEBUG
#define _DEBUG
#endif // NDEBUG
#include <dbgint.h>  // Provides access to the heap internals, specifically the memory block header structure.
#ifdef NDEBUG
#undef _DEBUG
#endif // NDEBUG
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
HANDLE currentprocess; // Pseudo-handle for the current process.
HANDLE currentthread;  // Pseudo-handle for the current thread.

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
    // and callstack of the specified element.
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

// heapalloc - Calls to HeapAlloc are patched through to this function. This
//   function calls the real HeapAlloc and then invokes VLD's internal mapping
//   function to track the resulting block returned by HeapAlloc.
//
//  - heap (IN): Handle to the heap from which to allocate memory. Passed on to
//      HeapAlloc without modification.
//
//  - flags (IN): Heap allocation control flags. See HeapAlloc documentation for
//      details. Passed on to HeapAlloc without modification.
//
//  - bytes (IN): Size, in bytes, of the block to allocate. Passed on to
//      HeapAlloc without modification.
//
//  Return Value:
//
//    Returns the return value from HeapAlloc. See HeapAlloc documentation for
//    details.
//
LPVOID VisualLeakDetector::heapalloc (HANDLE heap, DWORD flags, DWORD bytes)
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

// heapcreate - Calls to HeapCreate are patched through to this function. This
//   function calls the real HeapCreate and then creates VLD's internal
//   datastructures for tracking allocations from the heap created.
//
//  - options (IN): Heap options. See HeapCreate documentation for details.
//      Passed on to HeapCreate without modification.
//
//  - initsize (IN): Initial size of the heap. Passed on to HeapCreate without
//      modification.
//
//  - maxsize (IN): Maximum size of the heap. Passed on to HeapCreate without
//      modification.
//
//  Return Value:
//
//    Returns the value returned by HeapCreate. See HeapCreate documentation
//    for details.
//
HANDLE VisualLeakDetector::heapcreate (DWORD options, SIZE_T initsize, SIZE_T maxsize)
{
    HANDLE heap = HeapCreate(options, initsize, maxsize);

    EnterCriticalSection(&visualleakdetector.m_heaplock);

    visualleakdetector.mapcreate(heap);

    LeaveCriticalSection(&visualleakdetector.m_heaplock);

    return heap;
}

// heapdestroy - Calls to HeapDestroy are patched through to this function. This
//   function clears and frees VLD's internal datastructures for the specified
//   heap and then invokes the real HeapDestroy function.
//
//  - heap (IN): Handle to the heap to be destroyed. Passed on to HeapDestroy
//      without modification.
//
//  Return Value:
//
//    Returns the valued returned by HeapDestroy. See HeapDestroy documentation
//    for details.
//
BOOL VisualLeakDetector::heapdestroy (HANDLE heap)
{
    EnterCriticalSection(&visualleakdetector.m_heaplock);

    visualleakdetector.mapdestroy(heap);

    LeaveCriticalSection(&visualleakdetector.m_heaplock);

    return HeapDestroy(heap);
}

// heapfree - Calls to HeapFree are patched through to this function. This
//   function invokes VLD's internal mapping function for tracking the block
//   that is being freed and then calls the real HeapFree.
//
//  - heap (IN): Handle to the heap to which the block being freed belongs.
//      Passed on to HeapFree without modification.
//
//  - flags (IN): Heap control flags. See HeapFree documentation for more
//      details. Passed on to HeapFree without modification.
//
//  - mem (IN): Pointer to the memory block being freed. Passed on to HeapFree
//      without modification.
//
//  Return Value:
//
//    Returns the value returned by HeapFree. See HeapFree documentation for
//    details.
//
BOOL VisualLeakDetector::heapfree (HANDLE heap, DWORD flags, LPVOID mem)
{
    EnterCriticalSection(&visualleakdetector.m_heaplock);

    visualleakdetector.mapfree(heap, mem);

    LeaveCriticalSection(&visualleakdetector.m_heaplock);

    return HeapFree(heap, flags, mem);
}

// heaprealloc - Calls to HeapReAlloc are patched through to this function. This
//   function calls the real HeapReAlloc and then invokes VLD's internal mapping
//   function for tracking the reallocation.
//
//  - heap (IN): Handle to the heap to reallocate memory from. Passed on to
//      HeapReAlloc without modification.
//
//  - flags (IN): Heap control flags. See HeapReAlloc documentation for details.
//      Passed on to HeapReAlloc without modification.
//
//  - bytes (IN): Size, in bytes, of the block to reallocate. Passed on to
//      HeapReAlloc without modification.
//
//  Return Value:
//
//    Returns the value returned by HeapReAlloc. See HeapReAlloc documentation
//    for more details.
//
LPVOID VisualLeakDetector::heaprealloc (HANDLE heap, DWORD flags, LPVOID mem, SIZE_T bytes)
{
    LPVOID newmem;
    
    EnterCriticalSection(&visualleakdetector.m_heaplock);

    newmem = HeapReAlloc(heap, flags, mem, bytes);

    visualleakdetector.maprealloc(heap, mem, newmem, bytes);

    LeaveCriticalSection(&visualleakdetector.m_heaplock);

    return newmem;
}

// mapalloc - VLD's internal mapping function for tracking memory allocations.
//   Any API that results in a memory allocation should eventually result in a
//   call to this function. This function then records the block's information
//   in VLD's block map. If the process fails to free this block before the
//   process terminates, VLD will find the block in the block map and detect
//   the leak.
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
    BlockMap          *blockmap;
    HeapMap::Iterator  heapit;
    blockinfo_t       *info;
    static SIZE_T      serialnumber = 0;

    if (!enabled()) {
        // Memory leak detection is disabled. Don't track allocations.
        return;
    }

    // Record the block's information and insert it into the heap's block map.
    info = new blockinfo_t;
    info->callstack.getstacktrace(m_maxtraceframes);
    info->serialnumber = serialnumber++;
    info->size = size;
    try {
        heapit = m_heapmap->find(heap);
        if (heapit == m_heapmap->end()) {
            // We haven't created a block map for this heap yet, do it now.
            mapcreate(heap);
            heapit = m_heapmap->find(heap);
        }
        blockmap = (*heapit).second;
        blockmap->insert(mem, info);
    }
    catch (exception_e e) {
        switch (e) {
        case duplicate_value:
            report("WARNING: Visual Leak Detector detected a double allocation.\n");
            break;

        default:
            report("ERROR: Visual Leak Detector: Unhandled exception.\n");
            assert(false);
        }
    }
}

// mapcreate - VLD's internal mapping function for tracking newly created
//   heaps. Any API that results in a new heap being created should eventually
//   result in a call to this function. This function creates the block map
//   required for tracking allocations from the new heap and inserts it into
//   the heap map.
//
//  - heap (IN): Handle to the newly created heap.
//
//  Return Value:
//
//    None.
//
void VisualLeakDetector::mapcreate (HANDLE heap)
{
    BlockMap *blockmap;

    try {
        // Create a new block map for this heap and insert it into the heap map.
        blockmap = new BlockMap;
        blockmap->reserve(BLOCKMAPRESERVE);
        m_heapmap->insert(heap, blockmap);
    }
    catch (exception_e e) {
        switch (e) {
        case duplicate_value:
            report("WARNING: Visual Leak Detector detected a duplicate heap.\n");
            break;

        default:
            report("ERROR: Visual Leak Detector: Unhandled exception.\n");
            assert(false);
        }
    }
}

// mapdestroy - VLD's internal mapping function for tracking heaps that are
//   destroyed. Any API that results in a heap being destroyed should
//   eventually result in a call to this function. This function clears and
//   frees the block map corresponding to the heap being destroyed.
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

// mapfree - VLD's internal mapping function for tracking freed memory blocks.
//   Any API that results in memory being freed should eventually result in a
//   call to this function. This function then acknowledges that the block is
//   being freed by removing the block from VLD's block map, thereby ensuring
//   that VLD will not flag this block as a leak.
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

// maprealloc - VLD's internal mapping function for tracking reallocated memory
//   blocks. Any API that results in a block being reallocated should eventually
//   result in a call to this function. This function will then determine
//   whether or not the block must be removed and a replacement block re-added
//   to VLD's block map in order to track the reallocation, in the same manner
//   that mapalloc tracks allocations.
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
//   key heap APIs for all modules loaded in the process (unless otherwise
//   excepted). One notable exception is the VLD DLL's module. It is not
//   necessary, nor desirable, to patch the heap APIs for VLD's own module.
//   This avoids potential infinte recursion when VLD itself needs to allocate
//   memory and also improves the performance of VLD's internal memory
//   allocations to minimize the overall performance impact of VLD on the host
//   process (no stack traces are taken for VLD's own allocations).
//
//  - modulename (IN): Name of a module loaded in the current process.
//
//  - modulebase (IN): Base address of the module.
//
//  - modulesize (IN): Total size of the module.
//
//  - context (IN): User-supplied context (Not currently used by VLD).
//
//  Return Value:
//
//    Always returns TRUE.
//
BOOL VisualLeakDetector::patchheapapis (PTSTR modulename, DWORD64 modulebase, ULONG modulesize, PVOID context)
{
    if (_stricmp(modulename, "vld.dll") == 0) {
        // Don't patch the VLD DLL's IAT. VLD's calls to the Windows heap APIs
        // will go directly to the real APIs.
        return TRUE;
    }
    _strlwr(modulename);
    if (visualleakdetector.m_optflags & VLD_OPT_MODULE_LIST_INCLUDE) {
        // Only patch this module if it is in the module list.
        if (strstr(visualleakdetector.m_modulelist, modulename) == NULL) {
            return TRUE;
        }
    }
    else {
        // Do not patch this module if it is in the module list.
        if (strstr(visualleakdetector.m_modulelist, modulename) != NULL) {
            return TRUE;
        }
    }

    // Patch key Windows heap APIs to functions provided by VLD.
    patchimport((HMODULE)modulebase, "kernel32.dll", "HeapAlloc",  heapalloc);
    patchimport((HMODULE)modulebase, "kernel32.dll", "HeapCreate", heapcreate);
    patchimport((HMODULE)modulebase, "kernel32.dll", "HeapDestroy", heapdestroy);
    patchimport((HMODULE)modulebase, "kernel32.dll", "HeapFree",   heapfree);
    patchimport((HMODULE)modulebase, "kernel32.dll", "HeapReAlloc", heaprealloc);

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
    HeapMap::Iterator   heapit;
    blockinfo_t        *info;
    unsigned long       leaksfound = 0;
    LPVOID              mem;
    SIZE_T              size;
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
        blockmap = (*heapit).second;
        for (blockit = blockmap->begin(); blockit != blockmap->end(); ++blockit) {
            // Found a block which is still in the BlockMap. We've identified a
            // potential memory leak.
            mem = (*blockit).first;
            info = (*blockit).second;
            size = info->size;
            if (iscrtblock(mem, size)) {
                crtheader = (_CrtMemBlockHeader*)mem;
                if (_BLOCK_TYPE(crtheader->nBlockUse) == _CRT_BLOCK) {
                    // This is a block allocated to the CRT heap and is marked
                    // as a block used internally by the CRT. This is not a leak
                    // because the CRT will free it after VLD is destroyed.
                    continue;
                }
                size = crtheader->nDataSize;
            }
            // It looks like a real memory leak.
            if (leaksfound == 0) {
                report("WARNING: Visual Leak Detector detected memory leaks!\n");
            }
            leaksfound++;
            report("---------- Block %ld at "ADDRESSFORMAT": %u bytes ----------\n", info->serialnumber, mem, size);
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
                dumpmemory(mem, (m_maxdatadump < info->size) ? m_maxdatadump : size);
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
//  - context (IN): User-supplied context (Not currently used by VLD).
//
//  Return Value:
//
//    Always returns TRUE.
//
BOOL VisualLeakDetector::restoreheapapis (PTSTR modulename, DWORD64 modulebase, ULONG modulesize, PVOID context)
{
    if (_stricmp(modulename, "vld.dll") == 0) {
        // The IAT for the VLD DLL did not get patched, so it does not need to
        // be restored.
        return TRUE;
    }

    // Restore the previously patched Windows heap APIs.
    restoreimport((HMODULE)modulebase, "kernel32.dll", "HeapAlloc", heapalloc);
    restoreimport((HMODULE)modulebase, "kernel32.dll", "HeapCreate", heapcreate);
    restoreimport((HMODULE)modulebase, "kernel32.dll", "HeapDestroy", heapdestroy);
    restoreimport((HMODULE)modulebase, "kernel32.dll", "HeapFree", heapfree);
    restoreimport((HMODULE)modulebase, "kernel32.dll", "HeapReAlloc", heaprealloc);

    return TRUE;
}
