////////////////////////////////////////////////////////////////////////////////
//  $Id: vldutil.cpp,v 1.1.2.2 2005/04/11 12:51:58 db Exp $
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

#include <cassert>

#define VLDBUILD     // Declare that we are building Visual Leak Detector
#include "vldutil.h"

////////////////////////////////////////////////////////////////////////////////
//
//  The BlockMap Class
//
////////////////////////////////////////////////////////////////////////////////

BlockMap::BlockMap ()
{
    m_free      = NULL;
    m_maphead   = NULL;
    m_maptail   = NULL;
    m_size      = 0;
    m_storehead = NULL;
    m_storetail = NULL;
}

BlockMap::BlockMap (const BlockMap &source)
{
    assert(false);
}

BlockMap::~BlockMap ()
{
    BlockMap::Chunk *chunk;

    while (m_storehead) {
        chunk = m_storehead;
        m_storehead = m_storehead->next;
        delete chunk;
    }
}

BlockMap::Pair* BlockMap::_find (unsigned long request, bool exact)
{
    unsigned long   count;
    BlockMap::Pair *high = m_maptail;
    unsigned long   highindex = m_size - 1;
    BlockMap::Pair *low = m_maphead;
    unsigned long   lowindex = 0;
    BlockMap::Pair *mid;
    unsigned long   midcount;
    unsigned long   midindex;

    while (low != high) {
        mid = low;
        midindex = lowindex + ((highindex - lowindex) / 2);
        midcount = midindex - lowindex;
        for (count = 0; count < midcount; count++) {
            mid = mid->next;
        }
        if (mid->request == request) {
            // Found an exact match.
            return mid;
        }
        else if (mid->request > request) {
            if (low->request > request) {
                // If an exact search, then the request number we're looking for
                // is not in the list. If an inexact search, then insert at the
                // head of the list.
                return NULL;
            }
            high = mid->prev;
            highindex = midindex - 1;
        }
        else {
            if (high->request < request) {
                if (exact) {
                    // The request number we're looking for is not in the list.
                    return NULL;
                }
                // Insert at the end of the list.
                return high;
            }
            low = mid->next;
            lowindex = midindex + 1;
        }
    }
    
    if (low && (low->request == request)) {
        // Found an exact match.
        return low;
    }
    else {
        if (exact) {
            // The request number we're looking for is not in the list.
            return NULL;
        }
        // Insert after the nearest existing lower request number.
        return low;
    }
}

void BlockMap::_free (BlockMap::Pair *pair)
{
    pair->next = m_free;
    m_free = pair;
}

void BlockMap::_unlink (BlockMap::Pair *pair)
{
    if (pair->prev) {
        pair->prev->next = pair->next;
    }
    else {
        m_maphead = pair->next;
    }

    if (pair->next) {
        pair->next->prev = pair->prev;
    }
    else {
        m_maptail = pair->prev;
    }
}

void BlockMap::erase (unsigned long request)
{
    BlockMap::Pair *pair = _find(request, true);

    if (pair) {
        _unlink(pair);
        pair->callstack.clear();
        _free(pair);
        m_size--;
    }
}

CallStack* BlockMap::find (unsigned long request)
{
    BlockMap::Pair *pair = _find(request, true);

    if (pair) {
        return &pair->callstack;
    }
    else {
        return NULL;
    }
}

void BlockMap::insert (BlockMap::Pair *pair)
{
    BlockMap::Pair *insertionpoint;

    insertionpoint = _find(pair->request, false);
    if (insertionpoint && (insertionpoint->request == pair->request)) {
        // The request number we are inserting already exists in the map.
        // Replace the existing pair with the new pair.
        if (insertionpoint->prev) {
            insertionpoint->prev->next = pair;
        }
        else {
            m_maphead = pair;
        }
        pair->prev = insertionpoint->prev;
        if (insertionpoint->next) {
            insertionpoint->next->prev = pair;
        }
        else {
            m_maptail = pair;
        }
        pair->next = insertionpoint->next;
        _free(insertionpoint);
        return;
    }

    if (insertionpoint == NULL) {
        // Insert at the beginning of the map.
        if (m_maphead) {
            m_maphead->prev = pair;
        }
        else {
            // Inserting into an empty list. Initialize the tail.
            m_maptail = pair;
        }
        pair->next = m_maphead;
        pair->prev = NULL;
        m_maphead = pair;
    }
    else {
        // Insert after the insertion point.
        if (insertionpoint->next) {
            // Inserting somewhere in the middle.
            insertionpoint->next->prev = pair;
        }
        else {
            // Inserting at the tail.
            m_maptail = pair;
        }
        pair->next = insertionpoint->next;
        pair->prev = insertionpoint;
        insertionpoint->next = pair;
    }
    m_size++;
}

BlockMap::Pair* BlockMap::make_pair (unsigned long request)
{
    BlockMap::Chunk *chunk;
    unsigned long    index;
    BlockMap::Pair  *pair;

    // Obtain a pair from the free list.
    if (m_free == NULL) {
        // No more free pairs. Allocate additional storage.
        chunk = new BlockMap::Chunk;
        chunk->next = NULL;
        if (m_storehead) {
            m_storetail->next = chunk;
            m_storetail = chunk;
        }
        else {
            m_storehead = chunk;
            m_storetail = chunk;
        }
        // Link the newly allocated storage to the free list.
        for (index = 0; index < BLOCKMAPCHUNKSIZE - 1; index++) {
            chunk->pairs[index].next = &chunk->pairs[index + 1];
        }
        chunk->pairs[index].next = NULL;
        m_free = chunk->pairs;
    }
    pair = m_free;
    m_free = m_free->next;

    pair->request = request;

    return pair;
}


////////////////////////////////////////////////////////////////////////////////
//
//  The CallStack Class
//
////////////////////////////////////////////////////////////////////////////////

// Constructor - Initializes the CallStack with zero capacity. No memory is
//   allocated until a frame is actually pushed onto the CallStack.
//
CallStack::CallStack ()
{
    m_capacity = 0;
    m_size     = 0;
    m_store    = NULL;
    m_topchunk = NULL;
    m_topindex = 0;
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

// Destructor - Frees all memory allocated to a CallStack.
//
CallStack::~CallStack ()
{
    CallStack::Chunk *chunk;

    while (m_store) {
        chunk = m_store;
        m_store = m_store->next;
        delete chunk;
    }
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
DWORD64 CallStack::operator [] (unsigned long index)
{
    unsigned long     count;
    CallStack::Chunk *chunk = m_store;
    unsigned long     chunknumber = index / CALLSTACKCHUNKSIZE;

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
    m_topchunk = m_store;
    m_topindex = 0;
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
void CallStack::push_back (DWORD64 programcounter)
{
    CallStack::Chunk *chunk;

    if (m_size == m_capacity) {
        // At current capacity. Allocate additional storage.
        chunk = new CallStack::Chunk;
        chunk->next = NULL;
        if (m_store) {
            m_topchunk->next = chunk;
            m_topchunk = chunk;
        }
        else {
            m_store = chunk;
            m_topchunk = chunk;
        }
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
unsigned long CallStack::size ()
{
    return m_size;
}
