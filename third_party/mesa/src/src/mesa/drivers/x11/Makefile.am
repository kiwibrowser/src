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

# This builds "stand-alone" Mesa, a version of libGL that does not need the
# GLX extension.  All rendering is converted to Xlib calls.  No hardware
# acceleration.

AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/mapi \
	-I$(top_srcdir)/src/mesa \
	-I$(top_srcdir)/src/mesa/main \
	$(X11_INCLUDES) \
	$(API_DEFINES) \
	$(DEFINES)

if HAVE_X11_DRIVER
lib_LTLIBRARIES = lib@GL_LIB@.la
endif

lib@GL_LIB@_la_SOURCES = \
	glxapi.h \
	glxheader.h \
	xfonts.h \
	xmesaP.h \
	fakeglx.c \
	glxapi.c \
	xfonts.c \
	xm_api.c \
	xm_buffer.c \
	xm_dd.c \
	xm_line.c \
	xm_tri.c

# Force usage of a C++ linker
nodist_EXTRA_lib@GL_LIB@_la_SOURCES = dummy.cpp

GL_MAJOR = 1
GL_MINOR = 6
GL_PATCH = 0

lib@GL_LIB@_la_LIBADD = \
	$(top_builddir)/src/mesa/libmesa.la \
	$(top_builddir)/src/mapi/glapi/libglapi.la
lib@GL_LIB@_la_LDFLAGS = \
	-version-number $(GL_MAJOR):$(GL_MINOR):$(GL_PATCH) \
	-no-undefined \
	$(GL_LIB_DEPS)

if BUILD_SHARED
# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.
all-local: lib@GL_LIB@.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR);
	ln -f .libs/lib@GL_LIB@.so.$(GL_MAJOR).$(GL_MINOR).$(GL_PATCH) $(top_builddir)/$(LIB_DIR)/lib@GL_LIB@.so.$(GL_MAJOR)
	ln -sf lib@GL_LIB@.so.$(GL_MAJOR) $(top_builddir)/$(LIB_DIR)/lib@GL_LIB@.so
endif
