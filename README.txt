Welcome to Yasm-NextGen!

This project is a translation of the Yasm mainline C code into ANSI/ISO C++.
It's currently somewhat of an experimental project, so a number of features
present in the mainline version of yasm are not yet implemented.  Also, it's
likely that the code may change drastically with little notice.

Major things that work:
  * GNU AS syntax (more complete than mainline)
  * NASM basic syntax (no preprocessor)
  * Most object files (ELF, Win32/64)
  * DWARF2 debug info

Major things that don't work / don't exist:
  * NASM preprocessor
  * Mach-O object files
  * List file output

Some interesting things being worked on:
  * "ygas" frontend - dropin replacement for GNU AS
  * "ygas" branch - configure based, stripped down distribution for the above
  * "ygas-c" branch - backtranslation to C of the above for improved
    portability
  * yobjdump - dropin replacement for GNU objdump

For more details on what's on our TODO list, see TODO.txt.

Prerequisites (what you need to build from source):

  * A standards-conforming C++ compiler (e.g. GCC 3.4+)
  * CMake 2.4+; 2.8+ recommended (http://www.cmake.org/)
  * Python (http://www.python.org/)

If you want to build unit tests, you will also need:
  * GoogleMock (http://code.google.com/p/googlemock/)
  * GoogleTest (http://code.google.com/p/googletest/)

If your C++ installation does not have TR1 functional headers, you will also
need Boost 1.33.1+ (http://www.boost.org/).  The configuration process will
autodetect this and report an error if it requires Boost and you do not have
it.

Also recommended (for development):

  * STLFilt (http://www.bdsoft.com/tools/stlfilt.html)

STLFilt makes C++ compiler error messages more readable.


Building Yasm-NextGen On UNIX
=============================

The standard build setup places the object directory separately from the
source.  This makes it easier to clean up any temporary build files as well
as keeps the source directory clean, making revision control systems less
noisy.  The standard place to put this for Yasm is "objdir".

To prepare your working copy for building, run:
  % mkdir objdir
  % cd objdir
  % cmake ..

Alternatively, you can run "ccmake .." which will display a text-mode GUI
that shows some of the configurable options.  Options may also be specified
on the cmake command line; e.g.:
  % cmake -DCMAKE_CXX_COMPILER=`which gfilt` ..
(this sets the C++ compiler to gfilt, the stlfilt frontend)

If you don't want to build unit tests, add -DBUILD_TESTS=NO to the cmake line.
This also removes the GoogleTest and GoogleMock build dependencies.
  % cmake -DBUILD_TESTS=NO ..

Then to build:
  % make

Test:
  % make test

etc.


Building Yasm-NextGen On Windows
================================

Create an object directory for the build.  Use cmake's gui to select the
source and object directories.  CMake may need some help in finding
prerequisites on Windows; it will tell you if it can't find a tool it needs
and will not produce build files until all prerequisites are met.
