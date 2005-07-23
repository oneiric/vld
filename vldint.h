////////////////////////////////////////////////////////////////////////////////
//  $Id: vldint.h,v 1.1 2005/07/23 00:22:07 dmouldin Exp $
//
//  Visual Leak Detector (Version 0.9i)
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

#ifndef _DEBUG
#error "Visual Leak Detector requires a *debug* C runtime library (compiler option /MDd, /MLd, /MTd, or /LDd)."
#endif // _DEBUG

// Standard headers
#include <cstdio>

// Microsoft-specific headers
#include <windows.h>    // crtdbg.h, dbghelp.h, dbgint.h, and mtdll.h depend on this.
#include <crtdbg.h>     // Provides heap debugging services.
#define __out_xcount(x) // Workaround for the specstrings bug in the dbghelp.h from Debugging Tools for Windows.
#include <dbghelp.h>    // Provides stack walking and symbol handling services.
#define _CRTBLD         // Force dbgint.h and mtdll.h to allow us to include them, even though we're not building the C runtime library.
#include <dbgint.h>     // Provides access to the heap internals, specifically the memory block header structure.
#ifdef _MT
#include <mtdll.h>      // Provides mutex locking services.
#endif // _MT
#undef _CRTBLD
#include <shlwapi.h>    // Provides path/file specification services.
#pragma comment(lib, "shlwapi.lib")

// VLD-specific headers
#include "vldutil.h"    // Provides utility functions and classes

// VLD version and library type definitions
#define VLD_VERSION "1.0"
#ifdef _DLL
#define VLD_LIBTYPE "multithreaded DLL"
#else
#ifdef _MT
#define VLD_LIBTYPE "multithreaded static"
#else
#define VLD_LIBTYPE "single-threaded static"
#endif // _MT
#endif // _DLL

// Architecture-specific definitions for x86 and x64
#if defined(_M_IX86)
#define SIZEOFPTR 4
#define X86X64ARCHITECTURE IMAGE_FILE_MACHINE_I386
#define AXREG eax
#define BPREG ebp
#elif defined(_M_X64)
#define SIZEOFPTR 8
#define X86X64ARCHITECTURE IMAGE_FILE_MACHINE_AMD64
#define AXREG rax
#define BPREG rbp
#endif // _M_IX86

// Thread local status flags
#define VLD_TLS_UNINITIALIZED 0x0 // Thread local storage for the current thread is uninitialized.
#define VLD_TLS_DISABLED      0x1 // If set, Visual Leak Detector is disabled for the current thread.
#define VLD_TLS_ENABLED       0x2 // If set, Visual Leak Detector is enabled for the current thread.

// Typedefs for explicit dynamic linking with functions exported from dbghelp.dll.
typedef BOOL (__stdcall *StackWalk64_t)(DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID, PREAD_PROCESS_MEMORY_ROUTINE64,
                                        PFUNCTION_TABLE_ACCESS_ROUTINE64, PGET_MODULE_BASE_ROUTINE64, PTRANSLATE_ADDRESS_ROUTINE64);
typedef PVOID (__stdcall *SymFunctionTableAccess64_t)(HANDLE, DWORD64);
typedef DWORD64 (__stdcall *SymGetModuleBase64_t)(HANDLE, DWORD64);
typedef BOOL (__stdcall *SymCleanup_t)(HANDLE);
typedef BOOL (__stdcall *SymFromAddr_t)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
typedef BOOL (__stdcall *SymGetLineFromAddr64_t)(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINE64);
typedef BOOL (__stdcall *SymInitialize_t)(HANDLE, PCTSTR, BOOL);
typedef DWORD (__stdcall *SymSetOptions_t)(DWORD);

////////////////////////////////////////////////////////////////////////////////
//
// The VisualLeakDetector Class
//
//   One global instance of this class is instantiated. Upon construction it
//   dynamically links with the Debug Help Library and registers our allocation
//   hook function with the debug heap. Upon destruction it checks for, and
//   reports, memory leaks.
//
//   It is constructed within the context of the process' main thread during C
//   runtime initialization and is destroyed in that same context after the
//   process has returned from its main function.
//
class VisualLeakDetector
{
public:
    VisualLeakDetector();
    ~VisualLeakDetector();

private:
    // Private Helper Functions - see each function definition for details.
    static int allochook (int type, void *pdata, size_t size, int use, long request, const unsigned char *file, int line);
    char* buildsymbolsearchpath ();
    void dumpuserdatablock (_CrtMemBlockHeader *pheader);
    inline bool enabled ();
#if defined(_M_IX86) || defined(_M_X64)
    DWORD_PTR getprogramcounterx86x64 ();
#endif // defined(_M_IX86) || defined(_M_X64)
    inline void getstacktrace (CallStack *callstack);
    inline void hookfree (void *pdata);
    inline void hookmalloc (long request);
    inline void hookrealloc (void *pdata, long request);
    bool linkdebughelplibrary ();
    void report (const char *format, ...);
    void reportleaks ();

    // Private Data
    BlockMap        *m_mallocmap;    // Map of allocated memory blocks
    _CRT_ALLOC_HOOK  m_poldhook;     // Pointer to the previously installed allocation hook function
    HANDLE           m_process;      // Handle to the current process - required for obtaining stack traces
    unsigned long    m_status;       // Status flags:
#define VLD_STATUS_INSTALLED     0x1 //   If set, VLD was successfully installed
#define VLD_STATUS_NEVER_ENABLED 0x2 //   If set, VLD started disabled, and has not yet been manually enabled
    HANDLE           m_thread;       // Pseudo-handle meaning "current thread" - required for obtaining stack traces
    DWORD            m_tlsindex;     // Index for thread-local storage of VLD data.

    // The Visual Leak Detector APIs are our friends.
    friend void VLDEnable ();
    friend void VLDDisable ();
};