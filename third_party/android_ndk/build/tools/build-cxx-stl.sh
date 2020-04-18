#!/bin/bash
#
# Copyright (C) 2013 The Android Open Source Project
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
#  This shell script is used to rebuild one of the NDK C++ STL
#  implementations from sources. To use it:
#
#   - Define CXX_STL to one of 'stlport' or 'libc++'
#   - Run it.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh
. `dirname $0`/builder-funcs.sh

CXX_STL_LIST="stlport libc++"

PROGRAM_PARAMETERS=""

PROGRAM_DESCRIPTION=\
"Rebuild one of the following NDK C++ runtimes: $CXX_STL_LIST.

This script is called when pacakging a new NDK release. It will simply
rebuild the static and shared libraries of a given C++ runtime from
sources.

Use the --stl=<name> option to specify which runtime you want to rebuild.

This requires a temporary NDK installation containing platforms and
toolchain binaries for all target architectures.

By default, this will try with the current NDK directory, unless
you use the --ndk-dir=<path> option.

If you want to use clang to rebuild the binaries, please use
--llvm-version=<ver> option.

The output will be placed in appropriate sub-directories of
<ndk>/sources/cxx-stl/$CXX_STL_SUBDIR, but you can override this with
the --out-dir=<path> option.
"

CXX_STL=
register_var_option "--stl=<name>" CXX_STL "Select C++ runtime to rebuild."

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Put prebuilt tarballs into <path>."

NDK_DIR=
register_var_option "--ndk-dir=<path>" NDK_DIR "Specify NDK root path for the build."

BUILD_DIR=
register_var_option "--build-dir=<path>" BUILD_DIR "Specify temporary build dir."

OUT_DIR=
register_var_option "--out-dir=<path>" OUT_DIR "Specify output directory directly."

ABIS="$PREBUILT_ABIS"
register_var_option "--abis=<list>" ABIS "Specify list of target ABIs."

NO_MAKEFILE=
register_var_option "--no-makefile" NO_MAKEFILE "Do not use makefile to speed-up build"

VISIBLE_STATIC=
register_var_option "--visible-static" VISIBLE_STATIC "Do not use hidden visibility for the static library"

WITH_DEBUG_INFO=
register_var_option "--with-debug-info" WITH_DEBUG_INFO "Build with -g.  STL is still built with optimization but with debug info"

GCC_VERSION=
register_var_option "--gcc-version=<ver>" GCC_VERSION "Specify GCC version"

LLVM_VERSION=
register_var_option "--llvm-version=<ver>" LLVM_VERSION "Specify LLVM version"

register_jobs_option

register_try64_option

extract_parameters "$@"

if [ -n "${LLVM_VERSION}" -a -n "${GCC_VERSION}" ]; then
    panic "Cannot set both LLVM_VERSION and GCC_VERSION. Make up your mind!"
fi

ABIS=$(commas_to_spaces $ABIS)

# Handle NDK_DIR
if [ -z "$NDK_DIR" ] ; then
    NDK_DIR=$ANDROID_NDK_ROOT
    log "Auto-config: --ndk-dir=$NDK_DIR"
else
  if [ ! -d "$NDK_DIR" ]; then
    panic "NDK directory does not exist: $NDK_DIR"
  fi
fi

# Handle OUT_DIR
if [ -z "$OUT_DIR" ] ; then
  OUT_DIR=$ANDROID_NDK_ROOT
  log "Auto-config: --out-dir=$OUT_DIR"
else
  mkdir -p "$OUT_DIR"
  fail_panic "Could not create directory: $OUT_DIR"
fi

# Check that --stl=<name> is used with one of the supported runtime names.
if [ -z "$CXX_STL" ]; then
  panic "Please use --stl=<name> to select a C++ runtime to rebuild."
fi

# Derive runtime, and normalize CXX_STL
CXX_SUPPORT_LIB=gabi++
case $CXX_STL in
  stlport)
    ;;
  libc++)
    CXX_SUPPORT_LIB=gabi++  # libc++abi
    ;;
  libc++-libc++abi)
    CXX_SUPPORT_LIB=libc++abi
    CXX_STL=libc++
    ;;
  libc++-gabi++)
    CXX_SUPPORT_LIB=gabi++
    CXX_STL=libc++
    ;;
  *)
    panic "Invalid --stl value ('$CXX_STL'), please use one of: $CXX_STL_LIST."
    ;;
esac

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
fail_panic "Could not create build directory: $BUILD_DIR"

# Location of the various C++ runtime source trees.  Use symlink from
# $BUILD_DIR instead of $NDK which may contain full path of builder's working dir

rm -f $BUILD_DIR/ndk
ln -sf $ANDROID_NDK_ROOT $BUILD_DIR/ndk

GABIXX_SRCDIR=$BUILD_DIR/ndk/$GABIXX_SUBDIR
STLPORT_SRCDIR=$BUILD_DIR/ndk/$STLPORT_SUBDIR
LIBCXX_SRCDIR=$BUILD_DIR/ndk/$LIBCXX_SUBDIR
LIBCXXABI_SRCDIR=$BUILD_DIR/ndk/$LIBCXXABI_SUBDIR

LIBCXX_INCLUDES="-I$LIBCXX_SRCDIR/libcxx/include -I$ANDROID_NDK_ROOT/sources/android/support/include -I$LIBCXXABI_SRCDIR/include"

COMMON_C_CXX_FLAGS="-fPIC -O2 -ffunction-sections -fdata-sections"
COMMON_CXXFLAGS="-fexceptions -frtti -fuse-cxa-atexit"

if [ "$WITH_DEBUG_INFO" ]; then
    COMMON_C_CXX_FLAGS="$COMMON_C_CXX_FLAGS -g"
fi

if [ "$CXX_STL" = "libc++" ]; then
    # Use clang to build libc++ by default.
    if [ -z "$LLVM_VERSION" -a -z "$GCC_VERSION" ]; then
        LLVM_VERSION=$DEFAULT_LLVM_VERSION
    fi
fi

# Determine GAbi++ build parameters. Note that GAbi++ is also built as part
# of STLport and Libc++, in slightly different ways.
if [ "$CXX_SUPPORT_LIB" = "gabi++" ]; then
    if [ "$CXX_STL" = "libc++" ]; then
        GABIXX_INCLUDES="$LIBCXX_INCLUDES"
        GABIXX_CXXFLAGS="$GABIXX_CXXFLAGS -DLIBCXXABI=1"
    else
        GABIXX_INCLUDES="-I$GABIXX_SRCDIR/include"
    fi
    GABIXX_CFLAGS="$COMMON_C_CXX_FLAGS $GABIXX_INCLUDES"
    GABIXX_CXXFLAGS="$GABIXX_CXXFLAGS $GABIXX_CFLAGS $COMMON_CXXFLAGS"
    GABIXX_SOURCES=$(cd $ANDROID_NDK_ROOT/$GABIXX_SUBDIR && ls src/*.cc)
    GABIXX_LDFLAGS="-ldl"
fi

# Determine STLport build parameters
STLPORT_CFLAGS="$COMMON_C_CXX_FLAGS -DGNU_SOURCE -I$STLPORT_SRCDIR/stlport $GABIXX_INCLUDES"
STLPORT_CXXFLAGS="$STLPORT_CFLAGS $COMMON_CXXFLAGS"
STLPORT_SOURCES=\
"src/dll_main.cpp \
src/fstream.cpp \
src/strstream.cpp \
src/sstream.cpp \
src/ios.cpp \
src/stdio_streambuf.cpp \
src/istream.cpp \
src/ostream.cpp \
src/iostream.cpp \
src/codecvt.cpp \
src/collate.cpp \
src/ctype.cpp \
src/monetary.cpp \
src/num_get.cpp \
src/num_put.cpp \
src/num_get_float.cpp \
src/num_put_float.cpp \
src/numpunct.cpp \
src/time_facets.cpp \
src/messages.cpp \
src/locale.cpp \
src/locale_impl.cpp \
src/locale_catalog.cpp \
src/facets_byname.cpp \
src/complex.cpp \
src/complex_io.cpp \
src/complex_trig.cpp \
src/string.cpp \
src/bitset.cpp \
src/allocators.cpp \
src/c_locale.c \
src/cxa.c"

# Determine Libc++ build parameters
LIBCXX_LINKER_SCRIPT=export_symbols.txt
LIBCXX_CFLAGS="$COMMON_C_CXX_FLAGS $LIBCXX_INCLUDES -Drestrict=__restrict__"
LIBCXX_CXXFLAGS="$LIBCXX_CFLAGS -DLIBCXXABI=1 -std=c++11 -D__STDC_FORMAT_MACROS"
if [ -f "$_BUILD_SRCDIR/$LIBCXX_LINKER_SCRIPT" ]; then
    LIBCXX_LDFLAGS="-Wl,--version-script,\$_BUILD_SRCDIR/$LIBCXX_LINKER_SCRIPT"
fi
LIBCXX_SOURCES=\
"libcxx/src/algorithm.cpp \
libcxx/src/bind.cpp \
libcxx/src/chrono.cpp \
libcxx/src/condition_variable.cpp \
libcxx/src/debug.cpp \
libcxx/src/exception.cpp \
libcxx/src/future.cpp \
libcxx/src/hash.cpp \
libcxx/src/ios.cpp \
libcxx/src/iostream.cpp \
libcxx/src/locale.cpp \
libcxx/src/memory.cpp \
libcxx/src/mutex.cpp \
libcxx/src/new.cpp \
libcxx/src/optional.cpp \
libcxx/src/random.cpp \
libcxx/src/regex.cpp \
libcxx/src/shared_mutex.cpp \
libcxx/src/stdexcept.cpp \
libcxx/src/string.cpp \
libcxx/src/strstream.cpp \
libcxx/src/system_error.cpp \
libcxx/src/thread.cpp \
libcxx/src/typeinfo.cpp \
libcxx/src/utility.cpp \
libcxx/src/valarray.cpp \
libcxx/src/support/android/locale_android.cpp \
"

LIBCXXABI_SOURCES=\
"../llvm-libc++abi/libcxxabi/src/abort_message.cpp \
../llvm-libc++abi/libcxxabi/src/cxa_aux_runtime.cpp \
../llvm-libc++abi/libcxxabi/src/cxa_default_handlers.cpp \
../llvm-libc++abi/libcxxabi/src/cxa_demangle.cpp \
../llvm-libc++abi/libcxxabi/src/cxa_exception.cpp \
../llvm-libc++abi/libcxxabi/src/cxa_exception_storage.cpp \
../llvm-libc++abi/libcxxabi/src/cxa_guard.cpp \
../llvm-libc++abi/libcxxabi/src/cxa_handlers.cpp \
../llvm-libc++abi/libcxxabi/src/cxa_new_delete.cpp \
../llvm-libc++abi/libcxxabi/src/cxa_personality.cpp \
../llvm-libc++abi/libcxxabi/src/cxa_thread_atexit.cpp \
../llvm-libc++abi/libcxxabi/src/cxa_unexpected.cpp \
../llvm-libc++abi/libcxxabi/src/cxa_vector.cpp \
../llvm-libc++abi/libcxxabi/src/cxa_virtual.cpp \
../llvm-libc++abi/libcxxabi/src/exception.cpp \
../llvm-libc++abi/libcxxabi/src/private_typeinfo.cpp \
../llvm-libc++abi/libcxxabi/src/stdexcept.cpp \
../llvm-libc++abi/libcxxabi/src/typeinfo.cpp
"

LIBCXXABI_UNWIND_SOURCES=\
"../llvm-libc++abi/libcxxabi/src/Unwind/libunwind.cpp \
../llvm-libc++abi/libcxxabi/src/Unwind/Unwind-EHABI.cpp \
../llvm-libc++abi/libcxxabi/src/Unwind/Unwind-sjlj.c \
../llvm-libc++abi/libcxxabi/src/Unwind/UnwindLevel1.c \
../llvm-libc++abi/libcxxabi/src/Unwind/UnwindLevel1-gcc-ext.c \
../llvm-libc++abi/libcxxabi/src/Unwind/UnwindRegistersRestore.S \
../llvm-libc++abi/libcxxabi/src/Unwind/UnwindRegistersSave.S \
"

# If the --no-makefile flag is not used, we're going to put all build
# commands in a temporary Makefile that we will be able to invoke with
# -j$NUM_JOBS to build stuff in parallel.
#
if [ -z "$NO_MAKEFILE" ]; then
    MAKEFILE=$BUILD_DIR/Makefile
else
    MAKEFILE=
fi

# Define a few common variables based on parameters.
case $CXX_STL in
  stlport)
    CXX_STL_LIB=libstlport
    CXX_STL_SUBDIR=$STLPORT_SUBDIR
    CXX_STL_SRCDIR=$STLPORT_SRCDIR
    CXX_STL_CFLAGS=$STLPORT_CFLAGS
    CXX_STL_CXXFLAGS=$STLPORT_CXXFLAGS
    CXX_STL_LDFLAGS=$STLPORT_LDFLAGS
    CXX_STL_SOURCES=$STLPORT_SOURCES
    CXX_STL_PACKAGE=stlport
    ;;
  libc++)
    CXX_STL_LIB=libc++
    CXX_STL_SUBDIR=$LIBCXX_SUBDIR
    CXX_STL_SRCDIR=$LIBCXX_SRCDIR
    CXX_STL_CFLAGS=$LIBCXX_CFLAGS
    CXX_STL_CXXFLAGS=$LIBCXX_CXXFLAGS
    CXX_STL_LDFLAGS=$LIBCXX_LDFLAGS
    CXX_STL_SOURCES=$LIBCXX_SOURCES
    CXX_STL_PACKAGE=libcxx
    ;;
  *)
    panic "Internal error: Unknown STL name '$CXX_STL'"
    ;;
esac

HIDDEN_VISIBILITY_FLAGS="-fvisibility=hidden -fvisibility-inlines-hidden"

# By default, all static libraries include hidden ELF symbols, except
# if one uses the --visible-static option.
if [ -z "$VISIBLE_STATIC" ]; then
    STATIC_CONLYFLAGS="$HIDDEN_VISIBILITY_FLAGS"
    STATIC_CXXFLAGS="$HIDDEN_VISIBILITY_FLAGS"
else
    STATIC_CONLYFLAGS=
    STATIC_CXXFLAGS=
fi
SHARED_CONLYFLAGS="$HIDDEN_VISIBILITY_FLAGS"
SHARED_CXXFLAGS=


# build_stl_libs_for_abi
# $1: ABI
# $2: build directory
# $3: build type: "static" or "shared"
# $4: installation directory
# $5: (optional) thumb
build_stl_libs_for_abi ()
{
    local ARCH BINPREFIX SYSROOT
    local ABI=$1
    local BUILDDIR="$2"
    local TYPE="$3"
    local DSTDIR="$4"
    local DEFAULT_CFLAGS DEFAULT_CXXFLAGS
    local SRC OBJ OBJECTS EXTRA_CFLAGS EXTRA_CXXFLAGS EXTRA_LDFLAGS LIB_SUFFIX GCCVER

    EXTRA_CFLAGS=""
    EXTRA_CXXFLAGS=""
    EXTRA_LDFLAGS="-Wl,--build-id"

    case $ABI in
        arm64-v8a)
            EXTRA_CFLAGS="-mfix-cortex-a53-835769"
            EXTRA_CXXFLAGS="-mfix-cortex-a53-835769"
            ;;
        x86|x86_64)
            # ToDo: remove the following once all x86-based device call JNI function with
            #       stack aligned to 16-byte
            EXTRA_CFLAGS="-mstackrealign"
            EXTRA_CXXFLAGS="-mstackrealign"
            ;;
        mips)
            EXTRA_CFLAGS="-mips32"
            EXTRA_CXXFLAGS="-mips32"
            EXTRA_LDFLAGS="-mips32"
            ;;
        mips32r6)
            EXTRA_CFLAGS="-mips32r6"
            EXTRA_CXXFLAGS="-mips32r6"
            EXTRA_LDFLAGS="-mips32r6"
            ;;
        mips64)
            EXTRA_CFLAGS="-mips64r6"
            EXTRA_CXXFLAGS=$EXTRA_CFLAGS
            ;;
    esac

    USE_LLVM_UNWIND=
    case $ABI in
        armeabi*)
            EXTRA_CXXFLAGS="$EXTRA_CXXFLAGS -DLIBCXXABI_USE_LLVM_UNWINDER=1"
            USE_LLVM_UNWIND=true
            ;;
        *)
            EXTRA_CXXFLAGS="$EXTRA_CXXFLAGS -DLIBCXXABI_USE_LLVM_UNWINDER=0"
            ;;
    esac

    if [ "$ABI" != "${ABI%%arm*}" -a "$ABI" = "${ABI%%64*}" ] ; then
        EXTRA_CFLAGS="$EXTRA_CFLAGS -mthumb"
        EXTRA_CXXFLAGS="$EXTRA_CXXFLAGS -mthumb"
    fi

    if [ "$TYPE" = "static" ]; then
        EXTRA_CFLAGS="$EXTRA_CFLAGS $STATIC_CONLYFLAGS"
        EXTRA_CXXFLAGS="$EXTRA_CXXFLAGS $STATIC_CXXFLAGS"
    else
        EXTRA_CFLAGS="$EXTRA_CFLAGS $SHARED_CONLYFLAGS"
        EXTRA_CXXFLAGS="$EXTRA_CXXFLAGS $SHARED_CXXFLAGS"
    fi

    DSTDIR=$DSTDIR/$CXX_STL_SUBDIR/libs/$ABI
    LIB_SUFFIX="$(get_lib_suffix_for_abi $ABI)"

    mkdir -p "$BUILDDIR"
    mkdir -p "$DSTDIR"

    if [ -n "$GCC_VERSION" ]; then
        GCCVER=$GCC_VERSION
        EXTRA_CFLAGS="$EXTRA_CFLAGS -std=c99"
    else
        ARCH=$(convert_abi_to_arch $ABI)
        GCCVER=$(get_default_gcc_version_for_arch $ARCH)
    fi

    # libc++ built with clang (for ABI armeabi-only) produces
    # libc++_shared.so and libc++_static.a with undefined __atomic_fetch_add_4
    # Add -latomic.
    if [ -n "$LLVM_VERSION" -a "$CXX_STL_LIB" = "libc++" ]; then
        # clang3.5+ use integrated-as as default, which has trouble compiling
        # llvm-libc++abi/libcxxabi/src/Unwind/UnwindRegistersRestore.S
        EXTRA_CFLAGS="${EXTRA_CFLAGS} -no-integrated-as"
        EXTRA_CXXFLAGS="${EXTRA_CXXFLAGS} -no-integrated-as"
        if [ "$ABI" = "armeabi" ]; then
            EXTRA_LDFLAGS="$EXTRA_LDFLAGS -latomic"
        fi
    fi

    builder_begin_android $ABI "$BUILDDIR" "$GCCVER" "$LLVM_VERSION" "$MAKEFILE"

    builder_set_dstdir "$DSTDIR"
    builder_reset_cflags DEFAULT_CFLAGS
    builder_reset_cxxflags DEFAULT_CXXFLAGS

    if [ "$CXX_SUPPORT_LIB" = "gabi++" ]; then
        builder_set_srcdir "$GABIXX_SRCDIR"
        builder_cflags "$DEFAULT_CFLAGS $GABIXX_CFLAGS $EXTRA_CFLAGS"
        builder_cxxflags "$DEFAULT_CXXFLAGS $GABIXX_CXXFLAGS $EXTRA_CXXFLAGS"
        builder_ldflags "$GABIXX_LDFLAGS $EXTRA_LDFLAGS"
        builder_sources $GABIXX_SOURCES
    fi

    # Build the runtime sources, except if we're only building GAbi++
    if [ "$CXX_STL" != "gabi++" ]; then
      builder_set_srcdir "$CXX_STL_SRCDIR"
      builder_reset_cflags
      builder_cflags "$DEFAULT_CFLAGS $CXX_STL_CFLAGS $EXTRA_CFLAGS"
      builder_reset_cxxflags
      builder_cxxflags "$DEFAULT_CXXFLAGS $CXX_STL_CXXFLAGS $EXTRA_CXXFLAGS"
      builder_ldflags "$CXX_STL_LDFLAGS $EXTRA_LDFLAGS"
      builder_sources $CXX_STL_SOURCES
      if [ "$CXX_SUPPORT_LIB" = "libc++abi" ]; then
          if [ "$USE_LLVM_UNWIND" = "true" ]; then
              builder_sources $LIBCXXABI_SOURCES $LIBCXXABI_UNWIND_SOURCES
          else
              builder_sources $LIBCXXABI_SOURCES
          fi
          builder_ldflags "-ldl"
      fi
      if [ "$CXX_STL" = "libc++" ]; then
        if [ "$ABI" = "${ABI%%64*}" ]; then
          if [ "$ABI" = "x86" ]; then
            builder_sources $SUPPORT32_SOURCES $SUPPORT32_SOURCES_x86
          else
            builder_sources $SUPPORT32_SOURCES
	  fi
        else
          builder_sources $SUPPORT64_SOURCES
        fi
      fi
    fi

    if [ "$TYPE" = "static" ]; then
        log "Building $DSTDIR/${CXX_STL_LIB}_static.a"
        builder_static_library ${CXX_STL_LIB}_static
    else
        log "Building $DSTDIR/${CXX_STL_LIB}_shared${LIB_SUFFIX}"
        builder_shared_library ${CXX_STL_LIB}_shared $LIB_SUFFIX
    fi

    builder_end
}

for ABI in $ABIS; do
    build_stl_libs_for_abi $ABI "$BUILD_DIR/$ABI/static" "static" "$OUT_DIR"
    build_stl_libs_for_abi $ABI "$BUILD_DIR/$ABI/shared" "shared" "$OUT_DIR"
done

if [ -n "$PACKAGE_DIR" ] ; then
    if [ "$CXX_STL" = "libc++" ]; then
        STL_DIR="llvm-libc++"
    elif [ "$CXX_STL" = "stlport" ]; then
        STL_DIR="stlport"
    else
        panic "Unknown STL: $CXX_STL"
    fi

    make_repo_prop "$OUT_DIR/$CXX_STL_SUBDIR"
    PACKAGE="$PACKAGE_DIR/${CXX_STL_PACKAGE}.zip"
    log "Packaging: $PACKAGE"
    pack_archive "$PACKAGE" "$OUT_DIR/sources/cxx-stl" "$STL_DIR"
    fail_panic "Could not package $CXX_STL binaries!"
fi

if [ -z "$OPTION_BUILD_DIR" ]; then
    log "Cleaning up..."
    rm -rf $BUILD_DIR
else
    log "Don't forget to cleanup: $BUILD_DIR"
fi

log "Done!"
