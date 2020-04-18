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

if HAVE_SHARED_GLAPI
SHARED_GLAPI_CFLAGS = -DGLX_SHARED_GLAPI
SHARED_GLAPI_LIBS = $(top_builddir)/src/mapi/shared-glapi/libglapi.la
endif

SUBDIRS=. tests

GLAPI_LIB = $(top_builddir)/src/mapi/glapi/libglapi.la

if HAVE_XF86VIDMODE
EXTRA_DEFINES_XF86VIDMODE = -DXF86VIDMODE
endif

AM_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/include/GL/internal \
	-I$(top_srcdir)/src/mapi \
	-I$(top_srcdir)/src/mapi/glapi \
	-I$(top_builddir)/src/mapi \
	-I$(top_builddir)/src/mapi/glapi \
	$(SHARED_GLAPI_CFLAGS) \
	$(EXTRA_DEFINES_XF86VIDMODE) \
	-D_REENTRANT \
	-DDEFAULT_DRIVER_DIR=\"$(DRI_DRIVER_SEARCH_DIR)\" \
	$(DEFINES) \
	$(LIBDRM_CFLAGS) \
	$(DRI2PROTO_CFLAGS) \
	$(GLPROTO_CFLAGS) \
	$(X11_INCLUDES)

lib_LTLIBRARIES = lib@GL_LIB@.la

noinst_LTLIBRARIES = libglx.la

libglx_la_SOURCES = \
	  clientattrib.c \
	  clientinfo.c \
	  compsize.c \
	  create_context.c \
	  eval.c \
	  glxconfig.c \
	  glxcmds.c \
	  glxcurrent.c \
	  glx_error.c \
	  glxext.c \
	  glxextensions.c \
	  indirect_glx.c \
	  indirect.c \
	  indirect_init.c \
	  indirect_size.c \
	  indirect_window_pos.c \
	  indirect_texture_compression.c \
	  indirect_transpose_matrix.c \
	  indirect_vertex_array.c \
	  indirect_vertex_program.c \
	  pixel.c \
	  pixelstore.c \
	  render2.c \
	  renderpix.c \
	  single2.c \
	  singlepix.c \
	  vertarr.c \
	  xfont.c \
	  glx_pbuffer.c \
	  glx_query.c \
	  drisw_glx.c \
	  dri_common.c \
	  dri_glx.c \
	  XF86dri.c \
	  glxhash.c \
	  dri2_glx.c \
	  dri2.c \
	  applegl_glx.c

GL_LIBS = \
	libglx.la \
	$(SHARED_GLAPI_LIBS) \
	$(GLAPI_LIB) \
	$(GL_LIB_DEPS)

GL_LDFLAGS = \
	-Wl,-Bsymbolic \
	-version-number 1:2 -no-undefined

lib@GL_LIB@_la_SOURCES =
lib@GL_LIB@_la_LIBADD = $(GL_LIBS)
lib@GL_LIB@_la_LDFLAGS = $(GL_LDFLAGS)

# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.
all-local: lib@GL_LIB@.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR);
	ln -f .libs/lib@GL_LIB@.so.1.2.0 $(top_builddir)/$(LIB_DIR)/lib@GL_LIB@.so.1
	ln -sf lib@GL_LIB@.so.1 $(top_builddir)/$(LIB_DIR)/lib@GL_LIB@.so
