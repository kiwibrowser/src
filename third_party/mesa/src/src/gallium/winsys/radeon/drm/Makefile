
TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = radeonwinsys

# get C_SOURCES
include Makefile.sources

LIBRARY_INCLUDES = -I$(TOP)/include \
		   $(shell $(PKG_CONFIG) libdrm --cflags-only-I)

include ../../../Makefile.template

symlinks:
