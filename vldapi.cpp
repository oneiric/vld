////////////////////////////////////////////////////////////////////////////////
//  $Id: vldapi.cpp,v 1.10 2006/10/26 23:04:41 dmouldin Exp $
//
//  Visual Leak Detector (Version 1.9a) - Exported APIs
//  Copyright (c) 2005-2006 Dan Moulding
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

#define VLDBUILD     // Declares that we are building Visual Leak Detector.
#include "vldint.h"  // Provides access to the Visual Leak Detector internals.
#include "vldheap.h" // Provides internal new and delete operators.

// Imported global variables.
extern VisualLeakDetector vld;
__declspec(thread) tls_t VisualLeakDetector::m_tls;

////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector APIs - see vldapi.h for each function's details.
//

extern "C" __declspec(dllexport) void VLDDisable ()
{
    if (!vld.enabled()) {
        // Already disabled for the current thread.
        return;
    }

    // Disable memory leak detection for the current thread.
    vld.m_tls.flags &= ~VLD_TLS_ENABLED;
}

extern "C" __declspec(dllexport) void VLDEnable ()
{
    if (vld.enabled()) {
        // Already enabled for the current thread.
        return;
    }

    // Enable memory leak detection for the current thread.
    vld.m_tls.flags |= VLD_TLS_ENABLED;
    vld.m_status &= ~VLD_STATUS_NEVER_ENABLED;
}
