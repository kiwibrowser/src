LIBNAME=dxgi
LIBRARY_INCLUDES=-I../../../../../include -Iinclude -I../gd3dapi -I../d3dapi -I../w32api -I../d3d1xstutil/include -I../include -I../../../include -I../../../auxiliary -I../../../state_trackers/egl/common
LIBRARY_DEFINES=-DDXGI_DRIVER_SEARCH_DIR=\"$(EGL_DRIVER_INSTALL_DIR)\"
CPP_SOURCES=$(wildcard src/*.cpp)

include ../Makefile.inc

ifneq ($(findstring x11, $(EGL_PLATFORMS)),)
LIBRARY_DEFINES += -DGALLIUM_DXGI_USE_X11
endif
ifneq ($(findstring drm, $(EGL_PLATFORMS)),)
LIBRARY_DEFINES += -DGALLIUM_DXGI_USE_DRM
endif
ifneq ($(findstring fbdev, $(EGL_PLATFORMS)),)
LIBRARY_DEFINES += -DGALLIUM_DXGI_USE_FBDEV
endif

