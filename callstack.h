////////////////////////////////////////////////////////////////////////////////
//  $Id: callstack.h,v 1.2 2006/01/17 01:28:54 dmouldin Exp $
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

#include <cassert>
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
    CallStack (const CallStack &source);
    ~CallStack ();

    // Public APIs - see each function definition for details.
    void clear ();
    void dump (bool showuselessframes) const;
    void getstacktrace (SIZE_T maxdepth);
    bool operator == (const CallStack &other) const;
    DWORD_PTR operator [] (unsigned long index) const;
    void push_back (const DWORD_PTR programcounter);
    unsigned long size () const;

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