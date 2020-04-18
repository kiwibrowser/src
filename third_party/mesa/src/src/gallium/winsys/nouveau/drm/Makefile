TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = nouveaudrm

C_SOURCES = nouveau_drm_winsys.c

LIBRARY_INCLUDES = $(shell $(PKG_CONFIG) libdrm libdrm_nouveau --cflags-only-I)
LIBRARY_DEFINES = $(shell $(PKG_CONFIG) libdrm libdrm_nouveau --cflags-only-other)

include ../../../Makefile.template
