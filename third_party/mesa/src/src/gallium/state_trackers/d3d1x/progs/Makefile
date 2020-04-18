LIBRARY_INCLUDES = -Id3d10app -Id3d11app -I../gd3dapi  -I../d3dapi -I../w32api
LIBS= \
	../dxgi/libdxgi.a \
	../gd3d1x/libgd3d1x.a \
	../d3d1xshader/libd3d1xshader.a \
	../d3d1xstutil/libd3d1xstutil.a \
	../../egl/libegl.a \
	../../../auxiliary/libgallium.a \
	../../../winsys/sw/wrapper/libwsw.a \
	../../../winsys/sw/xlib/libws_xlib.a \
	../../../winsys/sw/dri/libswdri.a \
	../../../../../lib/libEGL.so

LDADD=-ldl

all: bin/d3d10tri bin/d3d11tri bin/d3d11tex bin/d3d11gears
include ../Makefile.inc

ifneq ($(findstring fbdev, $(EGL_PLATFORMS)),)
LIBS += ../../../winsys/sw/fbdev/libfbdev.a
endif

ifneq ($(findstring x11, $(EGL_PLATFORMS)),)
LDADD += -ldrm -lX11 -lXext -lXfixes
LIBS += ../../../winsys/sw/xlib/libws_xlib.a
endif

ifneq ($(findstring kms, $(EGL_PLATFORMS)),)
LDADD += -ldrm
endif

LIBS_D3D10 = ../dxgid3d10/libdxgid3d10.a ../gd3d10/libgd3d10.a $(LIBS)
LIBS_D3D11 = ../dxgid3d11/libdxgid3d11.a ../gd3d11/libgd3d11.a $(LIBS)

bin/d3d10tri: d3d10app/d3d10x11main.o d3d10tri/d3d10tri.o $(LIBS_D3D10)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(filter %.o,$^) -o $@ -Wl,--start-group  $(LIBS_D3D10) -Wl,--end-group $(LDADD)

bin/d3d11tri: d3d11app/d3d11x11main.o d3d11tri/d3d11tri.o $(LIBS_D3D11)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(filter %.o,$^) -o $@ -Wl,--start-group  $(LIBS_D3D11) -Wl,--end-group $(LDADD)

bin/d3d11tex: d3d11app/d3d11x11main.o d3d11tex/d3d11tex.o $(LIBS_D3D11)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(filter %.o,$^) -o $@ -Wl,--start-group  $(LIBS_D3D11) -Wl,--end-group $(LDADD)

bin/d3d11gears: d3d11app/d3d11x11main.o d3d11gears/d3d11gears.o $(LIBS_D3D11)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(filter %.o,$^) -o $@ -Wl,--start-group  $(LIBS_D3D11) -Wl,--end-group $(LDADD)

