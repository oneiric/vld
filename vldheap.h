////////////////////////////////////////////////////////////////////////////////
//  $Id: vldheap.h,v 1.3 2006/01/27 23:07:23 dmouldin Exp $
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

#pragma once

#ifndef VLDBUILD
#error "This header should only be included by Visual Leak Detector when building it from source. Applications should never include this header."
#endif

#include <windows.h>

// Memory block header structure used internally by VLD. All internally
// allocated blocks are allocated from VLD's private heap and have this header
// prepended to them.
typedef struct vldblockheader_s
{
    const char              *file;
    int                      line;
    struct vldblockheader_s *next;
    struct vldblockheader_s *prev;
    SIZE_T                   serialnumber;
    unsigned int             size;
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