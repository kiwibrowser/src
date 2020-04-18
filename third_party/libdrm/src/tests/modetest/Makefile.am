include Makefile.sources

AM_CFLAGS = $(filter-out -Wpointer-arith, $(WARN_CFLAGS))

AM_CFLAGS += \
	-pthread \
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/tests \
	-I$(top_srcdir)

if HAVE_INSTALL_TESTS
bin_PROGRAMS = \
	modetest
else
noinst_PROGRAMS = \
	modetest
endif

modetest_SOURCES = $(MODETEST_FILES)

modetest_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/tests/util/libutil.la \
	$(CAIRO_LIBS)
