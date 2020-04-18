TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = radeonsi_drv.so

C_SOURCES = \
	target.c \
	xorg.c

DRIVER_DEFINES = \
	-DHAVE_CONFIG_H -DGALLIUM_RBUG -DGALLIUM_TRACE -DGALLIUM_GALAHAD

DRIVER_PIPES = \
	$(TOP)/src/gallium/state_trackers/xorg/libxorgtracker.a \
	$(TOP)/src/gallium/drivers/radeonsi/libradeonsi.a \
	$(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/galahad/libgalahad.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a

DRIVER_LINKS = \
	$(shell $(PKG_CONFIG) --libs libdrm libdrm_radeon)

include ../Makefile.xorg
