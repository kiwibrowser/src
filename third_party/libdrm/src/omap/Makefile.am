AM_CFLAGS = \
	$(WARN_CFLAGS) \
	-I$(top_srcdir) \
	$(PTHREADSTUBS_CFLAGS) \
	-I$(top_srcdir)/include/drm

libdrm_omap_la_LTLIBRARIES = libdrm_omap.la
libdrm_omap_ladir = $(libdir)
libdrm_omap_la_LDFLAGS = -version-number 1:0:0 -no-undefined
libdrm_omap_la_LIBADD = ../libdrm.la @PTHREADSTUBS_LIBS@

libdrm_omap_la_SOURCES = omap_drm.c

libdrm_omapcommonincludedir = ${includedir}/omap
libdrm_omapcommoninclude_HEADERS = omap_drm.h

libdrm_omapincludedir = ${includedir}/libdrm
libdrm_omapinclude_HEADERS = omap_drmif.h

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = libdrm_omap.pc

TESTS = omap-symbol-check
EXTRA_DIST = $(TESTS)
