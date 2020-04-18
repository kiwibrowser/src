include Makefile.sources

AM_CFLAGS = \
	$(WARN_CFLAGS) \
	-I$(top_srcdir) \
	$(PTHREADSTUBS_CFLAGS) \
	-I$(top_srcdir)/include/drm

libdrm_etnaviv_ladir = $(libdir)
libdrm_etnaviv_la_LTLIBRARIES = libdrm_etnaviv.la
libdrm_etnaviv_la_LDFLAGS = -version-number 1:0:0 -no-undefined
libdrm_etnaviv_la_LIBADD = \
	../libdrm.la \
	@PTHREADSTUBS_LIBS@ \
	@CLOCK_LIB@

libdrm_etnaviv_la_SOURCES = $(LIBDRM_ETNAVIV_FILES)

libdrm_etnavivincludedir = ${includedir}/libdrm
libdrm_etnavivinclude_HEADERS = $(LIBDRM_ETNAVIV_H_FILES)

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = libdrm_etnaviv.pc

TESTS = etnaviv-symbol-check
EXTRA_DIST = $(TESTS)
