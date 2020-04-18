#!/bin/bash

# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -o nounset
set -o errexit

# The script is located in "native_client/tests/spec2k"
# Set pwd to spec2k/
cd "$(dirname "$0")"
if [[ $(basename "$(pwd)") != "spec2k" ]] ; then
  echo "ERROR: cannot find the spec2k/ directory"
  exit -1
fi

# TODO(pdox): Remove this dependency.
source ../../pnacl/scripts/common-tools.sh
readonly NACL_ROOT="$(GetAbsolutePath "../../")"
SetScriptPath "$(pwd)/run_all.sh"
SetLogDirectory "${NACL_ROOT}/toolchain/test-log"
readonly TESTS_ARCHIVE=arm-spec.tar.gz

######################################################################
# CONFIGURATION
######################################################################
# TODO(robertm): make this configurable from the commandline

readonly LIST_INT_C="164.gzip 175.vpr 176.gcc 181.mcf 186.crafty 197.parser \
253.perlbmk 254.gap 255.vortex 256.bzip2 300.twolf"

readonly LIST_FP_C="177.mesa 179.art 183.equake 188.ammp"

readonly LIST_INT_CPP="252.eon"

SPEC2K_BENCHMARKS="${LIST_FP_C} ${LIST_INT_C} ${LIST_INT_CPP}"

# One of {./run.train.sh, ./run.ref.sh}
SPEC2K_SCRIPT="./run.train.sh"

# Uncomment this to disable verification of test results.
# Otherwise, verification time is part of overall benchmarking time
# (should probably separate that out).
# export VERIFY=no
export VERIFY=${VERIFY:-yes}
export MAKEOPTS=${MAKEOPTS:-}

# Helper script to process timing / other perf data.
# Export these paths to Makefile.common, which will be included by other
# Makefiles in random sub-directories (where pwd will be different).
export PERF_LOGGER="$(pwd)/emit_perf_log.sh"
export COMPILE_REPEATER="$(pwd)/compile_repeater.sh"
# Number of times to repeat a timed step.
export SPEC_RUN_REPETITIONS=${SPEC_RUN_REPETITIONS:-1}
export SPEC_COMPILE_REPETITIONS=${SPEC_COMPILE_REPETITIONS:-1}

export DASHDASH=""

######################################################################
# Helper
######################################################################

readonly SCONS_OUT="${NACL_ROOT}/scons-out"
readonly TC_ROOT="${NACL_ROOT}/toolchain"
readonly TC_BASE="${TC_ROOT}/${SCONS_BUILD_PLATFORM}_${BUILD_ARCH_SHORT}"

readonly ARM_TRUSTED_TC="${TC_BASE}/arm_trusted"
readonly QEMU_TOOL="${ARM_TRUSTED_TC}/run_under_qemu_arm"

readonly ARM_LLC_NEXE="${TC_BASE}/pnacl_translator/translator/arm/bin/pnacl-llc.nexe"

readonly NNACL_TC="${TC_BASE}/nacl_${BUILD_ARCH_SHORT}_glibc"
readonly RUNNABLE_LD_X8632="${NNACL_TC}/x86_64-nacl/lib32/runnable-ld.so"
readonly RUNNABLE_LD_X8664="${NNACL_TC}/x86_64-nacl/lib/runnable-ld.so"

echo_file_size() {
  local file=$1
  echo "Uncompressed size of ${file} is $(cat ${file} | wc -c)"
  echo "Gzipped size of ${file} is $(gzip ${file} -c | wc -c)"
}

######################################################################
# Various Setups
######################################################################
#@ invocation
#@    run_all.sh <mode> <mode-arg>*

#@ ------------------------------------------------------------
#@ Available Setups:
#@ ------------------------------------------------------------

# Setups for building (but not running) Pexes only. Used for arm-hw translator
# testing where pexes are build on x86 and translated on arm
SetupPnaclPexeOpt() {
  SUFFIX=opt.stripped.pexe
}

SetupPnaclPexe() {
  SUFFIX=unopt.stripped.pexe
}

#@
#@ SetupGccX8632
#@   use system compiler for x86-32
SetupGccX8632() {
  PREFIX=
  SUFFIX=gcc.x8632
}

#@
#@ SetupGccX8632Opt
#@   use system compiler for x86-32 with optimization
SetupGccX8632Opt() {
  PREFIX=
  SUFFIX=gcc.opt.x8632
}

#@
#@ SetupGccX8664
#@   use system compiler for x86-64
SetupGccX8664() {
  PREFIX=
  SUFFIX=gcc.x8664
}

#@
#@ SetupGccX8664Opt
#@   use system compiler for x86-64 with optimization
SetupGccX8664Opt() {
  PREFIX=
  SUFFIX=gcc.opt.x8664
}

#@
#@ SetupEmcc
#@   use Emscripten emcc compiler for Asm.js JavaScript generation
SetupEmcc() {
  PREFIX=../run_asmjs.sh
  SUFFIX=emcc.html
  VERIFY=no
}

#@
#@ SetupLlvmX8632
#@   use system compiler for x86-32
SetupLlvmX8632() {
  PREFIX=
  SUFFIX=llvm.x8632
}

#@
#@ SetupLlvmX8632Opt
#@   use system compiler for x86-32 with optimization
SetupLlvmX8632Opt() {
  PREFIX=
  SUFFIX=llvm.opt.x8632
}

#@
#@ SetupLlvmX8664
#@   use system compiler for x86-64
SetupLlvmX8664() {
  PREFIX=
  SUFFIX=llvm.x8664
}

#@
#@ SetupLlvmX8664Opt
#@   use system compiler for x86-64 with optimization
SetupLlvmX8664Opt() {
  PREFIX=
  SUFFIX=llvm.opt.x8664
}

#@
#@ SetupLlvmArm
#@   use system compiler for ARM
SetupLlvmArm() {
  PREFIX=
  SUFFIX=llvm.hw.arm
}

#@
#@ SetupLlvmArmOpt
#@   use system compiler for ARM with optimization
SetupLlvmArmOpt() {
  PREFIX=
  SUFFIX=llvm.opt.hw.arm
}

######################################################################

SetupNaclX8632Common() {
  SetupSelLdr x86-32
}

#@
#@ SetupNaclX8632
#@   use nacl-gcc compiler
SetupNaclX8632() {
  SetupNaclX8632Common
  SUFFIX=nacl.x8632
}

#@
#@ SetupNaclX8632Opt
#@   use nacl-gcc compiler with optimizations
SetupNaclX8632Opt() {
  SetupNaclX8632Common
  SUFFIX=nacl.opt.x8632
}

SetupNaclX8664Common() {
  SetupSelLdr x86-64
}

#@
#@ SetupNaclX8664
#@   use nacl-gcc64 compiler
SetupNaclX8664() {
  SetupNaclX8664Common
  SUFFIX=nacl.x8664
}

#@
#@ SetupNaclX8664Opt
#@   use nacl-gcc64 compiler with optimizations
SetupNaclX8664Opt() {
  SetupNaclX8664Common
  SUFFIX=nacl.opt.x8664
}

SetupNaclDynX8632Common() {
  SetupSelLdr x86-32 "" "-s" "${RUNNABLE_LD_X8632}"
}

#@
#@ SetupNaclDynX8632
#@   use nacl-gcc compiler with glibc toolchain and dynamic linking
SetupNaclDynX8632() {
  SetupNaclDynX8632Common
  SUFFIX=nacl.dyn.x8632
}

#@
#@ SetupNaclDynX8632Opt
#@   use nacl-gcc compiler with glibc toolchain and dynamic linking
SetupNaclDynX8632Opt() {
  SetupNaclDynX8632Common
  SUFFIX=nacl.dyn.opt.x8632
}

SetupNaclDynX8664Common() {
  SetupSelLdr x86-64 "" "-s" "${RUNNABLE_LD_X8664}"
}

#@
#@ SetupNaclDynX8664
#@   use nacl64-gcc compiler with glibc toolchain and dynamic linking
SetupNaclDynX8664() {
  SetupNaclDynX8664Common
  SUFFIX=nacl.dyn.x8664
}

#@
#@ SetupNaclDynX8664Opt
#@   use nacl64-gcc compiler with glibc toolchain and dynamic linking
SetupNaclDynX8664Opt() {
  SetupNaclDynX8664Common
  SUFFIX=nacl.dyn.opt.x8664
}

######################################################################

SetupPnaclX8664Common() {
  SetupSelLdr x86-64
}

#@
#@ SetupPnaclX8664
#@    use pnacl x86-64 compiler (no lto)
SetupPnaclX8664() {
  SetupPnaclX8664Common
  SUFFIX=pnacl.x8664
}

#@
#@ SetupPnaclX8664Opt
#@    use pnacl x86-64 compiler (with lto)
SetupPnaclX8664Opt() {
  SetupPnaclX8664Common
  SUFFIX=pnacl.opt.x8664
}

#@
#@ SetupPnaclX8664OptSz
#@    use pnacl x86-64 compiler (with lto) plus Subzero
SetupPnaclX8664OptSz() {
  SetupPnaclX8664Common
  SUFFIX=pnacl.opt.sz.x8664
}

#@
#@ SetupPnaclX8664ZBSOpt
#@    use pnacl x86-64 compiler (with lto)
#@    use x86-64 zero-based sandbox
SetupPnaclX8664ZBSOpt() {
  SetupSelLdr x86-64 "" "-c"
  # TODO(arbenson): Give this a different suffix to differentitate
  # from the existing x86-64 build, and make the corresponding
  # changes to the build process.
  SUFFIX=pnacl.opt.x8664
}

#@
#@ SetupPnaclTranslatorX8664
#@    use pnacl x8664 translator (no lto)
SetupPnaclTranslatorX8664() {
  SetupPnaclX8664Common
  SUFFIX=pnacl_translator.x8664
}

#@
#@ SetupPnaclTranslatorX8664Opt
#@    use pnacl x8664 translator (with lto)
SetupPnaclTranslatorX8664Opt() {
  SetupPnaclX8664Common
  SUFFIX=pnacl_translator.opt.x8664
}

#@
#@ SetupPnaclTranslatorFastX8664Opt
#@    use pnacl x8664 translator fast mode (with lto)
SetupPnaclTranslatorFastX8664Opt() {
  SetupPnaclX8664Common
  SUFFIX=pnacl_translator_fast.opt.x8664
}

#@
#@ SetupPnaclTranslatorFastX8664OptSz
#@    use pnacl x8664 Subzero translator (with lto)
SetupPnaclTranslatorFastX8664OptSz() {
  SetupPnaclX8664Common
  SUFFIX=pnacl_translator_fast.opt.sz.x8664
}

#@
#@ SetupPnaclTranslator1ThreadX8664Opt
#@    use pnacl x8664 translator (with lto). Compile w/ 1 thread.
SetupPnaclTranslator1ThreadX8664Opt() {
  SetupPnaclX8664Common
  SUFFIX=pnacl_translator_1thread.opt.x8664
}

#@
#@ SetupPnaclTranslatorFast1ThreadX8664Opt
#@    use pnacl x8664 translator fast mode (with lto). Compile w/ 1 thread.
SetupPnaclTranslatorFast1ThreadX8664Opt() {
  SetupPnaclX8664Common
  SUFFIX=pnacl_translator_fast_1thread.opt.x8664
}

#@
#@ SetupPnaclTranslatorFast1ThreadX8664OptSz
#@    use pnacl x8664 Subzero translator (with lto). Compile w/ 1 thread.
SetupPnaclTranslatorFast1ThreadX8664OptSz() {
  SetupPnaclX8664Common
  SUFFIX=pnacl_translator_fast_1thread.opt.sz.x8664
}

SetupPnaclX8632Common() {
  SetupSelLdr x86-32
}

#@
#@ SetupPnaclX8632
#@    use pnacl x86-32 compiler (no lto)
SetupPnaclX8632() {
  SetupPnaclX8632Common
  SUFFIX=pnacl.x8632
}

#@
#@ SetupPnaclX8632Opt
#@    use pnacl x86-32 compiler (with lto)
SetupPnaclX8632Opt() {
  SetupPnaclX8632Common
  SUFFIX=pnacl.opt.x8632
}

#@
#@ SetupPnaclX8632OptSz
#@    use pnacl x86-32 compiler (with lto) plus Subzero
SetupPnaclX8632OptSz() {
  SetupPnaclX8632Common
  SUFFIX=pnacl.opt.sz.x8632
}

#@
#@ SetupPnaclTranslatorX8632
#@    use pnacl x8632 translator (no lto)
SetupPnaclTranslatorX8632() {
  SetupPnaclX8632Common
  SUFFIX=pnacl_translator.x8632
}

#@
#@ SetupPnaclTranslatorX8632Opt
#@    use pnacl x8632 translator (with lto)
SetupPnaclTranslatorX8632Opt() {
  SetupPnaclX8632Common
  SUFFIX=pnacl_translator.opt.x8632
}

#@
#@ SetupPnaclTranslatorFastX8632Opt
#@    use pnacl x8632 translator fast mode (with lto)
SetupPnaclTranslatorFastX8632Opt() {
  SetupPnaclX8632Common
  SUFFIX=pnacl_translator_fast.opt.x8632
}

#@
#@ SetupPnaclTranslatorFastX8632OptSz
#@    use pnacl x8632 Subzero translator (with lto)
SetupPnaclTranslatorFastX8632OptSz() {
  SetupPnaclX8632Common
  SUFFIX=pnacl_translator_fast.opt.sz.x8632
}

#@
#@ SetupPnaclTranslator1ThreadX8632Opt
#@    use pnacl x8632 translator (with lto). Compile w/ 1 thread.
SetupPnaclTranslator1ThreadX8632Opt() {
  SetupPnaclX8632Common
  SUFFIX=pnacl_translator_1thread.opt.x8632
}

#@
#@ SetupPnaclTranslatorFast1ThreadX8632Opt
#@    use pnacl x8632 translator fast mode (with lto). Compile w/ 1 thread.
SetupPnaclTranslatorFast1ThreadX8632Opt() {
  SetupPnaclX8632Common
  SUFFIX=pnacl_translator_fast_1thread.opt.x8632
}

#@
#@ SetupPnaclTranslatorFast1ThreadX8632OptSz
#@    use pnacl x8632 Subzero translator (with lto). Compile w/ 1 thread.
SetupPnaclTranslatorFast1ThreadX8632OptSz() {
  SetupPnaclX8632Common
  SUFFIX=pnacl_translator_fast_1thread.opt.sz.x8632
}

#@
#@ SetupGccArm
#@   use gcc cross compiler
SetupGccArm() {
  PREFIX="${QEMU_TOOL}"
  SUFFIX=gcc.arm
}

#@
#@ SetupGccArmOpt
#@   use gcc cross compiler
SetupGccArmOpt() {
  PREFIX="${QEMU_TOOL}"
  SUFFIX=gcc.opt.arm
}

SetupPnaclArmCommon() {
  SetupSelLdr arm "${QEMU_TOOL}" "-Q"
}

#@
#@ SetupPnaclArmOpt
#@    use pnacl arm compiler (with lto)  -- run with QEMU
SetupPnaclArmOpt() {
  SetupPnaclArmCommon
  SUFFIX=pnacl.opt.arm
}

#@
#@ SetupPnaclArm
#@    use pnacl arm compiler (no lto)  -- run with QEMU
SetupPnaclArm() {
  SetupPnaclArmCommon
  SUFFIX=pnacl.arm
}

#@
#@ SetupPnaclTranslatorArm
#@    use pnacl arm translator (no lto)
SetupPnaclTranslatorArm() {
  SetupPnaclArmCommon
  SUFFIX=pnacl_translator.arm
}

#@
#@ SetupPnaclTranslatorArmOpt
#@    use pnacl arm translator (with lto)
SetupPnaclTranslatorArmOpt() {
  SetupPnaclArmCommon
  SUFFIX=pnacl_translator.opt.arm
}

#@
#@ SetupPnaclTranslatorFastArmOpt
#@    use pnacl arm translator fast mode (with lto)
SetupPnaclTranslatorFastArmOpt() {
  SetupPnaclArmCommon
  SUFFIX=pnacl_translator_fast.opt.arm
}

SetupPnaclArmCommonHW() {
  SetupSelLdr arm
}

#@
#@ SetupPnaclArmOptHW
#@    use pnacl arm compiler (with lto) -- run on ARM hardware
SetupPnaclArmOptHW() {
  SetupPnaclArmCommonHW
  SUFFIX=pnacl.opt.arm
}

#@
#@ SetupPnaclArmOptSzHW
#@    use pnacl arm compiler (with lto) plus Subzero -- run on ARM hardware
SetupPnaclArmOptSzHW() {
  SetupPnaclArmCommonHW
  SUFFIX=pnacl.opt.sz.arm
}

#@
#@ SetupPnaclArmHW
#@    use pnacl arm compiler (no lto) -- run on ARM hardware
SetupPnaclArmHW() {
  SetupPnaclArmCommonHW
  SUFFIX=pnacl.arm
}

#@
#@ SetupPnaclTranslatorArmHW
#@    use pnacl arm translator (no lto) -- run on ARM hardware
SetupPnaclTranslatorArmHW() {
  SetupPnaclArmCommonHW
  SUFFIX=pnacl_translator.hw.arm
}

#@
#@ SetupPnaclTranslatorArmOptHW
#@    use pnacl arm translator (with lto) -- run on ARM hardware
SetupPnaclTranslatorArmOptHW() {
  SetupPnaclArmCommonHW
  SUFFIX=pnacl_translator.opt.hw.arm
}

#@
#@ SetupPnaclTranslatorArmOptSzHW
#@    use pnacl arm translator (with lto) plus Subzero -- run on ARM hardware
SetupPnaclTranslatorArmOptSzHW() {
  SetupPnaclArmCommonHW
  SUFFIX=pnacl_translator.opt.hw.sz.arm
}

#@
#@ SetupPnaclTranslatorFastArmOptHW
#@    use pnacl arm translator fast mode (with lto) -- run on ARM hardware
SetupPnaclTranslatorFastArmOptHW() {
  SetupPnaclArmCommonHW
  SUFFIX=pnacl_translator_fast.opt.hw.arm
}

#@
#@ SetupPnaclTranslatorFastArmOptSzHW
#@    use pnacl arm translator fast mode (with lto) plus Subzero -- run on ARM
#@    hardware
SetupPnaclTranslatorFastArmOptSzHW() {
  SetupPnaclArmCommonHW
  SUFFIX=pnacl_translator_fast.opt.hw.sz.arm
}

#@
#@ SetupPnaclTranslator1ThreadArmOptHW
#@    use pnacl arm translator (with lto) -- run on ARM hardware.
#@    compile with 1 thread.
SetupPnaclTranslator1ThreadArmOptHW() {
  SetupPnaclArmCommonHW
  SUFFIX=pnacl_translator_1thread.opt.hw.arm
}

#@
#@ SetupPnaclTranslatorFast1ThreadArmOptHW
#@    use pnacl arm translator fast mode (with lto) -- run on ARM hardware
#@    compile with 1 thread.
SetupPnaclTranslatorFast1ThreadArmOptHW() {
  SetupPnaclArmCommonHW
  SUFFIX=pnacl_translator_fast_1thread.opt.hw.arm
}

#@
#@ SetupPnaclTranslatorFast1ThreadArmOptSzHW
#@    use pnacl arm translator fast mode (with lto) plus Subzero -- run on ARM
#@    hardware. compile with 1 thread.
SetupPnaclTranslatorFast1ThreadArmOptSzHW() {
  SetupPnaclArmCommonHW
  SUFFIX=pnacl_translator_fast_1thread.opt.hw.sz.arm
}

#@
#@ SetupWasm
#@   use emcc with wasm backend.
SetupWasm() {
  if [ -z $WASM_INSTALL_DIR ]; then
    echo 'error: WASM_INSTALL_DIR is not set'
    exit 1
  fi
  PREFIX=../run_wasm.sh
  SUFFIX=js
  # When running in d8, emcc does not support persistent output files.
  # So we don't verify the results.
  VERIFY=no
}

ConfigInfo() {
  SubBanner "Config Info"
  echo "benchmarks: $(GetBenchmarkList "$@")"
  echo "script:     $(GetInputSize "$@")"
  echo "suffix      ${SUFFIX}"
  echo "verify      ${VERIFY}"
  echo "prefix     ${PREFIX}"

}

######################################################################
# Functions intended to be called
######################################################################
#@
#@ ------------------------------------------------------------
#@ Available Modes:
#@ ------------------------------------------------------------

#@
#@ GetBenchmarkList
#@
#@   Show available benchmarks
GetBenchmarkList() {
  if [[ $# -ge 1 ]]; then
      if [[ ($1 == "ref") || ($1 == "train") ]]; then
          shift
      fi
  fi

  if [[ ($# == 0) || ($1 == "all") ]] ; then
      echo "${SPEC2K_BENCHMARKS[@]}"
  else
      echo "$@"
  fi
}

#+
#+ GetInputSize [train|ref]
#+
#+  Picks input size for spec runs (train or ref)
GetInputSize() {
  if [[ $# -ge 1 ]]; then
    case $1 in
      train)
        echo "./run.train.sh"
        return
        ;;
      ref)
        echo "./run.ref.sh"
        return
        ;;
    esac
  fi
  echo ${SPEC2K_SCRIPT}
}

#+
#+ CheckFileBuilt <depname> <file> -
#+
#+   Check that a dependency is actually built.
CheckFileBuilt() {
  local depname="$1"
  local filename="$2"
  if [[ ! -f "${filename}" ]] ; then
    echo "You have not built ${depname} yet (${filename})!" 1>&2
    exit -1
  fi
}

#+
#+ SetupSelLdr <arch> <prefix> <extra_flags> <preload>
#+
#+   Set up PREFIX to run sel_ldr on <arch>.
#+   <prefix> precedes sel_ldr in the command.
#+   <extra_flags> are additional flags to sel_ldr.
#+   <preload> is used as the actual nexe to load, making the real nexe an arg.
SetupSelLdr() {
  local arch="$1"
  local prefix="${2-}"
  local extra_flags="${3-}"
  local preload="${4-}"

  local staging="${SCONS_OUT}/opt-${SCONS_BUILD_PLATFORM}-${arch}/staging"
  SEL_LDR="${staging}/sel_ldr"
  CheckFileBuilt "sel_ldr" "${SEL_LDR}"
  if [[ ${SCONS_BUILD_PLATFORM} = "linux" ]]; then
    SEL_LDR_BOOTSTRAP="${staging}/nacl_helper_bootstrap"
    CheckFileBuilt "bootstrap" "${SEL_LDR_BOOTSTRAP}"
  fi
  IRT_IMAGE="${SCONS_OUT}/nacl_irt-${arch}/staging/irt_core.nexe"
  CheckFileBuilt "IRT image" "${IRT_IMAGE}"

  local validator_bin="ncval"
  if [[ ${arch} = "arm" ]]; then
    validator_bin="arm-ncval-core"
  fi
  VALIDATOR="${staging}/${validator_bin}"
  # We don't CheckFileBuilt for VALIDATOR because we currently don't build
  # or use it on x86

  if [[ ${SCONS_BUILD_PLATFORM} = "linux" ]]; then
    TEMPLATE_DIGITS="XXXXXXXXXXXXXXXX"
    PREFIX="${prefix} ${SEL_LDR_BOOTSTRAP} \
${SEL_LDR} --r_debug=0x${TEMPLATE_DIGITS} \
--reserved_at_zero=0x${TEMPLATE_DIGITS} -B ${IRT_IMAGE} \
-a ${extra_flags} -f ${preload}"
  else
    PREFIX="${prefix} ${SEL_LDR} -B ${IRT_IMAGE} -a ${extra_flags} \
-f ${preload}"
  fi
  DASHDASH="--"
}

SCONS_COMMON="./scons --mode=opt-host,nacl -j8 --verbose"

EnableX8664ZeroBasedSandbox() {
  export NACL_ENABLE_INSECURE_ZERO_BASED_SANDBOX=1
}

build-runtime() {
  local platforms=$1
  local runtime_pieces=$2
  local extra_flags="${3-}"
  for platform in ${platforms} ; do
    echo "build-runtime: scons ${runtime_pieces} [${platform}]"
    (cd ${NACL_ROOT};
      ${SCONS_COMMON} ${extra_flags} platform=${platform} ${runtime_pieces})
  done
}

build-libs-nacl() {
  local platforms=$1
  shift 1
  for platform in ${platforms} ; do
    echo "build-libs-nacl: scons build_lib [${platform}] $*"
    (cd ${NACL_ROOT};
      ${SCONS_COMMON} platform=${platform} build_lib "$@")
  done
}

build-libs-pnacl() {
  pushd "${NACL_ROOT}"
  pnacl/build.sh sdk
  popd
}

#@
#@ CleanBenchmarks <benchmark>*
#@
#@   this is a deep clean and you have to rerun PopulateFromSpecHarness
CleanBenchmarks() {
  local list=$(GetBenchmarkList "$@")
  rm -rf bin/
  for i in ${list} ; do
    SubBanner "Cleaning: $i"
    cd $i
    make clean
    rm -rf src/ data/
    cd ..
  done
}

#@
#@ BuildBenchmarks <do_timing> <setup> <benchmark>*
#@
#@  Build all benchmarks according to the setup
#@  First arg should be either 0 (no timing) or 1 (run timing measure).
#@  Results are delivered to {execname}.compile_time
BuildBenchmarks() {
  export PREFIX=
  local timeit=$1
  local setup_func=$2
  "${setup_func}"
  shift 2

  local list=$(GetBenchmarkList "$@")
  ConfigInfo "$@"
  for i in ${list} ; do
    SubBanner "Building: $i"
    cd $i
    # SPEC_COMPONENT is used for Asm.js builds in Makefile.common.
    export SPEC_COMPONENT="${i}"

    make ${MAKEOPTS} measureit=${timeit} \
         PERF_LOGGER="${PERF_LOGGER}" \
         REPETITIONS="${SPEC_COMPILE_REPETITIONS}" \
         COMPILE_REPEATER="${COMPILE_REPEATER}" \
         SCONS_BUILD_PLATFORM=${SCONS_BUILD_PLATFORM} \
         BUILD_ARCH_SHORT=${BUILD_ARCH_SHORT} \
         ${i#*.}.${SUFFIX}
    cd ..
  done
}


#@ TimedRunCmd <time_result_file> {actual_cmd }
#@
#@  Run the command under time and dump time data to file.
TimedRunCmd() {
  target="$1"
  shift
  echo "Running: $@"
  /usr/bin/time -f "%U %S %e %C" --append -o "${target}" "$@"
}

#@
#@ RunBenchmarks <setup> [ref|train] <benchmark>*
#@
#@  Run all benchmarks according to the setup.
RunBenchmarks() {
  export PREFIX=
  local setup_func=$1
  "${setup_func}"
  shift
  local list=$(GetBenchmarkList "$@")
  local script=$(GetInputSize "$@")
  ConfigInfo "$@"
  for i in ${list} ; do
    SubBanner "Benchmarking: $i"
    pushd $i
    target_file=./${i#*.}.${SUFFIX}
    echo_file_size ${target_file}
    # SCRIPTNAME is needed by run_asmjs.sh so that it knows
    # which version of the prepackaged files to use.
    export SCRIPTNAME="${script}"
    echo "Running: ${script} ${target_file}"
    ${script} ${target_file}
    popd
  done
}


#@
#@ RunTimedBenchmarks <setup> [ref|train] <benchmark>*
#@
#@  Run all benchmarks according to the setup.
#@  All timing related files are stored in {execname}.run_time
#@  Note that the VERIFY variable effects the timing!
RunTimedBenchmarks() {
  export PREFIX=
  "$1"
  shift
  local list=$(GetBenchmarkList "$@")
  local script=$(GetInputSize "$@")

  ConfigInfo "$@"
  for i in ${list} ; do
    SubBanner "Benchmarking: $i"
    pushd $i
    local benchname=${i#*.}
    local target_file=./${benchname}.${SUFFIX}
    local time_file=${target_file}.run_time
    echo_file_size  ${target_file}
    # SCRIPTNAME is needed by run_asmjs.sh so that it knows
    # which version of the prepackaged files to use.
    export SCRIPTNAME="${script}"
    # Clear out the previous times.
    rm -f "${time_file}"
    echo "Running benchmark ${SPEC_RUN_REPETITIONS} times"
    for ((i=0; i<${SPEC_RUN_REPETITIONS}; i++))
    do
      TimedRunCmd ${time_file} ${script} ${target_file}
    done
    # TODO(jvoung): split runtimes by arch as well
    # i.e., pull "arch" out of SUFFIX and add to the "runtime" label.
    "${PERF_LOGGER}" LogRealTime "${time_file}" "runtime" \
      ${benchname} ${SUFFIX}
    popd
  done
}

TimeValidation() {
  local setup_func=$1
  "${setup_func}"
  shift
  local list=$(GetBenchmarkList "$@")
  for i in ${list}; do
    SubBanner "Validating: $i"
    pushd $i
    local benchname=${i#*.}
    local target_file=./${benchname}.${SUFFIX}
    local time_file=${target_file}.validation_time
    rm -f "${time_file}"
    for ((i=0; i<${SPEC_RUN_REPETITIONS}; i++))
    do
      TimedRunCmd ${time_file} "${VALIDATOR}" ${target_file}
    done
    "${PERF_LOGGER}" LogRealTime "${time_file}" "validationtime" \
      ${benchname} ${SUFFIX}
    popd
  done
  if [[ ${setup_func} =~ "Arm" ]]; then
    TimedRunCmd llc.validation_time "${VALIDATOR}" "${ARM_LLC_NEXE}"
    "${PERF_LOGGER}" LogRealTime llc.validation_time "validationtime" \
      "llc" ${SUFFIX}
  fi
}

#@
#@ BuildAndRunBenchmarks <setup> [ref|train] <benchmark>*
#@
#@   Builds and run all benchmarks according to the setup
BuildAndRunBenchmarks() {
  setup=$1
  shift
  BuildBenchmarks 0 ${setup} "$@"
  RunBenchmarks ${setup} "$@"
}

#@
#@ TimedBuildAndRunBenchmarks <setup> [ref|train] <benchmark>*
#@
#@   Builds and run all benchmarks according to the setup, using
#@   and records the time spent at each task..
#@   Results are saved in {execname}.compile_time and
#@   {execname}.run_time for each benchmark executable
#@   Note that the VERIFY variable effects the timing!
TimedBuildAndRunBenchmarks() {
  setup=$1
  shift
  BuildBenchmarks 1 ${setup} "$@"
  RunTimedBenchmarks ${setup} "$@"
}

#@
#@ PackageArmBinaries [usual var-args for RunBenchmarks]
#@
#@   Archives ARM binaries built from a local QEMU run of spec
#@
#@   Note: <setup> should be the QEMU setup (e.g., SetupPnaclArmOpt)
#@   Note: As with the other modes in this script, this script should be
#@   run from the directory of the script (not the native_client directory).
#@
PackageArmBinaries() {
  local BENCH_LIST=$(GetBenchmarkList "$@")

  local UNZIPPED_TAR=$(basename ${TESTS_ARCHIVE} .gz)

  # Switch to native_client directory (from tests/spec2k) so that
  # when we extract, the builder will have a more natural directory layout.
  pushd "../.."
  # Carefully tar only the parts of the spec harness that we need.
  # First prune
  find tests/spec2k -name '*.bc' -delete
  find tests/spec2k -maxdepth 1 -type f -print |
    xargs tar --no-recursion -cvf ${UNZIPPED_TAR}
  tar -rvf ${UNZIPPED_TAR} tests/spec2k/bin
  for i in ${BENCH_LIST} ; do
    tar -rvf ${UNZIPPED_TAR} tests/spec2k/$i
  done
  gzip -f ${UNZIPPED_TAR}
  popd
}

#@ UnpackArmBinaries
#@ Unpack a packaged archive of ARM SPEC binaries. The archive is
#@ located in the nacl root directory, but the script is run from the spec dir.
UnpackArmBinaries() {
  (cd ${NACL_ROOT};
    tar xvzf ${TESTS_ARCHIVE})
}

GetTestArchiveName() {
  echo ${TESTS_ARCHIVE}
}

#@
#@ PopulateFromSpecHarness <path> <benchmark>*
#@
#@   populate a few essential directories (src, date) from
#@   the given spec2k harness
PopulateFromSpecHarness() {
  harness=$1
  shift
  cp -r ${harness}/bin .
  local list=$(GetBenchmarkList "$@")
  echo ${list}
  for i in ${list} ; do
    SubBanner "Populating: $i"
    # fix the dir with the same name inside spec harness
    src=$(find -H ${harness} -name $i)
    # copy relevant dirs over
    echo "COPY"
    rm -rf src/ data/
    cp -r ${src}/data ${src}/src $i
    # patch if necessary
    if [[ -e $i/diff ]] ; then
      echo "PATCH"
      patch -l -d $i --verbose -p0 < $i/diff
    fi

    echo "COMPLETE"
  done
}

#@
#@ BuildPrerequisites <platform> <bitcode>
#@
#@   Invoke scons to build some potentially missing  components, e.g.
#@   sel_ldr, irt, some untrusted libraries.
#@   Those compoents should be present in the SDK but are not in the
#@   standard toolchain tarballs.
BuildPrerequisites() {
  local platforms=$1
  local bitcode=$2
  local extrabuild="${3-}"
  local extra_flags="${4-}"
  # Sel universal is only used for the pnacl sandboxed translator,
  # but prepare it just in case.
  # IRT is used both to run the tests and to run the pnacl sandboxed translator.
  build-runtime "${platforms}" "sel_ldr irt_core elf_loader" \
    ${extrabuild} ${extra_flags}
  if [ ${bitcode} == "bitcode" ] ; then
     build-libs-pnacl
  else
    # libs may be unnecessary for the glibc build, but build it just in case.
    build-libs-nacl "${platforms}"
  fi
}

#@
#@ BuildPrerequisitesSetupBased <setup>
#@
#@   Convenience wrapper for BuildPrerequisites
BuildPrerequisitesSetupBased() {
  local platforms=""
  local bitcode=""
  if [[ "$1" == SetupPnacl* ]] ; then
    bitcode="bitcode"
  fi
  if [[ "$1" == Setup*Arm* ]] ; then
    platforms="arm"
  elif [[ "$1" == Setup*X8632* ]] ; then
    platforms="x86-32"
  elif [[ "$1" == Setup*X8664* ]] ; then
    platforms="x86-64"
  else
    echo "Bad setup [$1]"
    exit -1
  fi
  BuildPrerequisites "${platforms}" "${bitcode}"
}
######################################################################
# Main
######################################################################

[ $# = 0 ] && set -- help  # Avoid reference to undefined $1.

if [ "$(type -t $1)" != "function" ]; then
  Usage
  echo "ERROR: unknown mode '$1'." >&2
  exit 1
fi

"$@"
