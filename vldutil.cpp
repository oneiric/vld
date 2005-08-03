////////////////////////////////////////////////////////////////////////////////
//  $Id: vldutil.cpp,v 1.12.2.1 2005/08/03 23:16:31 dmouldin Exp $
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

    m_max        = &m_nil;
    m_nil.color  = black;
    m_nil.data   = NULL;
    m_nil.key    = 0;
    m_nil.left   = &m_nil;
    m_nil.parent = NULL;
    m_nil.right  = &m_nil;
    m_root       = &m_nil;
    m_store.next = NULL;
    m_storetail  = &m_store;

    // Link together the initial free lists.
    for (index = 0; index < BLOCKMAPCHUNKSIZE - 1; index++) {
        m_store.nodes[index].parent = &m_store.nodes[index + 1];
        m_store.stacks[index].next = &m_store.stacks[index + 1];
    }
    m_store.nodes[index].parent = NULL;
    m_store.stacks[index].next = NULL;
    m_freenodes = m_store.nodes;
    m_freestacks = m_store.stacks;
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
    BlockMap::Chunk *chunk = m_store.next;
    BlockMap::Chunk *temp;

    while (chunk) {
        temp = chunk;
        chunk = chunk->next;
        delete temp;
    }
}

// _rotateleft: Rotates a pair of nodes counter-clockwise so that the parent
//   node becomes the left child and the right child becomes the parent.
//
//  - node (IN): Pointer to the node to rotate about (the parent of the pair).
//
//  Return Value:
//
//    None.
//
void BlockMap::_rotateleft (BlockMap::Node *node)
{
    BlockMap::Node *child = node->right;

    // Reassign the child's left subtree to the parent.
    node->right = child->left;
    if (child->left != &m_nil) {
        child->left->parent = node;
    }

    // Reassign the child/parent relationship.
    child->parent = node->parent;
    if (node->parent == &m_nil) {
        // The child becomes the new root node.
        m_root = child;
    }
    else {
        // Point the grandparent at the child.
        if (node == node->parent->left) {
            node->parent->left = child;
        }
        else {
            node->parent->right = child;
        }
    }
    child->left = node;
    node->parent = child;
}

// _rotateright - Rotates a pair of nodes clockwise so that the parent node
//   becomes the right child and the left child becomes the parent.
//
//  - node (IN): Pointer to the node to rotate about (the parent of the pair).
//
//  Return Value:
//
//    None.
//
void BlockMap::_rotateright (BlockMap::Node *node)
{
    BlockMap::Node *child = node->left;

    // Reassign the child's right subtree to the parent.
    node->left = child->right;
    if (child->right != &m_nil) {
        child->right->parent = node;
    }

    // Reassign the child/parent relationship.
    child->parent = node->parent;
    if (node->parent == &m_nil) {
        // The child becomes the new root node.
        m_root = child;
    }
    else {
        // Point the grandparent at the child.
        if (node == node->parent->left) {
            node->parent->left = child;
        }
        else {
            node->parent->right = child;
        }
    }
    child->right = node;
    node->parent = child;
}

// erase - Erases the block with the specified allocation request number from
//   the BlockMap.
//
//  - request (IN): The memory block allocation request number of the block to
//      erase from the map.
//
//  Return Value:
//
//    None.
//
void BlockMap::erase (unsigned long request)
{
    BlockMap::Node      *child;
    BlockMap::Node      *cur;
    BlockMap::Node      *erasure;
    BlockMap::Node      *node = m_root;
    BlockMap::Node      *sibling;
    BlockMap::StackLink *stacklink;

    // Find the node to delete.
    while (node != &m_nil) {
        if (node->key == request) {
            break;
        }
        else if (node->key > request) {
            // Go left.
            node = node->left;
        }
        else {
            // Go right.
            node = node->right;
        }
    }
    if (node == &m_nil) {
        // Node not found. Do nothing.
        return;
    }

    if ((node->left == &m_nil) || (node->right == &m_nil)) {
        // The node to be erased has less than two children. It can be directly
        // deleted from the tree.
        erasure = node;
        if (node == m_max) {
            // The maximum node is being deleted. Find the new maximum.
            if (m_max->left == &m_nil) {
                m_max = node->parent;
            }
            else {
                m_max = m_max->left;
                while (m_max->right != &m_nil) {
                    m_max = m_max->right;
                }
            }
        }
    }
    else {
        // The node to be erased has two children. It can only be deleted
        // indirectly. The actual node will stay where it is, but it's contents
        // will be replaced by it's in-order successor's contents. The successor
        // node will then be deleted. Find the successor.
        erasure = node->right;
        while (erasure->left != &m_nil) {
            erasure = erasure->left;
        }
    }

    // Select the child node which will replace the node to be deleted.
    if (erasure->left != &m_nil) {
        child = erasure->left;
    }
    else {
        child = erasure->right;
    }

    // Replace the node to be deleted with the selected child.
    child->parent = erasure->parent;
    if (child->parent == &m_nil) {
        // The root of the tree is being deleted. The child becomes root.
        m_root = child;
    }
    else {
        if (erasure == erasure->parent->left) {
            erasure->parent->left = child;
        }
        else {
            erasure->parent->right = child;
        }
    }

    if (erasure != node) {
        // The node being deleted from the tree is the successor of the actual
        // node to be erased. Replace the contents of the node to be erased
        // with the successor's contents.
        stacklink = node->data;

        node->data = erasure->data;
        node->key  = erasure->key;
        if (erasure == m_max) {
            // The maximum node has been transplanted.
            m_max = node;
        }
    }
    else {
        stacklink = erasure->data;
    }

    if (erasure->color == black) {
        // The node being deleted from the tree is black. Restructuring of the
        // tree may be needed so that black-height is maintained.
        cur = child;
        while ((cur != m_root) && (cur->color == black)) {
            if (cur == cur->parent->left) {
                // Current node is a left child.
                sibling = cur->parent->right;
                if (sibling->color == red) {
                    // Sibling is red. Rotate sibling up and color it black.
                    sibling->color = black;
                    cur->parent->color = red;
                    _rotateleft(cur->parent);
                    sibling = cur->parent->right;
                }
                if ((sibling->left->color == black) && (sibling->right->color == black)) {
                    // Both of sibling's children are black. Color sibling red.
                    sibling->color = red;
                    cur = cur->parent;
                }
                else {
                    // At least one of sibling's children is red.
                    if (sibling->right->color == black) {
                        sibling->left->color = black;
                        sibling->color = red;
                        _rotateright(sibling);
                        sibling = cur->parent->right;
                    }
                    sibling->color = cur->parent->color;
                    cur->parent->color = black;
                    sibling->right->color = black;
                    _rotateleft(cur->parent);
                    cur = m_root;
                }
            }
            else {
                // Current node is a right child.
                sibling = cur->parent->left;
                if (sibling->color == red) {
                    // Sibling is red. Rotate sibling up and color it black.
                    sibling->color = black;
                    cur->parent->color = red;
                    _rotateright(cur->parent);
                    sibling = cur->parent->left;
                }
                if ((sibling->left->color == black) && (sibling->right->color == black)) {
                    // Both of sibling's children are black. Color sibling red.
                    sibling->color = red;
                    cur = cur->parent;
                }
                else {
                    // At least one of sibling's children is red.
                    if (sibling->left->color == black) {
                        sibling->right->color = black;
                        sibling->color = red;
                        _rotateleft(sibling);
                        sibling = cur->parent->left;
                    }
                    sibling->color = cur->parent->color;
                    cur->parent->color = black;
                    sibling->left->color = black;
                    _rotateright(cur->parent);
                    cur = m_root;
                }
            }
        }
        cur->color = black;
    }

    // Put the node deleted from the tree onto the free list.
    erasure->parent = m_freenodes;
    m_freenodes = erasure;

    // Put the deleted stack onto the free list.
    stacklink->stack.clear();
    stacklink->next = m_freestacks;
    m_freestacks = stacklink;
}

// find - Retrieves the CallStack associated with the specified allocation 
//   request number.
//
//  - request (IN): Memory block allocation request number of the specific block
//      for which the CallStack should be retrieved.
//
//  Return Value:
//
//    None.
//
CallStack* BlockMap::find (unsigned long request)
{
    BlockMap::Node *cur = m_root;

    while (cur != &m_nil) {
        if (cur->key == request) {
            // Found a match.
            return &cur->data->stack;
        }
        else if (cur->key > request) {
            // Go left.
            cur = cur->left;
        }
        else {
            // Go right.
            cur = cur->right;
        }
    }

    // Node not found.
    return NULL;
}

// insert - Inserts an allocation request number into the map. Once the request
//   number has been inserted, the returned CallStack pointer can be used to
//   access the CallStack associated with that request number.
//
//  - request (IN): Memory block allocation request number to be inserted into
//      the map.
//
//  Return Value:
//
//    Returns a pointer to the CallStack associated with the allocation request
//    number inserted into the map. This pointer can be used to populate the
//    CallStack which will initially be empty.
//
CallStack* BlockMap::insert (unsigned long request)
{
    BlockMap::Chunk     *chunk;
    BlockMap::Node      *cur;
    unsigned long        index;
    BlockMap::Node      *node;
    BlockMap::Node      *parent;
    BlockMap::StackLink *stacklink;
    BlockMap::Node      *uncle;

    // Find the location where the new node should be inserted. Because memory
    // block allocation request numbers always increase, new nodes will almost
    // always go to the far right of the tree. To squeeze out a bit of extra
    // efficiency, first compare with the maximum node.
    if (request > m_max->key) {
        parent = m_max;
    }
    else {
        parent = &m_nil;
        cur = m_root;
        while (cur != &m_nil) {
            parent = cur;
            if (cur->key == request) {
                // Overwrite the existing CallStack.
                cur->data->stack.clear();
                return &cur->data->stack;
            }
            else if (cur->key > request) {
                // Go left.
                cur = cur->left;
            }
            else {
                // Go right.
                cur = cur->right;
            }
        }
    }

    // Obtain a new node and new stack from the free list.
    if ((m_freenodes == NULL) || (m_freestacks == NULL)) {
        // Allocate additional storage.
        chunk = new BlockMap::Chunk;
        chunk->next = NULL;
        for (index = 0; index < BLOCKMAPCHUNKSIZE - 1; index++) {
            chunk->nodes[index].parent = &chunk->nodes[index + 1];
            chunk->stacks[index].next = &chunk->stacks[index + 1];
        }
        chunk->nodes[index].parent = NULL;
        chunk->stacks[index].next = NULL;
        m_freenodes = chunk->nodes;
        m_freestacks = chunk->stacks;
        m_storetail->next = chunk;
        m_storetail = chunk;
    }
    node = m_freenodes;
    stacklink = m_freestacks;
    m_freenodes = m_freenodes->parent;
    m_freestacks = m_freestacks->next;

    // Initialize the new node and insert it.
    node->color  = red;
    node->data   = stacklink;
    node->key    = request;
    node->left   = &m_nil;
    node->parent = parent;
    node->right  = &m_nil;
    if (parent == &m_nil) {
        // The tree is empty. The new node becomes root.
        m_root = node;
        m_max = node;
    }
    else {
        if (parent->key > request) {
            // New node is a left child.
            parent->left = node;
        }
        else {
            // New node is a right child.
            parent->right = node;
            if (parent == m_max) {
                m_max = node;
            }
        }
    }

    // Rebalance and/or adjust the tree, if necessary.
    cur = node;
    while (cur->parent->color == red) {
        // Double-red violation. Rebalancing/adjustment needed.
        if (cur->parent == cur->parent->parent->left) {
            // Parent is the left child. Uncle is the right child.
            uncle = cur->parent->parent->right;
            if (uncle->color == red) {
                // Uncle is red. Recolor.
                cur->parent->parent->color = red;
                cur->parent->color = black;
                uncle->color = black;
                cur = cur->parent->parent;
            }
            else {
                // Uncle is black. Restructure.
                if (cur == cur->parent->right) {
                    cur = cur->parent;
                    _rotateleft(cur);
                }
                cur->parent->color = black;
                cur->parent->parent->color = red;
                _rotateright(cur->parent->parent);
            }
        }
        else {
            // Parent is the right child. Uncle is the left child.
            uncle = cur->parent->parent->left;
            if (uncle->color == red) {
                // Uncle is red. Recolor.
                cur->parent->parent->color = red;
                cur->parent->color = black;
                uncle->color = black;
                cur = cur->parent->parent;
            }
            else {
                // Uncle is black. Restructure.
                if (cur == cur->parent->left) {
                    cur = cur->parent;
                    _rotateright(cur);
                }
                cur->parent->color = black;
                cur->parent->parent->color = red;
                _rotateleft(cur->parent->parent);
            }
        }
    }

    // The root node is always colored black.
    m_root->color = black;

    return &node->data->stack;
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
    CallStack::Chunk *chunk = m_store.next;
    CallStack::Chunk *temp;

    while (chunk) {
        temp = chunk;
        chunk = temp->next;
        delete temp;
    }
}

// operator == - Equality operator. Compares the CallStack to another CallStack
//   for equality. Two CallStacks are equal if they are the same size and if
//   every frame in each is identical to the corresponding frame in the other.
//
//  target (IN) - Reference to the CallStack to compare the current CallStack
//    against for equality.
//
//  Return Value:
//
//    Returns true if the two CallStacks are equal. Otherwise returns false.
//
bool CallStack::operator == (const CallStack &target)
{
    CallStack::Chunk       *chunk = &m_store;
    unsigned long           index;
    CallStack::Chunk       *prevchunk = NULL;
    const CallStack::Chunk *targetchunk = &target.m_store;

    if (m_size != target.m_size) {
        // They can't be equal if the sizes are different.
        return false;
    }

    // Walk the chunk list and within each chunk walk the frames array until we
    // either find a mismatch, or until we reach the end of the call stacks.
    while (prevchunk != m_topchunk) {
        for (index = 0; index < ((chunk == m_topchunk) ? m_topindex : CALLSTACKCHUNKSIZE); index++) {
            if (chunk->frames[index] != targetchunk->frames[index]) {
                // Found a mismatch. They are not equal.
                return false;
            }
        }
        prevchunk = chunk;
        chunk = chunk->next;
        targetchunk = targetchunk->next;
    }

    // Reached the end of the call stacks. They are equal.
    return true;
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
DWORD_PTR CallStack::operator [] (unsigned long index)
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
void CallStack::push_back (const DWORD_PTR programcounter)
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
