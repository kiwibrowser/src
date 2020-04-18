LIBNAME=dxgi
LIBRARY_INCLUDES=-I$(D3D1X)/gd3dapi -I$(D3D1X)/d3dapi -I$(D3D1X)/w32api
OBJECTS=dxgi_dll.o ../../dxgi/libdxgi.a ../../d3d1xstutil/libd3d1xstutil.a ../../../egl/libegl.a ../../../../auxiliary/libgallium.a ../../../../winsys/sw/xlib/libws_xlib.a
LDADD=-lgdi32 -lEGL -lXfixes -lX11 -ldrm -ldl -lXext

include ../Makefile.wine
