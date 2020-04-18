AM_CFLAGS = \
	-I $(top_srcdir)/include/drm \
	-I $(top_srcdir)/etnaviv \
	-I $(top_srcdir)

if HAVE_INSTALL_TESTS
bin_PROGRAMS = \
	etnaviv_2d_test \
	etnaviv_cmd_stream_test \
	etnaviv_bo_cache_test
else
noinst_PROGRAMS = \
	etnaviv_2d_test \
	etnaviv_cmd_stream_test \
	etnaviv_bo_cache_test
endif

etnaviv_2d_test_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/etnaviv/libdrm_etnaviv.la

etnaviv_2d_test_SOURCES = \
	cmdstream.xml.h \
	etnaviv_2d_test.c \
	state.xml.h \
	state_2d.xml.h \
	write_bmp.c \
	write_bmp.h

etnaviv_cmd_stream_test_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/etnaviv/libdrm_etnaviv.la

etnaviv_cmd_stream_test_SOURCES = \
	etnaviv_cmd_stream_test.c

etnaviv_bo_cache_test_LDADD = \
	$(top_builddir)/libdrm.la \
	$(top_builddir)/etnaviv/libdrm_etnaviv.la

etnaviv_bo_cache_test_SOURCES = \
	etnaviv_bo_cache_test.c
