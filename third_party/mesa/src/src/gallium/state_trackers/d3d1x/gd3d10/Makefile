LIBNAME=gd3d10
CPP_SOURCES=d3d10.generated.cpp
LIBRARY_INCLUDES=-I../gd3d1x -I../gd3dapi -I../d3dapi -I../w32api -I../d3d1xstutil/include -I../d3d1xshader/include -I../../../include -I../../../auxiliary -I../../../state_trackers/egl/common

GEN_D3D10=perl d3d10.pl

include ../Makefile.inc

d3d10.generated.o: d3d10_objects.generated.h d3d10_screen.generated.h d3d10_context.generated.h d3d10_misc.generated.h
 
d3d10.generated.cpp: ../gd3d11/d3d11.cpp d3d10.pl
	$(GEN_D3D10) $< > $@
d3d10_objects.generated.h: ../gd3d11/d3d11_objects.h d3d10.pl
	$(GEN_D3D10) $< > $@
d3d10_screen.generated.h: ../gd3d11/d3d11_screen.h d3d10.pl
	$(GEN_D3D10) $< > $@
d3d10_context.generated.h: ../gd3d11/d3d11_context.h d3d10.pl
	$(GEN_D3D10) $< > $@
d3d10_misc.generated.h: ../gd3d11/d3d11_misc.h d3d10.pl
	$(GEN_D3D10) $< > $@
