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
    "GetProcAddress",     NULL,                     _GetProcAddress, // Not heap related, but can be used to obtain pointers to heap functions.
	"GetProcAddressForCaller", NULL,                _GetProcAddressForCaller, // Not heap related, but can be used to obtain pointers to heap functions.
	"GetProcessHeap",     (LPVOID*)&m_GetProcessHeap, _GetProcessHeap,
    NULL,                 NULL, NULL
};

patchentry_t VisualLeakDetector::m_kernel32Patch [] = {
    "GetProcAddress",     NULL,                     _GetProcAddress, // Not heap related, but can be used to obtain pointers to heap functions.
    "HeapAlloc",          NULL,                     _HeapAlloc,
    "HeapCreate",         (LPVOID*)&m_HeapCreate,   _HeapCreate,
    "HeapDestroy",        NULL,                     _HeapDestroy,
    "HeapFree",           (LPVOID*)&m_HeapFree,     _HeapFree,
    "HeapReAlloc",        NULL,                     _HeapReAlloc,
    NULL,                 NULL,                     NULL
};

#define ORDINAL(x)          (LPCSTR)x
#if !defined(_M_X64)
#define ORDINAL2(x86, x64)  (LPCSTR)x86
#else
#define ORDINAL2(x86, x64)  (LPCSTR)x64
#endif

GetProcAddress_t VisualLeakDetector::m_GetProcAddress = NULL;
GetProcAddressForCaller_t VisualLeakDetector::m_GetProcAddressForCaller = NULL;
GetProcessHeap_t VisualLeakDetector::m_GetProcessHeap = NULL;
HeapCreate_t VisualLeakDetector::m_HeapCreate = NULL;
HeapFree_t VisualLeakDetector::m_HeapFree = NULL;

static patchentry_t mfc42Patch [] = {
    // XXX why are the vector new operators missing for mfc42.dll?
    //ORDINAL(711),           &Mfc60::pmfcd_scalar_new,             Mfc60::mfcd_scalar_new,
    NULL,                   NULL,                                   NULL
};

static patchentry_t mfc42dPatch [] = {
    // XXX why are the vector new operators missing for mfc42d.dll?
    ORDINAL(711),         &Mfc60d::data.pmfcd_scalar_new,             Mfc60d::mfcd_scalar_new,
    ORDINAL(712),         &Mfc60d::data.pmfcd__scalar_new_dbg_4p,     Mfc60d::mfcd__scalar_new_dbg_4p,
    ORDINAL(714),         &Mfc60d::data.pmfcd__scalar_new_dbg_3p,     Mfc60d::mfcd__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc42uPatch [] = {
    // XXX why are the vector new operators missing for mfc42u.dll?
    //ORDINAL(711),         &VS60::data.pmfcud_scalar_new,            VS60::mfcud_scalar_new,
    NULL,                 NULL,                                     NULL
};

static patchentry_t mfc42udPatch [] = {
    // XXX why are the vector new operators missing for mfc42ud.dll?
    ORDINAL(711),         &Mfc60d::data.pmfcud_scalar_new,            Mfc60d::mfcud_scalar_new,
    ORDINAL(712),         &Mfc60d::data.pmfcud__scalar_new_dbg_4p,    Mfc60d::mfcud__scalar_new_dbg_4p,
    ORDINAL(714),         &Mfc60d::data.pmfcud__scalar_new_dbg_3p,    Mfc60d::mfcud__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc70Patch [] = {
    //ORDINAL(257),         &Mfc70::data.pmfcd_vector_new,             Mfc70::mfcd_vector_new,
    //ORDINAL(832),         &Mfc70::data.pmfcd_scalar_new,             Mfc70::mfcd_scalar_new,
    NULL,                 NULL,                                     NULL
};

static patchentry_t mfc70dPatch [] = {
    ORDINAL(257),         &Mfc70d::data.pmfcd_vector_new,             Mfc70d::mfcd_vector_new,
    ORDINAL(258),         &Mfc70d::data.pmfcd__vector_new_dbg_4p,     Mfc70d::mfcd__vector_new_dbg_4p,
    ORDINAL(259),         &Mfc70d::data.pmfcd__vector_new_dbg_3p,     Mfc70d::mfcd__vector_new_dbg_3p,
    ORDINAL(832),         &Mfc70d::data.pmfcd_scalar_new,             Mfc70d::mfcd_scalar_new,
    ORDINAL(833),         &Mfc70d::data.pmfcd__scalar_new_dbg_4p,     Mfc70d::mfcd__scalar_new_dbg_4p,
    ORDINAL(834),         &Mfc70d::data.pmfcd__scalar_new_dbg_3p,     Mfc70d::mfcd__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc70uPatch [] = {
    //ORDINAL(258),         &Mfc70::data.pmfcud_vector_new,             Mfc70::mfcud_vector_new,
    //ORDINAL(833),         &Mfc70::data.pmfcud_scalar_new,             Mfc70::mfcud_scalar_new,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc70udPatch [] = {
    ORDINAL(258),         &Mfc70d::data.pmfcud_vector_new,            Mfc70d::mfcud_vector_new,
    ORDINAL(259),         &Mfc70d::data.pmfcud__vector_new_dbg_4p,    Mfc70d::mfcud__vector_new_dbg_4p,
    ORDINAL(260),         &Mfc70d::data.pmfcud__vector_new_dbg_3p,    Mfc70d::mfcud__vector_new_dbg_3p,
    ORDINAL(833),         &Mfc70d::data.pmfcud_scalar_new,            Mfc70d::mfcud_scalar_new,
    ORDINAL(834),         &Mfc70d::data.pmfcud__scalar_new_dbg_4p,    Mfc70d::mfcud__scalar_new_dbg_4p,
    ORDINAL(835),         &Mfc70d::data.pmfcud__scalar_new_dbg_3p,    Mfc70d::mfcud__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc71Patch [] = {
    //ORDINAL(267),         &Mfc71::data.pmfcd_vector_new,              MfcMfc71::mfcd_vector_new,
    //ORDINAL(893),         &Mfc71::data.pmfcd_scalar_new,              MfcMfc71::mfcd_scalar_new,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc71dPatch [] = {
    ORDINAL(267),         &Mfc71d::data.pmfcd_vector_new,             Mfc71d::mfcd_vector_new,
    ORDINAL(268),         &Mfc71d::data.pmfcd__vector_new_dbg_4p,     Mfc71d::mfcd__vector_new_dbg_4p,
    ORDINAL(269),         &Mfc71d::data.pmfcd__vector_new_dbg_3p,     Mfc71d::mfcd__vector_new_dbg_3p,
    ORDINAL(893),         &Mfc71d::data.pmfcd_scalar_new,             Mfc71d::mfcd_scalar_new,
    ORDINAL(894),         &Mfc71d::data.pmfcd__scalar_new_dbg_4p,     Mfc71d::mfcd__scalar_new_dbg_4p,
    ORDINAL(895),         &Mfc71d::data.pmfcd__scalar_new_dbg_3p,     Mfc71d::mfcd__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc71uPatch [] = {
    //ORDINAL(267),         &Mfc71::data.pmfcud_vector_new,             Mfc71::mfcud_vector_new,
    //ORDINAL(893),         &Mfc71::data.pmfcud_scalar_new,             Mfc71::mfcud_scalar_new,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc71udPatch [] = {
    ORDINAL(267),         &Mfc71d::data.pmfcud_vector_new,            Mfc71d::mfcud_vector_new,
    ORDINAL(268),         &Mfc71d::data.pmfcud__vector_new_dbg_4p,    Mfc71d::mfcud__vector_new_dbg_4p,
    ORDINAL(269),         &Mfc71d::data.pmfcud__vector_new_dbg_3p,    Mfc71d::mfcud__vector_new_dbg_3p,
    ORDINAL(893),         &Mfc71d::data.pmfcud_scalar_new,            Mfc71d::mfcud_scalar_new,
    ORDINAL(894),         &Mfc71d::data.pmfcud__scalar_new_dbg_4p,    Mfc71d::mfcud__scalar_new_dbg_4p,
    ORDINAL(895),         &Mfc71d::data.pmfcud__scalar_new_dbg_3p,    Mfc71d::mfcud__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc80Patch [] = {
    //ORDINAL(267),         &Mfc80::data.pmfcd_vector_new,              Mfc80::mfcd_vector_new,
    //ORDINAL2(893,907),    &Mfc80::data.pmfcd_scalar_new,              Mfc80::mfcd_scalar_new,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc80dPatch [] = {
    ORDINAL(267),         &Mfc80d::data.pmfcd_vector_new,             Mfc80d::mfcd_vector_new,
    ORDINAL(268),         &Mfc80d::data.pmfcd__vector_new_dbg_4p,     Mfc80d::mfcd__vector_new_dbg_4p,
    ORDINAL(269),         &Mfc80d::data.pmfcd__vector_new_dbg_3p,     Mfc80d::mfcd__vector_new_dbg_3p,
    ORDINAL2(893,907),    &Mfc80d::data.pmfcd_scalar_new,             Mfc80d::mfcd_scalar_new,
    ORDINAL2(894,908),    &Mfc80d::data.pmfcd__scalar_new_dbg_4p,     Mfc80d::mfcd__scalar_new_dbg_4p,
    ORDINAL2(895,909),    &Mfc80d::data.pmfcd__scalar_new_dbg_3p,     Mfc80d::mfcd__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc80uPatch [] = {
    //ORDINAL(267),         &Mfc80::data.pmfcud_vector_new,             Mfc80::mfcud_vector_new,
    //ORDINAL2(893,907),    &Mfc80::data.pmfcud_scalar_new,             Mfc80::mfcud_scalar_new,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc80udPatch [] = {
    ORDINAL(267),         &Mfc80d::data.pmfcud_vector_new,            Mfc80d::mfcud_vector_new,
    ORDINAL(268),         &Mfc80d::data.pmfcud__vector_new_dbg_4p,    Mfc80d::mfcud__vector_new_dbg_4p,
    ORDINAL(269),         &Mfc80d::data.pmfcud__vector_new_dbg_3p,    Mfc80d::mfcud__vector_new_dbg_3p,
    ORDINAL2(893,907),    &Mfc80d::data.pmfcud_scalar_new,            Mfc80d::mfcud_scalar_new,
    ORDINAL2(894,908),    &Mfc80d::data.pmfcud__scalar_new_dbg_4p,    Mfc80d::mfcud__scalar_new_dbg_4p,
    ORDINAL2(895,909),    &Mfc80d::data.pmfcud__scalar_new_dbg_3p,    Mfc80d::mfcud__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc90Patch [] = {
    ORDINAL(265),         &Mfc90::data.pmfcd_vector_new,              Mfc90::mfcd_vector_new,
    ORDINAL2(798, 776),   &Mfc90::data.pmfcd_scalar_new,              Mfc90::mfcd_scalar_new,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc90dPatch [] = {
    ORDINAL(267),         &Mfc90d::data.pmfcd_vector_new,             Mfc90d::mfcd_vector_new,
    ORDINAL(268),         &Mfc90d::data.pmfcd__vector_new_dbg_4p,     Mfc90d::mfcd__vector_new_dbg_4p,
    ORDINAL(269),         &Mfc90d::data.pmfcd__vector_new_dbg_3p,     Mfc90d::mfcd__vector_new_dbg_3p,
    ORDINAL2(931, 909),   &Mfc90d::data.pmfcd_scalar_new,             Mfc90d::mfcd_scalar_new,
    ORDINAL2(932, 910),   &Mfc90d::data.pmfcd__scalar_new_dbg_4p,     Mfc90d::mfcd__scalar_new_dbg_4p,
    ORDINAL2(933, 911),   &Mfc90d::data.pmfcd__scalar_new_dbg_3p,     Mfc90d::mfcd__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc90uPatch [] = {
    ORDINAL(265),         &Mfc90::data.pmfcud_vector_new,             Mfc90::mfcud_vector_new,
    ORDINAL2(798, 776),   &Mfc90::data.pmfcud_scalar_new,             Mfc90::mfcud_scalar_new,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc90udPatch [] = {
    ORDINAL(267),         &Mfc90d::data.pmfcud_vector_new,            Mfc90d::mfcud_vector_new,
    ORDINAL(268),         &Mfc90d::data.pmfcud__vector_new_dbg_4p,    Mfc90d::mfcud__vector_new_dbg_4p,
    ORDINAL(269),         &Mfc90d::data.pmfcud__vector_new_dbg_3p,    Mfc90d::mfcud__vector_new_dbg_3p,
    ORDINAL2(935, 913),   &Mfc90d::data.pmfcud_scalar_new,            Mfc90d::mfcud_scalar_new,
    ORDINAL2(936, 914),   &Mfc90d::data.pmfcud__scalar_new_dbg_4p,    Mfc90d::mfcud__scalar_new_dbg_4p,
    ORDINAL2(937, 915),   &Mfc90d::data.pmfcud__scalar_new_dbg_3p,    Mfc90d::mfcud__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc100Patch [] = {
    ORDINAL(265),         &Mfc100::data.pmfcd_vector_new,             Mfc100::mfcd_vector_new,
    ORDINAL2(1294, 1272), &Mfc100::data.pmfcd_scalar_new,             Mfc100::mfcd_scalar_new,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc100dPatch [] = {
    ORDINAL(267),         &Mfc100d::data.pmfcd_vector_new,            Mfc100d::mfcd_vector_new,
    ORDINAL(268),         &Mfc100d::data.pmfcd__vector_new_dbg_4p,    Mfc100d::mfcd__vector_new_dbg_4p,
    ORDINAL(269),         &Mfc100d::data.pmfcd__vector_new_dbg_3p,    Mfc100d::mfcd__vector_new_dbg_3p,
    ORDINAL2(1427, 1405), &Mfc100d::data.pmfcd_scalar_new,            Mfc100d::mfcd_scalar_new,
    ORDINAL2(1428, 1406), &Mfc100d::data.pmfcd__scalar_new_dbg_4p,    Mfc100d::mfcd__scalar_new_dbg_4p,
    ORDINAL2(1429, 1407), &Mfc100d::data.pmfcd__scalar_new_dbg_3p,    Mfc100d::mfcd__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc100uPatch [] = {
    ORDINAL(265),         &Mfc100::data.pmfcud_vector_new,            Mfc100::mfcud_vector_new,
    ORDINAL2(1298, 1276), &Mfc100::data.pmfcud_scalar_new,            Mfc100::mfcud_scalar_new,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc100udPatch [] = {
    ORDINAL(267),         &Mfc100d::data.pmfcud_vector_new,           Mfc100d::mfcud_vector_new,
    ORDINAL(268),         &Mfc100d::data.pmfcud__vector_new_dbg_4p,   Mfc100d::mfcud__vector_new_dbg_4p,
    ORDINAL(269),         &Mfc100d::data.pmfcud__vector_new_dbg_3p,   Mfc100d::mfcud__vector_new_dbg_3p,
    ORDINAL2(1434, 1412), &Mfc100d::data.pmfcud_scalar_new,           Mfc100d::mfcud_scalar_new,
    ORDINAL2(1435, 1413), &Mfc100d::data.pmfcud__scalar_new_dbg_4p,   Mfc100d::mfcud__scalar_new_dbg_4p,
    ORDINAL2(1436, 1414), &Mfc100d::data.pmfcud__scalar_new_dbg_3p,   Mfc100d::mfcud__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc110Patch [] = {
    ORDINAL(265),         &Mfc110::data.pmfcd_vector_new,             Mfc110::mfcd_vector_new,
    ORDINAL2(1498, 1476), &Mfc110::data.pmfcd_scalar_new,             Mfc110::mfcd_scalar_new,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc110dPatch [] = {
    ORDINAL(267),         &Mfc110d::data.pmfcd_vector_new,            Mfc110d::mfcd_vector_new,
    ORDINAL(268),         &Mfc110d::data.pmfcd__vector_new_dbg_4p,    Mfc110d::mfcd__vector_new_dbg_4p,
    ORDINAL(269),         &Mfc110d::data.pmfcd__vector_new_dbg_3p,    Mfc110d::mfcd__vector_new_dbg_3p,
    ORDINAL2(1629, 1607), &Mfc110d::data.pmfcd_scalar_new,            Mfc110d::mfcd_scalar_new,
    ORDINAL2(1630, 1608), &Mfc110d::data.pmfcd__scalar_new_dbg_4p,    Mfc110d::mfcd__scalar_new_dbg_4p,
    ORDINAL2(1631, 1609), &Mfc110d::data.pmfcd__scalar_new_dbg_3p,    Mfc110d::mfcd__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc110uPatch [] = {
    ORDINAL(265),         &Mfc110::data.pmfcud_vector_new,            Mfc110::mfcud_vector_new,
    ORDINAL2(1502, 1480), &Mfc110::data.pmfcud_scalar_new,            Mfc110::mfcud_scalar_new,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc110udPatch [] = {
    ORDINAL(267),         &Mfc110d::data.pmfcud_vector_new,           Mfc110d::mfcud_vector_new,
    ORDINAL(268),         &Mfc110d::data.pmfcud__vector_new_dbg_4p,   Mfc110d::mfcud__vector_new_dbg_4p,
    ORDINAL(269),         &Mfc110d::data.pmfcud__vector_new_dbg_3p,   Mfc110d::mfcud__vector_new_dbg_3p,
    ORDINAL2(1636, 1614), &Mfc110d::data.pmfcud_scalar_new,           Mfc110d::mfcud_scalar_new,
    ORDINAL2(1637, 1615), &Mfc110d::data.pmfcud__scalar_new_dbg_4p,   Mfc110d::mfcud__scalar_new_dbg_4p,
    ORDINAL2(1638, 1616), &Mfc110d::data.pmfcud__scalar_new_dbg_3p,   Mfc110d::mfcud__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc120Patch [] = {
    ORDINAL(265),         &Mfc120::data.pmfcd_vector_new,             Mfc120::mfcd_vector_new,
    ORDINAL2(1502, 1480), &Mfc120::data.pmfcd_scalar_new,             Mfc120::mfcd_scalar_new,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc120dPatch [] = {
    ORDINAL(267),         &Mfc120d::data.pmfcd_vector_new,            Mfc120d::mfcd_vector_new,
    ORDINAL(268),         &Mfc120d::data.pmfcd__vector_new_dbg_4p,    Mfc120d::mfcd__vector_new_dbg_4p,
    ORDINAL(269),         &Mfc120d::data.pmfcd__vector_new_dbg_3p,    Mfc120d::mfcd__vector_new_dbg_3p,
    ORDINAL2(1633, 1611), &Mfc120d::data.pmfcd_scalar_new,            Mfc120d::mfcd_scalar_new,
    ORDINAL2(1634, 1612), &Mfc120d::data.pmfcd__scalar_new_dbg_4p,    Mfc120d::mfcd__scalar_new_dbg_4p,
    ORDINAL2(1635, 1613), &Mfc120d::data.pmfcd__scalar_new_dbg_3p,    Mfc120d::mfcd__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc120uPatch [] = {
    ORDINAL(265),         &Mfc120::data.pmfcud_vector_new,            Mfc120::mfcud_vector_new,
    ORDINAL2(1506, 1484), &Mfc120::data.pmfcud_scalar_new,            Mfc120::mfcud_scalar_new,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc120udPatch [] = {
    ORDINAL(267),         &Mfc120d::data.pmfcud_vector_new,           Mfc120d::mfcud_vector_new,
    ORDINAL(268),         &Mfc120d::data.pmfcud__vector_new_dbg_4p,   Mfc120d::mfcud__vector_new_dbg_4p,
    ORDINAL(269),         &Mfc120d::data.pmfcud__vector_new_dbg_3p,   Mfc120d::mfcud__vector_new_dbg_3p,
    ORDINAL2(1640, 1618), &Mfc120d::data.pmfcud_scalar_new,           Mfc120d::mfcud_scalar_new,
    ORDINAL2(1641, 1619), &Mfc120d::data.pmfcud__scalar_new_dbg_4p,   Mfc120d::mfcud__scalar_new_dbg_4p,
    ORDINAL2(1642, 1620), &Mfc120d::data.pmfcud__scalar_new_dbg_3p,   Mfc120d::mfcud__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc140Patch[] = {
    ORDINAL(265),         &Mfc140::data.pmfcd_vector_new,             Mfc140::mfcd_vector_new,
    ORDINAL2(1507, 1485), &Mfc140::data.pmfcd_scalar_new,             Mfc140::mfcd_scalar_new,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc140dPatch[] = {
    ORDINAL(267),         &Mfc140d::data.pmfcd_vector_new,            Mfc140d::mfcd_vector_new,
    ORDINAL(268),         &Mfc140d::data.pmfcd__vector_new_dbg_4p,    Mfc140d::mfcd__vector_new_dbg_4p,
    ORDINAL(269),         &Mfc140d::data.pmfcd__vector_new_dbg_3p,    Mfc140d::mfcd__vector_new_dbg_3p,
    ORDINAL2(1638, 1616), &Mfc140d::data.pmfcd_scalar_new,            Mfc140d::mfcd_scalar_new,
    ORDINAL2(1639, 1617), &Mfc140d::data.pmfcd__scalar_new_dbg_4p,    Mfc140d::mfcd__scalar_new_dbg_4p,
    ORDINAL2(1640, 1618), &Mfc140d::data.pmfcd__scalar_new_dbg_3p,    Mfc140d::mfcd__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc140uPatch[] = {
    ORDINAL(265),         &Mfc140::data.pmfcud_vector_new,            Mfc140::mfcud_vector_new,
    ORDINAL2(1511, 1489), &Mfc140::data.pmfcud_scalar_new,            Mfc140::mfcud_scalar_new,
    NULL,                 NULL,                                         NULL
};

static patchentry_t mfc140udPatch[] = {
    ORDINAL(267),         &Mfc140d::data.pmfcud_vector_new,           Mfc140d::mfcud_vector_new,
    ORDINAL(268),         &Mfc140d::data.pmfcud__vector_new_dbg_4p,   Mfc140d::mfcud__vector_new_dbg_4p,
    ORDINAL(269),         &Mfc140d::data.pmfcud__vector_new_dbg_3p,   Mfc140d::mfcud__vector_new_dbg_3p,
    ORDINAL2(1645, 1623), &Mfc140d::data.pmfcud_scalar_new,           Mfc140d::mfcud_scalar_new,
    ORDINAL2(1646, 1624), &Mfc140d::data.pmfcud__scalar_new_dbg_4p,   Mfc140d::mfcud__scalar_new_dbg_4p,
    ORDINAL2(1647, 1625), &Mfc140d::data.pmfcud__scalar_new_dbg_3p,   Mfc140d::mfcud__scalar_new_dbg_3p,
    NULL,                 NULL,                                         NULL
};

static patchentry_t msvcrtPatch [] = {
    "_calloc_dbg",        &VS60::data.pcrtd__calloc_dbg,       VS60::crtd__calloc_dbg,
    "_malloc_dbg",        &VS60::data.pcrtd__malloc_dbg,       VS60::crtd__malloc_dbg,
    "_realloc_dbg",       &VS60::data.pcrtd__realloc_dbg,      VS60::crtd__realloc_dbg,
    scalar_new_dbg_name,  &VS60::data.pcrtd__scalar_new_dbg,    VS60::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS60::data.pcrtd__vector_new_dbg,    VS60::crtd__vector_new_dbg,
    "calloc",             &VS60::data.pcrtd_calloc,             VS60::crtd_calloc,
    "malloc",             &VS60::data.pcrtd_malloc,             VS60::crtd_malloc,
    "realloc",            &VS60::data.pcrtd_realloc,            VS60::crtd_realloc,
    "_strdup",            &VS60::data.pcrtd__strdup,            VS60::crtd__strdup,
    scalar_new_name,      &VS60::data.pcrtd_scalar_new,         VS60::crtd_scalar_new,
    vector_new_name,      &VS60::data.pcrtd_vector_new,         VS60::crtd_vector_new,
    NULL,                 NULL,                                 NULL
};

static patchentry_t msvcrtdPatch [] = {
    "_calloc_dbg",        &VS60d::data.pcrtd__calloc_dbg,       VS60d::crtd__calloc_dbg,
    "_malloc_dbg",        &VS60d::data.pcrtd__malloc_dbg,       VS60d::crtd__malloc_dbg,
    "_realloc_dbg",       &VS60d::data.pcrtd__realloc_dbg,      VS60d::crtd__realloc_dbg,
    scalar_new_dbg_name,  &VS60d::data.pcrtd__scalar_new_dbg,   VS60d::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS60d::data.pcrtd__vector_new_dbg,   VS60d::crtd__vector_new_dbg,
    "calloc",             &VS60d::data.pcrtd_calloc,            VS60d::crtd_calloc,
    "malloc",             &VS60d::data.pcrtd_malloc,            VS60d::crtd_malloc,
    "realloc",            &VS60d::data.pcrtd_realloc,           VS60d::crtd_realloc,
    "_strdup",            &VS60d::data.pcrtd__strdup,           VS60d::crtd__strdup,
    scalar_new_name,      &VS60d::data.pcrtd_scalar_new,        VS60d::crtd_scalar_new,
    vector_new_name,      &VS60d::data.pcrtd_vector_new,        VS60d::crtd_vector_new,
    NULL,                 NULL,                                 NULL
};

static patchentry_t msvcr70Patch [] = {
    scalar_new_dbg_name,  &VS70::data.pcrtd__scalar_new_dbg,    VS70::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS70::data.pcrtd__vector_new_dbg,    VS70::crtd__vector_new_dbg,
    "calloc",             &VS70::data.pcrtd_calloc,             VS70::crtd_calloc,
    "malloc",             &VS70::data.pcrtd_malloc,             VS70::crtd_malloc,
    "realloc",            &VS70::data.pcrtd_realloc,            VS70::crtd_realloc,
    "_strdup",            &VS70::data.pcrtd__strdup,            VS70::crtd__strdup,
    "_wcsdup",            &VS70::data.pcrtd__wcsdup,            VS70::crtd__wcsdup,
    scalar_new_name,      &VS70::data.pcrtd_scalar_new,         VS70::crtd_scalar_new,
    vector_new_name,      &VS70::data.pcrtd_vector_new,         VS70::crtd_vector_new,
    NULL,                 NULL,                                 NULL
};

static patchentry_t msvcr70dPatch [] = {
    "_calloc_dbg",        &VS70d::data.pcrtd__calloc_dbg,       VS70d::crtd__calloc_dbg,
    "_malloc_dbg",        &VS70d::data.pcrtd__malloc_dbg,       VS70d::crtd__malloc_dbg,
    "_realloc_dbg",       &VS70d::data.pcrtd__realloc_dbg,      VS70d::crtd__realloc_dbg,
    scalar_new_dbg_name,  &VS70d::data.pcrtd__scalar_new_dbg,   VS70d::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS70d::data.pcrtd__vector_new_dbg,   VS70d::crtd__vector_new_dbg,
    "calloc",             &VS70d::data.pcrtd_calloc,            VS70d::crtd_calloc,
    "malloc",             &VS70d::data.pcrtd_malloc,            VS70d::crtd_malloc,
    "realloc",            &VS70d::data.pcrtd_realloc,           VS70d::crtd_realloc,
    "_strdup",            &VS70d::data.pcrtd__strdup,           VS70d::crtd__strdup,
    "_wcsdup",            &VS70d::data.pcrtd__wcsdup,           VS70d::crtd__wcsdup,
    scalar_new_name,      &VS70d::data.pcrtd_scalar_new,        VS70d::crtd_scalar_new,
    vector_new_name,      &VS70d::data.pcrtd_vector_new,        VS70d::crtd_vector_new,
    NULL,                 NULL,                                 NULL
};

static patchentry_t msvcr71Patch [] = {
    scalar_new_dbg_name,  &VS71::data.pcrtd__scalar_new_dbg,    VS71::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS71::data.pcrtd__vector_new_dbg,    VS71::crtd__vector_new_dbg,
    "calloc",             &VS71::data.pcrtd_calloc,             VS71::crtd_calloc,
    "malloc",             &VS71::data.pcrtd_malloc,             VS71::crtd_malloc,
    "realloc",            &VS71::data.pcrtd_realloc,            VS71::crtd_realloc,
    "_strdup",            &VS71::data.pcrtd__strdup,            VS71::crtd__strdup,
    "_wcsdup",            &VS71::data.pcrtd__wcsdup,            VS71::crtd__wcsdup,
    scalar_new_name,      &VS71::data.pcrtd_scalar_new,         VS71::crtd_scalar_new,
    vector_new_name,      &VS71::data.pcrtd_vector_new,         VS71::crtd_vector_new,
    NULL,                 NULL,                                 NULL
};

static patchentry_t msvcr71dPatch [] = {
    "_calloc_dbg",        &VS71d::data.pcrtd__calloc_dbg,       VS71d::crtd__calloc_dbg,
    "_malloc_dbg",        &VS71d::data.pcrtd__malloc_dbg,       VS71d::crtd__malloc_dbg,
    "_realloc_dbg",       &VS71d::data.pcrtd__realloc_dbg,      VS71d::crtd__realloc_dbg,
    "_strdup_dbg",        &VS71d::data.pcrtd__strdup_dbg,       VS71d::crtd__strdup_dbg,
    "_wcsdup_dbg",        &VS71d::data.pcrtd__wcsdup_dbg,       VS71d::crtd__wcsdup_dbg,
    scalar_new_dbg_name,  &VS71d::data.pcrtd__scalar_new_dbg,   VS71d::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS71d::data.pcrtd__vector_new_dbg,   VS71d::crtd__vector_new_dbg,
    "calloc",             &VS71d::data.pcrtd_calloc,            VS71d::crtd_calloc,
    "malloc",             &VS71d::data.pcrtd_malloc,            VS71d::crtd_malloc,
    "realloc",            &VS71d::data.pcrtd_realloc,           VS71d::crtd_realloc,
    "_strdup",            &VS71d::data.pcrtd__strdup,           VS71d::crtd__strdup,
    "_wcsdup",            &VS71d::data.pcrtd__wcsdup,           VS71d::crtd__wcsdup,
    scalar_new_name,      &VS71d::data.pcrtd_scalar_new,        VS71d::crtd_scalar_new,
    vector_new_name,      &VS71d::data.pcrtd_vector_new,        VS71d::crtd_vector_new,
    NULL,                 NULL,                                 NULL
};

static patchentry_t msvcr80Patch [] = {
    scalar_new_dbg_name,  &VS80::data.pcrtd__scalar_new_dbg,    VS80::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS80::data.pcrtd__vector_new_dbg,    VS80::crtd__vector_new_dbg,
    "calloc",             &VS80::data.pcrtd_calloc,             VS80::crtd_calloc,
    "malloc",             &VS80::data.pcrtd_malloc,             VS80::crtd_malloc,
    "realloc",            &VS80::data.pcrtd_realloc,            VS80::crtd_realloc,
    "_strdup",            &VS80::data.pcrtd__strdup,            VS80::crtd__strdup,
    "_wcsdup",            &VS80::data.pcrtd__wcsdup,            VS80::crtd__wcsdup,
    scalar_new_name,      &VS80::data.pcrtd_scalar_new,         VS80::crtd_scalar_new,
    vector_new_name,      &VS80::data.pcrtd_vector_new,         VS80::crtd_vector_new,
    "_aligned_malloc",              &VS80::data.pcrtd_aligned_malloc,                   VS80::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS80::data.pcrtd_aligned_offset_malloc,            VS80::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS80::data.pcrtd_aligned_realloc,                  VS80::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS80::data.pcrtd_aligned_offset_realloc,           VS80::crtd__aligned_offset_realloc,
    NULL,                           NULL,                                               NULL
};

static patchentry_t msvcr80dPatch [] = {
    "_calloc_dbg",        &VS80d::data.pcrtd__calloc_dbg,       VS80d::crtd__calloc_dbg,
    "_malloc_dbg",        &VS80d::data.pcrtd__malloc_dbg,       VS80d::crtd__malloc_dbg,
    "_realloc_dbg",       &VS80d::data.pcrtd__realloc_dbg,      VS80d::crtd__realloc_dbg,
    "_strdup_dbg",        &VS80d::data.pcrtd__strdup_dbg,       VS80d::crtd__strdup_dbg,
    "_wcsdup_dbg",        &VS80d::data.pcrtd__wcsdup_dbg,       VS80d::crtd__wcsdup_dbg,
    scalar_new_dbg_name,  &VS80d::data.pcrtd__scalar_new_dbg,   VS80d::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS80d::data.pcrtd__vector_new_dbg,   VS80d::crtd__vector_new_dbg,
    "calloc",             &VS80d::data.pcrtd_calloc,            VS80d::crtd_calloc,
    "malloc",             &VS80d::data.pcrtd_malloc,            VS80d::crtd_malloc,
    "realloc",            &VS80d::data.pcrtd_realloc,           VS80d::crtd_realloc,
    "_strdup",            &VS80d::data.pcrtd__strdup,           VS80d::crtd__strdup,
    "_wcsdup",            &VS80d::data.pcrtd__wcsdup,           VS80d::crtd__wcsdup,
    scalar_new_name,      &VS80d::data.pcrtd_scalar_new,        VS80d::crtd_scalar_new,
    vector_new_name,      &VS80d::data.pcrtd_vector_new,        VS80d::crtd_vector_new,
    "_aligned_malloc_dbg",          &VS80d::data.pcrtd__aligned_malloc_dbg,             VS80d::crtd__aligned_malloc_dbg,
    "_aligned_offset_malloc_dbg",   &VS80d::data.pcrtd__aligned_offset_malloc_dbg,      VS80d::crtd__aligned_offset_malloc_dbg,
    "_aligned_realloc_dbg",         &VS80d::data.pcrtd__aligned_realloc_dbg,            VS80d::crtd__aligned_realloc_dbg,
    "_aligned_offset_realloc_dbg",  &VS80d::data.pcrtd__aligned_offset_realloc_dbg,     VS80d::crtd__aligned_offset_realloc_dbg,
    "_aligned_malloc",              &VS80d::data.pcrtd_aligned_malloc,                  VS80d::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS80d::data.pcrtd_aligned_offset_malloc,           VS80d::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS80d::data.pcrtd_aligned_realloc,                 VS80d::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS80d::data.pcrtd_aligned_offset_realloc,          VS80d::crtd__aligned_offset_realloc,
    NULL,                           NULL,                                               NULL
};

static patchentry_t msvcr90Patch [] = {
    scalar_new_dbg_name,  &VS90::data.pcrtd__scalar_new_dbg,    VS90::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS90::data.pcrtd__vector_new_dbg,    VS90::crtd__vector_new_dbg,
    "calloc",             &VS90::data.pcrtd_calloc,             VS90::crtd_calloc,
    "malloc",             &VS90::data.pcrtd_malloc,             VS90::crtd_malloc,
    "realloc",            &VS90::data.pcrtd_realloc,            VS90::crtd_realloc,
    "_recalloc",          &VS90::data.pcrtd_recalloc,           VS90::crtd__recalloc,
    "_strdup",            &VS90::data.pcrtd__strdup,            VS90::crtd__strdup,
    "_wcsdup",            &VS90::data.pcrtd__wcsdup,            VS90::crtd__wcsdup,
    scalar_new_name,      &VS90::data.pcrtd_scalar_new,         VS90::crtd_scalar_new,
    vector_new_name,      &VS90::data.pcrtd_vector_new,         VS90::crtd_vector_new,
    "_aligned_malloc",              &VS90::data.pcrtd_aligned_malloc,                   VS90::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS90::data.pcrtd_aligned_offset_malloc,            VS90::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS90::data.pcrtd_aligned_realloc,                  VS90::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS90::data.pcrtd_aligned_offset_realloc,           VS90::crtd__aligned_offset_realloc,
    "_aligned_recalloc",            &VS90::data.pcrtd_aligned_recalloc,                 VS90::crtd__aligned_recalloc,
    "_aligned_offset_recalloc",     &VS90::data.pcrtd_aligned_offset_recalloc,          VS90::crtd__aligned_offset_recalloc,
    NULL,                           NULL,                                               NULL
};

static patchentry_t msvcr90dPatch [] = {
    "_calloc_dbg",        &VS90d::data.pcrtd__calloc_dbg,       VS90d::crtd__calloc_dbg,
    "_malloc_dbg",        &VS90d::data.pcrtd__malloc_dbg,       VS90d::crtd__malloc_dbg,
    "_realloc_dbg",       &VS90d::data.pcrtd__realloc_dbg,      VS90d::crtd__realloc_dbg,
    "_recalloc_dbg",      &VS90d::data.pcrtd__recalloc_dbg,     VS90d::crtd__recalloc_dbg,
    "_strdup_dbg",        &VS90d::data.pcrtd__strdup_dbg,       VS90d::crtd__strdup_dbg,
    "_wcsdup_dbg",        &VS90d::data.pcrtd__wcsdup_dbg,       VS90d::crtd__wcsdup_dbg,
    scalar_new_dbg_name,  &VS90d::data.pcrtd__scalar_new_dbg,   VS90d::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS90d::data.pcrtd__vector_new_dbg,   VS90d::crtd__vector_new_dbg,
    "calloc",             &VS90d::data.pcrtd_calloc,            VS90d::crtd_calloc,
    "malloc",             &VS90d::data.pcrtd_malloc,            VS90d::crtd_malloc,
    "realloc",            &VS90d::data.pcrtd_realloc,           VS90d::crtd_realloc,
    "_recalloc",          &VS90d::data.pcrtd_recalloc,          VS90d::crtd__recalloc,
    "_strdup",            &VS90d::data.pcrtd__strdup,           VS90d::crtd__strdup,
    "_wcsdup",            &VS90d::data.pcrtd__wcsdup,           VS90d::crtd__wcsdup,
    scalar_new_name,      &VS90d::data.pcrtd_scalar_new,        VS90d::crtd_scalar_new,
    vector_new_name,      &VS90d::data.pcrtd_vector_new,        VS90d::crtd_vector_new,
    "_aligned_malloc_dbg",          &VS90d::data.pcrtd__aligned_malloc_dbg,             VS90d::crtd__aligned_malloc_dbg,
    "_aligned_offset_malloc_dbg",   &VS90d::data.pcrtd__aligned_offset_malloc_dbg,      VS90d::crtd__aligned_offset_malloc_dbg,
    "_aligned_realloc_dbg",         &VS90d::data.pcrtd__aligned_realloc_dbg,            VS90d::crtd__aligned_realloc_dbg,
    "_aligned_offset_realloc_dbg",  &VS90d::data.pcrtd__aligned_offset_realloc_dbg,     VS90d::crtd__aligned_offset_realloc_dbg,
    "_aligned_recalloc_dbg",        &VS90d::data.pcrtd__aligned_recalloc_dbg,           VS90d::crtd__aligned_recalloc_dbg,
    "_aligned_offset_recalloc_dbg", &VS90d::data.pcrtd__aligned_offset_recalloc_dbg,    VS90d::crtd__aligned_offset_recalloc_dbg,
    "_aligned_malloc",              &VS90d::data.pcrtd_aligned_malloc,                  VS90d::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS90d::data.pcrtd_aligned_offset_malloc,           VS90d::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS90d::data.pcrtd_aligned_realloc,                 VS90d::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS90d::data.pcrtd_aligned_offset_realloc,          VS90d::crtd__aligned_offset_realloc,
    "_aligned_recalloc",            &VS90d::data.pcrtd_aligned_recalloc,                VS90d::crtd__aligned_recalloc,
    "_aligned_offset_recalloc",     &VS90d::data.pcrtd_aligned_offset_recalloc,         VS90d::crtd__aligned_offset_recalloc,
    NULL,                           NULL,                                               NULL
};

static patchentry_t msvcr100Patch [] = {
    scalar_new_dbg_name,  &VS100::data.pcrtd__scalar_new_dbg,   VS100::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS100::data.pcrtd__vector_new_dbg,   VS100::crtd__vector_new_dbg,
    "calloc",             &VS100::data.pcrtd_calloc,            VS100::crtd_calloc,
    "malloc",             &VS100::data.pcrtd_malloc,            VS100::crtd_malloc,
    "realloc",            &VS100::data.pcrtd_realloc,           VS100::crtd_realloc,
    "_recalloc",          &VS100::data.pcrtd_recalloc,          VS100::crtd__recalloc,
    "_strdup",            &VS100::data.pcrtd__strdup,           VS100::crtd__strdup,
    "_wcsdup",            &VS100::data.pcrtd__wcsdup,           VS100::crtd__wcsdup,
    scalar_new_name,      &VS100::data.pcrtd_scalar_new,        VS100::crtd_scalar_new,
    vector_new_name,      &VS100::data.pcrtd_vector_new,        VS100::crtd_vector_new,
    "_aligned_malloc",              &VS100::data.pcrtd_aligned_malloc,                  VS100::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS100::data.pcrtd_aligned_offset_malloc,           VS100::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS100::data.pcrtd_aligned_realloc,                 VS100::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS100::data.pcrtd_aligned_offset_realloc,          VS100::crtd__aligned_offset_realloc,
    "_aligned_recalloc",            &VS100::data.pcrtd_aligned_recalloc,                VS100::crtd__aligned_recalloc,
    "_aligned_offset_recalloc",     &VS100::data.pcrtd_aligned_offset_recalloc,         VS100::crtd__aligned_offset_recalloc,
    NULL,                           NULL,                                               NULL,
};

static patchentry_t msvcr100dPatch [] = {
    "_calloc_dbg",        &VS100d::data.pcrtd__calloc_dbg,      VS100d::crtd__calloc_dbg,
    "_malloc_dbg",        &VS100d::data.pcrtd__malloc_dbg,      VS100d::crtd__malloc_dbg,
    "_realloc_dbg",       &VS100d::data.pcrtd__realloc_dbg,     VS100d::crtd__realloc_dbg,
    "_recalloc_dbg",      &VS100d::data.pcrtd__recalloc_dbg,    VS100d::crtd__recalloc_dbg,
    "_strdup_dbg",        &VS100d::data.pcrtd__strdup_dbg,      VS100d::crtd__strdup_dbg,
    "_wcsdup_dbg",        &VS100d::data.pcrtd__wcsdup_dbg,      VS100d::crtd__wcsdup_dbg,
    scalar_new_dbg_name,  &VS100d::data.pcrtd__scalar_new_dbg,  VS100d::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS100d::data.pcrtd__vector_new_dbg,  VS100d::crtd__vector_new_dbg,
    "calloc",             &VS100d::data.pcrtd_calloc,           VS100d::crtd_calloc,
    "malloc",             &VS100d::data.pcrtd_malloc,           VS100d::crtd_malloc,
    "realloc",            &VS100d::data.pcrtd_realloc,          VS100d::crtd_realloc,
    "_recalloc",          &VS100d::data.pcrtd_recalloc,         VS100d::crtd__recalloc,
    "_strdup",            &VS100d::data.pcrtd__strdup,          VS100d::crtd__strdup,
    "_wcsdup",            &VS100d::data.pcrtd__wcsdup,          VS100d::crtd__wcsdup,
    scalar_new_name,      &VS100d::data.pcrtd_scalar_new,       VS100d::crtd_scalar_new,
    vector_new_name,      &VS100d::data.pcrtd_vector_new,       VS100d::crtd_vector_new,
    "_aligned_malloc_dbg",          &VS100d::data.pcrtd__aligned_malloc_dbg,            VS100d::crtd__aligned_malloc_dbg,
    "_aligned_offset_malloc_dbg",   &VS100d::data.pcrtd__aligned_offset_malloc_dbg,     VS100d::crtd__aligned_offset_malloc_dbg,
    "_aligned_realloc_dbg",         &VS100d::data.pcrtd__aligned_realloc_dbg,           VS100d::crtd__aligned_realloc_dbg,
    "_aligned_offset_realloc_dbg",  &VS100d::data.pcrtd__aligned_offset_realloc_dbg,    VS100d::crtd__aligned_offset_realloc_dbg,
    "_aligned_recalloc_dbg",        &VS100d::data.pcrtd__aligned_recalloc_dbg,          VS100d::crtd__aligned_recalloc_dbg,
    "_aligned_offset_recalloc_dbg", &VS100d::data.pcrtd__aligned_offset_recalloc_dbg,   VS100d::crtd__aligned_offset_recalloc_dbg,
    "_aligned_malloc",              &VS100d::data.pcrtd_aligned_malloc,                 VS100d::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS100d::data.pcrtd_aligned_offset_malloc,          VS100d::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS100d::data.pcrtd_aligned_realloc,                VS100d::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS100d::data.pcrtd_aligned_offset_realloc,         VS100d::crtd__aligned_offset_realloc,
    "_aligned_recalloc",            &VS100d::data.pcrtd_aligned_recalloc,               VS100d::crtd__aligned_recalloc,
    "_aligned_offset_recalloc",     &VS100d::data.pcrtd_aligned_offset_recalloc,        VS100d::crtd__aligned_offset_recalloc,
    NULL,                           NULL,                                               NULL,
};

static patchentry_t msvcr110Patch [] = {
    scalar_new_dbg_name,  &VS110::data.pcrtd__scalar_new_dbg,   VS110::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS110::data.pcrtd__vector_new_dbg,   VS110::crtd__vector_new_dbg,
    "calloc",             &VS110::data.pcrtd_calloc,            VS110::crtd_calloc,
    "malloc",             &VS110::data.pcrtd_malloc,            VS110::crtd_malloc,
    "realloc",            &VS110::data.pcrtd_realloc,           VS110::crtd_realloc,
    "_recalloc",          &VS110::data.pcrtd_recalloc,          VS110::crtd__recalloc,
    "_strdup",            &VS110::data.pcrtd__strdup,           VS110::crtd__strdup,
    "_wcsdup",            &VS110::data.pcrtd__wcsdup,           VS110::crtd__wcsdup,
    scalar_new_name,      &VS110::data.pcrtd_scalar_new,        VS110::crtd_scalar_new,
    vector_new_name,      &VS110::data.pcrtd_vector_new,        VS110::crtd_vector_new,
    "_aligned_malloc",              &VS110::data.pcrtd_aligned_malloc,              VS110::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS110::data.pcrtd_aligned_offset_malloc,       VS110::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS110::data.pcrtd_aligned_realloc,             VS110::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS110::data.pcrtd_aligned_offset_realloc,      VS110::crtd__aligned_offset_realloc,
    "_aligned_recalloc",            &VS110::data.pcrtd_aligned_recalloc,            VS110::crtd__aligned_recalloc,
    "_aligned_offset_recalloc",     &VS110::data.pcrtd_aligned_offset_recalloc,     VS110::crtd__aligned_offset_recalloc,
    NULL,                           NULL,                                           NULL,
};

static patchentry_t msvcr110dPatch [] = {
    "_calloc_dbg",        &VS110d::data.pcrtd__calloc_dbg,      VS110d::crtd__calloc_dbg,
    "_malloc_dbg",        &VS110d::data.pcrtd__malloc_dbg,      VS110d::crtd__malloc_dbg,
    "_realloc_dbg",       &VS110d::data.pcrtd__realloc_dbg,     VS110d::crtd__realloc_dbg,
    "_recalloc_dbg",      &VS110d::data.pcrtd__recalloc_dbg,    VS110d::crtd__recalloc_dbg,
    "_strdup_dbg",        &VS110d::data.pcrtd__strdup_dbg,      VS110d::crtd__strdup_dbg,
    "_wcsdup_dbg",        &VS110d::data.pcrtd__wcsdup_dbg,      VS110d::crtd__wcsdup_dbg,
    scalar_new_dbg_name,  &VS110d::data.pcrtd__scalar_new_dbg,  VS110d::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS110d::data.pcrtd__vector_new_dbg,  VS110d::crtd__vector_new_dbg,
    "calloc",             &VS110d::data.pcrtd_calloc,           VS110d::crtd_calloc,
    "malloc",             &VS110d::data.pcrtd_malloc,           VS110d::crtd_malloc,
    "realloc",            &VS110d::data.pcrtd_realloc,          VS110d::crtd_realloc,
    "_recalloc",          &VS110d::data.pcrtd_recalloc,         VS110d::crtd__recalloc,
    "_strdup",            &VS110d::data.pcrtd__strdup,          VS110d::crtd__strdup,
    "_wcsdup",            &VS110d::data.pcrtd__wcsdup,          VS110d::crtd__wcsdup,
    scalar_new_name,      &VS110d::data.pcrtd_scalar_new,       VS110d::crtd_scalar_new,
    vector_new_name,      &VS110d::data.pcrtd_vector_new,       VS110d::crtd_vector_new,
    "_aligned_malloc_dbg",          &VS110d::data.pcrtd__aligned_malloc_dbg,            VS110d::crtd__aligned_malloc_dbg,
    "_aligned_offset_malloc_dbg",   &VS110d::data.pcrtd__aligned_offset_malloc_dbg,     VS110d::crtd__aligned_offset_malloc_dbg,
    "_aligned_realloc_dbg",         &VS110d::data.pcrtd__aligned_realloc_dbg,           VS110d::crtd__aligned_realloc_dbg,
    "_aligned_offset_realloc_dbg",  &VS110d::data.pcrtd__aligned_offset_realloc_dbg,    VS110d::crtd__aligned_offset_realloc_dbg,
    "_aligned_recalloc_dbg",        &VS110d::data.pcrtd__aligned_recalloc_dbg,          VS110d::crtd__aligned_recalloc_dbg,
    "_aligned_offset_recalloc_dbg", &VS110d::data.pcrtd__aligned_offset_recalloc_dbg,   VS110d::crtd__aligned_offset_recalloc_dbg,
    "_aligned_malloc",              &VS110d::data.pcrtd_aligned_malloc,                 VS110d::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS110d::data.pcrtd_aligned_offset_malloc,          VS110d::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS110d::data.pcrtd_aligned_realloc,                VS110d::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS110d::data.pcrtd_aligned_offset_realloc,         VS110d::crtd__aligned_offset_realloc,
    "_aligned_recalloc",            &VS110d::data.pcrtd_aligned_recalloc,               VS110d::crtd__aligned_recalloc,
    "_aligned_offset_recalloc",     &VS110d::data.pcrtd_aligned_offset_recalloc,        VS110d::crtd__aligned_offset_recalloc,
    NULL,                           NULL,                                               NULL,
};

static patchentry_t msvcr120Patch [] = {
    scalar_new_dbg_name,  &VS120::data.pcrtd__scalar_new_dbg,   VS120::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS120::data.pcrtd__vector_new_dbg,   VS120::crtd__vector_new_dbg,
    "calloc",             &VS120::data.pcrtd_calloc,            VS120::crtd_calloc,
    "malloc",             &VS120::data.pcrtd_malloc,            VS120::crtd_malloc,
    "realloc",            &VS120::data.pcrtd_realloc,           VS120::crtd_realloc,
    "_recalloc",          &VS120::data.pcrtd_recalloc,          VS120::crtd__recalloc,
    "_strdup",            &VS120::data.pcrtd__strdup,           VS120::crtd__strdup,
    "_wcsdup",            &VS120::data.pcrtd__wcsdup,           VS120::crtd__wcsdup,
    scalar_new_name,      &VS120::data.pcrtd_scalar_new,        VS120::crtd_scalar_new,
    vector_new_name,      &VS120::data.pcrtd_vector_new,        VS120::crtd_vector_new,
    "_aligned_malloc",              &VS120::data.pcrtd_aligned_malloc,              VS120::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS120::data.pcrtd_aligned_offset_malloc,       VS120::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS120::data.pcrtd_aligned_realloc,             VS120::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS120::data.pcrtd_aligned_offset_realloc,      VS120::crtd__aligned_offset_realloc,
    "_aligned_recalloc",            &VS120::data.pcrtd_aligned_recalloc,            VS120::crtd__aligned_recalloc,
    "_aligned_offset_recalloc",     &VS120::data.pcrtd_aligned_offset_recalloc,     VS120::crtd__aligned_offset_recalloc,
    NULL,                           NULL,                                           NULL,
};

static patchentry_t msvcr120dPatch [] = {
    "_calloc_dbg",        &VS120d::data.pcrtd__calloc_dbg,      VS120d::crtd__calloc_dbg,
    "_malloc_dbg",        &VS120d::data.pcrtd__malloc_dbg,      VS120d::crtd__malloc_dbg,
    "_realloc_dbg",       &VS120d::data.pcrtd__realloc_dbg,     VS120d::crtd__realloc_dbg,
    "_recalloc_dbg",      &VS120d::data.pcrtd__recalloc_dbg,    VS120d::crtd__recalloc_dbg,
    "_strdup_dbg",        &VS120d::data.pcrtd__strdup_dbg,      VS120d::crtd__strdup_dbg,
    "_wcsdup_dbg",        &VS120d::data.pcrtd__wcsdup_dbg,      VS120d::crtd__wcsdup_dbg,
    scalar_new_dbg_name,  &VS120d::data.pcrtd__scalar_new_dbg,  VS120d::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &VS120d::data.pcrtd__vector_new_dbg,  VS120d::crtd__vector_new_dbg,
    "calloc",             &VS120d::data.pcrtd_calloc,           VS120d::crtd_calloc,
    "malloc",             &VS120d::data.pcrtd_malloc,           VS120d::crtd_malloc,
    "realloc",            &VS120d::data.pcrtd_realloc,          VS120d::crtd_realloc,
    "_recalloc",          &VS120d::data.pcrtd_recalloc,         VS120d::crtd__recalloc,
    "_strdup",            &VS120d::data.pcrtd__strdup,          VS120d::crtd__strdup,
    "_wcsdup",            &VS120d::data.pcrtd__wcsdup,          VS120d::crtd__wcsdup,
    scalar_new_name,      &VS120d::data.pcrtd_scalar_new,       VS120d::crtd_scalar_new,
    vector_new_name,      &VS120d::data.pcrtd_vector_new,       VS120d::crtd_vector_new,
    "_aligned_malloc_dbg",          &VS120d::data.pcrtd__aligned_malloc_dbg,            VS120d::crtd__aligned_malloc_dbg,
    "_aligned_offset_malloc_dbg",   &VS120d::data.pcrtd__aligned_offset_malloc_dbg,     VS120d::crtd__aligned_offset_malloc_dbg,
    "_aligned_realloc_dbg",         &VS120d::data.pcrtd__aligned_realloc_dbg,           VS120d::crtd__aligned_realloc_dbg,
    "_aligned_offset_realloc_dbg",  &VS120d::data.pcrtd__aligned_offset_realloc_dbg,    VS120d::crtd__aligned_offset_realloc_dbg,
    "_aligned_recalloc_dbg",        &VS120d::data.pcrtd__aligned_recalloc_dbg,          VS120d::crtd__aligned_recalloc_dbg,
    "_aligned_offset_recalloc_dbg", &VS120d::data.pcrtd__aligned_offset_recalloc_dbg,   VS120d::crtd__aligned_offset_recalloc_dbg,
    "_aligned_malloc",              &VS120d::data.pcrtd_aligned_malloc,                 VS120d::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &VS120d::data.pcrtd_aligned_offset_malloc,          VS120d::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &VS120d::data.pcrtd_aligned_realloc,                VS120d::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &VS120d::data.pcrtd_aligned_offset_realloc,         VS120d::crtd__aligned_offset_realloc,
    "_aligned_recalloc",            &VS120d::data.pcrtd_aligned_recalloc,               VS120d::crtd__aligned_recalloc,
    "_aligned_offset_recalloc",     &VS120d::data.pcrtd_aligned_offset_recalloc,        VS120d::crtd__aligned_offset_recalloc,
    NULL,                           NULL,                                               NULL,
};

static patchentry_t ucrtbasePatch[] = {
    scalar_new_dbg_name,  &UCRT::data.pcrtd__scalar_new_dbg,   UCRT::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &UCRT::data.pcrtd__vector_new_dbg,   UCRT::crtd__vector_new_dbg,
    "calloc",             &UCRT::data.pcrtd_calloc,            UCRT::crtd_calloc,
    "malloc",             &UCRT::data.pcrtd_malloc,            UCRT::crtd_malloc,
    "realloc",            &UCRT::data.pcrtd_realloc,           UCRT::crtd_realloc,
    "_recalloc",          &UCRT::data.pcrtd_recalloc,          UCRT::crtd__recalloc,
    "_strdup",            &UCRT::data.pcrtd__strdup,           UCRT::crtd__strdup,
    "_wcsdup",            &UCRT::data.pcrtd__wcsdup,           UCRT::crtd__wcsdup,
    scalar_new_name,      &UCRT::data.pcrtd_scalar_new,        UCRT::crtd_scalar_new,
    vector_new_name,      &UCRT::data.pcrtd_vector_new,        UCRT::crtd_vector_new,
    "_aligned_malloc",              &UCRT::data.pcrtd_aligned_malloc,              UCRT::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &UCRT::data.pcrtd_aligned_offset_malloc,       UCRT::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &UCRT::data.pcrtd_aligned_realloc,             UCRT::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &UCRT::data.pcrtd_aligned_offset_realloc,      UCRT::crtd__aligned_offset_realloc,
    "_aligned_recalloc",            &UCRT::data.pcrtd_aligned_recalloc,            UCRT::crtd__aligned_recalloc,
    "_aligned_offset_recalloc",     &UCRT::data.pcrtd_aligned_offset_recalloc,     UCRT::crtd__aligned_offset_recalloc,
    NULL,                           NULL,                                           NULL,
};

static patchentry_t ucrtbasedPatch[] = {
    "_calloc_dbg",        &UCRTd::data.pcrtd__calloc_dbg,      UCRTd::crtd__calloc_dbg,
    "_malloc_dbg",        &UCRTd::data.pcrtd__malloc_dbg,      UCRTd::crtd__malloc_dbg,
    "_realloc_dbg",       &UCRTd::data.pcrtd__realloc_dbg,     UCRTd::crtd__realloc_dbg,
    "_recalloc_dbg",      &UCRTd::data.pcrtd__recalloc_dbg,    UCRTd::crtd__recalloc_dbg,
    "_strdup_dbg",        &UCRTd::data.pcrtd__strdup_dbg,      UCRTd::crtd__strdup_dbg,
    "_wcsdup_dbg",        &UCRTd::data.pcrtd__wcsdup_dbg,      UCRTd::crtd__wcsdup_dbg,
    scalar_new_dbg_name,  &UCRTd::data.pcrtd__scalar_new_dbg,  UCRTd::crtd__scalar_new_dbg,
    vector_new_dbg_name,  &UCRTd::data.pcrtd__vector_new_dbg,  UCRTd::crtd__vector_new_dbg,
    "calloc",             &UCRTd::data.pcrtd_calloc,           UCRTd::crtd_calloc,
    "malloc",             &UCRTd::data.pcrtd_malloc,           UCRTd::crtd_malloc,
    "realloc",            &UCRTd::data.pcrtd_realloc,          UCRTd::crtd_realloc,
    "_recalloc",          &UCRTd::data.pcrtd_recalloc,         UCRTd::crtd__recalloc,
    "_strdup",            &UCRTd::data.pcrtd__strdup,          UCRTd::crtd__strdup,
    "_wcsdup",            &UCRTd::data.pcrtd__wcsdup,          UCRTd::crtd__wcsdup,
    scalar_new_name,      &UCRTd::data.pcrtd_scalar_new,       UCRTd::crtd_scalar_new,
    vector_new_name,      &UCRTd::data.pcrtd_vector_new,       UCRTd::crtd_vector_new,
    "_aligned_malloc_dbg",          &UCRTd::data.pcrtd__aligned_malloc_dbg,            UCRTd::crtd__aligned_malloc_dbg,
    "_aligned_offset_malloc_dbg",   &UCRTd::data.pcrtd__aligned_offset_malloc_dbg,     UCRTd::crtd__aligned_offset_malloc_dbg,
    "_aligned_realloc_dbg",         &UCRTd::data.pcrtd__aligned_realloc_dbg,           UCRTd::crtd__aligned_realloc_dbg,
    "_aligned_offset_realloc_dbg",  &UCRTd::data.pcrtd__aligned_offset_realloc_dbg,    UCRTd::crtd__aligned_offset_realloc_dbg,
    "_aligned_recalloc_dbg",        &UCRTd::data.pcrtd__aligned_recalloc_dbg,          UCRTd::crtd__aligned_recalloc_dbg,
    "_aligned_offset_recalloc_dbg", &UCRTd::data.pcrtd__aligned_offset_recalloc_dbg,   UCRTd::crtd__aligned_offset_recalloc_dbg,
    "_aligned_malloc",              &UCRTd::data.pcrtd_aligned_malloc,                 UCRTd::crtd__aligned_malloc,
    "_aligned_offset_malloc",       &UCRTd::data.pcrtd_aligned_offset_malloc,          UCRTd::crtd__aligned_offset_malloc,
    "_aligned_realloc",             &UCRTd::data.pcrtd_aligned_realloc,                UCRTd::crtd__aligned_realloc,
    "_aligned_offset_realloc",      &UCRTd::data.pcrtd_aligned_offset_realloc,         UCRTd::crtd__aligned_offset_realloc,
    "_aligned_recalloc",            &UCRTd::data.pcrtd_aligned_recalloc,               UCRTd::crtd__aligned_recalloc,
    "_aligned_offset_recalloc",     &UCRTd::data.pcrtd_aligned_offset_recalloc,        UCRTd::crtd__aligned_offset_recalloc,
    NULL,                           NULL,                                               NULL,
};

patchentry_t VisualLeakDetector::m_ntdllPatch [] = {
    "RtlAllocateHeap",    NULL, _RtlAllocateHeap,
    "RtlFreeHeap",        NULL, _RtlFreeHeap,
    "RtlReAllocateHeap",  NULL, _RtlReAllocateHeap,
    NULL,                 NULL, NULL
};

patchentry_t VisualLeakDetector::m_ole32Patch [] = {
    "CoGetMalloc",        NULL, _CoGetMalloc,
    "CoTaskMemAlloc",     NULL, _CoTaskMemAlloc,
    "CoTaskMemRealloc",   NULL, _CoTaskMemRealloc,
    NULL,                 NULL, NULL
};

moduleentry_t VisualLeakDetector::m_patchTable [] = {
    // Win32 heap APIs.
    "kernel32.dll", FALSE,  0x0, m_kernelbasePatch, // we patch this record on Win7 and higher
    "kernel32.dll", FALSE,  0x0, m_kernel32Patch,

    // MFC new operators (exported by ordinal).
    "mfc42.dll",    TRUE,   0x0, mfc42Patch,
    "mfc42d.dll",   TRUE,   0x0, mfc42dPatch,
    "mfc42u.dll",   TRUE,   0x0, mfc42uPatch,
    "mfc42ud.dll",  TRUE,   0x0, mfc42udPatch,
    "mfc70.dll",    TRUE,   0x0, mfc70Patch,
    "mfc70d.dll",   TRUE,   0x0, mfc70dPatch,
    "mfc70u.dll",   TRUE,   0x0, mfc70uPatch,
    "mfc70ud.dll",  TRUE,   0x0, mfc70udPatch,
    "mfc71.dll",    TRUE,   0x0, mfc71Patch,
    "mfc71d.dll",   TRUE,   0x0, mfc71dPatch,
    "mfc71u.dll",   TRUE,   0x0, mfc71uPatch,
    "mfc71ud.dll",  TRUE,   0x0, mfc71udPatch,
    "mfc80.dll",    TRUE,   0x0, mfc80Patch,
    "mfc80d.dll",   TRUE,   0x0, mfc80dPatch,
    "mfc80u.dll",   TRUE,   0x0, mfc80uPatch,
    "mfc80ud.dll",  TRUE,   0x0, mfc80udPatch,
    "mfc90.dll",    TRUE,   0x0, mfc90Patch,
    "mfc90d.dll",   TRUE,   0x0, mfc90dPatch,
    "mfc90u.dll",   TRUE,   0x0, mfc90uPatch,
    "mfc90ud.dll",  TRUE,   0x0, mfc90udPatch,
    "mfc100.dll",   TRUE,   0x0, mfc100Patch,
    "mfc100d.dll",  TRUE,   0x0, mfc100dPatch,
    "mfc100u.dll",  TRUE,   0x0, mfc100uPatch,
    "mfc100ud.dll", TRUE,   0x0, mfc100udPatch,
    "mfc110.dll",   TRUE,   0x0, mfc110Patch,
    "mfc110d.dll",  TRUE,   0x0, mfc110dPatch,
    "mfc110u.dll",  TRUE,   0x0, mfc110uPatch,
    "mfc110ud.dll", TRUE,   0x0, mfc110udPatch,
    "mfc120.dll",   TRUE,   0x0, mfc120Patch,
    "mfc120d.dll",  TRUE,   0x0, mfc120dPatch,
    "mfc120u.dll",  TRUE,   0x0, mfc120uPatch,
    "mfc120ud.dll", TRUE,   0x0, mfc120udPatch,
    "mfc140.dll",   TRUE,   0x0, mfc140Patch,
    "mfc140d.dll",  TRUE,   0x0, mfc140dPatch,
    "mfc140u.dll",  TRUE,   0x0, mfc140uPatch,
    "mfc140ud.dll", TRUE,   0x0, mfc140udPatch,

    // CRT new operators and heap APIs.
    "msvcrt.dll",   FALSE,  0x0, msvcrtPatch,
    "msvcrtd.dll",  FALSE,  0x0, msvcrtdPatch,
    "msvcr70.dll",  FALSE,  0x0, msvcr70Patch,
    "msvcr70d.dll", FALSE,  0x0, msvcr70dPatch,
    "msvcr71.dll",  FALSE,  0x0, msvcr71Patch,
    "msvcr71d.dll", FALSE,  0x0, msvcr71dPatch,
    "msvcr80.dll",  FALSE,  0x0, msvcr80Patch,
    "msvcr80d.dll", FALSE,  0x0, msvcr80dPatch,
    "msvcr90.dll",  FALSE,  0x0, msvcr90Patch,
    "msvcr90d.dll", FALSE,  0x0, msvcr90dPatch,
    "msvcr100.dll", FALSE,  0x0, msvcr100Patch,
    "msvcr100d.dll",FALSE,  0x0, msvcr100dPatch,
    "msvcr110.dll", FALSE,  0x0, msvcr110Patch,
    "msvcr110d.dll",FALSE,  0x0, msvcr110dPatch,
    "msvcr120.dll", FALSE,  0x0, msvcr120Patch,
    "msvcr120d.dll",FALSE,  0x0, msvcr120dPatch,
    "ucrtbase.dll", FALSE,  0x0, ucrtbasePatch,
    "ucrtbased.dll",FALSE,  0x0, ucrtbasedPatch,

    // NT APIs.
    "ntdll.dll",    FALSE,  0x0, m_ntdllPatch,

    // COM heap APIs.
    "ole32.dll",    FALSE,  0x0, m_ole32Patch
};
