include Makefile.sources

noinst_LIBRARIES = libr300.a
check_PROGRAMS = r300_compiler_tests
testdir = compiler/tests
TESTS = r300_compiler_tests

AM_CFLAGS = \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/auxiliary \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/mesa \
	-I$(top_srcdir)/src/glsl \
	-I$(top_srcdir)/src/mapi \
	$(VISIBILITY_CFLAGS) \
	$(LLVM_CFLAGS) \
	$(RADEON_CFLAGS) \
	$(DEFINES) \
	$(PIC_FLAGS)

libr300_a_SOURCES = \
	$(C_SOURCES) \
	$(top_srcdir)/src/glsl/ralloc.c \
	$(top_srcdir)/src/mesa/program/register_allocate.c

r300_compiler_tests_LDADD = libr300.a
r300_compiler_tests_CPPFLAGS = \
	-I$(top_srcdir)/src/gallium/drivers/r300/compiler
r300_compiler_tests_SOURCES = \
	$(testdir)/r300_compiler_tests.c \
	$(testdir)/radeon_compiler_util_tests.c \
	$(testdir)/rc_test_helpers.c \
	$(testdir)/unit_test.c
