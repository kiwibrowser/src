#!/bin/bash
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

readonly SCRIPT_DIR="$(dirname "$0")"
readonly SCRIPT_DIR_ABS="$(cd "${SCRIPT_DIR}" ; pwd)"

set -x
set -e
set -u

for dirname in binutils gcc glibc linux-headers-for-nacl newlib ; do
  if [[ -d "$SCRIPT_DIR_ABS/SRC/$dirname" ]]; then (
    cd "$SCRIPT_DIR_ABS/SRC/$dirname"
    (git reset --hard &&
     git clean -d -f -x &&
     ../../update_to_latest.sh) ||
    (cd .. &&
     rm -rf "$dirname" &&
     cd .. &&
     make pinned-src-"$dirname")
  ) fi
done
