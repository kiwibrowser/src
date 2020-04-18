# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.68)
AC_INIT([Liblouis], [2.6.0], [liblouis-liblouisxml@freelists.org], [liblouis], [http://www.liblouis.org])
AC_CONFIG_SRCDIR([liblouis/lou_backTranslateString.c])
AM_CONFIG_HEADER(liblouis/config.h)

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# gnulib
gl_EARLY
gl_INIT

# make sure we have a decent malloc and realloc
gl_FUNC_MALLOC_GNU
if test $REPLACE_MALLOC = 1; then
AC_LIBOBJ([malloc])
fi
gl_MODULE_INDICATOR([malloc-gnu])

gl_FUNC_REALLOC_GNU
if test $REPLACE_REALLOC = 1; then
AC_LIBOBJ([realloc])
fi
gl_MODULE_INDICATOR([realloc-gnu])

# Checks for more programs.
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stddef.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([memset])

# This is for stuff that absolutely must end up in pyconfig.h.
# Please use pyport.h instead, if possible.
AH_TOP([
#ifndef LOUISCFG_H
#define LOUISCFG_H
])
AH_BOTTOM([
#endif /*LOUISCFG_H*/
])

# increment if the interface has additions, changes, removals.
LIBLOUIS_CURRENT=7

# increment any time the source changes; set to
#  0 if you increment CURRENT
LIBLOUIS_REVISION=0

# increment if any interfaces have been added; set to 0
#  if any interfaces have been changed or removed. removal has
#  precedence over adding, so set to 0 if both happened.
LIBLOUIS_AGE=5

AC_SUBST(LIBLOUIS_CURRENT)
AC_SUBST(LIBLOUIS_REVISION)
AC_SUBST(LIBLOUIS_AGE)

AC_ISC_POSIX
AC_HEADER_STDC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

# GNU help2man creates man pages from --help output; in many cases,
# this is sufficient, and obviates the need to maintain man pages
# separately. However, some developers do not have it so we do not
# make its use mandatory.
if test "x$cross_compiling" = xyes; then
  AC_MSG_WARN([cannot generate manual pages while cross compiling])
else
  AC_CHECK_PROG([HELP2MAN], [help2man], [help2man])
fi
AM_CONDITIONAL([HAVE_HELP2MAN], [test x$HELP2MAN = xhelp2man])

# Check if we have Python installed
AM_PATH_PYTHON([2.6],, [:])
AM_CONDITIONAL([HAVE_PYTHON], [test "$PYTHON" != :])

AC_PATH_PROG(PKG_CONFIG, pkg-config)

AC_ARG_ENABLE(ucs4,
              AC_HELP_STRING(--enable-ucs4, Enable 4 byte-wide characters),
              [],
              [enable_ucs4=no])

AC_MSG_CHECKING([whether 4 byte-wide characters should be supported])
AC_MSG_RESULT($enable_ucs4)

case "$enable_ucs4" in
yes) WIDECHAR_TYPE='unsigned int';;
*) WIDECHAR_TYPE='unsigned short int';;
esac
AC_SUBST(WIDECHAR_TYPE)
AM_CONDITIONAL([HAVE_UCS4], [test x$enable_ucs4 = xyes])

case $host in
  *mingw* | *cygwin*)
    CFLAGS="$CFLAGS -Wl,--add-stdcall-alias"
  ;;
esac

AC_CONFIG_FILES([
	Makefile
        doc/Makefile
        man/Makefile
        liblouis/Makefile
        liblouis/liblouis.h
	windows/Makefile
	windows/include/Makefile
        tables/Makefile
	liblouis.pc
        tests/Makefile
        tests/resolve_table.h
        tests/tables/Makefile
        tests/tables/moreTables/Makefile
        tests/tables/resolve_table/Makefile
        tests/tables/resolve_table/dir_1/Makefile
        tests/tables/resolve_table/dir_1/dir_1.1/Makefile
        tests/tables/resolve_table/dir_2/Makefile
        tests/doctests/Makefile
        tests/harness/Makefile
        tests/harnessSources/Makefile
	python/Makefile
	python/louis/Makefile
        tools/Makefile
	gnulib/Makefile
	m4/Makefile])
AC_OUTPUT

