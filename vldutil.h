////////////////////////////////////////////////////////////////////////////////
//  $Id: vldutil.h,v 1.1.2.1 2005/04/08 13:05:45 db Exp $
//
//  Visual Leak Detector (Version 0.9d)
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

#ifndef _VLDUTIL_H_
#define _VLDUTIL_H_

#ifndef VLDBUILD
#error "This header should only be included by Visual Leak Detector when building it from source. Applications should NEVER include this header."
#endif

#include <cassert>

#include <windows.h>
#define _CRTBLD
#include <dbgint.h>
#undef _CRTBLD

#define BLOCKMAPCHUNKSIZE   64
#define CALLSTACKCHUNKSIZE  32
#define VLDREQUESTNUMBER   -2

// operator new - Visual Leak Detector's internal new operator. Only VLD uses
//   this operator (note the static linkage). Applications linking with VLD will
//   still use the "standard" new operator.
//
//   This special new operator assigns each of VLD's allocations a reserved
//   allocation request number, which gives VLD the ability to detect leaks
//   within itself. Call stacks for internal leaks will not be available, since
//   attempting to collect that data would cause an infinite loop within VLD.
//   But this is better than no internal leak detection at all.
//
//  - size (AUTO): The size of the memory block to allocate. Note that the C++
//      language automatically calculates and passes this parameter to new. So
//      when using the new operator, this parameter is never explicitly passed
//      as in a traditional function call, but is derived from the data-type
//      given as an operand to the operator.
//
//  Return Value:
//
//    Returns a pointer to the beginning of the newly allocated object's memory.
//
inline static void* operator new (unsigned int size)
{
    void *pdata = malloc(size);

    //pHdr(pdata)->lRequest = VLDREQUESTNUMBER;

    return pdata;
}

////////////////////////////////////////////////////////////////////////////////
//
//  The CallStack Class
//
//    This data structure is similar in concept to an STL vector, but is
//    specifically tailored for use by VLD, making it more efficient than a
//    standard STL vector.
//
//    A CallStack is made up of a number of "chunks" which are arranged in a
//    linked list. Each chunk contains an array of frames (each frame is
//    represented by a program counter address). If we run out of space when
//    pushing new frames onto an existing chunk in the CallStack chunk list,
//    then a new chunk is allocated and appended to the end of the list. In this
//    way, the CallStack can grow dynamically as needed. New frames are always
//    pushed onto the chunk at the end of the list known as the "top" chunk.
//
//    When a CallStack is no longer needed (i.e. the memory block associated
//    with a CallStack has been freed) the memory allocated to the call stack is
//    not actually freed. Instead, the CallStack's data is simply reset.so that
//    the CallStack can be reused later without needing to reallocate memory.
//
class CallStack
{
    // The chunk list is made of a linked list of stackchunk_s structures
    class StackChunk {
    private:
        StackChunk *next;
        DWORD64     frames [CALLSTACKCHUNKSIZE];

        friend class CallStack;
    };

public:
    CallStack ();
    CallStack (const CallStack &source);
    ~CallStack ();

    // Public APIs - see each function definition for details
    DWORD64 operator [] (unsigned long index);

    void clear ();
    void push_back (DWORD64 programcounter);
    unsigned long size ();

private:
    // Private Data
    unsigned long  m_capacity;
    unsigned long  m_size;
    StackChunk    *m_store;
    StackChunk    *m_topchunk;
    unsigned long  m_topindex;
};

class BlockMap
{
public:
    class Pair {
    public:
        CallStack* getcallstack () { return &callstack; }

    private:
        Pair          *next;
        Pair          *prev;
        CallStack      callstack;
        unsigned long  request;

        friend class BlockMap;
    };

    class MapChunk {
    private:
        MapChunk *next;
        Pair      pairs [BLOCKMAPCHUNKSIZE];

        friend class BlockMap;
    };

    BlockMap ();
    BlockMap (const BlockMap &source);
    ~BlockMap ();

    void erase (unsigned long request);
    CallStack* find (unsigned long request);
    void insert (Pair *pair);
    Pair* make_pair (unsigned long request);

private:
    Pair* _find (unsigned long request, bool exact);
    void _free (Pair *pair);
    void _unlink (Pair *pair);

    Pair          *m_free;
    Pair          *m_maphead;
    Pair          *m_maptail;
    unsigned long  m_size;
    MapChunk      *m_storehead;
    MapChunk      *m_storetail;
};

#endif // _VLDUTIL_H_