# src/gallium/winsys/Makefile
TOP = ../../..
include $(TOP)/configs/current

SUBDIRS = $(GALLIUM_TARGET_DIRS)

default install clean:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) $@) || exit 1; \
		fi \
	done
