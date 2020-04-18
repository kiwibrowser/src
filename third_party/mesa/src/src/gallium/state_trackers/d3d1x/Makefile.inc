TOP=../../../../..
include $(TOP)/configs/current

IDL=$(wildcard *.idl include/*.idl)
IDL_H=$(IDL:.idl=.h)
LD=$(CXX) $(CXXFLAGS)

include ../../../Makefile.template

idl: $(IDL_H)

%.h: %.idl
	widl -I../gd3dapi -I../d3dapi -I../w32api -U /dev/null -H $@ $^ 

%.svg: %.dot
	dot -Tsvg -o $@ $<

%.pdf: %.dot
	dot -Tpdf -o $@ $<
