Visual Leak Detector MFC Example (Version 1.0.1)

  Example Program Using Visual Leak Detector in an MFC Application


About The MFC Example Program:
------------------------------
This is an example Visual C++ 6.0 MFC project that implements the Visual
Leak Detector.

To build the example project, simply open the project file and start a build.

After the project is built, running the program will pop up a simple
dialog box. If you leave the "Leak Some Memory" box checked, the program
will leak a small amount of memory as it exits. If you run the program
under the Visual C++ debugger, you should see a memory leak report in the
debug output window after the program exits. The report will include
detailed stack traces of the calls that allocated the leaked memory blocks.

Double-clicking on a source file/line number in the stack trace will take you to
that file and line in the editor. This allows you to quickly see where in the
program the memory was allocated and how it got there.


About Visual Leak Detector:
---------------------------
This memory leak detector is superior, in a number of ways, to the memory
leak detection provided natively by MFC or the Microsoft C runtime library.
First, built-in leak detection for non-MFC applications is not enabled by
default. It can be enabled, but implementing it is not very intuitive and
(for Visual C++ 6.0 at least) it doesn't work correctly unless you fix a
couple of bugs in the C runtime library yourself. And even when it is working
correctly its capabilities are somewhat limited. Here is a short list of
capabilities that Visual Leak Detector provides that the built-in leak detection
does not:

  + Provides a complete stack trace for each leaked block, including source
    file and line number information when available.
  + Provides complete data dumps (in hex and ASCII) of leaked blocks.
  + Customizable level of detail in the memory leak report via preprocessor
    macro definitions.

Okay, but how is Visual Leak Detector better than the dozens of other after-
market leak detectors out there?

  + Visual Leak Detector is now packaged as an easy-to-use library. Just copy
    the library to the appropriate directory and include a single header.
    Because the library is already built, you don't need to worry about things
    like installing the Platform SDK or Debugging Tools for Windows.
  + In addition to providing stack traces with source files, line numbers
    and function names, Visual Leak Detector also provides data dumps.
  + It works with both C and C++ programs (compatible with both malloc/free
    and new/delete).
  + The full source code to the library is included and it is well documented,
    so it is easy to customize it to suit your needs.


How To Use Visual Leak Detector:
--------------------------------
Earlier versions of Visual Leak Detector (VLD) had a design that required
absolutely no modification of the source files being debugged. But to acheive
this, VLD had to be compiled into your program from source. It turned out that
building VLD from source was, for most people, the hardest part about using it.

So to make life easier, VLD is now packaged as a library. This means that there
is no longer a need to build VLD from source. But it also means that there is
now a single header file that needs to be included in one of your own source
files that tells your program to link with the VLD library. But beyond that, you
don't need to make any other changes to your sources. In the end, this is a much
easier way to use VLD because it eliminates all of the complexity involved with
building VLD from source.

To use VLD with your project, follow these simple steps:

  1) Copy the VLD library (*.lib) files to your Visual C++ installation's "lib"
     subdirectory.

  2) Copy the VLD header (vld.h) file to your Visual C++ installation's
    "include" subdirectory.

  3) In one of your source files -- preferably the one with your program's main
     entry point -- add the line "#include <vld.h>". It's probably best to add
     this #include before any of the other #includes in your file, but is not
     absolutely necessary. In most cases, the specific order will not matter.

  4) Windows 2000 users will also need to copy dbghelp.dll to the directory
     where the executable being debugged resides. This would also apply to users
     of earlier versions of Windows, but I can't guarantee that VLD even works
     on versions prior to Windows 2000.

When you build debug versions of your program, by including the vld.h header
file, your program will be linked with VLD. When you run your program under the
Visual C++ debugger, VLD will display a memory leak report in the debugger's
output window when your program quits.

If memory leaks were detected, double-clicking on a source file/line number 
within the memory leak report will jump to that line of source in the editor
window. In this way, you can easily navigate the code that led up to the memory
allocation that resulted in a memory leak.


Configuring Visual Leak Detector:
---------------------------------
There are a few OPTIONAL preprocessor macros that you can define to contol
the level of detail provided in memory leak reports.

  + VLD_MAX_TRACE_FRAMES: By default, Visual Leak Detector will trace back
    as far as possible the call stack for each block that is allocated.
    Each frame traced adds additional overhead (in CPU time and memory use)
    to your debug executable. If you'd like to limit this overhead, you
    can define this macro to an integer value. The stack trace will stop
    when it has traced this number of frames. The frame count includes the
    "useless" frames which, by default, are not displayed in the debug
    output window (see VLD_SHOW_USELESS_FRAMES below). Usually, there will
    be about 5 or 6 "useless" frames at the beginning of the call stack.
    Keep this in mind when using this macro, or you may not see the number
    of frames you expect.

  + VLD_MAX_DATA_DUMP: Define this macro to an integer value to limit the
    amount of data displayed in memory block data dumps. When this number of
    bytes of data has been dumped, the dump will stop. This can be useful if
    any of the leaked blocks are very large and the debug output window
    becomes too cluttered. You can define this macro to 0 (zero) if you want
    to suppress the data dumps altogether.

  + VLD_SHOW_USELESS_FRAMES: By default, only "useful" frames are printed in
    the call stacks. Frames that are internal to the heap or Visual Leak
    Detector are not shown. Define this to force all frames of the call
    stacks to be printed. This macro might be useful if you need to debug
    Visual Leak Detector itself or if you want to customize it.

Defining these preprocessor macros is all you need to do to configure VLD with
C++ programs. But if you are using VLD with a C program, in addition to defining
these macros, you'll also need to place a call to VLDConfigure() somewhere in
your program. It's best to place the call to VLDConfigure() as early in your
program as possible.


Caveats:
--------
In order to be successful at detecting leaks, VLD's code must run before any of
the code being debugged. Most often this will happen without a hitch. However,
there are some rare instances where this might not happen: if any global
objects in the program have been placed in the "compiler" or "library"
initialization areas. However, user global objects are never placed in these
areas by default. They must be manually placed there by using the
"#pragma init_seg(...)" directive. As long as you are not using this directive
then VLD will not have any problem running before your code. If you are using it,
then you should take whatever measures are necessary to ensure that objects
from the VLD library are constructed before your first global object is
constructed. If this situation applies to you, but you are not concerned about
the possibility of memory leaks in your global objects' constructors, then
it will not be a problem if your global objects are consructed before VLD's
objects.


Building Visual Leak Detector From Source:
------------------------------------------
Because Visual Leak Detector is open source, the library can be built from
source if you want to tweak it to your liking. The hardest part about building
the VLD libraries from source is getting your build environment correctly set
up. But if you follow these instructions carefully, the  process should be
fairly painless.

  1) Visual Leak Detector depends on the Debug Help Library (dbghelp.dll)
     version 5.1 or later. Various versions of this DLL are shipped with the
     Windows operating systems. The latest version is always included with
     Debugging Tools for Windows. Version 6.3 of the DLL is included with
     VLD.
     * Windows XP users should already have a new enough version of dbghelp.dll
       in WINDOWS\system32. So, if you're running Windows XP, you don't need to
       do anything with the dbghelp.dll included with VLD.
     * Windows 2000 shipped with an older version of dbghelp.dll. To build VLD
       on Windows 2000, you must use a newer version (5.1 or newer). The best
       way is to use the copy of dbghelp.dll included with VLD (version 6.3).
       Place the included copy of the DLL in the directory where the executable
       you are debugging resides.
     * Windows Server 2003... I don't know about. But I'm fairly certain it
       ships with a newer version of dbghelp.dll, so Windows 2003 users probably
       don't need to do anything with the DLL.

  2) The Debug Help Library header file (dbghelp.h) won't compile unless you
     have a recent Platform SDK installed. I recommend installing the Windows XP
     SP2 Platform SDK. This SDK is compatible with Windows XP, Windows 2000, and
     Windows Server 2003. If you're debugging an application for Windows Server
     2003, the Windows Server 2003 Platform SDK will probably work as well, but
     I haven't tried it myself so I can't guarantee it will work. Both of these
     SDKs can be downloaded from Platform SDK Update.

  3) Once you have the Platform SDK installed, you'll need to make Visual C++
     aware of where it can find the updated headers and libraries. To do this,
     add the "include" and "lib" subdirectories from the Platform SDK
     installation directory to the include and library search paths in Visual
     C++:
     * Visual C++ 7: Go to Project Properties -> C/C++ -> General -> Additional
       Include Directories and add the "include" subdirectory from the Platform
       SDK. Make sure it's at the top of the list. Do the same for the "lib"
       subdirectory, but add it to Library Directories instead of Include
       Directories.
     * Visual C++ 6: Go to Tools -> Options -> Directories. Select "Include
       files" from the "Show Directories For" drop-down menu. Add the "include"
       subdirectory from the Platform SDK. Make sure it's at the top of the
       list. Do the same for the "lib" subdirectory, but add it to the "Library
       files" list instead of the "Include files" list.

  4) VLD also depends on a couple of other header files (dbgint.h and mtdll.h)
     that will only be installed if you elected to install the CRT source files
     when you installed Visual C++. If you didn't install the CRT sources,
     you'll need to re-run the Visual C++ installer and install them. If you are
     not sure whether you installed the CRT sources when you installed Visual
     C++, check to see if dbgint.h and mtdll.h exist in the CRT\src subdirectory
     of your Visual C++ installation. If those files are missing, or you don't
     have a CRT\src directory, then chances are you need to re-install Visual
     C++ with the CRT sources selected.

Once you've completed all of the above steps, your build environment should be
ready. To build VLD, just open the vld.dsp project and do a batch build to
build all six of the configurations:

  * The three debug configurations are for building versions of the library that
    have debugging information so that VLD itself can be conveniently debugged.

  * The three release configurations build the library for use in debugging
    other programs. There are three configurations each: one for each method of
    linking with the C runtime library (single-threaded, multithreaded, and
    DLL). When linking the VLD library against a program, the vld.h header file
    detects how the program is linking to the C runtime library and selects the
    appropriate VLD library from the three possible choices.

Note that when VLD is built, it always links against *debug* versions of the C
runtime libraries -- even the release builds of VLD do. This is a requirement.
VLD depends on features of the heap that are only present in debug versions of
the C runtime.

  Miscellaneous Notes on Building from Source
  -------------------------------------------
  VLD can also be directly linked to your program as a regular object (.obj)
  file if you want. Just add vld.cpp to an existing project. If your build
  environment is setup correctly, then vld.obj should effortlessly link with the
  program. I don't think there's any advantage to doing it this way, versus
  linking with the library.


Frequently Asked Questions:
---------------------------

Q: Is Visual Leak Detector compatible with non-Microsoft platforms?

     No. It is designed specifically for use with Visual C++, and it depends on
     heap debugging functions found only in Microsoft's C runtime library. It's
     called "Visual" Leak Detector for a reason.

Q: when running my program with VLD linked to it I get an error message saying,
   "the procedure entry point SymFromAddr could not be located in the dynamic
   link library dbghelp.dll".

     This typically only happens on Windows 2000 clients. It will happen if the
     Debug Help Library is out-of-date. Copy the included version of dbghelp.dll
     (version 6.3) to the directory where the executable you are debugging
     resides. If dbghelp.dll is missing for some reason, you can get it by
     installing Debugging Tools for Windows. I recommend installing version 6.3.

Q: When building VLD from source, I get Compile Error C1083: "Cannot open
   include file: 'dbgint.h': No such file or directory"

     This will happen if the CRT source files are not installed. These are an
     optional part of the installation when you first install Visual C++. Re-run
     the Visual C++ installation, if needed, to install them. If the CRT sources
     are already installed, make sure the CRT\src subdirectory of the Visual C++
     installation directory is in Visual C++'s include search path.

Q: When building VLD from source, I get Compile Error C2059: "syntax error :..."
   and a whole bunch of other compiler errors when compiling dbghelp.h.

     Visual C++ is including the wrong copy of dbghelp.h. Be sure that the
     Platform SDK "include" subdirectory is the first directory in Visual C++'s
     include search path.


License:
--------

Visual Leak Detector is distributed under the terms of the GNU Lesser General
Public License. See the COPYING.txt file for details.

The Debug Help Library (dbghelp.dll) distributed with this software is not part
of Visual Leak Detector and is not covered under the terms of the GNU Lesser
General Public License. It is a separately copyrighted work of Microsoft
Corporation. Microsoft reserves all its rights to its copyright in the Debug
Help Library. Neither your use of the Visual Leak Detector software, nor the
GNU Lesser General Public license grant you any rights to use the Debug Help
Library in ANY WAY (for example, redistributing it) that would infringe upon
Microsoft Corporation's copyright in the Debug Help Library.


Contacting The author:
----------------------

Please forward any bug reports, questions, comments or suggestions to me at
dmoulding@gmail.com.

Copyright (c) 2005 Dan Moulding