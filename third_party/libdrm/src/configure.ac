#  Copyright 2005 Adam Jackson.
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  on the rights to use, copy, modify, merge, publish, distribute, sub
#  license, and/or sell copies of the Software, and to permit persons to whom
#  the Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice (including the next
#  paragraph) shall be included in all copies or substantial portions of the
#  Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
#  ADAM JACKSON BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

AC_PREREQ([2.63])
AC_INIT([libdrm],
        [2.4.85],
        [https://bugs.freedesktop.org/enter_bug.cgi?product=DRI],
        [libdrm])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
PKG_PROG_PKG_CONFIG

# Require xorg-macros minimum of 1.12 for XORG_WITH_XSLTPROC
m4_ifndef([XORG_MACROS_VERSION],
          [m4_fatal([must install xorg-macros 1.12 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.12)
XORG_WITH_XSLTPROC
XORG_MANPAGE_SECTIONS

AM_INIT_AUTOMAKE([1.10 foreign dist-bzip2])

# Enable quiet compiles on automake 1.11.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Check for programs
AC_PROG_CC
AC_PROG_CC_C99

if test "x$ac_cv_prog_cc_c99" = xno; then
	AC_MSG_ERROR([Building libdrm requires C99 enabled compiler])
fi

AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_FUNC_ALLOCA

save_CFLAGS="$CFLAGS"
export CFLAGS="$CFLAGS -Werror"
AC_HEADER_MAJOR
CFLAGS="$save_CFLAGS"

AC_CHECK_HEADERS([sys/sysctl.h sys/select.h])

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

dnl pthread-stubs is mandatory on BSD platforms, due to the nature of the
dnl project. Even then there's a notable issue as described in the project README
case "$host_os" in
linux* | cygwin* | darwin* | solaris* | *-gnu* | gnu*)
    pthread_stubs_possible="no"
    ;;
* )
    pthread_stubs_possible="yes"
    ;;
esac

if test "x$pthread_stubs_possible" = xyes; then
    PKG_CHECK_MODULES(PTHREADSTUBS, pthread-stubs >= 0.4)
    AC_SUBST(PTHREADSTUBS_CFLAGS)
    AC_SUBST(PTHREADSTUBS_LIBS)
fi

pkgconfigdir=${libdir}/pkgconfig
AC_SUBST(pkgconfigdir)
libdrmdatadir=${datadir}/libdrm
AC_SUBST(libdrmdatadir)

AC_ARG_ENABLE([udev],
              [AS_HELP_STRING([--enable-udev],
                              [Enable support for using udev instead of mknod (default: disabled)])],
              [UDEV=$enableval], [UDEV=no])

AC_ARG_ENABLE(libkms,
	      AS_HELP_STRING([--disable-libkms],
	      [Disable KMS mm abstraction library (default: auto, enabled on supported platforms)]),
	      [LIBKMS=$enableval], [LIBKMS=auto])

AC_ARG_ENABLE(intel,
	      AS_HELP_STRING([--disable-intel],
	      [Enable support for intel's KMS API (default: auto, enabled on x86)]),
	      [INTEL=$enableval], [INTEL=auto])

AC_ARG_ENABLE(radeon,
	      AS_HELP_STRING([--disable-radeon],
	      [Enable support for radeon's KMS API (default: auto)]),
	      [RADEON=$enableval], [RADEON=auto])

AC_ARG_ENABLE(amdgpu,
	      AS_HELP_STRING([--disable-amdgpu],
	      [Enable support for amdgpu's KMS API (default: auto)]),
	      [AMDGPU=$enableval], [AMDGPU=auto])

AC_ARG_ENABLE(nouveau,
	      AS_HELP_STRING([--disable-nouveau],
	      [Enable support for nouveau's KMS API (default: auto)]),
	      [NOUVEAU=$enableval], [NOUVEAU=auto])

AC_ARG_ENABLE(vmwgfx,
	      AS_HELP_STRING([--disable-vmwgfx],
	      [Enable support for vmwgfx's KMS API (default: yes)]),
	      [VMWGFX=$enableval], [VMWGFX=yes])

AC_ARG_ENABLE(omap-experimental-api,
	      AS_HELP_STRING([--enable-omap-experimental-api],
	      [Enable support for OMAP's experimental API (default: disabled)]),
	      [OMAP=$enableval], [OMAP=no])

AC_ARG_ENABLE(exynos-experimental-api,
	      AS_HELP_STRING([--enable-exynos-experimental-api],
	      [Enable support for EXYNOS's experimental API (default: disabled)]),
	      [EXYNOS=$enableval], [EXYNOS=no])

AC_ARG_ENABLE(freedreno,
	      AS_HELP_STRING([--disable-freedreno],
	      [Enable support for freedreno's KMS API (default: auto, enabled on arm)]),
	      [FREEDRENO=$enableval], [FREEDRENO=auto])

AC_ARG_ENABLE(freedreno-kgsl,
	      AS_HELP_STRING([--enable-freedreno-kgsl],
	      [Enable support for freedreno's to use downstream android kernel API (default: disabled)]),
	      [FREEDRENO_KGSL=$enableval], [FREEDRENO_KGSL=no])

AC_ARG_ENABLE(tegra-experimental-api,
	      AS_HELP_STRING([--enable-tegra-experimental-api],
	      [Enable support for Tegra's experimental API (default: disabled)]),
	      [TEGRA=$enableval], [TEGRA=no])

AC_ARG_ENABLE(vc4,
	      AS_HELP_STRING([--disable-vc4],
	      [Enable support for vc4's API (default: auto, enabled on arm)]),
	      [VC4=$enableval], [VC4=auto])

AC_ARG_ENABLE(etnaviv-experimental-api,
	      AS_HELP_STRING([--enable-etnaviv-experimental-api],
	      [Enable support for etnaviv's experimental API (default: disabled)]),
	      [ETNAVIV=$enableval], [ETNAVIV=no])

AC_ARG_ENABLE(install-test-programs,
		  AS_HELP_STRING([--enable-install-test-programs],
		  [Install test programs (default: no)]),
		  [INSTALL_TESTS=$enableval], [INSTALL_TESTS=no])

dnl ===========================================================================
dnl check compiler flags
AC_DEFUN([LIBDRM_CC_TRY_FLAG], [
  AC_MSG_CHECKING([whether $CC supports $1])

  libdrm_save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $1"

  AC_COMPILE_IFELSE([AC_LANG_SOURCE([ ])], [libdrm_cc_flag=yes], [libdrm_cc_flag=no])
  CFLAGS="$libdrm_save_CFLAGS"

  if test "x$libdrm_cc_flag" = "xyes"; then
    ifelse([$2], , :, [$2])
  else
    ifelse([$3], , :, [$3])
  fi
  AC_MSG_RESULT([$libdrm_cc_flag])
])

dnl We use clock_gettime to check for timeouts in drmWaitVBlank

AC_CHECK_FUNCS([clock_gettime], [CLOCK_LIB=],
               [AC_CHECK_LIB([rt], [clock_gettime], [CLOCK_LIB=-lrt],
                             [AC_MSG_ERROR([Couldn't find clock_gettime])])])
AC_SUBST([CLOCK_LIB])

AC_CHECK_FUNCS([open_memstream], [HAVE_OPEN_MEMSTREAM=yes])

dnl Use lots of warning flags with with gcc and compatible compilers

dnl Note: if you change the following variable, the cache is automatically
dnl skipped and all flags rechecked.  So there's no need to do anything
dnl else.  If for any reason you need to force a recheck, just change
dnl MAYBE_WARN in an ignorable way (like adding whitespace)

MAYBE_WARN="-Wall -Wextra \
-Wsign-compare -Werror-implicit-function-declaration \
-Wpointer-arith -Wwrite-strings -Wstrict-prototypes \
-Wmissing-prototypes -Wmissing-declarations -Wnested-externs \
-Wpacked -Wswitch-enum -Wmissing-format-attribute \
-Wstrict-aliasing=2 -Winit-self \
-Wdeclaration-after-statement -Wold-style-definition \
-Wno-unused-parameter \
-Wno-attributes -Wno-long-long -Winline -Wshadow \
-Wno-missing-field-initializers"

# invalidate cached value if MAYBE_WARN has changed
if test "x$libdrm_cv_warn_maybe" != "x$MAYBE_WARN"; then
	unset libdrm_cv_warn_cflags
fi
AC_CACHE_CHECK([for supported warning flags], libdrm_cv_warn_cflags, [
	echo
	WARN_CFLAGS=""

	# Some warning options are not supported by all versions of
	# gcc, so test all desired options against the current
	# compiler.
	#
	# Note that there are some order dependencies
	# here. Specifically, an option that disables a warning will
	# have no net effect if a later option then enables that
	# warnings, (perhaps implicitly). So we put some grouped
	# options (-Wall and -Wextra) up front and the -Wno options
	# last.

	for W in $MAYBE_WARN; do
		LIBDRM_CC_TRY_FLAG([$W], [WARN_CFLAGS="$WARN_CFLAGS $W"])
	done

	libdrm_cv_warn_cflags=$WARN_CFLAGS
	libdrm_cv_warn_maybe=$MAYBE_WARN

	AC_MSG_CHECKING([which warning flags were supported])])
WARN_CFLAGS="$libdrm_cv_warn_cflags"

# Check for atomic intrinsics
AC_CACHE_CHECK([for native atomic primitives], drm_cv_atomic_primitives, [
	drm_cv_atomic_primitives="none"

	AC_LINK_IFELSE([AC_LANG_PROGRAM([[
	int atomic_add(int *i) { return __sync_add_and_fetch (i, 1); }
	int atomic_cmpxchg(int *i, int j, int k) { return __sync_val_compare_and_swap (i, j, k); }
					  ]],[[]])], [drm_cv_atomic_primitives="Intel"],[])

	if test "x$drm_cv_atomic_primitives" = "xnone"; then
		AC_CHECK_HEADER([atomic_ops.h], drm_cv_atomic_primitives="libatomic-ops")
	fi

	# atomic functions defined in <atomic.h> & libc on Solaris
	if test "x$drm_cv_atomic_primitives" = "xnone"; then
		AC_CHECK_FUNC([atomic_cas_uint], drm_cv_atomic_primitives="Solaris")
	fi
])

if test "x$drm_cv_atomic_primitives" = xIntel; then
	AC_DEFINE(HAVE_LIBDRM_ATOMIC_PRIMITIVES, 1,
		[Enable if your compiler supports the Intel __sync_* atomic primitives])
fi
if test "x$drm_cv_atomic_primitives" = "xlibatomic-ops"; then
	AC_DEFINE(HAVE_LIB_ATOMIC_OPS, 1, [Enable if you have libatomic-ops-dev installed])
fi

dnl Print out the approapriate message considering the value set be the
dnl respective in $1.
dnl $1 - value to be evaluated. Eg. $INTEL, $NOUVEAU, ...
dnl $2 - libdrm shortname. Eg. intel, freedreno, ...
dnl $3 - GPU name/brand. Eg. Intel, NVIDIA Tegra, ...
dnl $4 - Configure switch. Eg. intel, omap-experimental-api, ...
AC_DEFUN([LIBDRM_ATOMICS_NOT_FOUND_MSG], [
	case "x$1" in
		xyes)	AC_MSG_ERROR([libdrm_$2 depends upon atomic operations, which were not found for your compiler/cpu. Try compiling with -march=native, or install the libatomics-op-dev package, or, failing both of those, disable support for $3 GPUs by passing --disable-$4 to ./configure]) ;;
		xauto)	AC_MSG_WARN([Disabling $2. It depends on atomic operations, which were not found for your compiler/cpu. Try compiling with -march=native, or install the libatomics-op-dev package.]) ;;
		*)	;;
	esac
])

if test "x$drm_cv_atomic_primitives" = "xnone"; then
	LIBDRM_ATOMICS_NOT_FOUND_MSG($INTEL, intel, Intel, intel)
	INTEL=no

	LIBDRM_ATOMICS_NOT_FOUND_MSG($RADEON, radeon, Radeon, radeon)
	RADEON=no

	LIBDRM_ATOMICS_NOT_FOUND_MSG($AMDGPU, amdgpu, AMD, amdgpu)
	AMDGPU=no

	LIBDRM_ATOMICS_NOT_FOUND_MSG($NOUVEAU, nouveau, NVIDIA, nouveau)
	NOUVEAU=no

	LIBDRM_ATOMICS_NOT_FOUND_MSG($OMAP, omap, OMAP, omap-experimental-api)
	OMAP=no

	LIBDRM_ATOMICS_NOT_FOUND_MSG($FREEDRENO, freedreno, Qualcomm Adreno, freedreno)
	FREEDRENO=no

	LIBDRM_ATOMICS_NOT_FOUND_MSG($TEGRA, tegra, NVIDIA Tegra, tegra-experimental-api)
	TEGRA=no

	LIBDRM_ATOMICS_NOT_FOUND_MSG($ETNAVIV, etnaviv, Vivante, etnaviv-experimental-api)
	ETNAVIV=no
else
	if test "x$INTEL" = xauto; then
		case $host_cpu in
			i?86|x86_64)	INTEL=yes ;;
			*)		INTEL=no ;;
		esac
	fi
	if test "x$RADEON" = xauto; then
		RADEON=yes
	fi
	if test "x$AMDGPU" = xauto; then
		AMDGPU=yes
	fi
	if test "x$NOUVEAU" = xauto; then
		NOUVEAU=yes
	fi
	if test "x$FREEDRENO" = xauto; then
		case $host_cpu in
			arm*|aarch64)	FREEDRENO=yes ;;
			*)		FREEDRENO=no ;;
		esac
	fi
	if test "x$VC4" = xauto; then
		case $host_cpu in
			arm*|aarch64)	VC4=yes ;;
			*)		VC4=no ;;
		esac
	fi
fi

if test "x$INTEL" != "xno"; then
	PKG_CHECK_MODULES(PCIACCESS, [pciaccess >= 0.10])
fi
AC_SUBST(PCIACCESS_CFLAGS)
AC_SUBST(PCIACCESS_LIBS)

if test "x$UDEV" = xyes; then
	AC_DEFINE(UDEV, 1, [Have UDEV support])
fi

AC_CANONICAL_HOST
if test "x$LIBKMS" = xauto ; then
	case $host_os in
		linux*)		LIBKMS="yes" ;;
		freebsd* | kfreebsd*-gnu)
				LIBKMS="yes" ;;
		dragonfly*)	LIBKMS="yes" ;;
		*)		LIBKMS="no" ;;
	esac
fi

AM_CONDITIONAL(HAVE_LIBKMS, [test "x$LIBKMS" = xyes])

AM_CONDITIONAL(HAVE_INTEL, [test "x$INTEL" = xyes])
if test "x$INTEL" = xyes; then
	AC_DEFINE(HAVE_INTEL, 1, [Have intel support])
fi

AM_CONDITIONAL(HAVE_VMWGFX, [test "x$VMWGFX" = xyes])
if test "x$VMWGFX" = xyes; then
	AC_DEFINE(HAVE_VMWGFX, 1, [Have vmwgfx kernel headers])
fi

AM_CONDITIONAL(HAVE_NOUVEAU, [test "x$NOUVEAU" = xyes])
if test "x$NOUVEAU" = xyes; then
	AC_DEFINE(HAVE_NOUVEAU, 1, [Have nouveau (nvidia) support])
fi

AM_CONDITIONAL(HAVE_OMAP, [test "x$OMAP" = xyes])
if test "x$OMAP" = xyes; then
	AC_DEFINE(HAVE_OMAP, 1, [Have OMAP support])
fi

AM_CONDITIONAL(HAVE_EXYNOS, [test "x$EXYNOS" = xyes])
if test "x$EXYNOS" = xyes; then
	AC_DEFINE(HAVE_EXYNOS, 1, [Have EXYNOS support])
fi

AM_CONDITIONAL(HAVE_FREEDRENO, [test "x$FREEDRENO" = xyes])
if test "x$FREEDRENO" = xyes; then
	AC_DEFINE(HAVE_FREEDRENO, 1, [Have freedreno support])
fi

if test "x$FREEDRENO_KGSL" = xyes; then
	if test "x$FREEDRENO" != xyes; then
		AC_MSG_ERROR([Cannot enable freedreno KGSL interface if freedreno is disabled])
	fi
fi
AM_CONDITIONAL(HAVE_FREEDRENO_KGSL, [test "x$FREEDRENO_KGSL" = xyes])
if test "x$FREEDRENO_KGSL" = xyes; then
	AC_DEFINE(HAVE_FREEDRENO_KGSL, 1, [Have freedreno support for KGSL kernel interface])
fi

AM_CONDITIONAL(HAVE_RADEON, [test "x$RADEON" = xyes])
if test "x$RADEON" = xyes; then
	AC_DEFINE(HAVE_RADEON, 1, [Have radeon support])
fi

if test "x$AMDGPU" != xno; then
	# Detect cunit library
	PKG_CHECK_MODULES([CUNIT], [cunit >= 2.1], [have_cunit=yes], [have_cunit=no])
	# If pkg-config does not find cunit, check it using AC_CHECK_LIB.  We
	# do this because Debian (Ubuntu) lacks pkg-config file for cunit.
	# fixed in 2.1-2.dfsg-3: http://anonscm.debian.org/cgit/collab-maint/cunit.git/commit/?h=debian
	if test "x${have_cunit}" = "xno"; then
		AC_CHECK_LIB([cunit], [CU_initialize_registry], [have_cunit=yes], [have_cunit=no])
		if test "x${have_cunit}" = "xyes"; then
			CUNIT_LIBS="-lcunit"
			CUNIT_CFLAGS=""
			AC_SUBST([CUNIT_LIBS])
			AC_SUBST([CUNIT_CFLAGS])
		fi
	fi
else
	have_cunit=no
fi
AM_CONDITIONAL(HAVE_CUNIT, [test "x$have_cunit" != "xno"])

AM_CONDITIONAL(HAVE_AMDGPU, [test "x$AMDGPU" = xyes])
if test "x$AMDGPU" = xyes; then
	AC_DEFINE(HAVE_AMDGPU, 1, [Have amdgpu support])

	AC_DEFINE(HAVE_CUNIT, [test "x$have_cunit" != "xno"], [Enable CUNIT Have amdgpu support])

	if test "x$have_cunit" = "xno"; then
		AC_MSG_WARN([Could not find cunit library. Disabling amdgpu tests])
	fi
fi

AM_CONDITIONAL(HAVE_TEGRA, [test "x$TEGRA" = xyes])
if test "x$TEGRA" = xyes; then
	AC_DEFINE(HAVE_TEGRA, 1, [Have Tegra support])
fi

AM_CONDITIONAL(HAVE_VC4, [test "x$VC4" = xyes])
if test "x$VC4" = xyes; then
	AC_DEFINE(HAVE_VC4, 1, [Have VC4 support])
fi

AM_CONDITIONAL(HAVE_ETNAVIV, [test "x$ETNAVIV" = xyes])
if test "x$ETNAVIV" = xyes; then
	AC_DEFINE(HAVE_ETNAVIV, 1, [Have etnaviv support])
fi

AM_CONDITIONAL(HAVE_INSTALL_TESTS, [test "x$INSTALL_TESTS" = xyes])
if test "x$INSTALL_TESTS" = xyes; then
	AC_DEFINE(HAVE_INSTALL_TESTS, 1, [Install test programs])
fi

AC_ARG_ENABLE([cairo-tests],
              [AS_HELP_STRING([--enable-cairo-tests],
                              [Enable support for Cairo rendering in tests (default: auto)])],
              [CAIRO=$enableval], [CAIRO=auto])
if test "x$CAIRO" != xno; then
	PKG_CHECK_MODULES(CAIRO, cairo, [HAVE_CAIRO=yes], [HAVE_CAIRO=no])
fi
AC_MSG_CHECKING([whether to enable Cairo tests])
if test "x$CAIRO" = xauto; then
	CAIRO="$HAVE_CAIRO"
fi
if test "x$CAIRO" = xyes; then
	if ! test "x$HAVE_CAIRO" = xyes; then
		AC_MSG_ERROR([Cairo support required but not present])
	fi
	AC_DEFINE(HAVE_CAIRO, 1, [Have Cairo support])
fi
AC_MSG_RESULT([$CAIRO])
AM_CONDITIONAL(HAVE_CAIRO, [test "x$CAIRO" = xyes])

# xsltproc for docbook manpages
AC_ARG_ENABLE([manpages],
              AS_HELP_STRING([--enable-manpages], [enable manpages @<:@default=auto@:>@]),
              [MANS=$enableval], [MANS=auto])
AM_CONDITIONAL([BUILD_MANPAGES], [test "x$XSLTPROC" != "x" -a "x$MANS" != "xno"])

# check for offline man-pages stylesheet
AC_MSG_CHECKING([for docbook manpages stylesheet])
MANPAGES_STYLESHEET="http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl"
AC_PATH_PROGS_FEATURE_CHECK([XSLTPROC_TMP], [xsltproc],
                            AS_IF([`"$ac_path_XSLTPROC_TMP" --nonet "$MANPAGES_STYLESHEET" > /dev/null 2>&1`],
                                  [HAVE_MANPAGES_STYLESHEET=yes]))
if test "x$HAVE_MANPAGES_STYLESHEET" = "xyes"; then
        AC_SUBST(MANPAGES_STYLESHEET)
        AC_MSG_RESULT([yes])
else
        AC_MSG_RESULT([no])
fi
AM_CONDITIONAL([HAVE_MANPAGES_STYLESHEET], [test "x$HAVE_MANPAGES_STYLESHEET" = "xyes"])

AC_ARG_ENABLE(valgrind,
              [AS_HELP_STRING([--enable-valgrind],
                             [Build libdrm with  valgrind support (default: auto)])],
                             [VALGRIND=$enableval], [VALGRIND=auto])
if test "x$VALGRIND" != xno; then
	PKG_CHECK_MODULES(VALGRIND, [valgrind], [have_valgrind=yes], [have_valgrind=no])
fi
AC_MSG_CHECKING([whether to enable Valgrind support])
if test "x$VALGRIND" = xauto; then
	VALGRIND="$have_valgrind"
fi

if test "x$VALGRIND" = "xyes"; then
	if ! test "x$have_valgrind" = xyes; then
		AC_MSG_ERROR([Valgrind support required but not present])
	fi
	AC_DEFINE([HAVE_VALGRIND], 1, [Use valgrind intrinsics to suppress false warnings])
fi

AC_MSG_RESULT([$VALGRIND])

AC_ARG_WITH([kernel-source],
            [AS_HELP_STRING([--with-kernel-source],
              [specify path to linux kernel source])],
	    [kernel_source="$with_kernel_source"])
AC_SUBST(kernel_source)

AC_MSG_CHECKING([whether $CC supports __attribute__(("hidden"))])
AC_LINK_IFELSE([AC_LANG_PROGRAM([
    int foo_hidden( void ) __attribute__((visibility("hidden")));
])], HAVE_ATTRIBUTE_VISIBILITY="yes"; AC_MSG_RESULT([yes]), AC_MSG_RESULT([no]));

if test "x$HAVE_ATTRIBUTE_VISIBILITY" = xyes; then
    AC_DEFINE(HAVE_VISIBILITY, 1, [Compiler supports __attribute__(("hidden"))])
fi

AC_SUBST(WARN_CFLAGS)
AC_CONFIG_FILES([
	Makefile
	data/Makefile
	libkms/Makefile
	libkms/libkms.pc
	intel/Makefile
	intel/libdrm_intel.pc
	radeon/Makefile
	radeon/libdrm_radeon.pc
	amdgpu/Makefile
	amdgpu/libdrm_amdgpu.pc
	nouveau/Makefile
	nouveau/libdrm_nouveau.pc
	omap/Makefile
	omap/libdrm_omap.pc
	exynos/Makefile
	exynos/libdrm_exynos.pc
	freedreno/Makefile
	freedreno/libdrm_freedreno.pc
	tegra/Makefile
	tegra/libdrm_tegra.pc
	vc4/Makefile
	vc4/libdrm_vc4.pc
	etnaviv/Makefile
	etnaviv/libdrm_etnaviv.pc
	tests/Makefile
	tests/modeprint/Makefile
	tests/modetest/Makefile
	tests/kms/Makefile
	tests/kmstest/Makefile
	tests/proptest/Makefile
	tests/radeon/Makefile
	tests/amdgpu/Makefile
	tests/vbltest/Makefile
	tests/exynos/Makefile
	tests/tegra/Makefile
	tests/nouveau/Makefile
	tests/etnaviv/Makefile
	tests/util/Makefile
	man/Makefile
	libdrm.pc])
AC_OUTPUT

echo ""
echo "$PACKAGE_STRING will be compiled with:"
echo ""
echo "  libkms         $LIBKMS"
echo "  Intel API      $INTEL"
echo "  vmwgfx API     $VMWGFX"
echo "  Radeon API     $RADEON"
echo "  AMDGPU API     $AMDGPU"
echo "  Nouveau API    $NOUVEAU"
echo "  OMAP API       $OMAP"
echo "  EXYNOS API     $EXYNOS"
echo "  Freedreno API  $FREEDRENO (kgsl: $FREEDRENO_KGSL)"
echo "  Tegra API      $TEGRA"
echo "  VC4 API        $VC4"
echo "  Etnaviv API    $ETNAVIV"
echo ""
