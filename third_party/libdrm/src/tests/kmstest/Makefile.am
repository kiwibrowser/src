AM_CFLAGS = \
	$(WARN_CFLAGS)\
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/libkms/ \
	-I$(top_srcdir)/tests/ \
	-I$(top_srcdir)

if HAVE_INSTALL_TESTS
bin_PROGRAMS = \
	kmstest
else
noinst_PROGRAMS = \
	kmstest
endif

kmstest_SOURCES = \
	main.c

kmstest_LDADD = \
	$(top_builddir)/tests/util/libutil.la \
	$(top_builddir)/libkms/libkms.la \
	$(top_builddir)/libdrm.la

run: kmstest
	./kmstest
