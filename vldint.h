////////////////////////////////////////////////////////////////////////////////
//  $Id: vldint.h,v 1.10 2006/01/14 12:33:29 db Exp $
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

#ifndef _DEBUG
#error "Visual Leak Detector requires a *debug* C runtime library (compiler option /MDd, /MLd, /MTd, or /LDd)."
#endif // _DEBUG

// Standard headers
#include <cstdio>

// Microsoft-specific headers
#include <windows.h>    // crtdbg.h, dbghelp.h, dbgint.h, and mtdll.h depend on this.
#include <crtdbg.h>     // Provides heap debugging services.
#define __out_xcount(x) // Workaround for the specstrings.h bug in the Platform SDK
#include <dbghelp.h>    // Provides stack walking and symbol handling services.
#define _CRTBLD         // Force dbgint.h and mtdll.h to allow us to include them, even though we're not building the C runtime library.
#include <dbgint.h>     // Provides access to the heap internals, specifically the memory block header structure.
#ifdef _MT
#include <mtdll.h>      // Provides mutex locking services.
#endif // _MT
#undef _CRTBLD
#include <shlwapi.h>    // Provides path/file specification services.

// VLD-specific headers
#include "vldapi.h"     // Declares the Visual Leak Detector APIs
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
#define VLD_TLS_DISABLED      0x1 // If set, memory leak detection is disabled for the current thread.
#define VLD_TLS_ENABLED       0x2 // If set, memory leak detection is enabled for the current thread.

////////////////////////////////////////////////////////////////////////////////
//
// The VisualLeakDetector Class
//
//   One global instance of this class is instantiated. Upon construction it
//   registers our allocation hook function with the debug heap. Upon
//   destruction it checks for, and reports, memory leaks.
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
    void configure ();
    void dumpuserdatablock (const _CrtMemBlockHeader *pheader);
    bool enabled ();
    unsigned long eraseduplicates (const _CrtMemBlockHeader *pheader, size_t size, const CallStack *callstack);
#if defined(_M_IX86) || defined(_M_X64)
    DWORD_PTR getprogramcounterx86x64 ();
#endif // defined(_M_IX86) || defined(_M_X64)
    inline void getstacktrace (CallStack *callstack);
    inline void hookfree (const void *pdata);
    inline void hookmalloc (long request);
    inline void hookrealloc (const void *pdata, long request);
    void report (const char *format, ...);
    void reportconfig ();
    void reportleaks ();

    // Private Data
    unsigned long    m_optflags;       // Configuration options:
#define VLD_OPT_AGGREGATE_DUPLICATES 0x1 // If set, aggregate duplicate leaks in the leak report
#define VLD_OPT_SELF_TEST            0x2 // If set, peform a self-test to verify memory leak self-checking
#define VLD_OPT_SHOW_USELESS_FRAMES  0x4 // If set, include useless frames (e.g. internal to VLD) in call stacks
#define VLD_OPT_START_DISABLED       0x8 // If set, memory leak detection will initially disabled
    HMODULE          m_dbghelp;        // Handle to the Debug Help Library
    BlockMap        *m_mallocmap;      // Map of allocated memory blocks
    unsigned long    m_maxdatadump;    // Maximum number of user-data bytes to dump for each leaked block
    unsigned long    m_maxtraceframes; // Maximum number of frames per stack trace for each leaked block
    _CRT_ALLOC_HOOK  m_poldhook;       // Pointer to the previously installed allocation hook function
    HANDLE           m_process;        // Handle to the current process - required for obtaining stack traces
    char            *m_selftestfile;   // Filename where the memory leak self-test block is leaked
    int              m_selftestline;   // Line number where the memory leak self-test block is leaked
    unsigned long    m_status;         // Status flags:
#define VLD_STATUS_INSTALLED     0x1   //   If set, VLD was successfully installed
#define VLD_STATUS_NEVER_ENABLED 0x2   //   If set, VLD started disabled, and has not yet been manually enabled
    HANDLE           m_thread;         // Pseudo-handle meaning "current thread" - required for obtaining stack traces
    DWORD            m_tlsindex;       // Index for thread-local storage of VLD data

    // The Visual Leak Detector APIs are our friends.
    friend void VLDEnable ();
    friend void VLDDisable ();
};

// Configuration option default values
#define VLD_DEFAULT_OPT_FLAGS     0x0
#define VLD_DEFAULT_MAX_DATA_DUMP    0xffffffff
#define VLD_DEFAULT_MAX_TRACE_FRAMES 0xffffffff

