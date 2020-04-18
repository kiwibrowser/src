LIBNAME=d3d1xshader
CPP_SOURCES=$(wildcard src/*.cpp) src/sm4_text.cpp
LIBRARY_INCLUDES=-Iinclude -I../d3dapi -I../w32api
PROGS=tools/fxdis
PROGS_DEPS=libd3d1xshader.a
LIBS=$(PROGS_DEPS)

include ../Makefile.inc

include/sm4.h: include/sm4_defs.h

include/sm4_defs.h: $(wildcard defs/*.txt)
	./gen-header.sh $^ > $@

src/sm4_text.cpp: $(wildcard defs/*.txt)
	./gen-text.sh $^ > $@
