////////////////////////////////////////////////////////////////////////////////
//  $Id: vldutil.cpp,v 1.5 2005/04/17 13:29:15 db Exp $
//
//  Visual Leak Detector (Version 0.9f)
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

#define VLDBUILD     // Declare that we are building Visual Leak Detector
#include "vldutil.h"

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
void strapp (char **dest, char *source)
{
    size_t  length;
    char   *temp;

    temp = *dest;
    length = strlen(*dest) + strlen(source);
    *dest = new char [length + 1];
    strncpy(*dest, temp, length);
    strncat(*dest, source, length);
    delete [] temp;
}

// Constructor - Initializes the BlockMap with an initial size of zero and one
//   Chunk of capacity.
//
BlockMap::BlockMap ()
{
    unsigned long index;

    m_maphead        = NULL;
    m_maptail        = NULL;
    m_size           = 0;
    m_storehead.next = NULL;
    m_storetail      = &m_storehead;

    // Link together the initial free list.
    for (index = 0; index < BLOCKMAPCHUNKSIZE - 1; index++) {
        m_storehead.pairs[index].next = &m_storehead.pairs[index + 1];
    }
    m_storehead.pairs[index].next = NULL;
    m_free = m_storehead.pairs;
}

// Copy Constructor - For efficiency, we want to avoid ever making copies of
//   BlockMaps (only pointer passing or reference passing should be performed).
//   The sole purpose of this copy constructor is to ensure that no copying is
//   being done inadvertently.
BlockMap::BlockMap (const BlockMap &source)
{
    // Don't make copies of BlockMaps!
    assert(false);
}

// Destructor - Frees all memory allocated to the BlockMap.
BlockMap::~BlockMap ()
{
    BlockMap::Chunk *chunk;
    BlockMap::Chunk *temp;

    chunk = m_storehead.next;
    while (chunk) {
        temp = chunk;
        chunk = chunk->next;
        delete temp;
    }
}

// XXX This really needs to be replaced by a better search algorithm. A
// XXX balanced binary tree (e.g. a red-black tree) would probably be best
// XXX suited to the task.
//
// _find - Performs a linear search of the map to find the specified request
//   number.
//
//  - request (IN): Specifies the memory allocation request number of the Pair
//      to find.
//
//  Return Value:
//
//    Returns a pointer to the Pair with the specified request number. If no
//    matching Pair is found, returns NULL.
//
BlockMap::Pair* BlockMap::_find (unsigned long request)
{
    BlockMap::Pair *cur = m_maphead;

    while (cur) {
        if (cur->request == request) {
            return cur;
        }
        cur = cur->next;
    }

    return NULL;
}

// erase - Erases the Pair with the specified allocation request number.
//
//  - request (IN): Specifies the request number of the Pair to erase. If no
//      existing Pair matches this request number, nothing happens.
//
//  Return Value:
//
//    None.
//
void BlockMap::erase (unsigned long request)
{
    BlockMap::Pair *pair;

    // Search for an exact match. If an exact match is not found, do nothing.
    pair = _find(request);
    if (pair) {
        // An exact match was found. Remove the matching Pair from the map.
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

        // Reset the removed Pair.
        pair->callstack.clear();

        // Put the removed Pair onto the free list.
        pair->next = m_free;
        m_free = pair;

        m_size--;
    }
}

// find - Searches for and retrieves the CallStack matching the specified
//   request number.
//
//  - request (IN): Specifies the request number of the Pair to search for.
//
//  Return Value:
//
//    Returns a pointer to the matching CallStack. If no matching CallStack is
//    found, returns NULL.
//
CallStack* BlockMap::find (unsigned long request)
{
    BlockMap::Pair *pair;
    
    // Search for an exact match.
    pair = _find(request);
    if (pair) {
        // Found an exact match.
        return &pair->callstack;
    }
    else {
        // No exact match found.
        return NULL;
    }
}

// XXX This really needs to be replaced by a sorted-insertion algorithm. A
// XXX balanced binary tree (e.g. a red-black tree) would probably be best
// XXX suited to the task.
//
// insert - Inserts a new Pair into the map. make_pair() should be used to
//   obtain a pointer to the Pair to be inserted.
//
//  - pair (IN): Pointer to the Pair to be inserted. Call make_pair() to obtain
//      a pointer to a new Pair to be used for this parameter.
//
//  Return Value:
//
//    None.
//
void BlockMap::insert (BlockMap::Pair *pair)
{
    // Insert at the end of the map.
    if (m_maptail) {
        m_maptail->next = pair;
    }
    else {
        m_maphead = pair;
    }
    pair->prev = m_maptail;
    pair->next = NULL;
    m_maptail = pair;

    m_size++;
}

// make_pair - Obtains a free Pair that can be populated and inserted into the
//   BlockMap and initializes it with the specified request number.
//
//  - request (IN): Request number to use to initialize the new Pair.
//
//  Return Value:
//
//    Returns a pointer to the new Pair. This pointer can then be used to insert
//    the Pair into the BlockMap via insert().
//
BlockMap::Pair* BlockMap::make_pair (unsigned long request)
{
    BlockMap::Chunk *chunk;
    unsigned long    index;
    BlockMap::Pair  *pair;

    if (m_free == NULL) {
        // No more free Pairs. Allocate additional storage.
        chunk = new BlockMap::Chunk;
        chunk->next = NULL;
        m_storetail->next = chunk;
        m_storetail = chunk;

        // Link the newly allocated storage to the free list.
        for (index = 0; index < BLOCKMAPCHUNKSIZE - 1; index++) {
            chunk->pairs[index].next = &chunk->pairs[index + 1];
        }
        chunk->pairs[index].next = NULL;
        m_free = chunk->pairs;
    }

    // Obtain a Pair from the free list.
    pair = m_free;
    m_free = m_free->next;

    // Initialize the Pair with the supplied request number.
    pair->request = request;

    return pair;
}

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
    CallStack::Chunk *chunk;
    CallStack::Chunk *temp;

    chunk = m_store.next;
    while (chunk) {
        temp = chunk;
        chunk = temp->next;
        delete temp;
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
    CallStack::Chunk *chunk = &m_store;
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
    m_topchunk = &m_store;
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
unsigned long CallStack::size ()
{
    return m_size;
}
