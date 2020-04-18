if HAVE_SHARED_GLAPI
AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CPPFLAGS = \
	-I$(top_srcdir)/src/gtest/include \
	-I$(top_srcdir)/src/mapi \
	-I$(top_srcdir)/src/mesa \
	-I$(top_srcdir)/src/glx \
	-I$(top_srcdir)/include \
	$(X11_CFLAGS)

TESTS = glx-test
check_PROGRAMS = glx-test

glx_test_SOURCES =			\
	clientinfo_unittest.cpp		\
	create_context_unittest.cpp	\
	enum_sizes.cpp			\
	fake_glx_screen.cpp		\
	indirect_api.cpp

glx_test_LDADD = \
	$(top_builddir)/src/glx/libglx.la \
	$(top_builddir)/src/gtest/libgtest.la \
	$(top_builddir)/src/mapi/shared-glapi/libglapi.la \
	$(PTHREAD_LIBS)
endif
