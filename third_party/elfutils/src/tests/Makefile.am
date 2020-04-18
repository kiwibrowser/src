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
if MUDFLAP
BUILD_RPATH = \$$ORIGIN/../backends
else
BUILD_RPATH = \$$ORIGIN/../libasm:\$$ORIGIN/../libdw:\$$ORIGIN/../backends:\$$ORIGIN/../libelf
endif

AM_LDFLAGS =

if !STANDALONE
AM_CPPFLAGS += -I$(top_srcdir)/libasm -I$(top_srcdir)/libdw \
	    -I$(top_srcdir)/libdwfl \
	    -I$(top_srcdir)/libebl -I$(top_srcdir)/libelf \
	    -I$(top_srcdir)/lib -I..
AM_LDFLAGS += -Wl,-rpath-link,../libasm:../libdw:../libelf
endif

if TESTS_RPATH
AM_LDFLAGS += -Wl,-rpath,$(BUILD_RPATH)
tests_rpath = yes
else
tests_rpath = no
endif

check_PROGRAMS = arextract arsymtest newfile saridx scnnames sectiondump \
		  showptable update1 update2 update3 update4 test-nlist \
		  show-die-info get-files get-lines get-pubnames \
		  get-aranges allfcts line2addr addrscopes funcscopes \
		  show-abbrev hash newscn ecp dwflmodtest \
		  find-prologues funcretval allregs rdwrmmap \
		  dwfl-bug-addr-overflow arls dwfl-bug-fd-leak \
		  dwfl-addr-sect dwfl-bug-report early-offscn \
		  dwfl-bug-getmodules dwarf-getmacros addrcfi \
		  test-flag-nobits dwarf-getstring rerequest_tag \
		  alldts md5-sha1-test typeiter typeiter2 low_high_pc \
		  test-elf_cntl_gelf_getshdr dwflsyms dwfllines \
		  dwfl-report-elf-align varlocs backtrace backtrace-child \
		  backtrace-data backtrace-dwarf
asm_TESTS = asm-tst1 asm-tst2 asm-tst3 asm-tst4 asm-tst5 \
	    asm-tst6 asm-tst7 asm-tst8 asm-tst9

if BIARCH
check_PROGRAMS += backtrace-child-biarch
endif

# Substitute $(COMPILE).
backtrace-child-biarch$(EXEEXT): backtrace-child.c
	$(CC_BIARCH) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
		     $(AM_CPPFLAGS) $(CPPFLAGS) \
		     $(AM_CFLAGS) $(CFLAGS) $(backtrace_child_CFLAGS) \
		     $(AM_LDFLAGS) $(LDFLAGS) $(backtrace_child_LDFLAGS) \
		     -o $@ $<

TESTS = run-arextract.sh run-arsymtest.sh newfile test-nlist \
	update1 update2 update3 update4 \
	run-show-die-info.sh run-get-files.sh run-get-lines.sh \
	run-get-pubnames.sh run-get-aranges.sh run-allfcts.sh \
	run-show-abbrev.sh run-line2addr.sh hash \
	newscn run-strip-test.sh run-strip-test2.sh \
	run-strip-test3.sh run-strip-test4.sh run-strip-test5.sh \
	run-strip-test6.sh run-strip-test7.sh run-strip-test8.sh \
	run-strip-groups.sh run-strip-reloc.sh \
	run-unstrip-test.sh run-unstrip-test2.sh run-unstrip-M.sh \
	run-ecp-test.sh run-ecp-test2.sh run-alldts.sh \
	run-elflint-test.sh run-elflint-self.sh run-ranlib-test.sh \
	run-ranlib-test2.sh run-ranlib-test3.sh run-ranlib-test4.sh \
	run-addrscopes.sh run-strings-test.sh run-funcscopes.sh \
	run-find-prologues.sh run-allregs.sh run-addrcfi.sh \
	run-nm-self.sh run-readelf-self.sh \
	run-readelf-test1.sh run-readelf-test2.sh run-readelf-test3.sh \
	run-readelf-test4.sh run-readelf-twofiles.sh \
	run-readelf-macro.sh run-readelf-loc.sh \
	run-readelf-aranges.sh run-readelf-line.sh \
	run-native-test.sh run-bug1-test.sh \
	dwfl-bug-addr-overflow run-addrname-test.sh \
	dwfl-bug-fd-leak dwfl-bug-report \
	run-dwfl-bug-offline-rel.sh run-dwfl-addr-sect.sh \
	run-disasm-x86.sh run-disasm-x86-64.sh \
	run-early-offscn.sh run-dwarf-getmacros.sh \
	run-test-flag-nobits.sh run-prelink-addr-test.sh \
	run-dwarf-getstring.sh run-rerequest_tag.sh run-typeiter.sh \
	run-readelf-d.sh run-readelf-gdb_index.sh run-unstrip-n.sh \
	run-low_high_pc.sh run-macro-test.sh run-elf_cntl_gelf_getshdr.sh \
	run-test-archive64.sh run-readelf-vmcoreinfo.sh \
	run-readelf-mixed-corenote.sh run-dwfllines.sh \
	run-dwfl-report-elf-align.sh run-addr2line-test.sh \
	run-addr2line-i-test.sh run-varlocs.sh run-funcretval.sh \
	run-backtrace-native.sh run-backtrace-data.sh run-backtrace-dwarf.sh \
	run-backtrace-native-biarch.sh run-backtrace-native-core.sh \
	run-backtrace-native-core-biarch.sh run-backtrace-core-x86_64.sh \
	run-backtrace-core-i386.sh run-backtrace-core-ppc.sh \
	run-backtrace-core-s390x.sh run-backtrace-core-s390.sh

if !BIARCH
export ELFUTILS_DISABLE_BIARCH = 1
endif

if !STANDALONE
check_PROGRAMS += msg_tst md5-sha1-test
TESTS += msg_tst md5-sha1-test
endif

if LZMA
TESTS += run-readelf-s.sh run-dwflsyms.sh
endif

if HAVE_LIBASM
check_PROGRAMS += $(asm_TESTS)
TESTS += $(asm_TESTS)
endif

if ENABLE_DWZ
TESTS += run-readelf-dwz-multi.sh run-allfcts-multi.sh
endif


EXTRA_DIST = run-arextract.sh run-arsymtest.sh \
	     run-show-die-info.sh run-get-files.sh run-get-lines.sh \
	     run-get-pubnames.sh run-get-aranges.sh \
	     run-show-abbrev.sh run-strip-test.sh \
	     run-strip-test2.sh run-ecp-test.sh run-ecp-test2.sh \
	     testfile.bz2 testfile2.bz2 testfile3.bz2 testfile4.bz2 \
	     testfile5.bz2 testfile6.bz2 testfile7.bz2 testfile8.bz2 \
	     testfile9.bz2 testfile10.bz2 testfile11.bz2 testfile12.bz2 \
	     testfile13.bz2 run-strip-test3.sh run-allfcts.sh \
	     testfile_class_func.bz2 testfile_nested_funcs.bz2 \
	     run-line2addr.sh run-elflint-test.sh testfile14.bz2 \
	     run-strip-test4.sh run-strip-test5.sh run-strip-test6.sh \
	     run-strip-test7.sh run-strip-test8.sh run-strip-groups.sh \
	     run-strip-reloc.sh hello_i386.ko.bz2 hello_x86_64.ko.bz2 \
	     hello_ppc64.ko.bz2 hello_s390.ko.bz2 hello_aarch64.ko.bz2 \
	     run-unstrip-test.sh run-unstrip-test2.sh run-unstrip-M.sh\
	     run-elflint-self.sh run-ranlib-test.sh run-ranlib-test2.sh \
	     run-ranlib-test3.sh run-ranlib-test4.sh \
	     run-addrscopes.sh run-strings-test.sh run-funcscopes.sh \
	     run-nm-self.sh run-readelf-self.sh run-addrcfi.sh \
	     run-find-prologues.sh run-allregs.sh run-native-test.sh \
	     run-addrname-test.sh run-dwfl-bug-offline-rel.sh \
	     run-dwfl-addr-sect.sh run-early-offscn.sh \
	     run-dwarf-getmacros.sh run-test-flag-nobits.sh \
	     run-dwarf-getstring.sh run-rerequest_tag.sh run-alldts.sh \
	     testfile15.bz2 testfile15.debug.bz2 \
	     testfile16.bz2 testfile16.debug.bz2 \
	     testfile17.bz2 testfile17.debug.bz2 \
	     testfile18.bz2 testfile19.bz2 testfile19.index.bz2 \
	     testfile20.bz2 testfile20.index.bz2 \
	     testfile21.bz2 testfile21.index.bz2 \
	     testfile22.bz2 testfile23.bz2 testfile24.bz2 testfile25.bz2 \
	     testfile26.bz2 testfile27.bz2 \
	     coverage.sh test-subr.sh test-wrapper.sh \
	     run-readelf-test1.sh run-readelf-test2.sh run-readelf-test3.sh \
	     run-readelf-test4.sh run-readelf-twofiles.sh \
	     run-bug1-test.sh testfile28.bz2 testfile28.rdwr.bz2 \
	     testfile29.bz2 testfile29.rdwr.bz2 \
	     testfile30.bz2 testfile31.bz2 testfile32.bz2 testfile33.bz2 \
	     testfile34.bz2 testfile35.bz2 testfile35.debug.bz2 \
	     testfile36.bz2 testfile36.debug.bz2 \
	     testfile37.bz2 testfile37.debug.bz2 \
	     testfile38.bz2 testfile39.bz2 testfile40.bz2 testfile40.debug.bz2 \
	     testfile41.bz2 testfile42.bz2 testfile43.bz2 \
	     testfile44.S.bz2 testfile44.expect.bz2 run-disasm-x86.sh \
	     testfile45.S.bz2 testfile45.expect.bz2 run-disasm-x86-64.sh \
	     testfile46.bz2 testfile47.bz2 testfile48.bz2 testfile48.debug.bz2 \
	     testfile49.bz2 testfile50.bz2 testfile51.bz2 \
	     run-readelf-macro.sh testfilemacro.bz2 \
	     run-readelf-loc.sh testfileloc.bz2 \
	     run-readelf-aranges.sh run-readelf-line.sh testfilefoobarbaz.bz2 \
	     run-readelf-dwz-multi.sh libtestfile_multi_shared.so.bz2 \
	     testfile_multi.dwz.bz2 testfile_multi_main.bz2 \
	     testfile-dwzstr.bz2 testfile-dwzstr.multi.bz2 \
	     run-allfcts-multi.sh \
	     test-offset-loop.bz2 test-offset-loop.alt.bz2 \
	     run-prelink-addr-test.sh \
	     testfile52-32.so.bz2 testfile52-32.so.debug.bz2 \
	     testfile52-32.prelink.so.bz2 testfile52-32.noshdrs.so.bz2 \
	     testfile52-64.so.bz2 testfile52-64.so.debug.bz2 \
	     testfile52-64.prelink.so.bz2 testfile52-64.noshdrs.so.bz2 \
	     testfile53-32.bz2 testfile53-32.debug.bz2 \
	     testfile53-32.prelink.bz2 testfile53-64.bz2 \
	     testfile53-64.debug.bz2 testfile53-64.prelink.bz2 \
	     testfile54-32.so.bz2 testfile54-32.so.debug.bz2 \
	     testfile54-32.prelink.so.bz2 testfile54-32.noshdrs.so.bz2 \
	     testfile54-64.so.bz2 testfile54-64.so.debug.bz2 \
	     testfile54-64.prelink.so.bz2 testfile54-64.noshdrs.so.bz2 \
	     testfile55-32.bz2 testfile55-32.debug.bz2 \
	     testfile55-32.prelink.bz2 testfile55-64.bz2 \
	     testfile55-64.debug.bz2 testfile55-64.prelink.bz2 \
	     testfile56.bz2 testfile57.bz2 testfile58.bz2 \
	     run-typeiter.sh testfile59.bz2 \
	     run-readelf-d.sh testlib_dynseg.so.bz2 \
	     run-readelf-gdb_index.sh testfilegdbindex5.bz2 \
	     testfilegdbindex7.bz2 \
	     run-readelf-s.sh testfilebazdbg.bz2 testfilebazdyn.bz2 \
	     testfilebazmin.bz2 testfilebazdbg.debug.bz2 testfilebazmdb.bz2 \
	     testfilebaztab.bz2 testfilebasmin.bz2 \
	     testfilebazdbg_pl.bz2 testfilebazmin_pl.bz2 \
	     testfilebazdbg_plr.bz2 testfilebazmin_plr.bz2 \
	     testfilebazdbgppc64.bz2 testfilebazdbgppc64.debug.bz2 \
	     testfilebazdbgppc64_pl.bz2 testfilebazdbgppc64_plr.bz2 \
	     testfilebazdynppc64.bz2 testfilebazmdbppc64.bz2 \
	     testfilebazminppc64.bz2 testfilebazminppc64_pl.bz2 \
	     testfilebazminppc64_plr.bz2 testfilebaztabppc64.bz2 \
	     run-dwflsyms.sh \
	     run-unstrip-n.sh testcore-rtlib.bz2 testcore-rtlib-ppc.bz2 \
	     run-low_high_pc.sh testfile_low_high_pc.bz2 \
	     run-macro-test.sh testfile-macinfo.bz2 testfile-macros.bz2 \
	     run-elf_cntl_gelf_getshdr.sh \
	     run-test-archive64.sh testarchive64.a.bz2 \
	     testfile60.bz2 testfile61.bz2 \
	     run-readelf-vmcoreinfo.sh testfile62.bz2 \
	     run-readelf-mixed-corenote.sh testfile63.bz2 testfile64.bz2 \
	     testfile65.bz2 testfile67.bz2 testfile68.bz2 \
	     testfile69.core.bz2 testfile69.so.bz2 \
	     testfile70.core.bz2 testfile70.exec.bz2 testfile71.bz2 \
	     run-dwfllines.sh run-dwfl-report-elf-align.sh \
	     testfile-dwfl-report-elf-align-shlib.so.bz2 \
	     testfilenolines.bz2 test-core-lib.so.bz2 test-core.core.bz2 \
	     test-core.exec.bz2 run-addr2line-test.sh \
	     run-addr2line-i-test.sh testfile-inlines.bz2 \
	     testfileppc32.bz2 testfileppc64.bz2 \
	     testfiles390.bz2 testfiles390x.bz2 \
	     testfilearm.bz2 testfileaarch64.bz2 \
	     run-varlocs.sh \
	     testfile_const_type.c testfile_const_type.bz2 \
	     testfile_implicit_pointer.c testfile_implicit_pointer.bz2 \
	     testfile_parameter_ref.c testfile_parameter_ref.bz2 \
	     testfile_entry_value.c testfile_entry_value.bz2 \
	     testfile_implicit_value.c testfile_implicit_value.bz2 \
	     testfile_aarch64_core.bz2 \
	     run-funcretval.sh funcretval_test.c funcretval_test_aarch64.bz2 \
	     run-backtrace-data.sh run-backtrace-dwarf.sh cleanup-13.c \
	     run-backtrace-native.sh run-backtrace-native-biarch.sh \
	     run-backtrace-native-core.sh run-backtrace-native-core-biarch.sh \
	     run-backtrace-core-x86_64.sh run-backtrace-core-i386.sh \
	     backtrace-subr.sh backtrace.i386.core.bz2 backtrace.i386.exec.bz2 \
	     backtrace.x86_64.core.bz2 backtrace.x86_64.exec.bz2 \
	     backtrace.ppc.core.bz2 backtrace.ppc.exec.bz2 \
	     run-backtrace-core-ppc.sh testfile66.bz2 testfile66.core.bz2 \
	     backtrace.s390x.core.bz2 backtrace.s390x.exec.bz2 \
	     backtrace.s390.core.bz2 backtrace.s390.exec.bz2 \
	     run-backtrace-core-s390x.sh run-backtrace-core-s390.sh

if USE_VALGRIND
valgrind_cmd='valgrind -q --error-exitcode=1 --run-libc-freeres=no'
endif


installed_TESTS_ENVIRONMENT = libdir=$(DESTDIR)$(libdir); \
			      bindir=$(DESTDIR)$(bindir); \
			      LC_ALL=C; LANG=C; \
			      VALGRIND_CMD=$(valgrind_cmd); \
			      abs_srcdir=$(abs_srcdir); \
			      abs_builddir=$(abs_builddir); \
			      abs_top_builddir=$(abs_top_builddir); \
			      export abs_srcdir; export abs_builddir; \
			      export abs_top_builddir; \
			      export libdir; export bindir; \
			      export LC_ALL; export LANG; export VALGRIND_CMD; \
			      NM=$(NM); export NM;
installed_LOG_COMPILER = $(abs_srcdir)/test-wrapper.sh \
			 installed $(tests_rpath) \
			 '$(program_transform_name)'
if STANDALONE
TESTS_ENVIRONMENT = $(installed_TESTS_ENVIRONMENT)
LOG_COMPILER = $(installed_LOG_COMPILER)
else !STANDALONE
TESTS_ENVIRONMENT = LC_ALL=C; LANG=C; VALGRIND_CMD=$(valgrind_cmd); \
		    abs_srcdir=$(abs_srcdir);  abs_builddir=$(abs_builddir); \
		    abs_top_builddir=$(abs_top_builddir); \
		    export abs_srcdir; export abs_builddir; \
		    export abs_top_builddir; \
		    export LC_ALL; export LANG; export VALGRIND_CMD; \
		    NM=$(NM); export NM;
LOG_COMPILER = $(abs_srcdir)/test-wrapper.sh \
	       $(abs_top_builddir)/libdw:$(abs_top_builddir)/backends:$(abs_top_builddir)/libelf:$(abs_top_builddir)/libasm

installcheck-local:
	$(MAKE) $(AM_MAKEFLAGS) \
		TESTS_ENVIRONMENT="$(installed_TESTS_ENVIRONMENT)" \
		LOG_COMPILER="$(installed_LOG_COMPILER)" check-TESTS
endif !STANDALONE

if MUDFLAP
static_build = yes
endif

if STANDALONE
libdw = -ldw
libelf = -lelf
libasm = -lasm
libebl = -lebl
else !STANDALONE
if BUILD_STATIC
libdw = ../libdw/libdw.a $(zip_LIBS) $(libelf) $(libebl) -ldl
libelf = ../libelf/libelf.a
libasm = ../libasm/libasm.a
else
libdw = ../libdw/libdw.so
libelf = ../libelf/libelf.so
libasm = ../libasm/libasm.so
endif
libebl = ../libebl/libebl.a
libeu = ../lib/libeu.a
endif !STANDALONE

arextract_LDADD = $(libelf) $(libmudflap)
arsymtest_LDADD = $(libelf) $(libmudflap)
newfile_LDADD = $(libelf) $(libmudflap)
saridx_LDADD = $(libelf) $(libmudflap)
scnnames_LDADD = $(libelf) $(libmudflap)
sectiondump_LDADD = $(libelf) $(libmudflap)
showptable_LDADD = $(libelf) $(libmudflap)
hash_LDADD = $(libelf) $(libmudflap)
test_nlist_LDADD = $(libelf) $(libmudflap)
msg_tst_LDADD = $(libelf) $(libmudflap)
newscn_LDADD = $(libelf) $(libmudflap)
early_offscn_LDADD = $(libelf) $(libmudflap)
ecp_LDADD = $(libelf) $(libmudflap)
update1_LDADD = $(libelf) $(libmudflap)
update2_LDADD = $(libelf) $(libmudflap)
update3_LDADD = $(libebl) $(libelf) $(libmudflap)
update4_LDADD = $(libebl) $(libelf) $(libmudflap)
show_die_info_LDADD = $(libdw) $(libelf) $(libmudflap)
get_pubnames_LDADD = $(libdw) $(libelf) $(libmudflap)
show_abbrev_LDADD = $(libdw) $(libelf) $(libmudflap)
get_lines_LDADD = $(libdw) $(libelf) $(libmudflap)
get_files_LDADD = $(libdw) $(libelf) $(libmudflap)
get_aranges_LDADD = $(libdw) $(libelf) $(libmudflap)
allfcts_LDADD = $(libdw) $(libelf) $(libmudflap)
line2addr_no_Wformat = yes
line2addr_LDADD = $(libdw) $(libmudflap)
addrscopes_LDADD = $(libdw) $(libmudflap)
funcscopes_LDADD = $(libdw) $(libmudflap)
funcretval_LDADD = $(libdw) $(libmudflap)
allregs_LDADD = $(libdw) $(libmudflap)
find_prologues_LDADD = $(libdw) $(libmudflap)
#show_ciefde_LDADD = ../libdwarf/libdwarf.so $(libelf) $(libmudflap)
asm_tst1_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
asm_tst2_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
asm_tst3_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
asm_tst4_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
asm_tst5_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
asm_tst6_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
asm_tst7_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
asm_tst8_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
asm_tst9_LDADD = $(libasm) $(libebl) $(libelf) $(libmudflap) -ldl
dwflmodtest_LDADD = $(libdw) $(libebl) $(libelf) $(libmudflap) -ldl
rdwrmmap_LDADD = $(libelf) $(libmudflap)
dwfl_bug_addr_overflow_LDADD = $(libdw) $(libebl) $(libelf) $(libmudflap) -ldl
arls_LDADD = $(libelf) $(libmudflap)
dwfl_bug_fd_leak_LDADD = $(libdw) $(libebl) $(libelf) $(libmudflap) -ldl
dwfl_bug_report_LDADD = $(libdw) $(libebl) $(libelf) $(libmudflap) -ldl
dwfl_bug_getmodules_LDADD = $(libdw) $(libebl) $(libelf) $(libmudflap) -ldl
dwfl_addr_sect_LDADD = $(libdw) $(libebl) $(libelf) $(libmudflap) -ldl
dwarf_getmacros_LDADD = $(libdw) $(libmudflap)
dwarf_getstring_LDADD = $(libdw) $(libmudflap)
addrcfi_LDADD = $(libdw) $(libebl) $(libelf) $(libmudflap) -ldl
test_flag_nobits_LDADD = $(libelf) $(libmudflap)
rerequest_tag_LDADD = $(libdw) $(libmudflap)
alldts_LDADD = $(libebl) $(libelf) $(libmudflap)
md5_sha1_test_LDADD = $(libeu)
typeiter_LDADD = $(libdw) $(libelf) $(libmudflap)
typeiter2_LDADD = $(libdw) $(libelf) $(libmudflap)
low_high_pc_LDADD = $(libdw) $(libelf) $(libmudflap)
test_elf_cntl_gelf_getshdr_LDADD = $(libelf) $(libmudflap)
dwflsyms_LDADD = $(libdw) $(libelf) $(libmudflap)
dwfllines_LDADD = $(libdw) $(libelf) $(libmudflap)
dwfl_report_elf_align_LDADD = $(libdw) $(libmudflap)
varlocs_LDADD = $(libdw) $(libelf) $(libmudflap)
backtrace_LDADD = $(libdw) $(libelf) $(libmudflap)
# backtrace-child-biarch also uses those *_CFLAGS and *_LDLAGS variables:
backtrace_child_CFLAGS = -fPIE
backtrace_child_LDFLAGS = -pie -pthread
backtrace_child_biarch_SOURCES = backtrace-child.c
backtrace_data_LDADD = $(libdw) $(libelf) $(libmudflap)
backtrace_dwarf_CFLAGS = -Wno-unused-parameter
backtrace_dwarf_LDADD = $(libdw) $(libelf) $(libmudflap)

if GCOV
check: check-am coverage
.PHONY: coverage
coverage:
	-$(srcdir)/coverage.sh
endif
