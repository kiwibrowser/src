#!/bin/bash
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -Ceu


prefix="$1"

if [[ ! -d "$prefix" ]]; then
  echo "Usage: $0 toolchain-prefix"
  exit 1
fi


# Redirectors on *nix:
#   The purpose of redirectors on *nix is to provide convenience wrappers, for
#   example to emulate 32-bit compiler by calling 64-bit compiler with -m32.
#   Another purpose of redirectors is to save space by replacing duplicate
#   binaries with wrappers or links.
#
# Symbolic links vs. hard links:
#   On windows/cygwin, hard links are needed to run linked programs outside of
#   the cygwin shell. On *nix, there is no usage difference.
#   Here we handle only the *nix case and use the symbolic links.


while read src dst arg; do
  if [[ -e "$prefix/$(dirname "$src")/$dst" ]]; then
    ./create_redirector.sh "$prefix/$src" "$dst" "$arg"
  fi
done < redirect_table.txt
