TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = swrast_dri.so

DRIVER_DEFINES = \
	-D__NOT_HAVE_DRM_H -DGALLIUM_SOFTPIPE \
	-DGALLIUM_RBUG -DGALLIUM_TRACE

PIPE_DRIVERS = \
	$(TOP)/src/gallium/state_trackers/dri/sw/libdrisw.a \
	$(TOP)/src/gallium/winsys/sw/dri/libswdri.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a

ifeq ($(MESA_LLVM),1)
PIPE_DRIVERS += $(TOP)/src/gallium/drivers/llvmpipe/libllvmpipe.a
DRIVER_DEFINES += -DGALLIUM_LLVMPIPE
endif

SWRAST_COMMON_GALLIUM_SOURCES = \
	$(TOP)/src/mesa/drivers/dri/common/utils.c \
	$(TOP)/src/mesa/drivers/dri/common/drisw_util.c \
	$(TOP)/src/mesa/drivers/dri/common/xmlconfig.c

C_SOURCES = \
	swrast_drm_api.c \
	$(SWRAST_COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES =

include ../Makefile.dri

INCLUDES += \
	-I$(TOP)/src/gallium/winsys/sw/dri

symlinks:
