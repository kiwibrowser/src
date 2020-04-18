dnl Process this file with autoconf to produce a configure script.
dnl Configure input file for elfutils test suite.		-*-autoconf-*-
dnl
dnl Copyright (C) 2005 Red Hat, Inc.
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, version 2.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software Foundation,
dnl Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
dnl
AC_INIT([elfutils tests],[0.117],
	[http://bugzilla.redhat.com/bugzilla/],
	[elfutils-tests])

AC_COPYRIGHT([Copyright (C) 2005 Red Hat, Inc.])
AC_PREREQ(2.59)			dnl Minimum Autoconf version required.

AM_INIT_AUTOMAKE([foreign 1.7])

AC_CONFIG_SRCDIR([allfcts.c])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC

AC_CACHE_CHECK([for gcc with C99 support], ac_cv_c99, [dnl
old_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -std=gnu99"
AC_COMPILE_IFELSE([dnl
int foo (int a) { for (int i = 0; i < a; ++i) if (i % 4) break; int s = a; }],
		  ac_cv_c99=yes, ac_cv_c99=no)
CFLAGS="$old_CFLAGS"])
AS_IF([test "x$ac_cv_c99" != xyes],
      AC_MSG_ERROR([gcc with C99 support required]))

AC_CHECK_HEADERS([libelf.h elfutils/libdw.h],,
[AC_MSG_ERROR([elfutils-devel package not installed])])

AC_CHECK_LIB([asm], [asm_begin], [have_libasm=yes], [have_libasm=no])
AM_CONDITIONAL(HAVE_LIBASM, [test $have_libasm = yes])

AM_CONDITIONAL(STANDALONE, true)
AM_CONDITIONAL(BUILD_STATIC, false)
AM_CONDITIONAL(TESTS_RPATH, false)
AM_CONDITIONAL(MUDFLAP, false)
AM_CONDITIONAL(GCOV, false)

dnl Text of the config.h file.
AH_BOTTOM([#define ELFUTILS_HEADER(name) <elfutils/lib##name.h>])

AC_OUTPUT
