LIBNAME=gd3d1x
CPP_SOURCES=$(wildcard *.cpp)
LIBRARY_INCLUDES=-Iinclude -I../gd3dapi -I../d3dapi -I../w32api -I../d3d1xstutil/include -I../d3d1xshader/include -I../../../include -I../../../auxiliary -I../../../state_trackers/egl/common
PROGS=tools/dxbc2tgsi
PROGS_DEPS=libgd3d1x.a ../d3d1xshader/libd3d1xshader.a ../d3d1xstutil/libd3d1xstutil.a ../../../auxiliary/libgallium.a
LIBS=$(PROGS_DEPS) -ldl
include ../Makefile.inc
