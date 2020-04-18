#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
#@ This script builds the (trusted) sysroot image for ARM.
#@ It must be run from the native_client/ directory.
#@ Set $DIST to 'trusty' to build trusty rather then precise image.
#@
#@ The sysroot consists primarily of ARM headers and libraries.
#@ It also provides additional tools such as QEMU.
#@ It does NOT provide the actual cross compiler anymore.
#@ The cross compiler is now coming straight from a Debian package.
#@ So there is a one-time step required for all machines using this TC:
#@
#@  tools/trusted_cross_toolchains/trusted-toolchain-creator.armhf.sh InstallCrossArmBasePackages
#@
#@ Generally this script is invoked as:
#@
#@  tools/trusted_cross_toolchains/trusted-toolchain-creator.armhf.sh <mode> <args>*
#@
#@ List of modes:

######################################################################
# Config
######################################################################

set -o nounset
set -o errexit

DIST=${DIST:-precise}
readonly SCRIPT_DIR=$(cd $(dirname ${BASH_SOURCE}) && pwd)
readonly NACL_ROOT=$(cd ${SCRIPT_DIR}/../.. && pwd)
# this where we create the sysroot image
readonly INSTALL_ROOT=${NACL_ROOT}/toolchain/linux_x86/arm_trusted
readonly TAR_ARCHIVE=$(dirname ${NACL_ROOT})/out/sysroot_arm_trusted_${DIST}.tgz
readonly TMP=$(dirname ${NACL_ROOT})/out/sysroot_arm_trusted_${DIST}
readonly REQUIRED_TOOLS="wget"
readonly MAKE_OPTS="-j8"

######################################################################
# Package Config
######################################################################

# this where we get the cross toolchain from for the manual install:
readonly CROSS_ARM_TC_REPO=http://archive.ubuntu.com/ubuntu
# this is where we get all the armhf packages from
readonly REPO=http://ports.ubuntu.com/ubuntu-ports

readonly PACKAGE_LIST="${REPO}/dists/${DIST}/main/binary-armhf/Packages.bz2"
readonly PACKAGE_LIST2="${REPO}/dists/${DIST}-security/main/binary-armhf/Packages.bz2"

# Packages for the host system
readonly CROSS_ARM_TC_PACKAGES="g++-arm-linux-gnueabihf"

# NOTE: the package listing here should be updated using the
# GeneratePackageListXXX() functions below
readonly BASE_DEP_LIST="${SCRIPT_DIR}/packagelist.${DIST}.armhf.base"
if [ ! -f ${BASE_DEP_LIST} ]; then
  echo "Missing file: ${BASE_DEP_LIST}"
  exit 1
fi
readonly BASE_DEP_FILES="$(cat ${BASE_DEP_LIST})"

readonly RAW_DEP_LIST="${SCRIPT_DIR}/packagelist.${DIST}.armhf.sh"

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
  if [[ -f "$2" ]] ; then
     echo "$2 already in place"
  elif [[ $1 =~  'http://' ]] ; then
    SubBanner "downloading from $1 -> $2"
    wget $1 -O $2
  else
    SubBanner "copying from $1"
    cp $1 $2
  fi
}


# some sanity checks to make sure this script is run from the right place
# with the right tools
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

  if ! mkdir -p "${TMP}" ; then
     echo "ERROR: ${TMP} can't be created."
    exit -1
  fi

  for tool in ${REQUIRED_TOOLS} ; do
    if ! which ${tool} ; then
      echo "Required binary $tool not found."
      echo "Exiting."
      exit 1
    fi
  done
}


ChangeDirectory() {
  # Change direcotry to top 'native_client' directory.
  cd ${NACL_ROOT}
}


# TODO(robertm): consider wiping all of ${BASE_DIR}
ClearInstallDir() {
  Banner "clearing dirs in ${INSTALL_ROOT}"
  rm -rf ${INSTALL_ROOT}/*
}


CreateTarBall() {
  Banner "creating tar ball ${TAR_ARCHIVE}"
  tar cfz ${TAR_ARCHIVE} -C ${INSTALL_ROOT} .
}

######################################################################
# One of these has to be run ONCE per machine
######################################################################

#@
#@ InstallCrossArmBasePackages
#@
#@      Install packages needed for arm cross compilation.
InstallCrossArmBasePackages() {
  sudo apt-get install ${CROSS_ARM_TC_PACKAGES}
}

######################################################################
#
######################################################################

HacksAndPatches() {
  rel_path=toolchain/linux_x86/arm_trusted
  Banner "Misc Hacks & Patches"
  # these are linker scripts with absolute pathnames in them
  # which we rewrite here
  lscripts="${rel_path}/usr/lib/arm-linux-gnueabihf/libpthread.so \
            ${rel_path}/usr/lib/arm-linux-gnueabihf/libc.so"

  SubBanner "Rewriting Linker Scripts"
  sed -i -e 's|/usr/lib/arm-linux-gnueabihf/||g' ${lscripts}
  sed -i -e 's|/lib/arm-linux-gnueabihf/||g' ${lscripts}
}


InstallMissingArmLibrariesAndHeadersIntoSysroot() {
  Banner "Install Libs And Headers Into Sysroot"

  mkdir -p ${TMP}/armhf-packages
  mkdir -p ${INSTALL_ROOT}
  for file in $@ ; do
    local package="${TMP}/armhf-packages/${file##*/}"
    Banner "installing ${file}"
    DownloadOrCopy ${REPO}/pool/${file} ${package}
    SubBanner "extracting to ${INSTALL_ROOT}"
    if [[ ! -s ${package} ]] ; then
      echo
      echo "ERROR: bad package ${package}"
      exit -1
    fi
    dpkg --fsys-tarfile ${package}\
      | tar -xvf - --exclude=./usr/share -C ${INSTALL_ROOT}
  done
}


CleanupSysrootSymlinks() {
  Banner "jail symlink cleanup"

  pushd ${INSTALL_ROOT}
  find usr/lib -type l -printf '%p %l\n' | while read link target; do
    # skip links with non-absolute paths
    if [[ ${target} != /* ]] ; then
      continue
    fi
    echo "${link}: ${target}"
    case "${link}" in
      usr/lib/arm-linux-gnueabihf/*)
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
#@ BuildAndInstallQemu
#@
#@     Build ARM emulator including some patches for better tracing
#
# Historic Notes:
# Traditionally we were building static 32 bit images of qemu on a
# 64bit system which would run then on both x86-32 and x86-64 systems.
# The latest version of qemu contains new dependencies which
# currently make it impossible to build such images on 64bit systems
# We can build a static 64bit qemu but it does not work with
# the sandboxed translators for unknown reason.
# So instead we chose to build 32bit shared images.
#

readonly QEMU_TARBALL=qemu-2.3.0.tar.bz2
readonly QEMU_SHA=373d74bfafce1ca45f85195190d0a5e22b29299e
readonly QEMU_DIR=qemu-2.3.0

readonly QEMU_URL=http://wiki.qemu-project.org/download/${QEMU_TARBALL}
readonly QEMU_PATCH=tools/trusted_cross_toolchains/${QEMU_DIR}.patch_arm


BuildAndInstallQemu() {
  local saved_dir=$(pwd)
  local tmpdir="${TMP}/qemu.nacl"

  Banner "Building qemu in ${tmpdir}"

  if [ -z "${DEBIAN_SYSROOT:-}" ]; then
    echo "Please set \$DEBIAN_SYSROOT to the location of a debian/stable"
    echo "32-bit sysroot"
    echo "e.g. <chrome>/build/linux/debian_wheezy_amd64-sysroot"
    echo "Which itself is setup by chrome's install-debian.wheezy.sysroot.py"
    exit 1
  fi

  if [ ! -d "${DEBIAN_SYSROOT:-}" ]; then
    echo "\$DEBIAN_SYSROOT does not exist: $DEBIAN_SYSROOT"
    exit 1
  fi

  if [ -n "${QEMU_URL}" ]; then
    if [[ ! -f ${TMP}/${QEMU_TARBALL} ]]; then
      wget -O ${TMP}/${QEMU_TARBALL} $QEMU_URL
    fi

    echo "${QEMU_SHA}  ${TMP}/${QEMU_TARBALL}" | sha1sum --check -
  else
    if [[ ! -f "$QEMU_TARBALL" ]] ; then
      echo "ERROR: missing qemu tarball: $QEMU_TARBALL"
      exit 1
    fi
  fi

  rm -rf ${tmpdir}
  mkdir ${tmpdir}
  cd ${tmpdir}
  SubBanner "Untaring ${QEMU_TARBALL}"
  tar xf ${TMP}/${QEMU_TARBALL}
  cd ${QEMU_DIR}

  SubBanner "Patching ${QEMU_PATCH}"
  patch -p1 < ${saved_dir}/${QEMU_PATCH}

  SubBanner "Configuring"
  set -x
  # We forace gcc-4.6 since this the DEBIAN_SYSROOT image only
  # contains that C++ headers for version 4.6.
  env -i CC=gcc-4.6 CXX=g++-4.6  PATH=/usr/bin/:/bin LIBS=-lrt \
    ./configure \
    --extra-cflags="--sysroot=$DEBIAN_SYSROOT" \
    --extra-ldflags="-Wl,-rpath-link=$DEBIAN_SYSROOT/lib/amd64-linux-gnu" \
    --disable-system \
    --disable-docs \
    --enable-linux-user \
    --disable-bsd-user \
    --target-list=arm-linux-user \
    --disable-smartcard-nss \
    --disable-sdl

  SubBanner "Make"
  env -i PATH=/usr/bin/:/bin make ${MAKE_OPTS}

  SubBanner "Install ${INSTALL_ROOT}"
  cp arm-linux-user/qemu-arm ${INSTALL_ROOT}
  cd ${saved_dir}
  cp tools/trusted_cross_toolchains/qemu_tool_arm.sh ${INSTALL_ROOT}
  ln -sf qemu_tool_arm.sh ${INSTALL_ROOT}/run_under_qemu_arm
  set +x
}

#@
#@ BuildSysroot
#@
#@    Build everything and package it
BuildSysroot() {
  ClearInstallDir
  InstallMissingArmLibrariesAndHeadersIntoSysroot ${BASE_DEP_FILES}
  CleanupSysrootSymlinks
  HacksAndPatches
  BuildAndInstallQemu
  CreateTarBall
}

#@
#@ UploadArchive <revision>
#@
#@    Upload archive to Cloud Storage along with json manifest and
#@    update toolchain_revisions to point to new version.
#@    This requires write access the Cloud Storage bucket for Native Client.
UploadArchive() {
  REV=$1
  TAR_NAME=$(basename ${TAR_ARCHIVE})
  GS_FILE=nativeclient-archive2/toolchain/${REV}/${TAR_NAME}
  URL=https://storage.googleapis.com/${GS_FILE}
  set -x
  gsutil cp -a public-read ${TAR_ARCHIVE} gs://${GS_FILE}
  local package_version=build/package_version/package_version.py
  ${package_version} archive --archive-package arm_trusted ${TAR_ARCHIVE}@${URL}
  ${package_version} upload --upload-package arm_trusted --revision ${REV}
  ${package_version} setrevision --revision-package arm_trusted --revision \
      ${REV}
  set +x
}

#
# GeneratePackageList
#
#     Looks up package names in ${TMP}/Packages and write list of URLs
#     to output file.
#
GeneratePackageList() {
  local output_file=$1
  echo "Updating: ${output_file}"
  /bin/rm -f ${output_file}
  shift
  for pkg in $@ ; do
    local pkg_full=$(grep -A 1 "${pkg}\$" ${TMP}/Packages | tail -1 | egrep -o "pool/.*")
    if [[ -z ${pkg_full} ]]; then
        echo "ERROR: missing package: $pkg"
        exit 1
    fi
    echo $pkg_full | sed "s/^pool\///" >> $output_file
  done
  # sort -o does an in-place sort of this file
  sort $output_file -o $output_file
}

#@
#@ UpdatePackageLists
#@
#@    Regenerate the armhf package lists such that they contain an up-to-date
#@    list of URLs within the ubuntu archive.
#@
UpdatePackageLists() {
  local package_list="${TMP}/Packages.${DIST}.bz2"
  local package_list2="${TMP}/Packages.${DIST}-security.bz2"
  DownloadOrCopy ${PACKAGE_LIST} ${package_list}
  DownloadOrCopy ${PACKAGE_LIST2} ${package_list2}
  bzcat ${package_list} ${package_list2} | egrep '^(Package:|Filename:)' > ${TMP}/Packages

  . "${RAW_DEP_LIST}"
  GeneratePackageList ${BASE_DEP_LIST} "${BASE_PACKAGES}"
}

help() {
  Usage
}

if [[ $# -eq 0 ]] ; then
  echo "ERROR: you must specify a mode on the commandline"
  echo
  Usage
  exit -1
elif [[ "$(type -t $1)" != "function" ]]; then
  echo "ERROR: unknown function '$1'." >&2
  echo "For help, try:"
  echo "    $0 help"
  exit 1
else
  ChangeDirectory
  SanityCheck
  "$@"
fi
