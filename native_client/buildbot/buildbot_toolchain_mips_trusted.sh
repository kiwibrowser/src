#!/bin/bash
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script assumed to be run in native_client/
if [[ $(basename $(pwd)) != "native_client" ]]; then
  echo "ERROR: must be run in native_client!"
  exit 1
fi

set -x
set -e
set -u

readonly UP_DOWN_LOAD="buildbot/file_up_down_load.sh"
readonly TRUSTED_TOOLCHAIN_CREATOR=\
"tools/trusted_cross_toolchains/trusted-toolchain-creator.mipsel.debian.sh"

echo @@@BUILD_STEP compile_toolchain@@@
${TRUSTED_TOOLCHAIN_CREATOR} nacl_sdk

echo @@@BUILD_STEP upload_tarball@@@
${UP_DOWN_LOAD} UploadToolchainTarball ${BUILDBOT_GOT_REVISION} \
  linux_mips-trusted nacl_sdk.tgz
