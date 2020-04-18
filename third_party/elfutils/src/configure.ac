dnl Process this file with autoconf to produce a configure script.
dnl Configure input file for elfutils.                     -*-autoconf-*-
dnl
dnl Copyright (C) 1996-2014 Red Hat, Inc.
dnl
dnl This file is part of elfutils.
dnl
dnl  This file is free software; you can redistribute it and/or modify
dnl  it under the terms of the GNU General Public License as published by
dnl  the Free Software Foundation; either version 3 of the License, or
dnl  (at your option) any later version.
dnl
dnl  elfutils is distributed in the hope that it will be useful, but
dnl  WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl  GNU General Public License for more details.
dnl
dnl  You should have received a copy of the GNU General Public License
dnl  along with this program.  If not, see <http://www.gnu.org/licenses/>.
AC_INIT([elfutils],[0.158],[https://bugzilla.redhat.com/],[elfutils])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_FILES([config/Makefile])

AC_COPYRIGHT([Copyright (C) 1996-2014 Red Hat, Inc.])
AC_PREREQ(2.63)			dnl Minimum Autoconf version required.

dnl We use GNU make extensions; automake 1.10 defaults to -Wportability.
AM_INIT_AUTOMAKE([gnits 1.11 -Wno-portability dist-bzip2 no-dist-gzip parallel-tests])
AM_MAINTAINER_MODE

dnl Unique ID for this build.
MODVERSION="Build on $(hostname) $(date +%FT%R:%S%z)"
AC_SUBST([MODVERSION])
AC_DEFINE_UNQUOTED(MODVERSION, "$MODVERSION")
AH_TEMPLATE([MODVERSION], [Identifier for modules in the build.])

AC_CONFIG_SRCDIR([libelf/libelf.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES([m4/Makefile])

dnl The RPM spec file.  We substitute a few values in the file.
AC_CONFIG_FILES([elfutils.spec:config/elfutils.spec.in])


AC_CANONICAL_HOST

AC_ARG_ENABLE(deterministic-archives,
[AS_HELP_STRING([--enable-deterministic-archives],
		[ar and ranlib default to -D behavior])], [
if test "${enableval}" = no; then
  default_ar_deterministic=false
else
  default_ar_deterministic=true
fi], [default_ar_deterministic=false])
AC_DEFINE_UNQUOTED(DEFAULT_AR_DETERMINISTIC, $default_ar_deterministic,
		   [Should ar and ranlib use -D behavior by default?])

AC_ARG_ENABLE([thread-safety],
AS_HELP_STRING([--enable-thread-safety],
               [enable thread safety of libraries EXPERIMENTAL]),
               use_locks=$enableval, use_locks=no)
AM_CONDITIONAL(USE_LOCKS, test "$use_locks" = yes)
AS_IF([test "$use_locks" = yes], [AC_DEFINE(USE_LOCKS)])
AS_IF([test "$use_locks" = yes],
      [AC_MSG_WARN([thread-safety is EXPERIMENTAL tests might fail.])])

AH_TEMPLATE([USE_LOCKS], [Defined if libraries should be thread-safe.])

AC_ARG_ENABLE([dwz],
AS_HELP_STRING([--enable-dwz], [enable experimental GNU ref_alt FORM, dwz multi file support in libdw]),
enable_dwz=$enableval, enable_dwz=no)
AM_CONDITIONAL(ENABLE_DWZ, test "$enable_dwz" = yes)
AS_IF([test "$enable_dwz" = yes], [AC_DEFINE(ENABLE_DWZ)])
AS_IF([test "$enable_dwz" = no -a -d /usr/lib/debug/.dwz],
      [AC_MSG_WARN([--enable-dwz disabled and /usr/lib/debug/.dwz exists, native tests might fail])])

AH_TEMPLATE([ENABLE_DWZ], [Defined if libdw should support GNU ref_alt FORM, dwz multi files.])

AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_YACC
AM_PROG_LEX
# Only available since automake 1.12
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_CHECK_TOOL([READELF], [readelf])
AC_CHECK_TOOL([NM], [nm])

AC_CACHE_CHECK([for gcc with C99 support], ac_cv_c99, [dnl
old_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -std=gnu99"
AC_COMPILE_IFELSE([AC_LANG_SOURCE([dnl
int foo (int a) { for (int i = 0; i < a; ++i) if (i % 4) break; int s = a; }])],
		  ac_cv_c99=yes, ac_cv_c99=no)
CFLAGS="$old_CFLAGS"])
AS_IF([test "x$ac_cv_c99" != xyes],
      AC_MSG_ERROR([gcc with C99 support required]))

AC_CACHE_CHECK([for __thread support], ac_cv_tls, [dnl
# Use the same flags that we use for our DSOs, so the test is representative.
# Some old compiler/linker/libc combinations fail some ways and not others.
save_CFLAGS="$CFLAGS"
save_LDFLAGS="$LDFLAGS"
CFLAGS="-fpic $CFLAGS"
LDFLAGS="-shared -Wl,-z,defs,-z,relro $LDFLAGS"
AC_LINK_IFELSE([dnl
AC_LANG_PROGRAM([[#undef __thread
static __thread int a; int foo (int b) { return a + b; }]],
		[[exit (foo (0));]])],
	       ac_cv_tls=yes, ac_cv_tls=no)
CFLAGS="$save_CFLAGS"
LDFLAGS="$save_LDFLAGS"])
AS_IF([test "x$ac_cv_tls" != xyes],
      AC_MSG_ERROR([__thread support required]))

dnl This test must come as early as possible after the compiler configuration
dnl tests, because the choice of the file model can (in principle) affect
dnl whether functions and headers are available, whether they work, etc.
AC_SYS_LARGEFILE

dnl Enable the linker to be build as a native-only linker.  By default it
dnl can handle all architectures but this comes at a cost.  A native
dnl will be slightly faster, small, and has fewer dependencies.
native_ld=no
AC_ARG_ENABLE([generic],
AS_HELP_STRING([--disable-generic], [do not build generic linker]), [dnl
if test "$enable_generic" = no; then
  case "$host_cpu" in
   i?86)
    AC_DEFINE(NATIVE_ELF, 32)
    native_ld=yes
    base_cpu=i386
    ;;
   *)
    AC_MSG_ERROR([no machine-specific linker for this configuration available])
    ;;
  esac
fi])
AH_TEMPLATE([NATIVE_ELF],
[Define to 32 or 64 if a specific implementation is wanted.])
AM_CONDITIONAL(NATIVE_LD, test "$native_ld" = yes)
dnl The automake generated Makefile cannot deal with macros in the name
dnl of files if at any time there is no such file, even if the filename
dnl would not be used.
AS_IF([test -z "$base_cpu"], [base_cpu=none])
AC_SUBST(base_cpu)
dnl Support to work around automake's inflexible dependency generation.
dnl See src/Makefile.am for more information.
AM_CONDITIONAL(NEVER, false)

dnl Enable debugging via mudflap.  This option will cause most libraries
dnl to be built as archives which are statically linked into the applications.
dnl All code, as far as possible, is compiled instrumented to catch all
dnl the bugs valgrind is able to catch.
use_mudflap=no
AC_ARG_ENABLE([mudflap],
AS_HELP_STRING([--enable-mudflap],
[build binaries with mudflap instrumentation]), [dnl
if test "x$enable_mudflap" = xyes; then
  # Check whether the compiler support -fmudflap.
  old_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -fmudflap"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],[use_mudflap=yes],[use_mudflap=fail])
  CFLAGS="$old_CFLAGS"
fi])
if test "$use_mudflap" = fail; then
  AC_MSG_FAILURE([--enable-mudflap requires a compiler which understands this option])
fi
AM_CONDITIONAL(MUDFLAP, test "$use_mudflap" = yes)

dnl enable debugging of branch prediction.
AC_ARG_ENABLE([debugpred],
AS_HELP_STRING([--enable-debugpred],[build binaries with support to debug branch prediction]),
[use_debugpred=$enableval], [use_debugpred=no])
case $use_debugpred in
 yes) use_debugpred=1 ;;
 *)   use_debugpred=0 ;;
esac
AC_SUBST([DEBUGPRED], $use_debugpred)

dnl Enable gprof suport.
AC_ARG_ENABLE([gprof],
AS_HELP_STRING([--enable-gprof],[build binaries with gprof support]), [use_gprof=$enableval], [use_gprof=no])
if test "$use_gprof" = yes; then
  CFLAGS="$CFLAGS -pg"
  LDFLAGS="$LDFLAGS -pg"
fi
AM_CONDITIONAL(GPROF, test "$use_gprof" = yes)

# Enable gcov suport.
AC_ARG_ENABLE([gcov],
AS_HELP_STRING([--enable-gcov],[build binaries with gcov support]), [use_gcov=$enableval], [use_gcov=no])
if test "$use_gcov" = yes; then
  CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
  LDFLAGS="$LDFLAGS -fprofile-arcs"
fi
AM_CONDITIONAL(GCOV, test "$use_gcov" = yes)

AC_ARG_ENABLE([valgrind],
AS_HELP_STRING([--enable-valgrind],[run all tests under valgrind]),
[use_valgrind=$enableval], [use_valgrind=no])
if test "$use_valgrind" = yes; then
  AC_CHECK_PROG(HAVE_VALGRIND, valgrind, yes, no)
  if test "$HAVE_VALGRIND" = "no"; then
    AC_MSG_ERROR([valgrind not found])
  fi
fi
AM_CONDITIONAL(USE_VALGRIND, test "$use_valgrind" = yes)

AM_CONDITIONAL(BUILD_STATIC, [dnl
test "$use_mudflap" = yes -o "$use_gprof" = yes -o "$use_gcov" = yes])

AC_ARG_ENABLE([tests-rpath],
AS_HELP_STRING([--enable-tests-rpath],[build $ORIGIN-using rpath into tests]),
	       [tests_use_rpath=$enableval], [tests_use_rpath=no])
AM_CONDITIONAL(TESTS_RPATH, test "$tests_use_rpath" = yes)

LIBEBL_SUBDIR="$PACKAGE"
AC_ARG_ENABLE([libebl-subdir],
AS_HELP_STRING([--enable-libebl-subdir=DIR],
[install libebl_CPU modules in $(libdir)/DIR]), [dnl
LIBEBL_SUBDIR="$enable_libebl_subdir"])
AC_SUBST([LIBEBL_SUBDIR])
AC_DEFINE_UNQUOTED(LIBEBL_SUBDIR, "$LIBEBL_SUBDIR")
AH_TEMPLATE([LIBEBL_SUBDIR], [$libdir subdirectory containing libebl modules.])

dnl Test for zlib and bzlib, gives ZLIB/BZLIB .am
dnl conditional and config.h USE_ZLIB/USE_BZLIB #define.
save_LIBS="$LIBS"
LIBS=
eu_ZIPLIB(zlib,ZLIB,z,gzdirect,gzip)
eu_ZIPLIB(bzlib,BZLIB,bz2,BZ2_bzdopen,bzip2)
eu_ZIPLIB(lzma,LZMA,lzma,lzma_auto_decoder,[LZMA (xz)])
zip_LIBS="$LIBS"
LIBS="$save_LIBS"
AC_SUBST([zip_LIBS])

AC_CHECK_LIB([stdc++], [__cxa_demangle], [dnl
AC_DEFINE([USE_DEMANGLE], [1], [Defined if demangling is enabled])])
AM_CONDITIONAL(DEMANGLE, test "$ac_cv_lib_stdcpp___cxa_demangle" = yes)

dnl The directories with content.

dnl Documentation.
dnl Commented out for now.
dnl AC_CONFIG_FILES([doc/Makefile])

dnl Support library.
AC_CONFIG_FILES([lib/Makefile])

dnl ELF library.
AC_CONFIG_FILES([libelf/Makefile])

dnl Higher-level ELF support library.
AC_CONFIG_FILES([libebl/Makefile])

dnl DWARF library.
AC_CONFIG_FILES([libdw/Makefile])

dnl Higher-level DWARF support library.
AC_CONFIG_FILES([libdwfl/Makefile])

dnl CPU handling library.
AC_CONFIG_FILES([libcpu/Makefile])

dnl Assembler library.
AM_CONDITIONAL(HAVE_LIBASM, true)dnl Used in tests/Makefile.am, which see.
AC_CONFIG_FILES([libasm/Makefile])

dnl CPU-specific backend libraries.
AC_CONFIG_FILES([backends/Makefile])

dnl Tools.
AC_CONFIG_FILES([src/Makefile po/Makefile.in])

dnl Test suite.
AM_CONDITIONAL(STANDALONE, false)dnl Used in tests/Makefile.am, which see.
AC_CONFIG_FILES([tests/Makefile])

# Get the definitions necessary to create the Makefiles in the po
# subdirectories.  This is a small subset of the gettext rules.
AC_SUBST(USE_NLS, yes)
AM_PO_SUBDIRS

dnl Appended to the config.h file.
dnl We hide all kinds of configuration magic in lib/eu-config.h.
AH_BOTTOM([#include <eu-config.h>])

dnl Version compatibility header.
AC_CONFIG_FILES([version.h:config/version.h.in])
AC_SUBST([eu_version])

# 1.234<whatever> -> 1234<whatever>
case "$PACKAGE_VERSION" in
[[0-9]].*) eu_version=`echo "$PACKAGE_VERSION" | sed 's@\.@@'` ;;
*)     	   AC_MSG_ERROR([confused by version number '$PACKAGE_VERSION']) ;;
esac
case "$eu_version" in
*.*)
  # 1234.567 -> "1234", "567"
  eu_extra_version="${eu_version#*.}"
  eu_version="${eu_version%%.*}"
  case "$eu_extra_version" in
  [[0-9]][[0-9]][[0-9]]) ;;
  [[0-9]][[0-9]])	eu_extra_version="${eu_extra_version}0" ;;
  [[0-9]])	   	eu_extra_version="${eu_extra_version}00" ;;
  *) AC_MSG_ERROR([confused by version number '$PACKAGE_VERSION']) ;;
  esac
  ;;
*)
  eu_extra_version=000
  ;;
esac

case "$eu_version" in
      0[[0-9]][[0-9]][[0-9]]) eu_version="${eu_version#0}$eu_extra_version" ;;
[[0-9]][[0-9]][[0-9]][[0-9]]) eu_version="${eu_version}$eu_extra_version" ;;
[[0-9]][[0-9]][[0-9]])	      eu_version="${eu_version}0$eu_extra_version" ;;
[[0-9]][[0-9]])	  	      eu_version="${eu_version}00$eu_extra_version";;
*) AC_MSG_ERROR([confused by version number '$PACKAGE_VERSION']) ;;
esac

# Round up to the next release API (x.y) version.
eu_version=$(( (eu_version + 999) / 1000 ))

AC_CHECK_SIZEOF(long)

# On a 64-bit host where can can use $CC -m32, we'll run two sets of tests.
# Likewise in a 32-bit build on a host where $CC -m64 works.
utrace_BIARCH
# `$utrace_biarch' will be `-m64' even on an uniarch i386 machine.
CC_BIARCH="$CC $utrace_biarch"
AC_SUBST([CC_BIARCH])

AC_OUTPUT
