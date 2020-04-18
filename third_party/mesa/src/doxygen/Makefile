
default: full

all: full subset

%.tag: %.doxy
	doxygen $<

FULL = \
	main.doxy \
	math.doxy \
	vbo.doxy \
	glapi.doxy \
	glsl.doxy \
	shader.doxy \
	swrast.doxy \
	swrast_setup.doxy \
	tnl.doxy \
	tnl_dd.doxy \
	gbm.doxy

full: $(FULL:.doxy=.tag)
	$(foreach FILE,$(FULL),doxygen $(FILE);)

SUBSET = \
	main.doxy \
	math.doxy

subset: $(SUBSET:.doxy=.tag)
	$(foreach FILE,$(SUBSET),doxygen $(FILE);)

clean:
	-rm -rf $(FULL:.doxy=) $(SUBSET:.doxy=)
	-rm -rf *.tag
