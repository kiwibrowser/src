TOP = ../../../..
include $(TOP)/configs/current

LIBBASENAME = vdpau_radeonsi

DRIVER_INCLUDES = $(shell $(PKG_CONFIG) libdrm --cflags-only-I)

PIPE_DRIVERS = \
	$(TOP)/src/gallium/drivers/radeonsi/libradeonsi.a \
	$(TOP)/src/gallium/state_trackers/dri/drm/libdridrm.a \
	$(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(TOP)/src/gallium/drivers/noop/libnoop.a \
	$(TOP)/src/gallium/auxiliary/libgallium.a

C_SOURCES = \
	target.c \
	$(TOP)/src/gallium/auxiliary/vl/vl_winsys_dri.c \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

DRIVER_DEFINES = \
	-DGALLIUM_RBUG -DGALLIUM_TRACE -DGALLIUM_NOOP

DRIVER_LIBS = $(LIBDRM_LIBS) $(RADEON_LIBS) -lXfixes

include ../Makefile.vdpau

symlinks:
