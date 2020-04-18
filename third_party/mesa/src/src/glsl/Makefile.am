# Copyright © 2012 Jon TURNEY
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.


# builtin_compiler is built before libglsl to generate builtin_function.cpp for libglsl.
# For this to work, a dummy version of builtin_function.cpp, builtin_stubs.cpp, is used.

AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/mapi \
	-I$(top_srcdir)/src/mesa/ \
	$(LIBRARY_INCLUDES) \
	$(DEFINES) \
	$(LIBRARY_DEFINES) \
	$(API_DEFINES)
AM_CFLAGS = $(VISIBILITY_CFLAGS)
AM_CXXFLAGS = $(VISIBILITY_CXXFLAGS)

AM_LFLAGS = --nounistd -o$(LEX_OUTPUT_ROOT).c

include Makefile.sources

noinst_LTLIBRARIES = libglslcommon.la libglsl.la
noinst_PROGRAMS = glsl_compiler glsl_test builtin_compiler

# common sources for builtin_compiler and libglsl
libglslcommon_la_SOURCES = \
	glsl_lexer.ll \
	glsl_parser.cc \
	$(LIBGLSL_FILES) \
	$(LIBGLSL_CXX_FILES)

libglslcommon_la_LIBADD = glcpp/libglcpp.la

# common sources for builtin_compiler and glsl_compiler
GLSL2_SOURCES = \
	$(top_srcdir)/src/mesa/program/hash_table.c \
	$(top_srcdir)/src/mesa/program/symbol_table.c \
	$(GLSL_COMPILER_CXX_FILES)

libglsl_la_SOURCES = \
	builtin_function.cpp

libglsl_la_LIBADD = libglslcommon.la
libglsl_la_LDFLAGS =

glsl_compiler_SOURCES = \
	$(GLSL2_SOURCES)

glsl_compiler_LDADD = libglsl.la

glsl_test_SOURCES = \
	$(top_srcdir)/src/mesa/program/hash_table.c \
	$(top_srcdir)/src/mesa/program/symbol_table.c \
	$(GLSL_SRCDIR)/standalone_scaffolding.cpp \
	test.cpp \
	test_optpass.cpp

glsl_test_LDADD = libglsl.la

builtin_compiler_SOURCES = \
	$(GLSL2_SOURCES) \
	builtin_stubs.cpp

builtin_compiler_LDADD = libglslcommon.la

# automake <=1.11 and automake >=1.12 have different conventions for naming C++ header files
# made by yacc.  To work with both, we write our own rule rather than using automake's.
# When (if) we require automake >=1.12 in configure.ac, this can be removed, and we can use
# the automake rule.
glsl_parser.cc glsl_parser.h: glsl_parser.yy
	$(AM_V_GEN) $(YACC) -v -o glsl_parser.cc -p "_mesa_glsl_" --defines=glsl_parser.h $<

BUILT_SOURCES = glsl_parser.h builtin_function.cpp
CLEANFILES = glsl_lexer.cc glsl_parser.cc $(BUILT_SOURCES)

builtin_function.cpp: $(srcdir)/builtins/profiles/* $(srcdir)/builtins/ir/* $(srcdir)/builtins/glsl/* $(srcdir)/builtins/tools/generate_builtins.py $(srcdir)/builtins/tools/texture_builtins.py builtin_compiler$(EXEEXT)
	$(AM_V_GEN) $(PYTHON2) $(PYTHON_FLAGS) $(srcdir)/builtins/tools/generate_builtins.py ./builtin_compiler > builtin_function.cpp || rm -f builtin_function.cpp

glcpp/libglcpp.la:
	cd glcpp ; $(MAKE) $(AM_MAKEFLAGS)

SUBDIRS = glcpp

# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the library in the current directory.
all-local: libglsl.la
	ln -f .libs/libglsl.a .

CLEANFILES += libglsl.a
