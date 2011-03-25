////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - CallStack Class Implementations
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
#define VLDBUILD
#include "callstack.h"  // This class' header.
#include "utility.h"    // Provides various utility functions.
#include "vldheap.h"    // Provides internal new and delete operators.
#include "vldint.h"     // Provides access to VLD internals.

#define MAXSYMBOLNAMELENGTH 256

// Imported global variables.
extern HANDLE             currentprocess;
extern HANDLE             currentthread;
extern CRITICAL_SECTION   stackwalklock;
extern CRITICAL_SECTION   symbollock;

// Constructor - Initializes the CallStack with an initial size of zero and one
//   Chunk of capacity.
//
CallStack::CallStack ()
{
    m_capacity   = CALLSTACKCHUNKSIZE;
    m_size       = 0;
    m_status     = 0x0;
    m_store.next = NULL;
    m_topchunk   = &m_store;
    m_topindex   = 0;
    m_Resolved   = NULL;
}

// Copy Constructor - For efficiency, we want to avoid ever making copies of
//   CallStacks (only pointer passing or reference passing should be performed).
//   The sole purpose of this copy constructor is to ensure that no copying is
//   being done inadvertently.
//
CallStack::CallStack (const CallStack &)
{
    // Don't make copies of CallStacks!
    assert(FALSE);
}

// Destructor - Frees all memory allocated to the CallStack.
//
CallStack::~CallStack ()
{
    CallStack::chunk_t *chunk = m_store.next;
    CallStack::chunk_t *temp;

    while (chunk) {
        temp = chunk;
        chunk = temp->next;
        delete temp;
    }

    if (m_Resolved)
    {
        for (UINT32 frame = 0; frame < m_size; frame++)
        {
            delete m_Resolved[frame];
            m_Resolved[frame] = NULL;
        }
        delete [] m_Resolved;
    }

    m_Resolved = NULL;
}

// operator = - Assignment operator. For efficiency, we want to avoid ever
//   making copies of CallStacks (only pointer passing or reference passing
//   should be performed). The sole purpose of this assignment operator is to
//   ensure that no copying is being done inadvertently.
//
CallStack& CallStack::operator = (const CallStack &)
{
    // Don't make copies of CallStacks!
    assert(FALSE);
    return *this;
}

// operator == - Equality operator. Compares the CallStack to another CallStack
//   for equality. Two CallStacks are equal if they are the same size and if
//   every frame in each is identical to the corresponding frame in the other.
//
//  other (IN) - Reference to the CallStack to compare the current CallStack
//    against for equality.
//
//  Return Value:
//
//    Returns true if the two CallStacks are equal. Otherwise returns false.
//
BOOL CallStack::operator == (const CallStack &other) const
{
    if (m_size != other.m_size) {
        // They can't be equal if the sizes are different.
        return FALSE;
    }

    // Walk the chunk list and within each chunk walk the frames array until we
    // either find a mismatch, or until we reach the end of the call stacks.
    const CallStack::chunk_t *prevchunk = NULL;
    const CallStack::chunk_t *chunk = &m_store;
    const CallStack::chunk_t *otherchunk = &other.m_store;
    while (prevchunk != m_topchunk) {
        for (UINT32 index = 0; index < ((chunk == m_topchunk) ? m_topindex : CALLSTACKCHUNKSIZE); index++) {
            if (chunk->frames[index] != otherchunk->frames[index]) {
                // Found a mismatch. They are not equal.
                return FALSE;
            }
        }
        prevchunk = chunk;
        chunk = chunk->next;
        otherchunk = otherchunk->next;
    }

    // Reached the end of the call stacks. They are equal.
    return TRUE;
}

// operator [] - Random access operator. Retrieves the frame at the specified
//   index.
//
//   Note: We give up a bit of efficiency here, in favor of efficiency of push
//     operations. This is because walking of a CallStack is done infrequently
//     (only if a leak is found), whereas pushing is done very frequently (for
//     each frame in the program's call stack when the program allocates some
//     memory).
//
//  - index (IN): Specifies the index of the frame to retrieve.
//
//  Return Value:
//
//    Returns the program counter for the frame at the specified index. If the
//    specified index is out of range for the CallStack, the return value is
//    undefined.
//
UINT_PTR CallStack::operator [] (UINT32 index) const
{
    UINT32                    chunknumber = index / CALLSTACKCHUNKSIZE;
    const CallStack::chunk_t *chunk = &m_store;

    for (UINT32 count = 0; count < chunknumber; count++) {
        chunk = chunk->next;
    }

    return chunk->frames[index % CALLSTACKCHUNKSIZE];
}

// clear - Resets the CallStack, returning it to a state where no frames have
//   been pushed onto it, readying it for reuse.
//
//   Note: Calling this function does not release any memory allocated to the
//     CallStack. We give up a bit of memory-usage efficiency here in favor of
//     performance of push operations.
//
//  Return Value:
//
//    None.
//
VOID CallStack::clear ()
{
    m_size     = 0;
    m_topchunk = &m_store;
    m_topindex = 0;
}


// dump - Dumps a nicely formatted rendition of the CallStack, including
//   symbolic information (function names and line numbers) if available.
//   Had to get rid of the const qualifier because calling resolve can modify
//   a member variable. This is a result of the combined duties of CallStack::resolve
//   which can both dump or save the formatted stack info. Otherwise there would
//   be nearly identical code in both methods, which is worse than removing a mere 
//   const qualifier

//   Note: The symbol handler must be initialized prior to calling this
//     function.
//
//  - showinternalframes (IN): If true, then all frames in the CallStack will be
//      dumped. Otherwise, frames internal to the heap will not be dumped.
//
//  Return Value:
//
//    None.
//
VOID CallStack::dump (BOOL showinternalframes)
{
    // The stack was dumped already
    if (m_Resolved)
    {
        DumpResolved();
        return;
    }

    resolve(showinternalframes, FALSE);
}

// Formats the stack frame into a human readable format, and saves it for later retrieval.
void CallStack::Resolve(BOOL showinternalframes)
{
    if (m_Resolved != NULL)
    {
        resolve(showinternalframes, TRUE);
    }
}

// Resolve - Creates a nicely formatted rendition of the CallStack, including
//   symbolic information (function names and line numbers) if available. and 
//   saves it for later retrieval. This is almost identical to Callstack::dump above.
//
//   Note: The symbol handler must be initialized prior to calling this
//     function.
//
//  - showinternalframes (IN): If true, then all frames in the CallStack will be
//      dumped. Otherwise, frames internal to the heap will not be dumped.
//
//  - ResolveOnly (IN): If true, it does not print the results out to the standard
//    outputs, but saves the formatted rendition for later retrieval.
//
//  Return Value:
//
//    None.
//
void CallStack::resolve(BOOL showinternalframes, BOOL ResolveOnly)
{
    if (m_status & CALLSTACK_STATUS_INCOMPLETE) {
        // This call stack appears to be incomplete. Using StackWalk64 may be
        // more reliable.
        report(L"    HINT: The following call stack may be incomplete. Setting \"StackWalkMethod\"\n"
            L"      in the vld.ini file to \"safe\" instead of \"fast\" may result in a more\n"
            L"      complete stack trace.\n");
    }

    IMAGEHLP_LINE64  sourceinfo = { 0 };
    sourceinfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    if (ResolveOnly)
    {
        m_Resolved = new WCHAR*[m_size];
        ZeroMemory(m_Resolved, sizeof(WCHAR*) * m_size);
    }

    const UINT32 symbolBufSize = sizeof(SYMBOL_INFO) + (MAXSYMBOLNAMELENGTH * sizeof(WCHAR)) - 1;
    BYTE symbolbuffer [symbolBufSize] = { 0 };
    // Use static here to increase performance, and avoid heap allocs. Hopefully this won't
    // prove to be an issue in thread safety. If it does, it will have to be simply non-static.
    static WCHAR stack_line[MAXREPORTLENGTH + 1] = L"";
    WCHAR undecoratedname [MAXSYMBOLNAMELENGTH];
    WCHAR callingmodulename [MAX_PATH];

    // Iterate through each frame in the call stack.
    for (UINT32 frame = 0; frame < m_size; frame++)
    {
        // Try to get the source file and line number associated with
        // this program counter address.
        SIZE_T programcounter = (*this)[frame];
        EnterCriticalSection(&symbollock);
        BOOL             foundline = FALSE;
        DWORD            displacement = 0;
        foundline = SymGetLineFromAddrW64(currentprocess, programcounter, &displacement, &sourceinfo);
        if (foundline)
        {
            if (!showinternalframes) {
                _wcslwr_s(sourceinfo.FileName, wcslen(sourceinfo.FileName) + 1);
                if (wcsstr(sourceinfo.FileName, L"afxmem.cpp") ||
                    wcsstr(sourceinfo.FileName, L"dbgheap.c") ||
                    wcsstr(sourceinfo.FileName, L"malloc.c") ||
                    wcsstr(sourceinfo.FileName, L"new.cpp") ||
                    wcsstr(sourceinfo.FileName, L"newaop.cpp")) {
                        // Don't show frames in files internal to the heap.
                        LeaveCriticalSection(&symbollock);
                        continue;
                        
                }
            }
        }

        // Initialize structures passed to the symbol handler.
        SYMBOL_INFO* functioninfo = (SYMBOL_INFO*)&symbolbuffer;
        functioninfo->SizeOfStruct = sizeof(SYMBOL_INFO);
        functioninfo->MaxNameLen = MAXSYMBOLNAMELENGTH;

        // Try to get the name of the function containing this program
        // counter address.
        DWORD64          displacement64 = 0;
        LPWSTR           functionname;
        if (SymFromAddrW(currentprocess, programcounter, &displacement64, functioninfo)) {
            // Undecorate function name.
            if (UnDecorateSymbolName(functioninfo->Name, undecoratedname, MAXSYMBOLNAMELENGTH, UNDNAME_NAME_ONLY) > 0)
                functionname = undecoratedname;
            else
                functionname = functioninfo->Name;
        }
        else {
            // GetFormattedMessage( GetLastError() );
            functionname = L"(Function name unavailable)";
            displacement64 = 0;
        }
        LeaveCriticalSection(&symbollock);

        HMODULE hCallingModule = GetCallingModule(programcounter);
        LPWSTR modulename = L"(Module name unavailable)";
        if (hCallingModule && 
            GetModuleFileName(hCallingModule, callingmodulename, sizeof(callingmodulename)) > 0)
        {
            modulename = wcsrchr(callingmodulename, L'\\');
            if (modulename == NULL)
                modulename = wcsrchr(callingmodulename, L'/');
            if (modulename != NULL)
                modulename++;
            else
                modulename = callingmodulename;
        }

        int NumChars = -1;
        // Display the current stack frame's information.
        if (foundline) {
            if (displacement == 0)
                NumChars = _snwprintf_s(stack_line, _TRUNCATE, L"    %s (%d): %s!%s\n", 
                    sourceinfo.FileName, sourceinfo.LineNumber, modulename, functionname);
            else
                NumChars = _snwprintf_s(stack_line, _TRUNCATE, L"    %s (%d): %s!%s + 0x%X bytes\n", 
                    sourceinfo.FileName, sourceinfo.LineNumber, modulename, functionname, displacement);
        }
        else {
            if (displacement64 == 0)
                NumChars = _snwprintf_s(stack_line, _TRUNCATE, L"    " ADDRESSFORMAT L" (File and line number not available): %s!%s\n", 
                    programcounter, modulename, functionname);
            else
                NumChars = _snwprintf_s(stack_line, _TRUNCATE, L"    " ADDRESSFORMAT L" (File and line number not available): %s!%s + 0x%X bytes\n", 
                    programcounter, modulename, functionname, (DWORD)displacement64);	
        }
        if (ResolveOnly) {
            // Just truncate anything that is too long.
            if (NumChars >= 0)
            {
                m_Resolved[frame] = new WCHAR[NumChars + 1];
                wcscpy_s(m_Resolved[frame], NumChars + 1, stack_line);
            }
        }
        else {
            print(stack_line);
        }
    }
}

// DumpResolve
void CallStack::DumpResolved() const
{
    if (m_Resolved)
    {
        for (UINT32 frame = 0; frame < m_size; frame++)
        {
            report(L"%s", m_Resolved[frame]);
        }
    }
}



// getHashValue - Generate callstack hash value.
//
//  Return Value:
//
//    None.
//
DWORD CallStack::getHashValue () const
{
    DWORD       hashcode = 0xD202EF8D;

    // Iterate through each frame in the call stack.
    for (UINT32 frame = 0; frame < m_size; frame++) {
        UINT_PTR programcounter = (*this)[frame];
        hashcode = CalculateCRC32(programcounter, hashcode);
    }
    return hashcode;
}

// push_back - Pushes a frame's program counter onto the CallStack. Pushes are
//   always appended to the back of the chunk list (aka the "top" chunk).
//
//   Note: This function will allocate additional memory as necessary to make
//     room for new program counter addresses.
//
//  - programcounter (IN): The program counter address of the frame to be pushed
//      onto the CallStack.
//
//  Return Value:
//
//    None.
//
VOID CallStack::push_back (const UINT_PTR programcounter)
{
    if (m_size == m_capacity) {
        // At current capacity. Allocate additional storage.
        CallStack::chunk_t *chunk = new CallStack::chunk_t;
        chunk->next = NULL;
        m_topchunk->next = chunk;
        m_topchunk = chunk;
        m_topindex = 0;
        m_capacity += CALLSTACKCHUNKSIZE;
    }
    else if (m_topindex == CALLSTACKCHUNKSIZE) {
        // There is more capacity, but not in this chunk. Go to the next chunk.
        // Note that this only happens if this CallStack has previously been
        // cleared (clearing resets the data, but doesn't give up any allocated
        // space).
        m_topchunk = m_topchunk->next;
        m_topindex = 0;
    }

    m_topchunk->frames[m_topindex++] = programcounter;
    m_size++;
}

// getstacktrace - Traces the stack as far back as possible, or until 'maxdepth'
//   frames have been traced. Populates the CallStack with one entry for each
//   stack frame traced.
//
//   Note: This function uses a very efficient method to walk the stack from
//     frame to frame, so it is quite fast. However, unconventional stack frames
//     (such as those created when frame pointer omission optimization is used)
//     will not be successfully walked by this function and will cause the
//     stack trace to terminate prematurely.
//
//  - maxdepth (IN): Maximum number of frames to trace back.
//
//  - framepointer (IN): Frame (base) pointer at which to begin the stack trace.
//      If NULL, then the stack trace will begin at this function.
//
//  Return Value:
//
//    None.
//
VOID FastCallStack::getstacktrace (UINT32 maxdepth, context_t& context)
{
    UINT32  count = 0;
    UINT_PTR* framepointer = context.fp;

#if defined(_M_IX86)
    while (count < maxdepth) {
        if (*framepointer < (UINT)framepointer) {
            if (*framepointer == NULL) {
                // Looks like we reached the end of the stack.
                break;
            }
            else {
                // Invalid frame pointer. Frame pointer addresses should always
                // increase as we move up the stack.
                m_status |= CALLSTACK_STATUS_INCOMPLETE;
                break;
            }
        }
        if (*framepointer & (sizeof(UINT_PTR*) - 1)) {
            // Invalid frame pointer. Frame pointer addresses should always
            // be aligned to the size of a pointer. This probably means that
            // we've encountered a frame that was created by a module built with
            // frame pointer omission (FPO) optimization turned on.
            m_status |= CALLSTACK_STATUS_INCOMPLETE;
            break;
        }
        if (IsBadReadPtr((UINT*)*framepointer, sizeof(UINT_PTR*))) {
            // Bogus frame pointer. Again, this probably means that we've
            // encountered a frame built with FPO optimization.
            m_status |= CALLSTACK_STATUS_INCOMPLETE;
            break;
        }
        count++;
        push_back(*(framepointer + 1));
        framepointer = (UINT_PTR*)*framepointer;
    }
#elif defined(_M_X64)
    UINT32 maxframes = min(62, maxdepth + 10);
    static USHORT (WINAPI *s_pfnCaptureStackBackTrace)(ULONG FramesToSkip, ULONG FramesToCapture, PVOID* BackTrace, PULONG BackTraceHash) = 0;  
    if (s_pfnCaptureStackBackTrace == 0)  
    {  
        const HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");  
        reinterpret_cast<void*&>(s_pfnCaptureStackBackTrace)
            = ::GetProcAddress(hNtDll, "RtlCaptureStackBackTrace");
        if (s_pfnCaptureStackBackTrace == 0)  
            return;
    }
    UINT_PTR* myFrames = new UINT_PTR[maxframes];
    ZeroMemory(myFrames, sizeof(UINT_PTR) * maxframes);
    s_pfnCaptureStackBackTrace(0, maxframes, (PVOID*)myFrames, NULL);
    UINT32  startIndex = 0;
    while (count < maxframes) {
        if (myFrames[count] == 0)
            break;
        if (myFrames[count] == *(framepointer + 1))
            startIndex = count;
        count++;
    }
    count = startIndex;
    while (count < maxframes) {
        if (myFrames[count] == 0)
            break;
        push_back(myFrames[count]);
        count++;
    }
    delete [] myFrames;
#endif
}

// getstacktrace - Traces the stack as far back as possible, or until 'maxdepth'
//   frames have been traced. Populates the CallStack with one entry for each
//   stack frame traced.
//
//   Note: This function uses a documented Windows API to walk the stack. This
//     API is supposed to be the most reliable way to walk the stack. It claims
//     to be able to walk stack frames that do not follow the conventional stack
//     frame layout. However, this robustness comes at a cost: it is *extremely*
//     slow compared to walking frames by following frame (base) pointers.
//
//  - maxdepth (IN): Maximum number of frames to trace back.
//
//  - framepointer (IN): Frame (base) pointer at which to begin the stack trace.
//      If NULL, then the stack trace will begin at this function.
//
//  Return Value:
//
//    None.
//
VOID SafeCallStack::getstacktrace (UINT32 maxdepth, context_t& context)
{
    UINT_PTR* framepointer = context.fp;
    DWORD   architecture   = X86X64ARCHITECTURE;
    CONTEXT currentcontext;
    memset(&currentcontext, 0, sizeof(currentcontext));

    // Get the required values for initialization of the STACKFRAME64 structure
    // to be passed to StackWalk64(). Required fields are AddrPC and AddrFrame.
#if defined(_M_IX86)
    UINT_PTR programcounter = *(framepointer + 1);
    UINT_PTR stackpointer   = (*framepointer) - maxdepth * 10 * sizeof(void*);  // An approximation.
    currentcontext.SPREG  = stackpointer;
    currentcontext.BPREG  = (DWORD64)framepointer;
    currentcontext.IPREG  = programcounter;
#elif defined(_M_X64)
    currentcontext.SPREG  = context.Rsp;
    currentcontext.BPREG  = (DWORD64)framepointer;
    currentcontext.IPREG  = context.Rip;
#else
    // If you want to retarget Visual Leak Detector to another processor
    // architecture then you'll need to provide architecture-specific code to
    // obtain the program counter and stack pointer from the given frame pointer.
#error "Visual Leak Detector is not supported on this architecture."
#endif // _M_IX86 || _M_X64

    // Initialize the STACKFRAME64 structure.
    STACKFRAME64 frame;
    memset(&frame, 0x0, sizeof(frame));
    frame.AddrPC.Offset       = currentcontext.IPREG;
    frame.AddrPC.Mode         = AddrModeFlat;
    frame.AddrStack.Offset    = currentcontext.SPREG;
    frame.AddrStack.Mode      = AddrModeFlat;
    frame.AddrFrame.Offset    = currentcontext.BPREG;
    frame.AddrFrame.Mode      = AddrModeFlat;
    frame.Virtual             = TRUE;

    // Walk the stack.
    EnterCriticalSection(&stackwalklock);
    UINT32 count = 0;
    while (count < maxdepth) {
        count++;
        if (!StackWalk64(architecture, currentprocess, currentthread, &frame, &currentcontext, NULL,
            SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
                // Couldn't trace back through any more frames.
                break;
        }
        if (frame.AddrFrame.Offset == 0) {
            // End of stack.
            break;
        }

        // Push this frame's program counter onto the CallStack.
        push_back((UINT_PTR)frame.AddrPC.Offset);
    }
    LeaveCriticalSection(&stackwalklock);
}
