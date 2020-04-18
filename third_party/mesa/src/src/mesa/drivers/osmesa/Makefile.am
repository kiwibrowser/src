

# Copyright © 2012 Laurent Carlier <lordheavym@gmail.com>
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

AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/mapi \
	-I$(top_srcdir)/src/mesa/ \
	$(DEFINES) \
	$(API_DEFINES)
AM_CFLAGS = $(PTHREAD_CFLAGS) \
	$(VISIBILITY_CFLAGS)
AM_CXXFLAGS = $(PTHREAD_CFLAGS) \
	$(VISIBILITY_CXXFLAGS)

lib_LTLIBRARIES = lib@OSMESA_LIB@.la

nodist_EXTRA_lib@OSMESA_LIB@_la_SOURCES = dummy.cpp
lib@OSMESA_LIB@_la_SOURCES = osmesa.c

lib@OSMESA_LIB@_la_LDFLAGS = -module -version-number @OSMESA_VERSION@ -no-undefined

if HAVE_SHARED_GLAPI
GLAPI_LIB = $(top_builddir)/src/mapi/shared-glapi/libglapi.la
else
GLAPI_LIB = $(top_builddir)/src/mapi/glapi/libglapi.la
endif
lib@OSMESA_LIB@_la_LIBADD = \
	$(top_builddir)/src/mesa/libmesa.la \
	$(GLAPI_LIB) \
	$(OSMESA_LIB_DEPS)

if BUILD_SHARED
# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.
all-local: lib@OSMESA_LIB@.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR);
	ln -f .libs/lib@OSMESA_LIB@.so $(top_builddir)/$(LIB_DIR)/lib@OSMESA_LIB@.so;
	ln -f .libs/lib@OSMESA_LIB@.so.@OSMESA_VERSION@ $(top_builddir)/$(LIB_DIR)/lib@OSMESA_LIB@.so.@OSMESA_VERSION@;
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = osmesa.pc
