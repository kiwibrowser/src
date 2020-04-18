#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -o xtrace
set -o nounset
set -o errexit

######################################################################
# SCRIPT CONFIG
######################################################################

readonly CLOBBER=${CLOBBER:-yes}
readonly SCONS_TRUSTED="./scons --mode=opt-host -j8"
readonly SCONS_NACL="./scons --mode=opt-host,nacl -j8"
readonly SPEC_HARNESS=${SPEC_HARNESS:-$(pwd)/out/cpu2000}/

readonly TRYBOT_TESTS="176.gcc 179.art 181.mcf 197.parser 252.eon 254.gap"
readonly TRYBOT_X86_64_ZERO_BASED_SANDBOX_TESTS="176.gcc"

readonly BUILDBOT_PNACL="buildbot/buildbot_pnacl.sh"
readonly UP_DOWN_LOAD="buildbot/file_up_down_load.sh"

readonly SPEC_BASE="tests/spec2k"
readonly ARCHIVE_NAME=$(${SPEC_BASE}/run_all.sh GetTestArchiveName)

readonly NAME_ARM_TRY_UPLOAD=$(${BUILDBOT_PNACL} NAME_ARM_TRY_UPLOAD)
readonly NAME_ARM_TRY_DOWNLOAD=$(${BUILDBOT_PNACL} NAME_ARM_TRY_DOWNLOAD)
readonly NAME_ARM_UPLOAD=$(${BUILDBOT_PNACL} NAME_ARM_UPLOAD)
readonly NAME_ARM_DOWNLOAD=$(${BUILDBOT_PNACL} NAME_ARM_DOWNLOAD)

readonly QEMU_TOOL="$(pwd)/toolchain/linux_x86/arm_trusted/run_under_qemu_arm"

# Note: the tool for updating the canned nexes lives at:
#        tools/canned_nexe_tool.sh
readonly CANNED_NEXE_REV=1002

# If true, terminate script when first error is encountered.
readonly FAIL_FAST=${FAIL_FAST:-false}
RETCODE=0

# Print the number of tests being run for the buildbot status output
testcount() {
  local tests="$1"
  if [[ ${tests} == "all" ]]; then
    echo "all"
  else
    echo ${tests} | wc -w
  fi
}

# called when a commands invocation fails
handle-error() {
  RETCODE=1
  echo "@@@STEP_FAILURE@@@"
  if ${FAIL_FAST} ; then
    echo "FAIL_FAST enabled"
    exit 1
  fi
}

######################################################################
# SCRIPT ACTION
######################################################################

clobber-scons() {
  if [ "${CLOBBER}" == "yes" ] ; then
    rm -rf scons-out
  fi
}

clobber-harness() {
  if [ "${CLOBBER}" == "yes" ] ; then
    rm -rf out
  fi
}

clobber() {
  clobber-scons
  clobber-harness
}

download-spec2k-harness() {
  echo "@@@BUILD_STEP Download spec2k harness@@@"
  mkdir -p out
  ${NATIVE_PYTHON} ${GSUTIL} cp -a public-read \
    gs://nativeclient-private/cpu2000.tar.bz2 out/cpu2000.tar.bz2
  tar xvj -C out -f out/cpu2000.tar.bz2
}

# Make up for the toolchain tarballs not quite being a full SDK
# Also clean the SPEC dir (that step is here because it should
# not be run on hw bots which download rather than build binaries)
build-prerequisites() {
  echo "@@@BUILD_STEP build prerequisites [$*] @@@"
  pushd ${SPEC_BASE}
  ./run_all.sh BuildPrerequisites "$@"
  ./run_all.sh CleanBenchmarks
  ./run_all.sh PopulateFromSpecHarness "${SPEC_HARNESS}"
  popd
}

build-tests() {
  local setups="$1"
  local tests="$2"
  local timed="$3" # Do timing and size measurements
  local compile_repetitions="$4"
  local count=$(testcount "${tests}")

  pushd ${SPEC_BASE}
  for setup in ${setups}; do
    echo "@@@BUILD_STEP spec2k build [${setup}] [${count} tests]@@@"
    MAKEOPTS=-j8 \
    SPEC_COMPILE_REPETITIONS=${compile_repetitions} \
      ./run_all.sh BuildBenchmarks ${timed} ${setup} train ${tests} || \
        handle-error
  done
  popd
}

run-tests() {
  local setups="$1"
  local tests="$2"
  local timed="$3"
  local run_repetitions="$4"
  local count=$(testcount "${tests}")

  pushd ${SPEC_BASE}
  for setup in ${setups}; do
    echo "@@@BUILD_STEP spec2k run [${setup}] [${count} tests]@@@"
    if [ ${timed} == "1" ]; then
      SPEC_RUN_REPETITIONS=${run_repetitions} \
        ./run_all.sh RunTimedBenchmarks ${setup} train ${tests} || \
          handle-error
    else
      ./run_all.sh RunBenchmarks ${setup} train ${tests} || \
        handle-error
    fi
  done
  popd
}

upload-test-binaries() {
  local tests="$1"
  local try="$2" # set to "try" if this is a try run

  pushd ${SPEC_BASE}
  echo "@@@BUILD_STEP spec2k archive@@@"
  ./run_all.sh PackageArmBinaries ${tests}
  popd
  echo "@@@BUILD_STEP spec2k upload@@@"
  if [[ ${try} == "try" ]]; then
    ${UP_DOWN_LOAD} UploadArmBinariesForHWBotsTry ${NAME_ARM_TRY_UPLOAD} \
        ${ARCHIVE_NAME}
  else
    ${UP_DOWN_LOAD} UploadArmBinariesForHWBots ${NAME_ARM_UPLOAD} \
        ${ARCHIVE_NAME}
  fi
}

download-test-binaries() {
  local try="$1"
  echo "@@@BUILD_STEP spec2k download@@@"
  if [[ ${try} == "try" ]]; then
    ${UP_DOWN_LOAD} DownloadArmBinariesForHWBotsTry ${NAME_ARM_TRY_DOWNLOAD} \
        ${ARCHIVE_NAME}
  else
    ${UP_DOWN_LOAD} DownloadArmBinariesForHWBots ${NAME_ARM_DOWNLOAD} \
        ${ARCHIVE_NAME}
  fi
  echo "@@@BUILD_STEP spec2k untar@@@"
  pushd ${SPEC_BASE}
  ./run_all.sh UnpackArmBinaries
  popd
}

download-validator-test-nexes() {
  local arch="$1"
  echo "@@@BUILD_STEP validator test download@@@"
  ${UP_DOWN_LOAD} DownloadArchivedNexes ${CANNED_NEXE_REV} \
      "${arch}_giant" giant_nexe.tar.bz2
  # This generates "CannedNexes/" in the current directory
  rm -rf CannedNexes
  tar jxf giant_nexe.tar.bz2
}

get-validator() {
  local arch="$1"
  if [[ ${arch} == "x86-32" ]] ; then
    echo "$(pwd)/scons-out/opt-linux-x86-32/staging/ncval_new"
  elif [[ ${arch} == "x86-64" ]] ; then
    echo "$(pwd)/scons-out/opt-linux-x86-64/staging/ncval_new"
  elif [[ ${arch} == "arm" ]] ; then
    echo "$(pwd)/scons-out/opt-linux-arm/staging/arm-ncval-core"
  else
    echo "ERROR: unknown arch"
    exit 1
  fi
}

LogTimeHelper() {
  # This format is recognized by the buildbot system
  echo "RESULT $1_$2: $3= $(bc) seconds"
}

LogTimedRun() {
  local graph=$1
  local benchmark=$2
  local variant=$3
  shift 3
  # S: system mode CPU-seconds used by the process
  # U: user mode CPU-seconds  used by the process
  # We add a plus sign inbetween so that we can pipe the output to "bc"
  # Note: the  >() magic creates a "fake" file (think named pipe)
  #       which passes the output of time to LogTimeHelper
  /usr/bin/time -f "%U + %S" \
      --output >(LogTimeHelper ${graph} ${benchmark} ${variant}) \
      "$@"
}

build-validator() {
  local arch="$1"
  echo "@@@BUILD_STEP build validator [${arch}]@@@"
  if [[ ${arch} == "arm" ]] ; then
    # TODO(robertm): build the validator
    echo "NYI"
  elif [[ ${arch} == "x86-32" ]] ; then
    ${SCONS_NACL} platform=${arch} ncval_new
  elif [[ ${arch} == "x86-64" ]] ; then
    ${SCONS_NACL} platform=${arch} ncval_new
  else
    echo "ERROR: unknown arch"
    exit 1
  fi
}

measure-validator-speed() {
  local arch="$1"
  local validator=$(get-validator ${arch})

  echo "@@@BUILD_STEP validator speed test [${arch}]@@@"
  if [[ ! -e ${validator} ]] ; then
    echo "ERROR: missing validator executable: ${validator}"
    handle-error
    return
  fi

  if [[ ${arch} == "arm" && $(uname -p) != arm* ]] ; then
    # TODO(robertm): build the validator
    validator="${QEMU_TOOL} ${validator}"
  fi

  for nexe in CannedNexes/* ; do
    echo "timing validation of ${nexe}"
    ls --size --block-size=1 ${nexe}
    LogTimedRun  "validationtime" $(basename ${nexe}) "canned" \
        ${validator} ${nexe}
  done
}

######################################################################
# NOTE: trybots only runs a subset of the the spec2k tests

pnacl-trybot-arm-buildonly() {
  clobber
  download-spec2k-harness
  build-prerequisites "arm" "bitcode" "arm-ncval-core"
  ${BUILDBOT_PNACL} archive-for-hw-bots "${NAME_ARM_TRY_UPLOAD}" try
  build-tests SetupPnaclPexeOpt "${TRYBOT_TESTS}" 0 1
  upload-test-binaries "${TRYBOT_TESTS}" try
}

pnacl-trybot-arm-hw() {
  clobber
  ${BUILDBOT_PNACL} unarchive-for-hw-bots "${NAME_ARM_TRY_DOWNLOAD}" try
  download-test-binaries try
  build-tests SetupPnaclTranslatorArmOptHW "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslatorArmOptHW "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclTranslatorArmOptSzHW "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslatorArmOptSzHW "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclTranslator1ThreadArmOptHW "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslator1ThreadArmOptHW "${TRYBOT_TESTS}" 1 1
  pushd ${SPEC_BASE};
  ./run_all.sh TimeValidation SetupPnaclTranslatorArmOptHW "${TRYBOT_TESTS}" ||\
    handle-error
  popd
  build-tests SetupPnaclTranslatorFastArmOptHW "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslatorFastArmOptHW "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclTranslatorFastArmOptSzHW "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslatorFastArmOptSzHW "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclTranslatorFast1ThreadArmOptHW "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslatorFast1ThreadArmOptHW "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclTranslatorFast1ThreadArmOptSzHW "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslatorFast1ThreadArmOptSzHW "${TRYBOT_TESTS}" 1 1
}

pnacl-trybot-x8632() {
  clobber
  download-spec2k-harness
  build-prerequisites "x86-32" "bitcode"
  build-tests SetupPnaclX8632Opt "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclX8632Opt "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclX8632OptSz "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclX8632OptSz "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclTranslatorX8632Opt "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslatorX8632Opt "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclTranslator1ThreadX8632Opt "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslator1ThreadX8632Opt "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclTranslatorFastX8632Opt "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslatorFastX8632Opt "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclTranslatorFastX8632OptSz "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslatorFastX8632OptSz "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclTranslatorFast1ThreadX8632Opt "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslatorFast1ThreadX8632Opt "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclTranslatorFast1ThreadX8632OptSz "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslatorFast1ThreadX8632OptSz "${TRYBOT_TESTS}" 1 1
  build-validator x86-32
  download-validator-test-nexes x86-32
  measure-validator-speed x86-32
}

pnacl-x86-64-zero-based-sandbox() {
  # Clobber scons and rebuild sel_ldr with zero-based sandbox support.
  # If sel_ldr was built to a different directory and the test
  # runner knew where to find that separate sel_ldr, then we wouldn't
  # need to clobber here.
  clobber-scons
  export NACL_ENABLE_INSECURE_ZERO_BASED_SANDBOX=1
  build-prerequisites "x86-64" "bitcode" "x86_64_zero_based_sandbox=1"
  build-tests SetupPnaclX8664ZBSOpt \
    "${TRYBOT_X86_64_ZERO_BASED_SANDBOX_TESTS}" 1 1
  run-tests SetupPnaclX8664ZBSOpt \
    "${TRYBOT_X86_64_ZERO_BASED_SANDBOX_TESTS}" 1 1
}

pnacl-trybot-x8664() {
  clobber
  download-spec2k-harness
  build-prerequisites "x86-64" "bitcode"
  build-tests SetupPnaclX8664Opt "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclX8664Opt "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclX8664OptSz "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclX8664OptSz "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclTranslatorX8664Opt "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslatorX8664Opt "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclTranslator1ThreadX8664Opt "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslator1ThreadX8664Opt "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclTranslatorFastX8664Opt "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslatorFastX8664Opt "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclTranslatorFastX8664OptSz "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslatorFastX8664OptSz "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclTranslatorFast1ThreadX8664Opt "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslatorFast1ThreadX8664Opt "${TRYBOT_TESTS}" 1 1
  build-tests SetupPnaclTranslatorFast1ThreadX8664OptSz "${TRYBOT_TESTS}" 1 1
  run-tests SetupPnaclTranslatorFast1ThreadX8664OptSz "${TRYBOT_TESTS}" 1 1
  pnacl-x86-64-zero-based-sandbox
  build-validator x86-64
  download-validator-test-nexes x86-64
  measure-validator-speed x86-64
}

pnacl-arm-buildonly() {
  clobber
  download-spec2k-harness
  build-prerequisites "arm" "bitcode"
  ${BUILDBOT_PNACL} archive-for-hw-bots "${NAME_ARM_UPLOAD}" regular
  build-tests SetupPnaclPexeOpt all 0 1
  upload-test-binaries all regular
}

pnacl-arm-hw() {
  clobber
  ${BUILDBOT_PNACL} unarchive-for-hw-bots "${NAME_ARM_DOWNLOAD}" regular
  download-test-binaries regular
  build-tests SetupPnaclTranslatorArmOptHW all 1 1
  run-tests SetupPnaclTranslatorArmOptHW all 1 2
  build-tests SetupPnaclTranslatorArmOptSzHW all 1 1
  run-tests SetupPnaclTranslatorArmOptSzHW all 1 2
  # Only run 1 thread ARM tests 1x to save some time for now.
  # Hopefully perf infrastructure will smooth out flakes.
  # Otherwise, we'll bump the runs up to 2x as well.
  build-tests SetupPnaclTranslator1ThreadArmOptHW all 1 1
  run-tests SetupPnaclTranslator1ThreadArmOptHW all 1 1
  build-tests SetupPnaclTranslatorFastArmOptHW all 1 1
  run-tests SetupPnaclTranslatorFastArmOptHW all 1 2
  # Only run 1 thread ARM tests 1x to save some time for now.
  # Hopefully perf infrastructure will smooth out flakes.
  # Otherwise, we'll bump the runs up to 2x as well.
  build-tests SetupPnaclTranslatorFast1ThreadArmOptHW all 1 1
  run-tests SetupPnaclTranslatorFast1ThreadArmOptHW all 1 1
  build-tests SetupPnaclTranslatorFast1ThreadArmOptSzHW all 1 1
  run-tests SetupPnaclTranslatorFast1ThreadArmOptSzHW all 1 1
}

pnacl-x8664() {
  clobber
  download-spec2k-harness
  build-prerequisites "x86-64" "bitcode"
  local setups="SetupPnaclX8664Opt \
                SetupPnaclX8664OptSz \
                SetupPnaclTranslatorX8664Opt \
                SetupPnaclTranslator1ThreadX8664Opt \
                SetupPnaclTranslatorFastX8664Opt \
                SetupPnaclTranslatorFastX8664OptSz \
                SetupPnaclTranslatorFast1ThreadX8664OptSz \
                SetupPnaclTranslatorFast1ThreadX8664Opt"
  build-tests "${setups}" all 1 3
  run-tests "${setups}" all 1 3
  pnacl-x86-64-zero-based-sandbox
  build-validator x86-64
  download-validator-test-nexes x86-64
  measure-validator-speed x86-64
}

pnacl-x8632() {
  clobber
  download-spec2k-harness
  build-prerequisites "x86-32" "bitcode"
  local setups="SetupPnaclX8632Opt \
                SetupPnaclX8632OptSz \
                SetupPnaclTranslatorX8632Opt \
                SetupPnaclTranslator1ThreadX8632Opt \
                SetupPnaclTranslatorFastX8632OptSz \
                SetupPnaclTranslatorFastX8632Opt \
                SetupPnaclTranslatorFast1ThreadX8632Opt \
                SetupPnaclTranslatorFast1ThreadX8632OptSz"
  build-tests "${setups}" all 1 3
  run-tests "${setups}" all 1 3
  build-validator x86-32
  download-validator-test-nexes x86-32
  measure-validator-speed x86-32
}

nacl-x8632() {
  clobber
  download-spec2k-harness
  build-prerequisites "x86-32" ""
  local setups="SetupNaclX8632 \
                SetupNaclX8632Opt"
  build-tests "${setups}" all 1 3
  run-tests "${setups}" all 1 3
  build-validator x86-32
  download-validator-test-nexes x86-32
  measure-validator-speed x86-32
}

nacl-x8664() {
  clobber
  download-spec2k-harness
  build-prerequisites "x86-64" ""
  local setups="SetupNaclX8664 \
                SetupNaclX8664Opt"
  build-tests "${setups}" all 1 3
  run-tests "${setups}" all 1 3
  build-validator x86-64
  download-validator-test-nexes x86-64
  measure-validator-speed x86-64
}


######################################################################
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
