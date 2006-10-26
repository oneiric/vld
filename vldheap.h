////////////////////////////////////////////////////////////////////////////////
//  $Id: vldheap.h,v 1.6 2006/10/26 23:30:09 dmouldin Exp $
//
//  Visual Leak Detector (Version 1.9b) - Internal C++ Heap Management Defs.
//  Copyright (c) 2006 Dan Moulding
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

#pragma once

#ifndef VLDBUILD
#error \
"This header should only be included by Visual Leak Detector when building it from source. \
Applications should never include this header."
#endif

#include <windows.h>

// Memory block header structure used internally by VLD. All internally
// allocated blocks are allocated from VLD's private heap and have this header
// prepended to them.
typedef struct vldblockheader_s
{
    const char              *file;         // Name of the file where this block was allocated.
    int                      line;         // Line number within the above file where this block was allocated.
    struct vldblockheader_s *next;         // Pointer to the next block in the list of internally allocated blocks.
    struct vldblockheader_s *prev;         // Pointer to the preceding block in the list of internally allocated blocks.
    SIZE_T                   serialnumber; // Each block is assigned a unique serial number, starting from zero.
    unsigned int             size;         // The size of this memory block, not including this header.
} vldblockheader_t;

// Data-to-Header and Header-to-Data conversion
#define BLOCKHEADER(d) (vldblockheader_t*)(((PBYTE)d) - sizeof(vldblockheader_t))
#define BLOCKDATA(h) (LPVOID)(((PBYTE)h) + sizeof(vldblockheader_t))

// new and delete operators for allocating from VLD's private heap.
void operator delete (void *block);
void operator delete [] (void *block);
void operator delete (void *block, const char *file, int line);
void operator delete [] (void *block, const char *file, int line);
void* operator new (unsigned int size, const char *file, int line);
void* operator new [] (unsigned int size, const char *file, int line);

// All calls to the new operator from within VLD are mapped to the version of
// new that allocates from VLD's private heap.
#define new new(__FILE__, __LINE__)
