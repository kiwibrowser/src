#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -o xtrace
set -o nounset
set -o errexit

# Tell build.sh and test.sh that we're a bot.
export PNACL_BUILDBOT=true
# Make TC bots print all log output to console (this variable only affects
# build.sh output)
export PNACL_VERBOSE=true

# This affects trusted components of scons builds.
# The setting OPT reuslts in optimized/release trusted executables,
# the setting DEBUG results in unoptimized/debug trusted executables
BUILD_MODE_HOST=OPT
# If true, terminate script when first scons error is encountered.
FAIL_FAST=${FAIL_FAST:-true}
# This remembers when any build steps failed, but we ended up continuing.
RETCODE=0

readonly UP_DOWN_LOAD="buildbot/file_up_down_load.sh"
# This script is used by toolchain bots (i.e. tc-xxx functions)
readonly PNACL_BUILD="pnacl/build.sh"
readonly DRIVER_TESTS="pnacl/driver/tests/driver_tests.py"

# called when a scons invocation fails
handle-error() {
  RETCODE=1
  echo "@@@STEP_FAILURE@@@"
  if ${FAIL_FAST} ; then
    echo "FAIL_FAST enabled"
    exit 1
  fi
}

# Clear out object, and temporary directories.
clobber() {
  echo "@@@BUILD_STEP clobber@@@"
  rm -rf scons-out ../xcodebuild ../out
}

# Generate filenames for arm bot uploads and downloads
NAME_ARM_UPLOAD() {
  echo -n "${BUILDBOT_BUILDERNAME}/${BUILDBOT_GOT_REVISION}"
}

NAME_ARM_DOWNLOAD() {
  echo -n "${BUILDBOT_TRIGGERED_BY_BUILDERNAME}/${BUILDBOT_GOT_REVISION}"
}

NAME_ARM_TRY_UPLOAD() {
  echo -n "${BUILDBOT_BUILDERNAME}/"
  echo -n "${BUILDBOT_SLAVENAME}/"
  echo -n "${BUILDBOT_BUILDNUMBER}"
}
NAME_ARM_TRY_DOWNLOAD() {
  echo -n "${BUILDBOT_TRIGGERED_BY_BUILDERNAME}/"
  echo -n "${BUILDBOT_TRIGGERED_BY_SLAVENAME}/"
  echo -n "${BUILDBOT_TRIGGERED_BY_BUILDNUMBER}"
}

# Remove unneeded intermediate object files from the output to reduce the size
# of the tarball we copy between builders. The exception for the /lib directory
# is because nacl-clang needs crt{1,i,n}.o
prune-scons-out() {
  find scons-out/ \
    \( ! -path '*/lib/*' -a -name '*.o' -o -name '*.bc' -o \
    -name 'test_results' \) \
    -print0 | xargs -t -0 rm -rf
}

# Tar up the executables which are shipped to the arm HW bots
archive-for-hw-bots() {
  local name=$1
  local try=$2

  echo "@@@BUILD_STEP tar_generated_binaries@@@"
  prune-scons-out

  # delete nexes from pexe mode directories to force translation
  # TODO(dschuff) enable this once we can translate on the hw bots
  #find scons-out/*pexe*/ -name '*.nexe' -print0 | xargs -0 rm -f
  tar cvfz arm-scons.tgz scons-out/*arm*

  echo "@@@BUILD_STEP archive_binaries@@@"
  if [[ ${try} == "try" ]] ; then
    ${UP_DOWN_LOAD} UploadArmBinariesForHWBotsTry ${name} arm-scons.tgz
  else
    ${UP_DOWN_LOAD} UploadArmBinariesForHWBots ${name} arm-scons.tgz
  fi
}

# Untar archived executables for HW bots
unarchive-for-hw-bots() {
  local name=$1
  local try=$2

  echo "@@@BUILD_STEP fetch_binaries@@@"
  if [[ ${try} == "try" ]] ; then
    ${UP_DOWN_LOAD} DownloadArmBinariesForHWBotsTry ${name} arm-scons.tgz
  else
    ${UP_DOWN_LOAD} DownloadArmBinariesForHWBots ${name} arm-scons.tgz
  fi

  echo "@@@BUILD_STEP untar_binaries@@@"
  rm -rf scons-out/
  tar xvfz arm-scons.tgz --no-same-owner
}

# QEMU upload bot runs this function, and the hardware download bot runs
# mode-buildbot-arm-hw
mode-buildbot-arm() {
  clobber

  # Don't run the tests on qemu, only build them.
  # QEMU is too flaky for the main waterfall
  local mode
  if [ "${BUILD_MODE_HOST}" = "DEBUG" ] ; then
    mode="dbg"
  else
    mode="opt"
  fi
  buildbot/buildbot_pnacl.py --skip-run ${mode} arm pnacl
}

mode-buildbot-arm-hw() {
  local mode
  if [ "${BUILD_MODE_HOST}" = "DEBUG" ] ; then
    mode="dbg"
  else
    mode="opt"
  fi
  buildbot/buildbot_pnacl.py --skip-build ${mode} arm pnacl
}

mode-trybot-qemu() {
  clobber
  local arch=$1

  buildbot/buildbot_pnacl.py opt $arch pnacl
}

mode-buildbot-arm-dbg() {
  BUILD_MODE_HOST=DEDUG
  mode-buildbot-arm
  archive-for-hw-bots $(NAME_ARM_UPLOAD) regular
}

mode-buildbot-arm-opt() {
  mode-buildbot-arm
  archive-for-hw-bots $(NAME_ARM_UPLOAD) regular
}

mode-buildbot-arm-try() {
  mode-buildbot-arm
  archive-for-hw-bots $(NAME_ARM_TRY_UPLOAD) try
}

# NOTE: the hw bots are too slow to build stuff on so we just
#       use pre-built executables
mode-buildbot-arm-hw-dbg() {
  BUILD_MODE_HOST=DEDUG
  unarchive-for-hw-bots $(NAME_ARM_DOWNLOAD)  regular
  mode-buildbot-arm-hw
}

mode-buildbot-arm-hw-opt() {
  unarchive-for-hw-bots $(NAME_ARM_DOWNLOAD)  regular
  mode-buildbot-arm-hw
}

mode-buildbot-arm-hw-try() {
  unarchive-for-hw-bots $(NAME_ARM_TRY_DOWNLOAD)  try
  mode-buildbot-arm-hw
}

# These 2 functions are also suitable for local TC sanity testing.
tc-tests-all() {
  # All the SCons tests (the same ones run by the main waterfall bot)
  for arch in arm 32 64; do
    buildbot/buildbot_pnacl.py opt "${arch}" pnacl || handle-error
  done

  # Run the GCC torture tests just for x86-32.  Testing a single
  # architecture gives good coverage without taking too long.  We
  # don't test x86-64 here because some of the torture tests fail on
  # the x86-64 toolchain trybot (though not the buildbots, apparently
  # due to a hardware difference:
  # https://code.google.com/p/nativeclient/issues/detail?id=3697).

  # Build the SDK libs first so that linking will succeed.
  echo "@@@BUILD_STEP sdk libs @@@"
  ${PNACL_BUILD} sdk
  ./scons --verbose platform=x86-32 -j8 sel_ldr irt_core

  echo "@@@BUILD_STEP torture_tests x86-32 @@@"
  tools/toolchain_tester/torture_test.py pnacl x86-32 --verbose \
      --concurrency=8 || handle-error
}

tc-tests-fast() {
  local arch="$1"

  buildbot/buildbot_pnacl.py opt 32 pnacl
}

mode-buildbot-tc-x8664-linux() {
  FAIL_FAST=false
  export PNACL_TOOLCHAIN_DIR=linux_x86/pnacl_newlib

  HOST_ARCH=x86_64 tc-tests-all
}

mode-buildbot-tc-x8632-linux() {
  FAIL_FAST=false
  export PNACL_TOOLCHAIN_DIR=linux_x86/pnacl_newlib

  # For now, just use this bot to test a pure 32 bit build.
  HOST_ARCH=x86_32 tc-tests-fast "x86-32"
}


######################################################################
# On Windows, this script is invoked from a batch file.
# The inherited PWD environmental variable is a Windows-style path.
# This can cause problems with pwd and bash. This line fixes it.
cd -P .

# Script assumed to be run in native_client/
if [[ $(pwd) != */native_client ]]; then
  echo "ERROR: must be run in native_client!"
  exit 1
fi


if [[ $# -eq 0 ]] ; then
  echo "you must specify a mode on the commandline:"
  exit 1
fi

if [ "$(type -t $1)" != "function" ]; then
  Usage
  echo "ERROR: unknown mode '$1'." >&2
  exit 1
fi

"$@"

if [[ ${RETCODE} != 0 ]]; then
  echo "@@@BUILD_STEP summary@@@"
  echo There were failed stages.
  exit ${RETCODE}
fi
