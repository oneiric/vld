////////////////////////////////////////////////////////////////////////////////
//  $Id: vldint.h,v 1.12 2006/01/16 03:53:10 db Exp $
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

#include <windows.h>
#define __out_xcount(x) // Workaround for the specstrings.h bug in the Platform SDK
#include <dbghelp.h>    // Provides stack walking and symbol handling services.
#include <shlwapi.h>    // Provides path/file specification services.
#include "callstack.h"  // Provides a custom class for handline call stacks
#include "map.h"        // Provides a custom STL-like Map template

#define MODULELISTLENGTH 1024
#define VLDVERSION       "1.9a"

// Thread local status flags
#define VLD_TLS_UNINITIALIZED 0x0 // Thread local storage for the current thread is uninitialized.
#define VLD_TLS_DISABLED      0x1 // If set, memory leak detection is disabled for the current thread.
#define VLD_TLS_ENABLED       0x2 // If set, memory leak detection is enabled for the current thread.

// The Visual Leak Detector APIs.
extern "C" __declspec(dllexport) void VLDDisable ();
extern "C" __declspec(dllexport) void VLDEnable ();

// Data is collected for every block allocated from any heap in the process.
// The data is stored in this structure and these structures are stored in
// a BlockMap which maps each structure to its corresponding memory block.
typedef struct blockinfo_s
{
    CallStack callstack;
    SIZE_T    serialnumber;
    SIZE_T    size;
} blockinfo_t;

// BlockMaps map memory blocks (via their addresses) to blockinfo_t structures.
typedef Map<LPVOID, blockinfo_t*> BlockMap;

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

private:
    // Private Helper Functions - see each function definition for details.
    char* buildsymbolsearchpath ();
    void configure ();
    bool enabled ();
    unsigned long eraseduplicates (const BlockMap::Iterator &element);
    static LPVOID __stdcall heapalloc (HANDLE heap, DWORD flags, DWORD bytes);
    static HANDLE __stdcall heapcreate (DWORD options, SIZE_T initsize, SIZE_T maxsize);
    static BOOL __stdcall heapdestroy (HANDLE heap);
    static BOOL __stdcall heapfree (HANDLE heap, DWORD flags, LPVOID mem);
    static LPVOID __stdcall heaprealloc (HANDLE heap, DWORD flags, LPVOID mem, SIZE_T bytes);
    inline void mapalloc (HANDLE heap, LPVOID mem, SIZE_T size);
    inline void mapcreate (HANDLE heap);
    inline void mapdestroy (HANDLE heap);
    inline void mapfree (HANDLE heap, LPVOID mem);
    inline void maprealloc (HANDLE heap, LPVOID mem, LPVOID newmem, SIZE_T size);
    static BOOL __stdcall patchheapapis (PTSTR modulename, DWORD64 modulebase, ULONG modulesize, PVOID context);
    void reportconfig ();
    void reportleaks ();
    static BOOL __stdcall restoreheapapis (PTSTR modulename, DWORD64 modulebase, ULONG modulesize, PVOID context);

    // Private Data
    CRITICAL_SECTION  m_heaplock;       // Serialzes access to the heap map
    HeapMap          *m_heapmap;        // Map of all active heaps in the process
    unsigned long     m_maxdatadump;    // Maximum number of user-data bytes to dump for each leaked block
    unsigned long     m_maxtraceframes; // Maximum number of frames per stack trace for each leaked block
    char              m_modulelist [MODULELISTLENGTH]; // List of modules to include or exclude
    unsigned long     m_optflags;       // Configuration options:
#define VLD_OPT_AGGREGATE_DUPLICATES 0x1  // If set, aggregate duplicate leaks in the leak report
#define VLD_OPT_SELF_TEST            0x2  // If set, peform a self-test to verify memory leak self-checking
#define VLD_OPT_SHOW_USELESS_FRAMES  0x4  // If set, include useless frames (e.g. internal to VLD) in call stacks
#define VLD_OPT_START_DISABLED       0x8  // If set, memory leak detection will initially disabled
#define VLD_OPT_MODULE_LIST_INCLUDE  0x10 // If set, modules in the module list are included, all others are excluded.
    char             *m_selftestfile;   // Filename where the memory leak self-test block is leaked
    int               m_selftestline;   // Line number where the memory leak self-test block is leaked
    unsigned long     m_status;         // Status flags:
#define VLD_STATUS_INSTALLED     0x1    //   If set, VLD was successfully installed
#define VLD_STATUS_NEVER_ENABLED 0x2    //   If set, VLD started disabled, and has not yet been manually enabled
    DWORD             m_tlsindex;       // Index for thread-local storage of VLD data

    // The Visual Leak Detector APIs are our friends.
    friend __declspec(dllexport) void VLDDisable ();
    friend __declspec(dllexport) void VLDEnable ();
};

// Configuration option default values
#define VLD_DEFAULT_OPT_FLAGS        0x0
#define VLD_DEFAULT_MAX_DATA_DUMP    0xffffffff
#define VLD_DEFAULT_MAX_TRACE_FRAMES 0xffffffff
