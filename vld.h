////////////////////////////////////////////////////////////////////////////////
//  $Id: vld.h,v 1.2 2005/03/29 14:18:10 db Exp $
//
//  Visual Leak Detector Header (Version 0.9d)
//  Copyright (c) 2005 Dan Moulding
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU Lesser Public License as published by
//  the Free Software Foundation; either version 2.1 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser Public License for more details.
//
//  You should have received a copy of the GNU Lesser Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  See COPYING.txt for the full terms of the GNU Lesser Public License.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _VLD_H_
#define _VLD_H_

#ifdef _DEBUG

#include <cassert>

// Link with the appropriate Visual Leak Detector library. One of: multithreaded
// DLL, multithreaded static, or single threaded. All three link with debug
// versions of the CRT.
#if defined(_DLL)
#pragma comment(lib, "vldmtdll.lib")
#else
#if defined(_MT)
#pragma comment(lib, "vldmt.lib")
#else
#pragma comment(lib, "vld.lib")
#endif // _MT
#endif // _DLL

// Force a symbolic reference to the global VisualLeakDetector class object from
// the library. This enusres that the object is linked with the program, even
// though nobody directly references it outside of the library.
#pragma comment(linker, "/include:?visualleakdetector@@3VVisualLeakDetector@@A")

// Friend functions of the VisualLeakDetector class. These functions provide an
// interface with the Visual Leak Detector library for configuring options.
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
void VLDSetMaxDataDump (unsigned long bytes);
void VLDSetMaxTraceFrames (unsigned long frames);
void VLDShowUselessFrames (unsigned int show);
#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef __cplusplus

////////////////////////////////////////////////////////////////////////////////
//
//  The VisualLeakDetectorConfigurator Class
//
//    This class exists solely to configure the Visual Leak Detector at runtime.
//    A single global instance of this class is constructed. The constructor
//    configures the Visual Leak Detector library according to the preprocessor
//    macro configuration options that have been defined (if any).
//
class VisualLeakDetectorConfigurator {
public:
    VisualLeakDetectorConfigurator () {
        static bool already_instantiated = false;

        // Disallow more than one instantiation of the configurator.
        // If this asserting, then you've included vld.h more than once.
        // Include it in only ONE source file.
        assert(!already_instantiated);
        already_instantiated = true;

        // If any of the preprocessor macros are defined, set the configurator
        // up to configure those options at runtime. This allows the library
        // (which is already compiled) to be configured using the convenience
        // of the preprocessor macros, without having to modify any of the
        // sources being debugged (aside from #including this header).
#ifdef VLD_MAX_DATA_DUMP
        VLDSetMaxDataDump(VLD_MAX_DATA_DUMP);
#endif // VLD_MAX_DATA_DUMP

#ifdef VLD_MAX_TRACE_FRAMES
        VLDSetMaxTraceFrames(VLD_MAX_TRACE_FRAMES);
#endif // VLD_MAX_TRACE_FRAMES

#ifdef VLD_SHOW_USELESS_FRAMES
        VLDShowUselessFrames(1);
#endif // VLD_SHOW_USELESS_FRAMES
    }
};
static VisualLeakDetectorConfigurator visualleakdetectorconfigurator;

#else // __cplusplus

// VLDConfigure - Configures the Visual Leak Detector library according to
//   options specified via preprocessor macros.
//
//  Note: C programs will need to call this function if they want to override
//    the default configuration via the preprocessor macros. For C++ programs
//    calling this function is not necessary (the configurator object does it
//    automatically).
//
//  Return Value:
//
//    None.
//
void VLDConfigure () {
    static unsigned int already_configured = 0;

    // Disallow more than one configuration.
    // If this asserting, then you've called VLDConfigure() more than once.
    // Call it only ONCE from ONE source file.
    assert(!already_configured);
    already_configured = 1;

#ifdef VLD_MAX_DATA_DUMP
    VLDSetMaxDataDump(VLD_MAX_DATA_DUMP);
#endif // VLD_MAX_DATA_DUMP

#ifdef VLD_MAX_TRACE_FRAMES
    VLDSetMaxTraceFrames(VLD_MAX_TRACE_FRAMES);
#endif // VLD_MAX_TRACE_FRAMES

#ifdef VLD_SHOW_USELESS_FRAMES
    VLDShowUselessFrames(1);
#endif // VLD_SHOW_USELESS_FRAMES
}

#endif // __cplusplus

#endif // _DEBUG

#endif // _VLD_H_