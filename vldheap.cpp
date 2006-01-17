////////////////////////////////////////////////////////////////////////////////
//  $Id: vldheap.cpp,v 1.2 2006/01/17 23:16:53 dmouldin Exp $
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

#include <cassert>
#define VLDBUILD     // Declares that we are building Visual Leak Detector.
#include "vldheap.h" // Provides access to VLD's internal heap datastructures.
#undef new           // VLD's new operator does not apply in this file

// Global variales.
vldblockheader_t *vldblocklist = NULL; // List of internally allocated blocks on VLD's private heap.
HANDLE            vldheap;             // VLD's private heap.

// Local helper functions.
static inline void vlddelete (void *block);
static inline void* vldnew (unsigned int size, const char *file, int line);

// scalar delete operator - Delete operator used to free memory used internally by
//   VLD to VLD's private heap.
//
//  - block (IN): Pointer to the scalar memory block to free.
//
//  Return Value:
//
//    None.
//
void operator delete (void *block)
{
    vlddelete(block);
}

// vector delete operator - Delete operator used to free memory used internally
//   by VLD to VLD's private heap.
//
//  - block (IN): Pointer to the vector memory block to free.
//
//  Return Value:
//
//    None.
//
void operator delete [] (void *block)
{
    vlddelete(block);
}

// scalar delete operator - Delete operator used to free memory partially
//   allocated by new in the event that the corresponding new operator throws
//   an exception.
//
//  Note: This version of the delete operator should never be called directly.
//    The compiler automatically generates calls to this function as needed.
//
void operator delete (void *block, const char *file, int line)
{
    vlddelete(block);
}

// vector delete operator - Delete operator used to free memory partially
//   allocated by new in the event that the corresponding new operator throws
//   an exception.
//
//  Note: This version of the delete operator should never be called directly.
//    The compiler automatically generates calls to this function as needed.
//
void operator delete [] (void *block, const char *file, int line)
{
    vlddelete(block);
}

// scalar new operator - New operator used to allocate a scalar memory block
//   from VLD's private heap.
//
//  - size (IN): Size of the memory block to be allocated.
//
//  - file (IN): The name of the file from which this function is being
//      called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    If the allocation succeeds, a pointer to the allocated memory block is
//    returned. If the allocation fails, NULL is returned.
//
void* operator new (unsigned int size, const char *file, int line)
{
    return vldnew(size, file, line);
}

// vector new operator - New operator used to allocate a vector memory block
//   from VLD's private heap.
//
//  - size (IN): Size of the memory block to be allocated.
//
//  - file (IN): The name of the file from which this function is being
//      called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    If the allocation succeeds, a pointer to the allocated memory block is
//    returned. If the allocation fails, NULL is returned.
//
void* operator new [] (unsigned int size, const char *file, int line)
{
    return vldnew(size, file, line);
}

// vlddelete - Local helper function that actually frees memory back to VLD's
//   private heap.
//
//  - block (IN): Pointer to a memory block being freed.
//
//  Return Value:
//
//    None.
//
static void vlddelete (void *block)
{
    vldblockheader_t *header = BLOCKHEADER(block);

    // Unlink the block from the block list.
    if (header->prev) {
        header->prev->next = header->next;
    }
    else {
        vldblocklist = header->next;
    }

    if (header->next) {
        header->next->prev = header->prev;
    }

    // Free the block.
    assert(HeapFree(vldheap, 0x0, header));
}

// vldnew - Local helper function that actually allocates memory from VLD's
//   private heap. Appends a header to the returned block used for bookeeping
//   information that allows VLD to detect and report internal memory leaks.
//
//  - size (IN): Size of the memory block to be allocated.
//
//  - file (IN): Name of the file that called the new operator.
//
//  - line (IN): Line, in the above file, at which the new operator was called.
//
//  Return Value:
//
//    If the memory allocation succeeds, a pointer to the allocated memory
//    block is returned. If the allocation fails, NULL is returned.
//
static void* vldnew (unsigned int size, const char *file, int line)
{
    vldblockheader_t *header = (vldblockheader_t*)HeapAlloc(vldheap, 0x0, size + sizeof(vldblockheader_t));
    static SIZE_T     serialnumber = 0;

    if (header != NULL) {
        // Fill in the block's header information.
        header->file         = file;
        header->line         = line;
        header->serialnumber = serialnumber++;
        header->size         = size;

        // Link the block into the block list.
        header->next         = vldblocklist;
        if (header->next != NULL) {
            header->next->prev = header;
        }
        header->prev         = NULL;
        vldblocklist         = header;

        // Return a pointer to the beginning of the data-portion of the block.
        return BLOCKDATA(header);
    }

    return NULL;
}