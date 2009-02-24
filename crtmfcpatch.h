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

// Visual Studio 2005 DLLs
void* __cdecl crt80d__calloc_dbg (size_t num, size_t size, int type, char const *file, int line);
void* __cdecl crt80d__malloc_dbg (size_t size, int type, const char *file, int line);
void* __cdecl crt80d__realloc_dbg (void *mem, size_t size, int type, char const *file, int line);
void* __cdecl crt80d__scalar_new_dbg (unsigned int size, int type, char const *file, int line);
void* __cdecl crt80d__vector_new_dbg (unsigned int size, int type, char const *file, int line);
void* __cdecl crt80d_calloc (size_t num, size_t size);
void* __cdecl crt80d_malloc (size_t size);
void* __cdecl crt80d_realloc (void *mem, size_t size);
void* __cdecl crt80d_scalar_new (unsigned int size);
void* __cdecl crt80d_vector_new (unsigned int size);

// Visual Studio 2008 DLLs
void* __cdecl crt90d__calloc_dbg (size_t num, size_t size, int type, char const *file, int line);
void* __cdecl crt90d__malloc_dbg (size_t size, int type, const char *file, int line);
void* __cdecl crt90d__realloc_dbg (void *mem, size_t size, int type, char const *file, int line);
void* __cdecl crt90d__scalar_new_dbg (unsigned int size, int type, char const *file, int line);
void* __cdecl crt90d__vector_new_dbg (unsigned int size, int type, char const *file, int line);
void* __cdecl crt90d_calloc (size_t num, size_t size);
void* __cdecl crt90d_malloc (size_t size);
void* __cdecl crt90d_realloc (void *mem, size_t size);
void* __cdecl crt90d_scalar_new (unsigned int size);
void* __cdecl crt90d_vector_new (unsigned int size);

// Visual Studio 6.0 DLLs
void* __cdecl crtd__calloc_dbg (size_t num, size_t size, int type, char const *file, int line);
void* __cdecl crtd__malloc_dbg (size_t size, int type, char const *file, int line);
void* __cdecl crtd__realloc_dbg (void *mem, size_t size, int type, char const *file, int line);
void* __cdecl crtd__scalar_new_dbg (unsigned int size, int type, char const *file, int line);
void* __cdecl crtd__vector_new_dbg (unsigned int size, int type, char const *file, int line);
void* __cdecl crtd_calloc (size_t num, size_t size);
void* __cdecl crtd_malloc (size_t size);
void* __cdecl crtd_realloc (void *mem, size_t size);
void* __cdecl crtd_scalar_new (unsigned int size);
void* __cdecl crtd_vector_new (unsigned int size);

// MFC 4.2 DLLs
void* __cdecl mfc42d__scalar_new_dbg (unsigned int size, char const *file, int line);
void* __cdecl mfc42d_scalar_new (unsigned int size);

// MFC 8.0 DLLs
void* __cdecl mfc80d__scalar_new_dbg (unsigned int size, char const *file, int line);
void* __cdecl mfc80d__vector_new_dbg (unsigned int size, char const *file, int line);
void* __cdecl mfc80d_scalar_new (unsigned int size);
void* __cdecl mfc80d_vector_new (unsigned int size);

// MFC 9.0 DLLs
void* __cdecl mfc90d__scalar_new_dbg (unsigned int size, char const *file, int line);
void* __cdecl mfc90d__vector_new_dbg (unsigned int size, char const *file, int line);
void* __cdecl mfc90d_scalar_new (unsigned int size);
void* __cdecl mfc90d_vector_new (unsigned int size);
