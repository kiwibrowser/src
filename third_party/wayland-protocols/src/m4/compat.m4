dnl noarch_pkgconfigdir only available in pkg-config 0.27 and newer
dnl http://lists.freedesktop.org/archives/pkg-config/2012-July/000875.html
dnl Ubuntu 14.04 provides only pkg-config 0.26 so lacks this function.
dnl
dnl The Wayland project maintains automated builds for Ubuntu 14.04 in
dnl a Launchpad PPA.  14.04 is a Long Term Support distro release, which
dnl will reach EOL April 2019, however the Wayland PPA may stop targeting
dnl it some time after the next LTS release (April 2016).
m4_ifndef([PKG_NOARCH_INSTALLDIR], [AC_DEFUN([PKG_NOARCH_INSTALLDIR], [
    noarch_pkgconfigdir='${datadir}'/pkgconfig
    AC_SUBST([noarch_pkgconfigdir])
])])
