Visual Leak Detector (Version 1.0)

  Enhanced Memory Leak Detection for Visual C++


About Visual Leak Detector
--------------------------

Visual C++ provides some built-in memory leak detection features, but their
capabilities are minimal at best. This memory leak detector was created as a
free alternative to the built-in memory leak detection provided with Visual C++.
Here is a short list of of the most valuable capabilities that Visual Leak
Detector provides, all of which are lacking in the built-in detector:

  * Provides a complete stack trace for each leaked block, including source
    file and line number information when available.

  * Provides complete data dumps (in hex and ASCII) of leaked blocks.

  * Customizable level of detail in the memory leak report.

Other aftermarket leak detectors for Visual C++ are already available. But most
of the really popular ones, like Purify and BoundsChecker, are very expensive.
A few free alternatives exist, but they're often too intrusive, restrictive,
or unreliable. Here's some key advantages that Visual Leak Detector has over
many other free alternatives:

  * Visual Leak Detector is cleanly packaged as an easy-to-use library. You
    don't need to compile its source code to use it. And you only need to make
    minor additions to your own source code to integrate it with your program.

  * In addition to providing stack traces with source files, line numbers,
    and function names, Visual Leak Detector also provides data dumps.

  * It works with both C++ and C programs (compatible with both new/delete and
    malloc/free).

  * The full source code to the library is included and it is well documented,
    so it is easy to customize it to suit your needs.


How to Use Visual Leak Detector
-------------------------------

This section briefly describes the basics of using Visual Leak Detector (VLD).
For information on advanced usage, such as using VLD to debug DLLs, see the
"Advanced Topics" section in this document.

To use VLD with your project, follow these simple steps:

  1) Copy the VLD library (*.lib) files to your Visual C++ installation's "lib"
     subdirectory.

  2) Copy the VLD header files (vld.h and vldapi.h) to your Visual C++
     installation's "include" subdirectory.

  3) In the source file containing your program's main entry point, include the
     vld.h header file. It's best, but not absolutely required, to include this
     header before any other header files, except for stdafx.h. If the source
     file includes stdafx.h, then vld.h should be included after it.

  4) If you are running Windows 2000 or earlier, then you will need to copy
     dbghelp.dll to the directory where the executable being debugged resides.

  5) Build the debug version of your project.

VLD will detect memory leaks in your program whenever you run the debug version
under the Visual C++ debugger. A report of all the memory leaks detected will be
displayed in the debugger's output window when your program exits. Double-
clicking on a source file's line number in the memory leak report will take you
to that file and line in the editor window, allowing easy navigation of the code
path leading up to the allocation that resulted in a memory leak.

  NOTE: When you build release versions of your program, VLD will not be linked
        into the executable. So it is safe to leave the "#include <vld.h>" in
        your source files when doing release builds. Doing so will not result in
        any performance degradation or any other undesirable overhead.


Configuring Visual Leak Detector
--------------------------------

There are a few optional preprocessor macros that you can define to contol
certain aspects of VLD's operation, including the level of detail provided in
the memory leak report:

  * VLD_AGGREGATE_DUPLICATES

      Normally, VLD displays each individual leaked block in detail. Defining
      this macro will make VLD aggregate all leaks that share the same size and
      call stack under a single entry in the memory leak report. Only the first
      leaked block will be reported in detail. No other identical leaks will be
      displayed. Instead, a tally showing the total number of leaks matching
      that size and call stack will be shown. This can be useful if there are a
      small number of sources of leaks, but they are repeatedly leaking a very
      large number of memory blocks.

  * VLD_MAX_TRACE_FRAMES

      By default, VLD will trace the call stack for each allocated block as far
      back as possible. Each frame traced adds additional overhead (in both CPU
      time and memory usage) to your debug executable. If you'd like to limit
      this overhead, you can define this macro to an integer value. The stack
      trace will stop when it has traced this number of frames. The frame count
      includes the "useless" frames which, by default, are not displayed in the
      debugger's output window (see VLD_SHOW_USELESS_FRAMES below). Usually,
      there will be about five or six "useless" frames at the beginning of the
      call stack. Keep this in mind when using this macro, or you may not see
      the number of frames you expect.

  * VLD_MAX_DATA_DUMP

      Define this macro to an integer value to limit the amount of data
      displayed in memory block data dumps. When this number of bytes of data
      has been dumped, the dump will stop. This can be useful if any of the
      leaked blocks are very large and the debugger's output window becomes too
      cluttered. You can define this macro to 0 (zero) if you want to suppress
      data dumps altogether.

  * VLD_SHOW_USELESS_FRAMES

      By default, only "useful" frames are printed in the call stacks. Frames
      that are internal to the heap or VLD are not shown. Define this to force
      all frames of the call stacks to be printed. This macro is usually only
      useful for debugging VLD itself.


Controlling Visual Leak Detector at Runtime
-------------------------------------------

Using the default configuration, VLD's memory leak detection will be enabled
during the entire run of your program. In certain scenarious it may be desirable
to selectively disable memory leak detection in certain segments of your code.
VLD provides simple APIs for controlling the state of memory leak detection at
runtime. To access these APIs, include the vldapi.h header file. These APIs are
described here in detail:

  * VLDDisable

      This function disables memory leak detection. After calling this function,
      memory leak detection will remain disabled until it is explicitly
      re-enabled via a call to VLDEnable.

      void VLDDisable (void);

      Parameters:

      This function accepts no parameters.

      Return Value:

      None (this function always succeeds).

      Remarks:

      This function controls memory leak detection on a per-thread basis. In
      other words, calling this function disables memory leak detection for only
      the thread that called the function. Memory leak detection will remain
      enabled for any other threads in the same process. This insulates the
      programmer from having to synchronize multiple threads that disable and
      enable memory leak detection. However, note also that this means that in
      order to disable memory leak detection process-wide, this function must be
      called from every thread in the process.


  * VLDEnable()

      This function enables memory leak detection if it was previously disabled.
      After calling this function, memory leak detection will remain enabled
      unless it is explicitly disabled again via a call to VLDDisable().

      void VLDEnable (void);

      Parameters:

      This function accepts no paramters.

      Return Value:

      None (this function always succeeds).

      Remarks:

      This function controls memory leak detection on a per-thread basis. See
      the remarks for VLDDisable regarding multithreading and memory leak
      detection for details. Those same concepts also apply to this function.


Caveats
-------

In order to be successful at detecting leaks, VLD's code must run before any of
the code being debugged. Most often this will happen without a hitch. However,
there is one rare instances where this might not happen: if any global objects
in the program have been placed in the "compiler" initialization area. However,
user global objects are never placed in this area by default. They must be
manually placed there by using the "#pragma init_seg(compiler)" directive. As
long as you are not using this directive then VLD will not have any problem
running before your code. If you are using it, then you should take whatever
measures are necessary to ensure that objects from the VLD library are
constructed before your first global object is constructed. If this situation
applies to you, but you are not concerned about the possibility of memory leaks
in your global objects' constructors, then it will not be a problem if your
global objects are consructed before VLD's objects.


Building Visual Leak Detector from Source
-----------------------------------------

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
     * Windows Server 2003 users also should already have a new enough version
       of dbghelp.dll in WINDOWS\system32. So, if you're running Windows Server
       2003, you don't need to do anything with the dbghelp.dll included with
       VLD.

  2) VLD also depends on the Debug Help Library header file (dbghelp.h). This
     header file won't exist unless you have a recent Platform SDK installed. I
     recommend installing the Windows XP SP2 Platform SDK. This SDK is
     compatible with Windows XP, Windows 2000, and Windows Server 2003. If
     you're debugging an application for Windows Server 2003, the Windows Server
     2003 Platform SDK will probably work as well, but I haven't tried it myself
     so I can't guarantee it will work. Both of these SDKs can be downloaded
     from Platform SDK Update.

  3) Once you have the Platform SDK installed, you'll need to make Visual C++
     aware of where it can find the Debug Help Library header file. To do this,
     add the "include" subdirectory from the Platform SDK installation directory
     to the include search path in Visual C++:
     * Visual C++ 7: Go to Project Properties -> C/C++ -> General -> Additional
       Include Directories and add the "include" subdirectory from the Platform
       SDK. Make sure it's at the top of the list.
     * Visual C++ 6: Go to Tools -> Options -> Directories. Select "Include
       files" from the "Show Directories For" drop-down menu. Add the "include"
       subdirectory from the Platform SDK. Make sure it's at the top of the
       list.

  4) VLD also depends on two other header files (dbgint.h and mtdll.h) that will
     only be installed if you elected to install the CRT source files when you
     installed Visual C++. If you didn't install the CRT sources, you'll need to
     re-run the Visual C++ installer and install them. If you are not sure
     whether you installed the CRT sources when you installed Visual C++, check
     to see if dbgint.h and mtdll.h exist in the CRT\src subdirectory of your
     Visual C++ installation directory. If that file is missing, or you don't
     have a CRT\src directory, then chances are you need to re-install Visual
     C++ with the CRT sources selected.

  5) Make sure that your Visual C++ installation's CRT\src subdirectory is
     in the include search path. Refer to step 3 for instructions on how to
     add directories to the include search path. The CRT\src subdirectory should
     go after the default include directory. To summarize, your include search
     path should look like this:

       C:\Program Files\Microsoft Platform SDK for Windows XP SP2\Include
       C:\Program Files\Microsoft Visual Studio\VCx\Include
       C:\Program Files\Microsoft Visual Studio\CRT\src

     In the above example, "VCx" would be "VC7" for Visual Studio .NET or "VC98"
     for Visual Studio 6.0. Also, the name of your Platform SDK directory might
     be different from the example depending on which version of the Platform
     SDK you have installed.

Once you've completed all of the above steps, your build environment should be
ready. To build VLD, just open the vld.dsp project and do a batch build to
build all six of the configurations:

  * The three debug configurations are for building versions of the library that
    have debugging information so that VLD itself can be conveniently debugged.

  * The three release configurations build the library for use in debugging
    other programs.

  * There are three configurations each: one for each method of linking with the
    C runtime library (single-threaded, multithreaded, and DLL). When linking
    the VLD library against a program, the vld.h header file detects how the
    program is linking to the C runtime library and selects the appropriate VLD
    library from the three possible choices.

The "release" builds of the VLD libraries are not like typical release builds.
Despite the "release" name, they are actually meant to be linked only to debug
builds of your own programs. When doing release builds of your programs, VLD
will not be linked to them at all. In the context of VLD, "release" simply
means the versions that are optimized and have the symbols stripped from them
(to make the libraries smaller). They are the versions of the libraries that
are included in the release of VLD itself (hence the "release" name). So when
you are building the release libraries, you're really building the same
libraries that are included in the main VLD distribution. The "debug" builds of
VLD are strictly for debugging VLD itself (e.g. if you want to modify it or if
you need to fix a bug in it).

Using VLD on x64-based Windows
------------------------------

As of version 0.9i, VLD supports x64-based 64-bit Windows. However, the binaries
contained in the distributed versions of VLD are 32-bit only. To take advantage
of the 64-bit support, you'll need to build 64-bit versions of the libraries
from source. To build the 64-bit versions, follow the instructions for building
VLD from source. So long as they are built using an x64-compatible compiler in
64-bit mode, the resulting libraries will be 64-bit libraries.

NOTE: I have not personally tested the 64-bit extensions so they are not
absolutely guaranteed to work out-of-the-box. There may be a few lingering
64-bit compiler errors that still need to be worked out. If you need 64-bit
support and run into problems trying to build the source in 64-bit mode, please
let me know (my email address is listed at the end of this file). I'll be glad
to assist in getting the 64-bit code working properly.


Frequently Asked Questions
--------------------------

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

Q: When building VLD from source, I get the fatal error "C1189: #error :  ERROR:
   Use of C runtime library internal header file." in either the file stdio.h
   or in the file assert.h (or possibly in some other standard header file).

     Visual C++ is including the wrong copies of the standard headers. Be sure
     that the CRT\src subdirectory of your Visual C++ installation directory
     is listed AFTER the default include directory in Visual C++'s include
     search path.

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


License
-------

Visual Leak Detector is distributed under the terms of the GNU Lesser General
Public License. See the COPYING.txt file for details.

The Debug Help Library (dbghelp.dll) distributed with this software is not part
of Visual Leak Detector and is not covered under the terms of the GNU Lesser
General Public License. It is a separately copyrighted work of Microsoft
Corporation. Microsoft reserves all its rights to its copyright in the Debug
Help Library. Neither your use of the Visual Leak Detector software, nor your
license under the GNU Lesser General Public license grant you any rights to use
the Debug Help Library in ANY WAY (for example, redistributing it) that would
infringe upon Microsoft Corporation's copyright in the Debug Help Library.


Contacting the Author
---------------------

Please forward any bug reports, questions, comments or suggestions to me at
dmoulding@gmail.com.

Copyright (c) 2005 Dan Moulding