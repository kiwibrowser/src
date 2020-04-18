# Copyright © 2012 Intel Corporation
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

TOP=$(top_srcdir)

AM_CPPFLAGS = \
	$(DEFINES) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/mapi \
	-DMAPI_ABI_HEADER=\"vgapi_tmp.h\"

AM_CFLAGS = $(VISIBILITY_CFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = vg.pc

VG_includedir = $(includedir)/VG
VG_include_HEADERS = \
	$(top_srcdir)/include/VG/openvg.h \
	$(top_srcdir)/include/VG/vgext.h \
	$(top_srcdir)/include/VG/vgplatform.h \
	$(top_srcdir)/include/VG/vgu.h

lib_LTLIBRARIES = libOpenVG.la

include ../mapi/sources.mak
libOpenVG_la_SOURCES = $(MAPI_FILES) vgapi_tmp.h
libOpenVG_la_LIBADD = $(VG_LIB_DEPS)
libOpenVG_la_LDFLAGS = -version-number 1 -no-undefined

vgapi_tmp.h: $(srcdir)/vgapi.csv $(top_srcdir)/src/mapi/mapi/mapi_abi.py
	$(AM_V_GEN) $(PYTHON2) $(PYTHON_FLAGS) $(top_srcdir)/src/mapi/mapi/mapi_abi.py \
		--printer vgapi --mode lib $< > $@

BUILT_SOURCES = vgapi_tmp.h
CLEANFILES = $(BUILT_SOURCES)

# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.
all-local: libOpenVG.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR);
	ln -f .libs/libOpenVG.so $(top_builddir)/$(LIB_DIR)/libOpenVG.so
	ln -f .libs/libOpenVG.so.1 $(top_builddir)/$(LIB_DIR)/libOpenVG.so.1
	ln -f .libs/libOpenVG.so.1.0.0 $(top_builddir)/$(LIB_DIR)/libOpenVG.so.1.0.0
