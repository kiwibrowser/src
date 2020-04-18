TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = svgadrm

# get C_SOURCES
include Makefile.sources

LIBRARY_INCLUDES = \
       -I$(TOP)/src/gallium/drivers/svga \
       -I$(TOP)/src/gallium/drivers/svga/include \
       $(shell $(PKG_CONFIG) libdrm --cflags-only-I)

LIBRARY_DEFINES = \
       -std=gnu99 -fvisibility=hidden \
       -DHAVE_STDINT_H -D_FILE_OFFSET_BITS=64 \
       $(shell $(PKG_CONFIG) libdrm --cflags-only-other)

include ../../../Makefile.template
