Visual Leak Detector Example Console Application (Version 1.0)

  Example Program Using Visual Leak Detector in a Console Application


About The Example Program:
--------------------------
This is an example Visual C++ 6.0 console application project that
implements the Visual Leak Detector. It might be a good idea to read the
instructions for using Visual Leak Detector (see below) before you try to
build the project, just to be sure you've got the Debug Help library
installed and ready for use with Visual C++. 

The project actually contains two separate programs. A C program and a
C++ program. The C program is in main.c while the C++ program is in
main.cpp. You can select which one you want to build by selecting the
corresponding project configuration as the active configuration. The purpose
behind including both C and C++ versions is to demonstrate Visual Leak
Detector's compatibility with C's malloc and free as well as C++'s new and
delete.

After the project is built, running the program will display a simple
"Hello World!" message. But a small amount of memory (containing the
"Hello World!" string) will be leaked. If you run the program under the
Visual C++ debugger, you should see a memory leak report in the debug
output window after the program exits. The report will include detailed
stack traces of the calls that allocated the leaked memory blocks.

Clicking on a source file/line number in the stack trace will take you to
that file and line in the editor. This allows you to quickly see where in
your program the memory was allocated and how it got there.


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

  + Visual Leak Detector has an elegant interface: it doesn't require you to
    modify ANY of the source files you are debugging. You don't need to add
    any #includes, #defines, global variables, or function calls.
  + In addition to providing stack traces with source files, line numbers
    and function names, Visual Leak Detector also provides data dumps.
  + It works with both C and C++ programs (compatible with both malloc/free
    and new/delete).
  + It is well documented, so it is easy to customize it to suit your needs.


How to Use Visual Leak Detector:
--------------------------------
The hardest part about getting started with Visual Leak Detector is getting your
build environment correctly set up. But if you follow these instructions
carefully, the process should be fairly painless. Once your build environment
is set up and you are able to succesfully compile VLD, using it with your
projects is a snap.

  1) Visual Leak Detector requires the Debug Help Library (dbghelp.dll) version
     5.1 or later. Various versions of this DLL are shipped with the Windows
     operating systems. The latest version is always included with Debugging
     Tools for Windows.
     * Windows XP users should already have a new enough version of dbghelp.dll
       in WINDOWS\system32. So, if you're running Windows XP, you don't need to
       install Debugging Tools for Windows.
     * Windows 2000 shipped with an older version of dbghelp.dll. To use VLD on
       Windows 2000, you must get a newer version (5.1 or newer). The best way
       is to download Debugging Tools for Windows. After installing it, be sure
       to copy dbghelp.dll from the Debugging Tools for Windows installation
       directory to the directory where the executable you are debugging
       resides.
     * Windows Server 2003... I don't know about. But I'm fairly certain it
       ships with a newer version of dbghelp.dll, so Windows 2003 users probably
       don't need to install Debugging Tools for Windows.

  2) To successfully compile VLD and link it with the Debug Help Library
     (dbghelp.dll) you'll need recent versions of the dbghelp.h header and
     dbghelp.lib library files. The best way to get these two files is to
     install the Windows XP SP2 Platform SDK*. This SDK is compatible with
     Windows XP, Windows 2000, and Windows Server 2003. If you're debugging an
     application for Windows Server 2003, the Windows Server 2003 Platform SDK
     ought to have the required header and library files as well, but I haven't
     tried it myself so I can't guarantee it will work. Both of these SDKs can
     be downloaded from Platform SDK Update.

  3) Once you have the Platform SDK installed, you'll need to make Visual C++
     aware of where it can find the new dbghelp.h header and dbghelp.lib library
     files. To do this, add the "include" and "lib" subdirectories from the
     Platform SDK installation directory to the include and library search paths
     in Visual C++:
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
ready. The next step is to start using VLD with your project:

  1) To start using VLD with your project all you need to do is compile VLD with
     your executable. The easiest way to do that is to just add vld.cpp to your
     project. That's it. You don't need to #include any headers or anything.
  2) Run your program under the Visual C++ debugger. When your program exits, if
     there were any memory leaks, then VLD will display a memory leak report to
     the debug output window. Double-click on a source file/line number in the
     Call Stack section of the report to jump to that line of code in the
     editor.


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


Caveats:
--------
In order to be successful at detecting leaks, VLD's code must run before any of
the code being debugged. Most often this will happen without a hitch. However,
there is one rare instance I know of where this might not happen: if any global
objects in the program have been placed in the "compiler" initialization area.
However, user global objects are never placed in this area by default. They must
be manually placed there by using the "#pragma init_seg(compiler)" directive. As
long as you are not using this directive then VLD will not have any problem
running before your code. If you are using it, then you should take whatever
measures are necessary to construct the global VisualLeakDetector object before
your first global object is constructed.


Frequently Asked Questions:
---------------------------

Q: Is Visual Leak Detector compatible with non-Microsoft platforms?

     No. It is designed specifically for use with Visual C++, and it depends on
     heap debugging functions found only in Microsoft's C runtime library. It's
     called "Visual" Leak Detector for a reason.

Q: When compiling VLD, I get Compile Error C1083: "Cannot open include file:
   'dbgint.h': No such file or directory"

     This will happen if the CRT source files are not installed. These are an
     optional part of the installation when you first install Visual C++. Re-run
     the Visual C++ installation, if needed, to install them. If the CRT sources
     are already installed, make sure the CRT\src subdirectory of the Visual C++
     installation directory is in Visual C++'s include search path.

Q: when running my program with VLD compiled into it I get an error message
   saying, "the procedure entry point SymFromAddr could not be located in the
   dynamic link library dbghelp.dll".

     This typically only happens on Windows 2000 clients. It will happen if the
     Debug Help Library is out-of-date. Download the latest version of
     dbghelp.dll from Debugging Tools for Windows. If you've already downloaded
     it, be sure you've copied dbghelp.dll from the Debugging Tools for Windows
     installation directory to the directory where your executable resides.

Q: I get Compile Error C2059: "syntax error : '('," and a whole bunch of other
   compiler errors when compiling dbghelp.h.

     Visual C++ is including the wrong copy of dbghelp.h. Be sure that the
     include search path points to the Platform SDK "include" subdirectory, not
     the "sdk\inc" subdirectory of "Debugging Tools for Windows".

Q: I get a compile error at the very end of vld.cpp, saying, "C1010: unexpected
   end of file while looking for precompiled header directive"

     This is a common error when compiling VLD in an MFC application. MFC
     projects use precompiled headers by default. VLD is not designed to use
     precompiled headers, so precompiled headers must be disabled for vld.cpp.
     To turn off precompiled headers in VC++ 6.0, go to Project->Settings, then
     select the C/C++ Tab. Select precompiled headers from the "Category" list.
     Then select vld.cpp in the tree on the left and select the "Not using
     precompiled headers" option.


License:
--------

Visual Leak Detector is distributed under the terms of the GNU Lesser Public
License. See the COPYING.txt file for details.

Contacting the author:
----------------------

Please forward any bug reports, questions, comments or suggestions to
me at dmoulding@gmail.com.

Copyright (c) 2005 Dan Moulding