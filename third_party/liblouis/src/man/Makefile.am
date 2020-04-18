# Only build man pages if configure found the HELP2MAN tool
if HAVE_HELP2MAN
man_MANS =					\
	lou_allround.1				\
	lou_checkhyphens.1			\
	lou_checktable.1			\
	lou_debug.1				\
	lou_translate.1				\
	lou_trace.1
endif

CLEANFILES = $(man_MANS)

# The man pages depend on the --help strings and the version number.
common_mandeps = $(top_srcdir)/configure.ac

# We are not distributing the man pages otherwise we would need rules
# such as below. The user will have to install help2man if they want
# man pages.
# lou_allround.1: $(top_srcdir)/tools/lou_allround.c $(common_mandeps)
# 	 cd ../liblouis && $(MAKE) $(AM_MAKEFLAGS) liblouis.la
# 	 cd ../gnulib && $(MAKE) $(AM_MAKEFLAGS) libgnu.la
# 	 cd ../tools && $(MAKE) $(AM_MAKEFLAGS) lou_allround$(EXEEXT)
# 	$(HELP2MAN) ../tools/lou_allround$(EXEEXT) --info-page=$(PACKAGE) --output=$@

# Depend on the source, not the binary; we don't need to regenerate the
# binary when any source file changes, only the main one.
lou_allround.1: $(top_srcdir)/tools/lou_allround.c $(common_mandeps)
	$(HELP2MAN) ../tools/lou_allround$(EXEEXT) --info-page=$(PACKAGE) \
	--name="Test every capability of the liblouis library" \
	--output=$@

lou_checkhyphens.1: $(top_srcdir)/tools/lou_checkhyphens.c $(common_mandeps)
	$(HELP2MAN) ../tools/lou_checkhyphens$(EXEEXT) --info-page=$(PACKAGE) \
	--name="Check the accuracy of hyphenation in liblouis Braille translation tables" \
	--output=$@

lou_checktable.1: $(top_srcdir)/tools/lou_checktable.c $(common_mandeps)
	$(HELP2MAN) ../tools/lou_checktable$(EXEEXT) --info-page=$(PACKAGE) \
	--name="Test a liblouis Braille translation table" \
	--output=$@

lou_debug.1: $(top_srcdir)/tools/lou_debug.c $(common_mandeps)
	$(HELP2MAN) ../tools/lou_debug$(EXEEXT) --info-page=$(PACKAGE) \
	--name="A debugger for liblouis Braille translation tables" \
	--output=$@

lou_translate.1: $(top_srcdir)/tools/lou_translate.c $(common_mandeps)
	$(HELP2MAN) ../tools/lou_translate$(EXEEXT) --info-page=$(PACKAGE) \
	--name="A Braille translator for large scale testing of liblouis Braille translation tables" \
	--output=$@

lou_trace.1: $(top_srcdir)/tools/lou_trace.c $(common_mandeps)
	$(HELP2MAN) ../tools/lou_trace$(EXEEXT) --info-page=$(PACKAGE) \
	--name="A tool to list all the rules that were used for a Braille translation" \
	--output=$@
