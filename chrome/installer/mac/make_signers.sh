#!/bin/bash -p

# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script creates sign_app.sh and sign_versioned_dir.sh, the scripts that
# will be used to sign the application bundle and inner bundles. It also
# creates auxiliary files that these scripts need to do their jobs, such as
# the custom resource rules used to sign the outermost application bundle.
# The build places these in the "${mac_product_name} Packaging" directory next
# to the .app bundle. The packaging system is expected to run these scripts to
# sign everything.

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

if [[ ${#} -ne 3 ]]; then
  echo "usage: ${ME} packaging_dir mac_product_name version" >& 2
  exit 1
fi

packaging_dir="${1}"
mac_product_name="${2}"
version="${3}"

script_dir="$(dirname "${0}")"
in_files=(
  "${script_dir}/sign_app.sh.in"
  "${script_dir}/sign_versioned_dir.sh.in"
  "${script_dir}/app_resource_rules.plist.in"
)

# Double-backslash each dot: one backslash belongs in the regular expression,
# and the other backslash tells sed not to treat the first backslash
# specially.
version_regex="$(echo "${version}" | sed -e 's/\./\\\\./g')"

mkdir -p "${packaging_dir}"

for in_file in "${in_files[@]}"; do
  out_file="${packaging_dir}/$(basename "${in_file:0:${#in_file} - 3}")"
  sed -e "s/@MAC_PRODUCT_NAME@/${mac_product_name}/g" \
      -e "s/@VERSION@/${version}/g" \
      -e "s/@VERSION_REGEX@/${version_regex}/g" \
      < "${in_file}" \
      > "${out_file}"

  if [[ "${out_file: -3}" = ".sh" ]]; then
    chmod +x "${out_file}"
  fi
done
