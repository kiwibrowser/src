AC_INIT([OTS],
        [6.1.1],
        [https://github.com/khaledhosny/ots/issues],
        [ots],
        [https://github.com/khaledhosny/ots])

AC_CANONICAL_SYSTEM
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign subdir-objects])
AM_SILENT_RULES([yes])

AC_CYGWIN
AC_EXEEXT
AC_OBJEXT
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB
PKG_PROG_PKG_CONFIG([0.20])

AX_CXX_COMPILE_STDCXX([11], [noext], [mandatory])

if test "$GCC" = yes; then
	AX_CHECK_COMPILE_FLAG([-fsanitize=address], [ASAN_CFLAGS=-fsanitize=address], :)
	AX_CHECK_COMPILE_FLAG([-fsanitize=undefined], [UBSAN_CFLAGS=-fsanitize=undefined], :)
fi
AC_SUBST(ASAN_CFLAGS)
AC_SUBST(UBSAN_CFLAGS)
AM_CONDITIONAL(HAVE_FSANITIZE, [test "x$ASAN_CFLAGS" != "x" -o "x$UBSAN_CFLAGS" != "x"])

AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug=@<:@yes/no@:>@],
	               [Enable extra debugging information for font
	                sanitization @<:@default=no@:>@]))
AS_IF([test "x$enable_debug" = "xyes"], [
	AC_DEFINE(OTS_DEBUG, 1, [Enable extra debugging information])
])

AC_ARG_ENABLE([graphite],
	AS_HELP_STRING([--enable-graphite=@<:@yes/no@:>@],
                       [Sanitize Graphite tables @<:@default=yes@:>@]))
AS_IF([test "x$enable_graphite" != "xno"], [
	AC_DEFINE(OTS_GRAPHITE, 1, [Sanitize Graphite tables])
])
AM_CONDITIONAL(OTS_GRAPHITE, [test "x$enable_graphite" != "xno"])

have_freetype=false
PKG_CHECK_MODULES(FREETYPE, [freetype2], [have_freetype=true], :)
if $have_freetype; then
	AC_DEFINE(HAVE_FREETYPE, 1, [Have FreeType 2 library])
fi
AM_CONDITIONAL(HAVE_FREETYPE, [$have_freetype])

have_pthread=false
AX_PTHREAD([have_pthread=true])
if $have_pthread; then
	GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=1"
else
	GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=0"
fi
AC_SUBST([GTEST_CPPFLAGS])

have_coretext=false
AC_CHECK_TYPE([CTFontRef], [have_coretext=true], [], [#include <ApplicationServices/ApplicationServices.h>])
if $have_coretext; then
	CORETEXT_CFLAGS=
	CORETEXT_LIBS="-framework ApplicationServices"
	AC_DEFINE(HAVE_CORETEXT, 1, [Have Core Text framework])
fi
AC_SUBST(CORETEXT_CFLAGS)
AC_SUBST(CORETEXT_LIBS)
AM_CONDITIONAL(HAVE_CORETEXT, [$have_coretext])

have_win32=false
AC_CHECK_HEADERS(windows.h, have_win32=true)
if $have_win32; then
	AC_DEFINE(HAVE_WIN32, 1, [Have Windows API])
fi
AM_CONDITIONAL(HAVE_WIN32, [$have_win32])

have_systime=false
AC_CHECK_HEADER([sys/time.h], [have_systime=true])
AM_CONDITIONAL(HAVE_SYSTIME, [$have_systime])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
