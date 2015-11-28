////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - VisualLeakDetector Class Implementation
//  Copyright (c) 2005-2014 VLD Team
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

#include "stdafx.h"

#pragma comment(lib, "dbghelp.lib")

#include <sys/stat.h>

#define VLDBUILD         // Declares that we are building Visual Leak Detector.
#include "callstack.h"   // Provides a class for handling call stacks.
#include "crtmfcpatch.h" // Provides CRT and MFC patch functions.
#include "map.h"         // Provides a lightweight STL-like map template.
#include "ntapi.h"       // Provides access to NT APIs.
#include "set.h"         // Provides a lightweight STL-like set template.
#include "utility.h"     // Provides various utility functions.
#include "vldheap.h"     // Provides internal new and delete operators.
#include "vldint.h"      // Provides access to the Visual Leak Detector internals.
#include "loaderlock.h"

extern HANDLE           g_currentProcess;
extern CriticalSection  g_heapMapLock;
extern DbgHelp g_DbgHelp;

////////////////////////////////////////////////////////////////////////////////
//
// Debug CRT and MFC IAT Replacement Functions
//
// The addresses of these functions are not actually directly patched into the
// import address tables, but these functions do get indirectly called by the
// patch functions that are placed in the import address tables.
//
////////////////////////////////////////////////////////////////////////////////

// GetProcessHeap - Calls to GetProcessHeap are patched through to this function. This
//   function is just a wrapper around the real GetProcessHeap.
//
//  Return Value:
//
//    Returns the value returned by GetProcessHeap.
//
HANDLE VisualLeakDetector::_GetProcessHeap()
{
    PRINT_HOOKED_FUNCTION();
    // Get the process heap.
    HANDLE heap = m_GetProcessHeap();

    CriticalSectionLocker<> cs(g_heapMapLock);
    HeapMap::Iterator heapit = g_vld.m_heapMap->find(heap);
    if (heapit == g_vld.m_heapMap->end())
    {
        g_vld.mapHeap(heap);
        heapit = g_vld.m_heapMap->find(heap);
    }

    return heap;
}

// _HeapCreate - Calls to HeapCreate are patched through to this function. This
//   function is just a wrapper around the real HeapCreate that calls VLD's heap
//   creation tracking function after the heap has been created.
//
//  - options (IN): Heap options.
//
//  - initsize (IN): Initial size of the heap.
//
//  - maxsize (IN): Maximum size of the heap.
//
//  Return Value:
//
//    Returns the value returned by HeapCreate.
//
HANDLE VisualLeakDetector::_HeapCreate (DWORD options, SIZE_T initsize, SIZE_T maxsize)
{
    PRINT_HOOKED_FUNCTION();
    // Create the heap.
    HANDLE heap = m_HeapCreate(options, initsize, maxsize);

    CriticalSectionLocker<> cs(g_heapMapLock);

    // Map the created heap handle to a new block map.
    g_vld.mapHeap(heap);

    HeapMap::Iterator heapit = g_vld.m_heapMap->find(heap);
    assert(heapit != g_vld.m_heapMap->end());

    return heap;
}

// _HeapDestroy - Calls to HeapDestroy are patched through to this function.
//   This function is just a wrapper around the real HeapDestroy that calls
//   VLD's heap destruction tracking function after the heap has been destroyed.
//
//  - heap (IN): Handle to the heap to be destroyed.
//
//  Return Value:
//
//    Returns the valued returned by HeapDestroy.
//
BOOL VisualLeakDetector::_HeapDestroy (HANDLE heap)
{
    PRINT_HOOKED_FUNCTION();
    // After this heap is destroyed, the heap's address space will be unmapped
    // from the process's address space. So, we'd better generate a leak report
    // for this heap now, while we can still read from the memory blocks
    // allocated to it.
    if (!(g_vld.m_options & VLD_OPT_SKIP_HEAPFREE_LEAKS))
        g_vld.reportHeapLeaks(heap);

    g_vld.unmapHeap(heap);

    return HeapDestroy(heap);
}

// _RtlAllocateHeap - Calls to RtlAllocateHeap are patched through to this
//   function. This function invokes the real RtlAllocateHeap and then calls
//   VLD's allocation tracking function. Pretty much all memory allocations
//   will eventually result in a call to RtlAllocateHeap, so this is where we
//   finally map the allocated block.
//
//  - heap (IN): Handle to the heap from which to allocate memory.
//
//  - flags (IN): Heap allocation control flags.
//
//  - size (IN): Size, in bytes, of the block to allocate.
//
//  Return Value:
//
//    Returns the return value from RtlAllocateHeap.
//
LPVOID VisualLeakDetector::_RtlAllocateHeap (HANDLE heap, DWORD flags, SIZE_T size)
{
    PRINT_HOOKED_FUNCTION2();
    // Allocate the block.
    LPVOID block = RtlAllocateHeap(heap, flags, size);

    if ((block == NULL) || !g_vld.enabled())
        return block;

    if (!g_DbgHelp.IsLockedByCurrentThread()) { // skip dbghelp.dll calls
        CAPTURE_CONTEXT();
        CaptureContext cc(RtlAllocateHeap, context_);
        cc.Set(heap, block, NULL, size);
    }

    return block;
}

// HeapAlloc (kernel32.dll) call RtlAllocateHeap (ntdll.dll)
LPVOID VisualLeakDetector::_HeapAlloc (HANDLE heap, DWORD flags, SIZE_T size)
{
    PRINT_HOOKED_FUNCTION2();
    // Allocate the block.
    LPVOID block = HeapAlloc(heap, flags, size);

    if ((block == NULL) || !g_vld.enabled())
        return block;

    if (!g_DbgHelp.IsLockedByCurrentThread()) { // skip dbghelp.dll calls
        CAPTURE_CONTEXT();
        CaptureContext cc(HeapAlloc, context_);
        cc.Set(heap, block, NULL, size);
    }

    return block;
}

// _RtlFreeHeap - Calls to RtlFreeHeap are patched through to this function.
//   This function calls VLD's free tracking function and then invokes the real
//   RtlFreeHeap. Pretty much all memory frees will eventually result in a call
//   to RtlFreeHeap, so this is where we finally unmap the freed block.
//
//  - heap (IN): Handle to the heap to which the block being freed belongs.
//
//  - flags (IN): Heap control flags.
//
//  - mem (IN): Pointer to the memory block being freed.
//
//  Return Value:
//
//    Returns the value returned by RtlFreeHeap.
//
BYTE VisualLeakDetector::_RtlFreeHeap (HANDLE heap, DWORD flags, LPVOID mem)
{
    PRINT_HOOKED_FUNCTION2();
    BYTE status;

    if (!g_DbgHelp.IsLockedByCurrentThread()) // skip dbghelp.dll calls
    {
        // Record the current frame pointer.
        CAPTURE_CONTEXT();
        context_.func = reinterpret_cast<UINT_PTR>(RtlFreeHeap);

        // Unmap the block from the specified heap.
        g_vld.unmapBlock(heap, mem, context_);
    }

    status = RtlFreeHeap(heap, flags, mem);

    return status;
}

// HeapFree (kernel32.dll) call RtlFreeHeap (ntdll.dll)
BOOL VisualLeakDetector::_HeapFree (HANDLE heap, DWORD flags, LPVOID mem)
{
    PRINT_HOOKED_FUNCTION2();
    BOOL status;

    if (!g_DbgHelp.IsLockedByCurrentThread()) // skip dbghelp.dll calls
    {
        // Record the current frame pointer.
        CAPTURE_CONTEXT();
        context_.func = reinterpret_cast<UINT_PTR>(m_HeapFree);

        // Unmap the block from the specified heap.
        g_vld.unmapBlock(heap, mem, context_);
    }

    status = m_HeapFree(heap, flags, mem);

    return status;
}

// _RtlReAllocateHeap - Calls to RtlReAllocateHeap are patched through to this
//   function. This function invokes the real RtlReAllocateHeap and then calls
//   VLD's reallocation tracking function. All arguments passed to this function
//   are passed on to the real RtlReAllocateHeap without modification. Pretty
//   much all memory re-allocations will eventually result in a call to
//   RtlReAllocateHeap, so this is where we finally remap the reallocated block.
//
//  - heap (IN): Handle to the heap to reallocate memory from.
//
//  - flags (IN): Heap control flags.
//
//  - mem (IN): Pointer to the currently allocated block which is to be
//      reallocated.
//
//  - size (IN): Size, in bytes, of the block to reallocate.
//
//  Return Value:
//
//    Returns the value returned by RtlReAllocateHeap.
//
LPVOID VisualLeakDetector::_RtlReAllocateHeap (HANDLE heap, DWORD flags, LPVOID mem, SIZE_T size)
{
    PRINT_HOOKED_FUNCTION();

    // Reallocate the block.
    LPVOID newmem = RtlReAllocateHeap(heap, flags, mem, size);
    if ((newmem == NULL) || !g_vld.enabled())
        return newmem;

    if (!g_DbgHelp.IsLockedByCurrentThread()) { // skip dbghelp.dll calls
        CAPTURE_CONTEXT();
        CaptureContext cc(RtlReAllocateHeap, context_);
        cc.Set(heap, mem, newmem, size);
    }

    return newmem;
}

// for kernel32.dll
LPVOID VisualLeakDetector::_HeapReAlloc (HANDLE heap, DWORD flags, LPVOID mem, SIZE_T size)
{
    PRINT_HOOKED_FUNCTION();

    // Reallocate the block.
    LPVOID newmem = HeapReAlloc(heap, flags, mem, size);
    if ((newmem == NULL) || !g_vld.enabled())
        return newmem;

    if (!g_DbgHelp.IsLockedByCurrentThread()) { // skip dbghelp.dll calls
        CAPTURE_CONTEXT();
        CaptureContext cc(HeapReAlloc, context_);
        cc.Set(heap, mem, newmem, size);
    }

    return newmem;
}

////////////////////////////////////////////////////////////////////////////////
//
// COM IAT Replacement Functions
//
////////////////////////////////////////////////////////////////////////////////

// _CoGetMalloc - Calls to CoGetMalloc are patched through to this function.
//   This function returns a pointer to Visual Leak Detector's implementation
//   of the IMalloc interface, instead of returning a pointer to the system
//   implementation. This allows VLD's implementation of the IMalloc interface
//   (which is basically a thin wrapper around the system implementation) to be
//   invoked in place of the system implementation.
//
//  - context (IN): Reserved; value must be 1.
//
//  - imalloc (IN): Address of a pointer to receive the address of VLD's
//      implementation of the IMalloc interface.
//
//  Return Value:
//
//    Always returns S_OK.
//
HRESULT VisualLeakDetector::_CoGetMalloc (DWORD context, LPMALLOC *imalloc)
{
    PRINT_HOOKED_FUNCTION();
    static CoGetMalloc_t pCoGetMalloc = NULL;

    HRESULT hr = S_OK;

    HMODULE ole32;

    *imalloc = (LPMALLOC)&g_vld;

    if (pCoGetMalloc == NULL) {
        // This is the first call to this function. Link to the real
        // CoGetMalloc and get a pointer to the system implementation of the
        // IMalloc interface.
        ole32 = GetModuleHandleW(L"ole32.dll");
        pCoGetMalloc = (CoGetMalloc_t)g_vld._RGetProcAddress(ole32, "CoGetMalloc");
        hr = pCoGetMalloc(context, &g_vld.m_iMalloc);

        // Increment the library reference count to defer unloading the library,
        // since a call to CoGetMalloc returns the global pointer to the VisualLeakDetector object.
        HMODULE module = NULL;
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)g_vld.m_vldBase, &module);
    }
    else
    {
        // wait for different thread initialization
        int c = 0;
        while(g_vld.m_iMalloc == NULL && c < 10)
        {
            Sleep(1);
            c++;
        }
        if (g_vld.m_iMalloc == NULL)
            hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr)) {
        g_vld.AddRef();
    }
    return hr;
}

// _CoTaskMemAlloc - Calls to CoTaskMemAlloc are patched through to this
//   function. This function is just a wrapper around the real CoTaskMemAlloc
//   that sets appropriate flags to be consulted when the memory is actually
//   allocated by RtlAllocateHeap.
//
//  - size (IN): Size of the memory block to allocate.
//
//  Return Value:
//
//    Returns the value returned from CoTaskMemAlloc.
//
LPVOID VisualLeakDetector::_CoTaskMemAlloc (SIZE_T size)
{
    PRINT_HOOKED_FUNCTION();
    static CoTaskMemAlloc_t pCoTaskMemAlloc = NULL;

    if (pCoTaskMemAlloc == NULL) {
        // This is the first call to this function. Link to the real
        // CoTaskMemAlloc.
        HMODULE ole32 = GetModuleHandleW(L"ole32.dll");
        pCoTaskMemAlloc = (CoTaskMemAlloc_t)g_vld._RGetProcAddress(ole32, "CoTaskMemAlloc");
    }

    CAPTURE_CONTEXT();
    CaptureContext cc((void*)pCoTaskMemAlloc, context_);

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    return pCoTaskMemAlloc(size);
}

// _CoTaskMemRealloc - Calls to CoTaskMemRealloc are patched through to this
//   function. This function is just a wrapper around the real CoTaskMemRealloc
//   that sets appropriate flags to be consulted when the memory is actually
//   allocated by RtlAllocateHeap.
//
//  - mem (IN): Pointer to the memory block to reallocate.
//
//  - size (IN): Size, in bytes, of the block to reallocate.
//
//  Return Value:
//
//    Returns the value returned from CoTaskMemRealloc.
//
LPVOID VisualLeakDetector::_CoTaskMemRealloc (LPVOID mem, SIZE_T size)
{
    PRINT_HOOKED_FUNCTION();
    static CoTaskMemRealloc_t pCoTaskMemRealloc = NULL;

    if (pCoTaskMemRealloc == NULL) {
        // This is the first call to this function. Link to the real
        // CoTaskMemRealloc.
        HMODULE ole32 = GetModuleHandleW(L"ole32.dll");
        pCoTaskMemRealloc = (CoTaskMemRealloc_t)g_vld._RGetProcAddress(ole32, "CoTaskMemRealloc");
    }

    CAPTURE_CONTEXT();
    CaptureContext cc((void*)pCoTaskMemRealloc, context_);

    // Do the allocation. The block will be mapped by _RtlReAllocateHeap.
    return pCoTaskMemRealloc(mem, size);
}

////////////////////////////////////////////////////////////////////////////////
//
// Public COM IMalloc Implementation Functions
//
////////////////////////////////////////////////////////////////////////////////

// AddRef - Calls to IMalloc::AddRef end up here. This function is just a
//   wrapper around the real IMalloc::AddRef implementation.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::AddRef.
//
ULONG VisualLeakDetector::AddRef ()
{
    PRINT_HOOKED_FUNCTION();
    assert(m_iMalloc != NULL);
    if (m_iMalloc) {
        // Increment the library reference count to defer unloading the library,
        // since this function increments the reference count of the IMalloc interface.
        HMODULE module = NULL;
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)m_vldBase, &module);
        return m_iMalloc->AddRef();
    }
    return 0;
}

// Alloc - Calls to IMalloc::Alloc end up here. This function is just a wrapper
//   around the real IMalloc::Alloc implementation that sets appropriate flags
//   to be consulted when the memory is actually allocated by RtlAllocateHeap.
//
//  - size (IN): The size of the memory block to allocate.
//
//  Return Value:
//
//    Returns the value returned by the system's IMalloc::Alloc implementation.
//
LPVOID VisualLeakDetector::Alloc (_In_ SIZE_T size)
{
    PRINT_HOOKED_FUNCTION();
    UINT_PTR* cVtablePtr = (UINT_PTR*)((UINT_PTR*)m_iMalloc)[0];
    UINT_PTR iMallocAlloc = cVtablePtr[3];
    CAPTURE_CONTEXT();
    CaptureContext cc((void*)iMallocAlloc, context_);

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    assert(m_iMalloc != NULL);
    return (m_iMalloc) ? m_iMalloc->Alloc(size) : NULL;
}

// DidAlloc - Calls to IMalloc::DidAlloc will end up here. This function is just
//   a wrapper around the system implementation of IMalloc::DidAlloc.
//
//  - mem (IN): Pointer to a memory block to inquire about.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::DidAlloc.
//
INT VisualLeakDetector::DidAlloc (_In_opt_ LPVOID mem)
{
    PRINT_HOOKED_FUNCTION();
    assert(m_iMalloc != NULL);
    return (m_iMalloc) ? m_iMalloc->DidAlloc(mem) : 0;
}

// Free - Calls to IMalloc::Free will end up here. This function is just a
//   wrapper around the real IMalloc::Free implementation.
//
//  - mem (IN): Pointer to the memory block to be freed.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::Free (_In_opt_ LPVOID mem)
{
    PRINT_HOOKED_FUNCTION();
    assert(m_iMalloc != NULL);
    if (m_iMalloc) m_iMalloc->Free(mem);
}

// GetSize - Calls to IMalloc::GetSize will end up here. This function is just a
//   wrapper around the real IMalloc::GetSize implementation.
//
//  - mem (IN): Pointer to the memory block to inquire about.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::GetSize.
//
SIZE_T VisualLeakDetector::GetSize (_In_opt_ LPVOID mem)
{
    PRINT_HOOKED_FUNCTION();
    assert(m_iMalloc != NULL);
    return (m_iMalloc) ? m_iMalloc->GetSize(mem) : 0;
}

// HeapMinimize - Calls to IMalloc::HeapMinimize will end up here. This function
//   is just a wrapper around the real IMalloc::HeapMinimize implementation.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::HeapMinimize ()
{
    PRINT_HOOKED_FUNCTION();
    assert(m_iMalloc != NULL);
    if (m_iMalloc) m_iMalloc->HeapMinimize();
}

// QueryInterface - Calls to IMalloc::QueryInterface will end up here. This
//   function is just a wrapper around the real IMalloc::QueryInterface
//   implementation.
//
//  - iid (IN): COM interface ID to query about.
//
//  - object (IN): Address of a pointer to receive the requested interface
//      pointer.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::QueryInterface.
//
HRESULT VisualLeakDetector::QueryInterface (REFIID iid, LPVOID *object)
{
    PRINT_HOOKED_FUNCTION();
    assert(m_iMalloc != NULL);
    return (m_iMalloc) ? m_iMalloc->QueryInterface(iid, object) : E_UNEXPECTED;
}

// Realloc - Calls to IMalloc::Realloc will end up here. This function is just a
//   wrapper around the real IMalloc::Realloc implementation that sets
//   appropriate flags to be consulted when the memory is actually allocated by
//   RtlAllocateHeap.
//
//  - mem (IN): Pointer to the memory block to reallocate.
//
//  - size (IN): Size, in bytes, of the memory block to reallocate.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::Realloc.
//
LPVOID VisualLeakDetector::Realloc (_In_opt_ LPVOID mem, _In_ SIZE_T size)
{
    PRINT_HOOKED_FUNCTION();
    UINT_PTR* cVtablePtr = (UINT_PTR*)((UINT_PTR*)m_iMalloc)[0];
    UINT_PTR iMallocRealloc = cVtablePtr[4];
    CAPTURE_CONTEXT();
    CaptureContext cc((void*)iMallocRealloc, context_);

    // Do the allocation. The block will be mapped by _RtlReAllocateHeap.
    assert(m_iMalloc != NULL);
    return (m_iMalloc) ? m_iMalloc->Realloc(mem, size) : NULL;
}

// Release - Calls to IMalloc::Release will end up here. This function is just
//   a wrapper around the real IMalloc::Release implementation.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::Release.
//
ULONG VisualLeakDetector::Release ()
{
    PRINT_HOOKED_FUNCTION();
    assert(m_iMalloc != NULL);
    ULONG nCount = 0;
    if (m_iMalloc) {
        nCount = m_iMalloc->Release();

        // Decrement the library reference count.
        FreeLibrary(m_vldBase);
    }
    return nCount;
}
