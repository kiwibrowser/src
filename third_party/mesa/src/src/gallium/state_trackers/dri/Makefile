# src/gallium/state_trackers/dri/Makefile
TOP = ../../../..
include $(TOP)/configs/current

SUBDIRS = drm sw

default install clean:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) $@) || exit 1; \
		fi \
	done
