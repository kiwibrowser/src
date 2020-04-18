SUBDIRS=d3dapi gd3dapi docs d3d1xstutil d3d1xshader gd3d1x gd3d11 gd3d10 dxgi dxgid3d11 dxgid3d10

all:
	@for dir in $(SUBDIRS) ; do $(MAKE) -C "$$dir" || exit $?; done

clean:
	rm -f `find . -name \*.[oa]`
	rm -f `find . -name depend`

install:

