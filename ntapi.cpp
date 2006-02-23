////////////////////////////////////////////////////////////////////////////////
//  $Id: ntapi.cpp,v 1.1 2006/02/23 22:10:42 dmouldin Exp $
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

#define VLDBUILD
#include "ntapi.h"

// Global function pointers for explicit dynamic linking with NT APIs that can't
// be load-time linked (there is no import library available for these).
LdrLoadDll_t        LdrLoadDll;
RtlAllocateHeap_t   RtlAllocateHeap;
RtlFreeHeap_t       RtlFreeHeap;
RtlReAllocateHeap_t RtlReAllocateHeap;