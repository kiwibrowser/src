TOP = ../../../..
include $(TOP)/configs/current

LIBBASENAME = XvMCnouveau

PIPE_DRIVERS = \
	$(TOP)/src/gallium/winsys/nouveau/drm/libnouveaudrm.a \
	$(TOP)/src/gallium/drivers/nv50/libnv50.a \
	$(TOP)/src/gallium/drivers/nvc0/libnvc0.a \
	$(TOP)/src/gallium/drivers/nouveau/libnouveau.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/auxiliary/libgallium.a


C_SOURCES = \
	target.c \
	$(TOP)/src/gallium/auxiliary/vl/vl_winsys_dri.c \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

DRIVER_LIBS = $(shell $(PKG_CONFIG) libdrm_nouveau --libs) -lXfixes

DRIVER_INCLUDES = $(shell $(PKG_CONFIG) libdrm --cflags-only-I)

LINK_WITH_CXX=1

include ../Makefile.xvmc

symlinks:
