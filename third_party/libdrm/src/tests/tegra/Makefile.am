AM_CPPFLAGS = \
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/tegra \
	-I$(top_srcdir)

AM_CFLAGS = $(WARN_CFLAGS)

LDADD = \
	../../tegra/libdrm_tegra.la \
	../../libdrm.la

noinst_PROGRAMS = \
	openclose
