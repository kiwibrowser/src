#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
#@ This script creates the mips trusted SDK.
#@ It must be run from the native_client directory.

# This script is intended to build a mipsel-linux-gnu cross compilation
# toolchain that runs on x86 linux and generates code for a little-endian,
# hard-float, mips32 target.

######################################################################
# Config
######################################################################

set -o nounset
set -o errexit

readonly SCRIPT_DIR=$(dirname $0)
readonly NACL_ROOT=$(cd ${SCRIPT_DIR}/../.. && pwd)

readonly MAKE_OPTS="-j8"
readonly ARCH="mips32"

readonly GMP_URL="http://ftp.gnu.org/gnu/gmp/gmp-5.1.3.tar.bz2"
readonly GMP_SHA1SUM="b35928e2927b272711fdfbf71b7cfd5f86a6b165"

readonly MPFR_URL="http://ftp.gnu.org/gnu/mpfr/mpfr-3.1.2.tar.bz2"
readonly MPFR_SHA1SUM="46d5a11a59a4e31f74f73dd70c5d57a59de2d0b4"

readonly MPC_URL="http://ftp.gnu.org/gnu/mpc/mpc-1.0.2.tar.gz"
readonly MPC_SHA1SUM="5072d82ab50ec36cc8c0e320b5c377adb48abe70"

readonly GCC_URL="http://ftp.gnu.org/gnu/gcc/gcc-4.9.0/gcc-4.9.0.tar.bz2"
readonly GCC_SHA1SUM="fbde8eb49f2b9e6961a870887cf7337d31cd4917"

readonly BINUTILS_URL="http://ftp.gnu.org/gnu/binutils/binutils-2.24.tar.bz2"
readonly BINUTILS_SHA1SUM="7ac75404ddb3c4910c7594b51ddfc76d4693debb"

readonly KERNEL_URL="http://www.linux-mips.org/pub/linux/mips/kernel/v3.x/linux-3.14.2.tar.gz"
readonly KERNEL_SHA1SUM="9b094e817a7a9b7c09b5bc268e23de642c6c407a"

readonly GDB_URL="http://ftp.gnu.org/gnu/gdb/gdb-7.7.1.tar.bz2"
readonly GDB_SHA1SUM="35228319f7c715074a80be42fff64c7645227a80"

readonly GLIBC_URL="http://ftp.gnu.org/gnu/glibc/glibc-2.19.tar.bz2"
readonly GLIBC_SHA1SUM="382f4438a7321dc29ea1a3da8e7852d2c2b3208c"

readonly DOWNLOAD_QEMU_URL="http://wiki.qemu-project.org/download/qemu-2.0.0.tar.bz2"

readonly OUT_DIR=$(dirname ${NACL_ROOT})/out
readonly INSTALL_ROOT=${NACL_ROOT}/toolchain/linux_x86/mips_trusted
readonly JAIL_MIPS32=${INSTALL_ROOT}/sysroot
readonly TMP=${OUT_DIR}/sysroot_mips_trusted
readonly BUILD_DIR=${TMP}/build
readonly TAR_ARCHIVE=${OUT_DIR}/sysroot_mips_trusted_jessie.tar.gz
readonly PACKAGES="
libgcc1
libc6
libc6-dev
libstdc++6
libssl1.0.0
libssl-dev
zlib1g
zlib1g-dev
"

######################################################################
# Helper
######################################################################

Banner() {
  echo "######################################################################"
  echo $*
  echo "######################################################################"
}

SubBanner() {
  echo "......................................................................"
  echo $*
  echo "......................................................................"
}


Usage() {
  egrep "^#@" $0 | cut --bytes=3-
}

DownloadOrCopy() {
  local url=$1
  local filename="${TMP}/${url##*/}"
  local filetype="${url%%:*}"

  if [ "${filename}" == "" ]; then
    echo "Unknown error occured. Aborting."
    exit 1
  fi

  if [[ "${filetype}" ==  "http" || ${filetype} ==  "https" ]] ; then
    if [ ! -f "${filename}" ]; then
      SubBanner "downloading from ${url} -> ${filename}"
      wget ${url} -O ${filename}
    else
      SubBanner "using existing file: ${filename}"
    fi
  else
    SubBanner "copying from ${url}"
    cp ${url} ${filename}
  fi
}

DownloadOrCopyAndVerify() {
  local url=$1
  local checksum=$2
  local filename="${TMP}/${url##*/}"
  local filetype="${url%%:*}"

  if [ "${filename}" == "" ]; then
    echo "Unknown error occured. Aborting."
    exit 1
  fi

  if [[ "${filetype}" ==  "http" || ${filetype} ==  "https" ]] ; then
    if [ ! -f "${filename}" ]; then
      SubBanner "downloading from ${url} -> ${filename}"
      wget ${url} -O ${filename}
    else
      SubBanner "using existing file: ${filename}"
    fi
    if [ "${checksum}" != "nochecksum" ]; then
      if [ "$(sha1sum ${filename} | cut -d ' ' -f 1)" != "${checksum}" ]; then
        echo "${filename} sha1sum failed. Deleting file and aborting."
        rm -f ${filename}
        exit 1
      fi
    fi
  else
    SubBanner "copying from ${url}"
    cp ${url} ${filename}
  fi
}

######################################################################
#
######################################################################

# some sanity checks to make sure this script is run from the right place
# with the right tools.
SanityCheck() {
  Banner "Sanity Checks"
  if [[ $(basename $(pwd)) != "native_client" ]] ; then
    echo "ERROR: run this script from the native_client/ dir"
    exit -1
  fi

  if ! mkdir -p "${INSTALL_ROOT}" ; then
     echo "ERROR: ${INSTALL_ROOT} can't be created."
    exit -1
  fi

  for tool in wget ; do
    if ! which ${tool} ; then
      echo "Required binary $tool not found."
      echo "Exiting."
      exit 1
    fi
  done
}


ClearInstallDir() {
  Banner "clearing dirs in ${INSTALL_ROOT}"
  rm -rf ${INSTALL_ROOT}
  mkdir -p ${INSTALL_ROOT}
}


ClearBuildDir() {
  Banner "clearing dirs in ${BUILD_DIR}"
  rm -rf ${BUILD_DIR}/*
}


CreateTarBall() {
  Banner "creating tar ball ${TAR_ARCHIVE}"
  tar cfz ${TAR_ARCHIVE} -C ${INSTALL_ROOT} .
}


# Download the toolchain source tarballs or use a local copy when available.
DownloadOrCopyAndInstallToolchain() {
  Banner "Installing toolchain"

  tarball="${TMP}/${GCC_URL##*/}"
  DownloadOrCopyAndVerify ${GCC_URL} ${GCC_SHA1SUM}
  if [ ! -d ${TMP}/gcc-4.9.0 ]; then
    SubBanner "extracting from ${tarball}"
    tar jxf ${tarball} -C ${TMP}
  fi

  pushd ${TMP}/gcc-4.9.0

  local tarball="${TMP}/${GMP_URL##*/}"
  DownloadOrCopyAndVerify ${GMP_URL} ${GMP_SHA1SUM}
  SubBanner "extracting from ${tarball}"
  tar jxf ${tarball}
  local filename=`ls | grep gmp\-`
  rm -f gmp
  ln -s ${filename} gmp
  # Fix gmp configure problem with flex.
  sed -i "s/m4-not-needed/m4/" gmp/configure

  local tarball="${TMP}/${MPFR_URL##*/}"
  DownloadOrCopyAndVerify ${MPFR_URL} ${MPFR_SHA1SUM}
  SubBanner "extracting from ${tarball}"
  tar jxf ${tarball}
  local filename=`ls | grep mpfr\-`
  rm -f mpfr
  ln -s ${filename} mpfr

  local tarball="${TMP}/${MPC_URL##*/}"
  DownloadOrCopyAndVerify ${MPC_URL} ${MPC_SHA1SUM}
  SubBanner "extracting from ${tarball}"
  tar zxf ${tarball}
  local filename=`ls | grep mpc\-`
  rm -f mpc
  ln -s ${filename} mpc

  popd

  local tarball="${TMP}/${BINUTILS_URL##*/}"
  DownloadOrCopyAndVerify ${BINUTILS_URL} ${BINUTILS_SHA1SUM}
  if [ ! -d ${TMP}/binutils-2.24 ]; then
    SubBanner "extracting from ${tarball}"
    tar jxf ${tarball} -C ${TMP}
  fi

  tarball="${TMP}/${GDB_URL##*/}"
  DownloadOrCopyAndVerify ${GDB_URL} ${GDB_SHA1SUM}
  if [ ! -d ${TMP}/gdb-7.7.1 ]; then
    SubBanner "extracting from ${tarball}"
    tar jxf ${tarball} -C ${TMP}
  fi

  tarball="${TMP}/${KERNEL_URL##*/}"
  DownloadOrCopyAndVerify ${KERNEL_URL} ${KERNEL_SHA1SUM}
  SubBanner "extracting from ${tarball}"
  tar zxf ${tarball} -C ${TMP}

  tarball="${TMP}/${GLIBC_URL##*/}"
  DownloadOrCopyAndVerify ${GLIBC_URL} ${GLIBC_SHA1SUM}
  if [ ! -d ${TMP}/glibc-2.19 ]; then
    SubBanner "extracting from ${tarball}"
    tar jxf ${tarball} -C ${TMP}
  fi


  Banner "Preparing the code"

  # Fix a minor syntax issue in tc-mips.c.
  local OLD_TEXT="as_warn_where (fragp->fr_file, fragp->fr_line, msg);"
  local NEW_TEXT="as_warn_where (fragp->fr_file, fragp->fr_line, \"%s\", msg);"
  local FILE_NAME="${TMP}/binutils-2.24/gas/config/tc-mips.c"
  sed -i "s/${OLD_TEXT}/${NEW_TEXT}/g" "${FILE_NAME}"

  export PATH=${INSTALL_ROOT}/bin:$PATH
  local PREFIX_MIPSEL=/usr
  local PREFIX=/


  Banner "Building binutils"

  mkdir -p ${BUILD_DIR}/binutils/
  pushd ${BUILD_DIR}/binutils/

  SubBanner "Configuring"
  ${TMP}/binutils-2.24/configure \
    --prefix=${PREFIX}           \
    --target=mipsel-linux-gnu    \
    --with-sysroot=${JAIL_MIPS32}

  SubBanner "Make"
  make ${MAKE_OPTS} all-binutils all-gas all-ld

  SubBanner "Install"
  make ${MAKE_OPTS} DESTDIR=${INSTALL_ROOT} install-binutils install-gas \
    install-ld

  popd


  Banner "Building GCC (initial)"

  mkdir -p ${BUILD_DIR}/gcc/initial
  pushd ${BUILD_DIR}/gcc/initial

  SubBanner "Configuring"
  ${TMP}/gcc-4.9.0/configure \
    --prefix=${PREFIX}       \
    --disable-libssp         \
    --disable-libgomp        \
    --disable-libmudflap     \
    --disable-fixed-point    \
    --disable-decimal-float  \
    --with-mips-plt          \
    --with-endian=little     \
    --with-arch=${ARCH}      \
    --enable-languages=c     \
    --with-newlib            \
    --without-headers        \
    --disable-shared         \
    --disable-threads        \
    --disable-libquadmath    \
    --disable-libatomic      \
    --target=mipsel-linux-gnu

  SubBanner "Make"
  make ${MAKE_OPTS} all

  SubBanner "Install"
  make ${MAKE_OPTS} DESTDIR=${INSTALL_ROOT} install

  popd


  Banner "Installing Linux kernel headers"
  pushd ${TMP}/linux-3.14.2
  make headers_install ARCH=mips INSTALL_HDR_PATH=${JAIL_MIPS32}${PREFIX_MIPSEL}
  popd


  Banner "Building GLIBC"

  mkdir -p ${BUILD_DIR}/glibc
  pushd ${BUILD_DIR}/glibc

  BUILD_CC=gcc                      \
  AR=mipsel-linux-gnu-ar            \
  RANLIB=mipsel-linux-gnu-ranlibi   \
  CC=mipsel-linux-gnu-gcc           \
  CXX=mipsel-linux-gnu-g++          \
  ${TMP}/glibc-2.19/configure       \
    --prefix=${PREFIX_MIPSEL}       \
    --enable-add-ons                \
    --host=mipsel-linux-gnu         \
    --disable-profile               \
    --without-gd                    \
    --without-cvs                   \
    --build=i686-pc-linux-gnu       \
    --with-sysroot=${JAIL_MIPS32}   \
    --with-headers=${JAIL_MIPS32}${PREFIX_MIPSEL}/include


  SubBanner "Make"
  make ${MAKE_OPTS} all

  SubBanner "Install"
  make ${MAKE_OPTS} DESTDIR=${JAIL_MIPS32} install

  popd

  Banner "Building GCC (final)"

  mkdir -p ${BUILD_DIR}/gcc/final
  pushd ${BUILD_DIR}/gcc/final

  export MULTIARCH_DIRNAME=mipsel-linux-gnu

  ${TMP}/gcc-4.9.0/configure  \
    --prefix=${PREFIX}        \
    --disable-libssp          \
    --disable-libgomp         \
    --disable-libmudflap      \
    --disable-fixed-point     \
    --disable-decimal-float   \
    --with-mips-plt           \
    --with-endian=little      \
    --with-arch=${ARCH}       \
    --target=mipsel-linux-gnu \
    --enable-__cxa_atexit     \
    --enable-languages=c,c++  \
    --enable-multiarch        \
    --with-sysroot=${PREFIX}/sysroot \
    --with-build-sysroot=${JAIL_MIPS32}


  SubBanner "Make"
  make ${MAKE_OPTS} all

  SubBanner "Install"
  make ${MAKE_OPTS} DESTDIR=${INSTALL_ROOT} install

  popd


  Banner "Building GDB"

  mkdir -p ${BUILD_DIR}/gdb/
  pushd ${BUILD_DIR}/gdb/

  ${TMP}/gdb-7.7.1/configure   \
    --prefix=${PREFIX}         \
    --target=mipsel-linux-gnu

  SubBanner "Make"
  make ${MAKE_OPTS} all-gdb

  SubBanner "Install"
  make ${MAKE_OPTS} DESTDIR=${INSTALL_ROOT} install-gdb

  popd
}


# ----------------------------------------------------------------------
# mips32 deb files to complete our code sourcery jail
# ----------------------------------------------------------------------

readonly REPO=http://ftp.debian.org/debian
readonly MIPS32_PACKAGES=${REPO}/dists/jessie/main/binary-mipsel/Packages.gz
readonly PACKAGE_LIST=${SCRIPT_DIR}/packagelist.jessie.mipsel.base

#@
#@ GeneratePackageList
#@
GeneratePackageList() {
  Banner "generating package lists for mips32"
  rm -f ${TMP}/Packages.gz
  DownloadOrCopy ${MIPS32_PACKAGES}
  zcat ${TMP}/Packages.gz\
    | egrep '^(Package:|Filename:)' > ${TMP}/Packages_mipsel

  echo -n > ${PACKAGE_LIST}

  for pkg in ${PACKAGES} ; do
    grep  -A 1 "${pkg}\$" ${TMP}/Packages_mipsel\
      | egrep -o "pool/.*" >> ${PACKAGE_LIST}
  done
}

#@
#@ InstallPackages
#@
InstallPackages() {
  mkdir -p ${JAIL_MIPS32}
  local DEP_FILES_NEEDED_MIPS32=
  DEP_FILES_NEEDED_MIPS32=$(cat ${PACKAGE_LIST})

  for file in ${DEP_FILES_NEEDED_MIPS32} ; do
    local package="${TMP}/${file##*/}"
    Banner "installing ${file}"
    DownloadOrCopy ${REPO}/${file}
    SubBanner "extracting to ${JAIL_MIPS32}"
    dpkg --fsys-tarfile ${package}\
      | tar -xvf - --exclude=./usr/share -C ${JAIL_MIPS32}
  done
}

BuildAndInstallQemu() {
  local saved_dir=$(pwd)
  local tmpdir="${TMP}/qemu-mips.nacl"
  local tarball="qemu-2.0.0.tar.bz2"

  if [ -z "${DEBIAN_SYSROOT:-}" ]; then
    echo "Please set \$DEBIAN_SYSROOT to the location of a debian/stable"
    echo "sysroot."
    echo "e.g. <chrome>/build/linux/debian_wheezy_amd64-sysroot"
    echo "Which itself is setup by chrome's install-debian.wheezy.sysroot.py"
    exit 1
  fi

  if [ ! -d "${DEBIAN_SYSROOT:-}" ]; then
    echo "\$DEBIAN_SYSROOT does not exist: $DEBIAN_SYSROOT"
    exit 1
  fi

  Banner "Building qemu in ${tmpdir}"

  rm -rf ${tmpdir}
  mkdir ${tmpdir}
  cd ${tmpdir}

  SubBanner "Downloading"
  wget -c ${DOWNLOAD_QEMU_URL}

  SubBanner "Untarring"
  tar xf ${tarball}
  cd qemu-2.0.0

  SubBanner "Configuring"
  env -i CC=gcc-4.6 CXX=g++-4.6  PATH=/usr/bin/:/bin \
    ./configure \
    --extra-cflags="--sysroot=$DEBIAN_SYSROOT" \
    --extra-ldflags="-Wl,-rpath-link=$DEBIAN_SYSROOT/lib/amd64-linux-gnu" \
    --disable-system \
    --enable-linux-user \
    --disable-bsd-user \
    --target-list=mipsel-linux-user \
    --disable-sdl \
    --disable-linux-aio

  SubBanner "Make"
  env -i PATH=/usr/bin/:/bin make ${MAKE_OPTS}

  SubBanner "Install"
  cp mipsel-linux-user/qemu-mipsel ${INSTALL_ROOT}/qemu-mips32
  cd ${saved_dir}
  cp tools/trusted_cross_toolchains/qemu_tool_mips32.sh ${INSTALL_ROOT}
  ln -sf qemu_tool_mips32.sh ${INSTALL_ROOT}/run_under_qemu_mips32
}

CleanupSysrootSymlinks() {
  Banner "jail symlink cleanup"

  pushd ${JAIL_MIPS32}
  find usr/lib -type l -printf '%p %l\n' | while read link target; do
    # skip links with non-absolute paths
    if [[ ${target} != /* ]] ; then
      continue
    fi
    echo "${link}: ${target}"
    case "${link}" in
      usr/lib/mipsel-linux-gnu/*)
        # Relativize the symlink.
        ln -snfv "../../..${target}" "${link}"
        ;;
      usr/lib/*)
        # Relativize the symlink.
        ln -snfv "../..${target}" "${link}"
        ;;
    esac
  done

  find usr/lib -type l -printf '%p %l\n' | while read link target; do
    # Make sure we catch new bad links.
    if [ ! -r "${link}" ]; then
      echo "ERROR: FOUND BAD LINK ${link}"
      exit -1
    fi
  done
  popd
}

#@
#@ UploadArchive <revision>
#@
#@    Upload archive to Cloud Storage along with json manifest and
#@    update toolchain_revisions to point to new version.
#@    This requires write access the Cloud Storage bucket for Native Client.
UploadArchive() {
  local REV=$1
  local TAR_NAME=$(basename ${TAR_ARCHIVE})
  local GS_FILE=nativeclient-archive2/toolchain/${REV}/${TAR_NAME}
  local URL=https://storage.googleapis.com/${GS_FILE}
  set -x
  gsutil cp -a public-read ${TAR_ARCHIVE} gs://${GS_FILE}
  local package_version=build/package_version/package_version.py
  ${package_version} archive --archive-package linux_x86/mips_trusted \
      ${TAR_ARCHIVE}@${URL}
  ${package_version} upload --upload-package linux_x86/mips_trusted \
      --revision ${REV}
  ${package_version} setrevision --revision-package linux_x86/mips_trusted \
      --revision ${REV}
  set +x
}

help() {
  Usage
}

######################################################################
# Main
######################################################################

mkdir -p ${TMP}

BuildSysroot() {
  SanityCheck
  ClearInstallDir
  ClearBuildDir
  DownloadOrCopyAndInstallToolchain
  GeneratePackageList
  InstallPackages
  CleanupSysrootSymlinks
  BuildAndInstallQemu
  CreateTarBall
}

if [[ $# -eq 0 ]] ; then
  BuildSysroot
elif [[ "$(type -t $1)" != "function" ]]; then
  echo "ERROR: unknown function '$1'." >&2
  echo "For help, try:"
  echo "    $0 help"
  exit 1
else
  "$@"
fi
