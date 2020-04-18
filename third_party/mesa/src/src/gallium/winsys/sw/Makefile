# src/gallium/winsys/sw/Makefile
TOP = ../../../..
include $(TOP)/configs/current

SUBDIRS = null wrapper

# TODO: this should go through a further indirection level
# (i.e. EGL should set a variable that is checked here)
ifneq ($(findstring x11, $(EGL_PLATFORMS)),)
SUBDIRS += xlib
endif

ifneq ($(findstring fbdev, $(EGL_PLATFORMS)),)
SUBDIRS += fbdev
endif

ifneq ($(findstring wayland, $(EGL_PLATFORMS)),)
SUBDIRS += wayland
endif

default install clean:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) $@) || exit 1; \
		fi \
	done
