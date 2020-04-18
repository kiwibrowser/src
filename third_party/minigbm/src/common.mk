# Copyright 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# If this file is part of another source distribution, it's license may be
# stored in LICENSE.makefile or LICENSE.common.mk.
#
# NOTE NOTE NOTE
#  The authoritative common.mk is located in:
#    https://chromium.googlesource.com/chromiumos/platform2/+/master/common-mk
#  Please make all changes there, then copy into place in other repos.
# NOTE NOTE NOTE
#
# This file provides a common architecture for building C/C++ source trees.
# It uses recursive makefile inclusion to create a single make process which
# can be built in the source tree or with the build artifacts placed elsewhere.
#
# It is fully parallelizable for all targets, including static archives.
#
# To use:
# 1. Place common.mk in your top source level
# 2. In your top-level Makefile, place "include common.mk" at the top
# 3. In all subdirectories, create a 'module.mk' file that starts with:
#      include common.mk
#    And then contains the remainder of your targets.
# 4. All build targets should look like:
#    relative/path/target: relative/path/obj.o
#
# See existing makefiles for rule examples.
#
# Exported macros:
#   - cc_binary, cxx_binary provide standard compilation steps for binaries
#   - cxx_library, cc_library provide standard compilation steps for
#     shared objects.
#   All of the above optionally take an argument for extra flags.
#   - update_archive creates/updates a given .a target
#
# Instead of using the build macros, most users can just use wrapped targets:
#   - CXX_BINARY, CC_BINARY, CC_STATIC_BINARY, CXX_STATIC_BINARY
#   - CXX_LIBRARY, CC_LIBRARY, CC_STATIC_LIBRARY, CXX_STATIC_LIBRARY
#   - E.g., CXX_BINARY(mahbinary): foo.o
#   - object.depends targets may be used when a prerequisite is required for an
#     object file. Because object files result in multiple build artifacts to
#     handle PIC and PIE weirdness. E.g.
#       foo.o.depends: generated/dbus.h
#   - TEST(binary) or TEST(CXX_BINARY(binary)) may be used as a prerequisite
#     for the tests target to trigger an automated test run.
#   - CLEAN(file_or_dir) dependency can be added to 'clean'.
#
# If source code is being generated, rules will need to be registered for
# compiling the objects.  This can be done by adding one of the following
# to the Makefile:
#   - For C source files
#   $(eval $(call add_object_rules,sub/dir/gen_a.o sub/dir/b.o,CC,c,CFLAGS))
#   - For C++ source files
#   $(eval $(call add_object_rules,sub/dir/gen_a.o sub/dir/b.o,CXX,cc,CXXFLAGS))
#
# Exported targets meant to have prerequisites added to:
#  - all - Your desired targets should be given
#  - tests - Any TEST(test_binary) targets should be given
#  - FORCE - force the given target to run regardless of changes
#            In most cases, using .PHONY is preferred.
#
# Possible command line variables:
#   - COLOR=[0|1] to set ANSI color output (default: 1)
#   - VERBOSE=[0|1] to hide/show commands (default: 0)
#   - MODE=[opt|dbg|profiling] (default: opt)
#          opt - Enable optimizations for release builds
#          dbg - Turn down optimization for debugging
#          profiling - Turn off optimization and turn on profiling/coverage
#                      support.
#   - ARCH=[x86|arm|supported qemu name] (default: from portage or uname -m)
#   - SPLITDEBUG=[0|1] splits debug info in target.debug (default: 0)
#        If NOSTRIP=1, SPLITDEBUG will never strip the final emitted objects.
#   - NOSTRIP=[0|1] determines if binaries are stripped. (default: 1)
#        NOSTRIP=0 and MODE=opt will also drop -g from the CFLAGS.
#   - VALGRIND=[0|1] runs tests under valgrind (default: 0)
#   - OUT=/path/to/builddir puts all output in given path (default: $PWD)
#   - VALGRIND_ARGS="" supplies extra memcheck arguments
#
# Per-target(-ish) variable:
#   - NEEDS_ROOT=[0|1] allows a TEST() target to run with root.
#     Default is 0 unless it is running under QEmu.
#   - NEEDS_MOUNTS=[0|1] allows a TEST() target running on QEmu to get
#     setup mounts in the $(SYSROOT)
#
# Caveats:
# - Directories or files with spaces in them DO NOT get along with GNU Make.
#   If you need them, all uses of dir/notdir/etc will need to have magic
#   wrappers.  Proceed at risk to your own sanity.
# - External CXXFLAGS and CFLAGS should be passed via the environment since
#   this file does not use 'override' to control them.
# - Our version of GNU Make doesn't seem to support the 'private' variable
#   annotation, so you can't tag a variable private on a wrapping target.

# Behavior configuration variables
SPLITDEBUG ?= 0
NOSTRIP ?= 1
VALGRIND ?= 0
COLOR ?= 1
VERBOSE ?= 0
MODE ?= opt
CXXEXCEPTIONS ?= 0
ARCH ?= $(shell uname -m)

# Put objects in a separate tree based on makefile locations
# This means you can build a tree without touching it:
#   make -C $SRCDIR  # will create ./build-$(MODE)
# Or
#   make -C $SRCDIR OUT=$PWD
# This variable is extended on subdir calls and doesn't need to be re-called.
OUT ?= $(PWD)/

# Make OUT now so we can use realpath.
$(shell mkdir -p "$(OUT)")

# TODO(wad) Relative paths are resolved against SRC and not the calling dir.
# Ensure a command-line supplied OUT has a slash
override OUT := $(realpath $(OUT))/

# SRC is not meant to be set by the end user, but during make call relocation.
# $(PWD) != $(CURDIR) all the time.
export SRC ?= $(CURDIR)

# Re-start in the $(OUT) directory if we're not there.
# We may be invoked using -C or bare and we need to ensure behavior
# is consistent so we check both PWD vs OUT and PWD vs CURDIR.
override RELOCATE_BUILD := 0
ifneq (${PWD}/,${OUT})
override RELOCATE_BUILD := 1
endif
# Make sure we're running with no builtin targets. They cause
# leakage and mayhem!
ifneq (${PWD},${CURDIR})
override RELOCATE_BUILD := 1
# If we're run from the build dir, don't let it get cleaned up later.
ifeq (${PWD}/,${OUT})
$(shell touch "$(PWD)/.dont_delete_on_clean")
endif
endif  # ifneq (${PWD},${CURDIR}

# "Relocate" if we need to restart without implicit rules.
ifeq ($(subst r,,$(MAKEFLAGS)),$(MAKEFLAGS))
override RELOCATE_BUILD := 1
endif

ifeq (${RELOCATE_BUILD},1)
# By default, silence build output. Reused below as well.
QUIET = @
ifeq ($(VERBOSE),1)
  QUIET=
endif

# This target will override all targets, including prerequisites. To avoid
# calling $(MAKE) once per prereq on the given CMDGOAL, we guard it with a local
# variable.
RUN_ONCE := 0
MAKECMDGOALS ?= all
# Keep the rules split as newer make does not allow them to be declared
# on the same line.  But the way :: rules work, the _all here will also
# invoke the %:: rule while retaining "_all" as the default.
_all::
%::
	$(if $(filter 0,$(RUN_ONCE)), \
	  cd "$(OUT)" && \
	  $(MAKE) -r -I "$(SRC)" -f "$(CURDIR)/Makefile" \
	    SRC="$(CURDIR)" OUT="$(OUT)" $(foreach g,$(MAKECMDGOALS),"$(g)"),)
	$(eval RUN_ONCE := 1)
pass-to-subcall := 1
endif

ifeq ($(pass-to-subcall),)

# Only call MODULE if we're in a submodule
MODULES_LIST := $(filter-out Makefile %.d,$(MAKEFILE_LIST))
ifeq ($(words $(filter-out Makefile common.mk %.d $(SRC)/Makefile \
                           $(SRC)/common.mk,$(MAKEFILE_LIST))),0)

# All the top-level defines outside of module.mk.

#
# Helper macros
#

# Create the directory if it doesn't yet exist.
define auto_mkdir
  $(if $(wildcard $(dir $1)),$2,$(QUIET)mkdir -p "$(dir $1)")
endef

# Creates the actual archive with an index.
# The target $@ must end with .pic.a or .pie.a.
define update_archive
  $(call auto_mkdir,$(TARGET_OR_MEMBER))
  $(QUIET)# Create the archive in one step to avoid parallel use accessing it
  $(QUIET)# before all the symbols are present.
  @$(ECHO) "AR		$(subst \
$(SRC)/,,$(^:.o=$(suffix $(basename $(TARGET_OR_MEMBER))).o)) \
-> $(subst $(SRC)/,,$(TARGET_OR_MEMBER))"
  $(QUIET)$(AR) rcs $(TARGET_OR_MEMBER) \
          $(subst $(SRC)/,,$(^:.o=$(suffix $(basename $(TARGET_OR_MEMBER))).o))
endef

# Default compile from objects using pre-requisites but filters out
# subdirs and .d files.
define cc_binary
  $(call COMPILE_BINARY_implementation,CC,$(CFLAGS) $(1),$(EXTRA_FLAGS))
endef

define cxx_binary
  $(call COMPILE_BINARY_implementation,CXX,$(CXXFLAGS) $(1),$(EXTRA_FLAGS))
endef

# Default compile from objects using pre-requisites but filters out
# subdirs and .d files.
define cc_library
  $(call COMPILE_LIBRARY_implementation,CC,$(CFLAGS) $(1),$(EXTRA_FLAGS))
endef
define cxx_library
  $(call COMPILE_LIBRARY_implementation,CXX,$(CXXFLAGS) $(1),$(EXTRA_FLAGS))
endef

# Deletes files silently if they exist. Meant for use in any local
# clean targets.
define silent_rm
  $(if $(wildcard $(1)),
  $(QUIET)($(ECHO) -n '$(COLOR_RED)CLEANFILE$(COLOR_RESET)		' && \
    $(ECHO) '$(subst $(OUT)/,,$(wildcard $(1)))' && \
    $(RM) $(1) 2>/dev/null) || true,)
endef
define silent_rmdir
  $(if $(wildcard $(1)),
    $(if $(wildcard $(1)/*),
  $(QUIET)# $(1) not empty [$(wildcard $(1)/*)]. Not deleting.,
  $(QUIET)($(ECHO) -n '$(COLOR_RED)CLEANDIR$(COLOR_RESET)		' && \
    $(ECHO) '$(subst $(OUT)/,,$(wildcard $(1)))' && \
    $(RMDIR) $(1) 2>/dev/null) || true),)
endef

#
# Default variable values
#

# Only override toolchain vars if they are from make.
CROSS_COMPILE ?=
define override_var
ifneq ($(filter undefined default,$(origin $1)),)
$1 = $(CROSS_COMPILE)$2
endif
endef
$(eval $(call override_var,AR,ar))
$(eval $(call override_var,CC,gcc))
$(eval $(call override_var,CXX,g++))
$(eval $(call override_var,OBJCOPY,objcopy))
$(eval $(call override_var,PKG_CONFIG,pkg-config))
$(eval $(call override_var,RANLIB,ranlib))
$(eval $(call override_var,STRIP,strip))

RMDIR ?= rmdir
ECHO = /bin/echo -e

ifeq ($(lastword $(subst /, ,$(CC))),clang)
CDRIVER = clang
else
CDRIVER = gcc
endif

ifeq ($(lastword $(subst /, ,$(CXX))),clang++)
CXXDRIVER = clang
else
CXXDRIVER = gcc
endif

# Internal macro to support check_XXX macros below.
# Usage: $(call check_compile, [code], [compiler], [code_type], [c_flags],
#               [extra_c_flags], [library_flags], [success_ret], [fail_ret])
# Return: [success_ret] if compile succeeded, otherwise [fail_ret]
check_compile = $(shell printf '%b\n' $(1) | \
  $($(2)) $($(4)) -x $(3) $(LDFLAGS) $(5) - $(6) -o /dev/null > /dev/null 2>&1 \
  && echo "$(7)" || echo "$(8)")

# Helper macro to check whether a test program will compile with the specified
# compiler flags.
# Usage: $(call check_compile_cc, [code], [flags], [alternate_flags])
# Return: [flags] if compile succeeded, otherwise [alternate_flags]
check_compile_cc = $(call check_compile,$(1),CC,c,CFLAGS,$(2),,$(2),$(3))
check_compile_cxx = $(call check_compile,$(1),CXX,c++,CXXFLAGS,$(2),,$(2),$(3))

# Helper macro to check whether a test program will compile with the specified
# libraries.
# Usage: $(call check_compile_cc, [code], [library_flags], [alternate_flags])
# Return: [library_flags] if compile succeeded, otherwise [alternate_flags]
check_libs_cc = $(call check_compile,$(1),CC,c,CFLAGS,,$(2),$(2),$(3))
check_libs_cxx = $(call check_compile,$(1),CXX,c++,CXXFLAGS,,$(2),$(2),$(3))

# Helper macro to check whether the compiler accepts the specified flags.
# Usage: $(call check_compile_cc, [flags], [alternate_flags])
# Return: [flags] if compile succeeded, otherwise [alternate_flags]
check_cc = $(call check_compile_cc,'int main() { return 0; }',$(1),$(2))
check_cxx = $(call check_compile_cxx,'int main() { return 0; }',$(1),$(2))

# Choose the stack protector flags based on whats supported by the compiler.
SSP_CFLAGS := $(call check_cc,-fstack-protector-strong)
ifeq ($(SSP_CFLAGS),)
 SSP_CFLAGS := $(call check_cc,-fstack-protector-all)
endif

# To update these from an including Makefile:
#  CXXFLAGS += -mahflag  # Append to the list
#  CXXFLAGS := -mahflag $(CXXFLAGS) # Prepend to the list
#  CXXFLAGS := $(filter-out badflag,$(CXXFLAGS)) # Filter out a value
# The same goes for CFLAGS.
COMMON_CFLAGS-gcc := -fvisibility=internal -ggdb3 -Wa,--noexecstack
COMMON_CFLAGS-clang := -fvisibility=hidden -ggdb
COMMON_CFLAGS := -Wall -Werror -fno-strict-aliasing $(SSP_CFLAGS) -O1 -Wformat=2
CXXFLAGS += $(COMMON_CFLAGS) $(COMMON_CFLAGS-$(CXXDRIVER))
CFLAGS += $(COMMON_CFLAGS) $(COMMON_CFLAGS-$(CDRIVER))
CPPFLAGS += -D_FORTIFY_SOURCE=2

# Disable exceptions based on the CXXEXCEPTIONS setting.
ifeq ($(CXXEXCEPTIONS),0)
  CXXFLAGS := $(CXXFLAGS) -fno-exceptions -fno-unwind-tables \
    -fno-asynchronous-unwind-tables
endif

ifeq ($(MODE),opt)
  # Up the optimizations.
  CFLAGS := $(filter-out -O1,$(CFLAGS)) -O2
  CXXFLAGS := $(filter-out -O1,$(CXXFLAGS)) -O2
  # Only drop -g* if symbols aren't desired.
  ifeq ($(NOSTRIP),0)
    # TODO: do we want -fomit-frame-pointer on x86?
    CFLAGS := $(filter-out -ggdb3,$(CFLAGS))
    CXXFLAGS := $(filter-out -ggdb3,$(CXXFLAGS))
  endif
endif

ifeq ($(MODE),profiling)
  CFLAGS := $(CFLAGS) -O0 -g  --coverage
  CXXFLAGS := $(CXXFLAGS) -O0 -g  --coverage
  LDFLAGS := $(LDFLAGS) --coverage
endif

LDFLAGS := $(LDFLAGS) -Wl,-z,relro -Wl,-z,noexecstack -Wl,-z,now

# Fancy helpers for color if a prompt is defined
ifeq ($(COLOR),1)
COLOR_RESET = \x1b[0m
COLOR_GREEN = \x1b[32;01m
COLOR_RED = \x1b[31;01m
COLOR_YELLOW = \x1b[33;01m
endif

# By default, silence build output.
QUIET = @
ifeq ($(VERBOSE),1)
  QUIET=
endif

#
# Implementation macros for compile helpers above
#

# Useful for dealing with pie-broken toolchains.
# Call make with PIE=0 to disable default PIE use.
OBJ_PIE_FLAG = -fPIE
COMPILE_PIE_FLAG = -pie
ifeq ($(PIE),0)
  OBJ_PIE_FLAG =
  COMPILE_PIE_FLAG =
endif

# Favor member targets first for CXX_BINARY(%) magic.
# And strip out nested members if possible.
LP := (
RP := )
TARGET_OR_MEMBER = $(lastword $(subst $(LP), ,$(subst $(RP),,$(or $%,$@))))

# Default compile from objects using pre-requisites but filters out
# all non-.o files.
define COMPILE_BINARY_implementation
  @$(ECHO) "LD$(1)		$(subst $(PWD)/,,$(TARGET_OR_MEMBER))"
  $(call auto_mkdir,$(TARGET_OR_MEMBER))
  $(QUIET)$($(1)) $(COMPILE_PIE_FLAGS) -o $(TARGET_OR_MEMBER) \
    $(2) $(LDFLAGS) \
    $(filter %.o %.a,$(^:.o=.pie.o)) \
    $(foreach so,$(filter %.so,$^),-L$(dir $(so)) \
                            -l$(patsubst lib%,%,$(basename $(notdir $(so))))) \
    $(LDLIBS)
  $(call conditional_strip)
  @$(ECHO) -n "BIN		"
  @$(ECHO) "$(COLOR_GREEN)$(subst $(PWD)/,,$(TARGET_OR_MEMBER))$(COLOR_RESET)"
  @$(ECHO) "	$(COLOR_YELLOW)-----$(COLOR_RESET)"
endef

# TODO: add version support extracted from PV environment variable
#ifeq ($(PV),9999)
#$(warning PV=$(PV). If shared object versions matter, please force PV=.)
#endif
# Then add -Wl,-soname,$@.$(PV) ?

# Default compile from objects using pre-requisites but filters out
# all non-.o values. (Remember to add -L$(OUT) -llib)
COMMA := ,
define COMPILE_LIBRARY_implementation
  @$(ECHO) "SHARED$(1)	$(subst $(PWD)/,,$(TARGET_OR_MEMBER))"
  $(call auto_mkdir,$(TARGET_OR_MEMBER))
  $(QUIET)$($(1)) -shared -Wl,-E -o $(TARGET_OR_MEMBER) \
    $(2) $(LDFLAGS) \
    $(if $(filter %.a,$^),-Wl$(COMMA)--whole-archive,) \
    $(filter %.o ,$(^:.o=.pic.o)) \
    $(foreach a,$(filter %.a,$^),-L$(dir $(a)) \
                            -l$(patsubst lib%,%,$(basename $(notdir $(a))))) \
    $(foreach so,$(filter %.so,$^),-L$(dir $(so)) \
                            -l$(patsubst lib%,%,$(basename $(notdir $(so))))) \
    $(LDLIBS)
  $(call conditional_strip)
  @$(ECHO) -n "LIB		$(COLOR_GREEN)"
  @$(ECHO) "$(subst $(PWD)/,,$(TARGET_OR_MEMBER))$(COLOR_RESET)"
  @$(ECHO) "	$(COLOR_YELLOW)-----$(COLOR_RESET)"
endef

define conditional_strip
  $(if $(filter 0,$(NOSTRIP)),$(call strip_artifact))
endef

define strip_artifact
  @$(ECHO) "STRIP		$(subst $(OUT)/,,$(TARGET_OR_MEMBER))"
  $(if $(filter 1,$(SPLITDEBUG)), @$(ECHO) -n "DEBUG	"; \
    $(ECHO) "$(COLOR_YELLOW)\
$(subst $(OUT)/,,$(TARGET_OR_MEMBER)).debug$(COLOR_RESET)")
  $(if $(filter 1,$(SPLITDEBUG)), \
    $(QUIET)$(OBJCOPY) --only-keep-debug "$(TARGET_OR_MEMBER)" \
      "$(TARGET_OR_MEMBER).debug")
  $(if $(filter-out dbg,$(MODE)),$(QUIET)$(STRIP) --strip-unneeded \
    "$(TARGET_OR_MEMBER)",)
endef

#
# Global pattern rules
#

# Below, the archive member syntax is abused to create fancier
# syntactic sugar for recipe authors that avoids needed to know
# subcall options.  The downside is that make attempts to look
# into the phony archives for timestamps. This will cause the final
# target to be rebuilt/linked on _every_ call to make even when nothing
# has changed.  Until a better way presents itself, we have helpers that
# do the stat check on make's behalf.  Dodgy but simple.
define old_or_no_timestamp
  $(if $(realpath $%),,$(1))
  $(if $(shell find $^ -cnewer "$%" 2>/dev/null),$(1))
endef

define check_deps
  $(if $(filter 0,$(words $^)),\
    $(error Missing dependencies or declaration of $@($%)),)
endef

# Build a cxx target magically
CXX_BINARY(%):
	$(call check_deps)
	$(call old_or_no_timestamp,$(call cxx_binary))
clean: CLEAN(CXX_BINARY*)

CC_BINARY(%):
	$(call check_deps)
	$(call old_or_no_timestamp,$(call cc_binary))
clean: CLEAN(CC_BINARY*)

CXX_STATIC_BINARY(%):
	$(call check_deps)
	$(call old_or_no_timestamp,$(call cxx_binary,-static))
clean: CLEAN(CXX_STATIC_BINARY*)

CC_STATIC_BINARY(%):
	$(call check_deps)
	$(call old_or_no_timestamp,$(call cc_binary,-static))
clean: CLEAN(CC_STATIC_BINARY*)

CXX_LIBRARY(%):
	$(call check_deps)
	$(call old_or_no_timestamp,$(call cxx_library))
clean: CLEAN(CXX_LIBRARY*)

CXX_LIBARY(%):
	$(error Typo alert! LIBARY != LIBRARY)

CC_LIBRARY(%):
	$(call check_deps)
	$(call old_or_no_timestamp,$(call cc_library))
clean: CLEAN(CC_LIBRARY*)

CC_LIBARY(%):
	$(error Typo alert! LIBARY != LIBRARY)

CXX_STATIC_LIBRARY(%):
	$(call check_deps)
	$(call old_or_no_timestamp,$(call update_archive))
clean: CLEAN(CXX_STATIC_LIBRARY*)

CXX_STATIC_LIBARY(%):
	$(error Typo alert! LIBARY != LIBRARY)

CC_STATIC_LIBRARY(%):
	$(call check_deps)
	$(call old_or_no_timestamp,$(call update_archive))
clean: CLEAN(CC_STATIC_LIBRARY*)

CC_STATIC_LIBARY(%):
	$(error Typo alert! LIBARY != LIBRARY)


TEST(%): % qemu_chroot_install
	$(call TEST_implementation)
.PHONY: TEST

# multiple targets with a wildcard need to share an directory.
# Don't use this directly it just makes sure the directory is removed _after_
# the files are.
CLEANFILE(%):
	$(call silent_rm,$(TARGET_OR_MEMBER))
.PHONY: CLEANFILE

CLEAN(%): CLEANFILE(%)
	$(QUIET)# CLEAN($%) meta-target called
	$(if $(filter-out $(PWD)/,$(dir $(abspath $(TARGET_OR_MEMBER)))), \
	  $(call silent_rmdir,$(dir $(abspath $(TARGET_OR_MEMBER)))),\
	  $(QUIET)# Not deleting $(dir $(abspath $(TARGET_OR_MEMBER))) yet.)
.PHONY: CLEAN

#
# Top-level objects and pattern rules
#

# All objects for .c files at the top level
C_OBJECTS = $(patsubst $(SRC)/%.c,%.o,$(wildcard $(SRC)/*.c))


# All objects for .cxx files at the top level
CXX_OBJECTS = $(patsubst $(SRC)/%.cc,%.o,$(wildcard $(SRC)/*.cc))

# Note, the catch-all pattern rules don't work in subdirectories because
# we're building from the $(OUT) directory. At the top-level (here) they will
# work, but we go ahead and match using the module form.  Then we can place a
# generic pattern rule to capture leakage from the main Makefile. (Later in the
# file.)
#
# The reason target specific pattern rules work well for modules,
# MODULE_C_OBJECTS, is because it scopes the behavior to the given target which
# ensures we get a relative directory offset from $(OUT) which otherwise would
# not match without further magic on a per-subdirectory basis.

# Creates object file rules. Call with eval.
# $(1) list of .o files
# $(2) source type (CC or CXX)
# $(3) source suffix (cc or c)
# $(4) compiler flag name (CFLAGS or CXXFLAGS)
# $(5) source dir: _only_ if $(SRC). Leave blank for obj tree.
define add_object_rules
$(patsubst %.o,%.pie.o,$(1)): %.pie.o: $(5)%.$(3) %.o.depends
	$$(call auto_mkdir,$$@)
	$$(call OBJECT_PATTERN_implementation,$(2),\
          $$(basename $$@),$$($(4)) $$(CPPFLAGS) $$(OBJ_PIE_FLAG))

$(patsubst %.o,%.pic.o,$(1)): %.pic.o: $(5)%.$(3) %.o.depends
	$$(call auto_mkdir,$$@)
	$$(call OBJECT_PATTERN_implementation,$(2),\
          $$(basename $$@),$$($(4)) $$(CPPFLAGS) -fPIC)

# Placeholder for depends
$(patsubst %.o,%.o.depends,$(1)):
	$$(call auto_mkdir,$$@)
	$$(QUIET)touch "$$@"

$(1): %.o: %.pic.o %.pie.o
	$$(call auto_mkdir,$$@)
	$$(QUIET)touch "$$@"
endef

define OBJECT_PATTERN_implementation
  @$(ECHO) "$(1)		$(subst $(SRC)/,,$<) -> $(2).o"
  $(call auto_mkdir,$@)
  $(QUIET)$($(1)) -c -MD -MF $(2).d $(3) -o $(2).o $<
  $(QUIET)# Wrap all the deps in $$(wildcard) so a missing header
  $(QUIET)# won't cause weirdness.  First we remove newlines and \,
  $(QUIET)# then wrap it.
  $(QUIET)sed -i -e :j -e '$$!N;s|\\\s*\n| |;tj' \
    -e 's|^\(.*\s*:\s*\)\(.*\)$$|\1 $$\(wildcard \2\)|' $(2).d
endef

# Now actually register handlers for C(XX)_OBJECTS.
$(eval $(call add_object_rules,$(C_OBJECTS),CC,c,CFLAGS,$(SRC)/))
$(eval $(call add_object_rules,$(CXX_OBJECTS),CXX,cc,CXXFLAGS,$(SRC)/))

# Disable default pattern rules to help avoid leakage.
# These may already be handled by '-r', but let's keep it to be safe.
%: %.o ;
%.a: %.o ;
%.o: %.c ;
%.o: %.cc ;

# NOTE: A specific rule for archive objects is avoided because parallel
#       update of the archive causes build flakiness.
# Instead, just make the objects the prerequisites and use update_archive
# To use the foo.a(obj.o) functionality, targets would need to specify the
# explicit object they expect on the prerequisite line.

#
# Architecture detection and QEMU wrapping
#

HOST_ARCH ?= $(shell uname -m)
override ARCH := $(strip $(ARCH))
override HOST_ARCH := $(strip $(HOST_ARCH))
# emake will supply "x86" or "arm" for ARCH, but
# if uname -m runs and you get x86_64, then this subst
# will break.
ifeq ($(subst x86,i386,$(ARCH)),i386)
  QEMU_ARCH := $(subst x86,i386,$(ARCH))  # x86 -> i386
else ifeq ($(subst amd64,x86_64,$(ARCH)),x86_64)
  QEMU_ARCH := $(subst amd64,x86_64,$(ARCH))  # amd64 -> x86_64
else
  QEMU_ARCH = $(ARCH)
endif
override QEMU_ARCH := $(strip $(QEMU_ARCH))

# If we're cross-compiling, try to use qemu for running the tests.
ifneq ($(QEMU_ARCH),$(HOST_ARCH))
  ifeq ($(SYSROOT),)
    $(info SYSROOT not defined. qemu-based testing disabled)
  else
    # A SYSROOT is assumed for QEmu use.
    USE_QEMU ?= 1

    # Allow 64-bit hosts to run 32-bit without qemu.
    ifeq ($(HOST_ARCH),x86_64)
      ifeq ($(QEMU_ARCH),i386)
        USE_QEMU = 0
      endif
    endif
  endif
else
  USE_QEMU ?= 0
endif

# Normally we don't need to run as root or do bind mounts, so only
# enable it by default when we're using QEMU.
NEEDS_ROOT ?= $(USE_QEMU)
NEEDS_MOUNTS ?= $(USE_QEMU)

SYSROOT_OUT = $(OUT)
ifneq ($(SYSROOT),)
  SYSROOT_OUT = $(subst $(SYSROOT),,$(OUT))
else
  # Default to / when all the empty-sysroot logic is done.
  SYSROOT = /
endif

QEMU_NAME = qemu-$(QEMU_ARCH)
QEMU_PATH = /build/bin/$(QEMU_NAME)
QEMU_SYSROOT_PATH = $(SYSROOT)$(QEMU_PATH)
QEMU_SRC_PATH = /usr/bin/$(QEMU_NAME)
QEMU_BINFMT_PATH = /proc/sys/fs/binfmt_misc/$(QEMU_NAME)
QEMU_REGISTER_PATH = /proc/sys/fs/binfmt_misc/register

QEMU_MAGIC_arm = ":$(QEMU_NAME):M::\x7fELF\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x28\x00:\xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff\xfe\xff\xff\xff:/build/bin/qemu-arm:"


#
# Output full configuration at top level
#

# Don't show on clean
ifneq ($(MAKECMDGOALS),clean)
  $(info build configuration:)
  $(info - OUT=$(OUT))
  $(info - SRC=$(SRC))
  $(info - MODE=$(MODE))
  $(info - SPLITDEBUG=$(SPLITDEBUG))
  $(info - NOSTRIP=$(NOSTRIP))
  $(info - VALGRIND=$(VALGRIND))
  $(info - COLOR=$(COLOR))
  $(info - CXXEXCEPTIONS=$(CXXEXCEPTIONS))
  $(info - ARCH=$(ARCH))
  $(info - QEMU_ARCH=$(QEMU_ARCH))
  $(info - USE_QEMU=$(USE_QEMU))
  $(info - NEEDS_ROOT=$(NEEDS_ROOT))
  $(info - NEEDS_MOUNTS=$(NEEDS_MOUNTS))
  $(info - SYSROOT=$(SYSROOT))
  $(info )
endif

#
# Standard targets with detection for when they are improperly configured.
#

# all does not include tests by default
all:
	$(QUIET)(test -z "$^" && \
	$(ECHO) "You must add your targets as 'all' prerequisites") || true
	$(QUIET)test -n "$^"

# Builds and runs tests for the target arch
# Run them in parallel
# After the test have completed, if profiling, run coverage analysis
tests:
ifeq ($(MODE),profiling)
	@$(ECHO) "COVERAGE [$(COLOR_YELLOW)STARTED$(COLOR_RESET)]"
	$(QUIET)FILES="";						\
		for GCNO in `find . -name "*.gcno"`; do			\
			GCDA="$${GCNO%.gcno}.gcda";			\
			if [ -e $${GCDA} ]; then			\
				FILES="$${FILES} $${GCDA}";		\
			fi						\
		done;							\
		if [ -n "$${FILES}" ]; then				\
			gcov -l $${FILES};				\
			lcov --capture --directory .			\
				--output-file=lcov-coverage.info;	\
			genhtml lcov-coverage.info			\
				--output-directory lcov-html;		\
		fi
	@$(ECHO) "COVERAGE [$(COLOR_YELLOW)FINISHED$(COLOR_RESET)]"
endif
.PHONY: tests

qemu_chroot_install:
ifeq ($(USE_QEMU),1)
	$(QUIET)$(ECHO) "QEMU   Preparing $(QEMU_NAME)"
	@# Copying strategy
	@# Compare /usr/bin/qemu inode to /build/$board/build/bin/qemu, if different
	@# hard link to a temporary file, then rename temp to target. This should
	@# ensure that once $QEMU_SYSROOT_PATH exists it will always exist, regardless
	@# of simultaneous test setups.
	$(QUIET)if [[ ! -e $(QEMU_SYSROOT_PATH) || \
	    `stat -c %i $(QEMU_SRC_PATH)` != `stat -c %i $(QEMU_SYSROOT_PATH)` \
	    ]]; then \
	  $(ROOT_CMD) ln -Tf $(QEMU_SRC_PATH) $(QEMU_SYSROOT_PATH).$$$$; \
	  $(ROOT_CMD) mv -Tf $(QEMU_SYSROOT_PATH).$$$$ $(QEMU_SYSROOT_PATH); \
	fi

	@# Prep the binfmt handler. First mount if needed, then unregister any bad
	@# mappings and then register our mapping.
	@# There may still be some race conditions here where one script de-registers
	@# and another script starts executing before it gets re-registered, however
	@# it should be rare.
	-$(QUIET)[[ -e $(QEMU_REGISTER_PATH) ]] || \
	  $(ROOT_CMD) mount binfmt_misc -t binfmt_misc \
	    /proc/sys/fs/binfmt_misc

	-$(QUIET)if [[ -e $(QEMU_BINFMT_PATH) && \
	      `awk '$$1 == "interpreter" {print $$NF}' $(QEMU_BINFMT_PATH)` != \
	      "$(QEMU_PATH)" ]]; then \
	  echo -1 | $(ROOT_CMD) tee $(QEMU_BINFMT_PATH) >/dev/null; \
	fi

	-$(if $(QEMU_MAGIC_$(ARCH)),$(QUIET)[[ -e $(QEMU_BINFMT_PATH) ]] || \
	  echo $(QEMU_MAGIC_$(ARCH)) | $(ROOT_CMD) tee $(QEMU_REGISTER_PATH) \
	    >/dev/null)
endif
.PHONY: qemu_clean qemu_chroot_install

# TODO(wad) Move to -L $(SYSROOT) and fakechroot when qemu-user
#           doesn't hang traversing /proc from SYSROOT.
SUDO_CMD = sudo
UNSHARE_CMD = unshare
QEMU_CMD =
ROOT_CMD = $(if $(filter 1,$(NEEDS_ROOT)),$(SUDO_CMD) , )
MOUNT_CMD = $(if $(filter 1,$(NEEDS_MOUNTS)),$(ROOT_CMD) mount, \#)
UMOUNT_CMD = $(if $(filter 1,$(NEEDS_MOUNTS)),$(ROOT_CMD) umount, \#)
QEMU_LDPATH = $(SYSROOT_LDPATH):/lib64:/lib:/usr/lib64:/usr/lib
ROOT_CMD_LDPATH = $(SYSROOT_LDPATH):$(SYSROOT)/lib64:
ROOT_CMD_LDPATH := $(ROOT_CMD_LDPATH):$(SYSROOT)/lib:$(SYSROOT)/usr/lib64:
ROOT_CMD_LDPATH := $(ROOT_CMD_LDPATH):$(SYSROOT)/usr/lib
ifeq ($(USE_QEMU),1)
  export QEMU_CMD = \
   $(SUDO_CMD) chroot $(SYSROOT) $(QEMU_PATH) \
   -drop-ld-preload \
   -E LD_LIBRARY_PATH="$(QEMU_LDPATH):$(patsubst $(OUT),,$(LD_DIRS))" \
   -E HOME="$(HOME)" -E SRC="$(SRC)" --
  # USE_QEMU conditional function
  define if_qemu
    $(1)
  endef
else
  ROOT_CMD = $(if $(filter 1,$(NEEDS_ROOT)),sudo, ) \
    LD_LIBRARY_PATH="$(ROOT_CMD_LDPATH):$(LD_DIRS)"
  define if_qemu
    $(2)
  endef
endif

VALGRIND_CMD =
ifeq ($(VALGRIND),1)
  VALGRIND_CMD = /usr/bin/valgrind --tool=memcheck $(VALGRIND_ARGS) --
endif

define TEST_implementation
  $(QUIET)$(call TEST_setup)
  $(QUIET)$(call TEST_run)
  $(QUIET)$(call TEST_teardown)
  $(QUIET)exit $$(cat $(OUT)$(TARGET_OR_MEMBER).status.test)
endef

define TEST_setup
  @$(ECHO) -n "TEST		$(TARGET_OR_MEMBER) "
  @$(ECHO) "[$(COLOR_YELLOW)SETUP$(COLOR_RESET)]"
  $(QUIET)# Setup a target-specific results file
  $(QUIET)(echo > $(OUT)$(TARGET_OR_MEMBER).setup.test)
  $(QUIET)(echo 1 > $(OUT)$(TARGET_OR_MEMBER).status.test)
  $(QUIET)(echo > $(OUT)$(TARGET_OR_MEMBER).cleanup.test)
  $(QUIET)# No setup if we are not using QEMU
  $(QUIET)# TODO(wad) this is racy until we use a vfs namespace
  $(call if_qemu,\
    $(QUIET)(echo "mkdir -p '$(SYSROOT)/proc' '$(SYSROOT)/dev' \
                            '$(SYSROOT)/mnt/host/source'" \
             >> "$(OUT)$(TARGET_OR_MEMBER).setup.test"))
  $(call if_qemu,\
    $(QUIET)(echo "$(MOUNT_CMD) --bind /mnt/host/source \
             '$(SYSROOT)/mnt/host/source'" \
             >> "$(OUT)$(TARGET_OR_MEMBER).setup.test"))
  $(call if_qemu,\
    $(QUIET)(echo "$(MOUNT_CMD) --bind /proc '$(SYSROOT)/proc'" \
             >> "$(OUT)$(TARGET_OR_MEMBER).setup.test"))
  $(call if_qemu,\
    $(QUIET)(echo "$(MOUNT_CMD) --bind /dev '$(SYSROOT)/dev'" \
             >> "$(OUT)$(TARGET_OR_MEMBER).setup.test"))
endef

define TEST_teardown
  @$(ECHO) -n "TEST		$(TARGET_OR_MEMBER) "
  @$(ECHO) "[$(COLOR_YELLOW)TEARDOWN$(COLOR_RESET)]"
  $(call if_qemu, $(QUIET)$(SHELL) "$(OUT)$(TARGET_OR_MEMBER).cleanup.test")
endef

# Use GTEST_ARGS.[arch] if defined.
override GTEST_ARGS.real = \
 $(call if_qemu,$(GTEST_ARGS.qemu.$(QEMU_ARCH)),$(GTEST_ARGS.host.$(HOST_ARCH)))

define TEST_run
  @$(ECHO) -n "TEST		$(TARGET_OR_MEMBER) "
  @$(ECHO) "[$(COLOR_GREEN)RUN$(COLOR_RESET)]"
  $(QUIET)(echo 1 > "$(OUT)$(TARGET_OR_MEMBER).status.test")
  $(QUIET)(echo $(ROOT_CMD) SRC="$(SRC)" $(QEMU_CMD) $(VALGRIND_CMD) \
    "$(strip $(call if_qemu, $(SYSROOT_OUT),$(OUT))$(TARGET_OR_MEMBER))" \
      $(if $(filter-out 0,$(words $(GTEST_ARGS.real))),$(GTEST_ARGS.real),\
           $(GTEST_ARGS)) >> "$(OUT)$(TARGET_OR_MEMBER).setup.test")
  -$(QUIET)$(call if_qemu,$(SUDO_CMD) $(UNSHARE_CMD) -m) $(SHELL) \
      $(OUT)$(TARGET_OR_MEMBER).setup.test \
  && echo 0 > "$(OUT)$(TARGET_OR_MEMBER).status.test"
endef

# Recursive list reversal so that we get RMDIR_ON_CLEAN in reverse order.
define reverse
$(if $(1),$(call reverse,$(wordlist 2,$(words $(1)),$(1)))) $(firstword $(1))
endef

clean: qemu_clean
clean: CLEAN($(OUT)*.d) CLEAN($(OUT)*.o) CLEAN($(OUT)*.debug)
clean: CLEAN($(OUT)*.test) CLEAN($(OUT)*.depends)
clean: CLEAN($(OUT)*.gcno) CLEAN($(OUT)*.gcda) CLEAN($(OUT)*.gcov)
clean: CLEAN($(OUT)lcov-coverage.info) CLEAN($(OUT)lcov-html)

clean:
	$(QUIET)# Always delete the containing directory last.
	$(call silent_rmdir,$(OUT))

FORCE: ;
# Empty rule for use when no special targets are needed, like large_tests
NONE:

.PHONY: clean NONE valgrind NONE
.DEFAULT_GOAL  :=  all
# Don't let make blow away "intermediates"
.PRECIOUS: %.pic.o %.pie.o %.a %.pic.a %.pie.a %.test

# Start accruing build info
OUT_DIRS = $(OUT)
LD_DIRS = $(OUT)
SRC_DIRS = $(SRC)

include $(wildcard $(OUT)*.d)
SUBMODULE_DIRS = $(wildcard $(SRC)/*/module.mk)
include $(SUBMODULE_DIRS)


else  ## In duplicate inclusions of common.mk

# Get the current inclusion directory without a trailing slash
MODULE := $(patsubst %/,%, \
           $(dir $(lastword $(filter-out %common.mk,$(MAKEFILE_LIST)))))
MODULE := $(subst $(SRC)/,,$(MODULE))
MODULE_NAME := $(subst /,_,$(MODULE))
#VPATH := $(MODULE):$(VPATH)


# Depth first
$(eval OUT_DIRS += $(OUT)$(MODULE))
$(eval SRC_DIRS += $(OUT)$(MODULE))
$(eval LD_DIRS := $(LD_DIRS):$(OUT)$(MODULE))

# Add the defaults from this dir to rm_clean
clean: CLEAN($(OUT)$(MODULE)/*.d) CLEAN($(OUT)$(MODULE)/*.o)
clean: CLEAN($(OUT)$(MODULE)/*.debug) CLEAN($(OUT)$(MODULE)/*.test)
clean: CLEAN($(OUT)$(MODULE)/*.depends)
clean: CLEAN($(OUT)$(MODULE)/*.gcno) CLEAN($(OUT)$(MODULE)/*.gcda)
clean: CLEAN($(OUT)$(MODULE)/*.gcov) CLEAN($(OUT)lcov-coverage.info)
clean: CLEAN($(OUT)lcov-html)

$(info + submodule: $(MODULE_NAME))
# We must eval otherwise they may be dropped.
MODULE_C_OBJECTS = $(patsubst $(SRC)/$(MODULE)/%.c,$(MODULE)/%.o,\
  $(wildcard $(SRC)/$(MODULE)/*.c))
$(eval $(MODULE_NAME)_C_OBJECTS ?= $(MODULE_C_OBJECTS))
MODULE_CXX_OBJECTS = $(patsubst $(SRC)/$(MODULE)/%.cc,$(MODULE)/%.o,\
  $(wildcard $(SRC)/$(MODULE)/*.cc))
$(eval $(MODULE_NAME)_CXX_OBJECTS ?= $(MODULE_CXX_OBJECTS))

# Note, $(MODULE) is implicit in the path to the %.c.
# See $(C_OBJECTS) for more details.
# Register rules for the module objects.
$(eval $(call add_object_rules,$(MODULE_C_OBJECTS),CC,c,CFLAGS,$(SRC)/))
$(eval $(call add_object_rules,$(MODULE_CXX_OBJECTS),CXX,cc,CXXFLAGS,$(SRC)/))

# Continue recursive inclusion of module.mk files
SUBMODULE_DIRS = $(wildcard $(SRC)/$(MODULE)/*/module.mk)
include $(wildcard $(OUT)$(MODULE)/*.d)
include $(SUBMODULE_DIRS)

endif
endif  ## pass-to-subcall wrapper for relocating the call directory
