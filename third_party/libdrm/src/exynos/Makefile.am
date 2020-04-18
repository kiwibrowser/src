AM_CFLAGS = \
	$(WARN_CFLAGS) \
	-I$(top_srcdir) \
	$(PTHREADSTUBS_CFLAGS) \
	-I$(top_srcdir)/include/drm

libdrm_exynos_la_LTLIBRARIES = libdrm_exynos.la
libdrm_exynos_ladir = $(libdir)
libdrm_exynos_la_LDFLAGS = -version-number 1:0:0 -no-undefined
libdrm_exynos_la_LIBADD = ../libdrm.la @PTHREADSTUBS_LIBS@

libdrm_exynos_la_SOURCES = \
	exynos_drm.c \
	exynos_fimg2d.c \
	fimg2d_reg.h

libdrm_exynoscommonincludedir = ${includedir}/exynos
libdrm_exynoscommoninclude_HEADERS = exynos_drm.h exynos_fimg2d.h

libdrm_exynosincludedir = ${includedir}/libdrm
libdrm_exynosinclude_HEADERS = exynos_drmif.h

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = libdrm_exynos.pc

TESTS = exynos-symbol-check
EXTRA_DIST = $(TESTS)
