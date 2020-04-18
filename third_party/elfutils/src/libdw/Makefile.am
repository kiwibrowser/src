## Process this file with automake to create Makefile.in
##
## Copyright (C) 2002-2010, 2012 Red Hat, Inc.
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
if BUILD_STATIC
AM_CFLAGS += -fpic
endif
AM_CPPFLAGS += -I$(srcdir)/../libelf
VERSION = 1

lib_LIBRARIES = libdw.a
if !MUDFLAP
noinst_LIBRARIES = libdw_pic.a
noinst_PROGRAMS = $(noinst_LIBRARIES:_pic.a=.so)
endif

include_HEADERS = dwarf.h
pkginclude_HEADERS = libdw.h

libdw_a_SOURCES = dwarf_begin.c dwarf_begin_elf.c dwarf_end.c dwarf_getelf.c \
		  dwarf_getpubnames.c dwarf_getabbrev.c dwarf_tag.c \
		  dwarf_error.c dwarf_nextcu.c dwarf_diename.c dwarf_offdie.c \
		  dwarf_attr.c dwarf_formstring.c \
		  dwarf_abbrev_hash.c dwarf_sig8_hash.c \
		  dwarf_attr_integrate.c dwarf_hasattr_integrate.c \
		  dwarf_child.c dwarf_haschildren.c dwarf_formaddr.c \
		  dwarf_formudata.c dwarf_formsdata.c dwarf_lowpc.c \
		  dwarf_entrypc.c dwarf_haspc.c dwarf_highpc.c dwarf_ranges.c \
		  dwarf_formref.c dwarf_formref_die.c dwarf_siblingof.c \
		  dwarf_dieoffset.c dwarf_cuoffset.c dwarf_diecu.c \
		  dwarf_hasattr.c dwarf_hasform.c \
		  dwarf_whatform.c dwarf_whatattr.c \
		  dwarf_bytesize.c dwarf_arrayorder.c dwarf_bitsize.c \
		  dwarf_bitoffset.c dwarf_srclang.c dwarf_getabbrevtag.c \
		  dwarf_getabbrevcode.c dwarf_abbrevhaschildren.c \
		  dwarf_getattrcnt.c dwarf_getabbrevattr.c \
		  dwarf_getsrclines.c dwarf_getsrc_die.c \
		  dwarf_getscopes.c dwarf_getscopes_die.c dwarf_getscopevar.c \
		  dwarf_linesrc.c dwarf_lineno.c dwarf_lineaddr.c \
		  dwarf_linecol.c dwarf_linebeginstatement.c \
		  dwarf_lineendsequence.c dwarf_lineblock.c \
		  dwarf_lineprologueend.c dwarf_lineepiloguebegin.c \
		  dwarf_lineisa.c dwarf_linediscriminator.c \
		  dwarf_lineop_index.c \
		  dwarf_onesrcline.c dwarf_formblock.c \
		  dwarf_getsrcfiles.c dwarf_filesrc.c dwarf_getsrcdirs.c \
		  dwarf_getlocation.c dwarf_getstring.c dwarf_offabbrev.c \
		  dwarf_getaranges.c dwarf_onearange.c dwarf_getarangeinfo.c \
		  dwarf_getarange_addr.c dwarf_getattrs.c dwarf_formflag.c \
		  dwarf_getmacros.c dwarf_macro_opcode.c dwarf_macro_param1.c \
		  dwarf_macro_param2.c dwarf_addrdie.c \
		  dwarf_getfuncs.c  \
		  dwarf_decl_file.c dwarf_decl_line.c dwarf_decl_column.c \
		  dwarf_func_inline.c dwarf_getsrc_file.c \
		  libdw_findcu.c libdw_form.c libdw_alloc.c \
		  libdw_visit_scopes.c \
		  dwarf_entry_breakpoints.c \
		  dwarf_next_cfi.c \
		  cie.c fde.c cfi.c frame-cache.c \
		  dwarf_frame_info.c dwarf_frame_cfa.c dwarf_frame_register.c \
		  dwarf_cfi_addrframe.c \
		  dwarf_getcfi.c dwarf_getcfi_elf.c dwarf_cfi_end.c \
		  dwarf_aggregate_size.c dwarf_getlocation_implicit_pointer.c \
		  dwarf_getlocation_die.c dwarf_getlocation_attr.c

if MAINTAINER_MODE
BUILT_SOURCES = $(srcdir)/known-dwarf.h
MAINTAINERCLEANFILES = $(srcdir)/known-dwarf.h
$(srcdir)/known-dwarf.h: $(top_srcdir)/config/known-dwarf.awk $(srcdir)/dwarf.h
	gawk -f $^ > $@.new
	mv -f $@.new $@
endif

if !MUDFLAP
libdw_pic_a_SOURCES =
am_libdw_pic_a_OBJECTS = $(libdw_a_SOURCES:.c=.os)

libdw_so_SOURCES =
libdw.so: $(srcdir)/libdw.map libdw_pic.a \
	  ../libdwfl/libdwfl_pic.a ../libebl/libebl.a \
	  ../libelf/libelf.so
# The rpath is necessary for libebl because its $ORIGIN use will
# not fly in a setuid executable that links in libdw.
	$(LINK) -shared -o $@ -Wl,--soname,$@.$(VERSION),-z,defs \
		-Wl,--enable-new-dtags,-rpath,$(pkglibdir) \
		-Wl,--version-script,$<,--no-undefined \
		-Wl,--whole-archive $(filter-out $<,$^) -Wl,--no-whole-archive\
		-ldl $(zip_LIBS)
	if $(READELF) -d $@ | fgrep -q TEXTREL; then exit 1; fi
	ln -fs $@ $@.$(VERSION)

install: install-am libdw.so
	$(mkinstalldirs) $(DESTDIR)$(libdir)
	$(INSTALL_PROGRAM) libdw.so $(DESTDIR)$(libdir)/libdw-$(PACKAGE_VERSION).so
	ln -fs libdw-$(PACKAGE_VERSION).so $(DESTDIR)$(libdir)/libdw.so.$(VERSION)
	ln -fs libdw.so.$(VERSION) $(DESTDIR)$(libdir)/libdw.so

uninstall: uninstall-am
	rm -f $(DESTDIR)$(libdir)/libdw-$(PACKAGE_VERSION).so
	rm -f $(DESTDIR)$(libdir)/libdw.so.$(VERSION)
	rm -f $(DESTDIR)$(libdir)/libdw.so
	rmdir --ignore-fail-on-non-empty $(DESTDIR)$(includedir)/elfutils
endif

libdwfl_objects = $(shell $(AR) t ../libdwfl/libdwfl.a)
libdw_a_LIBADD = $(addprefix ../libdwfl/,$(libdwfl_objects))

noinst_HEADERS = libdwP.h memory-access.h dwarf_abbrev_hash.h \
		 dwarf_sig8_hash.h cfi.h encoded-value.h

EXTRA_DIST = libdw.map known-dwarf.h

MOSTLYCLEANFILES = $(am_libdw_pic_a_OBJECTS) libdw.so.$(VERSION)
