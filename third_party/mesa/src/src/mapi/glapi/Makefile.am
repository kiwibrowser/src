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

TOP = $(top_srcdir)
include sources.mak
include ../mapi/sources.mak

AM_CPPFLAGS = \
        $(DEFINES) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/mapi \
        -I$(top_builddir)/src/mapi \
	-I$(top_srcdir)/src/mesa

if HAVE_X86_ASM
GLAPI_ASM_SOURCES = $(X86_API)
endif
if HAVE_X86_64_ASM
GLAPI_ASM_SOURCES = $(X86_64_API)
endif
if HAVE_SPARC_ASM
GLAPI_ASM_SOURCES = $(SPARC_API)
endif

if HAVE_SHARED_GLAPI
AM_CPPFLAGS += \
	-DMAPI_MODE_BRIDGE \
	-DMAPI_ABI_HEADER=\"glapi/glapi_mapi_tmp.h\"
glapi_GLAPI_sources =
glapi_ASM_sources =
glapi_MAPI_sources = $(MAPI_BRIDGE_FILES)
else
AM_CPPFLAGS += \
	-DMAPI_MODE_UTIL
glapi_GLAPI_sources = $(GLAPI_SOURCES)
glapi_ASM_sources = $(GLAPI_ASM_SOURCES)
glapi_MAPI_sources = $(MAPI_UTIL_FILES)
endif

noinst_LTLIBRARIES = libglapi.la

libglapi_la_SOURCES = $(glapi_GLAPI_sources) $(glapi_ASM_sources) $(glapi_MAPI_sources)
libglapi_la_LIBADD =
libglapi_la_LDFLAGS =

# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to libglapi.a here
all-local: libglapi.la
	ln -sf .libs/libglapi.a libglapi.a

CLEANFILES = libglapi.a
