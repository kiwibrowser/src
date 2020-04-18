#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
set -o nounset
set -o errexit

# Script for building just llc and reducing its memory footprint

readonly CC=gcc
readonly CXX=g++

readonly MAKE_OPTS="-j12 VERBOSE=1"

enable-32bit() {
    readonly CFLAGS=-m32
    readonly CXXFLAGS=-m32
    readonly LDFLAGS=-m32
}

readonly PNACL_ROOT="$(pwd)/pnacl"
readonly TC_BUILD="${PNACL_ROOT}/build"
readonly TC_BUILD_LLVM="${TC_BUILD}/llc"

readonly TC_SRC="${PNACL_ROOT}/src"
readonly TC_SRC_UPSTREAM="${TC_SRC}/upstream"
readonly TC_SRC_LLVM="${TC_SRC_UPSTREAM}/llvm"

readonly LLVM_EXTRA_OPTIONS="--enable-optimized"

readonly CROSS_TARGET_ARM=arm-none-linux-gnueabi


llvm-unlink-clang() {
  if [ -d "${TC_SRC_LLVM}" ]; then
    rm -f "${TC_SRC_LLVM}"/tools/clang
  fi
}


llc-configure() {
  local srcdir="${TC_SRC_LLVM}"
  local objdir="${TC_BUILD_LLVM}"

  rm -rf "${objdir}"
  mkdir -p "${objdir}"
  pushd "${objdir}"

  # Some components like ARMDisassembler may time out when built with -O3.
  # If we ever start using MSVC, we may also need to tone down the opt level
  # (see the settings in the CMake file for those components).
  local llvm_extra_opts=${LLVM_EXTRA_OPTIONS}

  llvm-unlink-clang
  # The --with-binutils-include is to allow llvm to build the gold plugin
  env -i PATH=/usr/bin/:/bin \
             MAKE_OPTS=${MAKE_OPTS} \
             CC="${CC}" \
             CXX="${CXX}" \
             CFLAGS="${CFLAGS}" \
             CXXFLAGS="${CXXFLAGS}" \
             LDFLAGS="${LDFLAGS}" \
             ${srcdir}/configure \
             --disable-jit \
             --enable-static \
             --enable-targets=x86,x86_64,arm \
             --target=${CROSS_TARGET_ARM} \
             ${llvm_extra_opts}
  popd
}


llc-make() {
  local srcdir="${TC_SRC_LLVM}"
  local objdir="${TC_BUILD_LLVM}"

  pushd "${objdir}"

  llvm-unlink-clang

  env -i PATH=/usr/bin/:/bin \
           MAKE_OPTS="${MAKE_OPTS}" \
           PNACL_BROWSER_TRANSLATOR=0 \
           NACL_SB_JIT=0 \
           CC="${CC}" \
           CXX="${CXX}" \
           CFLAGS="${CFLAGS}" \
           CXXFLAGS="${CXXFLAGS}" \
           LDFLAGS="${LDFLAGS}" \
           make ${MAKE_OPTS} tools-only
  popd
}


llc-install() {
  # This just works on linux and assumes release builds...
  # TODO(pnacl-team): Make this path configurable.
  cp ${TC_BUILD_LLVM}/Release+Asserts/bin/llc \
    toolchain/linux_x86/pnacl_newlib_raw/host_x86_32/bin/llc
}


llc-run-x8632() {
  local pexe=$1
  ${TC_BUILD_LLVM}/Release+Asserts/bin/llc \
      -mcpu=pentium4 \
      -mtriple=i686-none-nacl-gnu \
      -filetype=obj \
      -streaming-bitcode \
      -tail-merge-threshold=50 \
      ${pexe} \
      -o ${pexe}.o \
      -metadata-text ${pexe}.meta
}

# to use tc malloc, get it from here and configure it like so:
# https://code.google.com/p/gperftools/
# CXXFLAGS=-m32   LDFLAGS=-m32 CFLAGS=-m32   ./configure
llc-run-x8632-heapprofile() {
   export LD_PRELOAD="${TCMALLOC_SO}"
   export HEAPPROFILE="$1.heapprofile"
   llc-run-x8632 "$@"
}

enable-32bit

"$@"
