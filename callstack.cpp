////////////////////////////////////////////////////////////////////////////////
//  $Id: callstack.cpp,v 1.2 2006/01/16 03:51:10 db Exp $
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

#include <windows.h>
#define __out_xcount(x) // Workaround for the specstrings.h bug in the Platform SDK.
#include <dbghelp.h>    // Provides symbol handling services.

#define VLDBUILD
#include "callstack.h"  // This class' header.
#include "utility.h"    // Provides various utility functions.
#include "vldheap.h"    // Provides internal new and delete operators.

#define MAXSYMBOLNAMELENGTH 256

// Imported global variables.
extern HANDLE currentprocess;
extern HANDLE currentthread;

// Constructor - Initializes the CallStack with an initial size of zero and one
//   Chunk of capacity.
//
CallStack::CallStack ()
{
    m_capacity   = CALLSTACKCHUNKSIZE;
    m_size       = 0;
    m_store.next = NULL;
    m_topchunk   = &m_store;
    m_topindex   = 0;
}

// Copy Constructor - For efficiency, we want to avoid ever making copies of
//   CallStacks (only pointer passing or reference passing should be performed).
//   The sole purpose of this copy constructor is to ensure that no copying is
//   being done inadvertently.
//
CallStack::CallStack (const CallStack &source)
{
    // Don't make copies of CallStacks!
    assert(false);
}

// Destructor - Frees all memory allocated to the CallStack.
//
CallStack::~CallStack ()
{
    CallStack::Chunk *chunk = m_store.next;
    CallStack::Chunk *temp;

    while (chunk) {
        temp = chunk;
        chunk = temp->next;
        delete temp;
    }
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
bool CallStack::operator == (const CallStack &other) const
{
    const CallStack::Chunk *chunk = &m_store;
    unsigned long           index;
    const CallStack::Chunk *otherchunk = &other.m_store;
    const CallStack::Chunk *prevchunk = NULL;

    if (m_size != other.m_size) {
        // They can't be equal if the sizes are different.
        return false;
    }

    // Walk the chunk list and within each chunk walk the frames array until we
    // either find a mismatch, or until we reach the end of the call stacks.
    while (prevchunk != m_topchunk) {
        for (index = 0; index < ((chunk == m_topchunk) ? m_topindex : CALLSTACKCHUNKSIZE); index++) {
            if (chunk->frames[index] != otherchunk->frames[index]) {
                // Found a mismatch. They are not equal.
                return false;
            }
        }
        prevchunk = chunk;
        chunk = chunk->next;
        otherchunk = otherchunk->next;
    }

    // Reached the end of the call stacks. They are equal.
    return true;
}

// operator [] - Random access operator. Retrieves the frame at the specified
//   index.
//
//  Note: We give up a bit of efficiency here, in favor of efficiency of push
//   operations. This is because walking of a CallStack is done infrequently
//   (only if a leak is found), whereas pushing is done very frequently (for
//   each frame in the program's call stack when the program allocates some
//   memory).
//
//  - index (IN): Specifies the index of the frame to retrieve.
//
//  Return Value:
//
//    Returns the program counter for the frame at the specified index. If the
//    specified index is out of range for the CallStack, the return value is
//    undefined.
//
DWORD_PTR CallStack::operator [] (unsigned long index) const
{
    unsigned long           count;
    const CallStack::Chunk *chunk = &m_store;
    unsigned long           chunknumber = index / CALLSTACKCHUNKSIZE;

    for (count = 0; count < chunknumber; count++) {
        chunk = chunk->next;
    }

    return chunk->frames[index % CALLSTACKCHUNKSIZE];
}

// clear - Resets the CallStack, returning it to a state where no frames have
//   been pushed onto it, readying it for reuse.
//
//  Note: Calling this function does not release any memory allocated to the
//   CallStack. We give up a bit of memory-usage efficiency here in favor of
//   performance of push operations.
//
//  Return Value:
//
//    None.
//
void CallStack::clear ()
{
    m_size     = 0;
    m_topchunk = &m_store;
    m_topindex = 0;
}

// dump - Dumps a nicely formatted rendition of the CallStack, including
//   symbolic information (function names and line numbers) if available.
//
//  - showuselessframes (IN): If true, then all frames in the CallStack will be
//      dumped. Otherwise, frames internal to the heap or Visual Leak Detector
//      will not be included in the dump.
//
//  Return Value:
//
//    None.
//
void CallStack::dump (bool showuselessframes) const
{
    DWORD            displacement;
    DWORD64          displacement64;
    BOOL             foundline;
    unsigned long    frame;
    SYMBOL_INFO     *functioninfo;
    char            *functionname;
    IMAGEHLP_LINE64  sourceinfo = { 0 };
    unsigned char    symbolbuffer [sizeof(SYMBOL_INFO) + (MAXSYMBOLNAMELENGTH * sizeof(TCHAR)) - 1] = { 0 };

    // Initialize structures passed to the symbol handler.
    functioninfo = (SYMBOL_INFO*)&symbolbuffer;
    functioninfo->SizeOfStruct = sizeof(SYMBOL_INFO);
    functioninfo->MaxNameLen = MAXSYMBOLNAMELENGTH;
    sourceinfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    // Iterate through each frame in the call stack.
    for (frame = 0; frame < m_size; frame++) {
        // Try to get the source file and line number associated with
        // this program counter address.
        if ((foundline = SymGetLineFromAddr64(currentprocess, (*this)[frame], &displacement, &sourceinfo)) == TRUE) {
            // Unless the "show useless frames" option has been enabled,
            // don't show frames that are internal to the heap or Visual
            // Leak Detector.
            if (!(showuselessframes)) {
                _strlwr(sourceinfo.FileName);
                if (strstr(sourceinfo.FileName, "afxmem.cpp") ||
                    strstr(sourceinfo.FileName, "callstack.cpp") ||
                    strstr(sourceinfo.FileName, "dbgheap.c") ||
                    strstr(sourceinfo.FileName, "malloc.c") ||
                    strstr(sourceinfo.FileName, "new.cpp") ||
                    strstr(sourceinfo.FileName, "vld.cpp")) {
                    continue;
                }
            }
        }

        // Try to get the name of the function containing this program
        // counter address.
        if (SymFromAddr(currentprocess, (*this)[frame], &displacement64, functioninfo)) {
            functionname = functioninfo->Name;
        }
        else {
            functionname = "(Function name unavailable)";
        }

        // Display the current stack frame's information.
        if (foundline) {
            report("    %s (%d): %s\n", sourceinfo.FileName, sourceinfo.LineNumber, functionname);
        }
        else {
            report("    "ADDRESSFORMAT" (File and line number not available): ", (*this)[frame]);
            report("%s\n", functionname);
        }
    }
}

// getstacktrace - Traces the stack, starting from this function, as far
//   back as possible. Populates the current CallStack with one entry for each
//   stack frame traced. Requires architecture-specific code for retrieving
//   the current frame pointer and program counter.
//
//  Note: This function assumes that the current CallStack is already empty.
//   Calling this function on a CallStack that has already been populate with
//   a stack trace will result in the new stack trace being appended to the
//   existing stack trace.
//
//  - maxdepth (IN): Specifies the maximum depth of the stack trace. The trace
//      will stop when this number of frames has been trace, or when the end of
//      the stack is reached, whichever happens first.
//
//  Return Value:
//
//    None.
//
void CallStack::getstacktrace (SIZE_T maxdepth)
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
    while (count < maxdepth) {
        count++;
        if (!StackWalk64(architecture, currentprocess, currentthread, &frame, &context,
                         NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
            // Couldn't trace back through any more frames.
            break;
        }
        if (frame.AddrFrame.Offset == 0) {
            // End of stack.
            break;
        }

        // Push this frame's program counter onto the CallStack.
        push_back((DWORD_PTR)frame.AddrPC.Offset);
    }
}

// push_back - Pushes a frame's program counter onto the CallStack. Pushes are
//   always appended to the back of the chunk list (aka the "top" chunk).
//
//  Note: This function will allocate additional memory as necessary to make
//    room for new program counter addresses.
//
//  - programcounter (IN): The program counter address of the frame to be pushed
//      onto the CallStack.
//
//  Return Value:
//
//    None.
//
void CallStack::push_back (const DWORD_PTR programcounter)
{
    CallStack::Chunk *chunk;

    if (m_size == m_capacity) {
        // At current capacity. Allocate additional storage.
        chunk = new CallStack::Chunk;
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

// size - Retrieves the current size of the CallStack. Size should not be
//   confused with capacity. Capacity is an internal parameter that indicates
//   the total reserved capacity of the CallStack, which includes any free
//   space already allocated. Size represents only the number of frames that
//   have been pushed onto the CallStack.
//
//  Return Value:
//
//    Returns the number of frames currently stored in the CallStack.
//
unsigned long CallStack::size () const
{
    return m_size;
}
