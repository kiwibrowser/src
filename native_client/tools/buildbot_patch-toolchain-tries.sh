#!/bin/bash
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cd "$(dirname "$0")"
for i in binutils gcc glibc linux-headers-for-nacl newlib; do
  (
    if [ -s "toolchain-try.$i.patch" ]; then
      echo "@@@BUILD_STEP $i try patch@@@"
      make "pinned-src-$i"
      cd "SRC/$i"
      patch -p1 < ../../"toolchain-try.$i.patch"
    fi
    rm -f ../../"toolchain-try.$i.patch"
  )
done
