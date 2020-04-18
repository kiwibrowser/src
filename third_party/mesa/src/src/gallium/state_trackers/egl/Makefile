TOP = ../../../..
include $(TOP)/configs/current

common_INCLUDES = \
	-I. \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/egl/main \
	-I$(TOP)/src/egl/wayland/wayland-drm/ \
	-I$(TOP)/include

common_SOURCES = $(wildcard common/*.c)
common_OBJECTS = $(common_SOURCES:.c=.o)

x11_INCLUDES = \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/glx \
	-I$(TOP)/src/mapi \
	-I$(TOP)/src/mesa \
	$(X11_CFLAGS) \
	$(shell $(PKG_CONFIG) --cflags-only-I libdrm dri2proto)

x11_SOURCES = $(wildcard x11/*.c) \
	      $(TOP)/src/glx/dri2.c
x11_OBJECTS = $(x11_SOURCES:.c=.o)

wayland_INCLUDES = \
	-I$(TOP)/src/gallium/winsys \
	-I$(TOP)/src/egl/wayland/wayland-egl \
	-I$(TOP)/src/egl/wayland/wayland-drm \
	$(shell $(PKG_CONFIG) --cflags-only-I libdrm wayland-client wayland-server)

wayland_SOURCES = $(wildcard wayland/*.c)
wayland_OBJECTS = $(wayland_SOURCES:.c=.o)

drm_INCLUDES = -I$(TOP)/src/gallium/winsys $(shell $(PKG_CONFIG) --cflags-only-I libdrm) \
	       -I$(TOP)/src/gbm/main -I$(TOP)/src/gallium/state_trackers/gbm
drm_SOURCES = $(wildcard drm/*.c)
drm_OBJECTS = $(drm_SOURCES:.c=.o)


fbdev_INCLUDES = -I$(TOP)/src/gallium/winsys/sw
fbdev_SOURCES = $(wildcard fbdev/*.c)
fbdev_OBJECTS = $(fbdev_SOURCES:.c=.o)


null_INCLUDES = -I$(TOP)/src/gallium/winsys/sw
null_SOURCES = $(wildcard null/*.c)
null_OBJECTS = $(null_SOURCES:.c=.o)


ALL_INCLUDES = $(common_INCLUDES) $(x11_INCLUDES) $(drm_INCLUDES) $(fbdev_INCLUDES) $(null_INCLUDES) $(wayland_INCLUDES)
ALL_SOURCES = $(common_SOURCES) $(x11_SOURCES) $(drm_SOURCES) $(fbdev_SOURCES) $(null_SOURCES) $(wayland_SOURCES)

EGL_OBJECTS = $(common_OBJECTS)
EGL_CPPFLAGS = $(common_INCLUDES)

# add backends
ifneq ($(findstring x11, $(EGL_PLATFORMS)),)
EGL_OBJECTS += $(x11_OBJECTS)
EGL_CPPFLAGS += -DHAVE_X11_BACKEND
endif
ifneq ($(findstring wayland, $(EGL_PLATFORMS)),)
EGL_OBJECTS += $(wayland_OBJECTS)
EGL_CPPFLAGS += -DHAVE_WAYLAND_BACKEND
DEFINES += -DHAVE_WAYLAND_BACKEND
common_INCLUDES += $(wayland_INCLUDES)
endif
ifneq ($(findstring drm, $(EGL_PLATFORMS)),)
EGL_OBJECTS += $(drm_OBJECTS)
EGL_CPPFLAGS += -DHAVE_DRM_BACKEND
endif
ifneq ($(findstring fbdev, $(EGL_PLATFORMS)),)
EGL_OBJECTS += $(fbdev_OBJECTS)
EGL_CPPFLAGS += -DHAVE_FBDEV_BACKEND
endif
ifneq ($(findstring null, $(EGL_PLATFORMS)),)
EGL_OBJECTS += $(null_OBJECTS)
EGL_CPPFLAGS += -DHAVE_NULL_BACKEND
endif

##### TARGETS #####

default: depend libegl.a

libegl.a: $(EGL_OBJECTS) Makefile
	$(MKLIB) -o egl -static $(EGL_OBJECTS)

depend: 
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(ALL_INCLUDES) $(ALL_SOURCES) 2> /dev/null

clean:
	rm -f libegl.a
	rm -f $(EGL_OBJECTS)
	rm -f depend depend.bak

# Dummy target
install:
	@echo -n ""

##### RULES #####

define egl-cc
$(CC) -c $(common_INCLUDES) $($(1)_INCLUDES) $(DEFINES) $(CFLAGS) $< -o $@
endef

$(common_OBJECTS): %.o: %.c
	$(CC) -c $(EGL_CPPFLAGS) $(DEFINES) $(CFLAGS) $< -o $@

$(x11_OBJECTS): %.o: %.c
	$(call egl-cc,x11)

$(wayland_OBJECTS): %.o: %.c
	$(call egl-cc,wayland)

$(drm_OBJECTS): %.o: %.c
	$(call egl-cc,drm)

$(fbdev_OBJECTS): %.o: %.c
	$(call egl-cc,fbdev)

$(null_OBJECTS): %.o: %.c
	$(call egl-cc,null)

sinclude depend
