TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = ws_xlib

LIBRARY_INCLUDES = \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/gallium/auxiliary \
	$(X11_CFLAGS)

C_SOURCES = \
	xlib_sw_winsys.c 

include ../../../Makefile.template


