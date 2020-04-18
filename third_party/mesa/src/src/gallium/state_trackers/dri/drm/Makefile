TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = dridrm

LIBRARY_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/mapi \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/gallium/state_trackers/dri/common \
	-I$(TOP)/src/mesa/drivers/dri/common \
	-I$(TOP)/src/mesa/main \
         $(shell $(PKG_CONFIG) --cflags-only-I libdrm)


C_SOURCES = \
	dri_context.c \
	dri_screen.c \
	dri_drawable.c \
	dri2.c

#	$(TOP)/src/mesa/drivers/dri/common/utils.c \
	$(TOP)/src/mesa/drivers/dri/common/dri_util.c \
	$(TOP)/src/mesa/drivers/dri/common/xmlconfig.c \
	$(TOP)/src/mesa/drivers/common/driverfuncs.c

include ../../../Makefile.template
