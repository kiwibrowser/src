TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = xorgtracker

LIBRARY_INCLUDES = \
	-DHAVE_CONFIG_H \
	$(shell $(PKG_CONFIG) xextproto --atleast-version=7.0.99.1 \
				&& echo "-DHAVE_XEXTPROTO_71") \
	$(shell $(PKG_CONFIG) libkms --atleast-version=1.0 \
				&& echo "-DHAVE_LIBKMS") \
	$(shell $(PKG_CONFIG) libkms --silence-errors --cflags-only-I) \
	$(shell $(PKG_CONFIG) --cflags-only-I pixman-1 xorg-server libdrm xproto dri2proto) \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/drivers/dri/common \
	-I$(TOP)/src/mesa/main

C_SOURCES = $(wildcard ./*.c)

include ../../Makefile.template
