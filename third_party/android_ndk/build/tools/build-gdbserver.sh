#!/bin/bash
#
# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#  This shell script is used to rebuild the gdbserver binary from
#  the Android NDK's prebuilt binaries.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh

PROGRAM_PARAMETERS="<arch> <target-triple> <src-dir> <ndk-dir>"

PROGRAM_DESCRIPTION=\
"Rebuild the gdbserver prebuilt binary for the Android NDK toolchain.

Where <src-dir> is the location of the gdbserver sources,
<ndk-dir> is the top-level NDK installation path and <toolchain>
is the name of the toolchain to use (e.g. arm-linux-androideabi-4.8).

The final binary is placed under:

    <build-out>/gdbserver

NOTE: The --platform option is ignored if --sysroot is used."

VERBOSE=no

BUILD_OUT=
register_var_option "--build-out=<path>" BUILD_OUT "Set temporary build directory"

SYSROOT=
register_var_option "--sysroot=<path>" SYSROOT "Specify sysroot directory directly"

NOTHREADS=no
register_var_option "--disable-threads" NOTHREADS "Disable threads support"

GDB_VERSION=
register_var_option "--gdb-version=<name>" GDB_VERSION "Use specific gdb version."

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Archive binary into specific directory"

register_jobs_option

register_try64_option

extract_parameters "$@"

if [ -z "$BUILD_OUT" ]; then
  echo "ERROR: --build-out is required"
  exit 1
fi

INSTALL_DIR=$BUILD_OUT/install
BUILD_OUT=$BUILD_OUT/build

set_parameters ()
{
    ARCH="$1"
    GDBSERVER_HOST="$2"
    SRC_DIR="$3"
    NDK_DIR="$4"
    GDBVER=

    # Check architecture
    #
    if [ -z "$ARCH" ] ; then
        echo "ERROR: Missing target architecture. See --help for details."
        exit 1
    fi

    log "Targetting CPU: $ARCH"

    # Check host value
    #
    if [ -z "$GDBSERVER_HOST" ] ; then
        echo "ERROR: Missing target triple. See --help for details."
        exit 1
    fi

    log "GDB target triple: $GDBSERVER_HOST"

    # Check source directory
    #
    if [ -z "$SRC_DIR" ] ; then
        echo "ERROR: Missing source directory parameter. See --help for details."
        exit 1
    fi

    if [ -n "$GDB_VERSION" ]; then
        GDBVER=$GDB_VERSION
    else
        GDBVER=$(get_default_gdbserver_version)
    fi

    SRC_DIR2="$SRC_DIR/gdb/gdb-$GDBVER/gdb/gdbserver"
    if [ -d "$SRC_DIR2" ] ; then
        SRC_DIR="$SRC_DIR2"
        log "Found gdbserver source directory: $SRC_DIR"
    fi

    if [ ! -f "$SRC_DIR/gdbreplay.c" ] ; then
        echo "ERROR: Source directory does not contain gdbserver sources: $SRC_DIR"
        exit 1
    fi

    log "Using source directory: $SRC_DIR"

    # Check NDK installation directory
    #
    if [ -z "$NDK_DIR" ] ; then
        echo "ERROR: Missing NDK directory parameter. See --help for details."
        exit 1
    fi

    if [ ! -d "$NDK_DIR" ] ; then
        echo "ERROR: NDK directory does not exist: $NDK_DIR"
        exit 1
    fi

    log "Using NDK directory: $NDK_DIR"
}

set_parameters $PARAMETERS

if [ "$PACKAGE_DIR" ]; then
    mkdir -p "$PACKAGE_DIR"
    fail_panic "Could not create package directory: $PACKAGE_DIR"
fi

prepare_target_build

GCC_VERSION=$(get_default_gcc_version_for_arch $ARCH)
log "Using GCC version: $GCC_VERSION"
TOOLCHAIN_PREFIX=$ANDROID_BUILD_TOP/prebuilts/ndk/current/
TOOLCHAIN_PREFIX+=$(get_toolchain_binprefix_for_arch $ARCH $GCC_VERSION)

# Determine cflags when building gdbserver
GDBSERVER_CFLAGS=
case "$ARCH" in
arm*)
    GDBSERVER_CFLAGS+="-fno-short-enums"
    ;;
esac

case "$ARCH" in
*64)
    GDBSERVER_CFLAGS+=" -DUAPI_HEADERS"
    ;;
esac


PLATFORM="android-$LATEST_API_LEVEL"

# Check build directory
#
fix_sysroot "$SYSROOT"
log "Using sysroot: $SYSROOT"

log "Using build directory: $BUILD_OUT"
run rm -rf "$BUILD_OUT"
run mkdir -p "$BUILD_OUT"

# Copy the sysroot to a temporary build directory
BUILD_SYSROOT="$BUILD_OUT/sysroot"
run mkdir -p "$BUILD_SYSROOT"
run cp -RHL "$SYSROOT"/* "$BUILD_SYSROOT"

# Make sure multilib toolchains have lib64
if [ ! -d "$BUILD_SYSROOT/usr/lib64" ] ; then
    mkdir "$BUILD_SYSROOT/usr/lib64"
fi

# Make sure multilib toolchains know their target
TARGET_FLAG=
if [ "$ARCH" = "mips" ] ; then
    TARGET_FLAG=-mips32
fi

LIBDIR=$(get_default_libdir_for_arch $ARCH)

# Remove libthread_db to ensure we use exactly the one we want.
rm -f $BUILD_SYSROOT/usr/$LIBDIR/libthread_db*
rm -f $BUILD_SYSROOT/usr/include/thread_db.h

if [ "$NOTHREADS" != "yes" ] ; then
    # We're going to rebuild libthread_db.o from its source
    # that is under sources/android/libthread_db and place its header
    # and object file into the build sysroot.
    LIBTHREAD_DB_DIR=$ANDROID_NDK_ROOT/sources/android/libthread_db
    if [ ! -d "$LIBTHREAD_DB_DIR" ] ; then
        dump "ERROR: Missing directory: $LIBTHREAD_DB_DIR"
        exit 1
    fi

    run cp $LIBTHREAD_DB_DIR/thread_db.h $BUILD_SYSROOT/usr/include/
    run ${TOOLCHAIN_PREFIX}gcc --sysroot=$BUILD_SYSROOT $TARGET_FLAG -o $BUILD_SYSROOT/usr/$LIBDIR/libthread_db.o -c $LIBTHREAD_DB_DIR/libthread_db.c
    run ${TOOLCHAIN_PREFIX}ar -rD $BUILD_SYSROOT/usr/$LIBDIR/libthread_db.a $BUILD_SYSROOT/usr/$LIBDIR/libthread_db.o
    if [ $? != 0 ] ; then
        dump "ERROR: Could not compile libthread_db.c!"
        exit 1
    fi
fi

log "Using build sysroot: $BUILD_SYSROOT"

# configure the gdbserver build now
dump "Configure: $ARCH gdbserver-$GDBVER build with $PLATFORM"

# This flag is required to link libthread_db statically to our
# gdbserver binary. Otherwise, the program will try to dlopen()
# the threads binary, which is not possible since we build a
# static executable.
CONFIGURE_FLAGS="--with-libthread-db=$BUILD_SYSROOT/usr/$LIBDIR/libthread_db.a"
# Disable libinproctrace.so which needs crtbegin_so.o and crtbend_so.o instead of
# CRTBEGIN/END above.  Clean it up and re-enable it in the future.
CONFIGURE_FLAGS=$CONFIGURE_FLAGS" --disable-inprocess-agent"
# gdb 7.7 builds with -Werror by default, but they redefine constants such as
# HWCAP_VFPv3 in a way that's compatible with glibc's headers but not our
# kernel uapi headers. We should send a patch upstream to add the missing
# #ifndefs, but for now just build gdbserver without -Werror.
CONFIGURE_FLAGS=$CONFIGURE_FLAGS" --enable-werror=no"

cd $BUILD_OUT &&
export CC="${TOOLCHAIN_PREFIX}gcc --sysroot=$BUILD_SYSROOT $TARGET_FLAG" &&
export AR="${TOOLCHAIN_PREFIX}ar" &&
export RANLIB="${TOOLCHAIN_PREFIX}ranlib" &&
export CFLAGS="-O2 $GDBSERVER_CFLAGS"  &&
export LDFLAGS="-static -Wl,-z,nocopyreloc -Wl,--no-undefined" &&
run $SRC_DIR/configure \
--build=x86_64-linux-gnu \
--host=$GDBSERVER_HOST \
$CONFIGURE_FLAGS
if [ $? != 0 ] ; then
    dump "Could not configure gdbserver build."
    exit 1
fi

# build gdbserver
dump "Building : $ARCH gdbserver."
cd $BUILD_OUT &&
run make -j$NUM_JOBS
if [ $? != 0 ] ; then
    dump "Could not build $ARCH gdbserver. Use --verbose to see why."
    exit 1
fi

# install gdbserver
#
# note that we install it in the toolchain bin directory
# not in $SYSROOT/usr/bin
#
if [ "$NOTHREADS" = "yes" ] ; then
    DSTFILE="gdbserver-nothreads"
else
    DSTFILE="gdbserver"
fi

dump "Install  : $ARCH $DSTFILE."
mkdir -p $INSTALL_DIR &&
run ${TOOLCHAIN_PREFIX}objcopy --strip-unneeded \
  $BUILD_OUT/gdbserver $INSTALL_DIR/$DSTFILE
fail_panic "Could not install $DSTFILE."

make_repo_prop "$INSTALL_DIR"
cp "$SRC_DIR/../../COPYING" "$INSTALL_DIR/NOTICE"
fail_panic "Could not copy license file!"

dump "Done."
