////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - CRT and MFC IAT Patch Functions Header
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

#pragma once

#ifndef VLDBUILD
#error \
"This header should only be included by Visual Leak Detector when building it from source. \
Applications should never include this header."
#endif

#include "vldint.h"
extern __declspec(dllexport) VisualLeakDetector vld;

#define TEMPLATE_HEADER \
template<wchar_t const *crtddll, wchar_t const *mfcddll, wchar_t const *mfcuddll,\
    char const *crtd_vector_new_name, char const *crtd_vector_new_dbg_name,\
    char const *crtd_scalar_new_name, char const *crtd_scalar_new_dbg_name,\
    int mfcd_vector_new_ordinal, int mfcd_vector_new_dbg_4p_ordinal, int mfcd_vector_new_dbg_3p_ordinal,\
    int mfcd_scalar_new_ordinal, int mfcd_scalar_new_dbg_4p_ordinal, int mfcd_scalar_new_dbg_3p_ordinal,\
    int mfcud_vector_new_ordinal, int mfcud_vector_new_dbg_4p_ordinal, int mfcud_vector_new_dbg_3p_ordinal,\
    int mfcud_scalar_new_ordinal, int mfcud_scalar_new_dbg_4p_ordinal, int mfcud_scalar_new_dbg_3p_ordinal>

#define TEMPLATE_ARGS \
    crtddll, mfcddll, mfcuddll,\
    crtd_vector_new_name, crtd_vector_new_dbg_name,\
    crtd_scalar_new_name, crtd_scalar_new_dbg_name,\
    mfcd_vector_new_ordinal, mfcd_vector_new_dbg_4p_ordinal, mfcd_vector_new_dbg_3p_ordinal,\
    mfcd_scalar_new_ordinal, mfcd_scalar_new_dbg_4p_ordinal, mfcd_scalar_new_dbg_3p_ordinal,\
    mfcud_vector_new_ordinal, mfcud_vector_new_dbg_4p_ordinal, mfcud_vector_new_dbg_3p_ordinal,\
    mfcud_scalar_new_ordinal, mfcud_scalar_new_dbg_4p_ordinal, mfcud_scalar_new_dbg_3p_ordinal

TEMPLATE_HEADER
class CrtMfcPatch
{
public:
    static void* __cdecl crtd__calloc_dbg (size_t num, size_t size, int type, char const *file, int line);
    static void* __cdecl crtd__malloc_dbg (size_t size, int type, const char *file, int line);
    static void* __cdecl crtd__realloc_dbg (void *mem, size_t size, int type, char const *file, int line);
    static void* __cdecl crtd__scalar_new_dbg (size_t size, int type, char const *file, int line);
    static void* __cdecl crtd__vector_new_dbg (size_t size, int type, char const *file, int line);
    static void* __cdecl crtd_calloc (size_t num, size_t size);
    static void* __cdecl crtd_malloc (size_t size);
    static void* __cdecl crtd_realloc (void *mem, size_t size);
    static void* __cdecl crtd_scalar_new (size_t size);
    static void* __cdecl crtd_vector_new (size_t size);

    template<char const *procname>
    static void* __cdecl crtd_new_dbg (context_t& context, size_t size, int type, char const *file, int line);
    template<char const *procname>
    static void* __cdecl crtd_new (context_t& context, size_t size);

    static void* __cdecl mfcd_vector_new (size_t size);
    static void* __cdecl mfcd__vector_new_dbg_4p (size_t size, int type, char const *file, int line);
    static void* __cdecl mfcd__vector_new_dbg_3p (size_t size, char const *file, int line);
    static void* __cdecl mfcd_scalar_new (size_t size);
    static void* __cdecl mfcd__scalar_new_dbg_4p (size_t size, int type, char const *file, int line);
    static void* __cdecl mfcd__scalar_new_dbg_3p (size_t size, char const *file, int line);
    static void* __cdecl mfcud_vector_new (size_t size);
    static void* __cdecl mfcud__vector_new_dbg_4p (size_t size, int type, char const *file, int line);
    static void* __cdecl mfcud__vector_new_dbg_3p (size_t size, char const *file, int line);
    static void* __cdecl mfcud_scalar_new (size_t size);
    static void* __cdecl mfcud__scalar_new_dbg_4p (size_t size, int type, char const *file, int line);
    static void* __cdecl mfcud__scalar_new_dbg_3p (size_t size, char const *file, int line);

    template<wchar_t const *mfcdll, int ordinal>
    static void* __cdecl mfcd_new_dbg (context_t& context, size_t size, int type, char const *file, int line);
    template<wchar_t const *mfcdll, int ordinal>
    static void* __cdecl mfcd_new_dbg (context_t& context, size_t size, char const *file, int line);
    template<wchar_t const *mfcdll, int ordinal>
    static void* __cdecl mfcd_new (context_t& context, size_t size);
};


////////////////////////////////////////////////////////////////////////////////
//
// Visual Studio DLLs
//
////////////////////////////////////////////////////////////////////////////////

// crtd__calloc_dbg - Calls to _calloc_dbg from msvcrXXd.dll are patched
//   through to this function.
//
//  - mem (IN): Pointer to the memory block to be reallocated.
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
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::crtd__calloc_dbg (size_t      num,
                                                    size_t      size,
                                                    int         type,
                                                    char const *file,
                                                    int         line)
{
    static _calloc_dbg_t pcrtxxd__calloc_dbg = NULL;

    context_t context;
    HMODULE msvcrxxd;

    CAPTURE_CONTEXT(context);

    if (pcrtxxd__calloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _calloc_dbg.
        msvcrxxd = GetModuleHandle(crtddll);
        pcrtxxd__calloc_dbg = (_calloc_dbg_t)GetProcAddress(msvcrxxd, "_calloc_dbg");
    }

    return vld.__calloc_dbg(pcrtxxd__calloc_dbg, context, num, size, type, file, line);
}

// crtd__malloc_dbg - Calls to _malloc_dbg from msvcrXXd.dll are patched
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
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::crtd__malloc_dbg (size_t      size,
                                                    int         type,
                                                    char const *file,
                                                    int         line)
{
    static _malloc_dbg_t pcrtxxd__malloc_dbg = NULL;

	context_t context;
    HMODULE msvcrxxd;

    CAPTURE_CONTEXT(context);

    if (pcrtxxd__malloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _malloc_dbg.
        msvcrxxd = GetModuleHandle(crtddll);
        pcrtxxd__malloc_dbg = (_malloc_dbg_t)GetProcAddress(msvcrxxd, "_malloc_dbg");
    }

    return vld.__malloc_dbg(pcrtxxd__malloc_dbg, context, size, type, file, line);
}

// crtd__realloc_dbg - Calls to _realloc_dbg from msvcrXXd.dll are patched
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
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _realloc_dbg.
//
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::crtd__realloc_dbg (void       *mem,
                                                     size_t     size,
                                                     int        type,
                                                     char const *file,
                                                     int        line)
{
    static _realloc_dbg_t pcrtxxd__realloc_dbg = NULL;

	context_t context;
    HMODULE msvcrxxd;

    CAPTURE_CONTEXT(context);

    if (pcrtxxd__realloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _realloc_dbg.
        msvcrxxd = GetModuleHandle(crtddll);
        pcrtxxd__realloc_dbg = (_realloc_dbg_t)GetProcAddress(msvcrxxd, "_realloc_dbg");
    }

    return vld.__realloc_dbg(pcrtxxd__realloc_dbg, context, mem, size, type, file, line);
}

// crtd__scalar_new_dbg - Calls to the CRT's debug scalar new operator from
//   msvcrXXd.dll are patched through to this function.
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
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::crtd__scalar_new_dbg (size_t      size,
                                                        int         type,
                                                        char const *file,
                                                        int         line)
{
	context_t context;
    CAPTURE_CONTEXT(context);

    return crtd_new_dbg<crtd_scalar_new_dbg_name>(context, size, type, file, line);
}

// crtd__vector_new_dbg - Calls to the CRT's debug vector new operator from
//   msvcrXXd.dll are patched through to this function.
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
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::crtd__vector_new_dbg (size_t      size,
                                                        int         type,
                                                        char const *file,
                                                        int         line)
{
    context_t context;
    CAPTURE_CONTEXT(context);

    return crtd_new_dbg<crtd_vector_new_dbg_name>(context, size, type, file, line);
}

// crtd_calloc - Calls to calloc from msvcrXXd.dll are patched through to
//   this function.
//
//  - dll (IN): The name of the dll
//
//  - num (IN): The number of blocks, of size 'size', to be allocated.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the valued returned from calloc.
//
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::crtd_calloc (size_t num, size_t size)
{
    static calloc_t pcrtxxd_calloc = NULL;

	context_t context;
    HMODULE msvcrxxd;

    CAPTURE_CONTEXT(context);

    if (pcrtxxd_calloc == NULL) {
        // This is the first call to this function. Link to the real malloc.
        msvcrxxd = GetModuleHandle(crtddll);
        pcrtxxd_calloc = (calloc_t)GetProcAddress(msvcrxxd, "calloc");
    }

    return vld._calloc(pcrtxxd_calloc, context, num, size);
}

// crtd_malloc - Calls to malloc from msvcrXXd.dll are patched through to
//   this function.
//
//  - dll (IN): The name of the dll
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the valued returned from malloc.
//
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::crtd_malloc (size_t size)
{
    static malloc_t pcrtxxd_malloc = NULL;

	context_t context;
    HMODULE msvcrxxd;

    CAPTURE_CONTEXT(context);

    if (pcrtxxd_malloc == NULL) {
        // This is the first call to this function. Link to the real malloc.
        msvcrxxd = GetModuleHandle(crtddll);
        pcrtxxd_malloc = (malloc_t)GetProcAddress(msvcrxxd, "malloc");
    }

    return vld._malloc(pcrtxxd_malloc, context, size);
}

// crtd_realloc - Calls to realloc from msvcrXXd.dll are patched through to
//   this function.
//
//  - dll (IN): The name of the dll
//
//  - mem (IN): Pointer to the memory block to reallocate.
//
//  - size (IN): Size of the memory block to reallocate.
//
//  Return Value:
//
//    Returns the value returned from realloc.
//
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::crtd_realloc (void *mem, size_t size)
{
    static realloc_t pcrtxxd_realloc = NULL;

	context_t context;
    HMODULE msvcrxxd;

    CAPTURE_CONTEXT(context);

    if (pcrtxxd_realloc == NULL) {
        // This is the first call to this function. Link to the real realloc.
        msvcrxxd = GetModuleHandle(crtddll);
        pcrtxxd_realloc = (realloc_t)GetProcAddress(msvcrxxd, "realloc");
    }

    return vld._realloc(pcrtxxd_realloc, context, mem, size);
}

// crtd_scalar_new - Calls to the CRT's scalar new operator from msvcrXXd.dll
//   are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the CRT scalar new operator.
//
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::crtd_scalar_new (size_t size)
{
	context_t context;
    CAPTURE_CONTEXT(context);

    return crtd_new<crtd_scalar_new_name>(context, size);
}

// crtd_vector_new - Calls to the CRT's vector new operator from msvcrXXd.dll
//   are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the CRT vector new operator.
//
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::crtd_vector_new (size_t size)
{
    context_t context;
    CAPTURE_CONTEXT(context);

    return crtd_new<crtd_vector_new_name>(context, size);
}

// crtd_new_dbg - A template function for implementation of patch functions to
//   the CRT's debug new operator from msvcrXXd.dll
//
//  - procname (IN): The debug new operator's name
//
//  - fp (IN): Frame pointer from the call that initiated this allocation.
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
//    Returns the value returned by the CRT debug new operator.
//
TEMPLATE_HEADER
template<char const *procname>
void* CrtMfcPatch<TEMPLATE_ARGS>::crtd_new_dbg (context_t&  context,
                                                size_t      size,
                                                int         type,
                                                char const *file,
                                                int         line)
{
    static new_dbg_crt_t pcrtxxd_new_dbg = NULL;

    HMODULE msvcrxxd;

    if (pcrtxxd_new_dbg == NULL) {
        // This is the first call to this function. Link to the real CRT debug
        // new operator.
        msvcrxxd = GetModuleHandle(crtddll);
        pcrtxxd_new_dbg = (new_dbg_crt_t)GetProcAddress(msvcrxxd, procname);
    }

    return vld.new_dbg_crt(pcrtxxd_new_dbg, context, size, type, file, line);
}

// crt_new - A template function for implementing patch functions to the
//   CRT's new operator from msvcrXXd.dll
//
//  - dll (IN): The name of the dll
//
//  - procname (IN): The debug new operator's name
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the CRT new operator.
//
TEMPLATE_HEADER
template<char const *procname>
void* CrtMfcPatch<TEMPLATE_ARGS>::crtd_new (context_t& context, size_t size)
{
    static new_t pcrtxxd_scalar_new = NULL;

    HMODULE msvcrxxd;

    if (pcrtxxd_scalar_new == NULL) {
        // This is the first call to this function. Link to the real CRT new
        // operator.
        msvcrxxd = GetModuleHandle(crtddll);
        pcrtxxd_scalar_new = (new_t)GetProcAddress(msvcrxxd, procname);
    }

    return vld._new(pcrtxxd_scalar_new, context, size);
}

////////////////////////////////////////////////////////////////////////////////
//
// MFC DLLs
//
////////////////////////////////////////////////////////////////////////////////

// mfcd__scalar_new_dbg_3p - Calls to the MFC debug scalar new operator from
//   mfcXXd.dll are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The "use type" of the block to be allocated.
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
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::mfcd__scalar_new_dbg_4p (size_t       size,
                                                           int          type,
                                                           char const  *file,
                                                           int          line)
{
	context_t context;
    CAPTURE_CONTEXT(context);

    return mfcd_new_dbg<mfcddll, mfcd_scalar_new_dbg_4p_ordinal>
                       (context, size, type, file, line);
}

// mfcd__scalar_new_dbg_3p - Calls to the MFC debug scalar new operator from
//   mfcXXd.dll are patched through to this function.
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
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::mfcd__scalar_new_dbg_3p (size_t       size,
                                                           char const  *file,
                                                           int          line)
{
	context_t context;	
    CAPTURE_CONTEXT(context);

    return mfcd_new_dbg<mfcddll, mfcd_scalar_new_dbg_3p_ordinal>
                       (context, size, file, line);
}

// mfcd__vector_new_dbg_4p - Calls to the MFC debug vector new operator from
//   mfcXXd.dll are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The "use type" of the block to be allocated.
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
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::mfcd__vector_new_dbg_4p (size_t       size,
                                                           int          type,
                                                           char const  *file,
                                                           int          line)
{
    context_t context;
    CAPTURE_CONTEXT(context);

    return mfcd_new_dbg<mfcddll, mfcd_vector_new_dbg_4p_ordinal>
                       (context, size, type, file, line);
}

// mfcd__vector_new_dbg_3p - Calls to the MFC debug vector new operator from
//   mfcXXd.dll are patched through to this function.
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
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::mfcd__vector_new_dbg_3p (size_t       size,
                                                           char const  *file,
                                                           int          line)
{
    context_t context;
    CAPTURE_CONTEXT(context);

    return mfcd_new_dbg<mfcddll, mfcd_vector_new_dbg_3p_ordinal>
                       (context, size, file, line);
}

// mfcd_scalar_new - Calls to the MFC scalar new operator from mfcXXd.dll are
//   patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the MFC scalar new operator.
//
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::mfcd_scalar_new (size_t size)
{
	context_t context;
    CAPTURE_CONTEXT(context);

    return mfcd_new<mfcddll, mfcd_scalar_new_ordinal>(context, size);
}

// mfcd_vector_new - Calls to the MFC vector new operator from mfcXXd.dll are
//   patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the MFC vector new operator.
//
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::mfcd_vector_new (size_t size)
{
    context_t context;
    CAPTURE_CONTEXT(context);

    return mfcd_new<mfcddll, mfcd_vector_new_ordinal>(context, size);
}

// mfcud__scalar_new_dbg_4p - Calls to the MFC debug scalar new operator from
//   mfcXXud.dll are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The "use type" of the block to be allocated.
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
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::mfcud__scalar_new_dbg_4p (size_t      size,
                                                            int         type,
                                                            char const *file,
                                                            int         line)
{
    context_t context;
    CAPTURE_CONTEXT(context);

    return mfcd_new_dbg<mfcuddll, mfcud_scalar_new_dbg_4p_ordinal>
                       (context, size, type, file, line);
}

// mfcud__scalar_new_dbg_3p - Calls to the MFC debug scalar new operator from
//   mfcXXud.dll are patched through to this function.
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
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::mfcud__scalar_new_dbg_3p (size_t      size,
                                                            char const *file,
                                                            int         line)
{
    context_t context;
    CAPTURE_CONTEXT(context);

    return mfcd_new_dbg<mfcuddll, mfcud_scalar_new_dbg_3p_ordinal>
                       (context, size, file, line);
}

// mfcud__vector_new_dbg_4p - Calls to the MFC debug vector new operator from
//   mfcXXud.dll are patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The "use type" of the block to be allocated.
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
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::mfcud__vector_new_dbg_4p (size_t      size,
                                                            int         type,
                                                            char const *file,
                                                            int         line)
{
    context_t context;
    CAPTURE_CONTEXT(context);

    return mfcd_new_dbg<mfcuddll, mfcud_vector_new_dbg_4p_ordinal>
                       (context, size, type, file, line);
}

// mfcud__vector_new_dbg_3p - Calls to the MFC debug vector new operator from
//   mfcXXud.dll are patched through to this function.
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
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::mfcud__vector_new_dbg_3p (size_t      size,
                                                            char const *file,
                                                            int         line)
{
    context_t context;
    CAPTURE_CONTEXT(context);

    return mfcd_new_dbg<mfcuddll, mfcud_vector_new_dbg_3p_ordinal>
                       (context, size, file, line);
}

// mfcud_scalar_new - Calls to the MFC scalar new operator from mfcXXud.dll are
//   patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the MFC scalar new operator.
//
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::mfcud_scalar_new (size_t size)
{
    context_t context;
    CAPTURE_CONTEXT(context);

    return mfcd_new<mfcuddll, mfcud_scalar_new_ordinal>(context, size);
}

// mfcud_vector_new - Calls to the MFC vector new operator from mfcXXud.dll are
//   patched through to this function.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the MFC vector new operator.
//
TEMPLATE_HEADER
void* CrtMfcPatch<TEMPLATE_ARGS>::mfcud_vector_new (size_t size)
{
    context_t context;
    CAPTURE_CONTEXT(context);

    return mfcd_new<mfcuddll, mfcud_vector_new_ordinal>(context, size);
}

// mfcd_new_dbg - A generic function for implementing patch functions to the MFC
//   debug new operators:
//   void* __cdecl operator new[](size_t size, int type, char const *file, int line)
//   void* __cdecl operator new(size_t size, int type, char const *file, int line)
//
//  - mfcdll (IN): The name of the MFC DLL
//
//  - ordinal (IN): The debug new operator's ordinal value
//
//  - type (IN): The "use type" of the block to be allocated.
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
//    Returns the value returned by the MFC debug new operator.
//
TEMPLATE_HEADER
template<wchar_t const *mfcdll, int ordinal>
void* CrtMfcPatch<TEMPLATE_ARGS>::mfcd_new_dbg (context_t& context,
                                                size_t      size,
                                                int         type,
                                                char const *file,
                                                int         line)
{
    static new_dbg_crt_t pmfcxxd__new_dbg = NULL;

    HMODULE mfcxxd;

    if (pmfcxxd__new_dbg == NULL) {
        // This is the first call to this function. Link to the real MFC debug
        // new operator.
        mfcxxd = GetModuleHandle(mfcdll);
        pmfcxxd__new_dbg = (new_dbg_crt_t)GetProcAddress(mfcxxd, (LPCSTR)ordinal);
    }

    return vld.new_dbg_mfc(pmfcxxd__new_dbg, context, size, type, file, line);
}

// mfcd_new_dbg - A generic function for implementing patch functions to the MFC
//   debug new operators:
//   void* __cdecl operator new[](size_t size, char const *file, int line)
//   void* __cdecl operator new(size_t size, char const *file, int line)
//
//  - mfcdll (IN): The name of the MFC DLL
//
//  - ordinal (IN): The debug new operator's ordinal value
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
//    Returns the value returned by the MFC debug new operator.
//
TEMPLATE_HEADER
template<wchar_t const *mfcdll, int ordinal>
void* CrtMfcPatch<TEMPLATE_ARGS>::mfcd_new_dbg (context_t& context,
                                                size_t      size,
                                                char const *file,
                                                int         line)
{
    static new_dbg_mfc_t pmfcxxd__new_dbg = NULL;

    HMODULE mfcxxd;

    if (pmfcxxd__new_dbg == NULL) {
        // This is the first call to this function. Link to the real MFC debug
        // new operator.
        mfcxxd = GetModuleHandle(mfcdll);
        pmfcxxd__new_dbg = (new_dbg_mfc_t)GetProcAddress(mfcxxd, (LPCSTR)ordinal);
    }

    return vld.new_dbg_mfc(pmfcxxd__new_dbg, context, size, file, line);
}

// mfcd_new - A generic function for implementing patch functions to the MFC new
//   operators.
//
//  - mfcdll (IN): The name of the MFC DLL
//
//  - ordinal (IN): The new operator's ordinal value
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the MFC new operator.
//
TEMPLATE_HEADER
template<wchar_t const *mfcdll, int ordinal>
void* CrtMfcPatch<TEMPLATE_ARGS>::mfcd_new (context_t& context, size_t size)
{
    static new_t pmfcxxd_new = NULL;

    HMODULE mfcxxd;

    if (pmfcxxd_new == NULL) {
        // This is the first call to this function. Link to the real CRT new
        // operator.
        mfcxxd = GetModuleHandle(mfcdll);
        pmfcxxd_new = (new_t)GetProcAddress(mfcxxd, (LPCSTR)ordinal);
    }

    return vld._new(pmfcxxd_new, context, size);
}

#undef TEMPLATE_HEADER
#undef TEMPLATE_ARGS

#ifndef WIN64
//void * __cdecl operator new(unsigned int,int,char const *,int)
const extern char    scalar_new_dbg_name[] = "??2@YAPAXIHPBDH@Z";
//void * __cdecl operator new[](unsigned int,int,char const *,int)
const extern char    vector_new_dbg_name[] = "??_U@YAPAXIHPBDH@Z";
//void * __cdecl operator new(unsigned int)
const extern char    scalar_new_name[] = "??2@YAPAXI@Z";
//void * __cdecl operator new[](unsigned int)
const extern char    vector_new_name[] = "??_U@YAPAXI@Z";
#else
//void * __ptr64 __cdecl operator new(unsigned __int64,int,char const * __ptr64,int)
const extern char    scalar_new_dbg_name[] = "??2@YAPEAX_KHPEBDH@Z";
//void * __ptr64 __cdecl operator new[](unsigned __int64,int,char const * __ptr64,int)
const extern char    vector_new_dbg_name[] = "??_U@YAPEAX_KHPEBDH@Z";
//void * __ptr64 __cdecl operator new(unsigned __int64)
const extern char    scalar_new_name[] = "??2@YAPEAX_K@Z";
//void * __ptr64 __cdecl operator new[](unsigned __int64)
const extern char    vector_new_name[] = "??_U@YAPEAX_K@Z";
#endif

const extern wchar_t msvcrtd_dll[]  = L"msvcrtd.dll";
const extern wchar_t mfc42d_dll[]   = L"mfc42d.dll";
const extern wchar_t mfc42ud_dll[]  = L"mfc42ud.dll";
const extern wchar_t msvcr70d_dll[] = L"msvcr70d.dll";
const extern wchar_t mfc70d_dll[]   = L"mfc70d.dll";
const extern wchar_t mfc70ud_dll[]  = L"mfc70ud.dll";
const extern wchar_t msvcr71d_dll[] = L"msvcr71d.dll";
const extern wchar_t mfc71d_dll[]   = L"mfc71d.dll";
const extern wchar_t mfc71ud_dll[]  = L"mfc71ud.dll";
const extern wchar_t msvcr80d_dll[] = L"msvcr80d.dll";
const extern wchar_t mfc80d_dll[]   = L"mfc80d.dll";
const extern wchar_t mfc80ud_dll[]  = L"mfc80ud.dll";
const extern wchar_t msvcr90d_dll[] = L"msvcr90d.dll";
const extern wchar_t mfc90d_dll[]   = L"mfc90d.dll";
const extern wchar_t mfc90ud_dll[]  = L"mfc90ud.dll";
const extern wchar_t msvcr100d_dll[] = L"msvcr100d.dll";
const extern wchar_t mfc100d_dll[]   = L"mfc100d.dll";
const extern wchar_t mfc100ud_dll[]  = L"mfc100ud.dll";

// Visual Studio 6.0
typedef CrtMfcPatch<msvcrtd_dll, mfc42d_dll, mfc42ud_dll,
                    vector_new_name, vector_new_dbg_name,
                    scalar_new_name, scalar_new_dbg_name,
                    0, 0, 0, 711, 712, 714,
                    0, 0, 0, 711, 712, 714>
        VS60;
// Visual Studio .NET 2002
typedef CrtMfcPatch<msvcr70d_dll, mfc70d_dll, mfc70ud_dll,
                    vector_new_name, vector_new_dbg_name,
                    scalar_new_name, scalar_new_dbg_name,
                    257, 258, 259, 832, 833, 834,
                    258, 259, 260, 833, 834, 835>
        VS70;
// Visual Studio .NET 2003
typedef CrtMfcPatch<msvcr71d_dll, mfc71d_dll, mfc71ud_dll,
                    vector_new_name, vector_new_dbg_name,
                    scalar_new_name, scalar_new_dbg_name,
                    267, 268, 269, 893, 894, 895,
                    267, 268, 269, 893, 894, 895>
        VS71;
// Visual Studio 2005
typedef CrtMfcPatch<msvcr80d_dll, mfc80d_dll, mfc80ud_dll,
                    vector_new_name, vector_new_dbg_name,
                    scalar_new_name, scalar_new_dbg_name,
                    267, 268, 269, 893, 894, 895,
                    267, 268, 269, 893, 894, 895>
        VS80;
// Visual Studio 2008
typedef CrtMfcPatch<msvcr90d_dll, mfc90d_dll, mfc90ud_dll,
                    vector_new_name, vector_new_dbg_name,
                    scalar_new_name, scalar_new_dbg_name,
                    267, 268, 269, 931, 932, 933,
                    267, 268, 269, 935, 936, 937>
        VS90;
// Visual Studio 2010
typedef CrtMfcPatch<msvcr100d_dll, mfc100d_dll, mfc100ud_dll,
                    vector_new_name, vector_new_dbg_name,
                     scalar_new_name, scalar_new_dbg_name,
                    267, 268, 269, 1427, 1428, 1429,
                    267, 268, 269, 1434, 1435, 1436>
        VS100;
