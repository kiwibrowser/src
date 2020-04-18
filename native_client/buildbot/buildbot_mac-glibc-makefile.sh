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
# toolchain/mac_x86/nacl_x86_glibc we have to keep the old format
# inside of the tar (toolchain/mac_x86) so that the untar toolchain script
# is backwards compatible and can untar old tars. Eventually this will be
# unnecessary with the new package_version scheme since how to untar the
# tar file will be embedded inside of the package file so they can differ
# between revisions.
export TOOLCHAINLOC=toolchain
export TOOLCHAINNAME=mac_x86
export INST_GLIBC_PROGRAM="$PWD/tools/glibc_download.sh"

# This is where we want the toolchain when moving to native_client/toolchain.
OUT_TOOLCHAINLOC=toolchain/mac_x86
OUT_TOOLCHAINNAME=nacl_x86_glibc
CORE_SDK=core_sdk
CORE_SDK_WORK=core_sdk_work

TOOL_TOOLCHAIN="$TOOLCHAINLOC/$TOOLCHAINNAME"
OUT_TOOLCHAIN="${OUT_TOOLCHAINLOC}/${OUT_TOOLCHAINNAME}"

echo @@@BUILD_STEP gclient_runhooks@@@
gclient runhooks --force

echo @@@BUILD_STEP clobber_toolchain@@@
rm -rf scons-out tools/BUILD/* tools/out/* tools/toolchain \
  tools/glibc tools/glibc.tar tools/toolchain.t* "${OUT_TOOLCHAIN}" \
  tools/${CORE_SDK} tools/${CORE_SDK_WORK} .tmp ||
  echo already_clean
mkdir -p "tools/${TOOL_TOOLCHAIN}"

echo @@@BUILD_STEP clean_sources@@@
tools/update_all_repos_to_latest.sh

# glibc_download.sh can return three return codes:
#  0 - glibc is successfully downloaded and installed
#  1 - glibc is not downloaded but another run may help
#  2+ - glibc is not downloaded and can not be downloaded later
#
# If the error result is 2 or more we are stopping the build
echo @@@BUILD_STEP check_glibc_revision_sanity@@@
echo "Try to download glibc revision $(tools/glibc_revision.sh)"
if tools/glibc_download.sh "tools/${TOOL_TOOLCHAIN}" 1; then
  INST_GLIBC_PROGRAM=true
elif (($?>1)); then
  echo @@@STEP_FAILURE@@@
  exit 100
fi

if [[ "${BUILDBOT_SLAVE_TYPE:-Trybot}" == "Trybot" ]]; then
echo @@@BUILD_STEP setup source@@@
(cd tools; ./buildbot_patch-toolchain-tries.sh)
fi

echo @@@BUILD_STEP compile_toolchain@@@
(
  cd tools
  make -j8 buildbot-build-with-glibc
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
  ./canonicalize_timestamps.sh "${TOOLCHAINLOC}"
  ./canonicalize_timestamps.sh "${CORE_SDK}"

  echo @@@BUILD_STEP tar_toolchain@@@
  tar Scf toolchain.tar "${TOOLCHAINLOC}"
  bzip2 -k -9 toolchain.tar
  gzip -n -9 toolchain.tar

  echo @@@BUILD_STEP tar_core_sdk@@@
  tar Scf core_sdk.tar "${CORE_SDK}"
  bzip2 -k -9 core_sdk.tar
  gzip -n -9 core_sdk.tar
)

echo @@@BUILD_STEP archive_build@@@
for suffix in gz bz2 ; do
  $GSUTIL cp -a public-read \
    tools/toolchain.tar.$suffix \
    gs://${GSD_BUCKET}/${UPLOAD_LOC}/toolchain_mac_x86.tar.$suffix
  $GSUTIL cp -a public-read \
    tools/core_sdk.tar.$suffix \
    gs://${GSD_BUCKET}/${UPLOAD_LOC}/core_sdk_mac_x86.tar.$suffix
done
echo @@@STEP_LINK@download@http://gsdview.appspot.com/${GSD_BUCKET}/${UPLOAD_LOC}/@@@

echo @@@BUILD_STEP archive_extract_packages@@@
python build/package_version/package_version.py \
  archive --archive-package=${TOOLCHAINNAME}/nacl_x86_glibc --extract \
  --extra-archive gdb_x86_64_apple_darwin.tgz \
  tools/toolchain.tar.bz2,${TOOL_TOOLCHAIN}@https://storage.googleapis.com/${GSD_BUCKET}/${UPLOAD_LOC}/toolchain_mac_x86.tar.bz2 \
  tools/core_sdk.tar.bz2,${CORE_SDK}@https://storage.googleapis.com/${GSD_BUCKET}/${UPLOAD_LOC}/core_sdk_mac_x86.tar.bz2

python build/package_version/package_version.py \
  archive --archive-package=${TOOLCHAINNAME}/nacl_x86_glibc_raw --extract \
  --extra-archive gdb_x86_64_apple_darwin.tgz \
  tools/toolchain.tar.bz2,${TOOL_TOOLCHAIN}@https://storage.googleapis.com/${GSD_BUCKET}/${UPLOAD_LOC}/toolchain_mac_x86.tar.bz2

echo @@@BUILD_STEP upload_package_info@@@
python build/package_version/package_version.py \
  --cloud-bucket ${GSD_BUCKET} --annotate \
  upload --skip-missing \
  --upload-package=${TOOLCHAINNAME}/nacl_x86_glibc --revision=${UPLOAD_REV}

python build/package_version/package_version.py \
  --cloud-bucket ${GSD_BUCKET} --annotate \
  upload --skip-missing \
  --upload-package=${TOOLCHAINNAME}/nacl_x86_glibc_raw --revision=${UPLOAD_REV}

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

export INSIDE_TOOLCHAIN=1
python buildbot/buildbot_standard.py --scons-args='no_gdb_tests=1' \
  opt 32 glibc || fail


# sync_backports is obsolete and should probably be removed.
# if [[ "${BUILD_COMPATIBLE_TOOLCHAINS:-yes}" != "no" ]]; then
#   echo @@@BUILD_STEP sync backports@@@
#   rm -rf tools/BACKPORTS/ppapi*
#   tools/BACKPORTS/build_backports.sh VERSIONS mac glibc
# fi

exit $exit_status
