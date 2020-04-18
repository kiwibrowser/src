AM_CPPFLAGS = \
	-I$(top_srcdir)/src/gtest/include \
	-I$(top_srcdir)/src/mesa \
	-I$(top_srcdir)/src/mapi \
	-I$(top_srcdir)/src/glsl \
	-I$(top_srcdir)/include

TESTS_ENVIRONMENT= \
	export PYTHON2=$(PYTHON2); \
	export PYTHON_FLAGS=$(PYTHON_FLAGS);

TESTS = \
	optimization-test \
	ralloc-test \
	uniform-initializer-test

check_PROGRAMS = 				\
	ralloc-test				\
	uniform-initializer-test

uniform_initializer_test_SOURCES =		\
	copy_constant_to_storage_tests.cpp	\
	set_uniform_initializer_tests.cpp	\
	uniform_initializer_utils.cpp
uniform_initializer_test_CFLAGS = 		\
	$(PTHREAD_CFLAGS)
uniform_initializer_test_LDADD =		\
	$(top_builddir)/src/gtest/libgtest.la	\
	$(top_builddir)/src/glsl/libglsl.la	\
	$(top_builddir)/src/mesa/libmesa.la	\
	$(PTHREAD_LIBS)

ralloc_test_SOURCES = ralloc_test.cpp $(top_builddir)/src/glsl/ralloc.c
ralloc_test_CFLAGS = $(PTHREAD_CFLAGS)
ralloc_test_LDADD = $(top_builddir)/src/gtest/libgtest.la $(PTHREAD_LIBS)
