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

if ! cygcheck -V >/dev/null 2>/dev/null; then
  echo "No cygcheck found"
  exit 1
fi

echo "WARNING: hang can occur on FAT, use NTFS"


# Redirectors on windows:
#   NaCl tools are cygwin programs. To run, they need cygwin DLLs of the same
#   or later version of cygwin they were compiled with.
#
#   To avoid requiring users to install or upgrade cygwin, we couple the tools
#   with the corresponding DLLs. We place these DLLs in every directory where
#   the tools are, so that windows algorithm for locating DLLs pick DLLs
#   provided by us.
#
#   Unfortunately, when cygwin program is forked/execed by another cygwin
#   program, both are required to use the same version of cygwin DLLs. The
#   common case is when user starts a tool from cygwin bash.
#
#   This is solved by hiding actual tools under /libexec (which is a directory
#   for programs to be run by other programs rather than by users) and providing
#   trivial non-cygwin redirectors (launchers) for these tools.
#
# Symbolic links vs. hard links:
#   On windows/cygwin, hard links are needed to run linked programs outside of
#   the cygwin shell. On *nix, there is no usage difference.
#   Here we handle only the windows/cygwin case and use the hard links.


# Destination of a redirector is always under /libexec. When redirector source
# name is equal to redirector destination name, it means the source is an actual
# tool to be hidden under /libexec.
#
# Redirector source can be updated after the redirector was created. Overwrite
# the destination in this case.

./redirector.exe | while IFS='|' read src dst arg; do
  if [[ -e "$prefix/$src" ]]; then
    if ! cmp -s ./redirector.exe "$prefix/$src"; then
      if [[ "$(basename "$src")" = "$(basename "$dst")" ]]; then
        mv -f "$prefix/$src" "$prefix/$dst"
      fi
    fi
  fi
done

# Install redirectors for existing redirector destinations.

./redirector.exe | while IFS='|' read src dst arg; do
  if [[ -e "$prefix/$dst" ]]; then
    ln -fn ./redirector.exe "$prefix/$src"
  fi
done


# Inject DLLs:
#   get list of (directory, dll):
#     for each exe:
#       run cygcheck to get list of DLLs
#     keep unique pairs only
#   for each (directory, dll):
#     if dll is from /usr/bin:
#       add link to the dll in the directory
#
# We dump all DLLs and filter /usr/bin DLLs later to save cygpath calls.

find "$prefix" -name "*.exe" -print0 | while read -r -d $'\0' exe; do
  dir="$(dirname "$exe")"
  win_exe="$(cygpath -w "$exe")"
  cd "$dir" && cygcheck "$win_exe" | while read -r win_dll; do
    echo "$dir:$win_dll"
  done
done | sort -u | while IFS=':' read -r dir win_dll; do
  dll="$(cygpath "$win_dll")"
  if [[ "$dll" = /usr/bin/*.dll ]]; then
    ln -fn "$dll" "$dir"
  fi
done
