## Process this file with automake to create Makefile.in
##
## Copyright (C) 2000-2010, 2013 Red Hat, Inc.
## Copyright (C) 2012 Tilera Corporation
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
include $(top_srcdir)/config/eu.am
AM_CPPFLAGS += -I$(top_srcdir)/libebl -I$(top_srcdir)/libasm \
	   -I$(top_srcdir)/libelf -I$(top_srcdir)/libdw


modules = i386 sh x86_64 ia64 alpha arm aarch64 sparc ppc ppc64 s390 \
	  tilegx
libebl_pic = libebl_i386_pic.a libebl_sh_pic.a libebl_x86_64_pic.a    \
	     libebl_ia64_pic.a libebl_alpha_pic.a libebl_arm_pic.a    \
	     libebl_aarch64_pic.a libebl_sparc_pic.a libebl_ppc_pic.a \
	     libebl_ppc64_pic.a libebl_s390_pic.a libebl_tilegx_pic.a
noinst_LIBRARIES = $(libebl_pic)
noinst_DATA = $(libebl_pic:_pic.a=.so)


if MUDFLAP
libelf = ../libelf/libelf.a
libdw = ../libdw/libdw.a
else
libelf = ../libelf/libelf.so
libdw = ../libdw/libdw.so
endif

i386_SRCS = i386_init.c i386_symbol.c i386_corenote.c i386_cfi.c \
	    i386_retval.c i386_regs.c i386_auxv.c i386_syscall.c \
	    i386_initreg.c
cpu_i386 = ../libcpu/libcpu_i386.a
libebl_i386_pic_a_SOURCES = $(i386_SRCS)
am_libebl_i386_pic_a_OBJECTS = $(i386_SRCS:.c=.os)

sh_SRCS = sh_init.c sh_symbol.c sh_corenote.c sh_regs.c sh_retval.c
libebl_sh_pic_a_SOURCES = $(sh_SRCS)
am_libebl_sh_pic_a_OBJECTS = $(sh_SRCS:.c=.os)

x86_64_SRCS = x86_64_init.c x86_64_symbol.c x86_64_corenote.c x86_64_cfi.c \
	      x86_64_retval.c x86_64_regs.c i386_auxv.c x86_64_syscall.c \
	      x86_64_initreg.c
cpu_x86_64 = ../libcpu/libcpu_x86_64.a
libebl_x86_64_pic_a_SOURCES = $(x86_64_SRCS)
am_libebl_x86_64_pic_a_OBJECTS = $(x86_64_SRCS:.c=.os)

ia64_SRCS = ia64_init.c ia64_symbol.c ia64_regs.c ia64_retval.c
libebl_ia64_pic_a_SOURCES = $(ia64_SRCS)
am_libebl_ia64_pic_a_OBJECTS = $(ia64_SRCS:.c=.os)

alpha_SRCS = alpha_init.c alpha_symbol.c alpha_retval.c alpha_regs.c \
	     alpha_corenote.c alpha_auxv.c
libebl_alpha_pic_a_SOURCES = $(alpha_SRCS)
am_libebl_alpha_pic_a_OBJECTS = $(alpha_SRCS:.c=.os)

arm_SRCS = arm_init.c arm_symbol.c arm_regs.c arm_corenote.c \
	   arm_auxv.c arm_attrs.c arm_retval.c arm_cfi.c
libebl_arm_pic_a_SOURCES = $(arm_SRCS)
am_libebl_arm_pic_a_OBJECTS = $(arm_SRCS:.c=.os)

aarch64_SRCS = aarch64_init.c aarch64_regs.c aarch64_symbol.c	\
	       aarch64_corenote.c aarch64_retval.c aarch64_cfi.c
libebl_aarch64_pic_a_SOURCES = $(aarch64_SRCS)
am_libebl_aarch64_pic_a_OBJECTS = $(aarch64_SRCS:.c=.os)
aarch64_regs_no_Wformat = yes

sparc_SRCS = sparc_init.c sparc_symbol.c sparc_regs.c sparc_retval.c \
	     sparc_corenote.c sparc64_corenote.c sparc_auxv.c
libebl_sparc_pic_a_SOURCES = $(sparc_SRCS)
am_libebl_sparc_pic_a_OBJECTS = $(sparc_SRCS:.c=.os)

ppc_SRCS = ppc_init.c ppc_symbol.c ppc_retval.c ppc_regs.c \
	   ppc_corenote.c ppc_auxv.c ppc_attrs.c ppc_syscall.c \
	   ppc_cfi.c ppc_initreg.c
libebl_ppc_pic_a_SOURCES = $(ppc_SRCS)
am_libebl_ppc_pic_a_OBJECTS = $(ppc_SRCS:.c=.os)

ppc64_SRCS = ppc64_init.c ppc64_symbol.c ppc64_retval.c \
	     ppc64_corenote.c ppc_regs.c ppc_auxv.c ppc_attrs.c ppc_syscall.c \
	     ppc_cfi.c ppc_initreg.c ppc64_resolve_sym.c
libebl_ppc64_pic_a_SOURCES = $(ppc64_SRCS)
am_libebl_ppc64_pic_a_OBJECTS = $(ppc64_SRCS:.c=.os)

s390_SRCS = s390_init.c s390_symbol.c s390_regs.c s390_retval.c \
	    s390_corenote.c s390x_corenote.c s390_cfi.c s390_initreg.c \
	    s390_unwind.c
libebl_s390_pic_a_SOURCES = $(s390_SRCS)
am_libebl_s390_pic_a_OBJECTS = $(s390_SRCS:.c=.os)

tilegx_SRCS = tilegx_init.c tilegx_symbol.c tilegx_regs.c \
              tilegx_retval.c tilegx_corenote.c
libebl_tilegx_pic_a_SOURCES = $(tilegx_SRCS)
am_libebl_tilegx_pic_a_OBJECTS = $(tilegx_SRCS:.c=.os)


libebl_%.so libebl_%.map: libebl_%_pic.a $(libelf) $(libdw)
	@rm -f $(@:.so=.map)
	echo 'ELFUTILS_$(PACKAGE_VERSION) { global: $*_init; local: *; };' \
	  > $(@:.so=.map)
	$(LINK) -shared -o $(@:.map=.so) \
		-Wl,--whole-archive $< $(cpu_$*) -Wl,--no-whole-archive \
		-Wl,--version-script,$(@:.so=.map) \
		-Wl,-z,defs -Wl,--as-needed $(libelf) $(libdw) $(libmudflap)
	$(textrel_check)

libebl_i386.so: $(cpu_i386)
libebl_x86_64.so: $(cpu_x86_64)

install: install-am install-ebl-modules
install-ebl-modules:
	$(mkinstalldirs) $(DESTDIR)$(libdir)/$(LIBEBL_SUBDIR)
	for m in $(modules); do \
	  $(INSTALL_PROGRAM) libebl_$${m}.so $(DESTDIR)$(libdir)/$(LIBEBL_SUBDIR)/libebl_$${m}-$(PACKAGE_VERSION).so; \
	  ln -fs libebl_$${m}-$(PACKAGE_VERSION).so $(DESTDIR)$(libdir)/$(LIBEBL_SUBDIR)/libebl_$${m}.so; \
	done

uninstall: uninstall-am
	for m in $(modules); do \
	  rm -f $(DESTDIR)$(libdir)/$(LIBEBL_SUBDIR)/libebl_$${m}-$(PACKAGE_VERSION).so; \
	  rm -f $(DESTDIR)$(libdir)/$(LIBEBL_SUBDIR)/libebl_$${m}.so; \
	done
	rmdir --ignore-fail-on-non-empty $(DESTDIR)$(libdir)/$(LIBEBL_SUBDIR)

noinst_HEADERS = libebl_CPU.h common-reloc.c linux-core-note.c x86_corenote.c
EXTRA_DIST = $(foreach m,$(modules),$($(m)_SRCS)) $(modules:=_reloc.def)

CLEANFILES += $(foreach m,$(modules),\
			libebl_$(m).map libebl_$(m).so \
			$(am_libebl_$(m)_pic_a_OBJECTS))
