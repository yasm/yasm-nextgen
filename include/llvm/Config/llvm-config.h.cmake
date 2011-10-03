/*===-- llvm/config/llvm-config.h - llvm configure variable -------*- C -*-===*/
/*                                                                            */
/*                     The LLVM Compiler Infrastructure                       */
/*                                                                            */
/* This file is distributed under the University of Illinois Open Source      */
/* License. See LICENSE.TXT for details.                                      */
/*                                                                            */
/*===----------------------------------------------------------------------===*/

/* This file enumerates all of the llvm variables from configure so that
   they can be in exported headers and won't override package specific
   directives.  This is a C file so we can include it in the llvm-c headers.  */

/* To avoid multiple inclusions of these variables when we include the exported
   headers and config.h, conditionally include these.  */
/* TODO: This is a bit of a hack.  */
#ifndef CONFIG_H

/* Define if threads enabled */
#cmakedefine01 LLVM_ENABLE_THREADS

/* Has gcc/MSVC atomic intrinsics */
#cmakedefine01 LLVM_HAS_ATOMICS

/* Define if this is Unixish platform */
#cmakedefine LLVM_ON_UNIX ${LLVM_ON_UNIX}

/* Define if this is Win32ish platform */
#cmakedefine LLVM_ON_WIN32 ${LLVM_ON_WIN32}

#endif
