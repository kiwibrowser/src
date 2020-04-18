AM_CFLAGS = \
	-I $(top_srcdir)/include/drm \
	-I $(top_srcdir)/amdgpu \
	-I $(top_srcdir) \
	-pthread

LDADD = $(top_builddir)/libdrm.la \
	$(top_builddir)/amdgpu/libdrm_amdgpu.la \
	$(CUNIT_LIBS)

if HAVE_INSTALL_TESTS
bin_PROGRAMS = \
	amdgpu_test
else
noinst_PROGRAMS = \
	amdgpu_test
endif

amdgpu_test_CPPFLAGS = $(CUNIT_CFLAGS)

amdgpu_test_SOURCES = \
	amdgpu_test.c \
	amdgpu_test.h \
	basic_tests.c \
	bo_tests.c \
	cs_tests.c \
	decode_messages.h \
	vce_tests.c \
	vce_ib.h \
	frame.h \
	uvd_enc_tests.c \
	vcn_tests.c \
	uve_ib.h \
	deadlock_tests.c
