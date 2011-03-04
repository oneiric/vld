////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - Various Utility Functions
//  Copyright (c) 2005-2011 Dan Moulding, Arkadiy Shapkin, Laurent Lessieux
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

#include "stdafx.h"
#define VLDBUILD        // Declares that we are building Visual Leak Detector.
#include "utility.h"    // Provides various utility functions and macros.
#include "vldheap.h"    // Provides internal new and delete operators.
#include "vldint.h"

// Imported Global Variables
extern CRITICAL_SECTION imagelock;

// Global variables.
static BOOL        reportdelay = FALSE;     // If TRUE, we sleep for a bit after calling OutputDebugString to give the debugger time to catch up.
static FILE       *reportfile = NULL;       // Pointer to the file, if any, to send the memory leak report to.
static BOOL        reporttodebugger = TRUE; // If TRUE, a copy of the memory leak report will be sent to the debugger for display.
static BOOL        reporttostdout = TRUE;   // If TRUE, a copy of the memory leak report will be sent to standart output.
static encoding_e  reportencoding = ascii;  // Output encoding of the memory leak report.

// dumpmemorya - Dumps a nicely formatted rendition of a region of memory.
//   Includes both the hex value of each byte and its ASCII equivalent (if
//   printable).
//
//  - address (IN): Pointer to the beginning of the memory region to dump.
//
//  - size (IN): The size, in bytes, of the region to dump.
//
//  Return Value:
//
//    None.
//
VOID dumpmemorya (LPCVOID address, SIZE_T size)
{
    WCHAR  ascdump [18] = {0};
    SIZE_T ascindex;
    BYTE   byte;
    SIZE_T byteindex;
    SIZE_T bytesdone;
    SIZE_T dumplen;
    WCHAR  formatbuf [BYTEFORMATBUFFERLENGTH];
    WCHAR  hexdump [HEXDUMPLINELENGTH] = {0};
    SIZE_T hexindex;

    // Each line of output is 16 bytes.
    if ((size % 16) == 0) {
        // No padding needed.
        dumplen = size;
    }
    else {
        // We'll need to pad the last line out to 16 bytes.
        dumplen = size + (16 - (size % 16));
    }

    // For each byte of data, get both the ASCII equivalent (if it is a
    // printable character) and the hex representation.
    bytesdone = 0;
    for (byteindex = 0; byteindex < dumplen; byteindex++) {
        hexindex = 3 * ((byteindex % 16) + ((byteindex % 16) / 4)); // 3 characters per byte, plus a 3-character space after every 4 bytes.
        ascindex = (byteindex % 16) + (byteindex % 16) / 8;         // 1 character per byte, plus a 1-character space after every 8 bytes.
        if (byteindex < size) {
            byte = ((PBYTE)address)[byteindex];
            _snwprintf_s(formatbuf, BYTEFORMATBUFFERLENGTH, _TRUNCATE, L"%.2X ", byte);
            formatbuf[3] = '\0';
            wcsncpy_s(hexdump + hexindex, HEXDUMPLINELENGTH - hexindex, formatbuf, 4);
            if (isgraph(byte)) {
                ascdump[ascindex] = (WCHAR)byte;
            }
            else {
                ascdump[ascindex] = L'.';
            }
        }
        else {
            // Add padding to fill out the last line to 16 bytes.
            wcsncpy_s(hexdump + hexindex, HEXDUMPLINELENGTH - hexindex, L"   ", 4);
            ascdump[ascindex] = L'.';
        }
        bytesdone++;
        if ((bytesdone % 16) == 0) {
            // Print one line of data for every 16 bytes. Include the
            // ASCII dump and the hex dump side-by-side.
            report(L"    %s    %s\n", hexdump, ascdump);
        }
        else {
            if ((bytesdone % 8) == 0) {
                // Add a spacer in the ASCII dump after every 8 bytes.
                ascdump[ascindex + 1] = L' ';
            }
            if ((bytesdone % 4) == 0) {
                // Add a spacer in the hex dump after every 4 bytes.
                wcsncpy_s(hexdump + hexindex + 3, HEXDUMPLINELENGTH - hexindex - 3, L"   ", 4);
            }
        }
    }
}

// dumpmemoryw - Dumps a nicely formatted rendition of a region of memory.
//   Includes both the hex value of each byte and its Unicode equivalent.
//
//  - address (IN): Pointer to the beginning of the memory region to dump.
//
//  - size (IN): The size, in bytes, of the region to dump.
//
//  Return Value:
//
//    None.
//
VOID dumpmemoryw (LPCVOID address, SIZE_T size)
{
    BYTE   byte;
    SIZE_T byteindex;
    SIZE_T bytesdone;
    SIZE_T dumplen;
    WCHAR  formatbuf [BYTEFORMATBUFFERLENGTH];
    WCHAR  hexdump [HEXDUMPLINELENGTH] = {0};
    SIZE_T hexindex;
    WORD   word;
    WCHAR  unidump [18] = {0};
    SIZE_T uniindex;

    // Each line of output is 16 bytes.
    if ((size % 16) == 0) {
        // No padding needed.
        dumplen = size;
    }
    else {
        // We'll need to pad the last line out to 16 bytes.
        dumplen = size + (16 - (size % 16));
    }

    // For each word of data, get both the Unicode equivalent and the hex
    // representation.
    bytesdone = 0;
    for (byteindex = 0; byteindex < dumplen; byteindex++) {
        hexindex = 3 * ((byteindex % 16) + ((byteindex % 16) / 4));   // 3 characters per byte, plus a 3-character space after every 4 bytes.
        uniindex = ((byteindex / 2) % 8) + ((byteindex / 2) % 8) / 8; // 1 character every other byte, plus a 1-character space after every 8 bytes.
        if (byteindex < size) {
            byte = ((PBYTE)address)[byteindex];
            _snwprintf_s(formatbuf, BYTEFORMATBUFFERLENGTH, _TRUNCATE, L"%.2X ", byte);
            formatbuf[BYTEFORMATBUFFERLENGTH - 1] = '\0';
            wcsncpy_s(hexdump + hexindex, HEXDUMPLINELENGTH - hexindex, formatbuf, 4);
            if (((byteindex % 2) == 0) && ((byteindex + 1) < dumplen)) {
                // On every even byte, print one character.
                word = ((PWORD)address)[byteindex / 2];
                if ((word == 0x0000) || (word == 0x0020)) {
                    unidump[uniindex] = L'.';
                }
                else {
                    unidump[uniindex] = word;
                }
            }
        }
        else {
            // Add padding to fill out the last line to 16 bytes.
            wcsncpy_s(hexdump + hexindex, HEXDUMPLINELENGTH - hexindex, L"   ", 4);
            unidump[uniindex] = L'.';
        }
        bytesdone++;
        if ((bytesdone % 16) == 0) {
            // Print one line of data for every 16 bytes. Include the
            // ASCII dump and the hex dump side-by-side.
            report(L"    %s    %s\n", hexdump, unidump);
        }
        else {
            if ((bytesdone % 8) == 0) {
                // Add a spacer in the ASCII dump after every 8 bytes.
                unidump[uniindex + 1] = L' ';
            }
            if ((bytesdone % 4) == 0) {
                // Add a spacer in the hex dump after every 4 bytes.
                wcsncpy_s(hexdump + hexindex + 3, HEXDUMPLINELENGTH - hexindex - 3, L"   ", 4);
            }
        }
    }
}

// findimport - Determines if the specified module imports the named import
//   from the named exporting module.
//
//  - importmodule (IN): Handle (base address) of the module to be searched to
//      see if it imports the specified import.
//
//  - exportmodule (IN): Handle (base address) of the module that exports the
//      import to be searched for.
//
//  - exportmodulename (IN): ANSI string containing the name of the module that
//      exports the import to be searched for.
//
//  - importname (IN): ANSI string containing the name of the import to search
//      for. May be an integer cast to a string if the import is exported by
//      ordinal.
//
//  Return Value:
//
//    Returns TRUE if the module imports to the specified import. Otherwise
//    returns FALSE.
//
BOOL findimport (HMODULE importmodule, HMODULE exportmodule, LPCSTR exportmodulename, LPCSTR importname)
{
    IMAGE_THUNK_DATA        *iate;
    IMAGE_IMPORT_DESCRIPTOR *idte;
    FARPROC                  import;
    IMAGE_SECTION_HEADER    *section;
    ULONG                    size;
            
    // Locate the importing module's Import Directory Table (IDT) entry for the
    // exporting module. The importing module actually can have several IATs --
    // one for each export module that it imports something from. The IDT entry
    // gives us the offset of the IAT for the module we are interested in.
    EnterCriticalSection(&imagelock);
    idte = (IMAGE_IMPORT_DESCRIPTOR*)ImageDirectoryEntryToDataEx((PVOID)importmodule, TRUE,
                                                                  IMAGE_DIRECTORY_ENTRY_IMPORT, &size, &section);
    LeaveCriticalSection(&imagelock);
    if (idte == NULL) {
        // This module has no IDT (i.e. it imports nothing).
        return FALSE;
    }
    while (idte->OriginalFirstThunk != 0x0) {
        if (_stricmp((PCHAR)R2VA(importmodule, idte->Name), exportmodulename) == 0) {
            // Found the IDT entry for the exporting module.
            break;
        }
        idte++;
    }
    if (idte->OriginalFirstThunk == 0x0) {
        // The importing module does not import anything from the exporting
        // module.
        return FALSE;
    }
    
    // Get the *real* address of the import. If we find this address in the IAT,
    // then we've found that the module does import the named import.
    import = GetProcAddress(exportmodule, importname);
    assert(import != NULL); // Perhaps the named export module does not actually export the named import?

    // Locate the import's IAT entry.
    iate = (IMAGE_THUNK_DATA*)R2VA(importmodule, idte->FirstThunk);
    while (iate->u1.Function != 0x0) {
        if (iate->u1.Function == (DWORD_PTR)import) {
            // Found the IAT entry. The module imports the named import.
            return TRUE;
        }
        iate++;
    }

    // The module does not import the named import.
    return FALSE;
}

// findpatch - Determines if the specified module has been patched to use the
//   specified replacement.
//
//  - importmodule (IN): Handle (base address) of the module to be searched to
//      see if it imports the specified replacement export.
//
//  - exportmodulename (IN): ANSI string containing the name of the module that
//      normally exports that import that would have been patched to use the
//      replacement export.
//
//  - replacement (IN): Address of the replacement, or destination, function or
//      variable to search for.
//
//  Return Value:
//
//    Returns TRUE if the module has been patched to use the specified
//    replacement export.
//
BOOL findpatch (HMODULE importmodule, moduleentry_t *module)
{
    IMAGE_IMPORT_DESCRIPTOR *idte;
    IMAGE_SECTION_HEADER    *section;
    ULONG                    size;

    // Locate the importing module's Import Directory Table (IDT) entry for the
    // exporting module. The importing module actually can have several IATs --
    // one for each export module that it imports something from. The IDT entry
    // gives us the offset of the IAT for the module we are interested in.
    EnterCriticalSection(&imagelock);
    idte = (IMAGE_IMPORT_DESCRIPTOR*)ImageDirectoryEntryToDataEx((PVOID)importmodule, TRUE,
        IMAGE_DIRECTORY_ENTRY_IMPORT, &size, &section);
    LeaveCriticalSection(&imagelock);
    if (idte == NULL) {
        // This module has no IDT (i.e. it imports nothing).
        return FALSE;
    }
    while (idte->OriginalFirstThunk != 0x0) {
        if (_stricmp((PCHAR)R2VA(importmodule, idte->Name), module->exportmodulename) == 0) {
            // Found the IDT entry for the exporting module.
            break;
        }
        idte++;
    }
    if (idte->OriginalFirstThunk == 0x0) {
        // The importing module does not import anything from the exporting
        // module.
        return FALSE;
    }

    int i = 0;
    patchentry_t *entry = module->patchtable;
    while(entry->importname)
    {
        LPCVOID replacement = entry->replacement;

        IMAGE_THUNK_DATA        *iate;

        // Locate the replacement's IAT entry.
        iate = (IMAGE_THUNK_DATA*)R2VA(importmodule, idte->FirstThunk);
        while (iate->u1.Function != 0x0) {
            if (iate->u1.Function == (DWORD_PTR)replacement) {
                // Found the IAT entry for the replacement. This patch has been
                // installed.
                return TRUE;
            }
            iate++;
        }
        entry++; i++;
    }

    // The module does not import the replacement. The patch has not been
    // installed.
    return FALSE;
}

// insertreportdelay - Sets the report function to sleep for a bit after each
//   call to OutputDebugString, in order to allow the debugger to catch up.
//
//  Return Value:
//
//    None.
//
VOID insertreportdelay ()
{
    reportdelay = TRUE;
}

// moduleispatched - Checks to see if any of the imports listed in the specified
//   patch table have been patched into the specified importmodule.
//
//  - importmodule (IN): Handle (base address) of the module to be queried to
//      determine if it has been patched.
//
//  - patchtable (IN): An array of patchentry_t structures specifying all of the
//      import patches to search for.
//
//  - tablesize (IN): Size, in entries, of the specified patch table.
//
//  Return Value:
//
//    Returns TRUE if at least one of the patches listed in the patch table is
//    installed in the importmodule. Otherwise returns FALSE.
//
BOOL moduleispatched (HMODULE importmodule, moduleentry_t patchtable [], UINT tablesize)
{
    moduleentry_t *entry;
    BOOL          found = FALSE;
    UINT          index;

    // Loop through the import patch table, individually checking each patch
    // entry to see if it is installed in the import module. If any patch entry
    // is installed in the import module, then the module has been patched.
    for (index = 0; index < tablesize; index++) {
        entry = &patchtable[index];
        found = findpatch(importmodule, entry);
        if (found == TRUE) {
            // Found one of the listed patches installed in the import module.
            return TRUE;
        }
    }

    // No patches listed in the patch table were found in the import module.
    return FALSE;
}

// patchimport - Patches all future calls to an imported function, or references
//   to an imported variable, through to a replacement function or variable.
//   Patching is done by replacing the import's address in the specified target
//   module's Import Address Table (IAT) with the address of the replacement
//   function or variable.
//
//  - importmodule (IN): Handle (base address) of the target module for which
//      calls or references to the import should be patched.
//
//  - exportmodule (IN): Handle (base address) of the module that exports the
//      the function or variable to be patched.
//
//  - exportmodulename (IN): ANSI string containing the name of the module that
//      exports the function or variable to be patched.
//
//  - importname (IN): ANSI string containing the name of the imported function
//      or variable to be patched. May be an integer cast to a string if the
//      import is exported by ordinal.
//
//  - replacement (IN): Address of the function or variable to which future
//      calls or references should be patched through to. This function or
//      variable can be thought of as effectively replacing the original import
//      from the point of view of the module specified by "importmodule".
//
//  Return Value:
//
//    Returns TRUE if the patch was installed into the import module. If the
//    import module does not import the specified export, so nothing changed,
//    then FALSE will be returned.
//   
BOOL patchimport (HMODULE importmodule, moduleentry_t *module)
{
    DWORD result = 0;
    HMODULE exportmodule = (HMODULE)module->modulebase;
    LPCSTR exportmodulename = module->exportmodulename;

    IMAGE_IMPORT_DESCRIPTOR *idte;
    IMAGE_SECTION_HEADER    *section;
    ULONG                    size;

    // Locate the importing module's Import Directory Table (IDT) entry for the
    // exporting module. The importing module actually can have several IATs --
    // one for each export module that it imports something from. The IDT entry
    // gives us the offset of the IAT for the module we are interested in.
    EnterCriticalSection(&imagelock);
    idte = (IMAGE_IMPORT_DESCRIPTOR*)ImageDirectoryEntryToDataEx((PVOID)importmodule, TRUE,
        IMAGE_DIRECTORY_ENTRY_IMPORT, &size, &section);
    LeaveCriticalSection(&imagelock);
    if (idte == NULL) {
        // This module has no IDT (i.e. it imports nothing).
        return FALSE;
    }
    while (idte->FirstThunk != 0x0) {
        if (_stricmp((PCHAR)R2VA(importmodule, idte->Name), exportmodulename) == 0) {
            // Found the IDT entry for the exporting module.
            break;
        }
        idte++;
    }
    if (idte->FirstThunk == 0x0) {
        // The importing module does not import anything from the exporting
        // module.
        return FALSE;
    }

    patchentry_t *entry = module->patchtable;
    int i = 0;
    while(entry->importname)
    {
        LPCSTR importname   = entry->importname;
        LPCVOID replacement = entry->replacement;
        IMAGE_THUNK_DATA        *iate;
        DWORD                    protect;
        FARPROC                  import = NULL;
        FARPROC                  import2 = NULL;

        // Get the *real* address of the import. If we find this address in the IAT,
        // then we've found the entry that needs to be patched.
        import2 = VisualLeakDetector::_RGetProcAddress(exportmodule, importname);
        import = GetProcAddress(exportmodule, importname);
        if ( import2 )
            import = import2;

        assert(import != NULL); // Perhaps the named export module does not actually export the named import?

        // Locate the import's IAT entry.
        iate = (IMAGE_THUNK_DATA*)R2VA(importmodule, idte->FirstThunk);
        while (iate->u1.Function != 0x0) {
            if (iate->u1.Function == (DWORD_PTR)import) {
                // Found the IAT entry. Overwrite the address stored in the IAT
                // entry with the address of the replacement. Note that the IAT
                // entry may be write-protected, so we must first ensure that it is
                // writable.
                if ( import != replacement )
                {
                    VirtualProtect(&iate->u1.Function, sizeof(iate->u1.Function), PAGE_EXECUTE_READWRITE, &protect);
                    iate->u1.Function = (DWORD_PTR)replacement;
                    VirtualProtect(&iate->u1.Function, sizeof(iate->u1.Function), protect, &protect);
                }
                // The patch has been installed in the import module.
                result++;
            }
            iate++;
        }
        entry++; i++;
    }

    // The import's IAT entry was not found. The importing module does not
    // import the specified import.
    return result > 0;
}

// patchmodule - Patches all imports listed in the supplied patch table, and
//   which are imported by the specified module, through to their respective
//   replacement functions.
//
//   Note: If the specified module does not import any of the functions listed
//     in the patch table, then nothing is changed for the specified module.
//
//  - importmodule (IN): Handle (base address) of the target module which is to
//      have its imports patched.
//
//  - patchtable (IN): An array of patchentry_t structures specifying all of the
//      imports to patch for the specified module.
//
//  - tablesize (IN): Size, in entries, of the specified patch table.
//
//  Return Value:
//
//    Returns TRUE if at least one of the patches listed in the patch table was
//    installed in the importmodule. Otherwise returns FALSE.
//
BOOL patchmodule (HMODULE importmodule, moduleentry_t patchtable [], UINT tablesize)
{
    moduleentry_t *entry;
    UINT          index;
    BOOL          patched = FALSE;

    // Loop through the import patch table, individually patching each import
    // listed in the table.
    for (index = 0; index < tablesize; index++) {
        entry = &patchtable[index];
        if (patchimport(importmodule, entry) == TRUE) {
            patched = TRUE;
        }
    }

    return patched;
}

// report - Sends a printf-style formatted message to the debugger for display
//   and/or to a file.
//
//   Note: A message longer than MAXREPORTLENGTH characters will be truncated
//     to MAXREPORTLENGTH.
//
//  - format (IN): Specifies a printf-compliant format string containing the
//      message to be sent to the debugger.
//
//  - ... (IN): Arguments to be formatted using the specified format string.
//
//  Return Value:
//
//    None.
//
VOID report (LPCWSTR format, ...)
{
    va_list args;
    size_t  count;
    CHAR    messagea [MAXREPORTLENGTH + 1];
    WCHAR   messagew [MAXREPORTLENGTH + 1];

    va_start(args, format);
    _vsnwprintf_s(messagew, MAXREPORTLENGTH + 1, _TRUNCATE, format, args);
    va_end(args);
    messagew[MAXREPORTLENGTH] = L'\0';

    if (reportencoding == unicode) {
        if (reportfile != NULL) {
            // Send the report to the previously specified file.
            fwrite(messagew, sizeof(WCHAR), wcslen(messagew), reportfile);
        }
        if ( reporttostdout )
            fwprintf(stdout,messagew);

        if (reporttodebugger) {
            OutputDebugStringW(messagew);
        }
    }
    else {
        if (wcstombs_s(&count, messagea, MAXREPORTLENGTH + 1, messagew, _TRUNCATE) == -1) {
            // Failed to convert the Unicode message to ASCII.
            assert(FALSE);
            return;
        }
        messagea[MAXREPORTLENGTH] = '\0';
        if (reportfile != NULL) {
            // Send the report to the previously specified file.
            fwrite(messagea, sizeof(CHAR), strlen(messagea), reportfile);
        }

        if ( reporttostdout )
            printf(messagea);

        if (reporttodebugger) {
            OutputDebugStringA(messagea);
        }
    }

    if (reporttodebugger && (reportdelay == TRUE)) {
        Sleep(10); // Workaround the Visual Studio 6 bug where debug strings are sometimes lost if they're sent too fast.
    }
}

// restoreimport - Restores the IAT entry for an import previously patched via
//   a call to "patchimport" to the original address of the import.
//
//  - importmodule (IN): Handle (base address) of the target module for which
//      calls or references to the import should be restored.
//
//  - exportmodule (IN): Handle (base address) of the module that exports the
//      function or variable to be patched.
//
//  - exportmodulename (IN): ANSI string containing the name of the module that
//      exports the function or variable to be patched.
//
//  - importname (IN): ANSI string containing the name of the imported function
//      or variable to be restored. May be an integer cast to a string if the
//      import is exported by ordinal.
//
//  - replacement (IN): Address of the function or variable which the import was
//      previously patched through to via a call to "patchimport".
//
//  Return Value:
//
//    None.
//   
VOID restoreimport (HMODULE importmodule, moduleentry_t* module)
{
    IMAGE_IMPORT_DESCRIPTOR *idte;
    IMAGE_SECTION_HEADER    *section;
    ULONG                    size;

    HMODULE exportmodule = (HMODULE)module->modulebase;
    LPCSTR exportmodulename = module->exportmodulename;

    // Locate the importing module's Import Directory Table (IDT) entry for the
    // exporting module. The importing module actually can have several IATs --
    // one for each export module that it imports something from. The IDT entry
    // gives us the offset of the IAT for the module we are interested in.
    EnterCriticalSection(&imagelock);
    idte = (IMAGE_IMPORT_DESCRIPTOR*)ImageDirectoryEntryToDataEx((PVOID)importmodule, TRUE,
        IMAGE_DIRECTORY_ENTRY_IMPORT, &size, &section);
    LeaveCriticalSection(&imagelock);
    if (idte == NULL) {
        // This module has no IDT (i.e. it imports nothing).
        return;
    }
    while (idte->OriginalFirstThunk != 0x0) {
        if (_stricmp((PCHAR)R2VA(importmodule, idte->Name), exportmodulename) == 0) {
            // Found the IDT entry for the exporting module.
            break;
        }
        idte++;
    }
    if (idte->OriginalFirstThunk == 0x0) {
        // The importing module does not import anything from the exporting
        // module.
        return;
    }

    int i = 0;
    patchentry_t *entry = module->patchtable;
    while(entry->importname)
    {
        IMAGE_THUNK_DATA        *iate;
        FARPROC                  import;
        DWORD                    protect;

        LPCSTR importname   = entry->importname;
        LPCVOID replacement = entry->replacement;

        // Get the *real* address of the import.
        import = GetProcAddress(exportmodule, importname);
        assert(import != NULL); // Perhaps the named export module does not actually export the named import?

        // Locate the import's original IAT entry (it currently has the replacement
        // address in it).
        iate = (IMAGE_THUNK_DATA*)R2VA(importmodule, idte->FirstThunk);
        while (iate->u1.Function != 0x0) {
            if (iate->u1.Function == (DWORD_PTR)replacement) {
                // Found the IAT entry. Overwrite the address stored in the IAT
                // entry with the import's real address. Note that the IAT entry may
                // be write-protected, so we must first ensure that it is writable.
                VirtualProtect(&iate->u1.Function, sizeof(iate->u1.Function), PAGE_EXECUTE_READWRITE, &protect);
                iate->u1.Function = (DWORD_PTR)import;
                VirtualProtect(&iate->u1.Function, sizeof(iate->u1.Function), protect, &protect);
                break;
            }
            iate++;
        }
        entry++; i++;
    }
}

// restoremodule - Restores all imports listed in the supplied patch table, and
//   which are imported by the specified module, to their original functions.
//
//   Note: If the specified module does not import any of the functions listed
//     in the patch table, then nothing is changed for the specified module.
//
//  - importmodule (IN): Handle (base address) of the target module which is to
//      have its imports restored.
//
//  - patchtable (IN): Array of patchentry_t structures specifying all of the
//      imports to restore for the specified module.
//
//  - tablesize (IN): Size, in entries, of the specified patch table.
//
//  Return Value:
//
//    None.
//
VOID restoremodule (HMODULE importmodule, moduleentry_t patchtable [], UINT tablesize)
{
    moduleentry_t *entry;
    UINT          index;

    // Loop through the import patch table, individually restoring each import
    // listed in the table.
    for (index = 0; index < tablesize; index++) {
        entry = &patchtable[index];
        restoreimport(importmodule, entry);
    }
}

// setreportencoding - Sets the output encoding of report messages to either
//   ASCII (the default) or Unicode.
//
//  - encoding (IN): Specifies either "ascii" or "unicode".
//
//  Return Value:
//
//    None.
//
VOID setreportencoding (encoding_e encoding)
{
    switch (encoding) {
    case ascii:
    case unicode:
        reportencoding = encoding;
        break;

    default:
        assert(FALSE);
    }
}

// setreportfile - Sets a destination file to which all report messages should
//   be sent. If this function is not called to set a destination file, then
//   report messages will be sent to the debugger instead of to a file.
//
//  - file (IN): Pointer to an open file, to which future report messages should
//      be sent.
//
//  - copydebugger (IN): If true, in addition to sending report messages to
//      the specified file, a copy of each message will also be sent to the
//      debugger.
//
//  Return Value:
//
//    None.
//
VOID setreportfile (FILE *file, BOOL copydebugger, BOOL tostdout)
{
    reportfile = file;
    reporttodebugger = copydebugger;
    reporttostdout = tostdout;
}

// strapp - Appends the specified source string to the specified destination
//   string. Allocates additional space so that the destination string "grows"
//   as new strings are appended to it. This function is fairly infrequently
//   used so efficiency is not a major concern.
//
//  - dest (IN/OUT): Address of the destination string. Receives the resulting
//      combined string after the append operation.
//
//  - source (IN): Source string to be appended to the destination string.
//
//  Return Value:
//
//    None.
//
VOID strapp (LPWSTR *dest, LPCWSTR source)
{
    SIZE_T length;
    LPWSTR temp;

    temp = *dest;
    length = wcslen(*dest) + wcslen(source);
    *dest = new WCHAR [length + 1];
    wcsncpy_s(*dest, length + 1, temp, _TRUNCATE);
    wcsncat_s(*dest, length + 1, source, _TRUNCATE);
    delete [] temp;
}

// strtobool - Converts string values (e.g. "yes", "no", "on", "off") to boolean
//   values.
//
//  - s (IN): String value to convert.
//
//  Return Value:
//
//    Returns TRUE if the string is recognized as a "true" string. Otherwise
//    returns FALSE.
//
BOOL strtobool (LPCWSTR s) {
    WCHAR *end;

    if ((_wcsicmp(s, L"true") == 0) ||
        (_wcsicmp(s, L"yes") == 0) ||
        (_wcsicmp(s, L"on") == 0) ||
        (wcstol(s, &end, 10) == 1)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

// _GetProcessIdOfThread - Returns the ID of the process owns the thread.
//
//  - thread (IN): The handle to the thread.
//
//  Return Value:
//
//    Returns the ID of the process that owns the thread. Otherwise returns 0.
//
DWORD _GetProcessIdOfThread (HANDLE thread)
{
    typedef struct _CLIENT_ID {
        HANDLE UniqueProcess;
        HANDLE UniqueThread;
    } CLIENT_ID, *PCLIENT_ID;

    typedef LONG NTSTATUS;
    typedef LONG KPRIORITY;

    typedef struct _THREAD_BASIC_INFORMATION {
        NTSTATUS  ExitStatus;
        PVOID     TebBaseAddress;
        CLIENT_ID ClientId;
        KAFFINITY AffinityMask;
        KPRIORITY Priority;
        KPRIORITY BasePriority;
    } THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

    const static THREADINFOCLASS ThreadBasicInformation = (THREADINFOCLASS)0;

    typedef NTSTATUS (WINAPI *PNtQueryInformationThread) (HANDLE thread,
                THREADINFOCLASS infoclass, PVOID buffer, ULONG buffersize,
                PULONG used);

    static PNtQueryInformationThread NtQueryInformationThread = NULL;

    THREAD_BASIC_INFORMATION tbi;
    NTSTATUS status;
    HMODULE  ntdll;
    if (NtQueryInformationThread == NULL) {
        ntdll = GetModuleHandleW(L"ntdll.dll");
        NtQueryInformationThread = (PNtQueryInformationThread)GetProcAddress(ntdll, "NtQueryInformationThread");
        if (NtQueryInformationThread == NULL) {
            return 0;
        }
    }

    status = NtQueryInformationThread(thread, ThreadBasicInformation, &tbi, sizeof(tbi), NULL);
    if(status < 0) {
        // Shall we go through all the trouble of setting last error?
        return 0;
    }

    return (DWORD)tbi.ClientId.UniqueProcess;
}

static const DWORD crctab[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
};

DWORD CalculateCRC32(UINT_PTR p, UINT startValue)
{      
    register DWORD hash = startValue;
    hash = (hash >> 8) ^ crctab[(hash & 0xff) ^ ((p >>  0) & 0xff)];
    hash = (hash >> 8) ^ crctab[(hash & 0xff) ^ ((p >>  8) & 0xff)];
    hash = (hash >> 8) ^ crctab[(hash & 0xff) ^ ((p >> 16) & 0xff)];
    hash = (hash >> 8) ^ crctab[(hash & 0xff) ^ ((p >> 24) & 0xff)];
#ifdef WIN64
    hash = (hash >> 8) ^ crctab[(hash & 0xff) ^ ((p >> 32) & 0xff)];
    hash = (hash >> 8) ^ crctab[(hash & 0xff) ^ ((p >> 40) & 0xff)];
    hash = (hash >> 8) ^ crctab[(hash & 0xff) ^ ((p >> 48) & 0xff)];
    hash = (hash >> 8) ^ crctab[(hash & 0xff) ^ ((p >> 56) & 0xff)];
#endif
    return hash;
}
