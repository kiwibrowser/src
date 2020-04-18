#!/bin/bash
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script assumed to be run in native_client/
if [[ $(pwd) != */native_client ]]; then
  echo "ERROR: must be run in native_client!"
  exit 1
fi

if [ $# -ne 0 ]; then
  echo "USAGE: $0"
  exit 2
fi

set -x
set -e
set -u


echo @@@BUILD_STEP clobber@@@
rm -rf scons-out toolchain ../xcodebuild ../out

echo @@@BUILD_STEP compile_toolchain@@@
tools/llvm/trusted-toolchain-creator.sh trusted_sdk arm-trusted.tgz
chmod a+r arm-trusted.tgz

echo @@@BUILD_STEP untar_toolchain@@@
# Untar toolchain mainly to be sure we can.
mkdir -p toolchain/linux_x86/arm_trusted
cd toolchain/linux_x86/arm_trusted
tar xfz ../../../arm-trusted.tgz
# Check that we can go into a part of it.
cd arm-2009q3
cd ../../..

if [[ "${BUILDBOT_SLAVE_TYPE:-Trybot}" != "Trybot" ]]; then
  echo @@@BUILD_STEP archive_build@@@
  gsutil=buildbot/gsutil.sh
  GS_BASE=gs://nativeclient-archive2/toolchain
  ${gsutil} cp -a public-read \
      arm-trusted.tgz \
      ${GS_BASE}/${BUILDBOT_GOT_REVISION}/naclsdk_linux_arm-trusted.tgz
  ${gsutil} -h Cache-Control:no-cache cp -a public-read \
      arm-trusted.tgz \
      ${GS_BASE}/latest/naclsdk_linux_arm-trusted.tgz
fi
