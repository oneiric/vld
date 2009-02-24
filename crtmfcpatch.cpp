////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - CRT and MFC IAT Patch Functions
//  Copyright (c) 2009 Dan Moulding
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

#define VLDBUILD
#include "crtmfcpatch.h"
#include "vldint.h"

extern VisualLeakDetector vld;

////////////////////////////////////////////////////////////////////////////////
//
// Visual Studio 2005 DLLs
//
////////////////////////////////////////////////////////////////////////////////

// crt80d__calloc_dbg - Calls to _calloc_dbg from msvcr80d.dll are patched
//   through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _calloc_dbg.
//
void* crt80d__calloc_dbg (size_t num, size_t size, int type, char const *file, int line)
{
    static _calloc_dbg_t pcrt80d__calloc_dbg = NULL;

    SIZE_T  fp;
    HMODULE msvcr80d;

    FRAMEPOINTER(fp);

    if (pcrt80d__calloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _malloc_dbg.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d__calloc_dbg = (_calloc_dbg_t)GetProcAddress(msvcr80d, "_calloc_dbg");
    }

    return vld.__calloc_dbg(pcrt80d__calloc_dbg, fp, num, size, type, file, line);
}

// crt80d__malloc_dbg - Calls to _malloc_dbg from msvcr80d.dll are patched
//   through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _malloc_dbg.
//
void* crt80d__malloc_dbg (size_t size, int type, char const *file, int line)
{
    static _malloc_dbg_t pcrt80d__malloc_dbg = NULL;

    SIZE_T  fp;
    HMODULE msvcr80d;

    FRAMEPOINTER(fp);

    if (pcrt80d__malloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _malloc_dbg.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d__malloc_dbg = (_malloc_dbg_t)GetProcAddress(msvcr80d, "_malloc_dbg");
    }

    return vld.__malloc_dbg(pcrt80d__malloc_dbg, fp, size, type, file, line);
}

// crt80d__realloc_dbg - Calls to _realloc_dbg from msvcr80d.dll are patched
//   through to this function.
//
//  - mem (IN): Pointer to the memory block to be reallocated.
//
//  - size (IN): The size of the memory block to reallocate.
//
//  - type (IN): The CRT "use type" of the block to be reallocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above filel, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _realloc_dbg.
//
void* crt80d__realloc_dbg (void *mem, size_t size, int type, char const *file, int line)
{
    static _realloc_dbg_t pcrt80d__realloc_dbg = NULL;

    SIZE_T  fp;
    HMODULE msvcr80d;

    FRAMEPOINTER(fp);

    if (pcrt80d__realloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _realloc_dbg.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d__realloc_dbg = (_realloc_dbg_t)GetProcAddress(msvcr80d, "_realloc_dbg");
    }

    return vld.__realloc_dbg(pcrt80d__realloc_dbg, fp, mem, size, type, file, line);
}

// crt80d__scalar_new_dbg - Calls to the CRT's debug scalar new operator from
//   msvcr80d.dll are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the CRT debug scalar new operator.
//
void* crt80d__scalar_new_dbg (unsigned int size, int type, char const *file, int line)
{
    static new_dbg_crt_t pcrt80d__scalar_new_dbg = NULL;

    SIZE_T  fp;
    HMODULE msvcr80d;

    FRAMEPOINTER(fp);

    if (pcrt80d__scalar_new_dbg == NULL) {
        // This is the first call to this function. Link to the real CRT debug
        // new operator.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d__scalar_new_dbg = (new_dbg_crt_t)GetProcAddress(msvcr80d, "??2@YAPAXIHPBDH@Z");
    }

    return vld.new_dbg_crt(pcrt80d__scalar_new_dbg, fp, size, type, file, line);
}

// crt80d__vector_new_dbg - Calls to the CRT's debug vector new operator from
//   msvcr80d.dll are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the CRT debug vector new operator.
//
void* crt80d__vector_new_dbg (unsigned int size, int type, char const *file, int line)
{
    static new_dbg_crt_t pcrt80d__vector_new_dbg = NULL;

    SIZE_T  fp;
    HMODULE msvcr80d;

    FRAMEPOINTER(fp);

    if (pcrt80d__vector_new_dbg == NULL) {
        // This is the first call to this function. Link to the real CRT debug
        // new operator.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d__vector_new_dbg = (new_dbg_crt_t)GetProcAddress(msvcr80d, "??_U@YAPAXIHPBDH@Z");
    }

    return vld.new_dbg_crt(pcrt80d__vector_new_dbg, fp, size, type, file, line);
}

// crt80d_calloc - Calls to calloc from msvcr80d.dll are patched through to
//   this function.
//
//  - num (IN): The number of blocks, of size 'size', to be allocated.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the valued returned from calloc.
//
void* crt80d_calloc (size_t num, size_t size)
{
    static calloc_t pcrt80d_calloc = NULL;

    SIZE_T  fp;
    HMODULE msvcr80d;

    FRAMEPOINTER(fp);

    if (pcrt80d_calloc == NULL) {
        // This is the first call to this function. Link to the real malloc.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d_calloc = (calloc_t)GetProcAddress(msvcr80d, "calloc");
    }

    return vld._calloc(pcrt80d_calloc, fp, num, size);
}

// crt80d_malloc - Calls to malloc from msvcr80d.dll are patched through to
//   this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the valued returned from malloc.
//
void* crt80d_malloc (size_t size)
{
    static malloc_t pcrt80d_malloc = NULL;

    SIZE_T  fp;
    HMODULE msvcr80d;

    FRAMEPOINTER(fp);

    if (pcrt80d_malloc == NULL) {
        // This is the first call to this function. Link to the real malloc.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d_malloc = (malloc_t)GetProcAddress(msvcr80d, "malloc");
    }

    return vld._malloc(pcrt80d_malloc, fp, size);
}

// crt80d_realloc - Calls to realloc from msvcr80d.dll are patched through to
//   this function.
//
//  - mem (IN): Pointer to the memory block to reallocate.
//
//  - size (IN): Size of the memory block to reallocate.
//
//  Return Value:
//
//    Returns the value returned from realloc.
//
void* crt80d_realloc (void *mem, size_t size)
{
    static realloc_t pcrt80d_realloc = NULL;

    SIZE_T  fp;
    HMODULE msvcr80d;

    FRAMEPOINTER(fp);

    if (pcrt80d_realloc == NULL) {
        // This is the first call to this function. Link to the real realloc.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d_realloc = (realloc_t)GetProcAddress(msvcr80d, "realloc");
    }

    return vld._realloc(pcrt80d_realloc, fp, mem, size);
}

// crt80d_scalar_new - Calls to the CRT's scalar new operator from msvcr80d.dll
//   are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the CRT scalar new operator.
//
void* crt80d_scalar_new (unsigned int size)
{
    static new_t pcrt80d_scalar_new = NULL;

    SIZE_T  fp;
    HMODULE msvcr80d;

    FRAMEPOINTER(fp);

    if (pcrt80d_scalar_new == NULL) {
        // This is the first call to this function. Link to the real CRT new
        // operator.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d_scalar_new = (new_t)GetProcAddress(msvcr80d, "??2@YAPAXI@Z");
    }

    return vld._new(pcrt80d_scalar_new, fp, size);
}

// crt80d_vector_new - Calls to the CRT's vector new operator from msvcr80d.dll
//   are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the CRT vector new operator.
//
void* crt80d_vector_new (unsigned int size)
{
    static new_t pcrt80d_vector_new = NULL;

    SIZE_T  fp;
    HMODULE msvcr80d;

    FRAMEPOINTER(fp);

    if (pcrt80d_vector_new == NULL) {
        // This is the first call to this function. Link to the real CRT new
        // operator.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d_vector_new = (new_t)GetProcAddress(msvcr80d, "??_U@YAPAXI@Z");
    }

    return vld._new(pcrt80d_vector_new, fp, size);
}


////////////////////////////////////////////////////////////////////////////////
//
// Visual Studio 2008 DLLs
//
////////////////////////////////////////////////////////////////////////////////

// crt90d__calloc_dbg - Calls to _calloc_dbg from msvcr90d.dll are patched
//   through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _calloc_dbg.
//
void* crt90d__calloc_dbg (size_t num, size_t size, int type, char const *file, int line)
{
    static _calloc_dbg_t pcrt90d__calloc_dbg = NULL;

    SIZE_T  fp;
    HMODULE msvcr90d;

    FRAMEPOINTER(fp);

    if (pcrt90d__calloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _malloc_dbg.
        msvcr90d = GetModuleHandle(L"msvcr90d.dll");
        pcrt90d__calloc_dbg = (_calloc_dbg_t)GetProcAddress(msvcr90d, "_calloc_dbg");
    }

    return vld.__calloc_dbg(pcrt90d__calloc_dbg, fp, num, size, type, file, line);
}

// crt90d__malloc_dbg - Calls to _malloc_dbg from msvcr90d.dll are patched
//   through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _malloc_dbg.
//
void* crt90d__malloc_dbg (size_t size, int type, char const *file, int line)
{
    static _malloc_dbg_t pcrt90d__malloc_dbg = NULL;

    SIZE_T  fp;
    HMODULE msvcr90d;

    FRAMEPOINTER(fp);

    if (pcrt90d__malloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _malloc_dbg.
        msvcr90d = GetModuleHandle(L"msvcr90d.dll");
        pcrt90d__malloc_dbg = (_malloc_dbg_t)GetProcAddress(msvcr90d, "_malloc_dbg");
    }

    return vld.__malloc_dbg(pcrt90d__malloc_dbg, fp, size, type, file, line);
}

// crt90d__realloc_dbg - Calls to _realloc_dbg from msvcr90d.dll are patched
//   through to this function.
//
//  - mem (IN): Pointer to the memory block to be reallocated.
//
//  - size (IN): The size of the memory block to reallocate.
//
//  - type (IN): The CRT "use type" of the block to be reallocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above filel, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _realloc_dbg.
//
void* crt90d__realloc_dbg (void *mem, size_t size, int type, char const *file, int line)
{
    static _realloc_dbg_t pcrt90d__realloc_dbg = NULL;

    SIZE_T  fp;
    HMODULE msvcr90d;

    FRAMEPOINTER(fp);

    if (pcrt90d__realloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _realloc_dbg.
        msvcr90d = GetModuleHandle(L"msvcr90d.dll");
        pcrt90d__realloc_dbg = (_realloc_dbg_t)GetProcAddress(msvcr90d, "_realloc_dbg");
    }

    return vld.__realloc_dbg(pcrt90d__realloc_dbg, fp, mem, size, type, file, line);
}

// crt90d__scalar_new_dbg - Calls to the CRT's debug scalar new operator from
//   msvcr90d.dll are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the CRT debug scalar new operator.
//
void* crt90d__scalar_new_dbg (unsigned int size, int type, char const *file, int line)
{
    static new_dbg_crt_t pcrt90d__scalar_new_dbg = NULL;

    SIZE_T  fp;
    HMODULE msvcr90d;

    FRAMEPOINTER(fp);

    if (pcrt90d__scalar_new_dbg == NULL) {
        // This is the first call to this function. Link to the real CRT debug
        // new operator.
        msvcr90d = GetModuleHandle(L"msvcr90d.dll");
        pcrt90d__scalar_new_dbg = (new_dbg_crt_t)GetProcAddress(msvcr90d, "??2@YAPAXIHPBDH@Z");
    }

    return vld.new_dbg_crt(pcrt90d__scalar_new_dbg, fp, size, type, file, line);
}

// crt90d__vector_new_dbg - Calls to the CRT's debug vector new operator from
//   msvcr90d.dll are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the CRT debug vector new operator.
//
void* crt90d__vector_new_dbg (unsigned int size, int type, char const *file, int line)
{
    static new_dbg_crt_t pcrt90d__vector_new_dbg = NULL;

    SIZE_T  fp;
    HMODULE msvcr90d;

    FRAMEPOINTER(fp);

    if (pcrt90d__vector_new_dbg == NULL) {
        // This is the first call to this function. Link to the real CRT debug
        // new operator.
        msvcr90d = GetModuleHandle(L"msvcr90d.dll");
        pcrt90d__vector_new_dbg = (new_dbg_crt_t)GetProcAddress(msvcr90d, "??_U@YAPAXIHPBDH@Z");
    }

    return vld.new_dbg_crt(pcrt90d__vector_new_dbg, fp, size, type, file, line);
}

// crt90d_calloc - Calls to calloc from msvcr90d.dll are patched through to
//   this function.
//
//  - num (IN): The number of blocks, of size 'size', to be allocated.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the valued returned from calloc.
//
void* crt90d_calloc (size_t num, size_t size)
{
    static calloc_t pcrt90d_calloc = NULL;

    SIZE_T  fp;
    HMODULE msvcr90d;

    FRAMEPOINTER(fp);

    if (pcrt90d_calloc == NULL) {
        // This is the first call to this function. Link to the real malloc.
        msvcr90d = GetModuleHandle(L"msvcr90d.dll");
        pcrt90d_calloc = (calloc_t)GetProcAddress(msvcr90d, "calloc");
    }

    return vld._calloc(pcrt90d_calloc, fp, num, size);
}

// crt90d_malloc - Calls to malloc from msvcr90d.dll are patched through to
//   this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the valued returned from malloc.
//
void* crt90d_malloc (size_t size)
{
    static malloc_t pcrt90d_malloc = NULL;

    SIZE_T  fp;
    HMODULE msvcr90d;

    FRAMEPOINTER(fp);

    if (pcrt90d_malloc == NULL) {
        // This is the first call to this function. Link to the real malloc.
        msvcr90d = GetModuleHandle(L"msvcr90d.dll");
        pcrt90d_malloc = (malloc_t)GetProcAddress(msvcr90d, "malloc");
    }

    return vld._malloc(pcrt90d_malloc, fp, size);
}

// crt90d_realloc - Calls to realloc from msvcr90d.dll are patched through to
//   this function.
//
//  - mem (IN): Pointer to the memory block to reallocate.
//
//  - size (IN): Size of the memory block to reallocate.
//
//  Return Value:
//
//    Returns the value returned from realloc.
//
void* crt90d_realloc (void *mem, size_t size)
{
    static realloc_t pcrt90d_realloc = NULL;

    SIZE_T  fp;
    HMODULE msvcr90d;

    FRAMEPOINTER(fp);

    if (pcrt90d_realloc == NULL) {
        // This is the first call to this function. Link to the real realloc.
        msvcr90d = GetModuleHandle(L"msvcr90d.dll");
        pcrt90d_realloc = (realloc_t)GetProcAddress(msvcr90d, "realloc");
    }

    return vld._realloc(pcrt90d_realloc, fp, mem, size);
}

// crt90d_scalar_new - Calls to the CRT's scalar new operator from msvcr90d.dll
//   are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the CRT scalar new operator.
//
void* crt90d_scalar_new (unsigned int size)
{
    static new_t pcrt90d_scalar_new = NULL;

    SIZE_T  fp;
    HMODULE msvcr90d;

    FRAMEPOINTER(fp);

    if (pcrt90d_scalar_new == NULL) {
        // This is the first call to this function. Link to the real CRT new
        // operator.
        msvcr90d = GetModuleHandle(L"msvcr90d.dll");
        pcrt90d_scalar_new = (new_t)GetProcAddress(msvcr90d, "??2@YAPAXI@Z");
    }

    return vld._new(pcrt90d_scalar_new, fp, size);
}

// crt90d_vector_new - Calls to the CRT's vector new operator from msvcr90d.dll
//   are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the CRT vector new operator.
//
void* crt90d_vector_new (unsigned int size)
{
    static new_t pcrt90d_vector_new = NULL;

    SIZE_T  fp;
    HMODULE msvcr90d;

    FRAMEPOINTER(fp);

    if (pcrt90d_vector_new == NULL) {
        // This is the first call to this function. Link to the real CRT new
        // operator.
        msvcr90d = GetModuleHandle(L"msvcr90d.dll");
        pcrt90d_vector_new = (new_t)GetProcAddress(msvcr90d, "??_U@YAPAXI@Z");
    }

    return vld._new(pcrt90d_vector_new, fp, size);
}


////////////////////////////////////////////////////////////////////////////////
//
// Visual Studio 6.0 DLLs
//
////////////////////////////////////////////////////////////////////////////////

// crtd__calloc_dbg - Calls to _calloc_dbg from msvcrtd.dll are patched through
//   to this function.
//
//  - num (IN): The number of blocks, of size 'size', to be allocated.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _calloc_dbg.
//
void* crtd__calloc_dbg (size_t num, size_t size, int type, char const *file, int line)
{
    static _calloc_dbg_t pcrtd__calloc_dbg = NULL;

    SIZE_T  fp;
    HMODULE msvcrtd;

    FRAMEPOINTER(fp);

    if (pcrtd__calloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _malloc_dbg.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd__calloc_dbg = (_calloc_dbg_t)GetProcAddress(msvcrtd, "_calloc_dbg");
    }

    return vld.__calloc_dbg(pcrtd__calloc_dbg, fp, num, size, type, file, line);
}

// crtd__malloc_dbg - Calls to _malloc_dbg from msvcrtd.dll are patched through
//   to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _malloc_dbg.
//
void* crtd__malloc_dbg (size_t size, int type, char const *file, int line)
{
    static _malloc_dbg_t pcrtd__malloc_dbg = NULL;

    SIZE_T  fp;
    HMODULE msvcrtd;

    FRAMEPOINTER(fp);

    if (pcrtd__malloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _malloc_dbg.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd__malloc_dbg = (_malloc_dbg_t)GetProcAddress(msvcrtd, "_malloc_dbg");
    }

    return vld.__malloc_dbg(pcrtd__malloc_dbg, fp, size, type, file, line);
}

// crtd__realloc_dbg - Calls to _realloc_dbg from msvcrtd.dll are patched
//   through to this function.
//
//  - mem (IN): Pointer to the memory block to be reallocated.
//
//  - size (IN): The size of the memory block to reallocate.
//
//  - type (IN): The CRT "use type" of the block to be reallocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above filel, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _realloc_dbg.
//
void* crtd__realloc_dbg (void *mem, size_t size, int type, char const *file, int line)
{
    static _realloc_dbg_t pcrtd__realloc_dbg = NULL;

    SIZE_T  fp;
    HMODULE msvcrtd;

    FRAMEPOINTER(fp);

    if (pcrtd__realloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _realloc_dbg.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd__realloc_dbg = (_realloc_dbg_t)GetProcAddress(msvcrtd, "_realloc_dbg");
    }

    return vld.__realloc_dbg(pcrtd__realloc_dbg, fp, mem, size, type, file, line);
}

// crtd__scalar_new_dbg - Calls to the CRT's debug scalar new operator from
//   msvcrtd.dll are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the CRT debug scalar new operator.
//
void* crtd__scalar_new_dbg (unsigned int size, int type, char const *file, int line)
{
    static new_dbg_crt_t pcrtd__scalar_new_dbg = NULL;

    SIZE_T  fp;
    HMODULE msvcrtd;

    FRAMEPOINTER(fp);

    if (pcrtd__scalar_new_dbg == NULL) {
        // This is the first call to this function. Link to the real CRT debug
        // scalar new operator.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd__scalar_new_dbg = (new_dbg_crt_t)GetProcAddress(msvcrtd, "??2@YAPAXIHPBDH@Z");
    }

    return vld.new_dbg_crt(pcrtd__scalar_new_dbg, fp, size, type, file, line);
}

// crtd__vector_new_dbg - Calls to the CRT's debug vector new operator from
//   msvcrtd.dll are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the CRT debug vector new operator.
//
void* crtd__vector_new_dbg (unsigned int size, int type, char const *file, int line)
{
    static new_dbg_crt_t pcrtd__vector_new_dbg = NULL;

    SIZE_T  fp;
    HMODULE msvcrtd;

    FRAMEPOINTER(fp);

    if (pcrtd__vector_new_dbg == NULL) {
        // This is the first call to this function. Link to the real CRT debug
        // vector new operator.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd__vector_new_dbg = (new_dbg_crt_t)GetProcAddress(msvcrtd, "??_U@YAPAXIHPBDH@Z");
    }

    return vld.new_dbg_crt(pcrtd__vector_new_dbg, fp, size, type, file, line);
}

// crtd_calloc - Calls to calloc from msvcrtd.dll are patched through to this
//   function.
//
//  - num (IN): The number of blocks, of size 'size', to be allocated.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the valued returned from calloc.
//
void* crtd_calloc (size_t num, size_t size)
{
    static calloc_t pcrtd_calloc = NULL;

    SIZE_T  fp;
    HMODULE msvcrtd;

    FRAMEPOINTER(fp);

    if (pcrtd_calloc == NULL) {
        // This is the first call to this function. Link to the real malloc.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd_calloc = (calloc_t)GetProcAddress(msvcrtd, "calloc");
    }

    return vld._calloc(pcrtd_calloc, fp, num, size);
}

// crtd_malloc - Calls to malloc from msvcrtd.dll are patched through to this
//   function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the valued returned from malloc.
//
void* crtd_malloc (size_t size)
{
    static malloc_t pcrtd_malloc = NULL;

    SIZE_T  fp;
    HMODULE msvcrtd;

    FRAMEPOINTER(fp);

    if (pcrtd_malloc == NULL) {
        // This is the first call to this function. Link to the real malloc.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd_malloc = (malloc_t)GetProcAddress(msvcrtd, "malloc");
    }

    return vld._malloc(pcrtd_malloc, fp, size);
}

// crtd_realloc - Calls to realloc from msvcrtd.dll are patched through to this
//   function.
//
//  - mem (IN): Pointer to the memory block to reallocate.
//
//  - size (IN): Size of the memory block to reallocate.
//
//  Return Value:
//
//    Returns the value returned from realloc.
//
void* crtd_realloc (void *mem, size_t size)
{
    static realloc_t pcrtd_realloc = NULL;

    SIZE_T  fp;
    HMODULE msvcrtd;

    FRAMEPOINTER(fp);

    if (pcrtd_realloc == NULL) {
        // This is the first call to this function. Link to the real realloc.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd_realloc = (realloc_t)GetProcAddress(msvcrtd, "realloc");
    }

    return vld._realloc(pcrtd_realloc, fp, mem, size);
}

// crtd_scalar_new - Calls to the CRT's scalar new operator from msvcrtd.dll
//   are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the CRT scalar new operator.
//
void* crtd_scalar_new (unsigned int size)
{
    static new_t pcrtd_scalar_new = NULL;

    SIZE_T  fp;
    HMODULE msvcrtd;

    FRAMEPOINTER(fp);

    if (pcrtd_scalar_new == NULL) {
        // This is the first call to this function. Link to the real CRT scalar
        // new operator.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd_scalar_new = (new_t)GetProcAddress(msvcrtd, "??2@YAPAXI@Z");
    }

    return vld._new(pcrtd_scalar_new, fp, size);
}

// crtd_vector_new - Calls to the CRT's vector new operator from msvcrtd.dll
//   are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the CRT vector new operator.
//
void* crtd_vector_new (unsigned int size)
{
    static new_t pcrtd_vector_new = NULL;

    SIZE_T  fp;
    HMODULE msvcrtd;

    FRAMEPOINTER(fp);

    if (pcrtd_vector_new == NULL) {
        // This is the first call to this function. Link to the real CRT vector
        // new operator.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd_vector_new = (new_t)GetProcAddress(msvcrtd, "??_U@YAPAXI@Z");
    }

    return vld._new(pcrtd_vector_new, fp, size);
}


////////////////////////////////////////////////////////////////////////////////
//
// MFC 4.2 DLLs
//
////////////////////////////////////////////////////////////////////////////////

// mfc42d__scalar_new_dbg - Calls to the MFC debug scalar new operator from
//   mfc42d.dll are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the MFC debug scalar new operator.
//
void* mfc42d__scalar_new_dbg (unsigned int size, char const *file, int line)
{
    static new_dbg_mfc_t pmfc42d__scalar_new_dbg = NULL;

    SIZE_T  fp;
    HMODULE mfc42d;

    FRAMEPOINTER(fp);

    if (pmfc42d__scalar_new_dbg == NULL) {
        // This is the first call to this function. Link to the real MFC debug
        // new operator.
        mfc42d = GetModuleHandle(L"mfc42d.dll");
        pmfc42d__scalar_new_dbg = (new_dbg_mfc_t)GetProcAddress(mfc42d, (LPCSTR)714);
    }

    return vld.new_dbg_mfc(pmfc42d__scalar_new_dbg, fp, size, file, line);
}

// mfc42d_scalar_new - Calls to the MFC scalar new operator from mfc42d.dll are
//   patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the MFC scalar new operator.
//
void* mfc42d_scalar_new (unsigned int size)
{
    static new_t pmfc42d_scalar_new = NULL;

    SIZE_T  fp;
    HMODULE mfc42d;

    FRAMEPOINTER(fp);

    if (pmfc42d_scalar_new == NULL) {
        // This is the first call to this function. Link to the real MFC new
        // operator.
        mfc42d = GetModuleHandle(L"mfc42d.dll");
        pmfc42d_scalar_new = (new_t)GetProcAddress(mfc42d, (LPCSTR)711);
    }

    return vld._new(pmfc42d_scalar_new, fp, size);
}


////////////////////////////////////////////////////////////////////////////////
//
// MFC 8.0 DLLs
//
////////////////////////////////////////////////////////////////////////////////

// mfc80d__scalar_new_dbg - Calls to the MFC debug scalar new operator from
//   mfc80d.dll are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the MFC debug scalar new operator.
//
void* mfc80d__scalar_new_dbg (unsigned int size, char const *file, int line)
{
new_dbg_mfc_t  pmfc80d__scalar_new_dbg = NULL;
    SIZE_T  fp;
    HMODULE mfc80d;

    FRAMEPOINTER(fp);

    if (pmfc80d__scalar_new_dbg == NULL) {
        // This is the first call to this function. Link to the real MFC debug
        // new operator.
        mfc80d = GetModuleHandle(L"mfc80d.dll");
        pmfc80d__scalar_new_dbg = (new_dbg_mfc_t)GetProcAddress(mfc80d, (LPCSTR)895);
    }

    return vld.new_dbg_mfc(pmfc80d__scalar_new_dbg, fp, size, file, line);
}

// mfc80d__vector_new_dbg - Calls to the MFC debug vector new operator from
//   mfc80d.dll are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the MFC debug vector new operator.
//
void* mfc80d__vector_new_dbg (unsigned int size, char const *file, int line)
{
new_dbg_mfc_t  pmfc80d__vector_new_dbg = NULL;
    SIZE_T  fp;
    HMODULE mfc80d;

    FRAMEPOINTER(fp);

    if (pmfc80d__vector_new_dbg == NULL) {
        // This is the first call to this function. Link to the real MFC debug
        // new operator.
        mfc80d = GetModuleHandle(L"mfc80d.dll");
        pmfc80d__vector_new_dbg = (new_dbg_mfc_t)GetProcAddress(mfc80d, (LPCSTR)269);
    }

    return vld.new_dbg_mfc(pmfc80d__vector_new_dbg, fp, size, file, line);
}

// mfc80d_scalar_new - Calls to the MFC scalar new operator from mfc80d.dll are
//   patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the MFC scalar new operator.
//
void* mfc80d_scalar_new (unsigned int size)
{
    static new_t pmfc80d_scalar_new = NULL;

    SIZE_T  fp;
    HMODULE mfc80d;

    FRAMEPOINTER(fp);

    if (pmfc80d_scalar_new == NULL) {
        // This is the first call to this function. Link to the real MFC 8.0 new
        // operator.
        mfc80d = GetModuleHandle(L"mfc80d.dll");
        pmfc80d_scalar_new = (new_t)GetProcAddress(mfc80d, (LPCSTR)893);
    }

    return vld._new(pmfc80d_scalar_new, fp, size);
}

// mfc80d_vector_new - Calls to the MFC vector new operator from mfc80d.dll are
//   patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the MFC vector new operator.
//
void* mfc80d_vector_new (unsigned int size)
{
    static new_t pmfc80d_vector_new = NULL;

    SIZE_T  fp;
    HMODULE mfc80d;

    FRAMEPOINTER(fp);

    if (pmfc80d_vector_new == NULL) {
        // This is the first call to this function. Link to the real MFC 8.0 new
        // operator.
        mfc80d = GetModuleHandle(L"mfc80d.dll");
        pmfc80d_vector_new = (new_t)GetProcAddress(mfc80d, (LPCSTR)267);
    }

    return vld._new(pmfc80d_vector_new, fp, size);
}


////////////////////////////////////////////////////////////////////////////////
//
// MFC 9.0 DLLs
//
////////////////////////////////////////////////////////////////////////////////

// mfc90d__scalar_new_dbg - Calls to the MFC debug scalar new operator from
//   mfc90d.dll are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the MFC debug scalar new operator.
//
void* mfc90d__scalar_new_dbg (unsigned int size, char const *file, int line)
{
    static new_dbg_mfc_t pmfc90d__scalar_new_dbg = NULL;

    SIZE_T  fp;
    HMODULE mfc90d;

    FRAMEPOINTER(fp);

    if (pmfc90d__scalar_new_dbg == NULL) {
        // This is the first call to this function. Link to the real MFC debug
        // new operator.
        mfc90d = GetModuleHandle(L"mfc90d.dll");
        pmfc90d__scalar_new_dbg = (new_dbg_mfc_t)GetProcAddress(mfc90d, (LPCSTR)933);
    }

    return vld.new_dbg_mfc(pmfc90d__scalar_new_dbg, fp, size, file, line);
}

// mfc90d__vector_new_dbg - Calls to the MFC debug vector new operator from
//   mfc90d.dll are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the MFC debug vector new operator.
//
void* mfc90d__vector_new_dbg (unsigned int size, char const *file, int line)
{
    static new_dbg_mfc_t pmfc90d__vector_new_dbg = NULL;

    SIZE_T  fp;
    HMODULE mfc90d;

    FRAMEPOINTER(fp);

    if (pmfc90d__vector_new_dbg == NULL) {
        // This is the first call to this function. Link to the real MFC debug
        // new operator.
        mfc90d = GetModuleHandle(L"mfc90d.dll");
        pmfc90d__vector_new_dbg = (new_dbg_mfc_t)GetProcAddress(mfc90d, (LPCSTR)269);
    }

    return vld.new_dbg_mfc(pmfc90d__vector_new_dbg, fp, size, file, line);
}

// mfc90d_scalar_new - Calls to the MFC scalar new operator from mfc90d.dll are
//   patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the MFC scalar new operator.
//
void* mfc90d_scalar_new (unsigned int size)
{
    static new_t pmfc90d_scalar_new = NULL;

    SIZE_T  fp;
    HMODULE mfc90d;

    FRAMEPOINTER(fp);

    if (pmfc90d_scalar_new == NULL) {
        // This is the first call to this function. Link to the real MFC 8.0 new
        // operator.
        mfc90d = GetModuleHandle(L"mfc90d.dll");
        pmfc90d_scalar_new = (new_t)GetProcAddress(mfc90d, (LPCSTR)931);
    }

    return vld._new(pmfc90d_scalar_new, fp, size);
}

// mfc90d_vector_new - Calls to the MFC vector new operator from mfc90d.dll are
//   patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the MFC vector new operator.
//
void* mfc90d_vector_new (unsigned int size)
{
    static new_t pmfc90d_vector_new = NULL;

    SIZE_T  fp;
    HMODULE mfc90d;

    FRAMEPOINTER(fp);

    if (pmfc90d_vector_new == NULL) {
        // This is the first call to this function. Link to the real MFC 8.0 new
        // operator.
        mfc90d = GetModuleHandle(L"mfc90d.dll");
        pmfc90d_vector_new = (new_t)GetProcAddress(mfc90d, (LPCSTR)267);
    }

    return vld._new(pmfc90d_vector_new, fp, size);
}
