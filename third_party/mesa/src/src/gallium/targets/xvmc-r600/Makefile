TOP = ../../../..
include $(TOP)/configs/current

LIBBASENAME = XvMCr600

DRIVER_INCLUDES = $(shell $(PKG_CONFIG) libdrm --cflags-only-I)

PIPE_DRIVERS = \
	$(TOP)/src/gallium/drivers/r600/libr600.a \
	$(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/auxiliary/libgallium.a

C_SOURCES = \
	target.c \
	$(TOP)/src/gallium/auxiliary/vl/vl_winsys_dri.c \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

DRIVER_LIBS = $(LIBDRM_LIBS) $(RADEON_LIBS) -lXfixes

include ../Makefile.xvmc

symlinks:
