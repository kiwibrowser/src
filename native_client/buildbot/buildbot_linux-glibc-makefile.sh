#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script assumed to be run in native_client/
if [[ ${PWD} != */native_client ]]; then
  echo "ERROR: must be run in native_client!"
  exit 1
fi

set -x
set -e
set -u

# Transitionally, even though our new toolchain location is under
# toolchain/linux_x86/nacl_x86_glibc we have to keep the old format
# inside of the tar (toolchain/linux_x86) so that the untar toolchain script
# is backwards compatible and can untar old tars. Eventually this will be
# unnecessary with the new package_version scheme since how to untar the
# tar file will be embedded inside of the package file so they can differ
# between revisions.
export TOOLCHAINLOC=toolchain
export TOOLCHAINNAME=linux_x86

# This is where we want the toolchain when moving to native_client/toolchain.
OUT_TOOLCHAINLOC=toolchain/linux_x86
OUT_TOOLCHAINNAME=nacl_x86_glibc
CORE_SDK=core_sdk
CORE_SDK_WORK=core_sdk_work

TOOL_TOOLCHAIN="${TOOLCHAINLOC}/${TOOLCHAINNAME}"
OUT_TOOLCHAIN="${OUT_TOOLCHAINLOC}/${OUT_TOOLCHAINNAME}"

echo @@@BUILD_STEP gclient_runhooks@@@
gclient runhooks --force

echo @@@BUILD_STEP clobber_toolchain@@@
rm -rf scons-out tools/SRC/*.patch* tools/BUILD/* tools/out tools/toolchain \
  tools/glibc tools/glibc.tar tools/toolchain.t* "${OUT_TOOLCHAIN}" \
  tools/${CORE_SDK} tools/${CORE_SDK_WORK} .tmp ||
  echo already_clean

echo @@@BUILD_STEP clean_sources@@@
tools/update_all_repos_to_latest.sh

if [[ "${BUILDBOT_SLAVE_TYPE:-Trybot}" == "Trybot" ]]; then
echo @@@BUILD_STEP setup source@@@
(cd tools; ./buildbot_patch-toolchain-tries.sh)
fi

echo @@@BUILD_STEP compile_toolchain@@@
(
  cd tools
  make -j8 buildbot-build-with-glibc
  if [[ "${BUILDBOT_SLAVE_TYPE:-Trybot}" != "Trybot" ]]; then
    make install-glibc INST_GLIBC_PREFIX="$PWD"
  fi
  mkdir -p SRC/newlib/newlib/libc/sys/nacl/include
  cp -aiv ../src/untrusted/pthread/{pthread.h,semaphore.h} \
    SRC/newlib/newlib/libc/sys/nacl/include
  find SRC/newlib/newlib/libc/sys/nacl -name .svn -print0 | xargs -0 rm -rf --
  ( cd SRC/newlib/newlib/libc/sys ; git add nacl )
  for file in SRC/gcc/gcc/configure.ac SRC/gcc/gcc/configure ; do
    cp -aiv $file $file.orig
    sed -e s"|\(CROSS_SYSTEM_HEADER_DIR='\)\(\\\$(gcc_tooldir)/sys-include'\)|\1\$(DESTDIR)\2|" \
      < $file.orig > $file
    touch -r $file.orig $file
    rm $file.orig
    ( cd SRC/gcc/gcc ; git add $(basename $file) )
  done
  make patches
  for patchname in SRC/*.patch ; do
    xz -k -9 "$patchname"
    bzip2 -k -9 "$patchname"
    gzip -9 "$patchname"
    zcat "$patchname".gz > "$patchname"
  done
  mkdir linux
  cp -aiv {SRC/linux-headers-for-nacl/include/,}linux/getcpu.h
  cp -aiv {../src/untrusted/include/machine/,}_default_types.h
  cp -aiv ../LICENSE LICENSE
  mv Makefile Makefile.orig
  . REVISIONS
  sed -e s"|^\\(CANNED_REVISION = \\)no$|\\1$BUILDBOT_GOT_REVISION|" \
      -e s'|^\(SRCDIR =\).*$|\1|' \
      -e s'|\(GCC_CC = \)gcc -m$(HOST_TOOLCHAIN_BITS)|\1gcc|' \
      -e s'|\(GLIBC_CC =.*\)|\1 -I$(abspath $(dir $(THISMAKEFILE)))|' \
      -e s'|\(LINUX_HEADERS = \).*|\1/usr/include|' \
      -e s"|\\(export NACL_FAKE_SONAME\\).*|\\1 = ${NACL_GLIBC_COMMIT:0:8}|" \
         < Makefile.orig > Makefile
  tar czSvpf nacltoolchain-buildscripts-r${BUILDBOT_GOT_REVISION}.tar.gz \
    LICENSE Makefile download_SRC.sh \
    _default_types.h linux newlib-libc-script \
    create_redirector{,s,s_cygwin}.sh redirector.exe redirect_table.txt \
    redirector/redirector{.c,.h} redirector/redirector_table.txt
  rm Makefile
  mv Makefile.orig Makefile
  rm linux/getcpu.h _default_types.h LICENSE
  rmdir linux
)

if [[ "${BUILDBOT_SLAVE_TYPE:-Trybot}" != "Trybot" ]]; then
  echo @@@BUILD_STEP tar_glibc@@@
  (
    cd tools
    cp --archive --sparse=always glibc glibc_sparse
    rm -rf glibc
    mv glibc_sparse glibc
    cd glibc
    tar zScf ../glibc.tgz ./*
    chmod a+r ../glibc.tgz
  )

  echo @@@BUILD_STEP archive_glibc@@@
  rev="$(tools/glibc_revision.sh)"
  wget https://gsdview.appspot.com/nativeclient-archive2/between_builders/x86_glibc/r"$rev"/glibc_x86.tar.gz -O /dev/null ||
  $GSUTIL cp -a public-read \
    tools/glibc.tgz \
    gs://nativeclient-archive2/between_builders/x86_glibc/r"$rev"/glibc_x86.tar.gz
  echo @@@STEP_LINK@download@http://gsdview.appspot.com/nativeclient-archive2/between_builders/x86_glibc/r"$rev"/@@@
fi

(
  cd tools
  echo @@@BUILD_STEP sparsify_toolchain@@@
  cp --archive --sparse=always "${TOOL_TOOLCHAIN}" "${TOOL_TOOLCHAIN}_sparse"
  rm -rf "${TOOL_TOOLCHAIN}"
  mv "${TOOL_TOOLCHAIN}_sparse" "${TOOL_TOOLCHAIN}"
)

echo @@@BUILD_STEP build_core_sdk@@@
# Use scons to generate the SDK headers and libraries.
python scons.py --nacl_glibc MODE=nacl naclsdk_validate=0 \
  nacl_glibc_dir="tools/${TOOL_TOOLCHAIN}" \
  DESTINATION_ROOT="tools/${CORE_SDK_WORK}" \
  includedir="tools/${CORE_SDK}/x86_64-nacl/include" \
  install_headers

python scons.py --nacl_glibc MODE=nacl naclsdk_validate=0 \
  platform=x86-32 \
  nacl_glibc_dir="tools/${TOOL_TOOLCHAIN}" \
  DESTINATION_ROOT="tools/${CORE_SDK_WORK}" \
  libdir="tools/${CORE_SDK}/x86_64-nacl/lib32" \
  install_lib

python scons.py --nacl_glibc MODE=nacl naclsdk_validate=0 \
  platform=x86-64 \
  nacl_glibc_dir="tools/${TOOL_TOOLCHAIN}" \
  DESTINATION_ROOT="tools/${CORE_SDK_WORK}" \
  libdir="tools/${CORE_SDK}/x86_64-nacl/lib" \
  install_lib


if [[ "${BUILDBOT_SLAVE_TYPE:-Trybot}" != "Trybot" ]]; then
  GSD_BUCKET=nativeclient-archive2
  UPLOAD_REV=${BUILDBOT_GOT_REVISION}
  UPLOAD_LOC=x86_toolchain/r${UPLOAD_REV}
else
  GSD_BUCKET=nativeclient-trybot/packages
  UPLOAD_REV=${BUILDBOT_BUILDERNAME}/${BUILDBOT_BUILDNUMBER}
  UPLOAD_LOC=x86_toolchain/${UPLOAD_REV}
fi

(
  cd tools
  echo @@@BUILD_STEP canonicalize timestamps@@@
  ./canonicalize_timestamps.sh "${TOOL_TOOLCHAIN}"
  ./canonicalize_timestamps.sh "${CORE_SDK}"

  echo @@@BUILD_STEP tar_toolchain@@@
  tar Scf toolchain.tar "${TOOL_TOOLCHAIN}"
  xz -k -9 toolchain.tar
  bzip2 -k -9 toolchain.tar
  gzip -n -9 toolchain.tar

  echo @@@BUILD_STEP tar_core_sdk@@@
  tar Scf core_sdk.tar "${CORE_SDK}"
  xz -k -9 core_sdk.tar
  bzip2 -k -9 core_sdk.tar
  gzip -n -9 core_sdk.tar
)

echo @@@BUILD_STEP archive_build@@@
for suffix in gz bz2 xz ; do
  $GSUTIL cp -a public-read \
    tools/toolchain.tar.$suffix \
    gs://${GSD_BUCKET}/${UPLOAD_LOC}/toolchain_linux_x86.tar.$suffix
  $GSUTIL cp -a public-read \
    tools/core_sdk.tar.$suffix \
    gs://${GSD_BUCKET}/${UPLOAD_LOC}/core_sdk_linux_x86.tar.$suffix
done
for patch in \
    tools/nacltoolchain-buildscripts-r${BUILDBOT_GOT_REVISION}.tar.gz \
    tools/SRC/*.patch* ; do
  filename="${patch#tools/}"
  filename="${filename#SRC/}"
  $GSUTIL cp -a public-read \
    $patch \
    gs://${GSD_BUCKET}/${UPLOAD_LOC}/$filename
done
echo @@@STEP_LINK@download@http://gsdview.appspot.com/${GSD_BUCKET}/${UPLOAD_LOC}/@@@

echo @@@BUILD_STEP archive_extract_packages@@@
python build/package_version/package_version.py \
  archive --archive-package=${TOOLCHAINNAME}/nacl_x86_glibc --extract \
  --extra-archive gdb_i686_linux.tgz \
  tools/toolchain.tar.bz2,${TOOL_TOOLCHAIN}@https://storage.googleapis.com/${GSD_BUCKET}/${UPLOAD_LOC}/toolchain_linux_x86.tar.bz2 \
  tools/core_sdk.tar.bz2,${CORE_SDK}@https://storage.googleapis.com/${GSD_BUCKET}/${UPLOAD_LOC}/core_sdk_linux_x86.tar.bz2

python build/package_version/package_version.py \
  archive --archive-package=${TOOLCHAINNAME}/nacl_x86_glibc_raw --extract \
  --extra-archive gdb_i686_linux.tgz \
  tools/toolchain.tar.bz2,${TOOL_TOOLCHAIN}@https://storage.googleapis.com/${GSD_BUCKET}/${UPLOAD_LOC}/toolchain_linux_x86.tar.bz2

echo @@@BUILD_STEP upload_package_info@@@
python build/package_version/package_version.py \
  --cloud-bucket=${GSD_BUCKET} --annotate \
  upload --skip-missing \
  --upload-package=${TOOLCHAINNAME}/nacl_x86_glibc --revision=${UPLOAD_REV}

python build/package_version/package_version.py \
  --cloud-bucket=${GSD_BUCKET} --annotate \
  upload --skip-missing \
  --upload-package=${TOOLCHAINNAME}/nacl_x86_glibc_raw --revision=${UPLOAD_REV}

echo @@@BUILD_STEP glibc_tests64@@@
(
  cd tools
  make glibc-check
)

# The script should exit nonzero if any test run fails.
# But that should not short-circuit the script due to the 'set -e' behavior.
exit_status=0
fail() {
  exit_status=1
  return 0
}

# Before we start testing, put in dummy mock archives so gyp can still untar
# the entire package.
python build/package_version/package_version.py fillemptytars \
  --fill-package nacl_x86_glibc

# First run 32bit tests, then 64bit tests.  Both should succeed.
export INSIDE_TOOLCHAIN=1
python buildbot/buildbot_standard.py --scons-args='no_gdb_tests=1' \
  --step-suffix=' (32)' opt 32 glibc || fail

python buildbot/buildbot_standard.py --scons-args='no_gdb_tests=1' \
  --step-suffix=' (64)' opt 64 glibc || fail

# sync_backports is obsolete and should probably be removed.
# if [[ "${BUILD_COMPATIBLE_TOOLCHAINS:-yes}" != "no" ]]; then
#   echo @@@BUILD_STEP sync backports@@@
#   rm -rf tools/BACKPORTS/ppapi*
#   tools/BACKPORTS/build_backports.sh VERSIONS linux glibc
# fi

exit $exit_status
