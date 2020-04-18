include Makefile.sources

AM_CFLAGS = \
	$(WARN_CFLAGS) \
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)

libkms_la_LTLIBRARIES = libkms.la
libkms_ladir = $(libdir)
libkms_la_LDFLAGS = -version-number 1:0:0 -no-undefined
libkms_la_LIBADD = ../libdrm.la

libkms_la_SOURCES = $(LIBKMS_FILES)

if HAVE_VMWGFX
libkms_la_SOURCES += $(LIBKMS_VMWGFX_FILES)
endif

if HAVE_INTEL
libkms_la_SOURCES += $(LIBKMS_INTEL_FILES)
endif

if HAVE_NOUVEAU
libkms_la_SOURCES += $(LIBKMS_NOUVEAU_FILES)
endif

if HAVE_RADEON
libkms_la_SOURCES += $(LIBKMS_RADEON_FILES)
endif

if HAVE_EXYNOS
libkms_la_SOURCES += $(LIBKMS_EXYNOS_FILES)
AM_CFLAGS += -I$(top_srcdir)/exynos
endif

libkmsincludedir = ${includedir}/libkms
libkmsinclude_HEADERS = $(LIBKMS_H_FILES)

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = libkms.pc

TESTS = kms-symbol-check
EXTRA_DIST = $(TESTS)
