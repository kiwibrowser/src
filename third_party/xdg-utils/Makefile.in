SUBDIRS = scripts tests

all:		$(SUBDIRS)
install:	$(SUBDIRS:%=%/__install__)
uninstall:	$(SUBDIRS:%=%/__uninstall__)
test:		dummy
	cd tests && $(MAKE) test
clean:		$(SUBDIRS:%=%/__clean__)
distclean:	clean $(SUBDIRS:%=%/__distclean__)
	rm -f config.* Makefile
	rm -rf autom4te.cache

release:	$(SUBDIRS:%=%/__release__) distclean
	rm -f *~

help:	
	@echo "Usage: make [install|uninstall|release]"

.PHONY: all install uninstall clean distclean dummy
dummy:

$(SUBDIRS): dummy
	@cd $@ && $(MAKE)

$(SUBDIRS:%=%/__uninstall__): dummy
	cd `dirname $@` && $(MAKE) uninstall

$(SUBDIRS:%=%/__install__): dummy
	cd `dirname $@` && $(MAKE) install

$(SUBDIRS:%=%/__clean__): dummy
	cd `dirname $@` && $(MAKE) clean

$(SUBDIRS:%=%/__release__): dummy
	cd `dirname $@` && $(MAKE) release

$(SUBDIRS:%=%/__distclean__): dummy
	cd `dirname $@` && $(MAKE) distclean

