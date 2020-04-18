## Process this file with automake to create Makefile.in
##
## Copyright (C) 1996-2013 Red Hat, Inc.
## This file is part of elfutils.
##
## This file is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## elfutils is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
##
include $(top_srcdir)/config/eu.am
DEFS += $(YYDEBUG) -DDEBUGPRED=@DEBUGPRED@ \
	-DSRCDIR=\"$(shell cd $(srcdir);pwd)\" -DOBJDIR=\"$(shell pwd)\"
AM_CPPFLAGS += -I$(srcdir)/../libelf -I$(srcdir)/../libebl \
	    -I$(srcdir)/../libdw -I$(srcdir)/../libdwfl \
	    -I$(srcdir)/../libasm

AM_LDFLAGS = -Wl,-rpath-link,../libelf:../libdw

no_mudflap.os = -fmudflap

YACC = @YACC@ -d
AM_YFLAGS = -pld
AM_LFLAGS = -Pld -olex.yy.c
## Uncomment to enable debugging of linker script parser
##YYDEBUG = -DYYDEBUG=1

native_ld = @native_ld@
base_cpu = @base_cpu@

bin_PROGRAMS = readelf nm size strip ld elflint findtextrel addr2line \
	       elfcmp objdump ranlib strings ar unstrip stack


ld_dsos = libld_elf_i386_pic.a
if NATIVE_LD
noinst_LIBRARIES = libld_elf.a libar.a
native_ld_cflags = -DBASE_ELF_NAME=elf_$(base_cpu)
else
noinst_LIBRARIES = libld_elf.a libar.a $(ld_dsos)
noinst_PROGRAMS = $(ld_dsos:_pic.a=.so)
endif
if NEVER
# We never build this library but we need to get the dependency files
# of all the linker backends that might be used in a non-generic linker.
noinst_LIBRARIES += libdummy.a
libdummy_a_SOURCES = i386_ld.c
endif


ld_SOURCES = ld.c ldgeneric.c ldlex.l ldscript.y symbolhash.c sectionhash.c \
	     versionhash.c

libar_a_SOURCES = arlib.c arlib2.c arlib-argp.c

noinst_HEADERS = ld.h symbolhash.h sectionhash.h versionhash.h \
		 ldscript.h xelf.h unaligned.h

EXTRA_DIST = elf32-i386.script libld_elf_i386.map $(ld_modules) arlib.h \
	     debugpred.h
ld_modules = i386_ld.c

bin_SCRIPTS = make-debug-archive
EXTRA_DIST += make-debug-archive.in
CLEANFILES += make-debug-archive

if BUILD_STATIC
libasm = ../libasm/libasm.a
libdw = ../libdw/libdw.a $(zip_LIBS) $(libelf) $(libebl) -ldl
libelf = ../libelf/libelf.a
else
libasm = ../libasm/libasm.so
libdw = ../libdw/libdw.so
libelf = ../libelf/libelf.so
endif
libebl = ../libebl/libebl.a
libeu = ../lib/libeu.a

if DEMANGLE
demanglelib = -lstdc++
endif

nm_no_Wformat = yes
size_no_Wformat = yes
strings_no_Wformat = yes
addr2line_no_Wformat = yes
# XXX While the file is not finished, don't warn about this
ldgeneric_no_Wunused = yes

readelf_LDADD = $(libdw) $(libebl) $(libelf) $(libeu) $(libmudflap) -ldl
nm_LDADD = $(libdw) $(libebl) $(libelf) $(libeu) $(libmudflap) -ldl \
	   $(demanglelib)
size_LDADD = $(libelf) $(libeu) $(libmudflap)
strip_LDADD = $(libebl) $(libelf) $(libeu) $(libmudflap) -ldl
ld_LDADD = $(libebl) $(libelf) $(libeu) $(libmudflap) -ldl
if NATIVE_LD
# -ldl is always needed for libebl.
ld_LDADD += libld_elf.a
endif
ld_LDFLAGS = -rdynamic
elflint_LDADD  = $(libebl) $(libelf) $(libeu) $(libmudflap) -ldl
findtextrel_LDADD = $(libdw) $(libelf) $(libmudflap)
addr2line_LDADD = $(libdw) $(libelf) $(libmudflap)
elfcmp_LDADD = $(libebl) $(libelf) $(libmudflap) -ldl
objdump_LDADD  = $(libasm) $(libebl) $(libelf) $(libeu) $(libmudflap) -ldl
ranlib_LDADD = libar.a $(libelf) $(libeu) $(libmudflap)
strings_LDADD = $(libelf) $(libeu) $(libmudflap)
ar_LDADD = libar.a $(libelf) $(libeu) $(libmudflap)
unstrip_LDADD = $(libebl) $(libelf) $(libdw) $(libeu) $(libmudflap) -ldl
stack_LDADD = $(libebl) $(libelf) $(libdw) $(libeu) $(libmudflap) -ldl \
	      $(demanglelib)

ldlex.o: ldscript.c
ldlex_no_Werror = yes
ldscript.h: ldscript.c

if NATIVE_LD
# Machine-specific linker code.
libld_elf_a_SOURCES := $(base_cpu)_ld.c
else
libld_elf_i386_pic_a_SOURCES =
am_libld_elf_i386_pic_a_OBJECTS = i386_ld.os

libld_elf_i386_so_SOURCES =
libld_elf_i386.so: libld_elf_i386_pic.a libld_elf_i386.map
	$(LINK) -shared -o $@ -Wl,--whole-archive,$<,--no-whole-archive \
		$(libelf) $(libeu) \
		-Wl,--version-script,$(srcdir)/libld_elf_i386.map
	$(textrel_check)
endif

# Special rule to make it possible to define libld_elf_a_SOURCES as we do.
# Otherwise make would complain.
.deps/none_ld.Po: none_ld.os
	-:


installcheck-binPROGRAMS: $(bin_PROGRAMS)
	bad=0; pid=$$$$; list="$(bin_PROGRAMS)"; for p in $$list; do \
	  case ' $(AM_INSTALLCHECK_STD_OPTIONS_EXEMPT) ' in \
	   *" $$p "* | *" $(srcdir)/$$p "*) continue;; \
	  esac; \
	  f=`echo "$$p" | \
	     sed 's,^.*/,,;s/$(EXEEXT)$$//;$(transform);s/$$/$(EXEEXT)/'`; \
	  for opt in --help --version; do \
	    if LD_LIBRARY_PATH=$(DESTDIR)$(libdir) \
	       $(DESTDIR)$(bindir)/$$f $$opt > c$${pid}_.out 2> c$${pid}_.err \
		 && test -n "`cat c$${pid}_.out`" \
		 && test -z "`cat c$${pid}_.err`"; then :; \
	    else echo "$$f does not support $$opt" 1>&2; bad=1; fi; \
	  done; \
	done; rm -f c$${pid}_.???; exit $$bad

CLEANFILES += none_ld.os $(ld_modules:.c=.os) *.gconv

MAINTAINERCLEANFILES = ldlex.c ldscript.c ldscript.h


make-debug-archive: $(srcdir)/make-debug-archive.in
	UNSTRIP=$(bindir)/`echo unstrip | sed '$(transform)'`; \
	AR=$(bindir)/`echo ar | sed '$(transform)'`; \
	sed -e "s,@UNSTRIP@,$$UNSTRIP,g" -e "s,@AR@,$$AR,g" \
	    -e "s%[@]PACKAGE_NAME[@]%$(PACKAGE_NAME)%g" \
	    -e "s%[@]PACKAGE_VERSION[@]%$(PACKAGE_VERSION)%g" \
	    $(srcdir)/make-debug-archive.in > $@.new
	chmod +x $@.new
	mv -f $@.new $@
