////////////////////////////////////////////////////////////////////////////////
//  $Id: utility.h,v 1.1 2006/01/15 07:01:23 db Exp $
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

// Microsoft-specific headers
#include <windows.h>

#ifdef _WIN64
#define ADDRESSFORMAT      "0x%.16X" // Format string for 64-bit addresses
#else
#define ADDRESSFORMAT      "0x%.8X"  // Format string for 32-bit addresses
#endif // _WIN64
#define BLOCKMAPCHUNKSIZE  64        // Size, in Pairs, of each BlockMap Chunk

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

// Relative Virtual Address to Virtual Address conversion.
#define R2VA(modulebase, rva) (((PBYTE)modulebase) + rva)

// Utility functions. See function definitions for details.
const char* booltostr (bool b);
void dumpmemory (const LPVOID address, SIZE_T length);
DWORD_PTR getprogramcounterx86x64 ();
bool iscrtblock (LPVOID mem, SIZE_T size);
void patchimport (HMODULE importmodule, const char *exportmodulename, const char *importname, void *replacement);
void report (const char *format, ...);
void restoreimport (HMODULE importmodule, const char *exportmodulename, const char *importname, void *replacement);
void strapp (char **dest, char *source);
bool strtobool (const char *s);
