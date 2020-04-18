## Process this file with automake to create Makefile.in
##
## Copyright (C) 2000-2010, 2013 Red Hat, Inc.
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
AM_CPPFLAGS += -I$(srcdir)/../libelf -I$(srcdir)/../libdw -I$(srcdir)/../libasm
VERSION = 1
LIBEBL_SUBDIR = @LIBEBL_SUBDIR@

lib_LIBRARIES = libebl.a

pkginclude_HEADERS = libebl.h

gen_SOURCES = eblopenbackend.c eblclosebackend.c eblstrtab.c \
	      eblreloctypename.c eblsegmenttypename.c \
	      eblsectiontypename.c eblmachineflagname.c \
	      eblsymboltypename.c ebldynamictagname.c eblsectionname.c \
	      eblobjecttypename.c eblsymbolbindingname.c \
	      eblbackendname.c eblshflagscombine.c eblwstrtab.c \
	      eblgstrtab.c eblosabiname.c \
	      eblmachineflagcheck.c eblmachinesectionflagcheck.c \
	      eblreloctypecheck.c eblrelocvaliduse.c eblrelocsimpletype.c \
	      ebldynamictagcheck.c eblcorenotetypename.c eblobjnotetypename.c \
	      eblcorenote.c eblobjnote.c ebldebugscnp.c \
	      eblgotpcreloccheck.c eblcopyrelocp.c eblsectionstripp.c \
	      eblelfclass.c eblelfdata.c eblelfmachine.c \
	      ebl_check_special_symbol.c eblbsspltp.c eblretval.c \
	      eblreginfo.c eblnonerelocp.c eblrelativerelocp.c \
	      eblsysvhashentrysize.c eblauxvinfo.c eblcheckobjattr.c \
	      ebl_check_special_section.c ebl_syscall_abi.c eblabicfi.c \
	      eblstother.c eblinitreg.c ebldwarftoregno.c eblnormalizepc.c \
	      eblunwind.c eblresolvesym.c

libebl_a_SOURCES = $(gen_SOURCES)

noinst_HEADERS = libeblP.h ebl-hooks.h

CLEANFILES += $(am_libebl_pic_a_OBJECTS)
