# Copyright © 2012 Intel Corporation
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

if NEED_LIBDRICORE
DRICORE_SUBDIR = libdricore
endif

SUBDIRS = x86 x86-64 . $(DRICORE_SUBDIR) drivers

gldir = $(includedir)/GL
gl_HEADERS = $(top_srcdir)/include/GL/*.h

.PHONY: main/git_sha1.h.tmp
main/git_sha1.h.tmp:
	@touch main/git_sha1.h.tmp
	@if test -d ../../.git; then \
		if which git > /dev/null; then \
		    git log -n 1 --oneline | \
			sed 's/^\([^ ]*\) .*/#define MESA_GIT_SHA1 "git-\1"/' \
			> main/git_sha1.h.tmp ; \
		fi \
	fi

main/git_sha1.h: main/git_sha1.h.tmp
	@echo "updating main/git_sha1.h"
	@if ! cmp -s main/git_sha1.h.tmp main/git_sha1.h; then \
		mv main/git_sha1.h.tmp main/git_sha1.h ;\
	else \
		rm main/git_sha1.h.tmp ;\
	fi

# include glapi_gen.mk for generating glapi headers for GLES
GLAPI = $(top_srcdir)/src/mapi/glapi/gen
include $(GLAPI)/glapi_gen.mk

BUILT_SOURCES = \
	main/git_sha1.h \
	main/api_exec_es1_dispatch.h \
	main/api_exec_es1_remap_helper.h \
	main/api_exec_es1.c \
	program/program_parse.tab.c \
	program/program_parse.tab.h \
	program/lex.yy.c
CLEANFILES = \
	$(BUILT_SOURCES) \
	git_sha1.h.tmp

main/api_exec_es1_dispatch.h: $(GLAPI)/gl_and_es_API.xml $(glapi_gen_dispatch_deps)
	$(call glapi_gen_dispatch,$<,es1)

main/api_exec_es1_remap_helper.h: $(GLAPI)/gl_and_es_API.xml $(glapi_gen_remap_deps)
	$(call glapi_gen_remap,$<,es1)

main/api_exec_es1.o: main/api_exec_es1_dispatch.h main/api_exec_es1_remap_helper.h

main/api_exec_es1.c: main/APIspec.xml main/es_generator.py main/APIspecutil.py main/APIspec.py
	$(AM_V_GEN) $(PYTHON2) $(PYTHON_FLAGS) $(srcdir)/main/es_generator.py \
	  -S $(srcdir)/main/APIspec.xml -V GLES1.1 > $@

program/program_parse.tab.c program/program_parse.tab.h: program/program_parse.y
	$(MKDIR_P) program
	$(AM_V_GEN) $(YACC) -p "_mesa_program_" -v -d --output=program/program_parse.tab.c $<

program/lex.yy.c: program/program_lexer.l
	$(MKDIR_P) program
	$(AM_V_GEN) $(LEX) --never-interactive --outfile=$@ $<

noinst_LTLIBRARIES =
if NEED_LIBMESA
noinst_LTLIBRARIES += libmesa.la
else
check_LTLIBRARIES = libmesa.la
endif
if HAVE_GALLIUM
noinst_LTLIBRARIES += libmesagallium.la
endif

SRCDIR = $(top_srcdir)/src/mesa/
BUILDDIR = $(top_builddir)/src/mesa/
include sources.mak

AM_CPPFLAGS = $(API_DEFINES) $(DEFINES) $(INCLUDE_DIRS)
AM_CFLAGS = $(LLVM_CFLAGS) $(VISIBILITY_CFLAGS)
AM_CXXFLAGS = $(LLVM_CFLAGS) $(VISIBILITY_CXXFLAGS)

# cannot just add $(MESA_ASM_FILES) to libmesa_la_SOURCES as it contains a configure substitution
MESA_ASM_FILES_FOR_ARCH =

if HAVE_X86_ASM
MESA_ASM_FILES_FOR_ARCH += $(X86_FILES)
AM_CPPFLAGS += \
        -I$(top_builddir)/src/mesa/x86 \
        -I$(top_srcdir)/src/mesa/x86
endif
if HAVE_X86_64_ASM
MESA_ASM_FILES_FOR_ARCH += $(X86_64_FILES)
AM_CPPFLAGS += \
        -I$(top_builddir)/src/mesa/x86-64 \
        -I$(top_srcdir)/src/mesa/x86-64
endif
if HAVE_SPARC_ASM
MESA_ASM_FILES_FOR_ARCH += $(SPARC_FILES)
AM_CPPFLAGS += \
        -I$(top_builddir)/src/mesa/sparc \
        -I$(top_srcdir)/src/mesa/sparc
endif

libmesa_la_SOURCES = \
	$(MESA_FILES) \
	$(MESA_CXX_FILES) \
        $(MESA_ASM_FILES_FOR_ARCH)

libmesa_la_LIBADD = $(top_builddir)/src/glsl/libglsl.la
libmesa_la_LDFLAGS =

libmesagallium_la_SOURCES = \
	$(MESA_GALLIUM_FILES) \
	$(MESA_GALLIUM_CXX_FILES) \
        $(MESA_ASM_FILES_FOR_ARCH)

libmesagallium_la_LIBADD = $(top_builddir)/src/glsl/libglsl.la
libmesagallium_la_LDFLAGS =

if HAVE_GALLIUM
# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the library in the current directory.
all-local: $(noinst_LTLIBRARIES)
	ln -f .libs/libmesagallium.a .
endif

CLEANFILES += libmesagallium.a

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gl.pc

# Emacs tags
tags:
	etags `find . -name \*.[ch]` $(TOP)/include/GL/*.h
