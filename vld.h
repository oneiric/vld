////////////////////////////////////////////////////////////////////////////////
//  $Id: vld.h,v 1.9 2005/05/02 11:22:55 db Exp $
//
//  Visual Leak Detector (Version 0.9i)
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

#ifndef _VLD_H_
#define _VLD_H_

#ifdef _DEBUG

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

////////////////////////////////////////////////////////////////////////////////
//
// Configuration Options
//

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// If VLD_MAX_DATA_DUMP is defined, then the amount of data shown in user-data
// memory dumps will be limited to the specified number of bytes.
#ifdef VLD_MAX_DATA_DUMP
unsigned long _VLD_maxdatadump = VLD_MAX_DATA_DUMP;
#else
unsigned long _VLD_maxdatadump = 0xffffffff;
#endif // VLD_MAX_DATA_DUMP

// If VLD_MAX_TRACE_FRAMES is defined, then the number of frames traced for each
// allocated memory block when walking the stack will be limited to the
// specified number of frames.
#ifdef VLD_MAX_TRACE_FRAMES
unsigned long _VLD_maxtraceframes = VLD_MAX_TRACE_FRAMES;
#else
unsigned long _VLD_maxtraceframes = 0xffffffff;
#endif // VLD_MAX_TRACE_FRAMES

// If VLD_SHOW_USELESS_FRAMES is defined, then all frames traced will be
// displayed, even frames internal to the heap and Visual Leak Detector.
#ifdef VLD_SHOW_USELESS_FRAMES
unsigned char _VLD_showuselessframes = 0x1;
#else 
unsigned char _VLD_showuselessframes = 0x0;
#endif // VLD_SHOW_USELESS_FRAMES

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _DEBUG

#endif // _VLD_H_
