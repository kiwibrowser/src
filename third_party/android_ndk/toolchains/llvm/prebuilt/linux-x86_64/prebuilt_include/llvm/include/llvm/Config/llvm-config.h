/* include/llvm/Config/llvm-config.h.  Generated from llvm-config.h.in by configure.  */
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

/* Installation directory for binary executables */
#define LLVM_BINDIR "/opt/llvm-android/bin"

/* Time at which LLVM was configured */
#define LLVM_CONFIGTIME "Tue May  8 14:22:45 CST 2012"

/* Installation directory for data files */
#define LLVM_DATADIR "/opt/llvm-android/share/llvm"

/* Target triple LLVM will generate code for by default */
#if defined(__APPLE__)
#define LLVM_DEFAULT_TARGET_TRIPLE "x86_64-apple-darwin"
#else
#define LLVM_DEFAULT_TARGET_TRIPLE "x86_64-unknown-linux"
#endif

/* Installation directory for documentation */
#define LLVM_DOCSDIR "/opt/llvm-android/share/doc/llvm"

/* Define if threads enabled */
#if !defined(_WIN32) && !defined(_WIN64)
#define LLVM_ENABLE_THREADS 1
#else
#define LLVM_ENABLE_THREADS 0
#endif

/* Installation directory for config files */
#define LLVM_ETCDIR "/opt/llvm-android/etc/llvm"

#if !defined(_WIN32) && !defined(_WIN64)

/* Has gcc/MSVC atomic intrinsics */
#define LLVM_HAS_ATOMICS 1

#else

#define LLVM_HAS_ATOMICS 0

#endif /* !defined(_WIN32) && !defined(_WIN64) */

/* Installation directory for include files */
#define LLVM_INCLUDEDIR "/opt/llvm-android/include"

/* Installation directory for .info files */
#define LLVM_INFODIR "/opt/llvm-android/info"

/* Installation directory for libraries */
#define LLVM_LIBDIR "/opt/llvm-android/lib"

/* Installation directory for man pages */
#define LLVM_MANDIR "/opt/llvm-android/man"

/* Define to path to circo program if found or 'echo circo' otherwise */
/* #undef LLVM_PATH_CIRCO */

/* Define to path to dot program if found or 'echo dot' otherwise */
/* #undef LLVM_PATH_DOT */

/* Define to path to dotty program if found or 'echo dotty' otherwise */
/* #undef LLVM_PATH_DOTTY */

/* Define to path to fdp program if found or 'echo fdp' otherwise */
/* #undef LLVM_PATH_FDP */

/* Define to path to Graphviz program if found or 'echo Graphviz' otherwise */
/* #undef LLVM_PATH_GRAPHVIZ */

/* Define to path to gv program if found or 'echo gv' otherwise */
/* #undef LLVM_PATH_GV */

/* Define to path to neato program if found or 'echo neato' otherwise */
/* #undef LLVM_PATH_NEATO */

/* Define to path to twopi program if found or 'echo twopi' otherwise */
/* #undef LLVM_PATH_TWOPI */

/* Define to path to xdot.py program if found or 'echo xdot.py' otherwise */
/* #undef LLVM_PATH_XDOT_PY */

/* Installation prefix directory */
#define LLVM_PREFIX "/opt/llvm-android"

/* Major version of the LLVM API */
#define LLVM_VERSION_MAJOR 5

/* Minor version of the LLVM API */
#define LLVM_VERSION_MINOR 0

/* Patch version of the LLVM API */
#define LLVM_VERSION_PATCH 300080

/* LLVM version string */
#define LLVM_VERSION_STRING "5.0.300080"

#include "llvm/Config/llvm-platform-config.h"

#endif
