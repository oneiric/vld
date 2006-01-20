////////////////////////////////////////////////////////////////////////////////
//  $Id: vldint.h,v 1.15 2006/01/20 01:29:08 dmouldin Exp $
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

#pragma once

#ifndef VLDBUILD
#error "This header should only be included by Visual Leak Detector when building it from source. Applications should never include this header."
#endif

#include <cstdio>
#include <windows.h>
#include "callstack.h"  // Provides a custom class for handling call stacks
#include "map.h"        // Provides a custom STL-like map template

#define VLDVERSION          _T("1.9a")
#define MAXMODULELISTLENGTH 512 // Maximum module list length, in characters.

// Thread local storage status values
#define VLD_TLS_UNINITIALIZED 0x0 // Thread local storage for the current thread is uninitialized.
#define VLD_TLS_DISABLED      0x1 // Memory leak detection is disabled for the current thread.
#define VLD_TLS_ENABLED       0x2 // Memory leak detection is enabled for the current thread.

// The Visual Leak Detector APIs.
extern "C" __declspec(dllexport) VOID VLDDisable ();
extern "C" __declspec(dllexport) VOID VLDEnable ();

// Data is collected for every block allocated from any heap in the process.
// The data is stored in this structure and these structures are stored in
// a BlockMap which maps each structure to its corresponding memory block.
typedef struct blockinfo_s
{
    CallStack callstack;
    SIZE_T    serialnumber;
    SIZE_T    size;
} blockinfo_t;

// This structure allows us to build a table of APIs which should be patched
// through to replacement functions provided by VLD.
typedef struct importpatchentry_s
{
    LPCSTR  exportmodulename; // The name of the module exporting the patched API.
    LPCSTR  importname;       // The name of the imported API being patched.
    LPCVOID replacement;      // Pointer to the function to which the imported API is patched through to.
} importpatchentry_t;

// BlockMaps map memory blocks (via their addresses) to blockinfo_t structures.
typedef Map<LPCVOID, blockinfo_t*> BlockMap;

// HeapMaps map heaps (via their handles) to BlockMaps.
typedef Map<HANDLE, BlockMap*> HeapMap;

////////////////////////////////////////////////////////////////////////////////
//
// The VisualLeakDetector Class
//
//   One global instance of this class is instantiated. Upon construction it
//   patches the import address table (IAT) of every module loaded in the
//   process (see the "patchimport" utility function) to allow key Windows heap
//   APIs to be patched through to, or redirected to, certain functions provided
//   by VLD. Patching the IATs in this manner allows VLD to be made aware of all
//   relevant heap activity, making it possible for VLD to detect and trace
//   memory leaks.
//
//   It is constructed within the context of the process' main thread during
//   process initialization and is destroyed in the same context during process
//   termination.
//
//   When the VisualLeakDetector object is destroyed, it consults its internal
//   datastructures, looking for any memory that has not been freed. A memory
//   leak report is then generated, indicating any memory leaks that may have
//   been identified.
//
class VisualLeakDetector
{
public:
    VisualLeakDetector();
    ~VisualLeakDetector();

    // Public APIs. These replace corresponding CRT and Windows heap APIs.
    static void __free_dbg (void *mem, int type);
    static void* __malloc_dbg (size_t size, int type, const char *file, int line);
    static void _free (void *mem);
    static LPVOID __stdcall _HeapAlloc (HANDLE heap, DWORD flags, SIZE_T bytes);
    static HANDLE __stdcall _HeapCreate (DWORD options, SIZE_T initsize, SIZE_T maxsize);
    static BOOL __stdcall _HeapDestroy (HANDLE heap);
    static BOOL __stdcall _HeapFree (HANDLE heap, DWORD flags, LPVOID mem);
    static LPVOID __stdcall _HeapReAlloc (HANDLE heap, DWORD flags, LPVOID mem, SIZE_T bytes);
    /*static HINSTANCE __stdcall _LoadLibraryExW (LPCWSTR modulename, HANDLE file, DWORD flags);*/
    static void* _malloc (size_t size);

private:
    // Private Helper Functions - see each function definition for details.
    LPTSTR buildsymbolsearchpath ();
    VOID configure ();
    BOOL enabled ();
    SIZE_T eraseduplicates (const BlockMap::Iterator &element);
    inline VOID mapblock (HANDLE heap, LPCVOID mem, SIZE_T size);
    inline VOID mapheap (HANDLE heap);
    static BOOL __stdcall patchapis (PCTSTR modulepath, DWORD64 modulebase, ULONG modulesize, PVOID context);
    inline VOID remapblock (HANDLE heap, LPCVOID mem, LPCVOID newmem, SIZE_T size);
    VOID reportconfig ();
    VOID reportleaks ();
    static BOOL __stdcall restoreapis (PCTSTR modulepath, DWORD64 modulebase, ULONG modulesize, PVOID context);
    inline VOID unmapblock (HANDLE heap, LPCVOID mem);
    inline VOID unmapheap (HANDLE heap);

    // Private Data
    CRITICAL_SECTION  m_heaplock;       // Serialzes access to the heap map.
    HeapMap          *m_heapmap;        // Map of all active heaps in the process.
    SIZE_T            m_maxdatadump;    // Maximum number of user-data bytes to dump for each leaked block.
    UINT32            m_maxtraceframes; // Maximum number of frames per stack trace for each leaked block.
    TCHAR             m_modulelist [MAXMODULELISTLENGTH]; // List of modules to be included or excluded from leak detection.
    UINT32            m_modulespatched; // Number of modules patched.
    UINT32            m_optflags;       // Configuration options:
#define VLD_OPT_AGGREGATE_DUPLICATES 0x1  // If set, aggregate duplicate leaks in the leak report.
#define VLD_OPT_MODULE_LIST_INCLUDE  0x2  // If set, modules in the module list are included, all others are excluded.
#define VLD_OPT_REPORT_TO_DEBUGGER   0x4  // If set, the memory leak report is sent to the debugger.
#define VLD_OPT_REPORT_TO_FILE       0x8  // If set, the memory leak report is sent to a file.
#define VLD_OPT_SELF_TEST            0x10 // If set, peform a self-test to verify memory leak self-checking.
#define VLD_OPT_SHOW_USELESS_FRAMES  0x20 // If set, include useless frames (e.g. internal to VLD) in call stacks.
#define VLD_OPT_START_DISABLED       0x40 // If set, memory leak detection will initially disabled.
    FILE             *m_reportfile;     // File where the memory leak report may be sent to.
    TCHAR             m_reportfilepath [MAX_PATH]; // Full path and name of file to send memory leak report to.
    const char       *m_selftestfile;   // Filename where the memory leak self-test block is leaked.
    int               m_selftestline;   // Line number where the memory leak self-test block is leaked.
    UINT32            m_status;         // Status flags:
#define VLD_STATUS_INSTALLED     0x1    //   If set, VLD was successfully installed.
#define VLD_STATUS_NEVER_ENABLED 0x2    //   If set, VLD started disabled, and has not yet been manually enabled.
    DWORD             m_tlsindex;       // Index for thread-local storage of VLD data.

    // The Visual Leak Detector APIs are our friends.
    friend __declspec(dllexport) VOID VLDDisable ();
    friend __declspec(dllexport) VOID VLDEnable ();
};

// Configuration option default values
#define VLD_DEFAULT_OPT_FLAGS        VLD_OPT_REPORT_TO_DEBUGGER
#define VLD_DEFAULT_MAX_DATA_DUMP    0xffffffff
#define VLD_DEFAULT_MAX_TRACE_FRAMES 0xffffffff
