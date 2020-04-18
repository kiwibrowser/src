SUBDIRS=dxgi d3d10 d3d10_1 d3d11

all:
	@for dir in $(SUBDIRS) ; do $(MAKE) -C "$$dir" || exit $?; done

clean:
	rm -f `find . -name \*.[oa]`
	rm -f `find . -name depend`

install:
	sudo install */*.dll.so /usr/lib/wine
