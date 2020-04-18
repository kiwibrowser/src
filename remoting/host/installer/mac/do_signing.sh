#!/bin/sh

# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script signs the Chromoting binaries, builds the Chrome Remote Desktop
# installer and then packages it into a .dmg.  It requires that Packages be
# installed (for 'packagesbuild').
# Packages: http://s.sudre.free.fr/Software/Packages/about.html
#
# usage: do_signing.sh output_dir input_dir [codesign_keychain codesign_id
#            [productsign_id]]
#
# The final disk image (dmg) is placed in |output_dir|.

set -e -u

ME="$(basename "${0}")"
readonly ME

declare -a g_cleanup_dirs

setup() {
  local input_dir="${1}"

  # The file that contains the properties for this signing build.
  # The file should contain only key=value pairs, one per line.
  PROPS_FILENAME="${input_dir}/do_signing.props"

  # Individually load the properties for this build. Don't 'source' the file
  # to guard against code accidentally being added to the props file.
  DMG_VOLUME_NAME=$(read_property "DMG_VOLUME_NAME")
  DMG_FILE_NAME=$(read_property "DMG_FILE_NAME")
  HOST_BUNDLE_NAME=$(read_property "HOST_BUNDLE_NAME")
  HOST_PKG=$(read_property "HOST_PKG")
  HOST_UNINSTALLER_NAME=$(read_property "HOST_UNINSTALLER_NAME")
  NATIVE_MESSAGING_HOST_BUNDLE_NAME=$(read_property\
    "NATIVE_MESSAGING_HOST_BUNDLE_NAME")
  REMOTE_ASSISTANCE_HOST_BUNDLE_NAME=$(read_property\
    "REMOTE_ASSISTANCE_HOST_BUNDLE_NAME")

  # Binaries to sign.
  ME2ME_HOST="PrivilegedHelperTools/${HOST_BUNDLE_NAME}"
  ME2ME_NM_HOST="PrivilegedHelperTools/${HOST_BUNDLE_NAME}/Contents/MacOS/"`
                `"${NATIVE_MESSAGING_HOST_BUNDLE_NAME}/Contents/MacOS/"`
                `"native_messaging_host"
  IT2ME_NM_HOST="PrivilegedHelperTools/${HOST_BUNDLE_NAME}/Contents/MacOS/"`
                `"${REMOTE_ASSISTANCE_HOST_BUNDLE_NAME}/Contents/MacOS/"`
                `"remote_assistance_host"
  UNINSTALLER="Applications/${HOST_UNINSTALLER_NAME}.app"

  # The Chromoting Host installer is a meta-package that consists of 3
  # components:
  #  * Chromoting Host Service package
  #  * Chromoting Host Uninstaller package
  #  * Keystone package (GoogleSoftwareUpdate - for Official builds only)
  PKGPROJ_HOST="ChromotingHost.pkgproj"
  PKGPROJ_HOST_SERVICE="ChromotingHostService.pkgproj"
  PKGPROJ_HOST_UNINSTALLER="ChromotingHostUninstaller.pkgproj"

  # Final (user-visible) pkg name.
  PKG_FINAL="${HOST_PKG}.pkg"

  DMG_FILE_NAME="${DMG_FILE_NAME}.dmg"

  # Temp directory for Packages output.
  PKG_DIR=build
  g_cleanup_dirs+=("${PKG_DIR}")

  # Temp directories for building the dmg.
  DMG_TEMP_DIR="$(mktemp -d -t "${ME}"-dmg)"
  g_cleanup_dirs+=("${DMG_TEMP_DIR}")

  DMG_EMPTY_DIR="$(mktemp -d -t "${ME}"-empty)"
  g_cleanup_dirs+=("${DMG_EMPTY_DIR}")
}

err() {
  echo "[$(date +'%Y-%m-%d %H:%M:%S%z')]: ${@}" >&2
}

err_exit() {
  err "${@}"
  exit 1
}

# shell_safe_path ensures that |path| is safe to pass to tools as a
# command-line argument. If the first character in |path| is "-", "./" is
# prepended to it. The possibly-modified |path| is output.
shell_safe_path() {
  local path="${1}"
  if [[ "${path:0:1}" = "-" ]]; then
    echo "./${path}"
  else
    echo "${path}"
  fi
}

# Read a single property from the properties file.
read_property() {
  local property="${1}"
  local filename="${PROPS_FILENAME}"
  echo `grep "\<${property}\>=" "${filename}" | tail -n 1 | cut -d "=" -f2-`
}

verify_clean_dir() {
  local dir="${1}"
  if [[ ! -d "${dir}" ]]; then
    mkdir "${dir}"
  fi

  if [[ -e "${output_dir}/${DMG_FILE_NAME}" ]]; then
    err "Output directory is dirty from previous build."
    exit 1
  fi
}

sign() {
  local name="${1}"
  local keychain="${2}"
  local id="${3}"

  if [[ ! -e "${name}" ]]; then
    err_exit "Input file doesn't exist: ${name}"
  fi

  echo Signing "${name}"
  if [[ -n "${keychain}" ]]; then
    codesign -vv -s "${id}" --keychain "${keychain}" "${name}"
  else
    codesign -vv -s "${id}" "${name}"
  fi
  codesign -v "${name}"
}

sign_binaries() {
  local input_dir="${1}"
  local keychain="${2}"
  local id="${3}"

  sign "${input_dir}/${ME2ME_NM_HOST}" "${keychain}" "${id}"
  sign "${input_dir}/${IT2ME_NM_HOST}" "${keychain}" "${id}"
  sign "${input_dir}/${ME2ME_HOST}" "${keychain}" "${id}"
  sign "${input_dir}/${UNINSTALLER}" "${keychain}" "${id}"
}

sign_installer() {
  local input_dir="${1}"
  local keychain="${2}"
  local id="${3}"

  local package="${input_dir}/${PKG_DIR}/${PKG_FINAL}"
  if [[ -n "${keychain}" ]]; then
    productsign --sign "${id}" --keychain "${keychain}" \
        "${package}" "${package}.signed"
  else
    productsign --sign "${id}" "${package}" "${package}.signed"
  fi
  mv -f "${package}.signed" "${package}"
}

build_package() {
  local pkg="${1}"
  echo "Building .pkg from ${pkg}"
  packagesbuild -v "${pkg}"
}

build_packages() {
  local input_dir="${1}"
  build_package "${input_dir}/${PKGPROJ_HOST_SERVICE}"
  build_package "${input_dir}/${PKGPROJ_HOST_UNINSTALLER}"
  build_package "${input_dir}/${PKGPROJ_HOST}"
}

build_dmg() {
  local input_dir="${1}"
  local output_dir="${2}"

  # Create the .dmg.
  echo "Building .dmg..."
  "${input_dir}/pkg-dmg" \
      --format UDBZ \
      --tempdir "${DMG_TEMP_DIR}" \
      --source "${DMG_EMPTY_DIR}" \
      --target "${output_dir}/${DMG_FILE_NAME}" \
      --volname "${DMG_VOLUME_NAME}" \
      --copy "${input_dir}/${PKG_DIR}/${PKG_FINAL}" \
      --copy "${input_dir}/Scripts/keystone_install.sh:/.keystone_install"

  if [[ ! -f "${output_dir}/${DMG_FILE_NAME}" ]]; then
    err_exit "Unable to create disk image: ${DMG_FILE_NAME}"
  fi
}

cleanup() {
  if [[ "${#g_cleanup_dirs[@]}" > 0 ]]; then
    rm -rf "${g_cleanup_dirs[@]}"
  fi
}

usage() {
  echo "Usage: ${ME} -o output_dir -i input_dir "\
      "[-c codesign_id] [-p productsign_id] [-k keychain]" >&2
  echo >&2
  echo "Usage (legacy): ${ME} output_dir input_dir [keychain codesign_id"\
      "[productsign_id]]" >&2
  echo >&2
  echo "  Sign the binaries using the specified <codesign_id>, build" >&2
  echo "  the installer, and then sign the installer using the given" >&2
  echo "  <productsign_id>." >&2
  echo "  If the signing ids are not specified then the" >&2
  echo "  installer is built without signing any binaries." >&2
  echo "  If <keychain> is specified, it must contain all the signing ids." >&2
  echo "  If not specified, then the default keychains will be used." >&2
}

main() {
  local output_dir=""
  local input_dir=""
  local do_sign_binaries=0
  local codesign_id=""
  local productsign_id=""
  local keychain=""

  local opt_count=${#}
  local OPTNAME OPTIND OPTARG
  while getopts ":o:i:c:p:k:h" OPTNAME; do
    case ${OPTNAME} in
      o )
        output_dir="$(shell_safe_path "${OPTARG}")"
        ;;
      i )
        input_dir="$(shell_safe_path "${OPTARG}")"
        ;;
      c )
        codesign_id="${OPTARG}"
        ;;
      p )
        productsign_id="${OPTARG}"
        ;;
      k )
        keychain="$(shell_safe_path "${OPTARG}")"
        ;;
      h )
        usage
        exit 0
        ;;
      * )
        err "Invalid command-line option: ${OPTARG}"
        usage
        exit 1
        ;;
    esac
  done
  shift $(($OPTIND - 1))

  # The opts need to be either all "flag-style" or all "position-style", not
  # both. If there are any leftover opts at this point, it should be all the
  # original ones (i.e. all positional).
  # TODO(mmoss): The positional handling is legacy and can go away once the
  # signing system has been migrated to using flags, which provides more
  # flexibility for adding/removing opts if needed. b/31931170
  if [[ ${#} -ne 0 ]]; then
    if [[ ${opt_count} -ne ${#} ]]; then
      err "Please use all flag opts, or all positional opts, not both."
      usage
      exit 1
    fi
    if [[ ${#} < 2 ]]; then
      err "Too few positional opts."
      usage
      exit 1
    fi
    output_dir="$(shell_safe_path "${1}")"
    input_dir="$(shell_safe_path "${2}")"
    keychain=""
    if [[ ${#} -ge 3 ]]; then
      keychain="$(shell_safe_path "${3}")"
    fi
    codesign_id=""
    if [[ ${#} -ge 4 ]]; then
      codesign_id="${4}"
    fi
    productsign_id=""
    if [[ ${#} -ge 5 ]]; then
      productsign_id="${5}"
    fi
  fi

  if [[ -z "${output_dir}" || -z "${input_dir}" ]]; then
    err "output_dir and input_dir are required."
    usage
    exit 1
  fi

  # There's no point in specifying a keychain but no identity, so it's probably
  # being called incorrectly.
  if [[ -n "${keychain}" && -z "${codesign_id}" ]]; then
    err_exit "Can't use keychain without a codesign_id"
  fi

  if [[ -n "${codesign_id}" ]]; then
    do_sign_binaries=1
    echo "Signing binaries with identity: ${codesign_id}"
    if [[ -n "${productsign_id}" ]]; then
      echo "Signing installer with identity: ${productsign_id}"
    fi
    if [[ -n "${keychain}" ]]; then
      echo "Using keychain: ${keychain}"
    fi
  else
    echo "Not signing binaries (no identify specified)"
  fi

  setup "${input_dir}"
  verify_clean_dir "${output_dir}"

  if [[ "${do_sign_binaries}" == 1 ]]; then
    sign_binaries "${input_dir}" "${keychain}" "${codesign_id}"
  fi
  build_packages "${input_dir}"
  if [[ "${do_sign_binaries}" == 1 && -n "${productsign_id}" ]]; then
    echo "Signing installer..."
    sign_installer "${input_dir}" "${keychain}" "${productsign_id}"
  fi
  build_dmg "${input_dir}" "${output_dir}"

  cleanup
}

main "${@}"
exit ${?}
