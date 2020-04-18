TOP = ../../../..
include $(TOP)/configs/current

LIBBASENAME = vdpau_softpipe

DRIVER_DEFINES = -DGALLIUM_SOFTPIPE
DRIVER_INCLUDES =

PIPE_DRIVERS = \
	$(TOP)/src/gallium/winsys/sw/xlib/libws_xlib.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/auxiliary/libgallium.a

C_SOURCES = \
	$(TOP)/src/gallium/auxiliary/vl/vl_winsys_xsp.c

DRIVER_LIBS =

include ../Makefile.vdpau

symlinks:
