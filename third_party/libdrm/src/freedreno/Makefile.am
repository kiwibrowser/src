AUTOMAKE_OPTIONS=subdir-objects
include Makefile.sources

AM_CFLAGS = \
	$(WARN_CFLAGS) \
	-I$(top_srcdir) \
	$(PTHREADSTUBS_CFLAGS) \
	$(VALGRIND_CFLAGS) \
	-I$(top_srcdir)/include/drm

libdrm_freedreno_la_LTLIBRARIES = libdrm_freedreno.la
libdrm_freedreno_ladir = $(libdir)
libdrm_freedreno_la_LDFLAGS = -version-number 1:0:0 -no-undefined
libdrm_freedreno_la_LIBADD = \
	../libdrm.la \
	@PTHREADSTUBS_LIBS@ \
	@CLOCK_LIB@

libdrm_freedreno_la_SOURCES = $(LIBDRM_FREEDRENO_FILES)
if HAVE_FREEDRENO_KGSL
libdrm_freedreno_la_SOURCES += $(LIBDRM_FREEDRENO_KGSL_FILES)
endif

libdrm_freedrenocommonincludedir = ${includedir}/freedreno
libdrm_freedrenocommoninclude_HEADERS = $(LIBDRM_FREEDRENO_H_FILES)

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = libdrm_freedreno.pc

TESTS = freedreno-symbol-check
EXTRA_DIST = $(TESTS)
