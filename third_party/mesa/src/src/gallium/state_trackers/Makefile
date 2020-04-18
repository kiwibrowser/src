TOP = ../../..
include $(TOP)/configs/current


SUBDIRS = $(GALLIUM_STATE_TRACKERS_DIRS)


default: subdirs


subdirs:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE)) || exit 1 ; \
		fi \
	done


clean:
	rm -f `find . -regex '.*\.l?[oa]'`
	rm -f `find . -name depend`


install:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) $@) || exit 1 ; \
		fi \
	done
