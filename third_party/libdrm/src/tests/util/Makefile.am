include Makefile.sources

noinst_LTLIBRARIES = \
	libutil.la

libutil_la_CPPFLAGS = \
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)

libutil_la_CFLAGS = \
	$(CAIRO_CFLAGS)

libutil_la_SOURCES = $(UTIL_FILES)
