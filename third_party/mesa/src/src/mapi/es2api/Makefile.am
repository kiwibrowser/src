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

AM_CPPFLAGS = \
	$(DEFINES) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/mapi \
	-DMAPI_MODE_BRIDGE \
	-DMAPI_ABI_HEADER=\"glapi_mapi_tmp.h\"

AM_CFLAGS = $(VISIBILITY_CFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = glesv2.pc

GLES2_includedir = $(includedir)/GLES2
GLES2_include_HEADERS = \
	$(top_srcdir)/include/GLES2/gl2.h \
	$(top_srcdir)/include/GLES2/gl2ext.h \
	$(top_srcdir)/include/GLES2/gl2platform.h

lib_LTLIBRARIES = libGLESv2.la

libGLESv2_la_SOURCES = ../mapi/entry.c glapi_mapi_tmp.h
libGLESv2_la_LIBADD = $(GLESv2_LIB_DEPS)
libGLESv2_la_LDFLAGS = -version-number 2 -no-undefined

if HAVE_SHARED_GLAPI
libGLESv2_la_LIBADD += ../shared-glapi/libglapi.la
endif

include ../glapi/gen/glapi_gen.mk
glapi_mapi_tmp.h: ../glapi/gen/gl_and_es_API.xml $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi,$<,es2api)

BUILT_SOURCES = glapi_mapi_tmp.h
CLEANFILES = $(BUILT_SOURCES)

# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.
all-local: libGLESv2.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR);
	ln -f .libs/libGLESv2.so $(top_builddir)/$(LIB_DIR)/libGLESv2.so
	ln -f .libs/libGLESv2.so.2 $(top_builddir)/$(LIB_DIR)/libGLESv2.so.2
	ln -f .libs/libGLESv2.so.2.0.0 $(top_builddir)/$(LIB_DIR)/libGLESv2.so.2.0.0
