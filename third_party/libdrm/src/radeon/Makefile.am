# Copyright © 2008 Jérôme Glisse
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# Authors:
#    Jérôme Glisse <glisse@freedesktop.org>

include Makefile.sources

AM_CFLAGS = \
	$(WARN_CFLAGS) \
	-I$(top_srcdir) \
	$(PTHREADSTUBS_CFLAGS) \
	-I$(top_srcdir)/include/drm

libdrm_radeon_la_LTLIBRARIES = libdrm_radeon.la
libdrm_radeon_ladir = $(libdir)
libdrm_radeon_la_LDFLAGS = -version-number 1:0:1 -no-undefined
libdrm_radeon_la_LIBADD = ../libdrm.la @PTHREADSTUBS_LIBS@

libdrm_radeon_la_SOURCES = $(LIBDRM_RADEON_FILES)

libdrm_radeonincludedir = ${includedir}/libdrm
libdrm_radeoninclude_HEADERS = $(LIBDRM_RADEON_H_FILES)

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = libdrm_radeon.pc

TESTS = radeon-symbol-check
EXTRA_DIST = $(LIBDRM_RADEON_BOF_FILES) $(TESTS)
