TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = i915_drv.so

C_SOURCES = \
	intel_target.c \
	intel_xorg.c

DRIVER_DEFINES = \
	-DHAVE_CONFIG_H -DGALLIUM_RBUG -DGALLIUM_TRACE -DGALLIUM_GALAHAD

DRIVER_PIPES = \
	$(TOP)/src/gallium/state_trackers/xorg/libxorgtracker.a \
	$(TOP)/src/gallium/winsys/i915/drm/libi915drm.a \
	$(TOP)/src/gallium/drivers/i915/libi915.a \
	$(TOP)/src/gallium/drivers/galahad/libgalahad.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a

DRIVER_LINKS = \
	$(shell $(PKG_CONFIG) --libs libdrm libdrm_intel)

include ../Makefile.xorg
