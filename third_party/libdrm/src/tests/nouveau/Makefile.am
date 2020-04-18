AM_CFLAGS = \
	-pthread \
	$(WARN_CFLAGS) \
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/nouveau \
	-I$(top_srcdir)

LDADD = \
	../../nouveau/libdrm_nouveau.la \
	../../libdrm.la \
	-ldl

TESTS = threaded

check_PROGRAMS = $(TESTS)

