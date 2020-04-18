#!/bin/bash -p

# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Using codesign, sign the installer tools. After signing, the signatures are
# verified.

set -eu

# Environment sanitization. Set a known-safe PATH. Clear environment variables
# that might impact the interpreter's operation. The |bash -p| invocation
# on the #! line takes the bite out of BASH_ENV, ENV, and SHELLOPTS (among
# other features), but clearing them here ensures that they won't impact any
# shell scripts used as utility programs. SHELLOPTS is read-only and can't be
# unset, only unexported.
export PATH="/usr/bin:/bin:/usr/sbin:/sbin"
unset BASH_ENV CDPATH ENV GLOBIGNORE IFS POSIXLY_CORRECT
export -n SHELLOPTS

ME="$(basename "${0}")"
readonly ME

if [[ ${#} -ne 3 && ${#} -ne 4 ]]; then
  echo "usage: ${ME} packaging_dir codesign_keychain codesign_id \
[--development]" >& 2
  exit 1
fi

packaging_dir="${1}"
codesign_keychain="${2}"
codesign_id="${3}"
is_development=

if [[ ${#} == 4 && ${4} == "--development" ]]; then
  is_development=1
fi

script_dir="$(dirname "${0}")"
source "${script_dir}/variables.sh"

executables=(goobspatch xzdec)
libraries=(liblzma_decompress.dylib)
declare -a everything

for executable in "${executables[@]}"; do
  sign_path="${packaging_dir}/${executable}"
  everything+=("${sign_path}")

  codesign_cmd=(
    codesign --sign "${codesign_id}" --keychain "${codesign_keychain}"
    "${sign_path}" --options "${enforcement_flags_installer_tools}"
  )

  if [[ -z "${is_development}" ]]; then
    requirement="designated => identifier \"${executable}\" \
${requirement_suffix}"
    codesign_cmd+=( -r="${requirement}" )
  fi

  "${codesign_cmd[@]}"
done

for library in "${libraries[@]}"; do
  sign_path="${packaging_dir}/${library}"
  everything+=("${sign_path}")

  codesign --sign "${codesign_id}" --keychain "${codesign_keychain}" \
      "${sign_path}"
done

for sign_path in "${everything[@]}"; do
  codesign --verify --deep -vvvvvv "${sign_path}"
done
