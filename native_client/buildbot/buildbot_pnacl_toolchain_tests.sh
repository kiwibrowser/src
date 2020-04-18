#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Run toolchain torture tests and llvm testsuite tests.
# For now, run on linux64, build and run unsandboxed newlib tests
# for all 3 architectures.
# Note: This script builds the toolchain from scratch but does
#       not build the translators and hence the translators
#       are from an older revision, see comment below.

set -o xtrace
set -o nounset
set -o errexit

readonly PNACL_BUILD="pnacl/build.sh"
readonly TOOLCHAIN_BUILD="toolchain_build/toolchain_build_pnacl.py"
readonly PACKAGES_SCRIPT="buildbot/packages.py"
readonly TEMP_PACKAGES="toolchain_build/out/packages.txt"
readonly UP_DOWN_LOAD="buildbot/file_up_down_load.sh"
readonly TORTURE_TEST="tools/toolchain_tester/torture_test.py"
readonly LLVM_TEST="pnacl/scripts/llvm-test.py"
readonly CLANG_UPDATE="../tools/clang/scripts/update.py"
readonly INSTALL_SUBDIR="toolchain/linux_x86/pnacl_newlib"
readonly INSTALL_ABSPATH=$(pwd)/${INSTALL_SUBDIR}
readonly LIBRARY_ABSPATH=${INSTALL_ABSPATH}/lib

# build.sh, llvm test suite and torture tests all use this value
export PNACL_CONCURRENCY=${PNACL_CONCURRENCY:-4}

# Change the  toolchain build script (PNACL_BUILD) behavior slightly
# wrt to error logging and mecurial retry delays.
# TODO(robertm): if this special casing is still needed,
#                make this into separate vars
export PNACL_BUILDBOT=true
# Make the toolchain build script (PNACL_BUILD) more verbose.
# This will also prevent bot timeouts which otherwise gets triggered
# by long periods without console output.
export PNACL_VERBOSE=true

EXIT_STATUS=0

clobber() {
  echo @@@BUILD_STEP clobber@@@
  rm -rf scons-out
  # Don't clobber pnacl_translator; these bots currently don't build
  # it, but they use the DEPSed-in version.
  rm -rf toolchain/linux_x86/pnacl_newlib* \
      toolchain/mac_x86/pnacl_newlib* \
      toolchain/win_x86/pnacl_newlib*
}

handle-error() {
  echo "@@@STEP_FAILURE@@@"
  EXIT_STATUS=1
}

ignore-error() {
  echo "@==  IGNORING AN ERROR  ==@"
}

#### Support for running arm sbtc tests on this bot, since we have
# less coverage on the main waterfall now:
# http://code.google.com/p/nativeclient/issues/detail?id=2581
readonly SCONS_COMMON="./scons --verbose bitcode=1 -j${PNACL_CONCURRENCY}"
readonly SCONS_COMMON_SLOW="./scons --verbose bitcode=1 -j2"

build-run-prerequisites() {
  local platform=$1
  ${SCONS_COMMON} platform=${platform} sel_ldr irt_core elf_loader
}


scons-tests-translator() {
  local platform=$1
  local flags="--mode=opt-host,nacl use_sandboxed_translator=1 \
               platform=${platform} -k skip_trusted_tests=1"
  local targets="small_tests medium_tests large_tests"

  # ROUND 1: regular builds
  # generate pexes with full parallelism
  echo "@@@BUILD_STEP scons-sb-trans-pexe [${platform}] [${targets}]@@@"
  ${SCONS_COMMON} ${flags} ${targets} \
      translate_in_build_step=0 do_not_run_tests=1 || handle-error

  # translate pexes
  echo "@@@BUILD_STEP scons-sb-trans-trans [${platform}] [${targets}]@@@"
  if [[ ${platform} = arm ]] ; then
      # For ARM we use less parallelism to avoid mysterious QEMU crashes.
      # We also force a timeout for translation only.
      export QEMU_PREFIX_HOOK="timeout 120"
      # Run sb translation twice in case we failed to translate some of the
      # pexes.  If there was an error in the first run this shouldn't
      # trigger a buildbot error.  Only the second run can make the bot red.
      ${SCONS_COMMON_SLOW} ${flags} ${targets} \
          do_not_run_tests=1 || ignore-error
      ${SCONS_COMMON_SLOW} ${flags} ${targets} \
          do_not_run_tests=1 || handle-error
      # Do not use the prefix hook for running actual tests as
      # it will break some of them due to exit code sign inversion.
      unset QEMU_PREFIX_HOOK
  else
      ${SCONS_COMMON} ${flags} ${targets} \
          do_not_run_tests=1 || handle-error
  fi
  # finally run the tests
  echo "@@@BUILD_STEP scons-sb-trans-run [${platform}] [${targets}]@@@"
  ${SCONS_COMMON_SLOW} ${flags} ${targets} || handle-error

  # ROUND 2: builds with "fast translation"
  flags="${flags} translate_fast=1"
  echo "@@@BUILD_STEP scons-sb-trans-pexe [fast] [${platform}] [${targets}]@@@"
  ${SCONS_COMMON} ${flags} ${targets} \
      translate_in_build_step=0 do_not_run_tests=1 || handle-error

  echo "@@@BUILD_STEP scons-sb-trans-trans [fast] [${platform}] [${targets}]@@@"
  if [[ ${platform} = arm ]] ; then
      # For ARM we use less parallelism to avoid mysterious QEMU crashes.
      # We also force a timeout for translation only.
      export QEMU_PREFIX_HOOK="timeout 120"
      # Run sb translation twice in case we failed to translate some of the
      # pexes.  If there was an error in the first run this shouldn't
      # trigger a buildbot error.  Only the second run can make the bot red.
      ${SCONS_COMMON_SLOW} ${flags} ${targets} \
          do_not_run_tests=1 || ignore-error
      ${SCONS_COMMON_SLOW} ${flags} ${targets} \
          do_not_run_tests=1 || handle-error
      # Do not use the prefix hook for running actual tests as
      # it will break some of them due to exit code sign inversion.
      unset QEMU_PREFIX_HOOK
  else
      ${SCONS_COMMON} ${flags} ${targets} \
          do_not_run_tests=1 || handle-error
  fi
  echo "@@@BUILD_STEP scons-sb-trans-run [fast] [${platform}] [${targets}]@@@"
  ${SCONS_COMMON_SLOW} ${flags} ${targets} || handle-error
}

scons-tests-x86-64-zero-based-sandbox() {
  echo "@@@BUILD_STEP hello_world (x86-64 zero-based sandbox)@@@"
  local flags="--mode=opt-host,nacl platform=x86-64 \
               x86_64_zero_based_sandbox=1"
  ${SCONS_COMMON} ${flags} "run_hello_world_test"
}


tc-test-bot() {
  local archset="$1"
  clobber

  # Update Clang
  python ${CLANG_UPDATE}

  # Only build MIPS stuff on mips bots
  if [[ ${archset} == "mips" ]]; then
    export PNACL_BUILD_MIPS=true
    # Don't run any of the tests yet
    echo "MIPS bot: Only running build, and not tests"
    archset=
  fi

  # Build the un-sandboxed toolchain. The build script outputs its own buildbot
  # annotations.
  ${TOOLCHAIN_BUILD} --verbose --sync --clobber --testsuite-sync \
                     --packages-file ${TEMP_PACKAGES}

  # Extract the built packages using the packages script.
  python ${PACKAGES_SCRIPT} extract --skip-missing --packages ${TEMP_PACKAGES}

  # Linking the tests require additional sdk libraries like libnacl.
  # Do this once and for all early instead of attempting to do it within
  # each test step and having some late test steps rely on early test
  # steps building the prerequisites -- sometimes the early test steps
  # get skipped.
  echo "@@@BUILD_STEP install sdk libraries @@@"
  ${PNACL_BUILD} sdk
  for arch in ${archset}; do
    # Similarly, build the run prerequisites (sel_ldr and the irt) early.
    echo "@@@BUILD_STEP build run prerequisites [${arch}]@@@"
    build-run-prerequisites ${arch}
  done


  # Run the torture tests and compiler_rt tests.
  for arch in ${archset}; do
    echo "@@@BUILD_STEP pnacl bitcode compiler_rt tests@@@"
    export PNACL_RUN_ARCH=${arch}
    make -C toolchain_build/src/compiler-rt \
      -f lib/builtins/Makefile-pnacl-bitcode \
      TCROOT=${INSTALL_ABSPATH} nacltest-pnacl || handle-error

    # The CC arg is just a dummy to keep the make scripts from complaining
    # if clang is not found in PATH
    echo "@@@BUILD_STEP clang compiler_rt tests $arch @@@"
    make -C toolchain_build/src/compiler-rt TCROOT=${INSTALL_ABSPATH} \
      CC=gcc nacltest-${arch} || handle-error

    echo "@@@BUILD_STEP torture_tests_clang $arch @@@"
    ${TORTURE_TEST} clang ${arch} --verbose \
      --concurrency=${PNACL_CONCURRENCY} || handle-error

    if [[ "${arch}" == "x86-32" ]]; then
      # Torture tests on x86-32 are covered by tc-tests-all in
      # buildbot_pnacl.sh.
      continue
    fi
    echo "@@@BUILD_STEP torture_tests_pnacl $arch @@@"
    ${TORTURE_TEST} pnacl ${arch} --verbose \
      --concurrency=${PNACL_CONCURRENCY} || handle-error
  done


  local optset
  optset[1]="--opt O3f --opt O2b"
  for arch in ${archset}; do
    # Run all appropriate frontend/backend optimization combinations.  For now,
    # this means running 2 combinations for x86 (plus one more for Subzero on
    # x86-32) since each takes about 20 minutes on the bots, and making a single
    # run elsewhere since e.g. arm takes about 75 minutes.  In a perfect world,
    # all 4 combinations would be run, plus more for Subzero.
    if [[ ${archset} =~ x86 ]]; then
      optset[2]="--opt O3f --opt O0b"
      if [[ ${archset} == arm || ${archset} == x86-32 || \
            ${archset} == x86-64 ]]; then
        # Run a Subzero -O2 test set on x86-32.
        optset[3]="--opt O3f --opt O2b_sz"
      fi
    fi
    for opt in "${optset[@]}"; do
      echo "@@@BUILD_STEP llvm-test-suite ${arch} ${opt} @@@"
      python ${LLVM_TEST} --testsuite-clean
      LD_LIBRARY_PATH=${LIBRARY_ABSPATH} python ${LLVM_TEST} \
        --testsuite-configure --testsuite-run --testsuite-report \
        --arch ${arch} ${opt} -v -c || handle-error
    done

    echo "@@@BUILD_STEP libcxx-test ${arch} @@@"
    LD_LIBRARY_PATH=${LIBRARY_ABSPATH} python ${LLVM_TEST} \
      --libcxx-test --arch ${arch} -c || handle-error

    # Note: we do not build the sandboxed translator on this bot
    # because this would add another 20min to the build time.
    # The upshot of this is that we are using the sandboxed
    # toolchain which is currently deps'ed in.
    # There is a small upside here: we will notice that bitcode has
    # changed in a way that is incompatible with older translators.
    # TODO(pnacl-team): rethink this.
    # Note: the tests which use sandboxed translation are at the end,
    # because they can sometimes hang on arm, causing buildbot to kill the
    # script without running any more tests.
    scons-tests-translator ${arch}

    if [[ ${arch} = x86-64 ]] ; then
      scons-tests-x86-64-zero-based-sandbox
    fi

  done
  exit $EXIT_STATUS
}


if [ $# = 0 ]; then
  # NOTE: this is used for manual testing only
  tc-test-bot "x86-64 x86-32 arm"
else
  "$@"
fi
