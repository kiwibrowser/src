TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = vatracker

VA_MAJOR = 0
VA_MINOR = 3
LIBRARY_DEFINES = -DVER_MAJOR=$(VA_MAJOR) -DVER_MINOR=$(VA_MINOR) $(STATE_TRACKER_DEFINES)

LIBRARY_INCLUDES = $(shell $(PKG_CONFIG) --cflags-only-I libva)

C_SOURCES = htab.c \
	    ftab.c \
	    va_context.c \
	    va_image.c \
	    va_subpicture.c \
	    va_buffer.c \
	    va_config.c \
            va_picture.c \
            va_surface.c \
	    va_display.c
	    


include ../../Makefile.template

