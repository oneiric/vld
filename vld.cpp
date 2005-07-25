////////////////////////////////////////////////////////////////////////////////
//  $Id: vld.cpp,v 1.20 2005/07/25 22:40:04 dmouldin Exp $
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

#define VLDBUILD    // Declares that we are building Visual Leak Detector
#include "vld.h"    // Provides common Visual Leak Detector definitions.
#include "vldint.h" // Provides access to the Visual Leak Detector internals.

// Configuration options defined in vld.h
extern "C" unsigned long _VLD_configflags;
extern "C" unsigned long _VLD_maxdatadump;
extern "C" unsigned long _VLD_maxtraceframes;

// Pointers to explicitly dynamically linked functions exported from dbghelp.dll.
static StackWalk64_t              pStackWalk64;
static SymCleanup_t               pSymCleanup;
static SymFromAddr_t              pSymFromAddr;
static SymFunctionTableAccess64_t pSymFunctionTableAccess64;
static SymGetModuleBase64_t       pSymGetModuleBase64;
static SymGetLineFromAddr64_t     pSymGetLineFromAddr64;
static SymInitialize_t            pSymInitialize;
static SymSetOptions_t            pSymSetOptions;

// The one and only VisualLeakDetector object instance. This is placed in the
// "compiler" initialization area, so that it gets constructed during C runtime
// initialization and before any user global objects are constructed. Also,
// disable the warning about us using the "compiler" initialization area.
#pragma warning (disable:4074)
#pragma init_seg (compiler)
VisualLeakDetector visualleakdetector;

// Constructor - Dynamically links with the Debug Help Library and installs the
//   allocation hook function so that the C runtime's debug heap manager will
//   call the hook function for every heap request.
//
VisualLeakDetector::VisualLeakDetector ()
{
    // Initialize private data.
    m_mallocmap  = new BlockMap;
    m_process    = GetCurrentProcess();
    m_status     = 0x0;
    m_thread     = GetCurrentThread();
    m_tlsindex   = TlsAlloc();

    if (m_tlsindex == TLS_OUT_OF_INDEXES) {
        report("ERROR: Visual Leak Detector: Couldn't allocate thread local storage.\n");
    }
    else if (linkdebughelplibrary()) {
        // Register our allocation hook function with the debug heap.
        m_poldhook = _CrtSetAllocHook(allochook);
        report("Visual Leak Detector Version "VLD_VERSION" installed ("VLD_LIBTYPE").\n");
        if (_VLD_configflags & VLD_CONFIG_START_DISABLED) {
            // Memory leak detection will initially be disabled.
            m_status |= VLD_STATUS_NEVER_ENABLED;
        }
        m_status |= VLD_STATUS_INSTALLED;
        return;
    }
    
    report("Visual Leak Detector IS NOT installed!\n");
}

// Destructor - Unhooks the hook function and outputs a memory leak report.
//
VisualLeakDetector::~VisualLeakDetector ()
{
    _CrtMemBlockHeader *pheader;
    char               *pheap;
    _CRT_ALLOC_HOOK     pprevhook;

#ifdef _MT
        _mlock(_HEAP_LOCK);
#endif // _MT

    if (m_status & VLD_STATUS_INSTALLED) {
        // Deregister the hook function.
        pprevhook = _CrtSetAllocHook(m_poldhook);
        if (pprevhook != allochook) {
            // WTF? Somebody replaced our hook before we were done. Put theirs
            // back, but notify the human about the situation.
            _CrtSetAllocHook(pprevhook);
            report("WARNING: Visual Leak Detector: The CRT allocation hook function was unhooked prematurely!\n"
                   "    There's a good possibility that any potential leaks have gone undetected!\n");
        }

        if (m_status & VLD_STATUS_NEVER_ENABLED) {
            // Visual Leak Detector started with leak detection disabled and
            // it was never enabled at runtime. A lot of good that does.
            report("WARNING: Visual Leak Detector: Memory leak detection was never enabled.\n");
        }
        else {
            // Generate a memory leak report.
            reportleaks();
        }

        // Unload the Debug Help Library.
        FreeLibrary(m_dbghelp);
    }

    // Free internally allocated resources.
    delete m_mallocmap;

    // Do a memory leak self-check.
    pheap = new char;
    pheader = pHdr(pheap)->pBlockHeaderNext;
    delete pheap;
    while (pheader) {
        if (_BLOCK_SUBTYPE(pheader->nBlockUse) == VLDINTERNALBLOCK) {
            // Doh! VLD still has an internally allocated block!
            // This won't ever actually happen, right guys?... guys?
            report("ERROR: Visual Leak Detector: Detected a memory leak internal to Visual Leak Detector!!\n");
            report("---------- Block %ld at "ADDRESSFORMAT": %u bytes ----------\n", pheader->lRequest, pbData(pheader), pheader->nDataSize);
            report("%s (%d): Full call stack not available.\n", pheader->szFileName, pheader->nLine);
            dumpuserdatablock(pheader);
            report("\n");
        }
        pheader = pheader->pBlockHeaderNext;
    }

#ifdef _MT
        _munlock(_HEAP_LOCK);
#endif // _MT

    report("Visual Leak Detector is now exiting.\n");
}

// allochook - This is a hook function that is installed into Microsoft's
//   CRT debug heap when the VisualLeakDetector object is constructed. Any time
//   an allocation, reallocation, or free is made from/to the debug heap,
//   the CRT will call into this hook function.
//
//  Note: The debug heap serializes calls to this function (i.e. the debug heap
//    is locked prior to calling this function). So we don't need to worry about
//    thread safety -- it's already taken care of for us.
//
//  - type (IN): Specifies the type of request (alloc, realloc, or free).
//
//  - pdata (IN): On a free allocation request, contains a pointer to the
//      user data section of the memory block being freed. On alloc requests,
//      this pointer will be NULL because no block has actually been allocated
//      yet.
//
//  - size (IN): Specifies the size (either real or requested) of the user
//      data section of the memory block being freed or requested. This function
//      ignores this value.
//
//  - use (IN): Specifies the "use" type of the block. This can indicate the
//      purpose of the block being requested. It can be for internal use by
//      the CRT, it can be an application defined "client" block, or it can
//      simply be a normal block. Client blocks are just normal blocks that
//      have been specifically tagged by the application so that the application
//      can separately keep track of the tagged blocks for debugging purposes.
//
//  - request (IN): Specifies the allocation request number. This is basically
//      a sequence number that is incremented for each allocation request. It
//      is used to uniquely identify each allocation.
//
//  - filename (IN): String containing the filename of the source line that
//      initiated this request. This function ignores this value.
//
//  - line (IN): Line number within the source file that initiated this request.
//      This function ignores this value.
//
//  Return Value:
//
//    Always returns true, unless another allocation hook function was already
//    installed before our hook function was called, in which case we'll return
//    whatever value the other hook function returns. Returning false will
//    cause the debug heap to deny the pending allocation request (this can be
//    useful for simulating out of memory conditions, but Visual Leak Detector
//    has no need to make use of this capability).
//
int VisualLeakDetector::allochook (int type, void *pdata, size_t size, int use, long request, const unsigned char *file, int line)
{
    static bool inallochook = false;
    int         status = true;

    if (inallochook || (use == _CRT_BLOCK)) {
        // Prevent the current thread from re-entering on allocs/reallocs/frees
        // that we or the CRT do internally to record the data we collect.
        if (visualleakdetector.m_poldhook) {
            status = visualleakdetector.m_poldhook(type, pdata, size, use, request, file, line);
        }
        return status;
    }
    inallochook = true;

    // Call the appropriate handler for the type of operation.
    switch (type) {
    case _HOOK_ALLOC:
        visualleakdetector.hookmalloc(request);
        break;

    case _HOOK_FREE:
        visualleakdetector.hookfree(pdata);
        break;

    case _HOOK_REALLOC:
        visualleakdetector.hookrealloc(pdata, request);
        break;

    default:
        visualleakdetector.report("WARNING: Visual Leak Detector: in allochook(): Unhandled allocation type (%d).\n", type);
        break;
    }

    if (visualleakdetector.m_poldhook) {
        status = visualleakdetector.m_poldhook(type, pdata, size, use, request, file, line);
    }
    inallochook = false;
    return status;
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
    char       *env;
    size_t      index;
    size_t      length;
    HMODULE     module;
    char       *path = new char [MAX_PATH];
    size_t      pos = 0;

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

// dumpuserdatablock - Dumps a nicely formatted rendition of the user-data
//   portion of a memory block to the debugger's output window. Includes both
//   the hex value of each byte and its ASCII equivalent (if printable).
//
//   By default the entire user data section of each block is dumped. However,
//   the data dump can be restricted to a limited number of bytes via
//   _VLD_maxdatadump.
//
//  - pheader (IN): Pointer to the header of the memory block to dump.
//
//  Return Value:
//
//    None.
//
void VisualLeakDetector::dumpuserdatablock (const _CrtMemBlockHeader *pheader)
{
    char          ascdump [18] = {0};
    size_t        ascindex;
    unsigned long byte;
    unsigned long bytesdone;
    unsigned long datalen;
    unsigned char datum;
    unsigned long dumplen;
    char          formatbuf [4];
    char          hexdump [58] = {0};
    size_t        hexindex;

    datalen = (_VLD_maxdatadump < pheader->nDataSize) ? _VLD_maxdatadump : pheader->nDataSize;

    // Each line of output is 16 bytes.
    if ((datalen % 16) == 0) {
        // No padding needed.
        dumplen = datalen;
    }
    else {
        // We'll need to pad the last line out to 16 bytes.
        dumplen = datalen + (16 - (datalen % 16));
    }

    // For each byte of data, get both the ASCII equivalent (if it is a
    // printable character) and the hex representation.
    report("  Data:\n");
    bytesdone = 0;
    for (byte = 0; byte < dumplen; byte++) {
        hexindex = 3 * ((byte % 16) + ((byte % 16) / 4)); // 3 characters per byte, plus a 3-character space after every 4 bytes.
        ascindex = (byte % 16) + (byte % 16) / 8; // 1 character per byte, plus a 1-character space after every 8 bytes.
        if (byte < datalen) {
            datum = ((unsigned char*)pbData(pheader))[byte];
            sprintf(formatbuf, "%.2X ", datum);
            strncpy(hexdump + hexindex, formatbuf, 4);
            if (isprint(datum) && (datum != ' ')) {
                ascdump[ascindex] = datum;
            }
            else {
                ascdump[ascindex] = '.';
            }
        }
        else {
            // Add padding to fill out the last line to 16 bytes.
            strncpy(hexdump + hexindex, "   ", 4);
            ascdump[ascindex] = '.';
        }
        bytesdone++;
        if ((bytesdone % 16) == 0) {
            // Print one line of data for every 16 bytes. Include the
            // ASCII dump and the hex dump side-by-side.
            report("    %s    %s\n", hexdump, ascdump);
        }
        else {
            if ((bytesdone % 8) == 0) {
                // Add a spacer in the ASCII dump after every two words.
                ascdump[ascindex + 1] = ' ';
            }
            if ((bytesdone % 4) == 0) {
                // Add a spacer in the hex dump after every word.
                strncpy(hexdump + hexindex + 3, "   ", 4);
            }
        }
    }
}

// enabled - Determines if memory leak detection is enabled for the current
//   thread.
//
//  Return Value:
//
//    Returns true if Visual Leak Detector is enabled for the current thread.
//    Otherwise, the function returns false.
//
bool VisualLeakDetector::enabled ()
{
    unsigned long status;

    status = (unsigned long)TlsGetValue(m_tlsindex);
    if (status == VLD_TLS_UNINITIALIZED) {
        // TLS is uninitialized for the current thread. Use the initial state.
        if (_VLD_configflags & VLD_CONFIG_START_DISABLED) {
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

// eraseduplicates - Erases, from the block map, blocks that appear to be
//   duplicate leaks of an already identified leak. This is used to avoid
//   finding duplicate leaks when running the leak report with the "aggregate
//   duplicates" feature turned on.
//
//  - pheader (IN): Pointer to the memory block header on the allocated list
//      from which to start searching for duplicates. Presumably, any duplicates
//      prior to this entry would already have been erased.
//
//  - size (IN): The size of the block for which to find duplicates. Blocks with
//      a different size will not be considered duplicates, even if the call
//      stack is identical.
//
//  - callstack (IN): Pointer to the CallStack of the block for which to search
//      for duplicates. Blocks with both matching size and call stack will be
//      considered duplicates and will be erased.
//
//  Return Value:
//
//    Returns the number of duplicate blocks erased from the block map.
//
unsigned long VisualLeakDetector::eraseduplicates (const _CrtMemBlockHeader *pheader, size_t size, const CallStack *callstack)
{
    unsigned long  erased = 0;
    CallStack     *target;

#ifdef _MT
    _mlock(_HEAP_LOCK);
#endif // _MT

    // Walk the allocated list, starting from the indicated block, looking for
    // other blocks with the same size and call stack as those specified.
    while (pheader) {
        if ((_BLOCK_SUBTYPE(pheader->nBlockUse) == VLDINTERNALBLOCK) || (pheader->nDataSize != size)) {
            // Skip internally allocated blocks and blocks of different sizes.
            pheader = pheader->pBlockHeaderNext;
            continue;
        }
        target = m_mallocmap->find(pheader->lRequest);
        if (target && ((*target) == (*callstack))) {
            // We've found a duplicate. Erase it.
            m_mallocmap->erase(pheader->lRequest);
            erased++;
        }
        pheader = pheader->pBlockHeaderNext;
    }

#ifdef _MT
    _munlock(_HEAP_LOCK);
#endif // _MT

    return erased;
}

// getprogramcounterx86x64 - Helper function that retrieves the program counter
//   (aka the EIP (x86) or RIP (x64) register) for getstacktrace() on Intel x86
//   or x64 architectures (x64 supports both AMD64 and Intel EM64T). There is no
//   way for software to directly read the EIP/RIP register. But it's value can
//   be obtained by calling into a function (in our case, this function) and
//   then retrieving the return address, which will be the program counter from
//   where the function was called.
//
//  Note: Inlining of this function must be disabled. The whole purpose of this
//    function's existence depends upon it being a *called* function.
//
//  Return Value:
//
//    Returns the return address of the current stack frame.
//
#if defined(_M_IX86) || defined(_M_X64)
#pragma auto_inline(off)
DWORD_PTR VisualLeakDetector::getprogramcounterx86x64 ()
{
    DWORD_PTR programcounter;

    __asm mov AXREG, [BPREG + SIZEOFPTR] // Get the return address out of the current stack frame
    __asm mov [programcounter], AXREG    // Put the return address into the variable we'll return

    return programcounter;
}
#pragma auto_inline(on)
#endif // defined(_M_IX86) || defined(_M_X64)

// getstacktrace - Traces the stack, starting from this function, as far
//   back as possible. Populates the provided CallStack with one entry for each
//   stack frame traced. Requires architecture-specific code for retrieving
//   the current frame pointer and program counter.
//
//   By default, all stack frames are traced. But the trace can be limited to
//   a maximum number of frames via _VLD_maxtraceframes.
//
//  - callstack (OUT): Pointer to an empty CallStack to be populated with
//    entries from the stack trace. Each frame traced will push one entry onto
//    the CallStack.
//
//  Return Value:
//
//    None.
//
void VisualLeakDetector::getstacktrace (CallStack *callstack)
{
    DWORD        architecture;
    CONTEXT      context;
    unsigned int count = 0;
    STACKFRAME64 frame;
    DWORD_PTR    framepointer;
    DWORD_PTR    programcounter;

    // Get the required values for initialization of the STACKFRAME64 structure
    // to be passed to StackWalk64(). Required fields are AddrPC and AddrFrame.
#if defined(_M_IX86) || defined(_M_X64)
    architecture = X86X64ARCHITECTURE;
    programcounter = getprogramcounterx86x64();
    __asm mov [framepointer], BPREG // Get the frame pointer (aka base pointer)
#else
// If you want to retarget Visual Leak Detector to another processor
// architecture then you'll need to provide architecture-specific code to
// retrieve the current frame pointer and program counter in order to initialize
// the STACKFRAME64 structure below.
#error "Visual Leak Detector is not supported on this architecture."
#endif // defined(_M_IX86) || defined(_M_X64)

    // Initialize the STACKFRAME64 structure.
    memset(&frame, 0x0, sizeof(frame));
    frame.AddrPC.Offset    = programcounter;
    frame.AddrPC.Mode      = AddrModeFlat;
    frame.AddrFrame.Offset = framepointer;
    frame.AddrFrame.Mode   = AddrModeFlat;

    // Walk the stack.
    while (count < _VLD_maxtraceframes) {
        count++;
        if (!pStackWalk64(architecture, m_process, m_thread, &frame, &context,
                          NULL, pSymFunctionTableAccess64, pSymGetModuleBase64, NULL)) {
            // Couldn't trace back through any more frames.
            break;
        }
        if (frame.AddrFrame.Offset == 0) {
            // End of stack.
            break;
        }

        // Push this frame's program counter onto the provided CallStack.
        callstack->push_back((DWORD_PTR)frame.AddrPC.Offset);
    }
}

// hookfree - Called by the allocation hook function in response to freeing a
//   block. Removes the block (and it's call stack) from the block map.
//
//  - pdata (IN): Pointer to the user data section of the memory block being
//      freed.
//
//  Return Value:
//
//    None.
//
void VisualLeakDetector::hookfree (const void *pdata)
{
    long request = pHdr(pdata)->lRequest;

    m_mallocmap->erase(request);
}

// hookmalloc - Called by the allocation hook function in response to an
//   allocation. Obtains a stack trace for the allocation and stores the
//   CallStack in the block allocation map along with the allocation request
//   number (which serves as a unique key for mapping each memory block to its
//   call stack).
//
//  - request (IN): The allocation request number. This value is provided to our
//      allocation hook function by the debug heap. We use it to uniquely
//      identify this particular allocation.
//
//  Return Value:
//
//    None.
//
void VisualLeakDetector::hookmalloc (long request)
{
    CallStack *callstack;

    if (!enabled()) {
        // Memory leak detection is disabled. Don't track allocations.
        return;
    }

    callstack = m_mallocmap->insert(request);
    getstacktrace(callstack);
}

// hookrealloc - Called by the allocation hook function in response to
//   reallocating a block. The debug heap insulates us from things such as
//   reallocating a zero size block (the same as a call to free()). So we don't
//   need to check for any special cases such as that. All reallocs are
//   essentially just a free/malloc sequence.
//
//  - pdata (IN): Pointer to the user data section of the memory block being
//      reallocated.
//
//  - request (IN): The allocation request number. This value is provided to our
//      allocation hook function by the debug heap. We use it to uniquely
//      identify this particular reallocation.
//
//  Return Value:
//
//    None.
//
void VisualLeakDetector::hookrealloc (const void *pdata, long request)
{
    // Do a free, then do a malloc.
    hookfree(pdata);
    hookmalloc(request);
}

// linkdebughelplibrary - Performs explicit dynamic linking to dbghelp.dll.
//   Implicitly linking with dbghelp.dll is not desirable because implicit
//   linking requires the import libary (dbghelp.lib). Because VLD is itself a
//   library, the implicit link with the import library will not happen until
//   VLD is linked with an executable. This would be bad because the import
//   library may not exist on the system building the executable. We get around
//   this by explicitly linking with dbghelp.dll. Because dbghelp.dll is
//   redistributable, we can safely assume that it will be on the system
//   building the executable.
//
//  Return Value:
//
//    - Returns "true" if dynamic linking was successful. Successful linking
//      means that the Debug Help Library was found and that all functions were
//      resolved.
//
//    - Returns "false" if dynamic linking failed.
//
bool VisualLeakDetector::linkdebughelplibrary ()
{
    char      *functionname;
    bool       status = true;

    // Load dbghelp.dll, and obtain pointers to the exported functions that we
    // will be using.
    m_dbghelp = LoadLibrary("dbghelp.dll");
    if (m_dbghelp) {
        functionname = "StackWalk64";
        pStackWalk64 = (StackWalk64_t)GetProcAddress(m_dbghelp, functionname);
        if (pStackWalk64 == NULL) {
            goto getprocaddressfailure;
        }
        functionname = "SymFunctionTableAccess64";
        pSymFunctionTableAccess64 = (SymFunctionTableAccess64_t)GetProcAddress(m_dbghelp, functionname);
        if (pSymFunctionTableAccess64 == NULL) {
            goto getprocaddressfailure;
        }
        functionname = "SymGetModuleBase64";
        pSymGetModuleBase64 = (SymGetModuleBase64_t)GetProcAddress(m_dbghelp, functionname);
        if (pSymGetModuleBase64 == NULL) {
            goto getprocaddressfailure;
        }
        functionname = "SymCleanup";
        pSymCleanup = (SymCleanup_t)GetProcAddress(m_dbghelp, functionname);
        if (pSymCleanup == NULL) {
            goto getprocaddressfailure;
        }
        functionname = "SymFromAddr";
        pSymFromAddr = (SymFromAddr_t)GetProcAddress(m_dbghelp, functionname);
        if (pSymFromAddr == NULL) {
            goto getprocaddressfailure;
        }
        functionname = "SymGetLineFromAddr64";
        pSymGetLineFromAddr64 = (SymGetLineFromAddr64_t)GetProcAddress(m_dbghelp, functionname);
        if (pSymGetLineFromAddr64 == NULL) {
            goto getprocaddressfailure;
        }
        functionname = "SymInitialize";
        pSymInitialize = (SymInitialize_t)GetProcAddress(m_dbghelp, functionname);
        if (pSymInitialize == NULL) {
            goto getprocaddressfailure;
        }
        functionname = "SymSetOptions";
        pSymSetOptions = (SymSetOptions_t)GetProcAddress(m_dbghelp, functionname);
        if (pSymSetOptions == NULL) {
            goto getprocaddressfailure;
        }
    }
    else {
        status = false;
        report("ERROR: Visual Leak Detector: Unable to load dbghelp.dll.\n");
    }

    return status;

getprocaddressfailure:
    report("ERROR: Visual Leak Detector: The procedure entry point %s could not be located "
           "in the dynamic link library dbghelp.dll.\n", functionname);
    return false;
}

// report - Sends a printf-style formatted message to the debugger for display.
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
void VisualLeakDetector::report (const char *format, ...)
{
    va_list args;
#define MAXREPORTMESSAGESIZE 513
    char    message [MAXREPORTMESSAGESIZE];

    va_start(args, format);
    _vsnprintf(message, MAXREPORTMESSAGESIZE, format, args);
    va_end(args);
    message[MAXREPORTMESSAGESIZE - 1] = '\0';

    OutputDebugString(message);
}

// reportleaks - Generates a memory leak report when the program terminates if
//   leaks were detected. The report is displayed in the debug output window.
//
//   By default, only "useful" frames are displayed in the Callstack section of
//   each memory block report. By "useful" we mean frames that are not internal
//   to the heap or Visual Leak Detector. However, if the "show useless frames"
//   option is enabled, then all frames will be shown. If the source file
//   information for a frame cannot be found, then the frame will be displayed
//   regardless of the state of the "show useless frames" option (this is
//   because the useless frames are identified by the source file). In most
//   cases, the symbols for the heap internals should be available so this
//   should rarely, if ever, be a problem.
//
//   For each leaked memory block, the Callstack section of the report is
//   followed by a dump of the user-data section of the memory block.
//
//  Return Value:
//
//    None.
//
void VisualLeakDetector::reportleaks ()
{
    CallStack          *callstack;
    DWORD               displacement;
    DWORD64             displacement64;
    unsigned long       duplicates;
    BOOL                foundline;
    unsigned long       frame;
    char               *functionname;
    unsigned long       leaksfound = 0;
    SYMBOL_INFO        *pfunctioninfo;
    _CrtMemBlockHeader *pheader;
    char               *pheap;
    IMAGEHLP_LINE64     sourceinfo;
#define MAXSYMBOLNAMELENGTH 256
#define SYMBOLBUFFERSIZE (sizeof(SYMBOL_INFO) + (MAXSYMBOLNAMELENGTH * sizeof(TCHAR)) - 1)
    unsigned char       symbolbuffer [SYMBOLBUFFERSIZE];
    char               *symbolpath;

    // Initialize structures passed to the symbol handler.
    pfunctioninfo = (SYMBOL_INFO*)symbolbuffer;
    memset(pfunctioninfo, 0x0, SYMBOLBUFFERSIZE);
    pfunctioninfo->SizeOfStruct = sizeof(SYMBOL_INFO);
    pfunctioninfo->MaxNameLen = MAXSYMBOLNAMELENGTH;
    memset(&sourceinfo, 0x0, sizeof(IMAGEHLP_LINE64));
    sourceinfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    // Initialize the symbol handler. We use it for obtaining source file/line
    // number information and function names for the memory leak report.
    symbolpath = buildsymbolsearchpath();
    pSymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
    if (!pSymInitialize(m_process, symbolpath, TRUE)) {
        report("WARNING: Visual Leak Detector: The symbol handler failed to initialize (error=%lu).\n"
               "    Stack traces will probably not be available for leaked blocks.\n", GetLastError());
    }
    delete [] symbolpath;

    // We employ a simple trick here to get a pointer to the first allocated
    // block: just allocate a new block and get the new block's memory header.
    // This works because the most recently allocated block is always placed at
    // the head of the allocated list. We can then walk the list from head to
    // tail. For each block still in the list we do a lookup to see if we have
    // an entry for that block in the allocation block map. If we do, it is a
    // leaked block and the map entry contains the call stack for that block.
#ifdef _MT
    _mlock(_HEAP_LOCK);
#endif // _MT
    pheap = new char;
    pheader = pHdr(pheap)->pBlockHeaderNext;
    delete pheap;
    while (pheader) {
        if (_BLOCK_SUBTYPE(pheader->nBlockUse) == VLDINTERNALBLOCK) {
            // Skip internally allocated blocks.
            pheader = pheader->pBlockHeaderNext;
            continue;
        }
        callstack = m_mallocmap->find(pheader->lRequest);
        if (callstack) {
            // Found a block which is still in the allocated list, and which we
            // have an entry for in the allocated block map. We've identified a
            // memory leak.
            if (leaksfound == 0) {
                report("WARNING: Visual Leak Detector detected memory leaks!\n");
            }
            leaksfound++;
            report("---------- Block %ld at "ADDRESSFORMAT": %u bytes ----------\n", pheader->lRequest, pbData(pheader), pheader->nDataSize);
            if (_VLD_configflags & VLD_CONFIG_AGGREGATE_DUPLICATES) {
                // Aggregate all other leaks which are duplicates of this one
                // under this same heading, to cut down on clutter.
                duplicates = eraseduplicates(pheader->pBlockHeaderNext, pheader->nDataSize, callstack);
                if (duplicates) {
                    report("A total of %lu leaks match this size and call stack. Showing only the first one.\n", duplicates + 1);
                    leaksfound += duplicates;
                }
            }
            report("  Call Stack:\n");

            // Iterate through each frame in the call stack.
            for (frame = 0; frame < callstack->size(); frame++) {
                // Try to get the source file and line number associated with
                // this program counter address.
                if ((foundline = pSymGetLineFromAddr64(m_process, (*callstack)[frame], &displacement, &sourceinfo)) == TRUE) {
                    // Unless the "show useless frames" option has been enabled,
                    // don't show frames that are internal to the heap or Visual
                    // Leak Detector. There is virtually no situation where they
                    // would be useful for finding the source of the leak.
                    if (!(_VLD_configflags & VLD_CONFIG_SHOW_USELESS_FRAMES)) {
                        if (strstr(sourceinfo.FileName, "afxmem.cpp") ||
                            strstr(sourceinfo.FileName, "dbgheap.c") ||
                            strstr(sourceinfo.FileName, "new.cpp") ||
                            strstr(sourceinfo.FileName, "vld.cpp")) {
                            continue;
                        }
                    }
                }

                // Try to get the name of the function containing this program
                // counter address.
                if (pSymFromAddr(m_process, (*callstack)[frame], &displacement64, pfunctioninfo)) {
                    functionname = pfunctioninfo->Name;
                }
                else {
                    functionname = "(Function name unavailable)";
                }

                // Display the current stack frame's information.
                if (foundline) {
                    report("    %s (%d): %s\n", sourceinfo.FileName, sourceinfo.LineNumber, functionname);
                }
                else {
                    report("    "ADDRESSFORMAT" (File and line number not available): ", (*callstack)[frame]);
                    report("%s\n", functionname);
                }
            }

            // Dump the data in the user data section of the memory block.
            if (_VLD_maxdatadump != 0) {
                dumpuserdatablock(pheader);
            }
            report("\n");
        }
        pheader = pheader->pBlockHeaderNext;
    }
#ifdef _MT
    _munlock(_HEAP_LOCK);
#endif // _MT

    // Show a summary.
    if (!leaksfound) {
        report("No memory leaks detected.\n");
    }
    else {
        report("Visual Leak Detector detected %lu memory leak", leaksfound);
        report((leaksfound > 1) ? "s.\n" : ".\n");
    }

    // Free resources used by the symbol handler.
    if (!pSymCleanup(m_process)) {
        report("WARNING: Visual Leak Detector: The symbol handler failed to deallocate resources (error=%lu).\n", GetLastError());
    }
}


////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector APIs - see vld.h for each function's details
//

void VLDEnable ()
{
    if (visualleakdetector.enabled()) {
        // Already enabled for the current thread.
        return;
    }

    // Enable memory leak detection for the current thread.
    TlsSetValue(visualleakdetector.m_tlsindex, (LPVOID)VLD_TLS_ENABLED);
    visualleakdetector.m_status &= ~VLD_STATUS_NEVER_ENABLED;
}

void VLDDisable ()
{
    if (!visualleakdetector.enabled()) {
        // Already disabled for the current thread.
        return;
    }

    // Disable memory leak detection for the current thread.
    TlsSetValue(visualleakdetector.m_tlsindex, (LPVOID)VLD_TLS_DISABLED);
}