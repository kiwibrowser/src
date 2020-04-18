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

GLAPI_LIB = ../mapi/glapi/libglapi.la

if HAVE_XF86VIDMODE
EXTRA_DEFINES_XF86VIDMODE = -DXF86VIDMODE
endif

AM_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/gbm/main \
	$(DEFINES) \
	$(EGL_CFLAGS) \
	-D_EGL_NATIVE_PLATFORM=$(EGL_NATIVE_PLATFORM) \
	-D_EGL_DRIVER_SEARCH_DIR=\"$(EGL_DRIVER_INSTALL_DIR)\" \
	-D_EGL_OS_UNIX=1

lib_LTLIBRARIES = libEGL.la

libEGL_la_SOURCES = \
	eglapi.c \
	eglarray.c \
	eglconfig.c \
	eglcontext.c \
	eglcurrent.c \
	egldisplay.c \
	egldriver.c \
	eglfallbacks.c \
	eglglobals.c \
	eglimage.c \
	egllog.c \
	eglmisc.c \
	eglmode.c \
	eglscreen.c \
	eglstring.c \
	eglsurface.c \
	eglsync.c \
	eglcompiler.h \
	eglconfig.h \
	eglcontext.h \
	eglcurrent.h \
	egldefines.h \
	egldisplay.h \
	egldriver.h \
	eglglobals.h \
	eglimage.h \
	egllog.h \
	eglmisc.h \
	eglmode.h \
	eglmutex.h \
	eglscreen.h \
	eglstring.h \
	eglsurface.h \
	eglsync.h

libEGL_la_LIBADD = \
	$(EGL_LIB_DEPS)
libEGL_la_LDFLAGS = -version-number 1:0 -no-undefined

if HAVE_EGL_PLATFORM_X11
AM_CFLAGS += -DHAVE_X11_PLATFORM
AM_CFLAGS += $(XCB_DRI2_CFLAGS)
libEGL_la_LIBADD += $(XCB_DRI2_LIBS)
endif

if HAVE_EGL_PLATFORM_WAYLAND
AM_CFLAGS += -DHAVE_WAYLAND_PLATFORM
AM_CFLAGS += $(WAYLAND_CFLAGS)
libEGL_la_LIBADD += $(WAYLAND_LIBS)
libEGL_la_LIBADD += $(LIBDRM_LIBS)
libEGL_la_LIBADD += ../wayland/wayland-drm/libwayland-drm.la
endif

if HAVE_EGL_PLATFORM_DRM
AM_CFLAGS += -DHAVE_DRM_PLATFORM
libEGL_la_LIBADD += ../../gbm/libgbm.la
endif

if HAVE_EGL_PLATFORM_FBDEV
AM_CFLAGS += -DHAVE_FBDEV_PLATFORM
endif

if HAVE_EGL_PLATFORM_NULL
AM_CFLAGS += -DHAVE_NULL_PLATFORM
endif

if HAVE_EGL_DRIVER_GLX
AM_CFLAGS += -D_EGL_BUILT_IN_DRIVER_GLX
libEGL_la_LIBADD += ../drivers/glx/libegl_glx.la
libEGL_la_LIBADD += $(X11_LIBS) $(DLOPEN_LIBS)
endif

if HAVE_EGL_DRIVER_DRI2
AM_CFLAGS += -D_EGL_BUILT_IN_DRIVER_DRI2
AM_CFLAGS += -DHAVE_XCB_DRI2
libEGL_la_LIBADD += ../drivers/dri2/libegl_dri2.la
libEGL_la_LIBADD += $(LIBUDEV_LIBS) $(DLOPEN_LIBS) $(LIBDRM_LIBS)
endif

# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.
all-local: libEGL.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR);
	ln -f .libs/libEGL.so.1.0.0 $(top_builddir)/$(LIB_DIR)/libEGL.so.1
	ln -sf libEGL.so.1 $(top_builddir)/$(LIB_DIR)/libEGL.so

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = egl.pc

khrdir = $(includedir)/KHR
khr_HEADERS = $(top_srcdir)/include/KHR/khrplatform.h

egldir = $(includedir)/EGL
egl_HEADERS = \
	$(top_srcdir)/include/EGL/eglext.h \
	$(top_srcdir)/include/EGL/egl.h \
	$(top_srcdir)/include/EGL/eglmesaext.h \
	$(top_srcdir)/include/EGL/eglplatform.h
