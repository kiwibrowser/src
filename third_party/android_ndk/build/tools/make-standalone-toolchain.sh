#!/bin/bash
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

# Create a standalone toolchain package for Android.

. `dirname $0`/prebuilt-common.sh

PROGRAM_PARAMETERS=""
PROGRAM_DESCRIPTION=\
"Generate a customized Android toolchain installation that includes
a working sysroot. The result is something that can more easily be
used as a standalone cross-compiler, e.g. to run configure and
make scripts."

TOOLCHAIN_NAME=
register_var_option "--toolchain=<name>" TOOLCHAIN_NAME "Specify toolchain name"

do_option_use_llvm () {
  true;
}
register_option "--use-llvm" do_option_use_llvm "No-op. Clang is always available."

STL=gnustl
register_var_option "--stl=<name>" STL "Specify C++ STL"

ARCH=
register_var_option "--arch=<name>" ARCH "Specify target architecture"

# Grab the ABIs that match the architecture.
ABIS=
register_var_option "--abis=<list>" ABIS "No-op. Derived from --arch or --toolchain."

NDK_DIR=
register_var_option "--ndk-dir=<path>" NDK_DIR "Unsupported."

PACKAGE_DIR=$TMPDIR
register_var_option "--package-dir=<path>" PACKAGE_DIR "Place package file in <path>"

INSTALL_DIR=
register_var_option "--install-dir=<path>" INSTALL_DIR "Don't create package, install files to <path> instead."

DRYRUN=
register_var_option "--dryrun" DRYRUN "Unsupported."

PLATFORM=
register_option "--platform=<name>" do_platform "Specify target Android platform/API level." "android-14"
do_platform () {
    PLATFORM=$1;
    if [ "$PLATFORM" = "android-L" ]; then
        echo "WARNING: android-L is renamed as android-21"
        PLATFORM=android-21
    fi
}

FORCE=
do_force () {
    FORCE=true
}
register_option "--force" do_force "Remove existing install directory."

extract_parameters "$@"

if [ -n "$NDK_DIR" ]; then
    dump "The --ndk-dir argument is no longer supported."
    exit 1
fi

if [ -n "$DRYRUN" ]; then
    dump "--dryrun is not supported."
    exit 1
fi

# Check TOOLCHAIN_NAME
ARCH_BY_TOOLCHAIN_NAME=
if [ -n "$TOOLCHAIN_NAME" ]; then
    case $TOOLCHAIN_NAME in
        arm-*)
            ARCH_BY_TOOLCHAIN_NAME=arm
            ;;
        x86-*)
            ARCH_BY_TOOLCHAIN_NAME=x86
            ;;
        mipsel-*)
            ARCH_BY_TOOLCHAIN_NAME=mips
            ;;
        aarch64-*)
            ARCH_BY_TOOLCHAIN_NAME=arm64
            ;;
        x86_64-linux-android-*)
            ARCH_BY_TOOLCHAIN_NAME=x86_64
            TOOLCHAIN_NAME=$(echo "$TOOLCHAIN_NAME" | sed -e 's/-linux-android//')
            echo "Auto-truncate: --toolchain=$TOOLCHAIN_NAME"
            ;;
        x86_64-*)
            ARCH_BY_TOOLCHAIN_NAME=x86_64
            ;;
        mips64el-*)
            ARCH_BY_TOOLCHAIN_NAME=mips64
            ;;
        *)
            echo "Invalid toolchain $TOOLCHAIN_NAME"
            exit 1
            ;;
    esac
fi
# Check ARCH
if [ -z "$ARCH" ]; then
    ARCH=$ARCH_BY_TOOLCHAIN_NAME
    if [ -z "$ARCH" ]; then
        ARCH=arm
    fi
    echo "Auto-config: --arch=$ARCH"
fi

# Install or Package
FORCE_ARG=
if [ -n "$INSTALL_DIR" ] ; then
    INSTALL_ARG="--install-dir=$INSTALL_DIR"
    INSTALL_LOCATION=$INSTALL_DIR
    if [ "$FORCE" = "true" ]; then
        FORCE_ARG="--force"
    else
        if [ -e "$INSTALL_DIR" ]; then
            dump "Refusing to clobber existing install directory: $INSTALL_DIR.

make-standalone-toolchain.sh used to install a new toolchain into an existing
directory. This is not desirable, as it will not clean up any stale files. If
you wish to remove the install directory before creation, pass --force."
            exit 1
        fi
    fi
else
    INSTALL_ARG="--package-dir=$PACKAGE_DIR"
fi

PLATFORM_NUMBER=${PLATFORM#android-}
if [ -n "$PLATFORM_NUMBER" ]; then
  PLATFORM_ARG="--api $PLATFORM_NUMBER"
else
  PLATFORM_ARG=""
fi

run python `dirname $0`/make_standalone_toolchain.py \
    --arch $ARCH $PLATFORM_ARG --stl $STL $INSTALL_ARG $FORCE_ARG
fail_panic "Failed to create toolchain."

if [ -n "$INSTALL_DIR" ]; then
    dump "Toolchain installed to $INSTALL_DIR."
else
    dump "Package installed to $PACKAGE_DIR."
fi
