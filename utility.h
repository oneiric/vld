////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - Various Utility Definitions
//  Copyright (c) 2005-2006 Dan Moulding
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
//  See COPYING.txt for the full terms of the GNU Lesser General Public License.
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef VLDBUILD
#error \
"This header should only be included by Visual Leak Detector when building it from source. \
Applications should never include this header."
#endif

#include <cstdio>
#include <windows.h>
#include <intrin.h>

#ifdef _WIN64
#define ADDRESSFORMAT   L"0x%.16X" // Format string for 64-bit addresses
#else
#define ADDRESSFORMAT   L"0x%.8X"  // Format string for 32-bit addresses
#endif // _WIN64
#define BOM             0xFEFF     // Unicode byte-order mark.
#define MAXREPORTLENGTH 511        // Maximum length, in characters, of "report" messages.

// Architecture-specific definitions for x86 and x64
#if defined(_M_IX86)
#define SIZEOFPTR 4
#define X86X64ARCHITECTURE IMAGE_FILE_MACHINE_I386
#define AXREG Eax
#define BPREG Ebp
#define IPREG Eip
#define SPREG Esp
#elif defined(_M_X64)
#define SIZEOFPTR 8
#define X86X64ARCHITECTURE IMAGE_FILE_MACHINE_AMD64
#define AXREG Rax
#define BPREG Rbp
#define IPREG Rip
#define SPREG Rsp
#endif // _M_IX86

typedef struct context_s
{
    UINT_PTR* fp;
#if defined(_M_X64)
    DWORD64 Rsp;
    DWORD64 Rip;
#endif // _M_IX86
} context_t;

#if defined(_M_IX86)
// Copies the current frame pointer to the supplied variable.
#define CAPTURE_CONTEXT(context)     context.fp = ((UINT_PTR*)_AddressOfReturnAddress()) - 1
#define GET_RETURN_ADDRESS(context)  *(context.fp + 1)
#elif defined(_M_X64)
// Capture current context
#define CAPTURE_CONTEXT(context)     CONTEXT _ctx;                          \
    RtlCaptureContext(&_ctx);                                               \
    context.Rsp = _ctx.Rsp; context.Rip = _ctx.Rip; \
    context.fp = ((UINT_PTR*)_AddressOfReturnAddress()) - 1
#define GET_RETURN_ADDRESS(context)  *(context.fp + 1)
#else
// If you want to retarget Visual Leak Detector to another processor
// architecture then you'll need to provide an architecture-specific macro to
// obtain the frame pointer (or other address) which can be used to obtain the
// return address and stack pointer of the calling frame.
#error "Visual Leak Detector is not supported on this architecture."
#endif // _M_IX86 || _M_X64

// Miscellaneous definitions
#define R2VA(modulebase, rva)  (((PBYTE)modulebase) + rva) // Relative Virtual Address to Virtual Address conversion.
#define BYTEFORMATBUFFERLENGTH 4
#define HEXDUMPLINELENGTH      58

// Reports can be encoded as either ASCII or Unicode (UTF-16).
enum encoding_e {
    ascii,
    unicode
};

// This structure allows us to build a table of APIs which should be patched
// through to replacement functions provided by VLD.
typedef struct patchentry_s
{
    LPCSTR  importname;       // The name (or ordinal) of the imported API being patched.
    LPCVOID replacement;      // Pointer to the function to which the imported API should be patched through to.
} patchentry_t;

typedef struct moduleentry_s
{
	LPCSTR          exportmodulename; // The name of the module exporting the patched API.
	UINT_PTR        modulebase;       // The base address of the exporting module (filled in at runtime when the modules are loaded).
	patchentry_t*   patchtable;
} moduleentry_t;

// Utility functions. See function definitions for details.
VOID dumpmemorya (LPCVOID address, SIZE_T length);
VOID dumpmemoryw (LPCVOID address, SIZE_T length);
BOOL findimport (HMODULE importmodule, HMODULE exportmodule, LPCSTR exportmodulename, LPCSTR importname);
BOOL findpatch (HMODULE importmodule, LPCSTR exportmodulename, LPCVOID replacement);
VOID insertreportdelay ();
BOOL moduleispatched (HMODULE importmodule, moduleentry_t patchtable [], UINT tablesize);
BOOL patchimport (HMODULE importmodule, moduleentry_t *module);
BOOL patchmodule (HMODULE importmodule, moduleentry_t patchtable [], UINT tablesize);
VOID report (LPCWSTR format, ...);
VOID restoreimport (HMODULE importmodule, moduleentry_t* module);
VOID restoremodule (HMODULE importmodule, moduleentry_t patchtable [], UINT tablesize);
VOID setreportencoding (encoding_e encoding);
VOID setreportfile (FILE *file, BOOL copydebugger, BOOL copytostdout);
VOID strapp (LPWSTR *dest, LPCWSTR source);
BOOL strtobool (LPCWSTR s);
#if _WIN32_WINNT < 0x0600 // Windows XP or earlier, no GetProcessIdOfThread()
DWORD _GetProcessIdOfThread (HANDLE thread);
#define GetProcessIdOfThread _GetProcessIdOfThread
#endif