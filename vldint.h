////////////////////////////////////////////////////////////////////////////////
//  $Id: vldint.h,v 1.17 2006/01/27 23:08:19 dmouldin Exp $
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
#include "callstack.h" // Provides a custom class for handling call stacks.
#include "map.h"       // Provides a custom STL-like map template.
#include "utility.h"   // Provides miscellaneous utility functions.

#define MAXMODULELISTLENGTH 512     // Maximum module list length, in characters.
#define VLDVERSION          L"1.9a"

// Thread local storage status values
#define VLD_TLS_UNINITIALIZED  0x0 // Thread local storage for the current thread is uninitialized.
#define VLD_TLS_DISABLED       0x1 // Memory leak detection is disabled for the current thread.
#define VLD_TLS_ENABLED        0x2 // Memory leak detection is enabled for the current thread.
#define VLD_TLS_INITDLLCRTHEAP 0x4 // Indicates that the shared DLL CRT heap's handle needs to be initialized.

// The Visual Leak Detector APIs.
extern "C" __declspec(dllexport) VOID VLDDisable ();
extern "C" __declspec(dllexport) VOID VLDEnable ();

// Data is collected for every block allocated from any heap in the process.
// The data is stored in this structure and these structures are stored in
// a BlockMap which maps each of these structures to its corresponding memory
// block.
typedef struct blockinfo_s {
    CallStack callstack;
    SIZE_T    serialnumber;
    SIZE_T    size;
} blockinfo_t;

// BlockMaps map memory blocks (via their addresses) to blockinfo_t structures.
typedef Map<LPCVOID, blockinfo_t*> BlockMap;

// HeapMaps map heaps (via their handles) to BlockMaps.
typedef Map<HANDLE, BlockMap*> HeapMap;

// This structure stores information, primarily the virtual address range, about
// a given module and can be used with the Set template because it supports the
// '<' operator (sorts by virtual address range).
typedef struct moduleinfo_s {
    BOOL operator < (const struct moduleinfo_s &other) const
    {
        if (addrhigh < other.addrlow) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }

    SIZE_T addrhigh;
    SIZE_T addrlow;
    UINT32 flags;
#define VLD_MODULE_EXCLUDED    0x1 // This module is attached, but is excluded from leak detection.
#define VLD_MODULE_NOTATTACHED 0x2 // VLD did not attach to this module
#define VLD_MODULE_VLDDLL      0x4 // This is vld.dll's own module.
} moduleinfo_t;

// ModuleSets store information about modules loaded in the process.
typedef Set<moduleinfo_t> ModuleSet;

// Return code type used by LdrLoadDll.
typedef ULONG NTSTATUS;

// Thread-local storage.
typedef struct tls_s {
    SIZE_T level;
    UINT32 status;
} tls_t;

// Unicode string structure used by LdrLoadDll.
typedef struct unicodestring_s {
    USHORT length;
    USHORT maxlength;
    PWSTR  buffer;
} unicodestring_t;

////////////////////////////////////////////////////////////////////////////////
//
// The VisualLeakDetector Class
//
//   One global instance of this class is instantiated. Upon construction it
//   patches the import address table (IAT) of (almost) every module loaded in
//   the process (see the "patchimport" utility function) to allow key Windows
//   heap APIs to be patched through to, or redirected to, certain functions
//   provided by VLD. Patching the IATs in this manner allows VLD to be made
//   aware of all relevant heap activity, making it possible for VLD to detect
//   and trace memory leaks.
//
//   The one global instance of this class is constructed within the context of
//   the process' main thread during process initialization and is destroyed in
//   the same context during process termination.
//
//   When the VisualLeakDetector object is destroyed, it consults its internal
//   datastructures, looking for any memory that has not been freed. A memory
//   leak report is then generated, indicating any memory leaks that may have
//   been identified.
//
//   This class is derived from IMalloc so that it can provide an implementation
//   of the IMalloc COM interface in order to support detection of COM-based
//   memory leaks. However, this implementation of IMalloc is actually just a
//   thin wrapper around the system's implementation of IMalloc.
//
class VisualLeakDetector : public IMalloc
{
public:
    VisualLeakDetector();
    ~VisualLeakDetector();

    // Public functions
    BOOL isvldaddress (SIZE_T address);

    // Public IMalloc methods - for support of COM-based memory leak detection.
    ULONG __stdcall AddRef ();
    LPVOID __stdcall Alloc (ULONG size);
    INT __stdcall DidAlloc (LPVOID mem);
    VOID __stdcall Free (LPVOID mem);
    ULONG __stdcall GetSize (LPVOID mem);
    VOID __stdcall HeapMinimize ();
    HRESULT __stdcall QueryInterface (REFIID iid, LPVOID *object);
    LPVOID __stdcall Realloc (LPVOID mem, ULONG size);
    ULONG __stdcall Release ();

private:
    // Import patching replacement functions - see each function definition for details.
    static void __cdecl __free_dbg (void *mem, int type);
    static void* __cdecl __malloc_dbg (size_t size, int type, const char *file, int line);
    static void* __cdecl __realloc_dbg (void *mem, size_t size, int type, const char *file, int line);
    static void __cdecl _crt_delete (void *mem);
    static void* __cdecl _crt_new (unsigned int size);
    static void* __cdecl _crt_new_dbg (unsigned int size, int type, const char *file, int line);
    static void __cdecl _free (void *mem);
    static HRESULT __stdcall _CoGetMalloc (DWORD context, LPMALLOC *imalloc);
    static LPVOID __stdcall _CoTaskMemAlloc (ULONG size);
    static LPVOID __stdcall _CoTaskMemRealloc (LPVOID mem, ULONG size);
    static VOID __stdcall _CoTaskMemFree (LPVOID mem);
    static LPVOID __stdcall _HeapAlloc (HANDLE heap, DWORD flags, SIZE_T size);
    static HANDLE __stdcall _HeapCreate (DWORD options, SIZE_T initsize, SIZE_T maxsize);
    static BOOL __stdcall _HeapDestroy (HANDLE heap);
    static BOOL __stdcall _HeapFree (HANDLE heap, DWORD flags, LPVOID mem);
    static LPVOID __stdcall _HeapReAlloc (HANDLE heap, DWORD flags, LPVOID mem, SIZE_T size);
    static NTSTATUS __stdcall _LdrLoadDll (LPWSTR searchpath, PDWORD flags, unicodestring_t *modulename, PHANDLE modulehandle);
    static void* __cdecl _malloc (size_t size);
    static void __cdecl _mfc42_delete (void *mem);
    static void* __cdecl _mfc42_new (unsigned int size);
    static void* __cdecl _mfc42_new_dbg (unsigned int size, const char *file, int line);
    static void* __cdecl _realloc (void *mem, size_t size);

    // Private functions - see each function definition for details.
    static BOOL __stdcall attachtomodule (PCWSTR modulepath, DWORD64 modulebase, ULONG modulesize, PVOID context);
    LPWSTR buildsymbolsearchpath ();
    VOID configure ();
    static BOOL __stdcall detachfrommodule (PCWSTR modulepath, DWORD64 modulebase, ULONG modulesize, PVOID context);
    BOOL enabled ();
    inline VOID enter ();
    SIZE_T eraseduplicates (const BlockMap::Iterator &element);
    inline VOID leave ();
    VOID mapblock (SIZE_T address, HANDLE heap, LPCVOID mem, SIZE_T size);
    VOID mapheap (HANDLE heap);
    VOID remapblock (SIZE_T address, HANDLE heap, LPCVOID mem, LPCVOID newmem, SIZE_T size);
    VOID reportconfig ();
    VOID reportleaks ();
    VOID unmapblock (HANDLE heap, LPCVOID mem);
    VOID unmapheap (HANDLE heap);

    // Private data.
    HeapMap             *m_heapmap;         // Map of all active heaps in the process.
    IMalloc             *m_imalloc;         // Pointer to the *real* system implementation of IMalloc.
    CRITICAL_SECTION     m_maplock;         // Serialzes access to the heap map.
    SIZE_T               m_maxdatadump;     // Maximum number of user-data bytes to dump for each leaked block.
    UINT32               m_maxtraceframes;  // Maximum number of frames per stack trace for each leaked block.
    WCHAR                m_modulelist [MAXMODULELISTLENGTH]; // List of modules to be included or excluded from leak detection.
    ModuleSet           *m_moduleset;       // Contains information about all modules loaded in the process.
    UINT32               m_modulesincluded; // Number of modules included in leak detection.
    UINT32               m_optflags;        // Configuration options:
#define VLD_OPT_AGGREGATE_DUPLICATES 0x1    //   If set, aggregate duplicate leaks in the leak report.
#define VLD_OPT_MODULE_LIST_INCLUDE  0x2    //   If set, modules in the module list are included, all others are excluded.
#define VLD_OPT_REPORT_TO_DEBUGGER   0x4    //   If set, the memory leak report is sent to the debugger.
#define VLD_OPT_REPORT_TO_FILE       0x8    //   If set, the memory leak report is sent to a file.
#define VLD_OPT_SELF_TEST            0x10   //   If set, peform a self-test to verify memory leak self-checking.
#define VLD_OPT_SHOW_USELESS_FRAMES  0x20   //   If set, include useless frames (e.g. internal to VLD) in call stacks.
#define VLD_OPT_START_DISABLED       0x40   //   If set, memory leak detection will initially disabled.
#define VLD_OPT_UNICODE_REPORT       0x80   //   If set, the leak report will be UTF-16 encoded instead of ASCII encoded.
    static patchentry_t  m_patchtable [];   // Table of imports patched for attaching VLD to other modules.
    FILE                *m_reportfile;      // File where the memory leak report may be sent to.
    WCHAR                m_reportfilepath [MAX_PATH]; // Full path and name of file to send memory leak report to.
    const char          *m_selftestfile;    // Filename where the memory leak self-test block is leaked.
    int                  m_selftestline;    // Line number where the memory leak self-test block is leaked.
    UINT32               m_status;          // Status flags:
#define VLD_STATUS_INSTALLED            0x1 //   If set, VLD was successfully installed.
#define VLD_STATUS_NEVER_ENABLED        0x2 //   If set, VLD started disabled, and has not yet been manually enabled.
#define VLD_STATUS_FORCE_REPORT_TO_FILE 0x4 //   If set, the leak report is being forced to a file.
    //DWORD                m_tlsindex;        // Index for thread-local storage of VLD data.

    // The Visual Leak Detector APIs are our friends.
    friend __declspec(dllexport) VOID VLDDisable ();
    friend __declspec(dllexport) VOID VLDEnable ();
};

// Configuration option default values
#define VLD_DEFAULT_OPT_FLAGS        VLD_OPT_REPORT_TO_DEBUGGER
#define VLD_DEFAULT_MAX_DATA_DUMP    0xffffffff
#define VLD_DEFAULT_MAX_TRACE_FRAMES 0xffffffff
#define VLD_DEFAULT_REPORT_FILE_NAME L".\\memory_leak_report.txt"
