Visual Leak Detector MFC Example (Version 1.0)

  Example Program Using Visual Leak Detector in an MFC Application

About The MFC Example Program:
------------------------------
This is an example Visual C++ 6.0 MFC project that implements the Visual
Leak Detector. It might be a good idea to read the instructions for using
Visual Leak Detector (see below) before you try to build the project,
just to be sure you've got the Debug Help library installed and ready
for use with Visual C++. 

After the project is built, running the program will pop up a simple
dialog box. If you leave the "Leak Some Memory" box checked, the program
will leak a small amount of memory as it exits. If you run the program
under the Visual C++ debugger, you should see a memory leak report in the
debug output window after the program exits. The report will include
detailed stack traces of the calls that allocated the leaked memory blocks.

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

  - Provides a complete stack trace for each leaked block, including source
    file and line number information when available.
  - Provides complete data dumps (in hex and ASCII) of leaked blocks.
  - Customizable level of detail in the memory leak report via preprocessor
    macro definitions.

Okay, but how is Visual Leak Detector better than the dozens of other after-
market leak detectors out there?

  - Visual Leak Detector has an elegant interface: it doesn't require you to
    modify ANY of the source files you are debugging. You don't need to add
    any #includes, #defines, global variables, or function calls.
  - In addition to providing stack traces with source files, line numbers
    and function names, Visual Leak Detector also provides data dumps.
  - It works with both C and C++ programs (compatible with both malloc/free
    and new/delete).
  - It is well documented, so it is easy to customize it to suit your needs.

How to Use Visual Leak Detector:
--------------------------------
Implementing Visual Leak Detector in your project is very easy. Provided
that you've completed the prerequisites, all you need to do is compile this
file and link it with your executable. There's no need to #include any
headers in your source files, and no need to add any new global variables or
function calls. Just add this file to your project.

So what are the prerequesites?
  1) You need a recent version of the Debug Help library (and header).
     Though the Debug Help library can be obtained in standalone form, the
     easiest way to get it, if you don't already have it, is to install the
     Platform SDK. Be sure to add the include and lib directories of the
     Platform SDK to the include and library directory search path in Visual
     C++ (Tools->Options->Directories in VC++ 6.0). This version of Visual
     Leak Detector is known to be compatible with the Platform SDK for
     Windows XP SP2 (and/or the Debug Help library version 6.3).

  2) Be sure you're not using precompiled headers when compiling this file.
     This is important especially if you are using Visual Leak Detector with
     an MFC application. MFC applications use a precompiled header by
     default for all source files. You can continue to use the precompiled
     header for other source files in your project, but not this one. To
     turn off precompiled headers in VC++ 6.0, go to Project->Settings, then
     select the C/C++ Tab. Select precompiled headers from the "Category"
     list. Then select vld.cpp in the tree on the left and select the
     "Not using precompiled headers" option.
     
When you link this file with your debug executable, anytime you run your
program under the debugger, a memory leak report will be printed out in the
debug output window. If there are memory leaks, you can double click on
source file/line numbers in the call stack to jump to that file and line in
the editor. Obviously, all the extra overhead incurred during memory
allocation will slow down your debug executable and add additional memory
usage. However, when you build a release executable, Visual Leak Detector is
not compiled into your program, so it will not add any overhead whatsoever.

There are a few OPTIONAL preprocessor macros that you can define to contol
the level of detail provided in memory leak reports.

  - VLD_MAX_TRACE_FRAMES: By default, Visual Leak Detector will trace back
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

  - VLD_MAX_DATA_DUMP: Define this macro to an integer value to limit the
    amount of data displayed in memory block data dumps. When this number of
    bytes of data has been dumped, the dump will stop. This can be useful if
    any of the leaked blocks are very large and the debug output window
    becomes too cluttered. You can define this macro to 0 (zero) if you want
    to suppress the data dumps altogether.

  - VLD_SHOW_USELESS_FRAMES: By default, only "useful" frames are printed in
    the call stacks. Frames that are internal to the heap or Visual Leak
    Detector are not shown. Define this to force all frames of the call
    stacks to be printed. This macro might be useful if you need to debug
    Visual Leak Detector itself or if you want to customize it.

Frequently Asked Questions:
---------------------------

Q: Is Visual Leak Detector compatible with non-Microsoft platforms?

     No. It is designed specifically for use with Visual C++, and it depends on
     heap debugging functions found only in Microsoft's C runtime library. It's
     called "Visual" Leak Detector for a reason.

Q: I'm getting a compile error that says "unexpected end of file while
   looking for precomipled header directive".

     This is a common error when compiling Visual Leak Detector in an MFC
     application. MFC projects use precompiled headers by default. You need to
     turn off precompiled headers for vld.cpp. See the instructions for using
     Visual Memory Leak Detector for more info. It would be possible for me to
     make Visual Leak Detector compatible with precompiled headers, but it would
     complicate the way projects interface with Visual Leak Detector. I think
     the convenience of having Visual Leak Detector neatly packaged in a single
     source file outweighs the negligible benefits of building it with
     precompiled headers.

Q: I'm getting a linker error. For example: cannot open file dbghelp.lib
    -- or --
   I'm getting a compile error. For example: cannot open include file
   dbghelp.h

     You need to be sure that the Debug Help library is installed. Install
     the Platform SDK. If you've already got the platform SDK installed,
     make sure that the lib and include directories from the Platform SDK
     are in Visual C++'s library and include search paths respectively.
     This allows the linker to find dbghelp.lib, and allows the compiler to
     find dbghelp.h. If you've done all this, but are getting other strange
     compile or link errors, then you might need to update to a newer
     version of the Platform SDK, and also make sure that the Platform SDK
     include and lib directories are before Visual C++'s default directories
     in the search path.

License:
--------

Visual Leak Detector is distributed under the terms of the GNU Lesser Public
License. See the COPYING.txt file for details.

Contacting the author:
----------------------

Please forward any bug reports, questions, comments or suggestions to
me at dmoulding@gmail.com.

Copyright (c) 2005 Dan Moulding