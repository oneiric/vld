////////////////////////////////////////////////////////////////////////////////
//  $Id: vldapi.cpp,v 1.5 2006/01/15 06:41:43 db Exp $
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

#define VLDBUILD    // Declares that we are building Visual Leak Detector.
#include "vldint.h" // Provides access to the Visual Leak Detector internals.

// Imported global variables.
extern VisualLeakDetector visualleakdetector;

////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector APIs - see vldapi.h for each function's details
//

extern "C" __declspec(dllexport) void VLDDisable ()
{
    if (!visualleakdetector.enabled()) {
        // Already disabled for the current thread.
        return;
    }

    // Disable memory leak detection for the current thread.
    TlsSetValue(visualleakdetector.m_tlsindex, (LPVOID)VLD_TLS_DISABLED);
}

extern "C" __declspec(dllexport) void VLDEnable ()
{
    if (visualleakdetector.enabled()) {
        // Already enabled for the current thread.
        return;
    }

    // Enable memory leak detection for the current thread.
    TlsSetValue(visualleakdetector.m_tlsindex, (LPVOID)VLD_TLS_ENABLED);
    visualleakdetector.m_status &= ~VLD_STATUS_NEVER_ENABLED;
}
