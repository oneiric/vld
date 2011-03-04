////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - Import Library Header
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

#pragma once

#include <windef.h>
#include "vld_def.h"

#ifdef _DEBUG

#pragma comment(lib, "vld.lib")

// Force a symbolic reference to the global VisualLeakDetector class object from
// the DLL. This ensures that the DLL is loaded and linked with the program,
// even if no code otherwise imports any of the DLL's exports.
#pragma comment(linker, "/include:__imp_?vld@@3VVisualLeakDetector@@A")

////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector APIs
//

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// VLDDisable - Disables Visual Leak Detector's memory leak detection at
//   runtime. If memory leak detection is already disabled, then calling this
//   function has no effect.
//
//  Note: In multithreaded programs, this function operates on a per-thread
//    basis. In other words, if you call this function from one thread, then
//    memory leak detection is only disabled for that thread. If memory leak
//    detection is enabled for other threads, then it will remain enabled for
//    those other threads. It was designed to work this way to insulate you,
//    the programmer, from having to ensure thread synchronization when calling
//    VLDEnable() and VLDDisable(). Without this, calling these two functions
//    unsynchronized could result in unpredictable and unintended behavior.
//    But this also means that if you want to disable memory leak detection
//    process-wide, then you need to call this function from every thread in
//    the process.
//
//  Return Value:
//
//    None.
//
__declspec(dllimport) void VLDDisable ();

// VLDEnable - Enables Visual Leak Detector's memory leak detection at runtime.
//   If memory leak detection is already enabled, which it is by default, then
//   calling this function has no effect.
//
//  Note: In multithreaded programs, this function operates on a per-thread
//    basis. In other words, if you call this function from one thread, then
//    memory leak detection is only enabled for that thread. If memory leak
//    detection is disabled for other threads, then it will remain disabled for
//    those other threads. It was designed to work this way to insulate you,
//    the programmer, from having to ensure thread synchronization when calling
//    VLDEnable() and VLDDisable(). Without this, calling these two functions
//    unsynchronized could result in unpredictable and unintended behavior.
//    But this also means that if you want to enable memory leak detection
//    process-wide, then you need to call this function from every thread in
//    the process.
//
//  Return Value:
//
//    None.
//
__declspec(dllimport) void VLDEnable ();

// VLDRestore - Restore Visual Leak Detector's previous state.
//
//  Return Value:
//
//    None.
//
__declspec(dllimport) void VLDRestore ();

// VLDGlobalDisable - Disables Visual Leak Detector's memory leak detection at
//   runtime in all threads. If memory leak detection is already disabled, 
//   then calling this function has no effect.
//
//  Return Value:
//
//    None.
//
__declspec(dllimport) void VLDGlobalDisable ();

// VLDGlobalEnable - Enables Visual Leak Detector's memory leak detection 
//   at runtime in all threads. If memory leak detection is already enabled, 
//   which it is by default, then calling this function has no effect.
//
//  Return Value:
//
//    None.
//
__declspec(dllimport) void VLDGlobalEnable ();

// VLDReportLeaks - Report leaks up to the execution point.
//
//  Return Value:
//
//    None.
//
__declspec(dllimport) void VLDReportLeaks ();


// VLDRefreshModules - Look for recently loaded DLLs and patch them if necessary.
//
//  Return Value:
//
//    None.
//
__declspec(dllimport) void VLDRefreshModules();


// VLDEnableModule - Enable Memory leak checking on the specified module.
//
//  Return Value:
//
//    None.
//

__declspec(dllimport) void VLDEnableModule(HMODULE);


// VLDDisableModule - Disable Memory leak checking on the specified module.
//
//  Return Value:
//
//    None.
//
__declspec(dllimport) void VLDDisableModule(HMODULE);

// VLDGetOptions - Return all current options.
//
//  Return Value:
//
//    Mask of current options.
//
__declspec(dllimport) UINT32 VLDGetOptions();

// VLDGetReportFilename - Return current report filename.
//
// filename: current report filename (max characters - MAX_PATH).
//
//  Return Value:
//
//    None.
//
__declspec(dllimport) void VLDGetReportFilename(WCHAR *filename);

// VLDSetOptions - Update the report options via function call rather than INI file.
//
// option_mask: Only the following flags are checked
// VLD_OPT_AGGREGATE_DUPLICATES
// VLD_OPT_SAFE_STACK_WALK
// VLD_OPT_SLOW_DEBUGGER_DUMP
// VLD_OPT_TRACE_INTERNAL_FRAMES
// VLD_OPT_START_DISABLED
//
// maxDataDump: maximum number of user-data bytes to dump for each leaked block.
//
// maxTraceFrames: maximum number of frames per stack trace for each leaked block.
//
//  Return Value:
//
//    None.
//
__declspec(dllimport) void VLDSetOptions(UINT32 option_mask, SIZE_T maxDataDump, UINT32 maxTraceFrames);

// VLDSetIncludeModules - Set list of modules included in leak detection.
//
// modules: list of modules to be forcefully included in leak detection.
//
//  Return Value:
//
//    None.
//
__declspec(dllimport) void VLDSetIncludeModules(CONST WCHAR *modules);

// VLDGetIncludeModules - Return current list of included modules.
//
// modules: destination string for list of included modules (maximum length 512 characters).
//
// size: maximum string size.
//
//  Return Value:
//
//    None.
//
__declspec(dllimport) BOOL VLDGetIncludeModules(WCHAR *modules, UINT size);

// VLDSetOptions - Update the report options via function call rather than INI file.
//
// Only the following flags are checked
// VLD_OPT_REPORT_TO_DEBUGGER
// VLD_OPT_REPORT_TO_FILE
// VLD_OPT_REPORT_TO_STDOUT
// VLD_OPT_UNICODE_REPORT
//
// filename is optional and can be NULL.
//
//  Return Value:
//
//    None.
//
__declspec(dllimport) void VLDSetReportOptions(UINT32 option_mask, CONST WCHAR *filename);

#ifdef __cplusplus
}
#endif // __cplusplus

#else // !_DEBUG

#define VLDEnable()
#define VLDDisable()
#define VLDRestore()
#define VLDReportLeaks()
#define VLDRefreshModules()
#define VLDEnableModule(a)
#define VLDDisableModule(b)
#define VLDGetOptions() 0
#define VLDGetReportFilename(a)
#define VLDSetOptions(a, b, c)
#define VLDSetIncludeModules(a)
#define VLDGetIncludeModules(a, b) FALSE
#define VLDSetReportOptions(a, b)

#endif // _DEBUG
