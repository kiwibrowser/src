# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

##############################################################################
# Helper script for NaCl toolchain development workflow.
#
# Buildbots:
# - Sync needed sources at pinned revision and build newlib-based toolchain:
#     make buildbot-build-with-newlib TOOLCHAINLOC=<where-to-install-the-toolchain>
#  or
#     make buildbot-build-with-glibc TOOLCHAINLOC=<where-to-install-the-toolchain>
#
# Development:
# - Sync all sources at pinned revision:
#     make sync-pinned
# - Sync all sources at most recent revision:
#     make sync
# - Build newlib-based toolchain from current sources:
#     make build-with-newlib TOOLCHAINLOC=<where-to-install-the-toolchain>
# - Build glibc-based toolchain from current sources:
#     make build-with-glibc TOOLCHAINLOC=<where-to-install-the-toolchain>
#
##############################################################################

default: build-with-glibc

# Delete the target file if the recipe fails after beginning to change the file
# http://www.gnu.org/software/make/manual/make.html#Errors (Errors in Recipes)
.DELETE_ON_ERROR: ;

THISMAKEFILE := $(lastword $(MAKEFILE_LIST))

SHELL = /bin/bash

GIT_BASE_URL = https://chromium.googlesource.com/native_client

CROSSARCH = x86_64-nacl
TOOLCHAINLOC ?= out
SDKLOC ?= $(abspath $(TOOLCHAINLOC))
TOOLCHAINNAME ?= nacl-sdk
SDKNAME ?= $(TOOLCHAINNAME)
SDKROOT ?= $(SDKLOC)/$(SDKNAME)
DESTDIR ?=

# We can't use CFLAGS and LDFLAGS since they are passed to sub-makes and
# those override configure parameters.
USER_CFLAGS = -O2 -g
USER_LDFLAGS = -s

# By default all toolchain executables are x86-32 executables, use
# HOST_TOOLCHAIN_BITS=64 to make them x86-64 executables.
HOST_TOOLCHAIN_BITS = 32

# If CANNED_REVISION is "no" then it's "from git build": Makefile uses file
# named "REVISIONS" to pull infomation from git and builds that.
# If CANNED_REVISION is set to some revision then Makefile will try to download
# sources from storage.googleapis.com and use them.
# You can also set CANNED_REVISION to "yes" if you want to just build sources
# already unpacked and ready to be built from SRC subdirectory.
CANNED_REVISION = no
ifeq ($(CANNED_REVISION), no)
include REVISIONS
endif
export NACL_FAKE_SONAME = $(shell commit=$(NACL_GLIBC_COMMIT);echo $${commit:0:8})

# Recent version of makeinfo (such as the one in ubuntu trusty) fail to
# compile the info files in our version of gcc. Setting this variable to
# no stops gcc from detecteing a modern version of makeinfo and effectivly
# stops it from building the info files.
export gcc_cv_prog_makeinfo_modern=no

# Toplevel installation directory.
# MUST be an absolute pathname, for configure --prefix=$(PREFIX)
PREFIX = $(abspath $(SDKROOT))

# Convert CROSSARCH (nacl or nacl64) to (32 or 64).
BITSPLATFORM = 64

LINUX_HEADERS = "$(abspath $(dir $(THISMAKEFILE)))/SRC/linux-headers-for-nacl/include"
HPREFIX = SRC/newlib/newlib/libc/sys/nacl

# No 'uname -o' on OSX
ifeq ($(shell uname -s), Darwin)
  PLATFORM = mac
  # Ensure that the resulting toolchain works on Mac OS X 10.5, since that
  # is not the default in newer versions of Xcode.
  export MACOSX_DEPLOYMENT_TARGET = 10.5
else
ifeq ($(shell uname -o), Cygwin)
  PLATFORM = win
else
ifeq ($(shell uname -o), Msys)
  PLATFORM = win
else
  PLATFORM = linux
endif
endif
endif

# SRCDIR should contain tarball for gcc-extras: gmp mpfr.
# You can skip use of these tarball if you make SRCDIR value empty.  In this
# case system-wide libraries will be used (they should be available for such
# use, obviously - and this is not always the case: for example they are not
# available on MacOS, on 64bit linux you generally can find 64bit versions of
# them, but not 32bit versions, etc).
SRCDIR = ../../../third_party

ifeq ($(PLATFORM), win)
  # Ugh, Cygwin and spaces in paths don't work well.
  # I'm explicitly coding the path.
  BUILDPATH = $(DESTDIR)$(PREFIX)/bin:/usr/local/bin:/usr/bin:/bin
  SCONS ?= scons.bat
  GIT ?= $(shell python ../../build/find_depot_tools.py)/git.bat
  REVISION ?= $(shell $(GIT) rev-parse HEAD | tr -d $$"\r")
  PREFIX_NATIVE = $(shell cygpath -m $(PREFIX))
  CREATE_REDIRECTORS = ./create_redirectors_cygwin.sh
else
  BUILDPATH = $(DESTDIR)$(PREFIX)/bin:$(PATH)
  SCONS ?= scons
  GIT ?= git
  REVISION ?= $(shell git rev-parse HEAD)
  PREFIX_NATIVE = $(DESTDIR)$(PREFIX)
  CREATE_REDIRECTORS = ./create_redirectors.sh
endif

REVISION_DATE ?= $(shell $(GIT) show --no-patch --format=%cd --date=short $(REVISION) | sed -e s'+\(....\)-\(..\)-\(..\).*+\1\2\3+')

##################################################################
#  The version numbers for the tools we will be building.
##################################################################
GMP_VERSION = 5.0.2
MPFR_VERSION = 3.0.1
define GCC_EXTRAS
gmp: $(GMP_VERSION), \
mpfr: $(MPFR_VERSION)
endef
BINUTILS_VERSION = 2.24.0
NACL_BINUTILS_GIT_BASE = 237df3fa4a1d939e6fd1af0c3e5029a25a137310
GCC_VERSION = 4.4.3
NACL_GCC_GIT_BASE = 4e0ae761f59baae95282ab07efa9b831ac524642
NEWLIB_VERSION = 2.1.0
NACL_NEWLIB_GIT_BASE = e14046f93c76ef701d8ad133d0ea2b96d3c1b578
GLIBC_VERSION = 2.9
NACL_GLIBC_GIT_BASE = 5c46008d0874c9b9d5f5f201a10e975d1fe84787

##################################################################
# Get or update the sources.
##################################################################

SRC:
	mkdir SRC

git-sources := binutils gcc glibc linux-headers-for-nacl newlib

nacl-name = $(patsubst nacl-linux-headers-for-nacl,linux-headers-for-nacl,nacl-$*)

all-git-sources = $(git-sources:%=SRC/%)
$(all-git-sources): SRC/%: | SRC
ifeq ($(CANNED_REVISION), no)
	git clone $(GIT_BASE_URL)/$(nacl-name).git SRC/$*
else
	./download_SRC.sh $(CANNED_REVISION)
endif

all-fetched-git-sources = $(git-sources:%=fetched-src-%)
.PHONY: $(all-fetched-git-sources)
$(all-fetched-git-sources): fetched-src-%: | SRC/%
	cd SRC/$* && git fetch

# Note that we can not change names of variables in REVISIONS files since they
# are used in other places, not just in this Makefile.  Small amount of sh-fu
# will convert linux-headers-for-nacl to LINUX_HEADERS_FOR_NACL_COMMIT.
all-pinned-git-sources = $(git-sources:%=pinned-src-%)
.PHONY: $(all-pinned-git-sources)
$(all-pinned-git-sources): pinned-src-%: fetched-src-%
	cd SRC/$* && git checkout \
	    "$($(shell echo $(nacl-name)| tr '[:lower:]-' '[:upper:]_')_COMMIT)"

all-latest-git-sources = $(git-sources:%=latest-src-%)
.PHONY: $(all-latest-git-sources)
$(all-latest-git-sources): latest-src-%: fetched-src-%
	./update_to_latest.sh SRC/$*

.PHONY: sync-pinned
sync-pinned: | $(all-pinned-git-sources)

.PHONY: sync
sync: | $(all-latest-git-sources)

# Requisites: libraries needed for GCC, but without any nacl-specific patches.
# They are only updated when pre-requisites version changes.
.PHONY: gcc-extras
gcc-extras:
	if [[ "$$(cat BUILD/.gcc-extras-version)" != "$(GCC_EXTRAS)" ]]; then \
	    rm -rf BUILD/.gcc-extra-* SRC/.gcc-extra-* && \
	    $(MAKE) -f $(THISMAKEFILE) install-gcc-extras && \
	    echo -n "$(GCC_EXTRAS)" > BUILD/.gcc-extras-version; \
	fi

gcc-extras := gmp mpfr

gcc_extra_version = $($(shell echo $*| tr '[:lower:]-' '[:upper:]_')_VERSION)

all-src-gcc-extras = $(gcc-extras:%=SRC/.gcc-extra-%)
$(all-src-gcc-extras): SRC/.gcc-extra-%: | SRC
	rm -rf SRC/.gcc-extra-$*
	cd SRC && tar xpf $(SRCDIR)/$*/$*-$(gcc_extra_version).tar.*
	mv SRC/$*-$(gcc_extra_version) SRC/.gcc-extra-$*

# All Macs need Core2 assembly and --enable-fat is broken with stock MacOS gcc.
ifneq ($(PLATFORM), mac)
gmp_use_fat_binary = --enable-fat
else
gmp_use_fat_binary =
endif

define gcc-extra-configure
rm -rf $@
mkdir -p $@
BUILD=$$PWD/BUILD && cd $@ && \
  ../../SRC/.gcc-extra-$(@:BUILD/.gcc-extra-build-%=%)/configure \
  CC="$(GCC_CC)" \
  CXX="$(GCC_CXX)" \
  CFLAGS="$(CFLAGS)" \
  CXXFLAGS="$(CXXFLAGS)" \
  CPPFLAGS="-fexceptions $(CPPFLAGS)" \
  --prefix=$$BUILD/.gcc-extra-install-$(@:BUILD/.gcc-extra-build-%=%) \
  --disable-shared
endef

BUILD/.gcc-extra-build-gmp: | BUILD SRC/.gcc-extra-gmp
	$(gcc-extra-configure) \
	    --enable-cxx $(gmp_use_fat_binary) \
	    ABI="$(HOST_TOOLCHAIN_BITS)"

BUILD/.gcc-extra-build-mpfr: | SRC/.gcc-extra-mpfr BUILD/.gcc-extra-install-gmp
	$(gcc-extra-configure) --with-gmp=$$BUILD/.gcc-extra-install-gmp

all-install-gcc-extras = $(gcc-extras:%=BUILD/.gcc-extra-install-%)
$(all-install-gcc-extras): BUILD/.gcc-extra-install-%: | SRC \
                                                        BUILD/.gcc-extra-build-%
	if mkdir BUILD/.gcc-extra-install-$*; then \
	    mkdir BUILD/.gcc-extra-install-$*/include && \
	    mkdir BUILD/.gcc-extra-install-$*/lib && \
	    ln -s lib BUILD/.gcc-extra-install-$*/lib64 && \
	    ln -s lib BUILD/.gcc-extra-install-$*/lib32; \
	fi
	cd BUILD/.gcc-extra-build-$* && $(MAKE) install

install-gcc-extras: | BUILD SRC $(all-install-gcc-extras)

# Create the build directories for compiled binaries.
BUILD:
	mkdir BUILD

##################################################################
# Create the SDK output directories.
##################################################################

# Create directory structure for the toolchain.
# On win, non-cygwin binaries do not follow cygwin symlinks. In case when we
# need multiple names for the directory, the name that might be used by win
# tools goes to the real directory and other names go to symlinks.
# Installers create real directories when they do not find an existing one
# to use. To prevent installers from creating a real directory instead of what
# should be a symlink we create everything in advance.
sdkdirs:
	echo "Creating the SDK tree at $(DESTDIR)$(PREFIX)"
	# Create installation directory for 64-bit libraries
	# See http://code.google.com/p/nativeclient/issues/detail?id=1975
	install -m 755 -d "$(DESTDIR)$(PREFIX)/$(CROSSARCH)/lib"
	# Create alias for libgcc_s.so
	# TODO: fix MULTILIB_OSDIRNAMES in gcc/config/i386/t-nacl64
	#       and get rid of this!
	ln -sfn lib $(DESTDIR)$(PREFIX)/$(CROSSARCH)/lib64
	# Create installation directory for 32-bit libraries
	install -m 755 -d "$(DESTDIR)$(PREFIX)/$(CROSSARCH)/lib32"
	# Create alias for newlib
	# Newlib uses "gcc -print-multi-lib" to determine multilib subdirectory
	# names and installs under $prefix accordingly. This seems confusing,
	# as "-print-multi-lib" uses MULTILIB_DIRNAMES, which is for libgcc,
	# while for newlib it looks better to use MULTILIB_OSDIRNAMES, which is
	# for system libraries. In our case these are "/lib" and "/lib/32" vs.
	# "/lib" and "/lib32" respectively. As a result, 32-bit newlib is
	# installed under "/lib/32" but searched under "/lib32".
	# We fix this by making "/lib/32" an alias for "/lib32".
	# TODO: sounds odd - probably my understanding is wrong?
	#       Go ask someone smart...
	ln -sfn ../lib32 $(DESTDIR)$(PREFIX)/$(CROSSARCH)/lib/32

##################################################################
# binutils:
# Builds the cross assembler, linker, archiver, etc.
##################################################################
BUILD/stamp-$(CROSSARCH)-binutils: | SRC/binutils BUILD
	rm -rf BUILD/build-binutils-$(CROSSARCH)
	mkdir BUILD/build-binutils-$(CROSSARCH)
	# We'd like to build binutils with -Werror, but there are a
	# number of warnings in the Mac version of GCC that prevent
	# us from building with -Werror today.
	cd BUILD/build-binutils-$(CROSSARCH) && \
	  CC="$(GCC_CC)" \
	  CFLAGS="$(USER_CFLAGS)" \
	  LDFLAGS="$(USER_LDFLAGS)" \
	  ../../SRC/binutils/configure \
	    --prefix=$(PREFIX) \
	    --target=$(CROSSARCH) \
	    --with-sysroot=$(PREFIX)/$(CROSSARCH) \
	    --disable-werror --enable-deterministic-archives --without-zlib
	$(MAKE) -C BUILD/build-binutils-$(CROSSARCH) all
	$(MAKE) -C BUILD/build-binutils-$(CROSSARCH) DESTDIR=$(DESTDIR) install
	touch $@

.PHONY: binutils
binutils: BUILD/stamp-$(CROSSARCH)-binutils

##################################################################
# pregcc:
# Builds the cross gcc used to build the libraries.
##################################################################

GCC_SRC_DIR = $(abspath SRC/gcc)

GMP_DIR = $(abspath BUILD/.gcc-extra-install-gmp)
MPFR_DIR = $(abspath BUILD/.gcc-extra-install-mpfr)

# For Linux we want to make sure we don't dynamically link in libstdc++
# because it ties our binaries to a host GCC version that other places
# the toolchain gets installed might not match.
ifeq ($(PLATFORM),linux)
lstdc++ = -Wl,-Bstatic -lstdc++ -Wl,-Bdynamic
else
lstdc++ = -lstdc++
endif

ifneq ($(SRCDIR),)
GCC_EXTRAS_FLAGS = \
    --with-gmp=$(GMP_DIR) \
    --with-mpfr=$(MPFR_DIR) \
    --with-host-libstdcxx="-lpwl $(lstdc++) -lm"
else
GCC_EXTRAS_FLAGS = \
    --with-gmp \
    --with-mpfr
endif

GCC_CONFIGURE_FLAGS = \
    --disable-decimal-float \
    --disable-libgomp \
    --disable-libmudflap \
    --disable-libssp \
    --disable-libstdcxx-pch \
    --target=$(CROSSARCH) \
    --enable-linker-build-id \
    $(GCC_EXTRAS_FLAGS)

ifdef MULTILIB
ifeq ($(MULTILIB),no)
GCC_CONFIGURE_FLAGS += --disable-multilib
else
$(error MULTILIB: Bad value)
endif
endif

ifeq ($(PLATFORM), mac)
GCC_CC = clang -m$(HOST_TOOLCHAIN_BITS) -fgnu89-inline
GCC_CXX = clang++ -m$(HOST_TOOLCHAIN_BITS)
else
GCC_CC = gcc -m$(HOST_TOOLCHAIN_BITS)
GCC_CXX = g++ -m$(HOST_TOOLCHAIN_BITS)
endif

GCC_DEFINES = \
    -Dinhibit_libc \
    -D__gthr_posix_h


GCC_CFLAGS_FOR_TARGET-nolibc =
GCC_CONFIGURE_FLAGS-nolibc = --disable-shared \
			     --disable-threads \
			     --enable-languages="c" \
			     --without-headers

GCC_CFLAGS_FOR_TARGET-newlib = -I$(HEADERS_FOR_BUILD)

GCC_CONFIGURE_FLAGS-newlib = --disable-shared \
			     --enable-languages="c,c++,objc" \
			     --enable-threads=nacl \
			     --enable-tls \
			     --with-newlib

GCC_CFLAGS_FOR_TARGET-glibc =
GCC_CONFIGURE_FLAGS-glibc = --enable-shared \
			    --enable-languages="c,c++,objc,obj-c++,fortran" \
			    --enable-threads=posix \
			    --enable-tls


BUILD/stamp-$(CROSSARCH)-pregcc: | SRC/gcc BUILD
ifneq ($(SRCDIR),)
	$(MAKE) -f $(THISMAKEFILE) gcc-extras
endif
	rm -rf BUILD/build-pregcc-$(CROSSARCH)
	mkdir BUILD/build-pregcc-$(CROSSARCH)
	cd BUILD/build-pregcc-$(CROSSARCH) && \
	PATH=$(BUILDPATH) \
	$(GCC_SRC_DIR)/configure \
	    CC="$(GCC_CC)" \
	    CFLAGS="$(USER_CFLAGS) $(GCC_DEFINES)" \
	    LDFLAGS="$(USER_LDFLAGS)" \
	    CFLAGS_FOR_TARGET="-O2 -g $(GCC_CFLAGS_FOR_TARGET-nolibc)" \
	    CXXFLAGS_FOR_TARGET="-O2 -g $(GCC_CFLAGS_FOR_TARGET-nolibc)" \
	    --prefix=$(PREFIX) \
	    $(GCC_CONFIGURE_FLAGS) \
	    $(GCC_CONFIGURE_FLAGS-nolibc)
	PATH=$(BUILDPATH) $(MAKE) \
	    -C BUILD/build-pregcc-$(CROSSARCH) \
	    all-gcc all-target-libgcc
	PATH=$(BUILDPATH) $(MAKE) \
	    -C BUILD/build-pregcc-$(CROSSARCH) \
	    DESTDIR=$(DESTDIR) \
	    install-gcc install-target-libgcc
	cp $(DESTDIR)$(PREFIX)/lib/gcc/$(CROSSARCH)/$(GCC_VERSION)/libgcc.a \
		$(DESTDIR)$(PREFIX)/lib/gcc/$(CROSSARCH)/$(GCC_VERSION)/libgcc_eh.a
	cp $(DESTDIR)$(PREFIX)/lib/gcc/$(CROSSARCH)/$(GCC_VERSION)/32/libgcc.a \
		$(DESTDIR)$(PREFIX)/lib/gcc/$(CROSSARCH)/$(GCC_VERSION)/32/libgcc_eh.a |\
	true
	touch $@

.PHONY: pregcc
pregcc: BUILD/stamp-$(CROSSARCH)-pregcc


##################################################################
# pregcc-standalone:
# Builds the cross gcc used to build glibc.
# TODO(eaeltsin): now works for Linux only, enable for Windows/Mac
# TODO(eaeltsin): get rid of pregcc in favor of pregcc-standalone
##################################################################

# Toplevel installation directory for pregcc.
# MUST be an absolute pathname, for configure --prefix=$(PREGCC_PREFIX)
# Pregcc is installed separately so that it is not overwritten with full gcc.
# Pregcc is needed for rebuilding glibc, while full gcc can't do that because
# of its incompatible libgcc.
PREGCC_PREFIX = $(abspath BUILD/install-pregcc-$(CROSSARCH))

# Build directory for pregcc.
PREGCC_BUILD_DIR = BUILD/build-pregcc-$(CROSSARCH)

# Build pregcc:
# create links to binutils:
#   Alternate approaches are to make PATH point to nacl binutils or to use
#   pregcc with -B option. Both seem unreliable, as after full gcc is installed
#   the search path will include full gcc stuff that should not be picked.
# make install:
#   DESTDIR should be ignored at this step.
BUILD/stamp-$(CROSSARCH)-pregcc-standalone: \
  BUILD/stamp-$(CROSSARCH)-binutils | SRC/gcc BUILD
ifneq ($(SRCDIR),)
	$(MAKE) -f $(THISMAKEFILE) gcc-extras
endif
	rm -rf $(PREGCC_PREFIX)
	mkdir -p $(PREGCC_PREFIX)/$(CROSSARCH)/bin
	for f in '$(DESTDIR)$(PREFIX)/$(CROSSARCH)/bin/*'; do \
	    ln -s $$f $(PREGCC_PREFIX)/$(CROSSARCH)/bin; \
	    done
	rm -rf $(PREGCC_BUILD_DIR)
	mkdir $(PREGCC_BUILD_DIR)
	cd $(PREGCC_BUILD_DIR) && \
	PATH=$(BUILDPATH) \
	$(GCC_SRC_DIR)/configure \
	    CC="$(GCC_CC)" \
	    CFLAGS="$(USER_CFLAGS) $(GCC_DEFINES)" \
	    LDFLAGS="$(USER_LDFLAGS)" \
	    CFLAGS_FOR_TARGET="-O2 -g $(GCC_CFLAGS_FOR_TARGET-nolibc)" \
	    CXXFLAGS_FOR_TARGET="-O2 -g $(GCC_CFLAGS_FOR_TARGET-nolibc)" \
	    --prefix=$(PREGCC_PREFIX) \
	    $(GCC_CONFIGURE_FLAGS) \
	    $(GCC_CONFIGURE_FLAGS-nolibc)
	PATH=$(BUILDPATH) $(MAKE) \
	    -C $(PREGCC_BUILD_DIR) \
	    all-gcc \
	    all-target-libgcc \
	    DESTDIR=
	PATH=$(BUILDPATH) $(MAKE) \
	    -C $(PREGCC_BUILD_DIR) \
	    install-gcc \
	    install-target-libgcc \
	    DESTDIR=
	touch $@


##################################################################
# newlib:
# Builds the bare-bones library used by NativeClient applications.
# NOTE: removes the default pthread.h to enable correct install
# by the Native Client threads package build.
##################################################################

NEWLIB_CFLAGS = -O2

BUILD/stamp-$(CROSSARCH)-newlib: | SRC/newlib BUILD newlib-libc-script
	rm -rf BUILD/build-newlib-$(CROSSARCH)
	mkdir BUILD/build-newlib-$(CROSSARCH)
	PATH=$(BUILDPATH) && export PATH && \
	  cd BUILD/build-newlib-$(CROSSARCH) && \
	  ../../SRC/newlib/configure \
		      --disable-libgloss \
		      --enable-newlib-iconv \
		      --enable-newlib-iconv-from-encodings=UTF-8,UTF-16LE,UCS-4LE,UTF-16,UCS-4 \
		      --enable-newlib-iconv-to-encodings=UTF-8,UTF-16LE,UCS-4LE,UTF-16,UCS-4 \
		      --enable-newlib-io-long-long \
		      --enable-newlib-io-long-double \
		      --enable-newlib-io-c99-formats \
		      --enable-newlib-mb \
	    --prefix=$(PREFIX) \
	    CFLAGS="$(USER_CFLAGS)" \
	    CFLAGS_FOR_TARGET='$(NEWLIB_CFLAGS)' \
	    CXXFLAGS_FOR_TARGET='$(NEWLIB_CFLAGS)' \
	    --target=$(CROSSARCH) && \
	  $(MAKE) && \
	  $(MAKE) DESTDIR=$(DESTDIR) install
ifeq ($(CANNED_REVISION), no)
# The buildbot script that creates the source tarball adds all the headers to
# the tarball itself (see buildbot/buildbot_linux-glibc-makefile.sh)
	cp -f $(addprefix ../src/untrusted/pthread/, pthread.h semaphore.h) \
	  $(DESTDIR)$(PREFIX)/$(CROSSARCH)/include
endif
	for bits in 32 64; do \
	  mv $(DESTDIR)$(PREFIX)/$(CROSSARCH)/lib$$bits/libc.a \
	     $(DESTDIR)$(PREFIX)/$(CROSSARCH)/lib$$bits/libcrt_common.a; \
	  if [[ "$$bits" = "32" ]]; then \
	    objformat=elf32-i386-nacl ; \
	  else \
	    objformat=elf64-x86-64-nacl ; \
	  fi ; \
	  sed "s/@OBJFORMAT@/$$objformat/" newlib-libc-script \
	    > $(DESTDIR)$(PREFIX)/$(CROSSARCH)/lib$$bits/libc.a; \
	done
	touch $@

.PHONY: newlib
newlib: BUILD/stamp-$(CROSSARCH)-newlib


##################################################################
# glibc:
##################################################################

# Build directory for glibc.
GLIBC_BUILD_DIR = BUILD/build-glibc-$(CROSSARCH)

# Glibc is built with pregcc.
GLIBC_CC = $(PREGCC_PREFIX)/bin/$(CROSSARCH)-gcc

# CFLAGS for building glibc.
GLIBC_CFLAGS = -O2 -g

ARCH_DEST = $(DESTDIR)$(PREFIX)/$(CROSSARCH)
ARCH_DEST_INC_NATIVE = $(PREFIX_NATIVE)/$(CROSSARCH)/include

# LIB_BITS is used with different values to execute targets in this Makefile for
# different architectures (32, 64) when building libraries (glibc and nacl).
# CROSSARCH and BITSPLATFORM could be used for this, but we better avoid
# redefining variables with recursive $(MAKE) calls.
LIB_BITS ?= 64
ARCH_DEST_LIB_NATIVE = $(PREFIX_NATIVE)/$(CROSSARCH)/$(if $(filter 32,$(LIB_BITS)),lib32,lib)

BUILD/stamp-glibc32: BUILD/stamp-$(CROSSARCH)-pregcc-standalone | SRC/glibc
	if [[ ! -d $(LINUX_HEADERS) ]] ; then \
	  $(MAKE) -f $(THISMAKEFILE) SRC/linux-headers-for-nacl ; \
	fi
	rm -rf BUILD/build-glibc32
	mkdir -p BUILD/build-glibc32/lib
	cd BUILD/build-glibc32 && ../../SRC/glibc/configure \
	    BUILD_CC="gcc -O2 -g" \
	    CC="$(GLIBC_CC) -m32" \
	    CFLAGS="-pipe -fno-strict-aliasing -mno-tls-direct-seg-refs -march=i486 $(GLIBC_CFLAGS)" \
	    libc_cv_forced_unwind=yes \
	    libc_cv_c_cleanup=yes \
	    libc_cv_slibdir=/lib32 \
	    libc_cv_z_combreloc=no \
	    --prefix= \
	    --libdir=/lib32 \
	    --host=i486-linux-gnu \
	    --with-headers=$(LINUX_HEADERS) \
	    --enable-kernel=2.6.18
	$(MAKE) -C BUILD/build-glibc32
	$(MAKE) -C BUILD/build-glibc32 install_root=$(DESTDIR)$(PREFIX)/$(CROSSARCH) install
	touch $@

BUILD/stamp-glibc64: BUILD/stamp-$(CROSSARCH)-pregcc-standalone | SRC/glibc
	if [[ ! -d $(LINUX_HEADERS) ]] ; then \
	  $(MAKE) -f $(THISMAKEFILE) SRC/linux-headers-for-nacl ; \
	fi
	rm -rf BUILD/build-glibc64
	mkdir -p BUILD/build-glibc64
	cd BUILD/build-glibc64 && ../../SRC/glibc/configure \
	    BUILD_CC="gcc -O2 -g" \
	    CC="$(GLIBC_CC) -m64" \
	    CFLAGS="-pipe -fno-strict-aliasing -mno-tls-direct-seg-refs $(GLIBC_CFLAGS)" \
	    libc_cv_forced_unwind=yes \
	    libc_cv_c_cleanup=yes \
	    libc_cv_slibdir=/lib \
	    libc_cv_z_combreloc=no \
	    --prefix= \
	    --libdir=/lib \
	    --host=x86_64-linux-gnu \
	    --with-headers=$(LINUX_HEADERS) \
	    --enable-kernel=2.6.18
	$(MAKE) -C BUILD/build-glibc64
	$(MAKE) -C BUILD/build-glibc64 install_root=$(DESTDIR)$(PREFIX)/$(CROSSARCH) install
	touch $@

# Can be used to make a glibc archive separately from the main install tree.
# Used, i.e., on buildbots.
INST_GLIBC_PREFIX ?= $(PREFIX)
.PHONY: install-glibc
install-glibc: BUILD/stamp-glibc32 BUILD/stamp-glibc64
	rm -rf "$(INST_GLIBC_PREFIX)"/glibc
	mkdir "$(INST_GLIBC_PREFIX)"/glibc
	$(MAKE) -f $(THISMAKEFILE) sdkdirs \
	  DESTDIR="" PREFIX="$(INST_GLIBC_PREFIX)/glibc"
	$(MAKE) -f $(THISMAKEFILE) -C BUILD/build-glibc32 \
	  install_root="$(INST_GLIBC_PREFIX)/glibc/$(CROSSARCH)" install
	$(MAKE) -f $(THISMAKEFILE) -C BUILD/build-glibc64 \
	  install_root="$(INST_GLIBC_PREFIX)/glibc/$(CROSSARCH)" install

.PHONY: export-headers
export-headers: SRC/newlib
ifeq ($(CANNED_REVISION), no)
	rm -rf $(HPREFIX)/{bits,sys,machine}
	../src/trusted/service_runtime/export_header.py \
	  ../src/trusted/service_runtime/include $(HPREFIX)
else
	true
endif

##################################################################
# Ad hoc linker scripts and a selection of NaCl headers for GCC.
##################################################################
.PHONY: glibc-adhoc-files
glibc-adhoc-files: | SRC/glibc
	if [[ ! -d $(LINUX_HEADERS) ]] ; then \
	  $(MAKE) -f $(THISMAKEFILE) SRC/linux-headers-for-nacl ; \
	fi
	install -m 755 -d  $(ARCH_DEST)/lib/ldscripts
	cp -f SRC/glibc/nacl/dyn-link/ldscripts/* \
	    $(ARCH_DEST)/lib/ldscripts/
	mkdir -p $(ARCH_DEST)/include/machine
	cp -rf $(LINUX_HEADERS)/{asm*,linux} $(ARCH_DEST)/include
ifeq ($(CANNED_REVISION), no)
	cp ../src/untrusted/include/machine/_default_types.h \
	    $(ARCH_DEST)/include/machine/_default_types.h
else
	cp _default_types.h \
	    $(ARCH_DEST)/include/machine/_default_types.h
endif
	for f in catchsegv gencat getconf getent iconv ldd locale \
	    localedef mtrace pcprofiledump rpcgen sprof tzselect xtrace; do \
	    rm -f $(ARCH_DEST)/bin/$$f ; \
	done
	# These libraries are in link lines because newlib needs them.
	# Since glibc doesn't need them, we just stub them out as empty
	# linker scripts.  For -lfoo the linker looks for libfoo.so first
	# and then libfoo.a, but only the latter under -static, so install
	# under .a names to cover both cases.
	for libdir in lib32 lib; do \
	  for lib in nacl nosys; do \
	    echo '/* Intentionally empty */' > \
		$(PREFIX_NATIVE)/$(CROSSARCH)/$${libdir}/lib$${lib}.a; \
	  done; \
	done

##################################################################
# gcc:
#   Builds GCC with glibc as a C library.
##################################################################
SYSINCLUDE_HACK_TARGET = $(DESTDIR)$(PREFIX)/$(CROSSARCH)/sys-include

BUILD/stamp-$(CROSSARCH)-full-gcc: glibc-adhoc-files
ifneq ($(SRCDIR),)
	$(MAKE) -f $(THISMAKEFILE) gcc-extras
endif
	rm -rf BUILD/build-full-gcc-$(CROSSARCH)
	mkdir BUILD/build-full-gcc-$(CROSSARCH){,/lib}
	ln -s $(DESTDIR)$(PREFIX)/$(CROSSARCH)/lib \
	  BUILD/build-full-gcc-$(CROSSARCH)/lib/gcc
	# See http://code.google.com/p/nativeclient/issues/detail?id=854
	rm -rf $(SYSINCLUDE_HACK_TARGET)
	ln -s include $(SYSINCLUDE_HACK_TARGET)
	cd BUILD/build-full-gcc-$(CROSSARCH) && \
	PATH=$(BUILDPATH) \
	$(GCC_SRC_DIR)/configure \
	    CC="$(GCC_CC)" \
	    CFLAGS="$(USER_CFLAGS) $(GCC_DEFINES)" \
	    LDFLAGS="$(USER_LDFLAGS)" \
	    CFLAGS_FOR_TARGET="-O2 -g $(GCC_CFLAGS_FOR_TARGET-glibc)" \
	    CXXFLAGS_FOR_TARGET="-O2 -g $(GCC_CFLAGS_FOR_TARGET-glibc)" \
	    --prefix=$(PREFIX) \
	    $(GCC_CONFIGURE_FLAGS) \
	    $(GCC_CONFIGURE_FLAGS-glibc)
ifeq ($(PLATFORM), linux)
	if [[ "$(CROSSARCH)" = "x86_64-nacl" ]] ; then \
	  if file BUILD/build-pregcc-x86_64-nacl/gcc/cc1 | grep -q x86-64; \
	  then \
	    export LD_PRELOAD=/lib64/libgcc_s.so.1 ; \
	  fi ; \
	else \
	  if ! (file BUILD/build-pregcc-x86_64-nacl/gcc/cc1 | grep -q x86-64); \
	  then \
	    export LD_PRELOAD=/lib/libgcc_s.so.1 ; \
	  fi ; \
	fi ; \
	PATH=$(BUILDPATH) $(MAKE) \
	    -C BUILD/build-full-gcc-$(CROSSARCH) \
	    all
else
	PATH=$(BUILDPATH) $(MAKE) \
	    -C BUILD/build-full-gcc-$(CROSSARCH) \
	    all
endif
	PATH=$(BUILDPATH) $(MAKE) \
	    -C BUILD/build-full-gcc-$(CROSSARCH) \
	    DESTDIR=$(DESTDIR) \
	    install
	# See http://code.google.com/p/nativeclient/issues/detail?id=854
	rm -rf $(SYSINCLUDE_HACK_TARGET)
	touch $@

##################################################################
# gcc:
# Builds the gcc that will be used to build applications.
##################################################################
BUILD/stamp-$(CROSSARCH)-gcc: BUILD/stamp-$(CROSSARCH)-newlib \
  | SRC/gcc BUILD
ifneq ($(SRCDIR),)
	$(MAKE) -f $(THISMAKEFILE) gcc-extras
endif
	rm -rf BUILD/build-gcc-$(CROSSARCH)
	mkdir BUILD/build-gcc-$(CROSSARCH)
	mkdir -p $(SYSINCLUDE_HACK_TARGET)
	# See http://code.google.com/p/nativeclient/issues/detail?id=854
	rm -rf $(SYSINCLUDE_HACK_TARGET)
	ln -s include $(SYSINCLUDE_HACK_TARGET)
	cd BUILD/build-gcc-$(CROSSARCH) && \
	PATH=$(BUILDPATH) \
	$(GCC_SRC_DIR)/configure \
	    CC="$(GCC_CC)" \
	    CFLAGS="$(USER_CFLAGS) $(GCC_DEFINES)" \
	    LDFLAGS="$(USER_LDFLAGS)" \
	    CFLAGS_FOR_TARGET="-O2 -g $(GCC_CFLAGS_FOR_TARGET-newlib)" \
	    CXXFLAGS_FOR_TARGET="-O2 -g $(GCC_CFLAGS_FOR_TARGET-newlib)" \
	    --prefix=$(PREFIX) \
	    $(GCC_CONFIGURE_FLAGS) \
	    $(GCC_CONFIGURE_FLAGS-newlib)
	PATH=$(BUILDPATH) $(MAKE) \
	    -C BUILD/build-gcc-$(CROSSARCH) \
	    all
	PATH=$(BUILDPATH) $(MAKE) \
	    -C BUILD/build-gcc-$(CROSSARCH) \
	    DESTDIR=$(DESTDIR) \
	    install
	# See http://code.google.com/p/nativeclient/issues/detail?id=854
	rm -rf $(SYSINCLUDE_HACK_TARGET)
	touch $@

.PHONY: gcc
gcc: BUILD/stamp-$(CROSSARCH)-gcc

##################################################################
# Install headers from the NaCl tree locally for the gcc build to see.
##################################################################
.PHONY: headers_for_build
headers_for_build:
	cd .. && \
	  ./$(SCONS) nacl_newlib_dir=$(PREFIX_NATIVE) \
		     --verbose platform=x86-$(BITSPLATFORM) \
		     install_headers includedir=$(HEADERS_FOR_BUILD_NATIVE)

HEADERS_FOR_BUILD = \
	$(abspath $(dir $(THISMAKEFILE)))/BUILD/headers_for_build

ifeq ($(PLATFORM), win)
  HEADERS_FOR_BUILD_NATIVE = `cygpath -m $(HEADERS_FOR_BUILD)`
else
  HEADERS_FOR_BUILD_NATIVE = $(HEADERS_FOR_BUILD)
endif

##################################################################
# Build the entire toolchain.
##################################################################

# On platforms where glibc build is slow or unavailable you can specify
# glibc_download.sh (or any other program) to download glibc
INST_GLIBC_PROGRAM ?= none
.PHONY: build-with-glibc
build-with-glibc: SRC/gcc
	$(MAKE) -f $(THISMAKEFILE) sdkdirs
	cp -f SRC/gcc/COPYING* $(DESTDIR)$(PREFIX)
	$(MAKE) -f $(THISMAKEFILE) BUILD/stamp-$(CROSSARCH)-binutils
ifeq ($(INST_GLIBC_PROGRAM), none)
	$(MAKE) -f $(THISMAKEFILE) BUILD/stamp-$(CROSSARCH)-pregcc-standalone
	$(MAKE) -f $(THISMAKEFILE) BUILD/stamp-glibc32
	$(MAKE) -f $(THISMAKEFILE) BUILD/stamp-glibc64
else
	$(INST_GLIBC_PROGRAM) "$(DESTDIR)$(PREFIX)"
endif
	cp -f SRC/glibc/sysdeps/nacl/{irt_syscalls,nacl_stat,nacl_dirent}.h \
	  "$(DESTDIR)$(PREFIX)/$(CROSSARCH)"/include
	$(MAKE) -f $(THISMAKEFILE) export-headers
	$(MAKE) -f $(THISMAKEFILE) glibc-adhoc-files
	$(MAKE) -f $(THISMAKEFILE) BUILD/stamp-$(CROSSARCH)-full-gcc
	$(CREATE_REDIRECTORS) "$(DESTDIR)$(PREFIX)"
	for dir in lib32 lib64 ; do ( \
	  cd $(DESTDIR)$(PREFIX)/$(CROSSARCH)/$$dir ; \
	  for lib in BrokenLocale anl c cidn crypt dl m nsl \
	    nss_{compat,dns,files,hesiod,nis,nisplus} pthread \
	    resolv rt util ; do \
	    for fulllib in lib$$lib.so.* ; do \
	      mv lib$$lib-$(GLIBC_VERSION).so "$$fulllib" ; \
	      ln -sfn "$$fulllib" lib$$lib-$(GLIBC_VERSION).so ; \
	    done ; \
	  done ; \
	  for fulllib in ld-linux.so.* ld-linux-x86-64.so.* ; do \
	    if [[ "$$fulllib" != *\** ]] ; then \
	      mv ld-$(GLIBC_VERSION).so "$$fulllib" ; \
	      ln -sfn "$$fulllib" ld-$(GLIBC_VERSION).so ; \
	    fi ; \
	  done ; \
	  for fulllib in libthread_db.so.* ; do \
	    mv libthread_db-1.0.so "$$fulllib" ; \
	    ln -sfn "$$fulllib" libthread_db-1.0.so ; \
	  done ; \
	  chmod a+x libgcc_s.so.1 ; \
	  for fulllib in libgfortran.so.3.* ; do \
	    mv "$$fulllib" libgfortran.so.3 ; \
	    ln -sfn libgfortran.so.3 "$$fulllib" ; \
	    ln -sfn libgfortran.so.3 libgfortran.so ; \
	  done ; \
	  for fulllib in libobjc.so.2.* ; do \
	    mv "$$fulllib" libobjc.so.2 ; \
	    ln -sfn libobjc.so.2 "$$fulllib" ; \
	    ln -sfn libobjc.so.2 libobjc.so ; \
	  done ; \
	  for fulllib in libstdc++.so.6.* ; do \
	    mv "$$fulllib" libstdc++.so.6 ; \
	    ln -sfn libstdc++.so.6 "$$fulllib" ; \
	    ln -sfn libstdc++.so.6 libstdc++.so ; \
	  done ; \
	) ; done
	rm -rf "$(DESTDIR)$(PREFIX)"/{include,lib/*.a*,$(CROSSARCH)/lib{,32}/*.la}
	rm -rf "$(DESTDIR)$(PREFIX)"/{lib/{*/*/*/*{,/*}.la,*.so*},lib{32,64}}

.PHONY: build-with-newlib
build-with-newlib: SRC/gcc
	$(MAKE) -f $(THISMAKEFILE) sdkdirs
	cp -f SRC/gcc/COPYING* $(DESTDIR)$(PREFIX)
	$(MAKE) -f $(THISMAKEFILE) BUILD/stamp-$(CROSSARCH)-binutils
	$(MAKE) -f $(THISMAKEFILE) BUILD/stamp-$(CROSSARCH)-pregcc
	$(MAKE) -f $(THISMAKEFILE) export-headers
	$(MAKE) -f $(THISMAKEFILE) BUILD/stamp-$(CROSSARCH)-newlib
	$(CREATE_REDIRECTORS) "$(DESTDIR)$(PREFIX)"
ifeq ($(CANNED_REVISION), no)
	$(MAKE) -f $(THISMAKEFILE) headers_for_build
endif
	$(MAKE) -f $(THISMAKEFILE) BUILD/stamp-$(CROSSARCH)-gcc
	$(CREATE_REDIRECTORS) "$(DESTDIR)$(PREFIX)"
	rm -rf "$(DESTDIR)$(PREFIX)"/{include,lib/*.a*,lib/*.so*,lib32,lib64}

ifeq ($(CANNED_REVISION), no)
# Newlib toolchain for buildbot.
.PHONY: buildbot-build-with-newlib
buildbot-build-with-newlib: | \
  buildbot-mark-version \
  pinned-src-newlib
	find SRC -print0 | xargs -0 touch -r SRC
	$(MAKE) -f $(THISMAKEFILE) build-with-newlib

# Don't generate patch files for things like gmp or linux-headers-for-nacl
# because these are not changed from upstream.
BINUTILS_PATCHNAME := naclbinutils-$(BINUTILS_VERSION)-$(REVISION)
GCC_PATCHNAME := naclgcc-$(GCC_VERSION)-$(REVISION)
GLIBC_PATCHNAME := naclglibc-$(GLIBC_VERSION)-$(REVISION)
NEWLIB_PATCHNAME := naclnewlib-$(NEWLIB_VERSION)-$(REVISION)

patch-names = $(BINUTILS_PATCHNAME) $(GCC_PATCHNAME) \
	      $(GLIBC_PATCHNAME) $(NEWLIB_PATCHNAME)
patch-list = $(patch-names:%=SRC/%.patch)

$(patch-list): SRC/%.patch:
	package=$@ && \
	package=$${package#SRC/nacl} && \
	package=$${package/.patch/} && \
	basename=$${package/-*-*} && \
	cd SRC/$${basename} && \
	  git diff --patience --patch-with-stat --no-renames \
	    --src-prefix=$$basename/ \
	    --dst-prefix=$$basename/ \
	    $(NACL_$(shell n=$@ ; n=$${n#SRC/nacl} ; echo $${n/-*-*/} | \
	             tr '[:lower:]-' '[:upper:]_')_GIT_BASE) \
	    > ../../$@

.PHONY: patches
patches: $(patch-list)

# Glibc toolchain for buildbot.
.PHONY: buildbot-build-with-glibc
buildbot-build-with-glibc: | \
  buildbot-mark-version \
  pinned-src-glibc \
  pinned-src-linux-headers-for-nacl \
  pinned-src-newlib
	rm -rf SRC/gcc/gmp-* SRC/gcc/mpfr-*
	find SRC -print0 | xargs -0 touch -r SRC
	$(MAKE) -f $(THISMAKEFILE) build-with-glibc

.PHONY: buildbot-mark-version
buildbot-mark-version: | \
  pinned-src-binutils \
  pinned-src-gcc
	printf -- "--- SRC/binutils/bfd/version.h\n\
	+++ SRC/binutils/bfd/version.h\n\
	@@ -3 +3 @@\n\
	-#define BFD_VERSION_STRING  @bfd_version_package@ @bfd_version_string@\n\
	+#define BFD_VERSION_STRING  @bfd_version_package@ @bfd_version_string@ \" $(REVISION_DATE) (Native Client $(REVISION), Git Commit `cd SRC/binutils ; LC_ALL=C git rev-parse HEAD`)\"\n" |\
	patch -p0
	echo $(REVISION_DATE) > SRC/gcc/gcc/DATESTAMP
	echo "Native Client $(REVISION), Git Commit `cd SRC/gcc ; LC_ALL=C git rev-parse HEAD`" > SRC/gcc/gcc/DEV-PHASE
endif

##################################################################
# Run DejaGnu tests.
##################################################################

SEL_LDR = $(abspath ../scons-out/opt-$(PLATFORM)-x86-$(BITSPLATFORM)/staging/sel_ldr)
DEJAGNU_TIMESTAMP := $(shell date +%y%m%d%H%M%S)

.PHONY: $(SEL_LDR)
$(SEL_LDR):
	(cd .. && \
	  ./$(SCONS) nacl_glibc_dir=$(DESTDIR)$(PREFIX) \
	    --mode=opt-host,nacl platform=x86-$(BITSPLATFORM) \
	    --verbose sel_ldr)

.PHONY: check
check: $(SEL_LDR)
	(cd .. && \
	  ./$(SCONS) nacl_glibc_dir=$(DESTDIR)$(PREFIX) \
	    --mode=opt-host,nacl platform=x86-$(BITSPLATFORM) \
	    --verbose run_hello_world_test)
	mkdir BUILD/build-gcc-$(CROSSARCH)/results.$(DEJAGNU_TIMESTAMP)
	$(MAKE) \
	    -C BUILD/build-gcc-$(CROSSARCH) \
	    DEJAGNU=$(abspath dejagnu/site.exp) \
	    RUNTESTFLAGS=" \
	        --target_board=nacl \
	        --outdir=$(abspath BUILD/build-gcc-$(CROSSARCH)/results.$(DEJAGNU_TIMESTAMP)) \
	        SIM=$(SEL_LDR)" \
	    LDFLAGS_FOR_TARGET="-lnosys" \
	    check


##################################################################
# Run GlibC tests.
##################################################################

.PHONY: glibc-check
glibc-check: $(SEL_LDR)
	GLIBC_TST_COLLECT2="$(PREGCC_PREFIX)/libexec/gcc/$(CROSSARCH)/$(GCC_VERSION)/collect2" \
	GLIBC_TST_STATIC_LDSCRIPT="$(DESTDIR)$(PREFIX)/$(CROSSARCH)"/lib/ldscripts/elf64_nacl.x.static \
	GLIBC_TST_NACL_LOADER="$(SEL_LDR)" \
	GLIBC_TST_NACL_LIBDIR="$(DESTDIR)$(PREFIX)/$(CROSSARCH)"/lib \
	  glibc-tests/run_tests.sh $(BITSPLATFORM)

.PHONY: glibc-check32
glibc-check32: $(SEL_LDR)
	GLIBC_TST_COLLECT2="$(PREGCC_PREFIX)/libexec/gcc/$(CROSSARCH)/$(GCC_VERSION)/collect2" \
	GLIBC_TST_STATIC_LDSCRIPT="$(DESTDIR)$(PREFIX)/$(CROSSARCH)"/lib/ldscripts/elf_nacl.x.static \
	GLIBC_TST_NACL_LOADER="$(SEL_LDR)" \
	GLIBC_TST_NACL_LIBDIR="$(DESTDIR)$(PREFIX)/$(CROSSARCH)"/lib32 \
	  glibc-tests/run_tests.sh $(BITSPLATFORM)


##################################################################
# Compile Native Client tests with the toolchain and run them.
##################################################################
.PHONY: nacl-check
nacl-check:
	(cd .. && \
	  ./$(SCONS) -k \
	    $(SCONS_DESTINATIONS_NOLIB) \
	    --mode=opt-host,nacl platform=x86-$(BITSPLATFORM) \
	    --nacl_glibc --verbose small_tests)


##################################################################
# Remove the BUILD directory.
# Library builds are maintained by scons.
##################################################################

.PHONY: clean
clean:
	rm -rf BUILD/*
