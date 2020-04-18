TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = vdpautracker

VDPAU_MAJOR = 1
VDPAU_MINOR = 0
LIBRARY_DEFINES = -DVER_MAJOR=$(VDPAU_MAJOR) -DVER_MINOR=$(VDPAU_MINOR) $(STATE_TRACKER_DEFINES)

LIBRARY_INCLUDES = $(shell $(PKG_CONFIG) --cflags-only-I vdpau)

C_SOURCES = htab.c \
	    ftab.c \
	    device.c \
	    query.c \
	    surface.c \
	    decode.c \
	    presentation.c \
	    bitmap.c \
	    output.c \
	    preemption.c \
	    mixer.c


include ../../Makefile.template

