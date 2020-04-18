TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = i915_dri.so

PIPE_DRIVERS = \
	$(TOP)/src/gallium/state_trackers/dri/drm/libdridrm.a \
	$(TOP)/src/gallium/winsys/i915/drm/libi915drm.a \
	$(TOP)/src/gallium/winsys/sw/wrapper/libwsw.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/galahad/libgalahad.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(TOP)/src/gallium/drivers/i915/libi915.a

C_SOURCES = \
	target.c \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

DRIVER_DEFINES = \
	-DGALLIUM_RBUG -DGALLIUM_TRACE -DGALLIUM_GALAHAD -DGALLIUM_SOFTPIPE

ifeq ($(MESA_LLVM),1)
PIPE_DRIVERS += $(TOP)/src/gallium/drivers/llvmpipe/libllvmpipe.a
DRIVER_DEFINES += -DGALLIUM_LLVMPIPE
endif

include ../Makefile.dri

GALLIUM_DRI_LIB_DEPS += $(INTEL_LIBS)

symlinks:
