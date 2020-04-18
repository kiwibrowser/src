AM_CPPFLAGS = \
	-I$(top_srcdir)/liblouis \
	-I$(top_srcdir)/gnulib

LDADD = \
	$(top_builddir)/liblouis/liblouis.la \
	$(top_builddir)/gnulib/libgnu.la \
	$(LTLIBINTL)

bin_PROGRAMS=			\
	lou_allround			\
	lou_checkhyphens \
	lou_checktable			\
	lou_debug \
	lou_translate \
	lou_trace

lou_allround_SOURCES=		\
	lou_allround.c
lou_checkhyphens_SOURCES= \
	lou_checkhyphens.c
lou_checktable_SOURCES = \
	lou_checktable.c
lou_debug_SOURCES = \
	lou_debug.c
lou_translate_SOURCES = \
	lou_translate.c
lou_trace_SOURCES = \
	lou_trace.c

# distribute the harness generator but do not install it
dist_bin_SCRIPTS = lou_harnessGenerator
