#!/bin/bash
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

DIR="$(dirname "$0")"
BINDIR="$DIR/../../../../third_party/valgrind/binaries/linux_x64/bin"
NACL_DANGEROUS_SKIP_QUALIFICATION_TEST=1 \
NACLENV_LD_PRELOAD=$BINDIR/../lib/vgpreload_memcheck-amd64-linux.so \
"$BINDIR/valgrind" --tool=memcheck --suppressions="$DIR/nacl.supp" "$@"
