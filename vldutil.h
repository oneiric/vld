////////////////////////////////////////////////////////////////////////////////
//  $Id: vldutil.h,v 1.12 2005/07/21 21:58:10 dmouldin Exp $
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

#ifndef _VLDUTIL_H_
#define _VLDUTIL_H_

#ifndef VLDBUILD
#error "This header should only be included by Visual Leak Detector when building it from source. Applications should never include this header."
#endif

// Standard headers
#include <cassert>

// Microsoft-specific headers
#include <windows.h> // Required by dbgint.h
#define _CRTBLD
#include <dbgint.h>  // Provides access to the heap internals, specifically the memory block header structure.
#undef _CRTBLD

#ifdef _WIN64
#define ADDRESSFORMAT      "0x%.16X" // Format string for 64-bit addresses
#else
#define ADDRESSFORMAT      "0x%.8X"  // Format string for 32-bit addresses
#endif // _WIN64
#define BLOCKMAPCHUNKSIZE  64        // Size, in Pairs, of each BlockMap Chunk
#define CALLSTACKCHUNKSIZE 32        // Size, in frames (DWORDs), of each CallStack Chunk.
#define VLDINTERNALBLOCK   0xbf42    // VLD internal memory block subtype

// operator new,
// operator new [] - Visual Leak Detector's internal new operators. Only VLD
//   uses these operators (note the static linkage). Applications linking with
//   VLD will still use the "standard" new operator.
//
//   These special new operators assign a VLD-specific "use" type to each
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
//    Both versions return a pointer to the beginning of the newly allocated
//    object's memory.
//
inline static void* operator new (unsigned int size, char *file, int line)
{
    void *pdata = _malloc_dbg(size, _CRT_BLOCK | (VLDINTERNALBLOCK << 16), file, line);

    return pdata;
}

inline static void* operator new[] (unsigned int size, char *file, int line)
{
    void *pdata = _malloc_dbg(size, _CRT_BLOCK | (VLDINTERNALBLOCK << 16), file, line);

    return pdata;
}

// Map new to the internal new operators.
#define new new(__FILE__, __LINE__)

// operator delete,
// operator delete [] - Matching delete operators for VLD's internal new
//   operators. These exist solely to satisfy the compiler. Unless there is an
//   exception while constructing an object allocated using VLD's new operators,
//   these versions of delete never get used.
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

inline static void operator delete[] (void *p, char *file, int line)
{
    _CrtMemBlockHeader *pheader = pHdr(p);

    _free_dbg(p, pheader->nBlockUse);
}

// operator delete,
// operator delete [] - Visual Leak Detector's internal delete operators. Only
//   VLD uses these operators (note the static linkage). Applications linking
//   with VLD will still use the "standard" delete operators.
//
//   These special delete operators ensure that the proper "use" type is passed
//   to the underlying heap functions to avoid asserts that will occur if the
//   type passed in (which for the default delete operators is _NORMAL_BLOCK)
//   does not match the block's type (which for VLD's internal blocks is
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

inline static void operator delete[] (void *p)
{
    _CrtMemBlockHeader *pheader = pHdr(p);

    _free_dbg(p, pheader->nBlockUse);
}

// strapp - Appends to strings. See function definition for details.
void strapp (char **dest, char *source);


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
public:
    CallStack ();
    CallStack (const CallStack &source);
    ~CallStack ();

    // Public APIs - see each function definition for details.
    DWORD_PTR operator [] (unsigned long index);

    void clear ();
    void push_back (DWORD_PTR programcounter);
    unsigned long size ();

private:
    // The chunk list is made of a linked list of Chunks.
    class Chunk {
    private:
        Chunk () {}
        Chunk (const Chunk &source) { assert(false); } // Do not make copies of Chunks!

        Chunk     *next;
        DWORD_PTR  frames [CALLSTACKCHUNKSIZE];

        friend class CallStack;
    };

    // Private Data
    unsigned long     m_capacity; // Current capacity limit (in frames)
    unsigned long     m_size;     // Current size (in frames)
    CallStack::Chunk  m_store;    // Pointer to the underlying data store (i.e. head of the Chunk list)
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
//  The purpose of the BlockMap is to map allocated memory blocks (via their
//  unique allocation request numbers) to the call stacks that allocated them.
//  One of the primary concerns of the BlockMap is to be able to quickly insert
//  search and delete. For this reason, the underlying data structure is
//  a red-black tree (a type of balanced binary tree).
//
//  The red-black tree is overlayed on top of larger "chunks" of pre-allocated
//  storage. These chunks, which are arranged in a linked list, make it possible
//  for the map to have reserve capacity, allowing it to grow dynamically
//  without incurring a heap hit each time a new element is added to the map.
//
class BlockMap
{
    enum color_e { black, red };

public:
    BlockMap ();
    BlockMap (const BlockMap &source);
    ~BlockMap ();

    // Public APIs - see each function definition for details.
    void erase (unsigned long request);
    CallStack* find (unsigned long request);
    CallStack* insert (unsigned long request);

private:
    class Chunk;

    // StackLinks allow the BlockMap to maintain a linked list of CallStacks for
    // its internal free list.
    class StackLink {
    private:
        StackLink *next;
        CallStack  stack;

        friend class BlockMap;
    };

    // Nodes form the basis of the red-black tree.
    class Node {
    private:
        Node () {}
        Node (const Node &node) { assert(false); } // Do not make copies of Nodes!

        color_e        color;
        StackLink     *data;
        unsigned long  key;
        Node          *left;
        Node          *parent;
        Node          *right;

        friend class BlockMap;
        friend class Chunk;
    };

    // The chunk list is made of a linked list of Chunks.
    class Chunk {
    private:
        Chunk () {}
        Chunk (const Chunk &source) { assert(false); } // Do not make copies of Chunks!

        Chunk     *next;
        Node       nodes [BLOCKMAPCHUNKSIZE];
        StackLink  stacks [BLOCKMAPCHUNKSIZE];

        friend class BlockMap;
    };

    // Private Helper Functions -- see each function for details
    inline void _rotateleft (BlockMap::Node *node);
    inline void _rotateright (BlockMap::Node *node);

    // Private Data
    BlockMap::Node      *m_freenodes;  // Pointer to the head of the free node list
    BlockMap::StackLink *m_freestacks; // Pointer to the head of the free stack list
    BlockMap::Node      *m_max;        // Pointer to the maximum node in the tree
    BlockMap::Node       m_nil;        // The nil node - all leaf nodes point to this, it is always colored black
    BlockMap::Node      *m_root;       // Pointer to the root of the red-black tree
    BlockMap::Chunk      m_store;      // The underlying data store (grows as needed)
    BlockMap::Chunk     *m_storetail;  // Pointer to the end of the underlying data store (i.e. tail of the Chunk list)
};

#endif // _VLDUTIL_H_
