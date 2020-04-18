#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
#@                 PNaCl toolchain build script
#@-------------------------------------------------------------------
#@ This script builds the ARM and PNaCl untrusted toolchains.
#@ It MUST be run from the native_client/ directory.
######################################################################
# Directory Layout Description
######################################################################
#
# All directories are relative to BASE which is
# On Linux: native_client/toolchain/linux_x86/pnacl_newlib/
# On Mac: native_client/toolchain/mac_x86/pnacl_newlib/
# On Windows: native_client/toolchain/win_x86/pnacl_newlib/
#
######################################################################

set -o nounset
set -o errexit

PWD_ON_ENTRY="$(pwd)"
# The script is located in "native_client/pnacl/".
# Set pwd to pnacl/
cd "$(dirname "$0")"
if [[ $(basename "$(pwd)") != "pnacl" ]] ; then
  echo "ERROR: cannot find pnacl/ directory"
  exit -1
fi

source scripts/common-tools.sh

readonly PNACL_ROOT="$(pwd)"
readonly NACL_ROOT="$(GetAbsolutePath ..)"
readonly GCLIENT_ROOT="$(GetAbsolutePath ${NACL_ROOT}/..)"
readonly SCONS_OUT="${NACL_ROOT}/scons-out"

SetScriptPath "${PNACL_ROOT}/build.sh"
SetLogDirectory "${PNACL_ROOT}/build/log"

readonly TOOLCHAIN_BUILD="${NACL_ROOT}/toolchain_build/toolchain_build_pnacl.py"

# For different levels of make parallelism change this in your env
readonly PNACL_CONCURRENCY=${PNACL_CONCURRENCY:-8}
# Concurrency for builds using the host's system compiler (which might be goma)
readonly PNACL_CONCURRENCY_HOST=${PNACL_CONCURRENCY_HOST:-${PNACL_CONCURRENCY}}
PNACL_PRUNE=${PNACL_PRUNE:-true}

# TODO(pdox): Decide what the target should really permanently be
readonly CROSS_TARGET_ARM=arm-none-linux-gnueabi
readonly REAL_CROSS_TARGET=arm-nacl

readonly DRIVER_DIR="${PNACL_ROOT}/driver"

readonly TOOLCHAIN_ROOT="${NACL_ROOT}/toolchain"
readonly TOOLCHAIN_BASE="${TOOLCHAIN_ROOT}/${SCONS_BUILD_PLATFORM}_x86"

readonly PNACL_MAKE_OPTS="${PNACL_MAKE_OPTS:-}"
readonly MAKE_OPTS="-j${PNACL_CONCURRENCY} VERBOSE=1 ${PNACL_MAKE_OPTS}"

readonly NONEXISTENT_PATH="/going/down/the/longest/road/to/nowhere"

# Git sources
readonly PNACL_GIT_ROOT="${NACL_ROOT}/toolchain_build/src"
readonly TC_SRC_BINUTILS="${PNACL_GIT_ROOT}/binutils"
readonly TC_SRC_LLVM="${PNACL_GIT_ROOT}/llvm"
readonly TC_SRC_SUBZERO="${PNACL_GIT_ROOT}/subzero"

readonly TOOLCHAIN_BUILD_OUT="${NACL_ROOT}/toolchain_build/out"

readonly TIMESTAMP_FILENAME="make-timestamp"

# PNaCl toolchain installation directories (absolute paths)
readonly SDK_INSTALL_ROOT="${TOOLCHAIN_BASE}/pnacl_newlib"
readonly INSTALL_ROOT="${TOOLCHAIN_BUILD_OUT}/translator_compiler_install"
readonly INSTALL_BIN="${INSTALL_ROOT}/bin"

# Native nacl lib directories
# The pattern `${INSTALL_LIB_NATIVE}${arch}' is used in many places.
readonly INSTALL_LIB_NATIVE="${INSTALL_ROOT}/translator/"

# PNaCl client-translators (sandboxed) binary locations
readonly INSTALL_TRANSLATOR="${TOOLCHAIN_BUILD_OUT}/sandboxed_translators_install"

# The INSTALL_HOST directory has host binaries and libs which
# are part of the toolchain (e.g. llvm and binutils).
# There are also tools-x86 and tools-arm which have host binaries which
# are not part of the toolchain but might be useful in the SDK, e.g.
# arm sel_ldr and x86-hosted arm/mips validators.
readonly INSTALL_HOST="${INSTALL_ROOT}"

# Component installation directories
readonly LLVM_INSTALL_DIR="${INSTALL_HOST}"
readonly BINUTILS_INSTALL_DIR="${INSTALL_HOST}"

# Location of the PNaCl tools defined for configure invocations.
readonly PNACL_CC="${INSTALL_BIN}/le32-nacl-clang"
readonly PNACL_CXX="${INSTALL_BIN}/le32-nacl-clang++"
readonly PNACL_LD="${INSTALL_BIN}/le32-nacl-ld"
readonly PNACL_PP="${INSTALL_BIN}/le32-nacl-clang -E"
readonly PNACL_AR="${INSTALL_BIN}/le32-nacl-ar"
readonly PNACL_RANLIB="${INSTALL_BIN}/le32-nacl-ranlib"
readonly PNACL_AS="${INSTALL_BIN}/le32-nacl-as"
readonly PNACL_NM="${INSTALL_BIN}/le32-nacl-nm"
readonly PNACL_TRANSLATE="${INSTALL_BIN}/pnacl-translate"
readonly PNACL_SIZE="${BINUTILS_INSTALL_DIR}/bin/${REAL_CROSS_TARGET}-size"
readonly PNACL_STRIP="${INSTALL_BIN}/${REAL_CROSS_TARGET}-strip"
readonly PNACL_CXXFILT="${BINUTILS_INSTALL_DIR}/bin/${REAL_CROSS_TARGET}-c++filt"
readonly ILLEGAL_TOOL="${INSTALL_BIN}"/pnacl-illegal

# Tools for building the LLVM BuildTools in the translator build
readonly HOST_CLANG_PATH="${GCLIENT_ROOT}/third_party/llvm-build/Release+Asserts/bin"
readonly HOST_CLANG="${HOST_CLANG_PATH}/clang"
# Use toolchain_build's libcxx install directory instead of ${INSTALL_ROOT}/lib
# because the latter also has the LLVM shared libs in it, and given how stupid
# the LLVM BuildTools build is, who knows what would happen if it found those.
readonly HOST_LIBCXX="${NACL_ROOT}/toolchain_build/out/libcxx_x86_64_linux_install"

# For a production (release) build, we want the sandboxed
# translator to only contain the code needed to handle
# its own architecture. For example, the translator shipped with
# an X86-32 browser would only be able to translate to X86-32 code.
# This is so that the translator binary is as small as possible.
#
# If SBTC_PRODUCTION is true, then the translators are built
# separately, one for each architecture, so that each translator
# can only target its own architecture.
#
# If SBTC_PRODUCTION is false, then we instead use PNaCl to
# build a `fat` translator which can target all supported
# architectures. This translator is built as a .pexe
# which can then be translated to each individual architecture.
SBTC_PRODUCTION=${SBTC_PRODUCTION:-true}

# Which arches to build for our sandboxed toolchain.
SBTC_ARCHES_ALL=${SBTC_ARCHES_ALL:-"armv7 i686 x86_64 mips"}

get-sbtc-llvm-arches() {
# For LLVM i686 brings in both i686 and x86_64.  De-dupe that.
  echo ${SBTC_ARCHES_ALL} \
    | sed 's/x86_64/i686/' | sed 's/i686\(.*\)i686/i686\1/'
}
SBTC_ARCHES_LLVM=$(get-sbtc-llvm-arches)


CC=${CC:-gcc}
CXX=${CXX:-g++}
AR=${AR:-ar}
RANLIB=${RANLIB:-ranlib}

if ${HOST_ARCH_X8632}; then
  # These are simple compiler wrappers to force 32bit builds
  CC="${PNACL_ROOT}/scripts/mygcc32"
  CXX="${PNACL_ROOT}/scripts/myg++32"
fi

######################################################################
######################################################################
#
#                     < USER ACCESSIBLE FUNCTIONS >
#
######################################################################
######################################################################

#@-------------------------------------------------------------------------

#@ translator-all   -  Build and install all of the translators.
translator-all() {
  StepBanner \
    "SANDBOXED TC [prod=${SBTC_PRODUCTION}] [arches=${SBTC_ARCHES_ALL}]"

  if ${SBTC_PRODUCTION}; then
    # Build each architecture separately.
    local arch
    for arch in ${SBTC_ARCHES_ALL} ; do
      llvm-sb ${arch}
    done
    for arch in ${SBTC_ARCHES_ALL} ; do
      binutils-gold-sb ${arch}
    done
  else
    # Using arch `universal` builds the sandboxed tools from a single
    # .pexe which support all targets.
    llvm-sb universal
    binutils-gold-sb universal
    if ${PNACL_PRUNE}; then
      # The universal pexes have already been translated.
      # They don't need to stick around.
      rm -rf "$(GetTranslatorInstallDir universal)"
    fi
  fi

  # Copy native libs to translator install dir.
  mkdir -p ${INSTALL_TRANSLATOR}/translator
  cp -a ${INSTALL_LIB_NATIVE}* ${INSTALL_TRANSLATOR}/translator

  driver-install-translator
}


#+ translator-prune    Remove leftover files like pexes from pnacl_translator
#+                     build and from translator-archive-pexes.
translator-prune() {
  find "${INSTALL_TRANSLATOR}" -name "*.pexe" -exec "rm" {} +
}


#+ translator-clean <arch> -
#+     Clean one translator install/build
translator-clean() {
  local arch=$1
  StepBanner "TRANSLATOR" "Clean ${arch}"
  rm -rf "$(GetTranslatorInstallDir ${arch})"
  rm -rf "$(GetTranslatorBuildDir ${arch})"
}

#########################################################################
#     CLIENT BINARIES (SANDBOXED)
#########################################################################

check-arch() {
  local arch=$1
  for valid_arch in i686 x86_64 armv7 mips universal ; do
    if [ "${arch}" == "${valid_arch}" ] ; then
      return
    fi
  done

  Fatal "ERROR: Unsupported arch [$1]. " \
        "Must be: i686, x86_64, armv7, mips, universal"
}

llvm-sb-setup() {
  local arch=$1
  LLVM_SB_LOG_PREFIX="llvm.sb.${arch}"
  LLVM_SB_OBJDIR="$(GetTranslatorBuildDir ${arch})/llvm-sb"

  # The SRPC headers are included directly from the nacl tree, as they are
  # not in the SDK. libsrpc should have already been built in the
  # translator_compiler step in toolchain_build.
  # This is always statically linked.
  local flags="-static -I$(GetAbsolutePath ${NACL_ROOT}/..) \
    -gline-tables-only "

  LLVM_SB_CONFIGURE_ENV=(
    AR="${PNACL_AR}" \
    AS="${PNACL_AS}" \
    CC="${PNACL_CC} ${flags}" \
    CXX="${PNACL_CXX} ${flags}" \
    LD="${PNACL_LD} ${flags}" \
    NM="${PNACL_NM}" \
    RANLIB="${PNACL_RANLIB}" \
    BUILD_CC="${HOST_CLANG}" \
    BUILD_CXX="${HOST_CLANG}++")
}

#+-------------------------------------------------------------------------
#+ llvm-sb <arch>- Build and install llvm tools (sandboxed)
llvm-sb() {
  local arch=$1
  check-arch ${arch}
  llvm-sb-setup ${arch}
  StepBanner "LLVM-SB" "Sandboxed pnacl-llc [${arch}]"
  local srcdir="${TC_SRC_LLVM}"
  assert-dir "${srcdir}" "You need to checkout llvm."

  if llvm-sb-needs-configure ${arch} ; then
    llvm-sb-clean ${arch}
    llvm-sb-configure ${arch}
  else
    SkipBanner "LLVM-SB" "configure ${arch}"
  fi

  llvm-sb-make ${arch}
  llvm-sb-install ${arch}
}

llvm-sb-needs-configure() {
  [ ! -f "${LLVM_SB_OBJDIR}/config.status" ]
  return $?
}

# llvm-sb-clean          - Clean llvm tools (sandboxed)
llvm-sb-clean() {
  local arch=$1
  StepBanner "LLVM-SB" "Clean ${arch}"
  local objdir="${LLVM_SB_OBJDIR}"

  rm -rf "${objdir}"
  mkdir -p "${objdir}"
}

# llvm-sb-configure - Configure llvm tools (sandboxed)
llvm-sb-configure() {
  local arch=$1

  StepBanner "LLVM-SB" "Configure ${arch}"
  local srcdir="${TC_SRC_LLVM}"
  local objdir="${LLVM_SB_OBJDIR}"
  local installdir="$(GetTranslatorInstallDir ${arch})"
  local targets=""
  local subzero_targets=""
  # For LLVM, "x86" brings in both i686 and x86_64.
  case ${arch} in
    i686)
      targets=x86
      subzero_targets=X8632
      ;;
    x86_64)
      targets=x86
      subzero_targets=X8664
      ;;
    armv7)
      targets=arm
      subzero_targets=ARM32
      ;;
    mips)
      targets=mips
      subzero_targets=MIPS32
      ;;
    universal)
      targets=x86,arm,mips
      subzero_targets=ARM32,MIPS32,X8632,X8664
      ;;
  esac

  spushd "${objdir}"
  # TODO(jvoung): remove ac_cv_func_getrusage=no once newlib has getrusage
  # in its headers.  Otherwise, configure thinks that we can link in
  # getrusage (stub is in libnacl), but we can't actually compile code
  # that uses ::getrusage because it's not in headers:
  # https://code.google.com/p/nativeclient/issues/detail?id=3657
  # Similar with getrlimit/setrlimit where struct rlimit isn't defined.
  RunWithLog \
      ${LLVM_SB_LOG_PREFIX}.configure \
      env -i \
      PATH="/usr/bin:/bin" \
      ${srcdir}/configure \
        "${LLVM_SB_CONFIGURE_ENV[@]}" \
        --prefix=${installdir} \
        --host=nacl \
        --enable-targets=${targets} \
        --enable-subzero-targets=${subzero_targets} \
        --disable-assertions \
        --disable-doxygen \
        --enable-pic=no \
        --enable-static \
        --enable-shared=no \
        --disable-jit \
        --enable-optimized \
        --enable-libcpp \
        --target=${CROSS_TARGET_ARM} \
        llvm_cv_link_use_export_dynamic=no \
        ac_cv_func_getrusage=no \
        ac_cv_func_getrlimit=no \
        ac_cv_func_setrlimit=no
  spopd
}

# llvm-sb-make - Make llvm tools (sandboxed)
llvm-sb-make() {
  local arch=$1
  StepBanner "LLVM-SB" "Make ${arch}"
  local objdir="${LLVM_SB_OBJDIR}"

  spushd "${objdir}"
  ts-touch-open "${objdir}"

  local tools_to_build="pnacl-llc subzero"
  local export_dyn_env="llvm_cv_link_use_export_dynamic=no"
  local isjit=0
  # The LLVM sandboxed build uses the normally-disallowed external
  # function __nacl_get_arch().  Allow that for now.
  RunWithLog ${LLVM_SB_LOG_PREFIX}.make \
      env -i PATH="/usr/bin:/bin:${HOST_CLANG_PATH}" \
      LDFLAGS="-Wl,-plugin-opt=no-finalize -Wl,-plugin-opt=no-abi-verify" \
      LD_LIBRARY_PATH="${HOST_LIBCXX}/lib" \
      ONLY_TOOLS="${tools_to_build}" \
      PNACL_BROWSER_TRANSLATOR=1 \
      KEEP_SYMBOLS=1 \
      NO_DEAD_STRIP=1 \
      VERBOSE=1 \
      SUBZERO_SRC_ROOT="${TC_SRC_SUBZERO}" \
      BUILD_CC="${HOST_CLANG}" \
      BUILD_CXX="${HOST_CLANG}++" \
      BUILD_CXXFLAGS="-stdlib=libc++ -I${HOST_LIBCXX}/include/c++/v1" \
      BUILD_LDFLAGS="-L${HOST_LIBCXX}/lib" \
      ${export_dyn_env} \
      make ${MAKE_OPTS} tools-only

  ts-touch-commit "${objdir}"

  spopd
}

# llvm-sb-install - Install llvm tools (sandboxed)
llvm-sb-install() {
  local arch=$1
  StepBanner "LLVM-SB" "Install ${arch}"

  local installdir="$(GetTranslatorInstallDir ${arch})"/bin
  mkdir -p "${installdir}"
  spushd "${installdir}"
  local objdir="${LLVM_SB_OBJDIR}"

  local tools="pnacl-llc"
  if [[ "${arch}" == "i686" || "${arch}" == "x86_64" || \
        "${arch}" == "armv7" ]]; then
    tools+=" pnacl-sz"
  fi
  for toolname in ${tools}; do
    cp -f "${objdir}"/Release*/bin/${toolname} .
    mv -f ${toolname} ${toolname}.pexe
    local arches=${arch}
    if [[ "${arch}" == "universal" ]]; then
      arches="${SBTC_ARCHES_ALL}"
    fi
    if [[ "${arch}" == "i686" ]]; then
      arches+=" x86-32-nonsfi"
    elif [[ "${arch}" == "armv7" ]]; then
      arches+=" arm-nonsfi"
    fi
    translate-sb-tool ${toolname} "${arches}"
    install-sb-tool ${toolname} "${arches}"
  done
  spopd
}

# translate-sb-tool <toolname> <arches>
#
# Translate <toolname>.pexe to <toolname>.<arch>.nexe in the current directory.
translate-sb-tool() {
  local toolname=$1
  local arches=$2
  local pexe="${toolname}.pexe"

  local tarch
  for tarch in ${arches}; do
    local nexe="${toolname}.${tarch}.nexe"
    StepBanner "TRANSLATE" \
               "Translating ${toolname}.pexe to ${tarch} (background)"
    # NOTE: we are using --noirt to build without a segment gap
    # since we aren't loading the IRT for the translator nexes.
    #
    # Compiling with -ffunction-sections, -fdata-sections, --gc-sections
    # helps reduce the size a bit. If you want to use --gc-sections to test out:
    # http://code.google.com/p/nativeclient/issues/detail?id=1591
    # you will need to do a build without these flags.
    local translate_flags="-ffunction-sections -fdata-sections --gc-sections \
      --allow-llvm-bitcode-input -arch ${tarch} "
    "${PNACL_TRANSLATE}" ${translate_flags} "${pexe}" -o "${nexe}" &
    QueueLastProcess
  done
  StepBanner "TRANSLATE" "Waiting for translation processes to finish"
  QueueWait

  # Test that certain symbols have been pruned before stripping.
  if [ "${toolname}" == "pnacl-llc" ]; then
    for tarch in ${arches}; do
      local nexe="${toolname}.${tarch}.nexe"
      local llvm_host_glob="${LLVM_INSTALL_DIR}/lib/libLLVM*so"
      python "${PNACL_ROOT}/prune_test.py" "${PNACL_NM}" \
        "${PNACL_CXXFILT}" "${llvm_host_glob}" "${nexe}"
    done
  fi

  if ${PNACL_PRUNE}; then
    # Strip the nexes.
    for tarch in ${arches}; do
      local nexe="${toolname}.${tarch}.nexe"
      ${PNACL_STRIP} "${nexe}"
    done
  fi
  StepBanner "TRANSLATE" "Done."
}

# install-sb-tool <toolname> <arches>
#
# Install <toolname>.<arch>.nexe from the current directory
# into the right location (as <toolname>.nexe)
install-sb-tool() {
  local toolname="$1"
  local arches="$2"
  local tarch
  for tarch in ${arches}; do
    local installbin="$(GetTranslatorInstallDir ${tarch})"/bin
    mkdir -p "${installbin}"
    mv -f ${toolname}.${tarch}.nexe "${installbin}"/${toolname}.nexe
    if ${PNACL_BUILDBOT}; then
      spushd "${installbin}"
      local tag="${toolname}_${tarch}_size"
      print-tagged-tool-sizes "${tag}" "$(pwd)/${toolname}.nexe"
      spopd
    fi
  done
}

GetTranslatorBuildDir() {
  local arch="$1"
  echo "${TOOLCHAIN_BUILD_OUT}/sandboxed_translators_work/translator-${arch//_/-}"
}

GetTranslatorInstallDir() {
  local arch
  case $1 in
    i686) arch=x86-32 ;;
    x86_64) arch=x86-64 ;;
    armv7) arch=arm ;;
    mips) arch=mips32 ;;
    x86-32-nonsfi) arch=x86-32-nonsfi ;;
    arm-nonsfi) arch=arm-nonsfi ;;
    default) arch=$1 ;;
  esac
  echo "${INSTALL_TRANSLATOR}"/translator/${arch}
}

### Sandboxed version of gold.

#+-------------------------------------------------------------------------
#+ binutils-gold-sb - Build and install gold (sandboxed)
#+                    This is the replacement for the old
#+                    final sandboxed linker which was bfd based.
#+                    It has nothing to do with the bitcode linker
#+                    which is also gold based.
binutils-gold-sb() {
  local arch=$1
  check-arch ${arch}
  StepBanner "GOLD-NATIVE-SB" "(libiberty + gold) ${arch}"

  local srcdir="${TC_SRC_BINUTILS}"
  assert-dir "${srcdir}" "You need to checkout gold."

  binutils-gold-sb-clean ${arch}
  binutils-gold-sb-configure ${arch}
  binutils-gold-sb-make ${arch}
  binutils-gold-sb-install ${arch}
}

# binutils-gold-sb-clean - Clean gold
binutils-gold-sb-clean() {
  local arch=$1
  StepBanner "GOLD-NATIVE-SB" "Clean ${arch}"
  local objdir="$(GetTranslatorBuildDir ${arch})/binutils-gold-sb"

  rm -rf "${objdir}"
  mkdir -p "${objdir}"
}

# binutils-gold-sb-configure - Configure binutils for gold (unsandboxed)
binutils-gold-sb-configure() {
  local arch=$1
  local srcdir="${TC_SRC_BINUTILS}"
  local objdir="$(GetTranslatorBuildDir ${arch})/binutils-gold-sb"
  local installbin="$(GetTranslatorInstallDir ${arch})/bin"

  # The SRPC headers are included directly from the nacl tree, as they are
  # not in the SDK. libsrpc should have already been built in the
  # translator_compiler step in toolchain_build.
  # The Gold sandboxed build uses the normally-disallowed external
  # function __nacl_get_arch().  Allow that for now.
  #
  local flags="-static -I$(GetAbsolutePath ${NACL_ROOT}/..) \
    -fno-exceptions -O3 -gline-tables-only"
  local configure_env=(
    AR="${PNACL_AR}" \
    AS="${PNACL_AS}" \
    CC="${PNACL_CC} ${flags}" \
    CXX="${PNACL_CXX} ${flags}" \
    CC_FOR_BUILD="${CC}" \
    CXX_FOR_BUILD="${CXX}" \
    LD="${PNACL_LD} ${flags}" \
    NM="${PNACL_NM}" \
    RANLIB="${PNACL_RANLIB}"
  )
  local gold_targets=""
  case ${arch} in
    i686)      gold_targets=i686-pc-nacl ;;
    x86_64)    gold_targets=x86_64-pc-nacl ;;
    armv7)     gold_targets=arm-pc-nacl ;;
    mips)      gold_targets=mips-pc-nacl ;;
    universal)
      gold_targets=i686-pc-nacl,x86_64-pc-nacl,arm-pc-nacl,mips-pc-nacl ;;
  esac

  # gold always adds "target" to the enabled targets so we are
  # little careful to not build too much
  # Note: we are (ab)using target for both --host and --target
  #       which configure expects to be present
  local target
  if [ ${arch} == "universal" ] ; then
    target=i686-pc-nacl
  else
    target=${gold_targets}
  fi

  StepBanner "GOLD-NATIVE-SB" "Configure (libiberty)"
  # Gold depends on liberty only for a few functions:
  # xrealloc, lbasename, etc.
  # we could remove these if necessary.

  mkdir -p "${objdir}/libiberty"
  spushd "${objdir}/libiberty"
  StepBanner "GOLD-NATIVE-SB" "Dir [$(pwd)]"
  local log_prefix="binutils-gold.sb.${arch}"
  RunWithLog "${log_prefix}".configure \
    env -i \
    PATH="/usr/bin:/bin" \
    "${configure_env[@]}" \
    ${srcdir}/libiberty/configure --prefix="${installbin}" \
    --host=${target} \
    --target=${target}
  spopd

  StepBanner "GOLD-NATIVE-SB" "Configure (gold) ${arch}"
  mkdir -p "${objdir}/gold"
  spushd "${objdir}/gold"
  StepBanner "GOLD-NATIVE-SB" "Dir [$(pwd)]"
  # Removed -Werror until upstream gold no longer has problems with new clang
  # warnings. http://code.google.com/p/nativeclient/issues/detail?id=2861
  # TODO(sehr,robertm): remove this when gold no longer has these.
  # Disable readv. We have a stub for it, but not the accompanying headers
  # in newlib, like sys/uio.h to actually compile with it.
  RunWithLog "${log_prefix}".configure \
    env -i \
    PATH="/usr/bin:/bin" \
    "${configure_env[@]}" \
    CXXFLAGS="" \
    CFLAGS="" \
    LDFLAGS="-Wl,-plugin-opt=no-finalize -Wl,-plugin-opt=no-abi-verify" \
    ac_cv_search_zlibVersion=no \
    ac_cv_header_sys_mman_h=no \
    ac_cv_func_mmap=no \
    ac_cv_func_mallinfo=no \
    ac_cv_func_readv=no \
    ac_cv_prog_cc_g=no \
    ac_cv_prog_cxx_g=no \
    ${srcdir}/gold/configure --prefix="${installbin}" \
                                      --enable-targets=${gold_targets} \
                                      --host=${target} \
                                      --target=${target} \
                                      --disable-nls \
                                      --enable-plugins=no \
                                      --enable-naclsrpc=yes \
                                      --disable-werror \
                                      --with-sysroot="${NONEXISTENT_PATH}"
  # Note: the extra ac_cv settings:
  # * eliminate unnecessary use of zlib
  # * eliminate use of mmap
  # (those should not have much impact on the non-sandboxed
  # version but help in the sandboxed case)
  # We also disable debug (-g) because the bitcode files become too big
  # (with ac_cv_prog_cc_g).

  # There's no point in setting the correct path as sysroot, because we
  # want the toolchain to be relocatable. The driver will use ld command-line
  # option --sysroot= to override this value and set it to the correct path.
  # However, we need to include --with-sysroot during configure to get this
  # option. So fill in a non-sense, non-existent path.
  spopd
}

# binutils-gold-sb-make - Make binutils (sandboxed)
binutils-gold-sb-make() {
  local arch=${1}
  local objdir="$(GetTranslatorBuildDir ${arch})/binutils-gold-sb"
  ts-touch-open "${objdir}/"

  StepBanner "GOLD-NATIVE-SB" "Make (liberty) ${arch}"
  spushd "${objdir}/libiberty"

  RunWithLog "binutils-gold.liberty.sb.${arch}".make \
      env -i PATH="/usr/bin:/bin" \
      make ${MAKE_OPTS}
  spopd

  StepBanner "GOLD-NATIVE-SB" "Make (gold) ${arch}"
  spushd "${objdir}/gold"
  RunWithLog "binutils-gold.sb.${arch}".make \
      env -i PATH="/usr/bin:/bin" \
      make ${MAKE_OPTS} ld-new
  spopd

  ts-touch-commit "${objdir}"
}

# binutils-gold-sb-install - Install gold
binutils-gold-sb-install() {
  local arch=$1
  local objdir="$(GetTranslatorBuildDir ${arch})/binutils-gold-sb"
  local installbin="$(GetTranslatorInstallDir ${arch})/bin"

  StepBanner "GOLD-NATIVE-SB" "Install [${installbin}] ${arch}"

  mkdir -p "${installbin}"
  spushd "${installbin}"

  # Install just "ld"
  cp "${objdir}"/gold/ld-new ld.pexe

  # Translate and install
  local arches=${arch}
  if [[ "${arch}" == "universal" ]]; then
    arches="${SBTC_ARCHES_ALL}"
  fi
  if [[ "${arch}" == "i686" ]]; then
    arches+=" x86-32-nonsfi"
  elif [[ "${arch}" == "armv7" ]]; then
    arches+=" arm-nonsfi"
  fi
  translate-sb-tool ld "${arches}"
  install-sb-tool ld "${arches}"
  spopd
}

#########################################################################
#     < SDK >
#########################################################################
SCONS_ARGS=(MODE=nacl
            -j${PNACL_CONCURRENCY}
            bitcode=1
            disable_nosys_linker_warnings=1
            naclsdk_validate=0
            --verbose)

SDK_IS_SETUP=false
sdk-setup() {
  if ${SDK_IS_SETUP} && [ $# -eq 0 ]; then
    return 0
  fi
  SDK_IS_SETUP=true

  SDK_INSTALL_LE32="${SDK_INSTALL_ROOT}/le32-nacl"
  SDK_INSTALL_LIB="${SDK_INSTALL_LE32}/lib"
  SDK_INSTALL_INCLUDE="${SDK_INSTALL_LE32}/include"
}

sdk() {
  sdk-setup "$@"
  StepBanner "SDK"
  sdk-clean
  sdk-headers
  sdk-libs
}

#+ sdk-clean             - Clean sdk stuff
sdk-clean() {
  sdk-setup "$@"
  StepBanner "SDK" "Clean"
  rm -rf "${SDK_INSTALL_LIB}"/{libnacl*.a,libpthread.a,libnosys.a}
  rm -rf "${SDK_INSTALL_INCLUDE}"/{irt*.h,pthread.h,semaphore.h,nacl}

  # clean scons obj dirs
  rm -rf "${SCONS_OUT}"/nacl-*-pnacl*
}

sdk-headers() {
  sdk-setup "$@"
  mkdir -p "${SDK_INSTALL_INCLUDE}"

  local extra_flags=""
  local neutral_platform="x86-32"

  StepBanner "SDK" "Install headers"
  spushd "${NACL_ROOT}"
  RunWithLog "sdk.headers" \
      ./scons \
      "${SCONS_ARGS[@]}" \
      ${extra_flags} \
      platform=${neutral_platform} \
      pnacl_newlib_dir="${SDK_INSTALL_ROOT}" \
      install_headers \
      includedir="$(PosixToSysPath "${SDK_INSTALL_INCLUDE}")"
  spopd
}

sdk-libs() {
  sdk-setup "$@"
  StepBanner "SDK" "Install libraries"
  mkdir -p "${SDK_INSTALL_LIB}"

  local extra_flags=""
  local neutral_platform="x86-32"

  spushd "${NACL_ROOT}"
  RunWithLog "sdk.libs.bitcode" \
      ./scons \
      "${SCONS_ARGS[@]}" \
      ${extra_flags} \
      platform=${neutral_platform} \
      pnacl_newlib_dir="${SDK_INSTALL_ROOT}" \
      install_lib \
      libdir="$(PosixToSysPath "${SDK_INSTALL_LIB}")"
  spopd
}


# install python scripts and redirector shell/batch scripts
driver-install-python() {
  local destdir="$1"
  shift
  local pydir="${destdir}/pydir"

  StepBanner "DRIVER" "Installing driver adaptors to ${destdir}"
  rm -rf "${destdir}"
  mkdir -p "${destdir}"
  mkdir -p "${pydir}"

  spushd "${DRIVER_DIR}"

  # Copy python scripts
  cp $@ driver_log.py driver_env.py driver_temps.py \
    *tools.py filetype.py loader.py nativeld.py "${pydir}"

  # Install redirector shell/batch scripts
  for name in $@; do
    local dest="${destdir}/${name/.py}"
    # In some situations cygwin cp messes up the permissions of the redirector
    # shell/batch scripts. Using cp -a seems to make sure it preserves them.
    cp -a redirect.sh "${dest}"
    chmod +x "${dest}"
    if ${BUILD_PLATFORM_WIN}; then
      cp -a redirect.bat "${dest}".bat
    fi
  done
  spopd
}

feature-version-file-install() {
  local install_root=$1
  # Scons tests can check this version number to decide whether to
  # enable tests for toolchain bug fixes or new features.  This allows
  # tests to be enabled on the toolchain buildbots/trybots before the
  # new toolchain version is rolled into the pinned version (i.e. before
  # the tests would pass on the main NaCl buildbots/trybots).
  #
  # If you are adding a test that depends on a toolchain change, you
  # can increment this version number manually.
  echo 26 > "${install_root}/FEATURE_VERSION"
}

#@ driver-install-translator - Install driver scripts for translator component
driver-install-translator() {
  local destdir="${INSTALL_TRANSLATOR}/bin"

  driver-install-python "${destdir}" pnacl-translate.py

  echo """HAS_FRONTEND=0""" > "${destdir}"/driver.conf

  feature-version-file-install ${INSTALL_TRANSLATOR}
}

######################################################################
######################################################################
#
# UTILITIES
#
######################################################################
######################################################################

#@-------------------------------------------------------------------------
#@ show-config
show-config() {
  Banner "Config Settings:"
  echo "PNACL_BUILDBOT:    ${PNACL_BUILDBOT}"
  echo "PNACL_CONCURRENCY: ${PNACL_CONCURRENCY}"
  echo "PNACL_DEBUG:       ${PNACL_DEBUG}"
  echo "PNACL_PRUNE:       ${PNACL_PRUNE}"
  echo "PNACL_VERBOSE:     ${PNACL_VERBOSE}"
  echo "INSTALL_ROOT:      ${INSTALL_ROOT}"
  Banner "Your Environment:"
  env | grep PNACL
  Banner "uname info for builder:"
  uname -a
}

#@ help                  - Usage information.
help() {
  Usage
}

#@ help-full             - Usage information including internal functions.
help-full() {
  Usage2
}

DebugRun() {
  if ${PNACL_DEBUG} || ${PNACL_BUILDBOT}; then
    "$@"
  fi
}

######################################################################
# Generate chromium perf bot logs for tracking the size of a binary.
#
print-tagged-tool-sizes() {
  local tag="$1"
  local binary="$2"

  # size output look like:
  #    text   data     bss     dec    hex  filename
  #  354421  16132  168920  539473  83b51  .../tool
  local sizes=($(${PNACL_SIZE} -B "${binary}" | grep '[0-9]\+'))
  echo "RESULT ${tag}: text= ${sizes[0]} bytes"
  echo "RESULT ${tag}: data= ${sizes[1]} bytes"
  echo "RESULT ${tag}: bss= ${sizes[2]} bytes"
  echo "RESULT ${tag}: total= ${sizes[3]} bytes"

  local file_size=($(du --bytes "${binary}"))
  echo "RESULT ${tag}: file_size= ${file_size[0]} bytes"
}

######################################################################
######################################################################
#
#                           < TIME STAMPING >
#
######################################################################
######################################################################

ts-dir-changed() {
  local tsfile="$1"
  local dir="$2"

  if [ -f "${tsfile}" ]; then
    local MODIFIED=$(find "${dir}" -type f -newer "${tsfile}")
    [ ${#MODIFIED} -gt 0 ]
    ret=$?
  else
    true
    ret=$?
  fi
  return $ret
}

# Check if the source for a given build has been modified
ts-modified() {
  local srcdir="$1"
  local objdir="$2"
  local tsfile="${objdir}/${TIMESTAMP_FILENAME}"

  ts-dir-changed "${tsfile}" "${srcdir}"
  return $?
}

ts-touch() {
  local tsfile="$1"
  touch "${tsfile}"
}

# Record the time when make begins, but don't yet
# write that to the timestamp file.
# (Just in case make fails)

ts-touch-open() {
  local objdir="$1"
  local tsfile="${objdir}/${TIMESTAMP_FILENAME}"
  local tsfile_open="${objdir}/${TIMESTAMP_FILENAME}_OPEN"

  rm -f "${tsfile}"
  touch "${tsfile_open}"
}


# Write the timestamp. (i.e. make has succeeded)

ts-touch-commit() {
  local objdir="$1"
  local tsfile="${objdir}/${TIMESTAMP_FILENAME}"
  local tsfile_open="${objdir}/${TIMESTAMP_FILENAME}_OPEN"

  mv -f "${tsfile_open}" "${tsfile}"
}


# ts-newer-than dirA dirB
# Compare the make timestamps in both object directories.
# returns true (0) if dirA is newer than dirB
# returns false (1) otherwise.
#
# This functions errs on the side of returning 0, since
# that forces a rebuild anyway.

ts-newer-than() {
  local objdir1="$1"
  local objdir2="$2"

  local tsfile1="${objdir1}/${TIMESTAMP_FILENAME}"
  local tsfile2="${objdir2}/${TIMESTAMP_FILENAME}"

  if [ ! -d "${objdir1}" ]; then return 0; fi
  if [ ! -d "${objdir2}" ]; then return 0; fi

  if [ ! -f "${tsfile1}" ]; then return 0; fi
  if [ ! -f "${tsfile2}" ]; then return 0; fi

  local MODIFIED=$(find "${tsfile1}" -newer "${tsfile2}")
  if [ ${#MODIFIED} -gt 0 ]; then
    return 0
  fi
  return 1
}


# Don't define any functions after this or they won't show up in completions
function-completions() {
  if [ $# = 0 ]; then set -- ""; fi
  compgen -A function -- $1
  exit 0
}

######################################################################
######################################################################
#
#                               < MAIN >
#
######################################################################
######################################################################

mkdir -p "${INSTALL_ROOT}"

if [ $# = 0 ]; then set -- help; fi  # Avoid reference to undefined $1.

# Accept one -- argument for some compatibility with google3
if [ $1 = "--tab_completion_word" ]; then
  set -- function-completions $2
fi

if [ "$(type -t $1)" != "function" ]; then
  #Usage
  echo "ERROR: unknown function '$1'." >&2
  echo "For help, try:"
  echo "    $0 help"
  exit 1
fi

"$@"
