#!/usr/bin/env bash
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Check that:
#  - all C++ and header files conform to clang-format style.
#  - all header files have appropriate include guards.
#  - all GN files are formatted with `gn format`.
fail=0

function check_clang_format() {
  if ! cmp -s <(clang-format -style=file "$1") "$1"; then
    echo "Needs format: $1"
    fail=1
  fi
}

function check_include_guard() {
  # Replace all folder slashes with underscores, and add "_" suffix.
  guard_name=${1//[\/\.]/_}_

  # This to-uppercase syntax is available in bash 4.0+
  guard_name=${guard_name^^}

  ifndef_count=$(grep -E "^#ifndef $guard_name\$" "$1" | wc -l)
  define_count=$(grep -E "^#define $guard_name\$" "$1" | wc -l)
  endif_count=$(grep -E "^#endif  // $guard_name\$" "$1" | wc -l)
  if [ $ifndef_count -ne 1 -o $define_count -ne 1 -o \
       $endif_count -ne 1 ]; then
    echo "Include guard missing/incorrect: $1"
    fail=1
  fi
}

function check_gn_format() {
  if ! which gn &>/dev/null; then
    echo "Please add gn to your PATH or manually check $1 for format errors."
  else
    if ! cmp -s <(cat "$1" | gn format --stdin) "$1"; then
      echo "Needs format: $1"
      fail=1
    fi
  fi
}

if [[ "${BASH_VERSION:0:1}" -lt 4 ]]; then
  echo "This script requires at least bash version 4.0, please upgrade!"
  echo "Your version: " $BASH_VERSION
  exit $fail
fi

for f in $(git diff --name-only --diff-filter=d @{u}); do
  # Skip third party files, except our custom BUILD.gns
  if [[ $f =~ third_party/[^\/]*/src ]]; then
    continue;
  fi

  # Skip statically copied Chromium QUIC build files.
  if [[ $f =~ third_party/chromium_quic/build ]]; then
    continue;
  fi

  if [[ $f =~ \.(cc|h)$ ]]; then
    # clang-format check.
    check_clang_format "$f"
    # Include guard check.
    if [[ $f =~ \.h$ ]]; then
      check_include_guard "$f"
    fi
  elif [[ $f =~ \.gn(i)?$ ]]; then
    check_gn_format "$f"
  fi
done

exit $fail
