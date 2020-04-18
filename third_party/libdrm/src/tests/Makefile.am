SUBDIRS = util kms modeprint proptest modetest vbltest

if HAVE_LIBKMS
SUBDIRS += kmstest
endif

if HAVE_RADEON
SUBDIRS += radeon
endif

if HAVE_AMDGPU
if HAVE_CUNIT
SUBDIRS += amdgpu
endif
endif

if HAVE_EXYNOS
SUBDIRS += exynos
endif

if HAVE_TEGRA
SUBDIRS += tegra
endif

if HAVE_ETNAVIV
SUBDIRS += etnaviv
endif

if HAVE_NOUVEAU
SUBDIRS += nouveau
endif

AM_CFLAGS = \
	$(WARN_CFLAGS)\
	-I $(top_srcdir)/include/drm \
	-I $(top_srcdir)

LDADD = $(top_builddir)/libdrm.la

TESTS = \
	drmsl \
	hash \
	random

check_PROGRAMS = \
	$(TESTS) \
	drmdevice
