////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - Exported APIs
//  Copyright (c) 2005-2011 Dan Moulding, Arkadiy Shapkin, Laurent Lessieux
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

#define VLDBUILD     // Declares that we are building Visual Leak Detector.
#include "vldint.h"  // Provides access to the Visual Leak Detector internals.
#include "vldheap.h" // Provides internal new and delete operators.

// Imported global variables.
extern VisualLeakDetector vld;

////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector APIs - see vldapi.h for each function's details.
//

extern "C" {

__declspec(dllexport) void VLDDisable ()
{
    vld.DisableLeakDetection();
}

__declspec(dllexport) void VLDEnable ()
{
    vld.EnableLeakDetection();
}

__declspec(dllexport) void VLDRestore ()
{
    vld.RestoreLeakDetectionState();
}

__declspec(dllexport) void VLDGlobalDisable ()
{
    vld.GlobalDisableLeakDetection();
}

__declspec(dllexport) void VLDGlobalEnable ()
{
    vld.GlobalEnableLeakDetection();
}

__declspec(dllexport) void VLDReportLeaks ()
{
    vld.ReportLeaks();
}

__declspec(dllexport) void VLDRefreshModules()
{
    vld.RefreshModules();
}

__declspec(dllexport) void VLDEnableModule(HMODULE module)
{
    vld.EnableModule(module);
}

__declspec(dllexport) void VLDDisableModule(HMODULE module)
{
    vld.DisableModule(module);
}

__declspec(dllexport) UINT32 VLDGetOptions()
{
    return vld.GetOptions();
}

__declspec(dllexport) void VLDGetReportFilename(WCHAR *filename)
{
    vld.GetReportFilename(filename);
}

__declspec(dllexport) void VLDSetOptions(UINT32 option_mask, SIZE_T maxDataDump, UINT32 maxTraceFrames)
{
    vld.SetOptions(option_mask, maxDataDump, maxTraceFrames);
}

__declspec(dllexport) void VLDSetModulesList(CONST WCHAR *modules, BOOL includeModules)
{
    vld.SetModulesList(modules, includeModules);
}

__declspec(dllexport) BOOL VLDGetModulesList(WCHAR *modules, UINT size)
{
    return vld.GetModulesList(modules, size);
}

__declspec(dllexport) void VLDSetReportOptions(UINT32 option_mask, CONST WCHAR *filename)
{
    vld.SetReportOptions(option_mask,filename);
}

__declspec(dllexport) void VLDResolveCallstacks()
{
	vld.ResolveCallstacks();
}

}
