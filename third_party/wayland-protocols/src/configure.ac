AC_PREREQ([2.64])

m4_define([wayland_protocols_major_version], [1])
m4_define([wayland_protocols_minor_version], [12])
m4_define([wayland_protocols_version],
          [wayland_protocols_major_version.wayland_protocols_minor_version])

AC_INIT([wayland-protocols],
        [wayland_protocols_version],
        [https://bugs.freedesktop.org/enter_bug.cgi?product=Wayland&component=wayland&version=unspecified],
        [wayland-protocols],
        [http://wayland.freedesktop.org/])

AC_CONFIG_MACRO_DIR([m4])

AC_SUBST([WAYLAND_PROTOCOLS_VERSION], [wayland_protocols_version])

AC_ARG_VAR([wayland_scanner], [The wayland-scanner executable])
AC_PATH_PROG([wayland_scanner], [wayland-scanner])
if test x$wayland_scanner = x; then
        if test "x$cross_compiling" != "xyes"; then
                PKG_CHECK_MODULES(WAYLAND_SCANNER, [wayland-scanner])
                wayland_scanner=`$PKG_CONFIG --variable=wayland_scanner wayland-scanner`
        else
                AC_MSG_WARN([You are cross compiling without wayland-scanner in your path.  make check will fail.])
        fi
fi

AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz tar-ustar])

AM_SILENT_RULES([yes])

PKG_NOARCH_INSTALLDIR

AC_CONFIG_FILES([
	Makefile
	wayland-protocols.pc
	wayland-protocols-uninstalled.pc
	])
AC_OUTPUT

AC_MSG_RESULT([
	Version			${WAYLAND_PROTOCOLS_VERSION}
	Prefix			${prefix}
	])
