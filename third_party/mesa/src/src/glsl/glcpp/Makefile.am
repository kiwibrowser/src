
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

include ../Makefile.sources

AM_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/mapi \
	-I$(top_srcdir)/src/mesa/ \
	$(VISIBILITY_CFLAGS) \
	$(LIBRARY_INCLUDES) \
	$(DEFINES) \
	$(LIBRARY_DEFINES) \
	$(API_DEFINES)

AM_YFLAGS = -v -d -p "glcpp_parser_"
AM_LFLAGS = --nounistd -o$(LEX_OUTPUT_ROOT).c

noinst_LTLIBRARIES = libglcpp.la
noinst_PROGRAMS = glcpp

libglcpp_la_SOURCES = \
	glcpp-lex.l \
	glcpp-parse.y \
	$(LIBGLCPP_FILES)

glcpp_SOURCES = \
	../ralloc.c \
	$(top_srcdir)/src/mesa/program/hash_table.c \
	glcpp.c

glcpp_LDADD = libglcpp.la

BUILT_SOURCES = glcpp-lex.c glcpp-parse.c glcpp-parse.h
CLEANFILES = $(BUILT_SOURCES)
