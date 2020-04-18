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

SUBDIRS = src

ACLOCAL_AMFLAGS = -I m4

doxygen:
	cd doxygen && $(MAKE)

check-local:
	$(MAKE) -C src/mapi/glapi/tests check
	$(MAKE) -C src/mapi/shared-glapi/tests check
	$(MAKE) -C src/mesa/main/tests check
	$(MAKE) -C src/glsl/tests check
	$(MAKE) -C src/glx/tests check

clean-local:
	-@touch $(top_builddir)/configs/current
	-@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) clean) ; \
		fi \
	done
	-@test -s $(top_builddir)/configs/current || rm -f $(top_builddir)/configs/current



distclean-local:
	-rm -rf lib*
	-rm -f $(top_builddir)/configs/current
	-find . '(' -name '*.o' -o -name '*.a' -o -name '*.so' -o \
	  -name depend -o -name depend.bak ')' -exec rm -f '{}' ';'

.PHONY: doxygen

# Rules for making release tarballs

PACKAGE_VERSION=9.0.3
PACKAGE_DIR = Mesa-$(PACKAGE_VERSION)
PACKAGE_NAME = MesaLib-$(PACKAGE_VERSION)

EXTRA_FILES = \
	aclocal.m4					\
	configure					\
	bin/ar-lib					\
	bin/compile					\
	bin/config.sub					\
	bin/config.guess				\
	bin/depcomp					\
	bin/install-sh					\
	bin/ltmain.sh					\
	bin/missing					\
	bin/ylwrap					\
	src/glsl/glsl_parser.cc				\
	src/glsl/glsl_parser.h				\
	src/glsl/glsl_lexer.cc				\
	src/glsl/glcpp/glcpp-lex.c			\
	src/glsl/glcpp/glcpp-parse.c			\
	src/glsl/glcpp/glcpp-parse.h			\
	src/mesa/main/api_exec_es1.c			\
	src/mesa/main/api_exec_es1_dispatch.h		\
	src/mesa/main/api_exec_es1_remap_helper.h	\
	src/mesa/main/api_exec_es2.c			\
	src/mesa/main/api_exec_es2_dispatch.h		\
	src/mesa/main/api_exec_es2_remap_helper.h	\
	src/mesa/program/lex.yy.c			\
	src/mesa/program/program_parse.tab.c		\
	src/mesa/program/program_parse.tab.h		\
	`git ls-files | grep "Makefile.am" | sed -e "s/Makefile.am/Makefile.in/"`


IGNORE_FILES = \
	-x autogen.sh


parsers: configure
	-@touch $(top_builddir)/configs/current
	$(MAKE) -C src/glsl glsl_parser.cc glsl_parser.h glsl_lexer.cc
	$(MAKE) -C src/glsl/glcpp glcpp-lex.c glcpp-parse.c glcpp-parse.h
	$(MAKE) -C src/mesa program/lex.yy.c program/program_parse.tab.c program/program_parse.tab.h

# Everything for new a Mesa release:
ARCHIVES = $(PACKAGE_NAME).tar.gz \
	$(PACKAGE_NAME).tar.bz2 \
	$(PACKAGE_NAME).zip

tarballs: md5
	rm -f ../$(PACKAGE_DIR) $(PACKAGE_NAME).tar

manifest.txt: .git
	( \
		ls -1 $(EXTRA_FILES) ; \
		git ls-files $(IGNORE_FILES) \
	) | sed -e '/^\(.*\/\)\?\./d' -e "s@^@$(PACKAGE_DIR)/@" > $@

../$(PACKAGE_DIR):
	ln -s $(PWD) $@

$(PACKAGE_NAME).tar: parsers ../$(PACKAGE_DIR) manifest.txt
	cd .. ; tar -cf $(PACKAGE_DIR)/$(PACKAGE_NAME).tar -T $(PACKAGE_DIR)/manifest.txt

$(PACKAGE_NAME).tar.gz: $(PACKAGE_NAME).tar ../$(PACKAGE_DIR)
	gzip --stdout --best $(PACKAGE_NAME).tar > $(PACKAGE_NAME).tar.gz

$(PACKAGE_NAME).tar.bz2: $(PACKAGE_NAME).tar
	bzip2 --stdout --best $(PACKAGE_NAME).tar > $(PACKAGE_NAME).tar.bz2

$(PACKAGE_NAME).zip: parsers ../$(PACKAGE_DIR) manifest.txt
	rm -f $(PACKAGE_NAME).zip ; \
	cd .. ; \
	zip -q -@ $(PACKAGE_NAME).zip < $(PACKAGE_DIR)/manifest.txt ; \
	mv $(PACKAGE_NAME).zip $(PACKAGE_DIR)

md5: $(ARCHIVES)
	@-md5sum $(PACKAGE_NAME).tar.gz
	@-md5sum $(PACKAGE_NAME).tar.bz2
	@-md5sum $(PACKAGE_NAME).zip

.PHONY: tarballs md5
