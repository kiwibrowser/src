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

AM_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/egl/main \
	-I$(top_srcdir)/src/gbm/main \
	-I$(top_srcdir)/src/gbm/backends/dri \
	-I$(top_srcdir)/src/egl/wayland/wayland-egl \
	-I$(top_srcdir)/src/egl/wayland/wayland-drm \
	-I$(top_builddir)/src/egl/wayland/wayland-drm \
	$(DEFINES) \
	$(LIBDRM_CFLAGS) \
	$(LIBUDEV_CFLAGS) \
	$(LIBKMS_CFLAGS) \
	-DDEFAULT_DRIVER_DIR=\"$(DRI_DRIVER_SEARCH_DIR)\"

noinst_LTLIBRARIES = libegl_dri2.la

libegl_dri2_la_SOURCES = \
	egl_dri2.c \
	common.c

libegl_dri2_la_LIBADD = \
	$(EGL_LIB_DEPS)

if HAVE_SHARED_GLAPI
AM_CFLAGS += -DHAVE_SHARED_GLAPI
endif

if HAVE_EGL_PLATFORM_X11
libegl_dri2_la_SOURCES += platform_x11.c
AM_CFLAGS += -DHAVE_X11_PLATFORM
AM_CFLAGS += $(XCB_DRI2_CFLAGS)
endif

if HAVE_EGL_PLATFORM_WAYLAND
libegl_dri2_la_SOURCES += platform_wayland.c
AM_CFLAGS += -DHAVE_WAYLAND_PLATFORM
AM_CFLAGS += $(WAYLAND_CFLAGS)
endif

if HAVE_EGL_PLATFORM_DRM
libegl_dri2_la_SOURCES += platform_drm.c
AM_CFLAGS += -DHAVE_DRM_PLATFORM
endif
