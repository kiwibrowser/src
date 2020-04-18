#!/bin/sh

# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -e -u

ME="$(basename "$0")"
readonly ME

KSADMIN=/Library/Google/GoogleSoftwareUpdate/GoogleSoftwareUpdate.bundle/Contents/MacOS/ksadmin
KSPID=com.google.chrome_remote_desktop

usage() {
  echo "Usage: ${ME} <channel>" >&2
  echo "where <channel> is 'beta' or 'stable'" >&2
}

log() {
  local message="$1"
  echo "${message}"
  logger "${message}"
}

checkroot() {
  if [[ "$(id -u)" != "0" ]]; then
     echo "This script requires root permissions" 1>&2
     exit 1
  fi
}

main() {
  local channel="$1"

  if [[ "${channel}" != "beta" && "${channel}" != "stable" ]]; then
    usage
    exit 1
  fi

  local channeltag="${channel}"
  if [[ "${channel}" == "stable" ]]; then
    channeltag=""
  fi

  log "Switching Chrome Remote Desktop channel to ${channel}"

  $KSADMIN --productid "$KSPID" --tag "${channeltag}"

  if [[ "${channel}" == "stable" ]]; then
    echo "You're not done yet!"
    echo "You must now UNINSTALL and RE-INSTALL the latest version of Chrome"
    echo "Remote Desktop to get your machine back on the stable channel."
    echo "Thank you!"
  else
    echo "Switch to ${channel} channel complete."
    echo "You will download ${channel} binaries during the next update check."
  fi
}

checkroot

if [[ $# < 1 ]]; then
  usage
  exit 1
fi

main "$@"
