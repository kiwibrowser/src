TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = vmwgfx_dri.so

PIPE_DRIVERS = \
	$(TOP)/src/gallium/state_trackers/dri/drm/libdridrm.a \
	$(TOP)/src/gallium/winsys/svga/drm/libsvgadrm.a \
	$(TOP)/src/gallium/winsys/sw/wrapper/libwsw.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(TOP)/src/gallium/drivers/svga/libsvga.a

C_SOURCES = \
	target.c \
	vmw_powf.c \
	$(COMMON_GALLIUM_SOURCES)

DRIVER_DEFINES = \
	-DGALLIUM_RBUG -DGALLIUM_TRACE

include ../Makefile.dri

symlinks:
