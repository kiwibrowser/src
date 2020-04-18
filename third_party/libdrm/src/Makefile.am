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

include Makefile.sources

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

AM_MAKEFLAGS = -s
AM_DISTCHECK_CONFIGURE_FLAGS = \
	--enable-udev \
	--enable-libkms \
	--enable-intel \
	--enable-radeon \
	--enable-amdgpu \
	--enable-nouveau \
	--enable-vc4 \
	--enable-vmwgfx \
	--enable-omap-experimental-api \
	--enable-exynos-experimental-api \
	--enable-freedreno \
	--enable-freedreno-kgsl\
	--enable-tegra-experimental-api \
	--enable-etnaviv-experimental-api \
	--enable-install-test-programs \
	--enable-cairo-tests \
	--enable-manpages \
	--enable-valgrind

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = libdrm.pc

if HAVE_LIBKMS
LIBKMS_SUBDIR = libkms
endif

if HAVE_INTEL
INTEL_SUBDIR = intel
endif

if HAVE_NOUVEAU
NOUVEAU_SUBDIR = nouveau
endif

if HAVE_RADEON
RADEON_SUBDIR = radeon
endif

if HAVE_AMDGPU
AMDGPU_SUBDIR = amdgpu
endif

if HAVE_OMAP
OMAP_SUBDIR = omap
endif

if HAVE_EXYNOS
EXYNOS_SUBDIR = exynos
endif

if HAVE_FREEDRENO
FREEDRENO_SUBDIR = freedreno
endif

if HAVE_TEGRA
TEGRA_SUBDIR = tegra
endif

if HAVE_VC4
VC4_SUBDIR = vc4
endif

if HAVE_ETNAVIV
ETNAVIV_SUBDIR = etnaviv
endif

if BUILD_MANPAGES
if HAVE_MANPAGES_STYLESHEET
MAN_SUBDIR = man
endif
endif

SUBDIRS = \
	. \
	$(LIBKMS_SUBDIR) \
	$(INTEL_SUBDIR) \
	$(NOUVEAU_SUBDIR) \
	$(RADEON_SUBDIR) \
	$(AMDGPU_SUBDIR) \
	$(OMAP_SUBDIR) \
	$(EXYNOS_SUBDIR) \
	$(FREEDRENO_SUBDIR) \
	$(TEGRA_SUBDIR) \
	$(VC4_SUBDIR) \
	$(ETNAVIV_SUBDIR) \
	data \
	tests \
	$(MAN_SUBDIR)

libdrm_la_LTLIBRARIES = libdrm.la
libdrm_ladir = $(libdir)
libdrm_la_LDFLAGS = -version-number 2:4:0 -no-undefined
libdrm_la_LIBADD = @CLOCK_LIB@ -lm

libdrm_la_CPPFLAGS = -I$(top_srcdir)/include/drm
AM_CFLAGS = \
	$(WARN_CFLAGS) \
	$(VALGRIND_CFLAGS)

libdrm_la_SOURCES = $(LIBDRM_FILES)

libdrmincludedir = ${includedir}
libdrminclude_HEADERS = $(LIBDRM_H_FILES)

klibdrmincludedir = ${includedir}/libdrm
klibdrminclude_HEADERS = $(LIBDRM_INCLUDE_H_FILES)

if HAVE_VMWGFX
klibdrminclude_HEADERS += $(LIBDRM_INCLUDE_VMWGFX_H_FILES)
endif

EXTRA_DIST = include/drm/README

copy-headers :
	cp -r $(kernel_source)/include/uapi/drm/*.h $(top_srcdir)/include/drm/

commit-headers : copy-headers
	git add include/drm/*.h
	git commit -am "Copy headers from kernel $$(GIT_DIR=$(kernel_source)/.git git describe)"
