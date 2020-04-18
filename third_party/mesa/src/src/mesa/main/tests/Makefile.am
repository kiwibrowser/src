AM_CFLAGS = \
	$(X11_CFLAGS) \
	$(PTHREAD_CFLAGS)
AM_CPPFLAGS = \
	-I$(top_builddir)/src/gtest/include \
	-I$(top_builddir)/src/mapi \
	-I$(top_builddir)/src/mesa \
	-I$(top_builddir)/include \
	$(X11_CFLAGS)

TESTS = main-test
check_PROGRAMS = main-test

main_test_SOURCES =			\
	enum_strings.cpp

main_test_LDADD = \
	$(top_builddir)/src/mesa/libmesa.la \
	$(top_builddir)/src/gtest/libgtest.la \
	$(PTHREAD_LIBS) \
	$(DLOPEN_LIBS)
