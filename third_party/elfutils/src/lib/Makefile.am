## Process this file with automake to create Makefile.in
##
## Copyright (C) 1996-2011 Red Hat, Inc.
## This file is part of elfutils.
##
## This file is free software; you can redistribute it and/or modify
## it under the terms of either
##
##   * the GNU Lesser General Public License as published by the Free
##     Software Foundation; either version 3 of the License, or (at
##     your option) any later version
##
## or
##
##   * the GNU General Public License as published by the Free
##     Software Foundation; either version 2 of the License, or (at
##     your option) any later version
##
## or both in parallel, as here.
##
## elfutils is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received copies of the GNU General Public License and
## the GNU Lesser General Public License along with this program.  If
## not, see <http://www.gnu.org/licenses/>.
##
include $(top_srcdir)/config/eu.am
AM_CFLAGS += -fpic
AM_CPPFLAGS += -I$(srcdir)/../libelf

noinst_LIBRARIES = libeu.a

libeu_a_SOURCES = xstrdup.c xstrndup.c xmalloc.c next_prime.c \
		  crc32.c crc32_file.c md5.c sha1.c \
		  color.c

noinst_HEADERS = fixedsizehash.h system.h dynamicsizehash.h list.h md5.h \
		 sha1.h eu-config.h
EXTRA_DIST = dynamicsizehash.c

if !GPROF
xmalloc_CFLAGS = -ffunction-sections
endif
