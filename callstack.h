////////////////////////////////////////////////////////////////////////////////
//  $Id: callstack.h,v 1.4 2006/02/23 22:01:00 dmouldin Exp $
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

#define CALLSTACKCHUNKSIZE 32

////////////////////////////////////////////////////////////////////////////////
//
//  The CallStack Class
//
//    CallStack objects can be used for obtaining, storing, and displaying the
//    call stack at a given point during program execution.
//
//    The primary data structure used by the CallStack is similar in concept to
//    a STL vector, but is specifically tailored for use by VLD, making it more
//    efficient than a standard STL vector.
//
//    Inside the CallStack are a number of "Chunks" which are arranged in a
//    linked list. Each Chunk contains an array of frames (each frame is
//    represented by a program counter address). If we run out of space when
//    pushing new frames onto an existing chunk in the CallStack Chunk list,
//    then a new Chunk is allocated and appended to the end of the list. In this
//    way, the CallStack can grow dynamically as needed. New frames are always
//    pushed onto the Chunk at the end of the list known as the "top" Chunk.
//
class CallStack
{
public:
    CallStack ();
    CallStack (const CallStack &other);
    ~CallStack ();

    // Public APIs - see each function definition for details.
    VOID clear ();
    VOID dump (BOOL showinternalframes) const;
    virtual VOID getstacktrace (UINT32 maxdepth, SIZE_T *framepointer) = 0;
    CallStack& operator = (const CallStack &other);
    BOOL operator == (const CallStack &other) const;
    SIZE_T operator [] (UINT32 index) const;
    VOID push_back (const SIZE_T programcounter);

protected:
    // Protected Data
    UINT32 m_status;                    // Status flags:
#define CALLSTACK_STATUS_INCOMPLETE 0x1 //   If set, the stack trace stored in this CallStack appears to be incomplete.

private:
    // The chunk list is made of a linked list of Chunks.
    typedef struct chunk_s {
        struct chunk_s *next;
        SIZE_T          frames [CALLSTACKCHUNKSIZE];
    } chunk_t;

    // Private Data
    UINT32              m_capacity; // Current capacity limit (in frames)
    UINT32              m_size;     // Current size (in frames)
    CallStack::chunk_t  m_store;    // Pointer to the underlying data store (i.e. head of the Chunk list)
    CallStack::chunk_t *m_topchunk; // Pointer to the Chunk at the top of the stack
    UINT32              m_topindex; // Index, within the top Chunk, of the top of the stack
};


////////////////////////////////////////////////////////////////////////////////
//
//  The FastCallStack Class
//
//    This class is a specialization of the CallStack class which provides a
//    very fast stack tracing function.
//
class FastCallStack : public CallStack
{
public:
    VOID getstacktrace (UINT32 maxdepth, SIZE_T *framepointer);
};

////////////////////////////////////////////////////////////////////////////////
//
//  The SafeCallStack Class
//
//    This class is a specialization of the CallStack class which provides a
//    more robust, but quite slow, stack tracing function.
//
class SafeCallStack : public CallStack
{
public:
    VOID getstacktrace (UINT32 maxdepth, SIZE_T *framepointer);
};