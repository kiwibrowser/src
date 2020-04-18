include Makefile.sources

AM_CFLAGS = \
	$(WARN_CFLAGS)\
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/tests \
	-I$(top_srcdir)

if HAVE_INSTALL_TESTS
bin_PROGRAMS = \
	proptest
else
noinst_PROGRAMS = \
	proptest
endif

proptest_SOURCES = $(PROPTEST_FILES)

proptest_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/tests/util/libutil.la
