TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = drisw

LIBRARY_DEFINES = -D__NOT_HAVE_DRM_H

LIBRARY_INCLUDES = \
	-I../dri \
	-I$(TOP)/include \
	-I$(TOP)/src/mapi \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/gallium/state_trackers/dri/common \
	-I$(TOP)/src/mesa/drivers/dri/common \
	-I$(TOP)/src/mesa/main \
	-D__NOT_HAVE_DRM_H


C_SOURCES = \
	dri_context.c \
	dri_screen.c \
	dri_drawable.c \
	drisw.c

include ../../../Makefile.template
