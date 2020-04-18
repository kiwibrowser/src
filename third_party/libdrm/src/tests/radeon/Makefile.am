AM_CFLAGS = \
	$(WARN_CFLAGS)\
	-I $(top_srcdir)/include/drm \
	-I $(top_srcdir)

LDADD = $(top_builddir)/libdrm.la

noinst_PROGRAMS = \
	radeon_ttm

radeon_ttm_SOURCES = \
	rbo.c \
	rbo.h \
	radeon_ttm.c
