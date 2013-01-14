////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - VisualLeakDetector Class Implementation
//  Copyright (c) 2005-2013 VLD Team
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

#define VLDBUILD         // Declares that we are building Visual Leak Detector.
#include "crtmfcpatch.h" // Provides CRT and MFC patch functions.
#include "vldint.h"      // Provides access to the Visual Leak Detector internals.

#ifndef WIN64
//void * __cdecl operator new(unsigned int,int,char const *,int)
const char    scalar_new_dbg_name[] = "??2@YAPAXIHPBDH@Z";
//void * __cdecl operator new[](unsigned int,int,char const *,int)
const char    vector_new_dbg_name[] = "??_U@YAPAXIHPBDH@Z";
//void * __cdecl operator new(unsigned int)
const char    scalar_new_name[] = "??2@YAPAXI@Z";
//void * __cdecl operator new[](unsigned int)
const char    vector_new_name[] = "??_U@YAPAXI@Z";
#else
//void * __ptr64 __cdecl operator new(unsigned __int64,int,char const * __ptr64,int)
const char    scalar_new_dbg_name[] = "??2@YAPEAX_KHPEBDH@Z";
//void * __ptr64 __cdecl operator new[](unsigned __int64,int,char const * __ptr64,int)
const char    vector_new_dbg_name[] = "??_U@YAPEAX_KHPEBDH@Z";
//void * __ptr64 __cdecl operator new(unsigned __int64)
const char    scalar_new_name[] = "??2@YAPEAX_K@Z";
//void * __ptr64 __cdecl operator new[](unsigned __int64)
const char    vector_new_name[] = "??_U@YAPEAX_K@Z";
#endif

// Global function pointers for explicit dynamic linking with functions listed
// in the import patch table. Using explicit dynamic linking minimizes VLD's
// footprint by loading only modules that are actually used. These pointers will
// be linked to the real functions the first time they are used.

// The import patch table: lists the heap-related API imports that VLD patches
// through to replacement functions provided by VLD. Having this table simply
// makes it more convenient to add additional IAT patches.
patchentry_t VisualLeakDetector::m_kernelbasePatch [] = {
    "GetProcAddress",     NULL, _GetProcAddress, // Not heap related, but can be used to obtain pointers to heap functions.
    NULL,                 NULL, NULL
};

patchentry_t VisualLeakDetector::m_kernel32Patch [] = {
    "HeapAlloc",          NULL, _HeapAlloc,
    "HeapCreate",         NULL, _HeapCreate,
    "HeapDestroy",        NULL, _HeapDestroy,
    "HeapFree",           NULL, _HeapFree,
    "HeapReAlloc",        NULL, _HeapReAlloc,
    NULL,                 NULL, NULL
};

#define ORDINAL(x)          (LPCSTR)x
#if !defined(_M_X64)
#define ORDINAL2(x86, x64)  (LPCSTR)x86
#else
#define ORDINAL2(x86, x64)  (LPCSTR)x64
#endif

VisualLeakDetector::_GetProcAddressType *VisualLeakDetector::m_original_GetProcAddress = NULL;

static patchentry_t mfc42Patch [] = {
    // XXX why are the vector new operators missing for mfc42.dll?
    //ORDINAL(711),         &VS60::pmfcd_scalar_new,              VS60::mfcd_scalar_new,
    NULL,                 NULL,                                 NULL
};

static patchentry_t mfc42dPatch [] = {
    // XXX why are the vector new operators missing for mfc42d.dll?
    ORDINAL(711),         &VS60d::pmfcd_scalar_new,             VS60d::mfcd_scalar_new,
    ORDINAL(712),         &VS60d::pmfcd__scalar_new_dbg_4p,     VS60d::mfcd__scalar_new_dbg_4p,
    ORDINAL(714),         &VS60d::pmfcd__scalar_new_dbg_3p,     VS60d::mfcd__scalar_new_dbg_3p,
    NULL,                 NULL,                                 NULL
};

static patchentry_t mfc42uPatch [] = {
    // XXX why are the vector new operators missing for mfc42u.dll?
    //ORDINAL(711),         &VS60::pmfcud_scalar_new,		        VS60::mfcud_scalar_new,
    NULL,                 NULL,                                 NULL
};

static patchentry_t mfc42udPatch [] = {
    // XXX why are the vector new operators missing for mfc42ud.dll?
    ORDINAL(711),         &VS60d::pmfcud_scalar_new,		    VS60d::mfcud_scalar_new,
    ORDINAL(712),         &VS60d::pmfcud__scalar_new_dbg_4p,    VS60d::mfcud__scalar_new_dbg_4p,
    ORDINAL(714),         &VS60d::pmfcud__scalar_new_dbg_3p,    VS60d::mfcud__scalar_new_dbg_3p,
    NULL,                 NULL,                                 NULL
};

static patchentry_t mfc70Patch [] = {
    //ORDINAL(257),         &VS70::pmfcd_vector_new,		        VS70::mfcd_vector_new,
    //ORDINAL(832),         &VS70::pmfcd_scalar_new,		        VS70::mfcd_scalar_new,
    NULL,                 NULL,                                 NULL
};															    
                                                                
static patchentry_t mfc70dPatch [] = {						    
    ORDINAL(257),         &VS70d::pmfcd_vector_new,		        VS70d::mfcd_vector_new,
    ORDINAL(258),         &VS70d::pmfcd__vector_new_dbg_4p,     VS70d::mfcd__vector_new_dbg_4p,
    ORDINAL(259),         &VS70d::pmfcd__vector_new_dbg_3p,     VS70d::mfcd__vector_new_dbg_3p,
    ORDINAL(832),         &VS70d::pmfcd_scalar_new,		        VS70d::mfcd_scalar_new,
    ORDINAL(833),         &VS70d::pmfcd__scalar_new_dbg_4p,     VS70d::mfcd__scalar_new_dbg_4p,
    ORDINAL(834),         &VS70d::pmfcd__scalar_new_dbg_3p,     VS70d::mfcd__scalar_new_dbg_3p,
    NULL,                 NULL,                                 NULL
};															    
                                                                
static patchentry_t mfc70uPatch [] = {						    
    //ORDINAL(258),         &VS70::pmfcud_vector_new,		        VS70::mfcud_vector_new,
    //ORDINAL(833),         &VS70::pmfcud_scalar_new,		        VS70::mfcud_scalar_new,
    NULL,                 NULL,                                 NULL
};

static patchentry_t mfc70udPatch [] = {
    ORDINAL(258),         &VS70d::pmfcud_vector_new,		    VS70d::mfcud_vector_new,
    ORDINAL(259),         &VS70d::pmfcud__vector_new_dbg_4p,    VS70d::mfcud__vector_new_dbg_4p,
    ORDINAL(260),         &VS70d::pmfcud__vector_new_dbg_3p,    VS70d::mfcud__vector_new_dbg_3p,
    ORDINAL(833),         &VS70d::pmfcud_scalar_new,		    VS70d::mfcud_scalar_new,
    ORDINAL(834),         &VS70d::pmfcud__scalar_new_dbg_4p,    VS70d::mfcud__scalar_new_dbg_4p,
    ORDINAL(835),         &VS70d::pmfcud__scalar_new_dbg_3p,    VS70d::mfcud__scalar_new_dbg_3p,
    NULL,                 NULL,                                 NULL
};

static patchentry_t mfc71Patch [] = {
    //ORDINAL(267),         &VS71::pmfcd_vector_new,		        VS71::mfcd_vector_new,
    //ORDINAL(893),         &VS71::pmfcd_scalar_new,		        VS71::mfcd_scalar_new,
    NULL,                 NULL,                                 NULL
};															    
                                                                
static patchentry_t mfc71dPatch [] = {						    
    ORDINAL(267),         &VS71d::pmfcd_vector_new,		        VS71d::mfcd_vector_new,
    ORDINAL(268),         &VS71d::pmfcd__vector_new_dbg_4p,     VS71d::mfcd__vector_new_dbg_4p,
    ORDINAL(269),         &VS71d::pmfcd__vector_new_dbg_3p,     VS71d::mfcd__vector_new_dbg_3p,
    ORDINAL(893),         &VS71d::pmfcd_scalar_new,		        VS71d::mfcd_scalar_new,
    ORDINAL(894),         &VS71d::pmfcd__scalar_new_dbg_4p,     VS71d::mfcd__scalar_new_dbg_4p,
    ORDINAL(895),         &VS71d::pmfcd__scalar_new_dbg_3p,     VS71d::mfcd__scalar_new_dbg_3p,
    NULL,                 NULL,                                 NULL
};															    
                                                                
static patchentry_t mfc71uPatch [] = {						    
    //ORDINAL(267),         &VS71::pmfcud_vector_new,		        VS71::mfcud_vector_new,
    //ORDINAL(893),         &VS71::pmfcud_scalar_new,		        VS71::mfcud_scalar_new,
    NULL,                 NULL,                                 NULL
};															    
                                                                
static patchentry_t mfc71udPatch [] = {						    
    ORDINAL(267),         &VS71d::pmfcud_vector_new,		    VS71d::mfcud_vector_new,
    ORDINAL(268),         &VS71d::pmfcud__vector_new_dbg_4p,    VS71d::mfcud__vector_new_dbg_4p,
    ORDINAL(269),         &VS71d::pmfcud__vector_new_dbg_3p,    VS71d::mfcud__vector_new_dbg_3p,
    ORDINAL(893),         &VS71d::pmfcud_scalar_new,		    VS71d::mfcud_scalar_new,
    ORDINAL(894),         &VS71d::pmfcud__scalar_new_dbg_4p,    VS71d::mfcud__scalar_new_dbg_4p,
    ORDINAL(895),         &VS71d::pmfcud__scalar_new_dbg_3p,    VS71d::mfcud__scalar_new_dbg_3p,
    NULL,                 NULL,                                 NULL
};															    
                                                                
static patchentry_t mfc80Patch [] = {						    
    //ORDINAL(267),         &VS80::pmfcd_vector_new,		        VS80::mfcd_vector_new,
    //ORDINAL2(893,907),    &VS80::pmfcd_scalar_new,		        VS80::mfcd_scalar_new,
    NULL,                 NULL,                                 NULL
};															    
                                                                
static patchentry_t mfc80dPatch [] = {						    
    ORDINAL(267),         &VS80d::pmfcd_vector_new,		        VS80d::mfcd_vector_new,
    ORDINAL(268),         &VS80d::pmfcd__vector_new_dbg_4p,     VS80d::mfcd__vector_new_dbg_4p,
    ORDINAL(269),         &VS80d::pmfcd__vector_new_dbg_3p,     VS80d::mfcd__vector_new_dbg_3p,
    ORDINAL2(893,907),    &VS80d::pmfcd_scalar_new,		        VS80d::mfcd_scalar_new,
    ORDINAL2(894,908),    &VS80d::pmfcd__scalar_new_dbg_4p,     VS80d::mfcd__scalar_new_dbg_4p,
    ORDINAL2(895,909),    &VS80d::pmfcd__scalar_new_dbg_3p,     VS80d::mfcd__scalar_new_dbg_3p,
    NULL,                 NULL,                                 NULL
};															    
                                                                
static patchentry_t mfc80uPatch [] = {						    
    //ORDINAL(267),         &VS80::pmfcud_vector_new,		        VS80::mfcud_vector_new,
    //ORDINAL2(893,907),    &VS80::pmfcud_scalar_new,		        VS80::mfcud_scalar_new,
    NULL,                 NULL,                                 NULL
};															    
                                                                
static patchentry_t mfc80udPatch [] = {						    
    ORDINAL(267),         &VS80d::pmfcud_vector_new,		    VS80d::mfcud_vector_new,
    ORDINAL(268),         &VS80d::pmfcud__vector_new_dbg_4p,    VS80d::mfcud__vector_new_dbg_4p,
    ORDINAL(269),         &VS80d::pmfcud__vector_new_dbg_3p,    VS80d::mfcud__vector_new_dbg_3p,
    ORDINAL2(893,907),    &VS80d::pmfcud_scalar_new,		    VS80d::mfcud_scalar_new,
    ORDINAL2(894,908),    &VS80d::pmfcud__scalar_new_dbg_4p,    VS80d::mfcud__scalar_new_dbg_4p,
    ORDINAL2(895,909),    &VS80d::pmfcud__scalar_new_dbg_3p,    VS80d::mfcud__scalar_new_dbg_3p,
    NULL,                 NULL,                                 NULL
};															    
                                                                
static patchentry_t mfc90Patch [] = {						    
    ORDINAL(265),         &VS90::pmfcd_vector_new,		        VS90::mfcd_vector_new,
    ORDINAL2(798, 776),   &VS90::pmfcd_scalar_new,		        VS90::mfcd_scalar_new,
    NULL,                 NULL,                                 NULL
};															    
                                                                
static patchentry_t mfc90dPatch [] = {						    
    ORDINAL(267),         &VS90d::pmfcd_vector_new,		        VS90d::mfcd_vector_new,
    ORDINAL(268),         &VS90d::pmfcd__vector_new_dbg_4p,     VS90d::mfcd__vector_new_dbg_4p,
    ORDINAL(269),         &VS90d::pmfcd__vector_new_dbg_3p,     VS90d::mfcd__vector_new_dbg_3p,
    ORDINAL2(931, 909),   &VS90d::pmfcd_scalar_new,		        VS90d::mfcd_scalar_new,
    ORDINAL2(932, 910),   &VS90d::pmfcd__scalar_new_dbg_4p,     VS90d::mfcd__scalar_new_dbg_4p,
    ORDINAL2(933, 911),   &VS90d::pmfcd__scalar_new_dbg_3p,     VS90d::mfcd__scalar_new_dbg_3p,
    NULL,                 NULL,                                 NULL
};															    
                                                                
static patchentry_t mfc90uPatch [] = {						    
    ORDINAL(265),         &VS90::pmfcud_vector_new,		        VS90::mfcud_vector_new,
    ORDINAL2(798, 776),   &VS90::pmfcud_scalar_new,		        VS90::mfcud_scalar_new,
    NULL,                 NULL,                                 NULL
};															    
                                                                
static patchentry_t mfc90udPatch [] = {						    
    ORDINAL(267),         &VS90d::pmfcud_vector_new,		    VS90d::mfcud_vector_new,
    ORDINAL(268),         &VS90d::pmfcud__vector_new_dbg_4p,    VS90d::mfcud__vector_new_dbg_4p,
    ORDINAL(269),         &VS90d::pmfcud__vector_new_dbg_3p,    VS90d::mfcud__vector_new_dbg_3p,
    ORDINAL2(935, 913),   &VS90d::pmfcud_scalar_new,		    VS90d::mfcud_scalar_new,
    ORDINAL2(936, 914),   &VS90d::pmfcud__scalar_new_dbg_4p,    VS90d::mfcud__scalar_new_dbg_4p,
    ORDINAL2(937, 915),   &VS90d::pmfcud__scalar_new_dbg_3p,    VS90d::mfcud__scalar_new_dbg_3p,
    NULL,                 NULL,                                 NULL
};
                                                                
static patchentry_t mfc100Patch [] = {						    
    ORDINAL(265),         &VS100::pmfcd_vector_new,		        VS100::mfcd_vector_new,
    ORDINAL2(1294, 1272), &VS100::pmfcd_scalar_new,		        VS100::mfcd_scalar_new,
    NULL,                 NULL,                                 NULL
};
                                                                
static patchentry_t mfc100dPatch [] = {						    
    ORDINAL(267),         &VS100d::pmfcd_vector_new,		    VS100d::mfcd_vector_new,
    ORDINAL(268),         &VS100d::pmfcd__vector_new_dbg_4p,    VS100d::mfcd__vector_new_dbg_4p,
    ORDINAL(269),         &VS100d::pmfcd__vector_new_dbg_3p,    VS100d::mfcd__vector_new_dbg_3p,
    ORDINAL2(1427, 1405), &VS100d::pmfcd_scalar_new,		    VS100d::mfcd_scalar_new,
    ORDINAL2(1428, 1406), &VS100d::pmfcd__scalar_new_dbg_4p,    VS100d::mfcd__scalar_new_dbg_4p,
    ORDINAL2(1429, 1407), &VS100d::pmfcd__scalar_new_dbg_3p,    VS100d::mfcd__scalar_new_dbg_3p,
    NULL,                 NULL,                                 NULL
};
                                                                
static patchentry_t mfc100uPatch [] = {						    
    ORDINAL(265),         &VS100::pmfcud_vector_new,		    VS100::mfcud_vector_new,
    ORDINAL2(1298, 1276), &VS100::pmfcud_scalar_new,		    VS100::mfcud_scalar_new,
    NULL,                 NULL,                                 NULL
};															    
                                                                
static patchentry_t mfc100udPatch [] = {					    
    ORDINAL(267),         &VS100d::pmfcud_vector_new,		    VS100d::mfcud_vector_new,
    ORDINAL(268),         &VS100d::pmfcud__vector_new_dbg_4p,   VS100d::mfcud__vector_new_dbg_4p,
    ORDINAL(269),         &VS100d::pmfcud__vector_new_dbg_3p,   VS100d::mfcud__vector_new_dbg_3p,
    ORDINAL2(1434, 1412), &VS100d::pmfcud_scalar_new,		    VS100d::mfcud_scalar_new,
    ORDINAL2(1435, 1413), &VS100d::pmfcud__scalar_new_dbg_4p,   VS100d::mfcud__scalar_new_dbg_4p,
    ORDINAL2(1436, 1414), &VS100d::pmfcud__scalar_new_dbg_3p,   VS100d::mfcud__scalar_new_dbg_3p,
    NULL,                 NULL,                                 NULL
};
                                                                
static patchentry_t mfc110Patch [] = {						    
    ORDINAL(265),         &VS110::pmfcd_vector_new,		        VS110::mfcd_vector_new,
    ORDINAL2(1498, 1476), &VS110::pmfcd_scalar_new,		        VS110::mfcd_scalar_new,
    NULL,                 NULL,                                 NULL
};
                                                                
static patchentry_t mfc110dPatch [] = {						    
    ORDINAL(267),         &VS110d::pmfcd_vector_new,		    VS110d::mfcd_vector_new,
    ORDINAL(268),         &VS110d::pmfcd__vector_new_dbg_4p,    VS110d::mfcd__vector_new_dbg_4p,
    ORDINAL(269),         &VS110d::pmfcd__vector_new_dbg_3p,    VS110d::mfcd__vector_new_dbg_3p,
    ORDINAL2(1629, 1607), &VS110d::pmfcd_scalar_new,		    VS110d::mfcd_scalar_new,
    ORDINAL2(1630, 1608), &VS110d::pmfcd__scalar_new_dbg_4p,    VS110d::mfcd__scalar_new_dbg_4p,
    ORDINAL2(1631, 1609), &VS110d::pmfcd__scalar_new_dbg_3p,    VS110d::mfcd__scalar_new_dbg_3p,
    NULL,                 NULL,                                 NULL
};
                                                                
static patchentry_t mfc110uPatch [] = {						    
    ORDINAL(265),         &VS110::pmfcud_vector_new,		    VS110::mfcud_vector_new,
    ORDINAL2(1502, 1480), &VS110::pmfcud_scalar_new,		    VS110::mfcud_scalar_new,
    NULL,                 NULL,                                 NULL
};															    
                                                                
static patchentry_t mfc110udPatch [] = {					    
    ORDINAL(267),         &VS110d::pmfcud_vector_new,		    VS110d::mfcud_vector_new,
    ORDINAL(268),         &VS110d::pmfcud__vector_new_dbg_4p,   VS110d::mfcud__vector_new_dbg_4p,
    ORDINAL(269),         &VS110d::pmfcud__vector_new_dbg_3p,   VS110d::mfcud__vector_new_dbg_3p,
    ORDINAL2(1636, 1614), &VS110d::pmfcud_scalar_new,		    VS110d::mfcud_scalar_new,
    ORDINAL2(1637, 1615), &VS110d::pmfcud__scalar_new_dbg_4p,   VS110d::mfcud__scalar_new_dbg_4p,
    ORDINAL2(1638, 1616), &VS110d::pmfcud__scalar_new_dbg_3p,   VS110d::mfcud__scalar_new_dbg_3p,
    NULL,                 NULL,                                 NULL
};

static patchentry_t msvcrtPatch [] = {
    scalar_new_dbg_name,  &VS60::pcrtd__scalar_new_dbg,	    VS60::crtd__scalar_new_dbg,
    //vector_new_dbg_name,  &VS60::pcrtd__vector_new_dbg,     VS60::crtd__vector_new_dbg,
    "calloc",             &VS60::pcrtd_calloc,              VS60::crtd_calloc,
    "malloc",             &VS60::pcrtd_malloc,              VS60::crtd_malloc,
    "realloc",            &VS60::pcrtd_realloc,             VS60::crtd_realloc,
    "_strdup",            &VS60::pcrtd__strdup,             VS60::crtd__strdup,
    scalar_new_name,      &VS60::pcrtd_scalar_new,          VS60::crtd_scalar_new,
    //vector_new_name,      &VS60::pcrtd_vector_new,          VS60::crtd_vector_new,
    NULL,                 NULL,                             NULL
};

static patchentry_t msvcrtdPatch [] = {
    "_calloc_dbg",        &VS60d::pcrtd__calloc_dbg,	    VS60d::crtd__calloc_dbg,
    "_malloc_dbg",        &VS60d::pcrtd__malloc_dbg,	    VS60d::crtd__malloc_dbg,
    "_realloc_dbg",       &VS60d::pcrtd__realloc_dbg,       VS60d::crtd__realloc_dbg,
    scalar_new_dbg_name,  &VS60d::pcrtd__scalar_new_dbg,    VS60d::crtd__scalar_new_dbg,
    //vector_new_dbg_name,  &VS60d::pcrtd__vector_new_dbg,     VS60d::crtd__vector_new_dbg,
    "calloc",             &VS60d::pcrtd_calloc,             VS60d::crtd_calloc,
    "malloc",             &VS60d::pcrtd_malloc,             VS60d::crtd_malloc,
    "realloc",            &VS60d::pcrtd_realloc,            VS60d::crtd_realloc,
    "_strdup",            &VS60d::pcrtd__strdup,            VS60d::crtd__strdup,
    scalar_new_name,      &VS60d::pcrtd_scalar_new,         VS60d::crtd_scalar_new,
    //vector_new_name,      &VS60d::pcrtd_vector_new,         VS60d::crtd_vector_new,
    NULL,                 NULL,                             NULL
};

static patchentry_t msvcr70Patch [] = {
    scalar_new_dbg_name,  &VS70::pcrtd__scalar_new_dbg,     VS70::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS70::pcrtd__vector_new_dbg,     VS70::crtd__vector_new_dbg,
    "calloc",             &VS70::pcrtd_calloc,              VS70::crtd_calloc,
    "malloc",             &VS70::pcrtd_malloc,              VS70::crtd_malloc,
    "realloc",            &VS70::pcrtd_realloc,             VS70::crtd_realloc,
    "_strdup",            &VS70::pcrtd__strdup,             VS70::crtd__strdup,
    "_wcsdup",            &VS70::pcrtd__wcsdup,             VS70::crtd__wcsdup,
    scalar_new_name,      &VS70::pcrtd_scalar_new,          VS70::crtd_scalar_new,
    vector_new_name,      &VS70::pcrtd_vector_new,          VS70::crtd_vector_new,
    NULL,                 NULL,                             NULL
};

static patchentry_t msvcr70dPatch [] = {
    "_calloc_dbg",        &VS70d::pcrtd__calloc_dbg,	    VS70d::crtd__calloc_dbg,
    "_malloc_dbg",        &VS70d::pcrtd__malloc_dbg,	    VS70d::crtd__malloc_dbg,
    "_realloc_dbg",       &VS70d::pcrtd__realloc_dbg,       VS70d::crtd__realloc_dbg,
    scalar_new_dbg_name,  &VS70d::pcrtd__scalar_new_dbg,    VS70d::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS70d::pcrtd__vector_new_dbg,    VS70d::crtd__vector_new_dbg,
    "calloc",             &VS70d::pcrtd_calloc,             VS70d::crtd_calloc,
    "malloc",             &VS70d::pcrtd_malloc,             VS70d::crtd_malloc,
    "realloc",            &VS70d::pcrtd_realloc,            VS70d::crtd_realloc,
    "_strdup",            &VS70d::pcrtd__strdup,            VS70d::crtd__strdup,
    "_wcsdup",            &VS70d::pcrtd__wcsdup,            VS70d::crtd__wcsdup,
    scalar_new_name,      &VS70d::pcrtd_scalar_new,         VS70d::crtd_scalar_new,
    vector_new_name,      &VS70d::pcrtd_vector_new,         VS70d::crtd_vector_new,
    NULL,                 NULL,                             NULL
};

static patchentry_t msvcr71Patch [] = {
    scalar_new_dbg_name,  &VS71::pcrtd__scalar_new_dbg,     VS71::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS71::pcrtd__vector_new_dbg,     VS71::crtd__vector_new_dbg,
    "calloc",             &VS71::pcrtd_calloc,              VS71::crtd_calloc,
    "malloc",             &VS71::pcrtd_malloc,              VS71::crtd_malloc,
    "realloc",            &VS71::pcrtd_realloc,             VS71::crtd_realloc,
    "_strdup",            &VS71::pcrtd__strdup,             VS71::crtd__strdup,
    "_wcsdup",            &VS71::pcrtd__wcsdup,             VS71::crtd__wcsdup,
    scalar_new_name,      &VS71::pcrtd_scalar_new,          VS71::crtd_scalar_new,
    vector_new_name,      &VS71::pcrtd_vector_new,          VS71::crtd_vector_new,
    NULL,                 NULL,                             NULL
};

static patchentry_t msvcr71dPatch [] = {
    "_calloc_dbg",        &VS71d::pcrtd__calloc_dbg,	    VS71d::crtd__calloc_dbg,
    "_malloc_dbg",        &VS71d::pcrtd__malloc_dbg,	    VS71d::crtd__malloc_dbg,
    "_realloc_dbg",       &VS71d::pcrtd__realloc_dbg,       VS71d::crtd__realloc_dbg,
    "_strdup_dbg",        &VS71d::pcrtd__strdup_dbg,        VS71d::crtd__strdup_dbg,
    "_wcsdup_dbg",        &VS71d::pcrtd__wcsdup_dbg,        VS71d::crtd__wcsdup_dbg,
    scalar_new_dbg_name,  &VS71d::pcrtd__scalar_new_dbg,    VS71d::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS71d::pcrtd__vector_new_dbg,    VS71d::crtd__vector_new_dbg,
    "calloc",             &VS71d::pcrtd_calloc,             VS71d::crtd_calloc,
    "malloc",             &VS71d::pcrtd_malloc,             VS71d::crtd_malloc,
    "realloc",            &VS71d::pcrtd_realloc,            VS71d::crtd_realloc,
    "_strdup",            &VS71d::pcrtd__strdup,            VS71d::crtd__strdup,
    "_wcsdup",            &VS71d::pcrtd__wcsdup,            VS71d::crtd__wcsdup,
    scalar_new_name,      &VS71d::pcrtd_scalar_new,         VS71d::crtd_scalar_new,
    vector_new_name,      &VS71d::pcrtd_vector_new,         VS71d::crtd_vector_new,
    NULL,                 NULL,                             NULL
};

static patchentry_t msvcr80Patch [] = {
    scalar_new_dbg_name,  &VS80::pcrtd__scalar_new_dbg,     VS80::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS80::pcrtd__vector_new_dbg,     VS80::crtd__vector_new_dbg,
    "calloc",             &VS80::pcrtd_calloc,              VS80::crtd_calloc,
    "malloc",             &VS80::pcrtd_malloc,              VS80::crtd_malloc,
    "realloc",            &VS80::pcrtd_realloc,             VS80::crtd_realloc,
    "_strdup",            &VS80::pcrtd__strdup,             VS80::crtd__strdup,
    "_wcsdup",            &VS80::pcrtd__wcsdup,             VS80::crtd__wcsdup,
    scalar_new_name,      &VS80::pcrtd_scalar_new,          VS80::crtd_scalar_new,
    vector_new_name,      &VS80::pcrtd_vector_new,          VS80::crtd_vector_new,
    "_aligned_malloc",			    &VS80::pcrtd_aligned_malloc,                VS80::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS80::pcrtd_aligned_offset_malloc,         VS80::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS80::pcrtd_aligned_realloc,               VS80::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS80::pcrtd_aligned_offset_realloc,        VS80::crtd__aligned_offset_realloc,
    NULL,                           NULL,                                       NULL
};

static patchentry_t msvcr80dPatch [] = {
    "_calloc_dbg",        &VS80d::pcrtd__calloc_dbg,	    VS80d::crtd__calloc_dbg,
    "_malloc_dbg",        &VS80d::pcrtd__malloc_dbg,	    VS80d::crtd__malloc_dbg,
    "_realloc_dbg",       &VS80d::pcrtd__realloc_dbg,	    VS80d::crtd__realloc_dbg,
    "_strdup_dbg",        &VS80d::pcrtd__strdup_dbg,        VS80d::crtd__strdup_dbg,
    "_wcsdup_dbg",        &VS80d::pcrtd__wcsdup_dbg,        VS80d::crtd__wcsdup_dbg,
    scalar_new_dbg_name,  &VS80d::pcrtd__scalar_new_dbg,    VS80d::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS80d::pcrtd__vector_new_dbg,    VS80d::crtd__vector_new_dbg,
    "calloc",             &VS80d::pcrtd_calloc,			    VS80d::crtd_calloc,
    "malloc",             &VS80d::pcrtd_malloc,			    VS80d::crtd_malloc,
    "realloc",            &VS80d::pcrtd_realloc,		    VS80d::crtd_realloc,
    "_strdup",            &VS80d::pcrtd__strdup,            VS80d::crtd__strdup,
    "_wcsdup",            &VS80d::pcrtd__wcsdup,            VS80d::crtd__wcsdup,
    scalar_new_name,      &VS80d::pcrtd_scalar_new,		    VS80d::crtd_scalar_new,
    vector_new_name,      &VS80d::pcrtd_vector_new,		    VS80d::crtd_vector_new,
    "_aligned_malloc_dbg",          &VS80d::pcrtd__aligned_malloc_dbg,          VS80d::crtd__aligned_malloc_dbg,
    "_aligned_offset_malloc_dbg",   &VS80d::pcrtd__aligned_offset_malloc_dbg,   VS80d::crtd__aligned_offset_malloc_dbg,
    "_aligned_realloc_dbg",		    &VS80d::pcrtd__aligned_realloc_dbg,         VS80d::crtd__aligned_realloc_dbg,
    "_aligned_offset_realloc_dbg",  &VS80d::pcrtd__aligned_offset_realloc_dbg,  VS80d::crtd__aligned_offset_realloc_dbg,
    "_aligned_malloc",			    &VS80d::pcrtd_aligned_malloc,               VS80d::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS80d::pcrtd_aligned_offset_malloc,        VS80d::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS80d::pcrtd_aligned_realloc,              VS80d::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS80d::pcrtd_aligned_offset_realloc,       VS80d::crtd__aligned_offset_realloc,
    NULL,                           NULL,                                       NULL
};

static patchentry_t msvcr90Patch [] = {
    scalar_new_dbg_name,  &VS90::pcrtd__scalar_new_dbg,     VS90::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS90::pcrtd__vector_new_dbg,     VS90::crtd__vector_new_dbg,
    "calloc",             &VS90::pcrtd_calloc,              VS90::crtd_calloc,
    "malloc",             &VS90::pcrtd_malloc,              VS90::crtd_malloc,
    "realloc",            &VS90::pcrtd_realloc,             VS90::crtd_realloc,
    "_recalloc",          &VS90::pcrtd_recalloc,            VS90::crtd__recalloc,
    "_strdup",            &VS90::pcrtd__strdup,             VS90::crtd__strdup,
    "_wcsdup",            &VS90::pcrtd__wcsdup,             VS90::crtd__wcsdup,
    scalar_new_name,      &VS90::pcrtd_scalar_new,          VS90::crtd_scalar_new,
    vector_new_name,      &VS90::pcrtd_vector_new,          VS90::crtd_vector_new,
    "_aligned_malloc",			    &VS90::pcrtd_aligned_malloc,                VS90::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS90::pcrtd_aligned_offset_malloc,         VS90::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS90::pcrtd_aligned_realloc,               VS90::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS90::pcrtd_aligned_offset_realloc,        VS90::crtd__aligned_offset_realloc,
    "_aligned_recalloc",            &VS90::pcrtd_aligned_recalloc,              VS90::crtd__aligned_recalloc,
    "_aligned_offset_recalloc",     &VS90::pcrtd_aligned_offset_recalloc,       VS90::crtd__aligned_offset_recalloc,
    NULL,                           NULL,                                       NULL
};

static patchentry_t msvcr90dPatch [] = {
    "_calloc_dbg",        &VS90d::pcrtd__calloc_dbg,	    VS90d::crtd__calloc_dbg,
    "_malloc_dbg",        &VS90d::pcrtd__malloc_dbg,	    VS90d::crtd__malloc_dbg,
    "_realloc_dbg",       &VS90d::pcrtd__realloc_dbg,       VS90d::crtd__realloc_dbg,
    "_recalloc_dbg",      &VS90d::pcrtd__recalloc_dbg,      VS90d::crtd__recalloc_dbg,
    "_strdup_dbg",        &VS90d::pcrtd__strdup_dbg,        VS90d::crtd__strdup_dbg,
    "_wcsdup_dbg",        &VS90d::pcrtd__wcsdup_dbg,        VS90d::crtd__wcsdup_dbg,
    scalar_new_dbg_name,  &VS90d::pcrtd__scalar_new_dbg,    VS90d::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS90d::pcrtd__vector_new_dbg,    VS90d::crtd__vector_new_dbg,
    "calloc",             &VS90d::pcrtd_calloc,             VS90d::crtd_calloc,
    "malloc",             &VS90d::pcrtd_malloc,             VS90d::crtd_malloc,
    "realloc",            &VS90d::pcrtd_realloc,            VS90d::crtd_realloc,
    "_recalloc",          &VS90d::pcrtd_recalloc,           VS90d::crtd__recalloc,
    "_strdup",            &VS90d::pcrtd__strdup,            VS90d::crtd__strdup,
    "_wcsdup",            &VS90d::pcrtd__wcsdup,            VS90d::crtd__wcsdup,
    scalar_new_name,      &VS90d::pcrtd_scalar_new,         VS90d::crtd_scalar_new,
    vector_new_name,      &VS90d::pcrtd_vector_new,         VS90d::crtd_vector_new,
    "_aligned_malloc_dbg",          &VS90d::pcrtd__aligned_malloc_dbg,          VS90d::crtd__aligned_malloc_dbg,
    "_aligned_offset_malloc_dbg",   &VS90d::pcrtd__aligned_offset_malloc_dbg,   VS90d::crtd__aligned_offset_malloc_dbg,
    "_aligned_realloc_dbg",		    &VS90d::pcrtd__aligned_realloc_dbg,         VS90d::crtd__aligned_realloc_dbg,
    "_aligned_offset_realloc_dbg",  &VS90d::pcrtd__aligned_offset_realloc_dbg,  VS90d::crtd__aligned_offset_realloc_dbg,
    "_aligned_recalloc_dbg",		&VS90d::pcrtd__aligned_recalloc_dbg,        VS90d::crtd__aligned_recalloc_dbg,
    "_aligned_offset_recalloc_dbg", &VS90d::pcrtd__aligned_offset_recalloc_dbg, VS90d::crtd__aligned_offset_recalloc_dbg,
    "_aligned_malloc",			    &VS90d::pcrtd_aligned_malloc,               VS90d::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS90d::pcrtd_aligned_offset_malloc,        VS90d::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS90d::pcrtd_aligned_realloc,              VS90d::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS90d::pcrtd_aligned_offset_realloc,       VS90d::crtd__aligned_offset_realloc,
    "_aligned_recalloc",            &VS90d::pcrtd_aligned_recalloc,             VS90d::crtd__aligned_recalloc,
    "_aligned_offset_recalloc",     &VS90d::pcrtd_aligned_offset_recalloc,      VS90d::crtd__aligned_offset_recalloc,
    NULL,                           NULL,                                       NULL
};

static patchentry_t msvcr100Patch [] = {
    scalar_new_dbg_name,  &VS100::pcrtd__scalar_new_dbg,    VS100::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS100::pcrtd__vector_new_dbg,    VS100::crtd__vector_new_dbg,
    "calloc",             &VS100::pcrtd_calloc,             VS100::crtd_calloc,
    "malloc",             &VS100::pcrtd_malloc,             VS100::crtd_malloc,
    "realloc",            &VS100::pcrtd_realloc,            VS100::crtd_realloc,
    "_recalloc",          &VS100::pcrtd_recalloc,           VS100::crtd__recalloc,
    "_strdup",            &VS100::pcrtd__strdup,            VS100::crtd__strdup,
    "_wcsdup",            &VS100::pcrtd__wcsdup,            VS100::crtd__wcsdup,
    scalar_new_name,      &VS100::pcrtd_scalar_new,         VS100::crtd_scalar_new,
    vector_new_name,      &VS100::pcrtd_vector_new,         VS100::crtd_vector_new,
    "_aligned_malloc",			    &VS100::pcrtd_aligned_malloc,               VS100::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS100::pcrtd_aligned_offset_malloc,        VS100::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS100::pcrtd_aligned_realloc,              VS100::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS100::pcrtd_aligned_offset_realloc,       VS100::crtd__aligned_offset_realloc,
    "_aligned_recalloc",            &VS100::pcrtd_aligned_recalloc,             VS100::crtd__aligned_recalloc,
    "_aligned_offset_recalloc",     &VS100::pcrtd_aligned_offset_recalloc,      VS100::crtd__aligned_offset_recalloc,
    NULL,                           NULL,                                       NULL,                                       
};

static patchentry_t msvcr100dPatch [] = {
    "_calloc_dbg",        &VS100d::pcrtd__calloc_dbg,	    VS100d::crtd__calloc_dbg,
    "_malloc_dbg",        &VS100d::pcrtd__malloc_dbg,	    VS100d::crtd__malloc_dbg,
    "_realloc_dbg",       &VS100d::pcrtd__realloc_dbg,      VS100d::crtd__realloc_dbg,
    "_recalloc_dbg",      &VS100d::pcrtd__recalloc_dbg,     VS100d::crtd__recalloc_dbg,
    "_strdup_dbg",        &VS100d::pcrtd__strdup_dbg,       VS100d::crtd__strdup_dbg,
    "_wcsdup_dbg",        &VS100d::pcrtd__wcsdup_dbg,       VS100d::crtd__wcsdup_dbg,
    scalar_new_dbg_name,  &VS100d::pcrtd__scalar_new_dbg,   VS100d::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS100d::pcrtd__vector_new_dbg,   VS100d::crtd__vector_new_dbg,
    "calloc",             &VS100d::pcrtd_calloc,            VS100d::crtd_calloc,
    "malloc",             &VS100d::pcrtd_malloc,            VS100d::crtd_malloc,
    "realloc",            &VS100d::pcrtd_realloc,           VS100d::crtd_realloc,
    "_recalloc",          &VS100d::pcrtd_recalloc,          VS100d::crtd__recalloc,
    "_strdup",            &VS100d::pcrtd__strdup,           VS100d::crtd__strdup,
    "_wcsdup",            &VS100d::pcrtd__wcsdup,           VS100d::crtd__wcsdup,
    scalar_new_name,      &VS100d::pcrtd_scalar_new,        VS100d::crtd_scalar_new,
    vector_new_name,      &VS100d::pcrtd_vector_new,        VS100d::crtd_vector_new,
    "_aligned_malloc_dbg",          &VS100d::pcrtd__aligned_malloc_dbg,         VS100d::crtd__aligned_malloc_dbg,
    "_aligned_offset_malloc_dbg",   &VS100d::pcrtd__aligned_offset_malloc_dbg,  VS100d::crtd__aligned_offset_malloc_dbg,
    "_aligned_realloc_dbg",		    &VS100d::pcrtd__aligned_realloc_dbg,        VS100d::crtd__aligned_realloc_dbg,
    "_aligned_offset_realloc_dbg",  &VS100d::pcrtd__aligned_offset_realloc_dbg, VS100d::crtd__aligned_offset_realloc_dbg,
    "_aligned_recalloc_dbg",		&VS100d::pcrtd__aligned_recalloc_dbg,       VS100d::crtd__aligned_recalloc_dbg,
    "_aligned_offset_recalloc_dbg", &VS100d::pcrtd__aligned_offset_recalloc_dbg,VS100d::crtd__aligned_offset_recalloc_dbg,
    "_aligned_malloc",			    &VS100d::pcrtd_aligned_malloc,              VS100d::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS100d::pcrtd_aligned_offset_malloc,       VS100d::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS100d::pcrtd_aligned_realloc,             VS100d::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS100d::pcrtd_aligned_offset_realloc,      VS100d::crtd__aligned_offset_realloc,
    "_aligned_recalloc",            &VS100d::pcrtd_aligned_recalloc,            VS100d::crtd__aligned_recalloc,
    "_aligned_offset_recalloc",     &VS100d::pcrtd_aligned_offset_recalloc,     VS100d::crtd__aligned_offset_recalloc,
    NULL,                           NULL,                                       NULL,                                       
};

static patchentry_t msvcr110Patch [] = {
    scalar_new_dbg_name,  &VS110::pcrtd__scalar_new_dbg,    VS110::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS110::pcrtd__vector_new_dbg,    VS110::crtd__vector_new_dbg,
    "calloc",             &VS110::pcrtd_calloc,             VS110::crtd_calloc,
    "malloc",             &VS110::pcrtd_malloc,             VS110::crtd_malloc,
    "realloc",            &VS110::pcrtd_realloc,            VS110::crtd_realloc,
    "_recalloc",          &VS110::pcrtd_recalloc,           VS110::crtd__recalloc,
    "_strdup",            &VS110::pcrtd__strdup,            VS110::crtd__strdup,
    "_wcsdup",            &VS110::pcrtd__wcsdup,            VS110::crtd__wcsdup,
    scalar_new_name,      &VS110::pcrtd_scalar_new,         VS110::crtd_scalar_new,
    vector_new_name,      &VS110::pcrtd_vector_new,         VS110::crtd_vector_new,
    "_aligned_malloc",			    &VS110::pcrtd_aligned_malloc,               VS110::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS110::pcrtd_aligned_offset_malloc,        VS110::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS110::pcrtd_aligned_realloc,              VS110::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS110::pcrtd_aligned_offset_realloc,       VS110::crtd__aligned_offset_realloc,
    "_aligned_recalloc",            &VS110::pcrtd_aligned_recalloc,             VS110::crtd__aligned_recalloc,
    "_aligned_offset_recalloc",     &VS110::pcrtd_aligned_offset_recalloc,      VS110::crtd__aligned_offset_recalloc,
    NULL,                           NULL,                                       NULL,                                       
};

static patchentry_t msvcr110dPatch [] = {
    "_calloc_dbg",        &VS110d::pcrtd__calloc_dbg,	    VS110d::crtd__calloc_dbg,
    "_malloc_dbg",        &VS110d::pcrtd__malloc_dbg,	    VS110d::crtd__malloc_dbg,
    "_realloc_dbg",       &VS110d::pcrtd__realloc_dbg,      VS110d::crtd__realloc_dbg,
    "_recalloc_dbg",      &VS110d::pcrtd__recalloc_dbg,     VS110d::crtd__recalloc_dbg,
    "_strdup_dbg",        &VS110d::pcrtd__strdup_dbg,       VS110d::crtd__strdup_dbg,
    "_wcsdup_dbg",        &VS110d::pcrtd__wcsdup_dbg,       VS110d::crtd__wcsdup_dbg,
    scalar_new_dbg_name,  &VS110d::pcrtd__scalar_new_dbg,   VS110d::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS110d::pcrtd__vector_new_dbg,   VS110d::crtd__vector_new_dbg,
    "calloc",             &VS110d::pcrtd_calloc,            VS110d::crtd_calloc,
    "malloc",             &VS110d::pcrtd_malloc,            VS110d::crtd_malloc,
    "realloc",            &VS110d::pcrtd_realloc,           VS110d::crtd_realloc,
    "_recalloc",          &VS110d::pcrtd_recalloc,          VS110d::crtd__recalloc,
    "_strdup",            &VS110d::pcrtd__strdup,           VS110d::crtd__strdup,
    "_wcsdup",            &VS110d::pcrtd__wcsdup,           VS110d::crtd__wcsdup,
    scalar_new_name,      &VS110d::pcrtd_scalar_new,        VS110d::crtd_scalar_new,
    vector_new_name,      &VS110d::pcrtd_vector_new,        VS110d::crtd_vector_new,
    "_aligned_malloc_dbg",          &VS110d::pcrtd__aligned_malloc_dbg,         VS110d::crtd__aligned_malloc_dbg,
    "_aligned_offset_malloc_dbg",   &VS110d::pcrtd__aligned_offset_malloc_dbg,  VS110d::crtd__aligned_offset_malloc_dbg,
    "_aligned_realloc_dbg",		    &VS110d::pcrtd__aligned_realloc_dbg,        VS110d::crtd__aligned_realloc_dbg,
    "_aligned_offset_realloc_dbg",  &VS110d::pcrtd__aligned_offset_realloc_dbg, VS110d::crtd__aligned_offset_realloc_dbg,
    "_aligned_recalloc_dbg",		&VS110d::pcrtd__aligned_recalloc_dbg,       VS110d::crtd__aligned_recalloc_dbg,
    "_aligned_offset_recalloc_dbg", &VS110d::pcrtd__aligned_offset_recalloc_dbg,VS110d::crtd__aligned_offset_recalloc_dbg,
    "_aligned_malloc",			    &VS110d::pcrtd_aligned_malloc,              VS110d::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS110d::pcrtd_aligned_offset_malloc,       VS110d::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS110d::pcrtd_aligned_realloc,             VS110d::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS110d::pcrtd_aligned_offset_realloc,      VS110d::crtd__aligned_offset_realloc,
    "_aligned_recalloc",            &VS110d::pcrtd_aligned_recalloc,            VS110d::crtd__aligned_recalloc,
    "_aligned_offset_recalloc",     &VS110d::pcrtd_aligned_offset_recalloc,     VS110d::crtd__aligned_offset_recalloc,
    NULL,                           NULL,                                       NULL,                                       
};

patchentry_t VisualLeakDetector::m_ntdllPatch [] = {
    "RtlAllocateHeap",    NULL, VisualLeakDetector::_RtlAllocateHeap,
    "RtlFreeHeap",        NULL, VisualLeakDetector::_RtlFreeHeap,
    "RtlReAllocateHeap",  NULL, VisualLeakDetector::_RtlReAllocateHeap,
    NULL,                 NULL, NULL
};

patchentry_t VisualLeakDetector::m_ole32Patch [] = {
    "CoGetMalloc",        NULL, VisualLeakDetector::_CoGetMalloc,
    "CoTaskMemAlloc",     NULL, VisualLeakDetector::_CoTaskMemAlloc,
    "CoTaskMemRealloc",   NULL, VisualLeakDetector::_CoTaskMemRealloc,
    NULL,                 NULL, NULL
};

moduleentry_t VisualLeakDetector::m_patchTable [] = {
    // Win32 heap APIs.
    "kernel32.dll", 0x0, m_kernelbasePatch, // we patch this record on Win7
    "kernel32.dll", 0x0, m_kernel32Patch,

    // MFC new operators (exported by ordinal).
    "mfc42.dll",    0x0, mfc42Patch,
    "mfc42d.dll",   0x0, mfc42dPatch,
    "mfc42u.dll",   0x0, mfc42uPatch,
    "mfc42ud.dll",  0x0, mfc42udPatch,
    "mfc70.dll",    0x0, mfc70Patch,
    "mfc70d.dll",   0x0, mfc70dPatch,
    "mfc70u.dll",   0x0, mfc70uPatch,
    "mfc70ud.dll",  0x0, mfc70udPatch,
    "mfc71.dll",    0x0, mfc71Patch,
    "mfc71d.dll",   0x0, mfc71dPatch,
    "mfc71u.dll",   0x0, mfc71uPatch,
    "mfc71ud.dll",  0x0, mfc71udPatch,
    "mfc80.dll",    0x0, mfc80Patch,
    "mfc80d.dll",   0x0, mfc80dPatch,
    "mfc80u.dll",   0x0, mfc80uPatch,
    "mfc80ud.dll",  0x0, mfc80udPatch,
    "mfc90.dll",    0x0, mfc90Patch,
    "mfc90d.dll",   0x0, mfc90dPatch,
    "mfc90u.dll",   0x0, mfc90uPatch,
    "mfc90ud.dll",  0x0, mfc90udPatch,
    "mfc100.dll",   0x0, mfc100Patch,
    "mfc100d.dll",  0x0, mfc100dPatch,
    "mfc100u.dll",  0x0, mfc100uPatch,
    "mfc100ud.dll", 0x0, mfc100udPatch,
    "mfc110.dll",   0x0, mfc110Patch,
    "mfc110d.dll",  0x0, mfc110dPatch,
    "mfc110u.dll",  0x0, mfc110uPatch,
    "mfc110ud.dll", 0x0, mfc110udPatch,

    // CRT new operators and heap APIs.
    "msvcrt.dll",   0x0, msvcrtPatch,
    "msvcrtd.dll",  0x0, msvcrtdPatch,
    "msvcr70.dll",  0x0, msvcr70Patch,
    "msvcr70d.dll", 0x0, msvcr70dPatch,
    "msvcr71.dll",  0x0, msvcr71Patch,
    "msvcr71d.dll", 0x0, msvcr71dPatch,
    "msvcr80.dll",  0x0, msvcr80Patch,
    "msvcr80d.dll", 0x0, msvcr80dPatch,
    "msvcr90.dll",  0x0, msvcr90Patch,
    "msvcr90d.dll", 0x0, msvcr90dPatch,
    "msvcr100.dll", 0x0, msvcr100Patch,
    "msvcr100d.dll",0x0, msvcr100dPatch,
    "msvcr110.dll", 0x0, msvcr110Patch,
    "msvcr110d.dll",0x0, msvcr110dPatch,

    // NT APIs.
    "ntdll.dll",    0x0, m_ntdllPatch,

    // COM heap APIs.
    "ole32.dll",    0x0, m_ole32Patch
};
