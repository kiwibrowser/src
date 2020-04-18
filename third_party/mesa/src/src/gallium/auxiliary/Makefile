TOP = ../../..
include $(TOP)/configs/current

LIBNAME = gallium

# get source lists
include Makefile.sources

C_SOURCES += $(GENERATED_SOURCES)

ifeq ($(MESA_LLVM),1)
C_SOURCES += \
	$(GALLIVM_SOURCES)
CPP_SOURCES += \
	$(GALLIVM_CPP_SOURCES)

# LLVM >= 3.2 requires -fno-rtti
ifeq ($(shell expr `echo $(LLVM_VERSION) | sed -e 's/\([0-9]\)\.\([0-9]\)/\10\2/g'` \>= 302),1)
CXXFLAGS += -fno-rtti
endif
endif


include ../Makefile.template

default install clean: %: subdirs-%

subdirs-%:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) $*) || exit 1; \
		fi \
	done

indices/u_indices_gen.c: indices/u_indices_gen.py
	$(PYTHON2) $< > $@

indices/u_unfilled_gen.c: indices/u_unfilled_gen.py
	$(PYTHON2) $< > $@

util/u_format_srgb.c: util/u_format_srgb.py
	$(PYTHON2) $< > $@

util/u_format_table.c: util/u_format_table.py util/u_format_pack.py util/u_format_parse.py util/u_format.csv
	$(PYTHON2) util/u_format_table.py util/u_format.csv > $@

# DO NOT DELETE
