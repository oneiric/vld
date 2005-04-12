////////////////////////////////////////////////////////////////////////////////
//  $Id: vldutil.h,v 1.1.2.3 2005/04/12 03:05:34 dmouldin Exp $
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
#error "This header should only be included by Visual Leak Detector when building it from source. Applications should never include this header."
#endif

// Standard headers
#define _CRTBLD
#include <cassert>

// Microsoft-specific headers
#include <windows.h> // Required by dbgint.h
#define _CRTBLD
#include <dbgint.h>  // Provides access to the heap internals, specifically the memory block header structure.
#undef _CRTBLD

#define BLOCKMAPCHUNKSIZE   64     // Size, in Pairs, of each BlockMap Chunk
#define CALLSTACKCHUNKSIZE  32     // Size, in frames (DWORDs), of each CallStack Chunk.
#define VLDINTERNALBLOCK    0xbf42 // VLD internal memory block subtype

////////////////////////////////////////////////////////////////////////////////
//
//  Utility Functions
//

// operator new - Visual Leak Detector's internal new operator. Only VLD uses
//   this operator (note the static linkage). Applications linking with VLD will
//   still use the "standard" new operator.
//
//   This special new operator assigns a VLD-specific "use" type to each
//   allocated block. VLD uses this type information to identify blocks
//   belonging to VLD. Ultimately this is used to detect memory leaks internal
//   to VLD.
//
//  - size (AUTO): The size of the memory block to allocate. Note that the C++
//      language automatically calculates and passes this parameter to new. So
//      when using the new operator, this parameter is never explicitly passed
//      as in a traditional function call, but is derived from the data-type
//      given as an operand to the operator.
//
//  - file (IN): String containing the name of the file in which new was called.
//      Can be used to locate the source of any internal leaks in lieu of stack
//      traces.
//
//  - line (IN): Line number at which new was called. Can be used to locate the
//      source of any internal leaks in lieu of stack traces.
//
//  Return Value:
//
//    Returns a pointer to the beginning of the newly allocated object's memory.
//
inline static void* operator new (unsigned int size, char *file, int line)
{
    void *pdata = _malloc_dbg(size, _CLIENT_BLOCK | (VLDINTERNALBLOCK << 16), file, line);

    return pdata;
}

// Map new to the internal new operator.
#define new new(__FILE__, __LINE__)

// operator delete - Matching delete operator for VLD's internal new operator.
//   This exists solely to satisfy the compiler. Unless there is an exception
//   while constructing an object allocated using VLD's new operator, this
//   version of delete never gets used.
//
//  - p (IN): Pointer to the user-data section of the memory block to be freed.
//
//  - file (IN): Placeholder. Ignored.
//
//  - line (IN): Placeholder. Ignored.
//
//  Return Value:
//
//    None.
//
inline static void operator delete (void *p, char *file, int line)
{
    _CrtMemBlockHeader *pheader = pHdr(p);

    _free_dbg(p, pheader->nBlockUse);
}

// operator delete - Visual Leak Detector's internal delete operator. Only VLD
//   uses this operator (note the static linkage). Applications linking with VLD
//   will still use the "standard" delete operator.
//
//   This special delete operator ensures that the proper "use" type is passed
//   to the underlying heap functions to avoid asserts that will occur if the
//   type passed in (which for the default delete opeator is _NORMAL_BLOCK) does
//   not match the block's type (which for VLD's internal blocks is
//   _CLIENT_BLOCK bitwise-OR'd with a VLD-specific subtype identifier).
//
//  - p (IN): Pointer to the user-data section of the memory block to be freed.
//
//  Return Value:
//
//    None.
//
inline static void operator delete (void *p)
{
    _CrtMemBlockHeader *pheader = pHdr(p);

    _free_dbg(p, pheader->nBlockUse);
}

void strapp (char **dest, char *source);


////////////////////////////////////////////////////////////////////////////////
//
//  Utility Classes
//

////////////////////////////////////////////////////////////////////////////////
//
//  The CallStack Class
//
//    This data structure is similar in concept to a STL vector, but is
//    specifically tailored for use by VLD, making it more efficient than a
//    standard STL vector.
//
//    A CallStack is made up of a number of "Chunks" which are arranged in a
//    linked list. Each Chunk contains an array of frames (each frame is
//    represented by a program counter address). If we run out of space when
//    pushing new frames onto an existing chunk in the CallStack Chunk list,
//    then a new Chunk is allocated and appended to the end of the list. In this
//    way, the CallStack can grow dynamically as needed. New frames are always
//    pushed onto the Chunk at the end of the list known as the "top" Chunk.
//
//    When a CallStack is no longer needed (i.e. the memory block associated
//    with a CallStack has been freed) the memory allocated to the CallStack is
//    not actually freed. Instead, the CallStack's data is simply reset so that
//    the CallStack can be reused later without needing to reallocate memory.
//
class CallStack
{
    // The chunk list is made of a linked list of Chunks.
    class Chunk {
        Chunk () {}
        Chunk (const Chunk &source) { assert(false); } // Do not make copies of Chunks!

    private:
        Chunk   *next;
        DWORD64  frames [CALLSTACKCHUNKSIZE];

        friend class CallStack;
    };

public:
    CallStack ();
    CallStack (const CallStack &source);
    ~CallStack ();

    // Public APIs - see each function definition for details.
    DWORD64 operator [] (unsigned long index);

    void clear ();
    void push_back (DWORD64 programcounter);
    unsigned long size ();

private:
    // Private Data
    unsigned long     m_capacity; // Current capacity limit (in frames)
    unsigned long     m_size;     // Current size (in frames)
    CallStack::Chunk *m_store;    // Pointer to the underlying data store (i.e. head of the Chunk list)
    CallStack::Chunk *m_topchunk; // Pointer to the Chunk at the top of the stack
    unsigned long     m_topindex; // Index, within the top Chunk, of the top of the stack
};

////////////////////////////////////////////////////////////////////////////////
//
//  The BlockMap Class
//
//  This data structure is similar in concept to a STL map, but is specifically
//  tailored for use by VLD, making it more efficient than a standard STL map.
//
//  A BlockMap is made of a number of "Chunks" which are arranged in a linked
//  list. Each Chunk contains an array of smaller linked list elements called
//  "Pairs". The Pairs are linked in sorted order according to each Pair's
//  index value, which is a memory block allocation request number. Links
//  between Pairs can cross Chunk boundaries. In this way, the linked list
//  of Pairs grows dynamically, but also has "reserved" space which reduces the
//  number of heap allocations performed.
//
//  Each pair contains a request number (the index or "key" value) and a
//  CallStack. Whenever a Pair is removed from the linked list, memory allocated
//  to the Pair's CallStack is not actually freed. Instead the CallStack's data
//  is simply reset and the Pair is placed on the BlockMap's "free" list. Then
//  if a new Pair is needed, before resorting to allocating a new Pair from the
//  heap, existing Pairs (with some reserved CallStack space already available
//  within them) are obtained from the free list. This cuts down on the number
//  of heap allocations performed when inserting into the BlockMap. In fact, for
//  most applications, new heap allocations internal to VLD should eventually
//  taper off to nothing as the application's memory usage peaks, even if the
//  application continues to free and reallocate memory from the heap.
//
class BlockMap
{
private:
    class Chunk;

public:
    // The map is made of a linked list of Pairs.
    class Pair {
    public:
        Pair (const Pair &source) { assert(false); } // Do not make copies of Pairs!

        CallStack* getcallstack () { return &callstack; }

    private:
        Pair () {} // No public constructor - use make_pair().

        Pair          *next;
        Pair          *prev;
        CallStack      callstack;
        unsigned long  request;

        friend class BlockMap;
        friend class Chunk;
    };

    BlockMap ();
    BlockMap (const BlockMap &source);
    ~BlockMap ();

    // Public APIs - see each function definition for details.
    void erase (unsigned long request);
    CallStack* find (unsigned long request);
    void insert (BlockMap::Pair *pair);
    BlockMap::Pair* make_pair (unsigned long request);

private:
    // Space for the linked list of Pairs is reserved in yet another linked
    // list, this time a linked list of Chunks.
    class Chunk {
        Chunk () {}
        Chunk (const Chunk &source) { assert(false); } // Do not make copies of Chunks!

    private:
        Chunk *next;
        Pair   pairs [BLOCKMAPCHUNKSIZE];

        friend class BlockMap;
    };

    // Private Helper Functions - see each function definition for details.
    inline BlockMap::Pair* _find (unsigned long request, bool exact);

    // Private Data
    BlockMap::Pair  *m_free;      // Pointer to the head of the free list
    BlockMap::Pair  *m_maphead;   // Pointer to the head of the map
    BlockMap::Pair  *m_maptail;   // Pointer to the tail of the map
    unsigned long    m_size;      // Current size of the map (in pairs)
    BlockMap::Chunk *m_storehead; // Pointer to the start of the underlying data store (i.e. head of the Chunk list)
    BlockMap::Chunk *m_storetail; // Pointer to the end of the underlying data store (i.e. tail of the Chunk list)
};

#endif // _VLDUTIL_H_