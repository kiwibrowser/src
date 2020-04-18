#!/bin/bash
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


# This script allows an LLVM change to be sent to the PNaCl toolchain
# trybots for testing.  It packages up the LLVM Git commit as a
# base64-encoded Git bundle so that it can be sent to the trybots as a
# text-only patch file.
#
# This script assumes you are using a Git checkout of the
# native_client source tree.
#
# Example usage: After making changes to toolchain_build/src/llvm and committing
# them, you can test the changes with the following:
#
#   ./pnacl/llvm_change_try_helper.sh  # Commits a trybot-only change
#   git try
#   git reset --hard HEAD^  # Removes the trybot-only change


set -eux

if [ "$#" = 1 ]; then
  component="$1"
elif [ "$#" = 0 ]; then
  # Default
  component=llvm
else
  echo "Usage: $0 [<component-name>]"
  echo "where <component-name> is the name of a subdirectory of toolchain_build/src/"
  exit 1
fi

top_dir="$(pwd)"

mkdir -p pnacl/not_for_commit

pushd toolchain_build/src/${component}
# Save the commit ID plus its subject line to give readable context.
git log --no-walk --pretty="format:%H%n%ad%n%s" HEAD \
    > $top_dir/pnacl/not_for_commit/${component}_commit_id
git bundle create $top_dir/pnacl/not_for_commit/${component}_bundle \
    origin/master..HEAD
popd

python -c "import base64, sys; base64.encode(sys.stdin, sys.stdout)" \
    < pnacl/not_for_commit/${component}_bundle \
    > pnacl/not_for_commit/${component}_bundle.b64

git add pnacl/not_for_commit/${component}_bundle.b64
git add pnacl/not_for_commit/${component}_commit_id

git commit -m "${component} patch for trybot" \
    pnacl/not_for_commit/${component}_bundle.b64 \
    pnacl/not_for_commit/${component}_commit_id
