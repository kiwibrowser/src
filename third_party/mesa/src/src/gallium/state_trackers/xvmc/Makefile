TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = xvmctracker

LIBRARY_INCLUDES = $(shell $(PKG_CONFIG) --cflags-only-I xvmc)

C_SOURCES = block.c \
            surface.c \
            context.c \
            subpicture.c \
            attributes.c

include ../../Makefile.template
