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


Documentation:
--------------

See the README.html file for the complete Visual Leak Detector documentation.
