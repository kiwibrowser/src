pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = main/gbm.pc

AM_CFLAGS = \
	-D_OS_UNIX=1 \
	-DMODULEDIR='"$(libdir)/gbm"' \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/gbm/main \
	$(LIBUDEV_CFLAGS) \
	$(LIBKMS_CFLAGS) \
	$(DLOPEN_CFLAGS) \
	$(DEFINES)

lib_LTLIBRARIES = libgbm.la
include_HEADERS = main/gbm.h

libgbm_la_SOURCES = \
	main/gbm.c \
	main/backend.c \
	main/common.c
libgbm_la_LDFLAGS = -version-info 1:0
libgbm_la_LIBADD = $(LIBUDEV_LIBS) $(LIBKMS_LIBS) $(DLOPEN_LIBS)

if HAVE_EGL_PLATFORM_WAYLAND
AM_CPPFLAGS = -DHAVE_WAYLAND_PLATFORM
AM_CFLAGS += $(WAYLAND_CFLAGS)
endif

if HAVE_DRI
noinst_LTLIBRARIES = libgbm_dri.la
libgbm_dri_la_SOURCES = \
	backends/dri/gbm_dri.c \
	backends/dri/driver_name.c

libgbm_dri_la_CFLAGS = \
	$(AM_CFLAGS) \
	-DDEFAULT_DRIVER_DIR='"$(DRI_DRIVER_SEARCH_DIR)"' \
	$(LIBDRM_CFLAGS)

libgbm_la_LIBADD += \
	libgbm_dri.la $(top_builddir)/src/mapi/shared-glapi/libglapi.la
endif

all-local: libgbm.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR);
	ln -f .libs/libgbm.so.1.0.0 $(top_builddir)/$(LIB_DIR)/libgbm.so
